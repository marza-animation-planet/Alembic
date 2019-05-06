#include "GpuCacheTree.h"
#include "AlembicSceneCache.h"
#include <maya/MThreadUtils.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>

#ifndef ALTGPUCACHETREE_VERSION
#  define ALTGPUCACHETREE_VERSION "1.0"
#endif

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
   MFnPlugin plugin(obj, "altGpuCacheTree", ALTGPUCACHETREE_VERSION, "Any");
   MStatus status;

   AlembicSceneCache::SetConcurrency(size_t(MThreadUtils::getNumThreads()));

   status = plugin.registerCommand("altGpuCacheTree", GpuCacheTree::create, GpuCacheTree::createSyntax);
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to register command 'altGpuCacheTree'");
      return status;
   }

   MGlobal::executeCommand("if (`pluginInfo -q -l altGpuCache` == 0) { loadPlugin altGpuCache; }");

   return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
   MFnPlugin plugin(obj);
   MStatus status;
   
   status = plugin.deregisterCommand("altGpuCacheTree");
   
   if (status != MS::kSuccess)
   {
      status.perror("Failed to deregister command 'altGpuCacheTree'");
      return status;
   }

   return status;
}
