#include "plugin.h"
#include "geomsrc.h"
#include "visitors.h"
#include <cstdlib>

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

// ---

AbcVRayGeom::AbcVRayGeom()
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

AbcVRayGeom::~AbcVRayGeom()
{
   clear();
}

void AbcVRayGeom::cleanTransformTemporaries()
{
   if (matrices)
   {
      delete[] matrices;
      matrices = 0;
   }
}

void AbcVRayGeom::cleanGeometryTemporaries()
{
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
   
   if (particleOrder)
   {
      delete[] particleOrder;
      particleOrder = 0;
   }
   
   std::map<std::string, VR::DefFloatListParam*>::iterator fit = floatParams.begin();
   while (fit != floatParams.end())
   {
      if (attachedParams.find(fit->second) == attachedParams.end())
      {
         std::map<std::string, VR::DefFloatListParam*>::iterator tmp = fit;
         ++tmp;
         
         delete fit->second;
         floatParams.erase(fit);
         
         fit = tmp;
      }
      else
      {
         ++fit;
      }
   }
   
   std::map<std::string, VR::DefColorListParam*>::iterator cit = colorParams.begin();
   while (cit != colorParams.end())
   {
      if (attachedParams.find(cit->second) == attachedParams.end())
      {
         std::map<std::string, VR::DefColorListParam*>::iterator tmp = cit;
         ++tmp;
         
         delete cit->second;
         colorParams.erase(cit);
         
         cit = tmp;
      }
      else
      {
         ++cit;
      }
   }
}

void AbcVRayGeom::clear()
{
   cleanGeometryTemporaries();
   cleanTransformTemporaries();
   
   numPoints = 0;
   numTriangles = 0;
   numFaces = 0;
   numFaceVertices = 0;
   numMatrices = 0;
   userAttr = "";
   
   floatParams.clear();
   colorParams.clear();
   attachedParams.clear();
   
   // Don't touch other V-Ray objects
   // - Parameters should all be store in factory
   // - Plugins are tracked by the plugin manager
   
   resetFlags();
}

void AbcVRayGeom::resetFlags()
{
   ignore = true;
   updatedFrameGeometry = false;
   invalidFrame = true;
}

void AbcVRayGeom::sortParticles(size_t count, const Alembic::Util::uint64_t *ids)
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

std::string AbcVRayGeom::remapParamName(const std::string &name) const
{
   std::map<std::string, std::string>::const_iterator it = remapParamNames.find(name);
   return (it == remapParamNames.end() ? name : it->second);
}

bool AbcVRayGeom::isValidParamName(const std::string &name) const
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

bool AbcVRayGeom::isFloatParam(const std::string &name) const
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

bool AbcVRayGeom::isColorParam(const std::string &name) const
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

void AbcVRayGeom::attachParams(Factory *f, bool verbose)
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
               std::cout << "[AlembicLoader] AbcVRayGeom::attachParams: Float parameter \"" << it->first << "\" attached" << std::endl;
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
               std::cout << "[AlembicLoader] AbcVRayGeom::attachParams: Color parameter \"" << it->first << "\" attached" << std::endl;
            }
         }
      }
   }
}

// ---

AlembicLoader::AlembicLoader(VR::VRayPluginDesc *desc)
   : VR::VRayStaticGeomSource(desc)
   , mPluginMgr(0)
   , mRenderTime(0)
   , mRenderFrame(0)
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

VR::VRayPlugin* AlembicLoader::createPlugin(const tchar *pluginType)
{
   TRACEM_MSG(AlembicLoader, createPlugin, pluginType);
   
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

void AlembicLoader::deletePlugin(VR::VRayPlugin *plugin)
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

void AlembicLoader::resetFlags()
{
   for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      it->second.resetFlags();
   }
}

void AlembicLoader::clear()
{
   for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      it->second.clear();
   }
}

AbcVRayGeom* AlembicLoader::addGeometry(const std::string &path, VR::VRayPlugin *geom, VR::VRayPlugin *mod)
{
   TRACEM_MSG(AlembicLoader, addGeometry, path);
   
   std::map<std::string, AbcVRayGeom>::iterator it = mGeoms.find(path);
   if (it != mGeoms.end())
   {
      return 0;
   }
   
   AbcVRayGeom &info = mGeoms[path];
   
   info.geometry = geom;
   info.modifier = mod;
   info.out = (mod ? mod : geom);
   info.source = static_cast<VR::StaticGeomSourceInterface*>(GET_INTERFACE(info.out, EXT_STATIC_GEOM_SOURCE));
   
   return &info;
}

void AlembicLoader::removeGeometry(const std::string &path)
{
   std::map<std::string, AbcVRayGeom>::iterator it = mGeoms.find(path);
   if (it != mGeoms.end())
   {
      // this should leave V-Ray plugins and parameters around
      mGeoms.erase(it);
   }
}

