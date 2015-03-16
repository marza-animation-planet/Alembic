#include "plugin.h"
#include <cstdlib>

AlembicLoader::AlembicLoader(VR::VRayPluginDesc *desc)
   : VR::VRayStaticGeomSource(desc)
   , mGeoSrc(0)
{
   TRACEM(AlembicLoader, AlembicLoader);
   
   mParams.setupCache(paramList);
}

AlembicLoader::~AlembicLoader()
{
   TRACEM(AlembicLoader, ~AlembicLoader);
}

PluginInterface* AlembicLoader::newInterface(InterfaceID id)
{
   switch (id)
   {
   case EXT_SCENE_MODIFIER:
      return static_cast<VR::VRaySceneModifierInterface*>(this);
   default:
      return VR::VRayStaticGeomSource::newInterface(id);
   }
}

PluginBase* AlembicLoader::getPlugin(void)
{
   return (PluginBase*) this;
}

void AlembicLoader::preRenderBegin(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, preRenderBegin);
   
   paramList->cacheParams();
   
   mGeoSrc = new AlembicGeometrySource(vray, &mParams, this);
   
   if (!mGeoSrc->isValid())
   {
      std::cerr << "[AlembicLoader] AlembicLoader::preRenderBegin: Invalid source" << std::endl;
      delete mGeoSrc;
      mGeoSrc = 0;
   }
}

void AlembicLoader::renderBegin(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, renderBegin);
   
   VR::VRayStaticGeomSource::renderBegin(vray);
}

void AlembicLoader::frameBegin(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, frameBegin);
   
   VR::VRayStaticGeomSource::frameBegin(vray);
   
   if (mGeoSrc)
   {
      mGeoSrc->beginFrame(vray);
   }
}

VR::VRayStaticGeometry* AlembicLoader::newInstance(VR::MaterialInterface *mtl,
                                                   VR::BSDFInterface *bsdf,
                                                   int renderID,
                                                   VR::VolumetricInterface *volume,
                                                   VR::LightList *lightList,
                                                   const VR::TraceTransform &baseTM,
                                                   int objectID,
                                                   const tchar *userAttr,
                                                   int primaryVisibility)
{
   TRACEM(AlembicLoader, newInstance);
   
   if (mGeoSrc)
   {
      mGeoSrc->instanciateGeometry(mtl, bsdf, renderID, volume, lightList, baseTM, objectID, userAttr, primaryVisibility);
      return mGeoSrc;
   }
   else
   {
      return 0;
   }
}
  
void AlembicLoader::deleteInstance(VR::VRayStaticGeometry *instance)
{
   TRACEM(AlembicLoader, deleteInstance);
   
   if (mGeoSrc)
   {
      mGeoSrc->destroyGeometryInstance();
   }
}

void AlembicLoader::frameEnd(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, frameEnd);
   
   if (mGeoSrc)
   {
      mGeoSrc->endFrame(vray);
   }
   
   VR::VRayStaticGeomSource::frameEnd(vray);
}

void AlembicLoader::renderEnd(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, renderEnd);
   
   VR::VRayStaticGeomSource::renderEnd(vray);
}

void AlembicLoader::dumpVRScene(const char *path)
{
   if (!mGeoSrc)
   {
      return;
   }
   
   std::string outpath;
   
   if (!path)
   {
      outpath = "AlembicLoader_expanded.vrscene";
      
      char *envval = getenv("VRAY_ALEMBIC_LOADER_DUMPFILE");
      if (envval)
      {
         outpath = envval;
      }
   }
   else
   {
      outpath = path;
   }
   
   VR::VRaySettingsHolder settings;
   
   settings.compressed = false;
   settings.useHexFormat = false;
   
   VR::SceneInfoWithPlugMan sceneInfo(*(mGeoSrc->pluginManager()), NULL, &settings);
   
   VR::VRayPluginsExporterBase *exporter = VR::VRayPluginsExporterBase::newVRayPluginsExporter(sceneInfo, outpath.c_str());
   
   double f = mGeoSrc->renderFrame();
   
   exporter->doExport(1, f, f, f, int(f));
   
   VR::VRayPluginsExporterBase::deleteVRayPluginsExporter(exporter);
}

void AlembicLoader::postRenderEnd(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, postRenderEnd);
   
   int debug = 0;
      
   char *envval = getenv("VRAY_ALEMBIC_LOADER_DEBUG");
   
   if (envval && sscanf(envval, "%d", &debug) == 1 && debug != 0)
   {
      dumpVRScene();
   }
   
   if (mGeoSrc)
   {
      
      
      delete mGeoSrc;
      mGeoSrc = 0;
   }
}
