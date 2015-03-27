#include <maya/MFnPlugin.h>
#include <maya/MDrawRegistry.h>
#include "AbcShape.h"
#include "AbcShapeImport.h"
#include "VP2.h"

#ifndef ABCSHAPE_VERSION
#  define ABCSHAPE_VERSION "1.0"
#endif

#ifdef ABCSHAPE_VRAY_SUPPORT

#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MGlobal.h>
#include <map>
#include <string>

MCallbackId gRenderGlobalsCreated = 0;
MCallbackId gRenderGlobalsDeleted = 0;
std::map<std::string, MCallbackId> gRenderGlobalsAttrChanged;

MCallbackId gVRaySettingsCreated = 0;
MCallbackId gVRaySettingsDeleted = 0;
std::map<std::string, MCallbackId> gVRaySettingsAttrChanged;

std::string gPreMel = NAME_PREFIX "AbcShapeVRayInfo -init";
std::string gPostPythonImp = "import " NAME_PREFIX "abcshape4vray";
std::string gPostPythonExc = NAME_PREFIX "abcshape4vray.PostTranslate()";
std::string gPostPython = gPostPythonImp + "; " + gPostPythonExc;

bool gCurrentRendererIsVRay = false;


#define AttrName(plug) plug.partialName(false, true, true, false, true, true)

static void StripLTWS(std::string &s)
{
   size_t p = s.find_first_not_of(" \t\n");
   
   if (p == std::string::npos)
   {
      s = "";
   }
   else
   {
      if (p > 0)
      {
         s = s.substr(p);
      }
      
      p = s.find_last_not_of(" \t\n");
      
      if (p != std::string::npos)
      {
         s = s.substr(0, p + 1);
      }
   }
}

static bool MelCompat(std::string &s)
{
   size_t p = s.find("AbcShapeVRayDisp");
   
   if (p != std::string::npos)
   {
      s = s.substr(0, p) + "AbcShapeVRayInfo" + s.substr(p + 16);
      return true;
   }
   else
   {
      return false;
   }
}

static bool PythonCompat(std::string &s)
{
   size_t p = s.find("abcshape4vray.CreateDispTextures");
   
   if (p != std::string::npos)
   {
      s = s.substr(0, p) + "abcshape4vray.PostTranslate" + s.substr(p + 32);
      return true;
   }
   else
   {
      return false;
   }
}

static void AppendMel(std::string &mel0, std::string &mel1)
{
   StripLTWS(mel0);
   StripLTWS(mel1);
   
   size_t l0 = mel0.length();
   size_t l1 = mel1.length();
   
   if (l1 > 0)
   {
      if (l0 > 0)
      {
         if (mel0[l0-1] == ';')
         {
            mel0 += " ";
         }
         else
         {
            mel0 += "; ";
         }
      }
      
      mel0 += mel1;
      
      l0 = mel0.length();
   }
   
   if (l0 > 0 && mel0[l0-1] != ';')
   {
      mel0 += ";";
   }
}

static bool RemoveMel(std::string &mel, const std::string &s)
{
   size_t p = mel.find(s);
   
   if (p != std::string::npos)
   {
      size_t len = s.length();
      
      std::string before = mel.substr(0, p);
      std::string after = mel.substr(p + len);
      
      StripLTWS(before);
      StripLTWS(after);
      
      size_t bl = before.length();
      size_t al = after.length();
      
      mel = before;
      
      if (al > 0)
      {
         // may start with ';'
         if (after[0] == ';')
         {
            if (bl > 0 && before[bl-1] != ';')
            {
               // keep heading ';'
               mel += after;
            }
            else
            {
               // remove heading ';'
               mel += after.substr(1);
            }
         }
         else
         {
            if (bl == 0 || before[bl-1] == ';')
            {
               // dont need to add ';'
               mel += after;
            }
            else
            {
               mel += "; " + after;
            }
         }
      }
      
      len = mel.length();
         
      if (len > 0 && mel[len-1] != ';')
      {
         mel += ";";
      }
      
      return true;
   }
   else
   {
      return false;
   }
}

static void AppendPython(std::string &py0, std::string &py1)
{
   StripLTWS(py0);
   StripLTWS(py1);
   
   size_t l0 = py0.length();
   size_t l1 = py1.length();
   
   if (l1 > 0)
   {
      if (l0 > 0)
      {
         if (py0[l0-1] == ';')
         {
            py0[l0-1] = '\n';
         }
         else
         {
            py0 += "\n";
         }
      }
      
      py0 += py1;
      
      l0 = py0.length();
   }
   
   if (l0 > 0 && py0[l0-1] == ';')
   {
      py0 = py0.substr(0, l0-1);
   }
}

