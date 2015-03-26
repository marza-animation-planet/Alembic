#ifndef __abc_vray_geomsrc_h__
#define __abc_vray_geomsrc_h__

#include "common.h"
#include "params.h"
#include <map>

struct AlembicGeometrySource : VR::VRayStaticGeometry
{
   struct GeomInfo
   {
      GeomInfo();
      ~GeomInfo();
      
      void resetFlags();
      void clear();
      
      // for points primitives
      void sortParticles(size_t n, const Alembic::Util::uint64_t *ids);
      void attachParams(Factory*, bool verbose=false);
      std::string remapParamName(const std::string &name) const;
      bool isValidParamName(const std::string &name) const;
      bool isFloatParam(const std::string &name) const;
      bool isColorParam(const std::string &name) const;
      
      
      VR::VRayPlugin *geometry; // source mesh or points
      VR::VRayPlugin *modifier; // displacement / subdivision modifier (optional)
      VR::VRayPlugin *out;
      VR::StaticGeomSourceInterface *source; // source interface
      VR::VRayStaticGeometry *instance;
      
      bool updatedFrameGeometry;
      bool invalidFrame;
      
      bool ignore;
      
      std::string userAttr;
      
      // transforms (world space)
      VR::TraceTransform *matrices;
      int numMatrices;
      
      // number of points
      size_t numPoints;
      
      // mesh vertex index remapping
      size_t numTriangles;
      size_t numFaces;
      size_t numFaceVertices;
      unsigned int *toVertexIndex;
      unsigned int *toPointIndex;
      unsigned int *toFaceIndex;
      
      // mesh/points commont params
      VectorListParam *positions;
      
      // mesh specific params
      VectorListParam *normals;
      IntListParam *faceNormals;
      VR::DefIntListParam *faces;
      VR::DefMapChannelsParam *channels;
      VR::DefStringListParam *channelNames;
      VR::DefIntListParam *edgeVisibility;
      // creases
      std::vector<float*> smoothNormals;
      
      // reference mesh parameters
      VR::DefVectorListParam *constPositions;
      VR::DefVectorListParam *constNormals;
      VR::DefIntListParam *constFaceNormals;
      
      // point specific params
      bool sortIDs;
      Alembic::Util::uint64_t *particleOrder;
      VR::DefVectorListParam *velocities;
      VR::DefColorListParam *accelerations;
      VR::DefIntListParam *ids;
      std::map<std::string, VR::DefFloatListParam*> floatParams;
      std::map<std::string, VR::DefColorListParam*> colorParams;
      std::map<std::string, std::string> remapParamNames; // key: alembic name, value: vray name
    
   private:
      
      std::set<VR::VRayPluginParameter*> attachedParams;
   };
   
   
   
   AlembicGeometrySource(VR::VRayRenderer *vray,
                         AlembicLoaderParams::Cache *params,
                         class AlembicLoader *loader);
   
   virtual ~AlembicGeometrySource();
   
   
   
   void beginFrame(VR::VRayRenderer *vray);
   
   void instanciateGeometry(VR::MaterialInterface *mtl,
                            VR::BSDFInterface *bsdf,
                            int renderID,
                            VR::VolumetricInterface *volume,
                            VR::LightList *lightList,
                            const VR::TraceTransform &baseTM,
                            int objectID,
                            const tchar *userAttr,
                            int primaryVisibility);
   
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
   
   void destroyGeometryInstance();
   
   void endFrame(VR::VRayRenderer *vray);
   
   bool isValid() const;
   
   
   VR::VRayPlugin* createPlugin(const tchar *pluginType);
   void deletePlugin(VR::VRayPlugin *plugin);
   
   
   inline AlembicLoaderParams::Cache* params() { return mParams; }
   inline PluginManager* pluginManager() { return mPluginMgr; }
   inline Factory* factory() { return &mFactory; }
   inline AlembicScene* scene() { return mScene; }
   inline AlembicScene* referenceScene() { return mRefScene; }
   inline const std::map<std::string, Alembic::Abc::M44d>& referenceTransforms() const { return mRefMatrices; }
   inline double renderTime() const { return mRenderTime; }
   inline double renderFrame() const { return mRenderFrame; }
   inline size_t numTimeSamples() const { return mSampleTimes.size(); }
   inline double sampleTime(size_t i) const { return mSampleTimes[i]; }
   inline double sampleFrame(size_t i) const { return mSampleFrames[i]; }
   inline double* sampleTimes() { return (mSampleTimes.size() > 0 ? &mSampleTimes[0] : 0); }
   inline double* sampleFrames() { return (mSampleFrames.size() > 0 ? &mSampleFrames[0] : 0); }
   
   GeomInfo* getInfo(const std::string &path);
   const GeomInfo* getInfo(const std::string &path) const;
   GeomInfo* addInfo(const std::string &path, VR::VRayPlugin *geom, VR::VRayPlugin *mod=0);
   void removeInfo(const std::string &path);
   
   void dumpVRScene(const char *path=0);
   
private:
  
   bool compileMatrices(GeomInfo &info, VR::TraceTransform* &tm, double* &times, int &tmCount);
  
private:
   
   AlembicLoaderParams::Cache *mParams;
   PluginManager *mPluginMgr;
   std::string mSceneID;
   AlembicScene *mScene;
   AlembicScene *mRefScene;
   VR::Table<VR::VRayPlugin*, -1> mCreatedPlugins;
   Factory mFactory;
   
   double mRenderTime;
   double mRenderFrame;
   std::vector<double> mSampleTimes;
   std::vector<double> mSampleFrames;
   
   std::map<std::string, GeomInfo> mGeoms;
   
   std::map<std::string, Alembic::Abc::M44d> mRefMatrices;
   
   class AlembicLoader *mLoader;
};

#endif