AbcVRayGeom* AlembicLoader::getGeometry(const std::string &path)
{
   std::map<std::string, AbcVRayGeom>::iterator it = mGeoms.find(path);
   return (it != mGeoms.end() ? &(it->second) : 0);
}

const AbcVRayGeom* AlembicLoader::getGeometry(const std::string &path) const
{
   std::map<std::string, AbcVRayGeom>::const_iterator it = mGeoms.find(path);
   return (it != mGeoms.end() ? &(it->second) : 0);
}

bool AlembicLoader::hasGeometry() const
{
   return (mGeoms.size() > 0);
}

void AlembicLoader::dumpVRScene(const char *path, double frame)
{
   if (!mPluginMgr)
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
   
   VR::SceneInfoWithPlugMan sceneInfo(*mPluginMgr, NULL, &settings);
   
   VR::VRayPluginsExporterBase *exporter = VR::VRayPluginsExporterBase::newVRayPluginsExporter(sceneInfo, outpath.c_str());
   
   exporter->doExport(1, frame, frame, frame, int(frame));
   
   VR::VRayPluginsExporterBase::deleteVRayPluginsExporter(exporter);
}

void AlembicLoader::preRenderBegin(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, preRenderBegin);
   
   paramList->cacheParams();
   
   VR::VRayPluginRendererInterface *pluginRenderer = (VR::VRayPluginRendererInterface*) GET_INTERFACE(vray, EXT_PLUGIN_RENDERER);
   
   if (pluginRenderer)
   {
      mPluginMgr = pluginRenderer->getPluginManager();
      
      // This won't read much data but for reference matrices
      AlembicGeometrySource src(this, false);
      
      AlembicScene *scene = src.scene();
      
      if (scene)
      {
         BuildPlugins bldplg(&src, vray);
         
         scene->visit(AlembicNode::VisitDepthFirst, bldplg);
      }
   }
   else
   {
      mPluginMgr = 0;
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
   
   // setup time samples
   
   const VR::VRayFrameData &frameData = vray->getFrameData();
   const VR::VRaySequenceData &seqData = vray->getSequenceData();
   
   bool ignoreMotionBlur = (mParams.ignoreDeformBlur && (mParams.ignoreTransforms || mParams.ignoreTransformBlur));
   
   mRenderFrame = vray->getTimeInFrames(frameData.t);
   mRenderTime = mRenderFrame / mParams.fps;
   
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
         mSampleTimes.push_back(f / mParams.fps);
         t += step;
      }
   }
   else
   {
      mSampleTimes.push_back(mRenderTime);
      mSampleFrames.push_back(mRenderFrame);
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
   
   // Ignore reference
   AlembicGeometrySource *src = new AlembicGeometrySource(this, true);
   
   if (src)
   {
      AlembicScene *scene = src->scene();
      
      if (scene && hasGeometry())
      {
         resetFlags();
         
         // Read frame geometry
         UpdateGeometry updgeo(src);
         scene->visit(AlembicNode::VisitDepthFirst, updgeo);
         
         
         std::string baseAttrStr = (userAttr ? userAttr : "");
         const char *sep = "";
         
         StringUserAttrs baseAttrs;
         StringUserAttrs::iterator attrIt;
         
         ParseStringUserAttrs(baseAttrStr, baseAttrs);
         
         for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
         {
            AbcVRayGeom &geom = it->second;
            
            if (geom.source && !geom.ignore)
            {
               StringUserAttrs geomAttrs;
               
               ParseStringUserAttrs(geom.userAttr, geomAttrs);
               
               std::string userAttrStr;
               
               sep = "";
               
               for (attrIt=geomAttrs.begin(); attrIt!=geomAttrs.end(); ++attrIt)
               {
                  userAttrStr += sep;
                  userAttrStr += geom.userAttr.substr(attrIt->second.start, attrIt->second.end - attrIt->second.start + 1);
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
               
               geom.instance = geom.source->newInstance(mtl, bsdf, renderID, volume, lightList, baseTM, objectID, userAttrStr.c_str(), primaryVisibility);
               
               geom.cleanGeometryTemporaries();
            }
         }
         
         src->cleanAlembicTemporaries();
      }
      else if (src)
      {
         delete src;
         src = 0;
      }
   }
   
   return src;
}

bool AlembicLoader::compileMatrices(AbcVRayGeom &geom, VR::TraceTransform* &tm, double* &times, int &tmCount)
{
   if (!geom.matrices)
   {
      return false;
   }
   else
   {
      bool allocated = false;
      
      if (tmCount == 0)
      {
         tm = geom.matrices;
         tmCount = geom.numMatrices;
         times = sampleFrames();
      }
      else if (geom.numMatrices == 0)
      {
         // no changes
      }
      else if (tmCount == 1)
      {
         VR::TraceTransform *_tm = new VR::TraceTransform[geom.numMatrices];
         allocated = true;
         
         for (int i=0; i<geom.numMatrices; ++i)
         {
            _tm[i] = tm[0] * geom.matrices[i];
         }
         
         tm = _tm;
         tmCount = geom.numMatrices;
         times = sampleFrames();
      }
      else if (geom.numMatrices == 1)
      {
         VR::TraceTransform *_tm = new VR::TraceTransform[tmCount];
         allocated = true;
         
         for (int i=0; i<tmCount; ++i)
         {
            _tm[i] = tm[i] * geom.matrices[0];
         }
         
         tm = _tm;
         // tmCount and times unchanged
      }
      else if (geom.numMatrices != tmCount)
      {
         std::cerr << "[AlembicLoader] AlembicLoader::compileMatrices: Matrix samples count mismatch (" << geom.numMatrices << " / " << tmCount << ")" << std::endl;
         // Should interpolate?
         tm = geom.matrices;
         tmCount = geom.numMatrices;
         times = sampleFrames();
      }
      else
      {
         // geom.numMatrices == tmCount > 1
         
         VR::TraceTransform *_tm = new VR::TraceTransform[geom.numMatrices];
         allocated = true;
         
         for (int i=0; i<geom.numMatrices; ++i)
         {
            if (fabs(times[i] - sampleFrame(i)) > 0.001f)
            {
               std::cerr << "[AlembicLoader] AlembicLoader::compileMatrices: Time mismatch for matrix sample " << i << " (" << sampleTime(i) << " / " << times[i] << ")" << std::endl;
               // Should interpolate?
               _tm[i] = geom.matrices[i];
            }
            else
            {
               _tm[i] = tm[i] * geom.matrices[i];
            }
         }
         
         tm = _tm;
         tmCount = geom.numMatrices;
         times = sampleFrames();
      }
      
      return allocated;
   }
}

void AlembicLoader::compileGeometry(VR::VRayRenderer *vray, VR::TraceTransform *tm, double *times, int tmCount)
{
   TRACEM(AlembicLoader, compileGeometry);
   
   for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      AbcVRayGeom &geom = it->second;
      
      if (!geom.instance)
      {
         continue;
      }
      
      VR::TraceTransform *_tm = tm;
      int _tmCount = tmCount;
      double *_times = times;
      
      bool allocated = compileMatrices(geom, _tm, _times, _tmCount);
      
      geom.instance->compileGeometry(vray, _tm, _times, _tmCount);
      
      if (allocated)
      {
         delete[] _tm;
      }
      
      geom.cleanTransformTemporaries();
   }
}

