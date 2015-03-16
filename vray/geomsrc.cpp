#include "geomsrc.h"
#include "visitors.h"
#include "plugin.h"
#include <algorithm>


AlembicGeometrySource::AlembicGeometrySource(AlembicLoader *loader, bool ignoreReference)
   : mScene(0)
   , mRefScene(0)
   , mLoader(loader)
{
   TRACEM(AlembicGeometrySource, AlembicGeometrySource);
   
   if (mLoader)
   {
      AlembicLoaderParams::Cache *params = mLoader->params();
      
      std::string path = (params->filename.empty() ? "" : params->filename.ptr());
      std::string refPath = (params->referenceFilename.empty() ? "" : params->referenceFilename.ptr());
      std::string objPath = (params->objectPath.empty() ? "" : params->objectPath.ptr());   
      
      if (path.length() > 0)
      {
         AlembicSceneFilter filter(objPath, "");
         
         // Allow for directory mappings here
         
         mScene = AlembicSceneCache::Ref(path, filter, false);
         
         if (mScene && !ignoreReference)
         {
            CollectWorldMatrices WM(this);
            
            if (refPath.length() > 0)
            {
               mRefScene = AlembicSceneCache::Ref(refPath, filter, false);
               
               if (mRefScene)
               {
                  mRefScene->visit(AlembicNode::VisitDepthFirst, WM);
                  mRefMatrices = WM.getWorldMatrices();
               }
            }
         }
      }
   }
}

AlembicGeometrySource::~AlembicGeometrySource()
{
   TRACEM(AlembicGeometrySource, ~AlembicGeometrySource);
   
   cleanAlembicTemporaries();
}

void AlembicGeometrySource::cleanAlembicTemporaries()
{
   if (mScene)
   {
      AlembicSceneCache::Unref(mScene);
      mScene = 0;
   }
   
   if (mRefScene)
   {
      AlembicSceneCache::Unref(mRefScene);
      mRefScene = 0;
   }
   
   mRefMatrices.clear();
}

void AlembicGeometrySource::compileGeometry(VR::VRayRenderer *vray, VR::TraceTransform *tm, double *times, int tmCount)
{
   TRACEM(AlembicGeometrySource, compileGeometry);
   
   if (mLoader)
   {
      mLoader->compileGeometry(vray, tm, times, tmCount);
   }
}

void AlembicGeometrySource::updateMaterial(VR::MaterialInterface *mtl,
                                           VR::BSDFInterface *bsdf,
                                           int renderID,
                                           VR::VolumetricInterface *volume,
                                           VR::LightList *lightList,
                                           int objectID)
{
   TRACEM(AlembicGeometrySource, updateMaterial);
   
   if (mLoader)
   {
      mLoader->updateMaterial(mtl, bsdf, renderID, volume, lightList, objectID);
   }
}

void AlembicGeometrySource::clearGeometry(VR::VRayRenderer *vray)
{
   TRACEM(AlembicGeometrySource, clearGeometry);
   
   if (mLoader)
   {
      mLoader->clearGeometry(vray);
   }
}

VR::VRayShadeData* AlembicGeometrySource::getShadeData(const VR::VRayContext *rc)
{
   TRACEM(AlembicGeometrySource, getShadeData);
   
   return 0;
}

VR::VRayShadeInstance* AlembicGeometrySource::getShadeInstance(const VR::VRayContext *rc)
{
   TRACEM(AlembicGeometrySource, getShadeInstance);
   
   return 0;
}

double AlembicGeometrySource::renderTime() const
{
   return (mLoader ? mLoader->renderTime() : 0.0);
}

double AlembicGeometrySource::renderFrame() const
{
   return (mLoader ? mLoader->renderFrame() : 0.0);
}

size_t AlembicGeometrySource::numTimeSamples() const
{
   return (mLoader ? mLoader->numTimeSamples() : 0);
}

double AlembicGeometrySource::sampleTime(size_t i) const
{
   return (mLoader ? mLoader->sampleTime(i) : 0.0);
}

double AlembicGeometrySource::sampleFrame(size_t i) const
{
   return (mLoader ? mLoader->sampleFrame(i) : 0.0);
}

double* AlembicGeometrySource::sampleTimes()
{
   return (mLoader ? mLoader->sampleTimes() : 0);
}

double* AlembicGeometrySource::sampleFrames()
{
   return (mLoader ? mLoader->sampleFrames() : 0);
}

VR::VRayPlugin* AlembicGeometrySource::createPlugin(const tchar *pluginType)
{
   return (mLoader ? mLoader->createPlugin(pluginType) : 0);
}

void AlembicGeometrySource::deletePlugin(VR::VRayPlugin *plugin)
{
   if (mLoader)
   {
      mLoader->deletePlugin(plugin);
   }
}

AlembicLoaderParams::Cache* AlembicGeometrySource::params()
{
   return (mLoader ? mLoader->params() : 0);
}

PluginManager* AlembicGeometrySource::pluginManager()
{ 
   return (mLoader ? mLoader->pluginManager() : 0);
}

Factory* AlembicGeometrySource::factory()
{ 
   return (mLoader ? mLoader->factory() : 0);
}

AbcVRayGeom* AlembicGeometrySource::getGeometry(const std::string &path)
{ 
   return (mLoader ? mLoader->getGeometry(path) : 0);
}

const AbcVRayGeom* AlembicGeometrySource::getGeometry(const std::string &path) const
{ 
   return (mLoader ? mLoader->getGeometry(path) : 0);
}

AbcVRayGeom* AlembicGeometrySource::addGeometry(const std::string &path, VR::VRayPlugin *geom, VR::VRayPlugin *mod)
{ 
   return (mLoader ? mLoader->addGeometry(path, geom, mod) : 0);
}

void AlembicGeometrySource::removeGeometry(const std::string &path)
{
   if (mLoader)
   {
      mLoader->removeGeometry(path);
   }
}
