#ifndef __abc_vray_geomsrc_h__
#define __abc_vray_geomsrc_h__

#include "common.h"
#include "params.h"
#include <map>

class AbcVRayGeom;

struct AlembicGeometrySource : VR::VRayStaticGeometry
{
   AlembicGeometrySource(class AlembicLoader *loader, bool ignoreReference);
   
   virtual ~AlembicGeometrySource();
   
   
   virtual void compileGeometry(VR::VRayRenderer *vray, VR::TraceTransform *tm, double *times, int tmCount);
   
   virtual void clearGeometry(VR::VRayRenderer *vray);
   
   virtual void updateMaterial(VR::MaterialInterface *mtl,
                               VR::BSDFInterface *bsdf,
                               int renderID,
                               VR::VolumetricInterface *volume,
                               VR::LightList *lightList,
                               int objectID);
   
   virtual VR::VRayShadeData* getShadeData(const VR::VRayContext *rc);
   
   virtual VR::VRayShadeInstance* getShadeInstance(const VR::VRayContext *rc);
   
   
   inline AlembicScene* scene() { return mScene; }
   inline AlembicScene* referenceScene() { return mRefScene; }
   inline const std::map<std::string, Alembic::Abc::M44d>& referenceTransforms() const { return mRefMatrices; }
   
   void cleanAlembicTemporaries();
   
   // All those forwards to AlembicLoader
   VR::VRayPlugin* createPlugin(const tchar *pluginType);
   void deletePlugin(VR::VRayPlugin *plugin);
   
   AlembicLoaderParams::Cache* params();
   PluginManager* pluginManager();
   Factory* factory();
   
   double* sampleTimes();
   double* sampleFrames();
   double renderTime() const;
   double renderFrame() const;
   size_t numTimeSamples() const;
   double sampleTime(size_t i) const;
   double sampleFrame(size_t i) const;
   
   AbcVRayGeom* getGeometry(const std::string &path);
   const AbcVRayGeom* getGeometry(const std::string &path) const;
   AbcVRayGeom* addGeometry(const std::string &path, VR::VRayPlugin *geom, VR::VRayPlugin *mod=0);
   void removeGeometry(const std::string &path);
   
private:
   
   AlembicScene *mScene;
   AlembicScene *mRefScene;
   std::map<std::string, Alembic::Abc::M44d> mRefMatrices;
   class AlembicLoader *mLoader;
};

#endif