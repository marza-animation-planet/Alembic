#include <maya/MFnPlugin.h>
#include "AbcShape.h"

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
   const char * pluginVersion = "1.0";
   MFnPlugin plugin(obj, "AbcShape", pluginVersion, "Any");

   MStatus status = plugin.registerShape("AbcShape",
                                         AbcShape::ID,
                                         AbcShape::creator,
                                         AbcShape::initialize,
                                         AbcShapeUI::creator);
   
   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);

   MStatus status = plugin.deregisterNode(AbcShape::ID);
   
   return status;
}
