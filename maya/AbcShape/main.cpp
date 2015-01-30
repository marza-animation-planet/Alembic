#include <maya/MFnPlugin.h>
#include <maya/MDrawRegistry.h>
#include "AbcShape.h"
#include "AbcShapeImport.h"
#include "VP2.h"

#ifndef ABCSHAPE_VERSION
#  define ABCSHAPE_VERSION "1.0"
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
   
   MString vrayCmdName = PREFIX_NAME("AbcShapeVRayDisp");
   
   status = plugin.registerCommand(vrayCmdName, AbcShapeVRayDisp::create, AbcShapeVRayDisp::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command '" + vrayCmdName + "'");
      return status;
   }
   
#endif

   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

   MString nodeName = PREFIX_NAME("AbcShape");
   MString commandName = PREFIX_NAME("AbcShapeImport");

   if (AbcShape::CallbackID != 0)
   {
      MDGMessage::removeCallback(AbcShape::CallbackID);
      AbcShape::CallbackID = 0;
   }
   
   MStatus status = plugin.deregisterCommand(commandName);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command '" + commandName + "'");
      return status;
   }

#ifdef ABCSHAPE_VRAY_SUPPORT
   
   MString vrayCmdName = PREFIX_NAME("AbcShapeVRayDisp");
   
   status = plugin.deregisterCommand(vrayCmdName);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command '" + vrayCmdName + "'");
      return status;
   }
   
#endif

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
