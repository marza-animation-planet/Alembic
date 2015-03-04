#ifndef __abc_vray_plugin_h__
#define __abc_vray_plugin_h__

#include "common.h"
#include "params.h"
#include "geomsrc.h"

struct AlembicLoader : VR::VRayStaticGeomSource,
                       VR::VRaySceneModifierInterface
{  
   AlembicLoader(VR::VRayPluginDesc *desc);
   virtual ~AlembicLoader();

   virtual PluginInterface* newInterface(InterfaceID id);
   virtual PluginBase* getPlugin(void);

   virtual VR::VRayStaticGeometry* newInstance(VR::MaterialInterface *mtl,
                                               VR::BSDFInterface *bsdf,
                                               int renderID,
                                               VR::VolumetricInterface *volume,
                                               VR::LightList *lightList,
                                               const VR::TraceTransform &baseTM,
                                               int objectID,
                                               const tchar *userAttr,
                                               int primaryVisibility);
   virtual void deleteInstance(VR::VRayStaticGeometry *instance);

   virtual void preRenderBegin(VR::VRayRenderer *vray);
   virtual void postRenderEnd(VR::VRayRenderer *vray);

   virtual void renderBegin(VR::VRayRenderer *vray);
   virtual void frameBegin(VR::VRayRenderer *vray);
   virtual void frameEnd(VR::VRayRenderer *vray);
   virtual void renderEnd(VR::VRayRenderer *vray);
   
   
   void dumpVRScene(const char *path=0);
  
private:

   AlembicLoaderParams::Cache mParams;
   AlembicGeometrySource *mGeoSrc;
};

#endif
