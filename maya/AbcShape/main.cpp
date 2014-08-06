#include <maya/MFnPlugin.h>
#include <maya/MDrawRegistry.h>
#include "AbcShape.h"
#include "AbcShapeImport.h"
#include "VP2.h"



PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
   const char * pluginVersion = "1.0";
   MFnPlugin plugin(obj, "AbcShape", pluginVersion, "Any");

   MStatus status = plugin.registerShape("AbcShape",
                                         AbcShape::ID,
                                         AbcShape::creator,
                                         AbcShape::initialize,
                                         AbcShapeUI::creator,
                                         &AbcShapeOverride::Classification);
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register shape 'AbcShape'");
      return status;
   }
   
   status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(AbcShapeOverride::Classification,
                                                                  AbcShapeOverride::Registrant,
                                                                  AbcShapeOverride::create);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register draw override for 'AbcShape'");
      return status;
   }
   
   status = plugin.registerCommand("AbcShapeImport", AbcShapeImport::create, AbcShapeImport::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command 'AbcShapeImport'");
      return status;
   }
   
   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

   if (AbcShape::CallbackID != 0)
   {
      MDGMessage::removeCallback(AbcShape::CallbackID);
      AbcShape::CallbackID = 0;
   }
   
   MStatus status = plugin.deregisterCommand("AbcShapeImport");
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command 'AbcShapeImport'");
      return status;
   }

   status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(AbcShapeOverride::Classification,
                                                                            AbcShapeOverride::Registrant);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister draw override for 'AbcShape'");
      return status;
   }
   
   status = plugin.deregisterNode(AbcShape::ID);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister shape 'AbcShape'");
      return status;
   }
   
   return status;
}
