#include "GpuCacheImport.h"
#include "AlembicSceneCache.h"
#include <maya/MThreadUtils.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>

#ifndef GPUCACHEIMPORT_VERSION
#  define GPUCACHEIMPORT_VERSION "1.0"
#endif

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
   MString commandName = "gpuCacheImport";
   
   MFnPlugin plugin(obj, commandName, GPUCACHEIMPORT_VERSION, "Any");

   AlembicSceneCache::SetConcurrency(size_t(MThreadUtils::getNumThreads()));

   status = plugin.registerCommand(commandName, GpuCacheImport::create, GpuCacheImport::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command '" + commandName + "'");
      return status;
   }

   MGlobal::executeCommand("if (`pluginInfo -q -l gpuCache` == 0) { loadPlugin gpuCache; }");

   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);
   
   MStatus status;
   MString commandName = "gpuCacheImport";
   
   status = plugin.deregisterCommand(commandName);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command '" + commandName + "'");
      return status;
   }

   return status;
}
