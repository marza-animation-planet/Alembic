#ifndef __abc_vray_plugin_h__
#define __abc_vray_plugin_h__

#include "common.h"
#include "params.h"

class AbcVRayGeom
{
public:
   
   AbcVRayGeom();
   ~AbcVRayGeom();
   
   void resetFlags();
   void clear();
   
   void cleanGeometryTemporaries();
   void cleanTransformTemporaries();
   
   // for points primitives
   void sortParticles(size_t n, const Alembic::Util::uint64_t *ids);
   void attachParams(Factory*, bool verbose=false);
   std::string remapParamName(const std::string &name) const;
   bool isValidParamName(const std::string &name) const;
   bool isFloatParam(const std::string &name) const;
   bool isColorParam(const std::string &name) const;
   
public:
   
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
   
   // ---
   
   VR::VRayPlugin* createPlugin(const tchar *pluginType);
   void deletePlugin(VR::VRayPlugin *plugin);
   
   inline PluginManager* pluginManager() { return mPluginMgr; }
   inline Factory* factory() { return &mFactory; }
   inline AlembicLoaderParams::Cache* params() { return &mParams; }
   
   inline double* sampleTimes() { return (mSampleTimes.size() > 0 ? &mSampleTimes[0] : 0); }
   inline double* sampleFrames() { return (mSampleFrames.size() > 0 ? &mSampleFrames[0] : 0); }
   inline double renderTime() const { return mRenderTime; }
   inline double renderFrame() const { return mRenderFrame; }
   inline size_t numTimeSamples() const { return mSampleTimes.size(); }
   inline double sampleTime(size_t i) const { return (i < mSampleTimes.size() ? mSampleTimes[i] : 0.0); }
   inline double sampleFrame(size_t i) const { return (i < mSampleFrames.size() ? mSampleFrames[i] : 0.0); }
   
   AbcVRayGeom* getGeometry(const std::string &path);
   const AbcVRayGeom* getGeometry(const std::string &path) const;
   AbcVRayGeom* addGeometry(const std::string &path, VR::VRayPlugin *geom, VR::VRayPlugin *mod=0);
   void removeGeometry(const std::string &path);
   
   void resetFlags();
   void clear();
   bool hasGeometry() const;
   
   void compileGeometry(VR::VRayRenderer *vray, VR::TraceTransform *tm, double *times, int tmCount);
   void clearGeometry(VR::VRayRenderer *vray);
   void updateMaterial(VR::MaterialInterface *mtl,
                       VR::BSDFInterface *bsdf,
                       int renderID,
                       VR::VolumetricInterface *volume,
                       VR::LightList *lightList,
                       int objectID);
   
   void dumpVRScene(const char *path=0, double frame=1.0);

private:
   
   bool compileMatrices(AbcVRayGeom &geom, VR::TraceTransform* &tm, double* &times, int &tmCount);

private:
   
   AlembicLoaderParams::Cache mParams;
   PluginManager *mPluginMgr;
   Factory mFactory;
   std::map<std::string, AbcVRayGeom> mGeoms;
   VR::Table<VR::VRayPlugin*, -1> mCreatedPlugins;
   double mRenderTime;
   double mRenderFrame;
   std::vector<double> mSampleTimes;
   std::vector<double> mSampleFrames;
};

#endif