static bool RemovePython(std::string &py, const std::string &s)
{
   size_t p = py.find(s);
      
   if (p != std::string::npos)
   {
      size_t len = s.length();
      
      std::string before = py.substr(0, p);
      std::string after = py.substr(p + len);
      
      StripLTWS(before);
      StripLTWS(after);
      
      size_t bl = before.length();
      size_t al = after.length();
      
      py = before;
      
      if (al > 0)
      {
         if (after[0] == ';')
         {
            after = after.substr(1);
         }
         
         if (bl > 0)
         {
            if (py[bl-1] == ';')
            {
               py[bl-1] = '\n';
            }
            else
            {
               py += "\n";
            }
         }
         
         py += after;
      }
      
      len = py.length();
      
      if (len > 0 && py[len-1] == ';')
      {
         py = py.substr(0, len-1);
      }
      
      return true;
   }
   else
   {
      return false;
   }
}

static void EnsurePreMel(MPlug &plug)
{
   std::string preMel = plug.asString().asChar();
   
   // For backward compatibility, replace old command name first
   bool changed = MelCompat(preMel);
   
   size_t p = preMel.find(gPreMel);
   
   if (p == std::string::npos)
   {
      AppendMel(preMel, gPreMel);
      changed = true;
   }
   
   if (changed)
   {
      plug.setString(preMel.c_str());
   }
}

static void RemovePreMel(MPlug &plug)
{
   std::string preMel = plug.asString().asChar();
   
   MelCompat(preMel);
   
   if (RemoveMel(preMel, gPreMel))
   {
      plug.setString(preMel.c_str());
   }
}

static void EnsurePostTranslatePython(MPlug &plug)
{
   std::string postPython = plug.asString().asChar();
   
   // For backward compatibility, replace old function call
   bool changed = PythonCompat(postPython);
   
   size_t p = postPython.find(gPostPythonExc);
   
   if (p == std::string::npos)
   {
      p = postPython.find(gPostPythonImp);
      
      if (p == std::string::npos)
      {
         AppendPython(postPython, gPostPython);
      }
      else
      {
         // missing PostTranslate call
         std::string before = postPython.substr(0, p + gPostPythonImp.length());
         std::string after = postPython.substr(p + gPostPythonImp.length());
         
         postPython = before;
         
         AppendPython(postPython, gPostPythonExc);
         AppendPython(postPython, after);
      }
      
      changed = true;
   }
   else
   {
      if (postPython.find(gPostPythonImp) == std::string::npos)
      {
         // missing import statement
         std::string before = postPython.substr(0, p);
         std::string after = postPython.substr(p);
         
         postPython = before;
         
         AppendPython(postPython, gPostPythonImp);
         AppendPython(postPython, after);
         
         changed = true;
      }
   }
   
   if (changed)
   {
      plug.setString(postPython.c_str());
   }
}

static void RemovePostTranslatePython(MPlug &plug)
{
   std::string postPython = plug.asString().asChar();
   
   PythonCompat(postPython);
   
   bool changed = RemovePython(postPython, gPostPythonImp);
   
   if (RemovePython(postPython, gPostPythonExc))
   {
      changed = true;
   }
   
   if (changed)
   {
      plug.setString(postPython.c_str());
   }
}

static void CurrentRendererChanged(const MString &name, MObject drg=MObject::kNullObj, MObject vrs=MObject::kNullObj)
{
   gCurrentRendererIsVRay = (name == "vray");
   
   MSelectionList sl;
   MObject obj;
   
   // Should actually do that for all 'renderGlobals' nodes
   if (!drg.isNull() ||
       (sl.add("defaultRenderGlobals") == MS::kSuccess &&
        sl.getDependNode(sl.length() - 1, obj) == MS::kSuccess))
   {
      MFnDependencyNode node(drg.isNull() ? obj : drg);
      
      MPlug plug = node.findPlug("preMel");
      
      if (!plug.isNull())
      {
         if (gCurrentRendererIsVRay)
         {
            EnsurePreMel(plug);
         }
         else
         {
            RemovePreMel(plug);
         }
      }
   }
   
   // Should actually do that for all 'VRaySettingsNode' nodes
   if (!vrs.isNull() ||
       (sl.add("vraySettings") == MS::kSuccess &&
        sl.getDependNode(sl.length() - 1, obj) == MS::kSuccess))
   {
      MFnDependencyNode node(vrs.isNull() ? obj : vrs);
      
      MPlug plug = node.findPlug("postTranslatePython");
      
      if (!plug.isNull())
      {
         if (gCurrentRendererIsVRay)
         {
            EnsurePostTranslatePython(plug);
         }
         else
         {
            RemovePostTranslatePython(plug);
         }
      }
   }
}

