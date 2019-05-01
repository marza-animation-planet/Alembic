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
   MFnPlugin plugin(obj, "gpuCacheImport", GPUCACHEIMPORT_VERSION, "Any");
   MStatus status;

   AlembicSceneCache::SetConcurrency(size_t(MThreadUtils::getNumThreads()));

   status = plugin.registerCommand("gpuCacheImport", GpuCacheImport::create, GpuCacheImport::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command 'gpuCacheImport'");
      return status;
   }

   MGlobal::executeCommand("if (`pluginInfo -q -l gpuCache` == 0) { loadPlugin gpuCache; }");

   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);
   MStatus status;
   
   status = plugin.deregisterCommand("gpuCacheImport");
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command 'gpuCacheImport'");
      return status;
   }

   return status;
}
