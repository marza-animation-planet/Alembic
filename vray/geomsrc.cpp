#include "geomsrc.h"
#include "visitors.h"
#include "plugin.h"
#include <algorithm>

// ---

struct AttrRange
{
   size_t start;
   size_t end;
};

typedef std::map<std::string, AttrRange> StringUserAttrs;

static bool ParseAttrName(const std::string &s, std::string &name)
{
   size_t p = s.find('=');
   
   if (p != std::string::npos)
   {
      name = s.substr(0, p);
      
      p = name.find_first_not_of(" \t");
      if (p != std::string::npos && p > 0)
      {
         name = name.substr(p);
      }
      
      p = name.find_last_not_of(" \t");
      if (p != std::string::npos && (p + 1) < name.length())
      {
         name = name.substr(0, p);
      }
      
      if (name.length() > 0)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

static void ParseStringUserAttrs(const std::string &s, StringUserAttrs &attrs)
{
   std::string tmp;
   std::string name;
   
   attrs.clear();
   
   size_t p0 = 0;
   size_t p1 = s.find(';', p0);
   
   while (p1 != std::string::npos)
   {
      tmp = s.substr(p0, p1-p0);
      
      if (ParseAttrName(tmp, name))
      {
         AttrRange range = {p0, p1 - 1};
         attrs[name] = range;
      }
      
      p0 = p1 + 1;
      p1 = s.find(';', p0);
   }
   
   tmp = s.substr(p0);
   
   if (ParseAttrName(tmp, name))
   {
      AttrRange range = {p0, s.length() - 1};
      attrs[name] = range;
   }
}

// ---

AlembicGeometrySource::GeomInfo::GeomInfo()
   : geometry(0)
   , modifier(0)
   , out(0)
   , source(0)
   , instance(0)
   , updatedFrameGeometry(false)
   , invalidFrame(true)      
   , ignore(true)
   , userAttr("")
   , matrices(0)
   , numMatrices(0)
   , numPoints(0)
   , numFaces(0)
   , numFaceVertices(0)
   , numTriangles(0)
   , toVertexIndex(0)
   , toPointIndex(0)
   , toFaceIndex(0)
   , positions(0)
   , normals(0)
   , faceNormals(0)
   , faces(0)
   , channels(0)
   , channelNames(0)
   , edgeVisibility(0)
   , constPositions(0)
   , constNormals(0)
   , constFaceNormals(0)
   , sortIDs(false)
   , particleOrder(0)
   , velocities(0)
   , accelerations(0)
   , ids(0)
{
}

AlembicGeometrySource::GeomInfo::~GeomInfo()
{
   clear();
}

void AlembicGeometrySource::GeomInfo::clear()
{
   if (matrices)
   {
      delete[] matrices;
      matrices = 0;
   }
   
   if (toPointIndex)
   {
      delete[] toPointIndex;
      toPointIndex = 0;
   }
   
   if (toVertexIndex)
   {
      delete[] toVertexIndex;
      toVertexIndex = 0;
   }
   
   if (toFaceIndex)
   {
      delete[] toFaceIndex;
      toFaceIndex = 0;
   }
   
   for (size_t i=0; i<smoothNormals.size(); ++i)
   {
      free(smoothNormals[i]);
   }
   smoothNormals.clear();
   
   numPoints = 0;
   numTriangles = 0;
   numFaces = 0;
   numFaceVertices = 0;
   numMatrices = 0;
   userAttr = "";
   
   if (particleOrder)
   {
      delete[] particleOrder;
      particleOrder = 0;
   }
   
   for (std::map<std::string, VR::DefFloatListParam*>::iterator it=floatParams.begin(); it!=floatParams.end(); ++it)
   {
      if (attachedParams.find(it->second) == attachedParams.end())
      {
         delete it->second;
      }
   }
   
   for (std::map<std::string, VR::DefColorListParam*>::iterator it=colorParams.begin(); it!=colorParams.end(); ++it)
   {
      if (attachedParams.find(it->second) == attachedParams.end())
      {
         delete it->second;
      }
   }
   
   floatParams.clear();
   colorParams.clear();
   attachedParams.clear();
   
   // Don't touch other V-Ray objects
   // - Parameters should all be store in factory
   // - Plugins are tracked by the plugin manager
   
   resetFlags();
}

void AlembicGeometrySource::GeomInfo::resetFlags()
{
   ignore = true;
   updatedFrameGeometry = false;
   invalidFrame = true;
}

struct CompareOp
{
   inline CompareOp(const Alembic::Util::uint64_t *ids)
      : _ids(ids)
   {
   }
   
   inline bool operator()(Alembic::Util::uint64_t i0, Alembic::Util::uint64_t i1) const
   {
      return (_ids[i0] < _ids[i1]);
   }
   
   private:
      
      const Alembic::Util::uint64_t *_ids;
};

void AlembicGeometrySource::GeomInfo::sortParticles(size_t count, const Alembic::Util::uint64_t *ids)
{
   if (particleOrder)
   {
      delete[] particleOrder;
      particleOrder = 0;
   }
   
   if (count == 0 || !ids)
   {
      return;
   }
   
   particleOrder = new Alembic::Util::uint64_t[count];
   
   for (size_t i=0; i<count; ++i)
   {
      particleOrder[i] = i;
   }
   
   if (!sortIDs)
   {
      return;
   }
   
   CompareOp comp(ids);
   
   std::sort(particleOrder, particleOrder+count, comp);
   
   // As we want to remap the destination index rather than the source one, reverse the mapping
   //  i.e.:
   //   out[particleOrder[i]] = in[j]
   //  rather than:
   //   out[i] = in[particleOrder[j]]
   //
   // This is because the source may be splitted into separate arrays
   
   Alembic::Util::uint64_t *inv = new Alembic::Util::uint64_t[count];
   
   for (size_t i=0; i<count; ++i)
   {
      inv[particleOrder[i]] = i;
   }
   
   delete[] particleOrder;
   
   particleOrder = inv;
}

std::string AlembicGeometrySource::GeomInfo::remapParamName(const std::string &name) const
{
   std::map<std::string, std::string>::const_iterator it = remapParamNames.find(name);
   return (it == remapParamNames.end() ? name : it->second);
}

bool AlembicGeometrySource::GeomInfo::isValidParamName(const std::string &name) const
{
   static std::set<std::string> sValidNames;
   
   if (sValidNames.size() == 0)
   {
      sValidNames.insert("colors");
      sValidNames.insert("emission_pp");
      sValidNames.insert("user_color_pp_1");
      sValidNames.insert("user_color_pp_2");
      sValidNames.insert("user_color_pp_3");
      sValidNames.insert("user_color_pp_4");
      sValidNames.insert("user_color_pp_5");
      sValidNames.insert("age_pp");
      sValidNames.insert("lifespan_pp");
      sValidNames.insert("opacity_pp");
      sValidNames.insert("user_float_pp_1");
      sValidNames.insert("user_float_pp_2");
      sValidNames.insert("user_float_pp_3");
      sValidNames.insert("user_float_pp_4");
      sValidNames.insert("user_float_pp_5");
      sValidNames.insert("radii");
      sValidNames.insert("sprite_scale_x");
      sValidNames.insert("sprite_scale_y");
      sValidNames.insert("sprite_rotate");
   }
   
   return (sValidNames.find(name) != sValidNames.end());
}

bool AlembicGeometrySource::GeomInfo::isFloatParam(const std::string &name) const
{
   static std::set<std::string> sValidNames;
   
   if (sValidNames.size() == 0)
   {
      sValidNames.insert("age_pp");
      sValidNames.insert("lifespan_pp");
      sValidNames.insert("opacity_pp");
      sValidNames.insert("user_float_pp_1");
      sValidNames.insert("user_float_pp_2");
      sValidNames.insert("user_float_pp_3");
      sValidNames.insert("user_float_pp_4");
      sValidNames.insert("user_float_pp_5");
      sValidNames.insert("radii");
      sValidNames.insert("sprite_scale_x");
      sValidNames.insert("sprite_scale_y");
      sValidNames.insert("sprite_rotate");
   }
   
   return (sValidNames.find(name) != sValidNames.end());
}

bool AlembicGeometrySource::GeomInfo::isColorParam(const std::string &name) const
{
   static std::set<std::string> sValidNames;
   
   if (sValidNames.size() == 0)
   {
      sValidNames.insert("colors");
      sValidNames.insert("emission_pp");
      sValidNames.insert("user_color_pp_1");
      sValidNames.insert("user_color_pp_2");
      sValidNames.insert("user_color_pp_3");
      sValidNames.insert("user_color_pp_4");
      sValidNames.insert("user_color_pp_5");
   }
   
   return (sValidNames.find(name) != sValidNames.end());
}

void AlembicGeometrySource::GeomInfo::attachParams(Factory *f, bool verbose)
{
   if (!out)
   {
      return;
   }
   
   for (std::map<std::string, VR::DefFloatListParam*>::iterator it=floatParams.begin(); it!=floatParams.end(); ++it)
   {
      if (attachedParams.find(it->second) == attachedParams.end())
      {
         if (out->setParameter(it->second))
         {
            f->saveInFactory(it->second);
            attachedParams.insert(it->second);
            
            if (verbose)
            {
               std::cout << "[AlembicLoader] AlembicGeometrySource::GeomInfo::attachParams: Float parameter \"" << it->first << "\" attached" << std::endl;
            }
         }
      }
   }
   
   for (std::map<std::string, VR::DefColorListParam*>::iterator it=colorParams.begin(); it!=colorParams.end(); ++it)
   {
      if (attachedParams.find(it->second) == attachedParams.end())
      {
         if (out->setParameter(it->second))
         {
            f->saveInFactory(it->second);
            attachedParams.insert(it->second);
            
            if (verbose)
            {
               std::cout << "[AlembicLoader] AlembicGeometrySource::GeomInfo::attachParams: Color parameter \"" << it->first << "\" attached" << std::endl;
            }
         }
      }
   }
}

// ---

AlembicGeometrySource::AlembicGeometrySource(VR::VRayRenderer *vray,
                                             AlembicLoaderParams::Cache *params,
                                             AlembicLoader *loader)
   : mParams(params)
   , mPluginMgr(0)
   , mScene(0)
   , mRefScene(0)
   , mRenderTime(0)
   , mRenderFrame(0)
   , mLoader(loader)
{
   TRACEM(AlembicGeometrySource, AlembicGeometrySource);
   
   VR::VRayPluginRendererInterface *pluginRenderer = (VR::VRayPluginRendererInterface*) GET_INTERFACE(vray, EXT_PLUGIN_RENDERER);
   
   if (pluginRenderer)
   {
      mPluginMgr = pluginRenderer->getPluginManager();
      
      std::string path = (params->filename.empty() ? "" : params->filename.ptr());
      std::string refPath = (params->referenceFilename.empty() ? "" : params->referenceFilename.ptr());
      std::string objPath = (params->objectPath.empty() ? "" : params->objectPath.ptr());   
      
      if (path.length() > 0)
      {
         const VR::VRaySequenceData &seq = vray->getSequenceData();
         
         int threadIndex = seq.threadManager->getThreadIndex();
         
         char tmp[64];
         sprintf(tmp, "%d", threadIndex);
         mSceneID = tmp;
         
         AlembicSceneFilter filter(objPath, "");
         
         // Allow for directory mappings here
         
         mScene = AlembicSceneCache::Ref(path, mSceneID, filter, true);
         
         if (mScene)
         {
            CollectWorldMatrices WM(this);
            
            if (refPath.length() > 0)
            {
               mRefScene = AlembicSceneCache::Ref(refPath, mSceneID, filter, true);
               
               if (mRefScene)
               {
                  mRefScene->visit(AlembicNode::VisitDepthFirst, WM);
                  mRefMatrices = WM.getWorldMatrices();
               }
               else
               {
                  // use mScene?
               }
            }
            
            BuildPlugins bldplg(this, vray);
            
            mScene->visit(AlembicNode::VisitDepthFirst, bldplg);
         }
      }
   }
}

void AlembicGeometrySource::dumpVRScene(const char *path)
{
   if (mLoader)
   {
      mLoader->dumpVRScene(path);
   }
}

AlembicGeometrySource::~AlembicGeometrySource()
{
   TRACEM(AlembicGeometrySource, ~AlembicGeometrySource);
   
   if (mScene)
   {
      AlembicSceneCache::Unref(mScene, mSceneID);
      mScene = 0;
   }
   
   if (mRefScene)
   {
      AlembicSceneCache::Unref(mRefScene, mSceneID);
      mRefScene = 0;
   }
      
   if (mPluginMgr)
   {
      for (int i=0; i<mCreatedPlugins.count(); ++i)
      {
         if (mCreatedPlugins[i])
         {
            mPluginMgr->deletePlugin(mCreatedPlugins[i]);
         }
      }
      
      mCreatedPlugins.freeMem();
   }
   
   mFactory.clear();
   
   mGeoms.clear();
   
   mPluginMgr = 0;
}

VR::VRayPlugin* AlembicGeometrySource::createPlugin(const tchar *pluginType)
{
   TRACEM_MSG(AlembicGeometrySource, createPlugin, pluginType);
   
   VR::VRayPlugin *rv = 0;
  
   if (mPluginMgr)
   {
      rv = (VR::VRayPlugin*) mPluginMgr->newPlugin(pluginType, NULL);
      if (rv)
      {
         mCreatedPlugins += rv;
      }
   }
   
   return rv;
}

void AlembicGeometrySource::deletePlugin(VR::VRayPlugin *plugin)
{
   if (mPluginMgr)
   {
      for (int i=0; i<mCreatedPlugins.count(); ++i)
      {
         if (mCreatedPlugins[i] == plugin)
         {
            mPluginMgr->deletePlugin(plugin);
            
            mCreatedPlugins[i] = 0;
         }
      }
   }
}

bool AlembicGeometrySource::isValid() const
{
   return (mGeoms.size() > 0);
}

AlembicGeometrySource::GeomInfo* AlembicGeometrySource::addInfo(const std::string &path, VR::VRayPlugin *geom, VR::VRayPlugin *mod)
{
   TRACEM_MSG(AlembicGeometrySource, addInfo, path);
   
   std::map<std::string, GeomInfo>::iterator it = mGeoms.find(path);
   if (it != mGeoms.end())
   {
      return 0;
   }
   
   GeomInfo &info = mGeoms[path];
   
   info.geometry = geom;
   info.modifier = mod;
   info.out = (mod ? mod : geom);
   info.source = static_cast<VR::StaticGeomSourceInterface*>(GET_INTERFACE(info.out, EXT_STATIC_GEOM_SOURCE));
   
   return &info;
}

void AlembicGeometrySource::removeInfo(const std::string &path)
{
   std::map<std::string, GeomInfo>::iterator it = mGeoms.find(path);
   if (it != mGeoms.end())
   {
      // this should leave V-Ray plugins and parameters around
      mGeoms.erase(it);
   }
}

AlembicGeometrySource::GeomInfo* AlembicGeometrySource::getInfo(const std::string &path)
{
   std::map<std::string, GeomInfo>::iterator it = mGeoms.find(path);
   return (it != mGeoms.end() ? &(it->second) : 0);
}

const AlembicGeometrySource::GeomInfo* AlembicGeometrySource::getInfo(const std::string &path) const
{
   std::map<std::string, GeomInfo>::const_iterator it = mGeoms.find(path);
   return (it != mGeoms.end() ? &(it->second) : 0);
}

void AlembicGeometrySource::beginFrame(VR::VRayRenderer *vray)
{
   TRACEM(AlembicGeometrySource, beginFrame);
   
   if (mScene)
   {
      const VR::VRayFrameData &frameData = vray->getFrameData();
      const VR::VRaySequenceData &seqData = vray->getSequenceData();
      
      bool ignoreMotionBlur = (mParams->ignoreDeformBlur && (mParams->ignoreTransforms || mParams->ignoreTransformBlur));
      
      mRenderFrame = vray->getTimeInFrames(frameData.t);
      mRenderTime = mRenderFrame / mParams->fps;
      
      mSampleTimes.clear();
      mSampleFrames.clear();
      
      if (seqData.params.moblur.on && seqData.params.moblur.geomSamples > 1 && !ignoreMotionBlur)
      {
         double t = frameData.frameStart;
         double step = (frameData.frameEnd - frameData.frameStart) / double(seqData.params.moblur.geomSamples - 1);
         
         while (t <= frameData.frameEnd)
         {
            double f = vray->getTimeInFrames(t);
            mSampleFrames.push_back(f);
            mSampleTimes.push_back(f / mParams->fps);
            t += step;
         }
      }
      else
      {
         mSampleTimes.push_back(mRenderTime);
         mSampleFrames.push_back(mRenderFrame);
      }
      
      for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
      {
         it->second.resetFlags();
      }
      
      UpdateGeometry updateGeometry(this);
      
      mScene->visit(AlembicNode::VisitDepthFirst, updateGeometry);
   }
}

void AlembicGeometrySource::instanciateGeometry(VR::MaterialInterface *mtl,
                                                VR::BSDFInterface *bsdf,
                                                int renderID,
                                                VR::VolumetricInterface *volume,
                                                VR::LightList *lightList,
                                                const VR::TraceTransform &baseTM,
                                                int objectID,
                                                const tchar *userAttr,
                                                int primaryVisibility)
{
   TRACEM(AlembicGeometrySource, instanciateGeometry);
   
   // UpdateGeometry was called, what are the geometry to be instanced?
   // We have a scene filter set
   // Even with that, depending on frame, geometry might visible or not
   // Do not instanciate geom for invisible bits!
   // UpdateGeometry should tag GeomInfo
   // source will always be there as we keep the plugins around until the end
   
   std::string baseAttrStr = (userAttr ? userAttr : "");
   const char *sep = "";
   
   StringUserAttrs baseAttrs;
   StringUserAttrs::iterator attrIt;
   
   ParseStringUserAttrs(baseAttrStr, baseAttrs);
   
   for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      GeomInfo &info = it->second;
      
      if (info.source && !info.ignore)
      {
         StringUserAttrs geomAttrs;
         
         ParseStringUserAttrs(info.userAttr, geomAttrs);
         
         std::string userAttrStr;
         
         sep = "";
         
         for (attrIt=geomAttrs.begin(); attrIt!=geomAttrs.end(); ++attrIt)
         {
            userAttrStr += sep;
            userAttrStr += info.userAttr.substr(attrIt->second.start, attrIt->second.end - attrIt->second.start + 1);
            sep = ";";
         }
         
         for (attrIt=baseAttrs.begin(); attrIt!=baseAttrs.end(); ++attrIt)
         {
            if (geomAttrs.find(attrIt->first) != geomAttrs.end())
            {
               continue;
            }
            userAttrStr += sep;
            userAttrStr += baseAttrStr.substr(attrIt->second.start, attrIt->second.end - attrIt->second.start + 1);
            sep = ";";
         }
         
         info.instance = info.source->newInstance(mtl, bsdf, renderID, volume, lightList, baseTM, objectID, userAttrStr.c_str(), primaryVisibility);
      }
   }
}

bool AlembicGeometrySource::compileMatrices(AlembicGeometrySource::GeomInfo &info, VR::TraceTransform* &tm, double* &times, int &tmCount)
{
   if (!info.matrices)
   {
      return false;
   }
   else
   {
      bool allocated = false;
      
      if (tmCount == 0)
      {
         tm = info.matrices;
         tmCount = info.numMatrices;
         times = sampleFrames();
      }
      else if (info.numMatrices == 0)
      {
         // no changes
      }
      else if (tmCount == 1)
      {
         VR::TraceTransform *_tm = new VR::TraceTransform[info.numMatrices];
         allocated = true;
         
         for (int i=0; i<info.numMatrices; ++i)
         {
            _tm[i] = tm[0] * info.matrices[i];
         }
         
         tm = _tm;
         tmCount = info.numMatrices;
         times = sampleFrames();
      }
      else if (info.numMatrices == 1)
      {
         VR::TraceTransform *_tm = new VR::TraceTransform[tmCount];
         allocated = true;
         
         for (int i=0; i<tmCount; ++i)
         {
            _tm[i] = tm[i] * info.matrices[0];
         }
         
         tm = _tm;
         // tmCount and times unchanged
      }
      else if (info.numMatrices != tmCount)
      {
         std::cerr << "[AlembicLoader] AlembicGeometrySource::compileMatrices: Matrix samples count mismatch (" << info.numMatrices << " / " << tmCount << ")" << std::endl;
         // Should interpolate?
         tm = info.matrices;
         tmCount = info.numMatrices;
         times = sampleFrames();
      }
      else
      {
         // info.numMatrices == tmCount > 1
         
         VR::TraceTransform *_tm = new VR::TraceTransform[info.numMatrices];
         allocated = true;
         
         for (int i=0; i<info.numMatrices; ++i)
         {
            if (fabs(times[i] - sampleFrame(i)) > 0.001f)
            {
               std::cerr << "[AlembicLoader] AlembicGeometrySource::compileMatrices: Time mismatch for matrix sample " << i << " (" << sampleTime(i) << " / " << times[i] << ")" << std::endl;
               // Should interpolate?
               _tm[i] = info.matrices[i];
            }
            else
            {
               _tm[i] = tm[i] * info.matrices[i];
            }
         }
         
         tm = _tm;
         tmCount = info.numMatrices;
         times = sampleFrames();
      }
      
      return allocated;
   }
}

void AlembicGeometrySource::compileGeometry(VR::VRayRenderer *vray, VR::TraceTransform *tm, double *times, int tmCount)
{
   TRACEM(AlembicGeometrySource, instanciateGeometry);
   
   for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      GeomInfo &info = it->second;
      
      if (!info.instance)
      {
         continue;
      }
      
      VR::TraceTransform *_tm = tm;
      int _tmCount = tmCount;
      double *_times = times;
      
      bool allocated = compileMatrices(info, _tm, _times, _tmCount);
      
      info.instance->compileGeometry(vray, _tm, _times, _tmCount);
      
      if (allocated)
      {
         delete[] _tm;
      }
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
   
   for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      GeomInfo &info = it->second;
      
      if (!info.instance)
      {
         continue;
      }
      
      info.instance->updateMaterial(mtl, bsdf, renderID, volume, lightList, objectID);
   }
}

void AlembicGeometrySource::clearGeometry(VR::VRayRenderer *vray)
{
   TRACEM(AlembicGeometrySource, clearGeometry);
   
   for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      GeomInfo &info = it->second;
      
      if (!info.instance)
      {
         continue;
      }
      
      info.instance->clearGeometry(vray);
   }
}

void AlembicGeometrySource::destroyGeometryInstance()
{
   TRACEM(AlembicGeometrySource, destroyGeometryInstance);
   
   for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      GeomInfo &info = it->second;
      
      if (info.source && info.instance)
      {
         info.source->deleteInstance(info.instance);
         info.instance = 0;
      }
   }
}

void AlembicGeometrySource::endFrame(VR::VRayRenderer *vray)
{
   TRACEM(AlembicGeometrySource, endFrame);
   
   for (std::map<std::string, GeomInfo>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      it->second.clear();
   }
   
   mSampleTimes.clear();
   mSampleFrames.clear();
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