static void RenderGlobalsAttrChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *)
{
   MString attr = AttrName(plug);
   
   if ((msg & MNodeMessage::kAttributeSet) != 0)
   {
      if (attr == "currentRenderer")
      {
         MObject obj = plug.node();
         
         CurrentRendererChanged(plug.asString(), obj);
      }
      else if (attr == "preMel")
      {
         if (gCurrentRendererIsVRay)
         {
            EnsurePreMel(plug);
         }
      }
   }
}

static void VRaySettingsAttrChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void *)
{
   MString attr = AttrName(plug);
   
   if ((msg & MNodeMessage::kAttributeSet) != 0)
   {
      if (attr == "postTranslatePython")
      {
         if (gCurrentRendererIsVRay)
         {
            EnsurePostTranslatePython(plug);
         }
      }
   }
}

static void RenderGlobalsDeleted(MObject &obj, void *)
{
   MFnDependencyNode node(obj);
   
   std::string key = node.name().asChar();
   
   std::map<std::string, MCallbackId>::iterator it = gRenderGlobalsAttrChanged.find(key);
   
   if (it != gRenderGlobalsAttrChanged.end())
   {
      MMessage::removeCallback(it->second);
      gRenderGlobalsAttrChanged.erase(it);
   }
}

static void RenderGlobalsCreated(MObject &obj, void *)
{
   MStatus stat;
   MFnDependencyNode node(obj);
   
   std::string key = node.name().asChar();
   
   MCallbackId id = MNodeMessage::addAttributeChangedCallback(obj, RenderGlobalsAttrChanged, 0, &stat);
   
   if (stat != MS::kSuccess)
   {
      MGlobal::displayWarning("[AbcShape] Failed to register attribute changed callback");
   }
   else
   {
      gRenderGlobalsAttrChanged[key] = id;
   }
}

static void VRaySettingsDeleted(MObject &obj, void *)
{
   MFnDependencyNode node(obj);
   
   std::string key = node.name().asChar();
   
   std::map<std::string, MCallbackId>::iterator it = gVRaySettingsAttrChanged.find(key);
   
   if (it != gVRaySettingsAttrChanged.end())
   {
      MMessage::removeCallback(it->second);
      gVRaySettingsAttrChanged.erase(it);
   }
}

static void VRaySettingsCreated(MObject &obj, void *)
{
   MStatus stat;
   MFnDependencyNode node(obj);
   
   std::string key = node.name().asChar();
   
   MCallbackId id = MNodeMessage::addAttributeChangedCallback(obj, VRaySettingsAttrChanged, 0, &stat);
   
   if (stat != MS::kSuccess)
   {
      MGlobal::displayWarning("[AbcShape] Failed to register attribute changed callback");
   }
   else
   {
      gVRaySettingsAttrChanged[key] = id;
   }
}

static void InitializeCallbacks()
{
   MSelectionList sl;
   MObject drg, vrs;
   MStatus stat;
   
   gRenderGlobalsCreated = MDGMessage::addNodeAddedCallback(RenderGlobalsCreated, "renderGlobals", 0, &stat);
      
   if (stat != MS::kSuccess)
   {
      MGlobal::displayWarning("[AbcShape] Failed to register 'renderGlobals' created callback");
   }
   
   gRenderGlobalsDeleted = MDGMessage::addNodeRemovedCallback(RenderGlobalsDeleted, "renderGlobals", 0, &stat);
   
   if (stat != MS::kSuccess)
   {
      MGlobal::displayWarning("[AbcShape] Failed to register 'renderGlobals' deelted callback");
   }
   
   gVRaySettingsCreated = MDGMessage::addNodeAddedCallback(VRaySettingsCreated, "VRaySettingsNode", 0, &stat);
   
   if (stat != MS::kSuccess)
   {
      MGlobal::displayWarning("[AbcShape] Failed to register 'VRaySettingsNode' created callback");
   }
   
   gVRaySettingsDeleted = MDGMessage::addNodeRemovedCallback(VRaySettingsDeleted, "VRaySettingsNode", 0, &stat);
   
   if (stat != MS::kSuccess)
   {
      MGlobal::displayWarning("[AbcShape] Failed to register 'VRaySettingsNode' deleted callback");
   }
   
   // Initial setup
   
   MString renderer = "dummy";
   
   if (sl.add("vraySettings") == MS::kSuccess &&
       sl.getDependNode(sl.length() - 1, vrs) == MS::kSuccess)
   {
      VRaySettingsCreated(vrs, 0);
   }
   
   if (sl.add("defaultRenderGlobals") == MS::kSuccess &&
       sl.getDependNode(sl.length() - 1, drg) == MS::kSuccess)
   {
      RenderGlobalsCreated(drg, 0);
      
      MFnDependencyNode node(drg);   
      MPlug plug = node.findPlug("currentRenderer");
   
      if (!plug.isNull())
      {
         renderer = plug.asString();
      }
   }
   
   CurrentRendererChanged(renderer, drg, vrs);
}