void AlembicLoader::updateMaterial(VR::MaterialInterface *mtl,
                                   VR::BSDFInterface *bsdf,
                                   int renderID,
                                   VR::VolumetricInterface *volume,
                                   VR::LightList *lightList,
                                   int objectID)
{
   TRACEM(AlembicLoader, updateMaterial);
   
   for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      AbcVRayGeom &geom = it->second;
      
      if (!geom.instance)
      {
         continue;
      }
      
      geom.instance->updateMaterial(mtl, bsdf, renderID, volume, lightList, objectID);
   }
}

void AlembicLoader::clearGeometry(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, clearGeometry);
   
   for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
   {
      AbcVRayGeom &geom = it->second;
      
      if (!geom.instance)
      {
         continue;
      }
      
      geom.instance->clearGeometry(vray);
   }
}

void AlembicLoader::deleteInstance(VR::VRayStaticGeometry *instance)
{
   TRACEM(AlembicLoader, deleteInstance);
   
   if (instance)
   {
      for (std::map<std::string, AbcVRayGeom>::iterator it=mGeoms.begin(); it!=mGeoms.end(); ++it)
      {
         AbcVRayGeom &geom = it->second;
         
         if (geom.source && geom.instance)
         {
            geom.source->deleteInstance(geom.instance);
            geom.instance = 0;
         }
      }
      
      delete (AlembicGeometrySource*) instance;
   }
}

void AlembicLoader::frameEnd(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, frameEnd);
   
   clear();
   
   mSampleTimes.clear();
   mSampleFrames.clear();
   
   VR::VRayStaticGeomSource::frameEnd(vray);
}

void AlembicLoader::renderEnd(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, renderEnd);
   
   VR::VRayStaticGeomSource::renderEnd(vray);
}

void AlembicLoader::postRenderEnd(VR::VRayRenderer *vray)
{
   TRACEM(AlembicLoader, postRenderEnd);
   
   int debug = 0;
      
   char *envval = getenv("VRAY_ALEMBIC_LOADER_DEBUG");
   
   if (envval && sscanf(envval, "%d", &debug) == 1 && debug != 0)
   {
      const VR::VRayFrameData &frameData = vray->getFrameData();
      
      dumpVRScene(0, vray->getTimeInFrames(frameData.t));
   }
   
   // Delete all create plugins and parameters
   
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