static void UninitializeCallbacks()
{
   if (gRenderGlobalsCreated)
   {
      MMessage::removeCallback(gRenderGlobalsCreated);
      gRenderGlobalsCreated = 0;
   }
   
   if (gRenderGlobalsDeleted)
   {
      MMessage::removeCallback(gRenderGlobalsDeleted);
      gRenderGlobalsDeleted = 0;
   }
   
   if (gVRaySettingsCreated)
   {
      MMessage::removeCallback(gVRaySettingsCreated);
      gVRaySettingsCreated = 0;
   }
   
   if (gVRaySettingsDeleted)
   {
      MMessage::removeCallback(gVRaySettingsDeleted);
      gVRaySettingsDeleted = 0;
   }
   
   std::map<std::string, MCallbackId>::iterator it;
   
   for (it=gRenderGlobalsAttrChanged.begin(); it!=gRenderGlobalsAttrChanged.end(); ++it)
   {
      MMessage::removeCallback(it->second);
   }
   gRenderGlobalsAttrChanged.clear();
   
   for (it=gVRaySettingsAttrChanged.begin(); it!=gVRaySettingsAttrChanged.end(); ++it)
   {
      MMessage::removeCallback(it->second);
   }
   gVRaySettingsAttrChanged.clear();
   
   // Cleanup
   
   CurrentRendererChanged("dummy");
}

#endif

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
   MString nodeName = PREFIX_NAME("AbcShape");
   MString commandName = PREFIX_NAME("AbcShapeImport");
   
   MFnPlugin plugin(obj, nodeName.asChar(), ABCSHAPE_VERSION, "Any");

   MStatus status = plugin.registerShape(nodeName,
                                         AbcShape::ID,
                                         AbcShape::creator,
                                         AbcShape::initialize,
                                         AbcShapeUI::creator,
                                         &AbcShapeOverride::Classification);
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register shape '" + nodeName + "'");
      return status;
   }
   
   status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(AbcShapeOverride::Classification,
                                                                  AbcShapeOverride::Registrant,
                                                                  AbcShapeOverride::create);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register draw override for '" + nodeName + "'");
      return status;
   }
   
   status = plugin.registerCommand(commandName, AbcShapeImport::create, AbcShapeImport::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command '" + commandName + "'");
      return status;
   }

#ifdef ABCSHAPE_VRAY_SUPPORT
   
   MString vrayCmdName = PREFIX_NAME("AbcShapeVRayInfo");
   
   status = plugin.registerCommand(vrayCmdName, AbcShapeVRayInfo::create, AbcShapeVRayInfo::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command '" + vrayCmdName + "'");
      return status;
   }
   
   // For backward compatibility
   MString oldVrayCmdName = PREFIX_NAME("AbcShapeVRayDisp");
   
   status = plugin.registerCommand(oldVrayCmdName, AbcShapeVRayInfo::create, AbcShapeVRayInfo::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command '" + oldVrayCmdName + "'");
      return status;
   }
   
   InitializeCallbacks();
   
#endif

   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);
   
   MStatus status;
   MString nodeName = PREFIX_NAME("AbcShape");
   MString commandName = PREFIX_NAME("AbcShapeImport");
   
#ifdef ABCSHAPE_VRAY_SUPPORT
   
   UninitializeCallbacks();
   
   MString oldVrayCmdName = PREFIX_NAME("AbcShapeVRayDisp");
   
   status = plugin.deregisterCommand(oldVrayCmdName);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command '" + oldVrayCmdName + "'");
      return status;
   }
   
   MString vrayCmdName = PREFIX_NAME("AbcShapeVRayInfo");
   
   status = plugin.deregisterCommand(vrayCmdName);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command '" + vrayCmdName + "'");
      return status;
   }
   
#endif
   
   status = plugin.deregisterCommand(commandName);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command '" + commandName + "'");
      return status;
   }

   status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(AbcShapeOverride::Classification,
                                                                    AbcShapeOverride::Registrant);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister draw override for '" + nodeName + "'");
      return status;
   }
   
   status = plugin.deregisterNode(AbcShape::ID);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister shape '" + nodeName + "'");
      return status;
   }
   
   return status;
}
