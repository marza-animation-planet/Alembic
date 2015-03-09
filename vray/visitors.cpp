#include "visitors.h"

BaseVisitor::BaseVisitor(AlembicGeometrySource *geosrc)
   : mGeoSrc(geosrc)
{
}

BaseVisitor::~BaseVisitor()
{
}

bool BaseVisitor::getVisibility(Alembic::Abc::ICompoundProperty props, double t)
{
   Alembic::Util::bool_t visible = true;
   
   if (props.valid())
   {
      const Alembic::Abc::PropertyHeader *header = props.getPropertyHeader("visible");
      
      if (header)
      {
         if (Alembic::Abc::ICharProperty::matches(*header))
         {
            Alembic::Abc::ICharProperty prop(props, "visible");
            
            Alembic::Util::int8_t v = 1;
            
            if (prop.isConstant())
            {
               prop.get(v);
            }
            else
            {
               size_t nsamples = prop.getNumSamples();
               Alembic::Abc::TimeSamplingPtr timeSampling = prop.getTimeSampling();
               std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime = timeSampling->getNearIndex(t, nsamples);
               prop.get(v, Alembic::Abc::ISampleSelector(indexTime.first));
            }
            
            visible = (v != 0);
         }
         else if (Alembic::Abc::IBoolProperty::matches(*header))
         {
            Alembic::Abc::IBoolProperty prop(props, "visible");
            
            if (prop.isConstant())
            {
               prop.get(visible);
            }
            else
            {
               size_t nsamples = prop.getNumSamples();
               Alembic::Abc::TimeSamplingPtr timeSampling = prop.getTimeSampling();
               std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime = timeSampling->getNearIndex(t, nsamples);
               prop.get(visible, Alembic::Abc::ISampleSelector(indexTime.first));
            }
         }
      }
   }
   
   return visible;
}

// ---

BuildPlugins::BuildPlugins(AlembicGeometrySource *geosrc,
                           VR::VRayRenderer *vray)
   : BaseVisitor(geosrc)
   , mVRay(vray)
{  
}

BuildPlugins::~BuildPlugins()
{
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicXform &node, AlembicNode *instance)
{
   return AlembicNode::ContinueVisit;
}

template <class Schema>
static AlembicGeometrySource::GeomInfo* BuildMeshPlugins(AlembicGeometrySource *src, AlembicNodeT<Schema> &node, AlembicNode *instance)
{
   AlembicGeometrySource::GeomInfo *info = src->getInfo(node.path());
   
   if (!info)
   {
      Factory *factory = src->factory();
      AlembicLoaderParams::Cache* params = src->params();
      
      VR::VRayPlugin *mesh = src->createPlugin("GeomStaticMesh");
      VR::VRayPlugin *mod = 0;
      
      mesh->setPluginName(node.path().c_str());
      
      mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("dynamic_geometry", 1)));
      //mesh->setParameter(src->factory()->saveInFactory(new VR::DefIntParam("primary_visibility", 1)));
      
      if (params->subdivEnable && (params->displacementType == 0 || !params->displace2d))
      {
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("smooth_uv", params->subdivUVs)));
         
         mod = src->createPlugin("GeomStaticSmoothedMesh");
         mod->setPluginName((node.path() + "@subdiv").c_str());
         mod->setParameter(factory->saveInFactory(new VR::DefPluginParam("mesh", mesh)));
         
         if (params->subdivUVs)
         {
            mod->setParameter(factory->saveInFactory(new VR::DefIntParam("preserve_map_borders", params->preserveMapBorders)));
         }
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("static_subdiv", params->staticSubdiv)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("classic_catmark", params->classicCatmark)));           
      }
      
      if (params->osdSubdivEnable)
      {
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_subdiv_enable", 1)));
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_subdiv_level", params->osdSubdivLevel)));
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_subdiv_type", params->osdSubdivType)));
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_subdiv_uvs", params->osdSubdivUVs)));
         if (params->osdSubdivUVs)
         {
            mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_preserve_map_borders", params->osdPreserveMapBorders)));
         }
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_preserve_geometry_borders", params->osdPreserveGeometryBorders)));
      }
      else
      {
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_subdiv_enable", 0)));
         mesh->setParameter(factory->saveInFactory(new VR::DefIntParam("osd_subdiv_level", 0)));
      }
      
      if (params->displacementType == 1)
      {
         if (!mod)
         {
            mod = src->createPlugin("GeomDisplacedMesh");
            mod->setPluginName((node.path() + "@disp").c_str());
            mod->setParameter(factory->saveInFactory(new VR::DefPluginParam("mesh", mesh)));
         }
         
         if (params->displacementTexFloat)
         {
            mod->setParameter(factory->saveInFactory(new VR::DefPluginParam("displacement_tex_float", params->displacementTexFloat->getPlugin())));
         }
         
         if (params->displacementTexColor)
         {
            mod->setParameter(factory->saveInFactory(new VR::DefPluginParam("displacement_tex_color", params->displacementTexColor->getPlugin())));
         }
         
         mod->setParameter(factory->saveInFactory(new VR::DefFloatParam("displacement_amount", params->displacementAmount)));
         mod->setParameter(factory->saveInFactory(new VR::DefFloatParam("displacement_shift", params->displacementShift)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("keep_continuity", params->keepContinuity)));
         mod->setParameter(factory->saveInFactory(new VR::DefFloatParam("water_level", params->waterLevel)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("vector_displacement", params->vectorDisplacement)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("use_bounds", params->useBounds)));
         if (params->useBounds)
         {
            mod->setParameter(factory->saveInFactory(new VR::DefColorParam("min_bound", params->minBound)));
            mod->setParameter(factory->saveInFactory(new VR::DefColorParam("max_bound", params->maxBound)));
         }
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("image_width", params->imageWidth)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("cache_normals", params->cacheNormals)));
         //mod->setParameter(factory->saveInFactory(new VR::DefIntParam("map_channel", params->mapChannel)));
         //mod->setParameter(factory->saveInFactory(new VR::DefIntParam("object_space_displacement", params->objectSpaceDisplacement)));
      }
      
      if (!params->useGlobals && mod)
      {
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("use_globals", 0)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("view_dep", params->viewDep)));
         mod->setParameter(factory->saveInFactory(new VR::DefFloatParam("edge_length", params->edgeLength)));
         mod->setParameter(factory->saveInFactory(new VR::DefIntParam("max_subdivs", params->maxSubdivs)));
      }
      
      info = src->addInfo(node.path(), mesh, mod);
      
      if (node.typedObject().getSchema().isConstant())
      {
         info->constPositions = factory->saveInFactory(new VR::DefVectorListParam("vertices"));
         info->constNormals = factory->saveInFactory(new VR::DefVectorListParam("normals"));
         info->constFaceNormals = factory->saveInFactory(new VR::DefIntListParam("faceNormals"));
         
         mesh->setParameter(info->constPositions);
         mesh->setParameter(info->constNormals);
         mesh->setParameter(info->constFaceNormals);
      }
      else
      {
         info->positions = factory->saveInFactory(new VectorListParam("vertices"));
         info->normals = factory->saveInFactory(new VectorListParam("normals"));
         info->faceNormals = factory->saveInFactory(new IntListParam("faceNormals"));
         
         mesh->setParameter(info->positions);
         mesh->setParameter(info->normals);
         mesh->setParameter(info->faceNormals);
      }
      
      info->faces = factory->saveInFactory(new VR::DefIntListParam("faces"));
      info->channels = factory->saveInFactory(new VR::DefMapChannelsParam("map_channels"));
      info->channelNames = factory->saveInFactory(new VR::DefStringListParam("map_channels_names"));
      info->edgeVisibility = factory->saveInFactory(new VR::DefIntListParam("edge_visibility"));
      
      mesh->setParameter(info->faces);
      mesh->setParameter(info->channels);
      mesh->setParameter(info->channelNames);
      mesh->setParameter(info->edgeVisibility);
      
      // Read reference mesh here
      if (src->referenceScene())
      {
         AlembicNode *refNode = src->referenceScene()->find(node.path());
         
         if (refNode)
         {
            AlembicMesh *refMesh = dynamic_cast<AlembicMesh*>(refNode);
            AlembicSubD *refSubd = (refMesh ? 0 : dynamic_cast<AlembicSubD*>(refNode));
            
            if (refMesh || refSubd)
            {
               VR::VRayPlugin *refPlugin = src->createPlugin("GeomStaticMesh");
               
               refPlugin->setPluginName((node.path()+"@reference").c_str());
               
               std::string infoKey = node.path() + "@reference";
               
               AlembicGeometrySource::GeomInfo *refInfo = src->addInfo(infoKey, refPlugin);
               
               if (refInfo)
               {
                  refInfo->constPositions = factory->saveInFactory(new VR::DefVectorListParam("vertices"));
                  refInfo->faces = factory->saveInFactory(new VR::DefIntListParam("faces"));
                  refInfo->constNormals = factory->saveInFactory(new VR::DefVectorListParam("normals"));
                  refInfo->constFaceNormals = factory->saveInFactory(new VR::DefIntListParam("faceNormals"));
                  refInfo->channels = factory->saveInFactory(new VR::DefMapChannelsParam("map_channels"));
                  refInfo->channelNames = factory->saveInFactory(new VR::DefStringListParam("map_channels_names"));
                  refInfo->edgeVisibility = factory->saveInFactory(new VR::DefIntListParam("edge_visibility"));
                  
                  refPlugin->setParameter(factory->saveInFactory(new VR::DefIntParam("dynamic_geometry", 1)));
                  //refPlugin->setParameter(src->factory()->saveInFactory(new VR::DefIntParam("primary_visibility", 1)));
                  refPlugin->setParameter(refInfo->constPositions);
                  refPlugin->setParameter(refInfo->constNormals);
                  refPlugin->setParameter(refInfo->faces);
                  refPlugin->setParameter(refInfo->constFaceNormals);
                  refPlugin->setParameter(refInfo->channels);
                  refPlugin->setParameter(refInfo->channelNames);
                  refPlugin->setParameter(refInfo->edgeVisibility);
                  
                  UpdateGeometry geoup(src);
                  
                  if (!geoup.readBaseMesh(node, refInfo))
                  {
                     factory->removeVRayPluginParameter(refInfo->constPositions);
                     factory->removeVRayPluginParameter(refInfo->faces);
                     factory->removeVRayPluginParameter(refInfo->constNormals);
                     factory->removeVRayPluginParameter(refInfo->constFaceNormals);
                     factory->removeVRayPluginParameter(refInfo->channels);
                     factory->removeVRayPluginParameter(refInfo->channelNames);
                     factory->removeVRayPluginParameter(refInfo->edgeVisibility);
                     
                     src->deletePlugin(refPlugin);
                     
                     refPlugin = 0;
                  }
                  else
                  {
                     if (refMesh)
                     {
                        geoup.readMeshNormals(*refMesh, refInfo);
                     }
                     geoup.readMeshUVs(node, refInfo);
                  }
                  
                  src->removeInfo(infoKey);
                  
                  if (refPlugin)
                  {
                     mesh->setParameter(factory->saveInFactory(new VR::DefPluginParam("reference_mesh", refPlugin)));
                     
                     std::map<std::string, Alembic::Abc::M44d>::const_iterator mit = src->referenceTransforms().find(instance ? instance->path() : node.path());
                     
                     if (mit != src->referenceTransforms().end())
                     {
                        VR::TraceTransform refMatrix;
                        
                        refMatrix.m.setCol(0, VR::Vector(mit->second[0][0], mit->second[0][1], mit->second[0][2]));
                        refMatrix.m.setCol(1, VR::Vector(mit->second[1][0], mit->second[1][1], mit->second[1][2]));
                        refMatrix.m.setCol(2, VR::Vector(mit->second[2][0], mit->second[2][1], mit->second[2][2]));
                        refMatrix.offs = VR::Vector(mit->second[3][0], mit->second[3][1], mit->second[3][2]);
                        
                        mesh->setParameter(factory->saveInFactory(new VR::DefTransformParam("reference_transform", refMatrix)));
                     }
                  }
               }
               else
               {
                  src->deletePlugin(refPlugin);
                  
                  refPlugin = 0;
               }
            }
         }
      }
   }
   
   if (instance && info)
   {
      AlembicGeometrySource::GeomInfo *iinfo = src->getInfo(instance->path());
      
      if (!iinfo)
      {
         iinfo = src->addInfo(instance->path(), info->out);
      }
      
      // do not return info pointer for instances
      return 0;
   }
   else
   {
      return info;
   }
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicMesh &node, AlembicNode *instance)
{
   BuildMeshPlugins(mGeoSrc, node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicSubD &node, AlembicNode *instance)
{
   AlembicGeometrySource::GeomInfo *info = BuildMeshPlugins(mGeoSrc, node, instance);
   
   if (info)
   {
      // Additionaly setup crease parameters...
      //   edge_creases_vertices
      //   edge_creases_sharpness
      //   vertex_creases_vertices
      //   vertex_creases_sharpness
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicPoints &node, AlembicNode *instance)
{
   AlembicGeometrySource::GeomInfo *info = mGeoSrc->getInfo(node.path());
   
   if (!info)
   {
      Factory *factory = mGeoSrc->factory();
      AlembicLoaderParams::Cache* params = mGeoSrc->params();
      
      VR::VRayPlugin *points = mGeoSrc->createPlugin("GeomParticleSystem");
      
      points->setPluginName(node.path().c_str());
      
      info = mGeoSrc->addInfo(node.path(), points);
      
      if (node.typedObject().getSchema().isConstant())
      {
         info->constPositions = factory->saveInFactory(new VR::DefVectorListParam("positions"));
         
         points->setParameter(info->constPositions);
      }
      else
      {
         info->positions = factory->saveInFactory(new VectorListParam("positions"));
         
         points->setParameter(info->positions);
      }
      
      info->velocities = factory->saveInFactory(new VR::DefVectorListParam("velocities"));
      points->setParameter(info->velocities);
      
      info->accelerations = factory->saveInFactory(new VR::DefColorListParam("acceleration_pp"));
      points->setParameter(info->accelerations);
      
      info->ids = factory->saveInFactory(new VR::DefIntListParam("ids"));
      points->setParameter(info->ids);
      
      std::string tmp0, tmp1, an, vn;
      size_t p0, p1, p2;
      
      info->remapParamNames.clear();
      
      tmp0 = (params->particleAttribs.ptr() ? params->particleAttribs.ptr() : "");
      p0 = 0;
      p1 = tmp0.find(';', p0);
      while (p1 != std::string::npos)
      {
         tmp1 = tmp0.substr(p0, p1-p0);
         p2 = tmp1.find('=');
         if (p2 != std::string::npos)
         {
            vn = tmp1.substr(0, p2);
            an = tmp1.substr(p2+1);
            info->remapParamNames[an] = vn;
            std::cout << "[AlembicLoader] BuildPlugins::enter(AlembicPoints): Set '" << vn << "' parameter from alembic '" << an << "' attribute" << std::endl;
         }
         p0 = p1 + 1;
         p1 = tmp0.find(';', p0);
      }
      tmp1 = tmp0.substr(p0);
      p2 = tmp1.find('=');
      if (p2 != std::string::npos)
      {
         vn = tmp1.substr(0, p2);
         an = tmp1.substr(p2+1);
         info->remapParamNames[an] = vn;
         std::cout << "[AlembicLoader] BuildPlugins::enter(AlembicPoints): Set '" << vn << "' parameter from alembic '" << an << "' attribute" << std::endl;
      }
      
      info->sortIDs = (params->sortIDs != 0);
      
      points->setParameter(factory->saveInFactory(new VR::DefIntParam("render_type", params->particleType)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("sprite_size_x", params->spriteSizeX)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("sprite_size_y", params->spriteSizeY)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("sprite_twist", params->spriteTwist)));
      points->setParameter(factory->saveInFactory(new VR::DefIntParam("sprite_orientation", params->spriteOrientation)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("radius", params->radius)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("point_size", params->pointSize)));
      points->setParameter(factory->saveInFactory(new VR::DefIntParam("point_radii", params->pointRadii)));
      points->setParameter(factory->saveInFactory(new VR::DefIntParam("point_world_size", params->pointWorldSize)));
      points->setParameter(factory->saveInFactory(new VR::DefIntParam("multi_count", params->multiCount)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("multi_radius", params->multiRadius)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("line_width", params->lineWidth)));
      points->setParameter(factory->saveInFactory(new VR::DefFloatParam("tail_length", params->tailLength)));
   }
   
   if (instance && info)
   {
      AlembicGeometrySource::GeomInfo *iinfo = mGeoSrc->getInfo(instance->path());
      
      if (!iinfo)
      {
         mGeoSrc->addInfo(instance->path(), info->out);
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicCurves &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicNuPatch &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn BuildPlugins::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mGeoSrc->params()->ignoreInstances)
      {
         return node.master()->enter(*this, &node);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}
   
void BuildPlugins::leave(AlembicNode &node, AlembicNode *instance)
{
}

// ---

CollectWorldMatrices::CollectWorldMatrices(AlembicGeometrySource *geosrc)
   : BaseVisitor(geosrc)
{
}
   
AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicXform &node, AlembicNode *)
{
   if (!node.isLocator() && !mGeoSrc->params()->ignoreTransforms)
   {
      Alembic::AbcGeom::IXformSchema schema = node.typedObject().getSchema();
      
      Alembic::Abc::M44d parentWorldMatrix;
      
      if (mMatrixStack.size() > 0)
      {
         parentWorldMatrix = mMatrixStack.back();
      }
      
      Alembic::Abc::M44d localMatrix = schema.getValue().getMatrix();
      
      mMatrixStack.push_back(schema.getInheritsXforms() ? localMatrix * parentWorldMatrix : localMatrix);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicMesh &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicSubD &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicPoints &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicCurves &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mGeoSrc->params()->ignoreInstances)
      {
         AlembicNode *m = node.master();
         return m->enter(*this, &node);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}

void CollectWorldMatrices::leave(AlembicXform &node, AlembicNode *)
{
   if (!node.isLocator() && !!mGeoSrc->params()->ignoreTransforms)
   {
      mMatrixStack.pop_back();
   }
}

void CollectWorldMatrices::leave(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance() && !mGeoSrc->params()->ignoreInstances)
   {
      AlembicNode *m = node.master();
      m->leave(*this, &node);
   }
}

// ---

UpdateGeometry::UpdateGeometry(AlembicGeometrySource *geosrc)
   : BaseVisitor(geosrc)
{
}

UpdateGeometry::~UpdateGeometry()
{
}

void UpdateGeometry::collectUserAttributes(Alembic::Abc::ICompoundProperty userProps,
                                           Alembic::Abc::ICompoundProperty geomParams,
                                           double t,
                                           bool interpolate,
                                           UserAttrsSet &attrs,
                                           bool object,
                                           bool primitive,
                                           bool point,
                                           bool vertex,
                                           bool uvs)
{
   if (userProps.valid() && object)
   {
      for (size_t i=0; i<userProps.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = userProps.getPropertyHeader(i);
         
         std::pair<std::string, UserAttribute> ua;
         
         ua.first = header.getName();
         // mGeoSrc->cleanAttribName(ua.first);
         InitUserAttribute(ua.second);
         
         if (ReadUserAttribute(ua.second, userProps, header, t, false, interpolate, mGeoSrc->params()->verbose))
         {
            attrs.object.insert(ua);
         }
         else
         {
            DestroyUserAttribute(ua.second);
         }
      }
   }
   
   if (geomParams.valid())
   {
      for (size_t i=0; i<geomParams.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = geomParams.getPropertyHeader(i);
         
         Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
         
         if (uvs &&
             scope == Alembic::AbcGeom::kFacevaryingScope &&
             Alembic::AbcGeom::IV3fGeomParam::matches(header) &&
             Alembic::AbcGeom::isUV(header))
         {
            std::pair<std::string, Alembic::AbcGeom::IV2fGeomParam> UV;
            
            UV.first = header.getName();
            // mGeoSrc->cleanAttribName(UV.first);
            UV.second = Alembic::AbcGeom::IV2fGeomParam(geomParams, header.getName());
            
            attrs.uvs.insert(UV);
         }
         else
         {
            std::pair<std::string, UserAttribute> ua;
            
            UserAttributes *targetAttrs = 0;
            
            ua.first = header.getName();
            InitUserAttribute(ua.second);
            
            switch (scope)
            {
            case Alembic::AbcGeom::kFacevaryingScope:
               if (vertex)
               {
                  targetAttrs = &(attrs.vertex);
               }
               break;
            case Alembic::AbcGeom::kVaryingScope:
            case Alembic::AbcGeom::kVertexScope:
               if (point)
               {
                  targetAttrs = &(attrs.point);
               }
               break;
            case Alembic::AbcGeom::kUniformScope:
               if (primitive)
               {
                  targetAttrs = &(attrs.primitive);
               }
               break;
            case Alembic::AbcGeom::kConstantScope:
               if (object)
               {
                  targetAttrs = &(attrs.object);
               }
               break;
            default:
               continue;
            }
            
            if (targetAttrs)
            {
               if (ReadUserAttribute(ua.second, geomParams, header, t, true, interpolate, mGeoSrc->params()->verbose))
               {
                  targetAttrs->insert(ua);
               }
               else
               {
                  DestroyUserAttribute(ua.second);
               }
            }
         }
      }
   }
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicXform &node, AlembicNode *instance)
{
   if (mGeoSrc->params()->verbose)
   {
      std::cout << "[AlembicLoader] UpdateGeometry::enter: AlembicXform \"" << (instance ? instance->path() : node.path()) << "\"";
      if (instance)
      {
         std::cout << " [instance of \"" << node.path() << "]";
      }
      std::cout << std::endl;
   }
   
   bool visible = isVisible(node);
   
   if (instance)
   {
      instance->setVisible(visible);
   }
   else
   {
      node.setVisible(visible);
   }
   
   if (visible && !node.isLocator() && !mGeoSrc->params()->ignoreTransforms)
   {
      TimeSampleList<Alembic::AbcGeom::IXformSchema> &xformSamples = node.samples().schemaSamples;
      
      mMatrixSamplesStack.push_back(std::vector<Alembic::Abc::M44d>());
      
      std::vector<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
      
      std::vector<Alembic::Abc::M44d> *parentMatrices = 0;
      if (mMatrixSamplesStack.size() >= 2)
      {
         parentMatrices = &(mMatrixSamplesStack[mMatrixSamplesStack.size()-2]);
      }
      
      double renderTime = mGeoSrc->renderTime();
      double renderFrame = mGeoSrc->renderFrame();
      const double *sampleTimes = 0;
      const double *sampleFrames = 0;
      size_t sampleCount = 0;
      
      if (!mGeoSrc->params()->ignoreTransformBlur)
      {
         sampleCount = mGeoSrc->numTimeSamples();
         sampleTimes = mGeoSrc->sampleTimes();
         sampleFrames = mGeoSrc->sampleFrames();
      }
      else
      {
         sampleCount = 1;
         sampleTimes = &renderTime;
         sampleFrames = &renderFrame;
      }
      
      for (size_t i=0; i<sampleCount; ++i)
      {
         double t = sampleTimes[i];
         node.sampleBounds(t, t, i > 0);
      }
      
      if (xformSamples.size() == 0)
      {
         // identity + inheritXforms
         if (parentMatrices)
         {
            for (size_t i=0; i<parentMatrices->size(); ++i)
            {
               matrices.push_back(parentMatrices->at(i));
            }
         }
         else
         {
            matrices.push_back(Alembic::Abc::M44d());
         }
      }
      else if (xformSamples.size() == 1)
      {
         bool inheritXforms = xformSamples.begin()->data().getInheritsXforms();
         Alembic::Abc::M44d matrix = xformSamples.begin()->data().getMatrix();
         
         if (inheritXforms && parentMatrices)
         {
            for (size_t i=0; i<parentMatrices->size(); ++i)
            {
               matrices.push_back(matrix * parentMatrices->at(i));
            }
         }
         else
         {
            matrices.push_back(matrix);
         }
      }
      else
      {
         Alembic::Abc::M44d matrix, m0, m1;
         bool inheritXforms = true;
         
         for (size_t i=0; i<sampleCount; ++i)
         {
            inheritXforms = true;
            
            double t = sampleTimes[i];
            
            TimeSampleList<Alembic::AbcGeom::IXformSchema>::ConstIterator samp0, samp1;
            double blend = 0.0;
            
            if (xformSamples.getSamples(t, samp0, samp1, blend))
            {
               inheritXforms = samp0->data().getInheritsXforms();
               
               if (blend > 0.0)
               {
                  if (samp0->data().getInheritsXforms() != samp1->data().getInheritsXforms())
                  {
                     matrix = samp0->data().getMatrix();
                  }
                  else
                  {
                     m0 = samp0->data().getMatrix();
                     m1 = samp1->data().getMatrix();
                     
                     // Decompose into TRS
                     //   Rotation as quaternion (axis angle) -> slerp
                     //   Shear?
                     Alembic::Abc::V3d scl0, shr0, scl1, shr1, tr0, tr1, scl, shr, tr;
                     Alembic::Abc::Quatd rot0, rot1, rot;
                     
                     if (!extractAndRemoveScalingAndShear(m0, scl0, shr0, false) || 
                         !extractAndRemoveScalingAndShear(m1, scl1, shr1, false))
                     {
                        // Fallback to matrix linear interpolation
                        matrix = (1.0 - blend) * m0 + blend * m1;
                     }
                     else
                     {
                        rot0 = extractQuat(m0);
                        rot1 = extractQuat(m1);
                        
                        tr0 = m0.translation();
                        tr1 = m1.translation();
                        
                        rot = slerp(rot0, rot1, blend);
                        scl = (1.0 - blend) * scl0 + blend * scl1;
                        shr = (1.0 - blend) * shr0 + blend * shr1;
                        tr = (1.0 - blend) * tr0 + blend * tr1;
                        
                        matrix.setTranslation(tr);
                        matrix.setAxisAngle(rot.axis(), rot.angle());
                        matrix.setScale(scl);
                        matrix.setShear(shr);
                     }
                  }
               }
               else
               {
                  matrix = samp0->data().getMatrix();
               }
            }
            else
            {
               matrix.makeIdentity();
            }
            
            if (inheritXforms && parentMatrices)
            {
               if (i < parentMatrices->size())
               {
                  matrix = matrix * parentMatrices->at(i);
               }
               else
               {
                  matrix = matrix * parentMatrices->at(0);
               }
            }
            
            matrices.push_back(matrix);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

void UpdateGeometry::readMeshNormals(AlembicMesh &node, AlembicGeometrySource::GeomInfo *info)
{
   Alembic::AbcGeom::IPolyMeshSchema &schema = node.typedObject().getSchema();
   
   bool isConst = (info->constNormals != 0);
   double renderFrame = mGeoSrc->renderFrame();
   
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
         
   Alembic::AbcGeom::IN3fGeomParam Nparam = schema.getNormalsParam();
   
   if (isConst)
   {
      if (Nparam.valid())
      {
         Alembic::AbcGeom::IN3fGeomParam::Sample theSample = Nparam.getIndexedValue();
         
         Alembic::Abc::N3fArraySamplePtr N = theSample.getVals();
         
         Alembic::Abc::UInt32ArraySamplePtr I = theSample.getIndices();
         
         info->constNormals->setCount(N->size(), renderFrame);
         VR::VectorList nl = info->constNormals->getVectorList(renderFrame);
         
         info->constFaceNormals->setCount(info->numTriangles * 3, renderFrame);
         VR::IntList il = info->constFaceNormals->getIntList(renderFrame);
         
         for (size_t i=0; i<N->size(); ++i)
         {
            Alembic::Abc::N3f n = N->get()[i];
            nl[i].set(n.x, n.y, n.z);
         }
         
         for (size_t i=0, k=0; i<info->numTriangles; ++i)
         {
            for (size_t j=0; j<3; ++j, ++k)
            {
               il[k] = I->get()[info->toVertexIndex[k]];
            }
         }
      }
      else
      {
         info->constNormals->setCount(0, renderFrame);
         info->constFaceNormals->setCount(0, renderFrame);
      }
   }
   else
   {
      bool varyingTopology = schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology;
      bool singleSample = (mGeoSrc->params()->ignoreDeformBlur || varyingTopology);
      double renderTime = mGeoSrc->renderTime();
      double *times = (singleSample ? &renderTime : mGeoSrc->sampleTimes());
      double *frames = (singleSample ? &renderFrame : mGeoSrc->sampleFrames());
      size_t ntimes = (singleSample ? 1 : mGeoSrc->numTimeSamples());
      double t;
      
      info->normals->clear();
      info->faceNormals->clear();
      
      if (Nparam.valid())
      {
         TimeSampleList<Alembic::AbcGeom::IN3fGeomParam>::ConstIterator Nsamp0, Nsamp1;
         double Nb = 0.0;
         
         for (size_t i=0; i<ntimes; ++i)
         {
            t = times[i];
            Nsamples.update(Nparam, t, t, i > 0);
         }
         
         if (Nsamples.size() == 1 || varyingTopology)
         {
            Nsamples.getSamples(renderTime, Nsamp0, Nsamp1, Nb);
            
            Alembic::Abc::N3fArraySamplePtr N = Nsamp0->data().getVals();
            
            Alembic::Abc::UInt32ArraySamplePtr I = Nsamp0->data().getIndices();
            
            VR::Table<VR::Vector> &nl = info->normals->at(renderFrame);
            nl.setCount(N->size(), true);
            
            VR::Table<int> &il = info->faceNormals->at(renderFrame);
            il.setCount(info->numTriangles * 3, true);
            
            for (size_t i=0; i<N->size(); ++i)
            {
               Alembic::Abc::N3f n = N->get()[i];
               nl[i].set(n.x, n.y, n.z);
            }
            
            for (size_t i=0, k=0; i<info->numTriangles; ++i)
            {
               for (size_t j=0; j<3; ++j, ++k)
               {
                  il[k] = I->get()[info->toVertexIndex[k]];
               }
            }
         }
         else
         {
            for (size_t s=0; s<ntimes; ++s)
            {
               t = times[s];
               
               Nsamples.getSamples(t, Nsamp0, Nsamp1, Nb);
               
               Alembic::Abc::N3fArraySamplePtr N0 = Nsamp0->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr I0 = Nsamp0->data().getIndices();
               
               VR::Table<VR::Vector> &nl = info->normals->at(frames[s]);
               nl.setCount(info->numTriangles * 3, true);
               
               VR::Table<int> &il = info->faceNormals->at(frames[s]);
               il.setCount(info->numTriangles * 3, true);
               
               if (Nb > 0.0)
               {
                  Alembic::Abc::N3fArraySamplePtr N1 = Nsamp1->data().getVals();
                  Alembic::Abc::UInt32ArraySamplePtr I1 = Nsamp1->data().getIndices();
                  
                  double Na = 1.0 - Nb;
                  
                  for (size_t i=0, k=0; i<info->numTriangles; ++i)
                  {
                     for (size_t j=0; j<3; ++j, ++k)
                     {
                        unsigned int i0 = I0->get()[info->toVertexIndex[k]];
                        unsigned int i1 = I1->get()[info->toVertexIndex[k]];
                        
                        Alembic::Abc::N3f n0 = N0->get()[i0];
                        Alembic::Abc::N3f n1 = N1->get()[i1];
                        
                        nl[k].set(Na * n0.x + Nb * n1.x,
                                  Na * n0.y + Nb * n1.y,
                                  Na * n0.z + Nb * n1.z);
                        il[k] = k;
                     }
                  }
               }
               else
               {
                  for (size_t i=0, k=0; i<info->numTriangles; ++i)
                  {
                     for (size_t j=0; j<3; ++j, ++k)
                     {
                        Alembic::Abc::N3f n = N0->get()[I0->get()[info->toVertexIndex[k]]];
                        
                        nl[k].set(n.x, n.y, n.z);
                        il[k] = k;
                     }
                  }
               }
            }
         }
      }
   }
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicMesh &node, AlembicNode *instance)
{
   if (mGeoSrc->params()->verbose)
   {
      std::cout << "[AlembicLoader] UpdateGeometry::enter: AlembicMesh \"" << (instance ? instance->path() : node.path()) << "\"";
      if (instance)
      {
         std::cout << " [instance of \"" << node.path() << "]";
      }
      std::cout << std::endl;
   }
   
   bool visible = isVisible(node);
   bool ignore = false;
   
   if (instance)
   {
      instance->setVisible(visible);
      ignore = !instance->isVisible(true);
   }
   else
   {
      node.setVisible(visible);
      ignore = !node.isVisible(true);
   }
   
   AlembicGeometrySource::GeomInfo *info = mGeoSrc->getInfo(node.path());
   AlembicGeometrySource::GeomInfo *instInfo = mGeoSrc->getInfo(instance ? instance->path() : node.path());
   
   if (instInfo && !ignore)
   {
      instInfo->ignore = false;
      
      // Setup matrices (instance specific)
      
      if (!mGeoSrc->params()->ignoreTransforms && mMatrixSamplesStack.size() > 0)
      {
         std::vector<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
         
         instInfo->numMatrices = int(matrices.size());
         instInfo->matrices = new VR::TraceTransform[instInfo->numMatrices];
         
         for (int i=0; i<instInfo->numMatrices; ++i)
         {
            Alembic::Abc::M44d &M = matrices[i];
            
            instInfo->matrices[i].m = VR::Matrix(VR::Vector(M[0][0], M[0][1], M[0][2]),
                                                 VR::Vector(M[1][0], M[1][1], M[1][2]),
                                                 VR::Vector(M[2][0], M[2][1], M[2][2]));
            instInfo->matrices[i].offs = VR::Vector(M[3][0], M[3][1], M[3][2]);
         }
      }
      
      // Setup geometry (once for all instances)
      
      if (info)
      {
         if (!info->updatedFrameGeometry)
         {
            Alembic::AbcGeom::IPolyMeshSchema &schema = node.typedObject().getSchema();
            
            bool varyingTopology = schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology;
            bool singleSample = (mGeoSrc->params()->ignoreDeformBlur || varyingTopology);
            double renderTime = mGeoSrc->renderTime();
            double renderFrame = mGeoSrc->renderFrame();
            double *times = (singleSample ? &renderTime : mGeoSrc->sampleTimes());
            double *frames = (singleSample ? &renderFrame : mGeoSrc->sampleFrames());
            size_t ntimes = (singleSample ? 1 : mGeoSrc->numTimeSamples());
            double t;
            
            // Collect user attributes
            UserAttrsSet attrs;
            bool interpolateAttribs = !varyingTopology;
            collectUserAttributes(schema.getUserProperties(), schema.getArbGeomParams(),
                                  renderTime, interpolateAttribs, attrs,
                                  true, true, true, true, true);
            
            info->invalidFrame = !readBaseMesh(node, info, attrs);
            
            if (info->invalidFrame)
            {
               instInfo->ignore = true;
            }
            else
            {
               // Output normals
               readMeshNormals(node, info);
               
               // Clear current channels
               info->channels->setCount(0, renderFrame);
               info->channelNames->setCount(0, renderFrame);
               
               // Reserve enough channels for UVs and user defined channels
               size_t maxChannels = 1 + attrs.uvs.size() + attrs.primitive.size() + attrs.point.size() + attrs.vertex.size();
               info->channels->reserve(maxChannels, renderFrame);
               info->channelNames->reserve(maxChannels, renderFrame);
               
               // Ouptut UVs
               readMeshUVs(node, info, attrs, false);
               
               // Output user attributes (as color channels)
               SetUserAttributes(info, attrs.object, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.primitive, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.point, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.vertex, renderFrame, mGeoSrc->params()->verbose);
            }
            
            // Clear user attributes
            DestroyUserAttributes(attrs.object);
            DestroyUserAttributes(attrs.primitive);
            DestroyUserAttributes(attrs.point);
            DestroyUserAttributes(attrs.vertex);
            attrs.uvs.clear();
            
            info->updatedFrameGeometry = true;
         }
         else if (info->invalidFrame)
         {
            // No valid source geometry for instance
            instInfo->ignore = true;
         }
      }
      else
      {
         // No source geometry for instance
         instInfo->ignore = true;
      }
   }
   else if (instInfo)
   {
      instInfo->ignore = true;
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicSubD &node, AlembicNode *instance)
{
   if (mGeoSrc->params()->verbose)
   {
      std::cout << "[AlembicLoader] UpdateGeometry::enter: AlembicSubD \"" << (instance ? instance->path() : node.path()) << "\"";
      if (instance)
      {
         std::cout << " [instance of \"" << node.path() << "]";
      }
      std::cout << std::endl;
   }
   
   bool visible = isVisible(node);
   bool ignore = false;
   
   if (instance)
   {
      instance->setVisible(visible);
      ignore = !instance->isVisible(true);
   }
   else
   {
      node.setVisible(visible);
      ignore = !node.isVisible(true);
   }
   
   AlembicGeometrySource::GeomInfo *info = mGeoSrc->getInfo(node.path());
   AlembicGeometrySource::GeomInfo *instInfo = mGeoSrc->getInfo(instance ? instance->path() : node.path());
   
   if (instInfo && !ignore)
   {
      instInfo->ignore = false;
      
      // Setup matrices (instance specific)
      
      if (!mGeoSrc->params()->ignoreTransforms && mMatrixSamplesStack.size() > 0)
      {
         std::vector<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
         
         instInfo->numMatrices = int(matrices.size());
         instInfo->matrices = new VR::TraceTransform[instInfo->numMatrices];
         
         for (int i=0; i<instInfo->numMatrices; ++i)
         {
            Alembic::Abc::M44d &M = matrices[i];
            
            instInfo->matrices[i].m = VR::Matrix(VR::Vector(M[0][0], M[0][1], M[0][2]),
                                                 VR::Vector(M[1][0], M[1][1], M[1][2]),
                                                 VR::Vector(M[2][0], M[2][1], M[2][2]));
            instInfo->matrices[i].offs = VR::Vector(M[3][0], M[3][1], M[3][2]);
         }
      }
      
      // Setup geometry (once for all instances)
      
      if (info)
      {
         if (!info->updatedFrameGeometry)
         {
            Alembic::AbcGeom::ISubDSchema &schema = node.typedObject().getSchema();
            
            bool varyingTopology = schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology;
            bool singleSample = (mGeoSrc->params()->ignoreDeformBlur || varyingTopology);
            double renderTime = mGeoSrc->renderTime();
            double renderFrame = mGeoSrc->renderFrame();
            double *times = (singleSample ? &renderTime : mGeoSrc->sampleTimes());
            double *frames = (singleSample ? &renderFrame : mGeoSrc->sampleFrames());
            size_t ntimes = (singleSample ? 1 : mGeoSrc->numTimeSamples());
            double t;
            
            // Collect user attributes
            UserAttrsSet attrs;
            bool interpolateAttribs = !varyingTopology;
            collectUserAttributes(schema.getUserProperties(), schema.getArbGeomParams(),
                                  renderTime, interpolateAttribs, attrs,
                                  true, true, true, true, true);
            
            info->invalidFrame = !readBaseMesh(node, info, attrs);
            
            if (info->invalidFrame)
            {
               instInfo->ignore = true;
            }
            else
            {
               // Output creases
               // TODO
               
               // Clear current channels
               info->channels->setCount(0, renderFrame);
               info->channelNames->setCount(0, renderFrame);
               
               // Reserve enough channels for UVs and user defined channels
               size_t maxChannels = 1 + attrs.uvs.size() + attrs.primitive.size() + attrs.point.size() + attrs.vertex.size();
               info->channels->reserve(maxChannels, renderFrame);
               info->channelNames->reserve(maxChannels, renderFrame);
               
               // Ouptut UVs
               readMeshUVs(node, info, attrs, false);
               
               // Output user attributes (as color channels)
               SetUserAttributes(info, attrs.object, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.primitive, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.point, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.vertex, renderFrame, mGeoSrc->params()->verbose);
            }
            
            // Clear user attributes
            DestroyUserAttributes(attrs.object);
            DestroyUserAttributes(attrs.primitive);
            DestroyUserAttributes(attrs.point);
            DestroyUserAttributes(attrs.vertex);
            attrs.uvs.clear();
            
            info->updatedFrameGeometry = true;
         }
         else if (info->invalidFrame)
         {
            // No valid source geometry for instance
            instInfo->ignore = true;
         }
      }
      else
      {
         // No source geometry for instance
         instInfo->ignore = true;
      }
   }
   else if (instInfo)
   {
      instInfo->ignore = true;
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicPoints &node, AlembicNode *instance)
{
   if (mGeoSrc->params()->verbose)
   {
      std::cout << "[AlembicLoader] UpdateGeometry::enter: AlembicPoints \"" << (instance ? instance->path() : node.path()) << "\"";
      if (instance)
      {
         std::cout << " [instance of \"" << node.path() << "]";
      }
      std::cout << std::endl;
   }
   
   bool visible = isVisible(node);
   bool ignore = false;
   
   if (instance)
   {
      instance->setVisible(visible);
      ignore = !instance->isVisible(true);
   }
   else
   {
      node.setVisible(visible);
      ignore = !node.isVisible(true);
   }
   
   AlembicGeometrySource::GeomInfo *info = mGeoSrc->getInfo(node.path());
   AlembicGeometrySource::GeomInfo *instInfo = mGeoSrc->getInfo(instance ? instance->path() : node.path());
   
   if (instInfo && !ignore)
   {
      instInfo->ignore = false;
      
      // Setup matrices (instance specific)
      
      if (!mGeoSrc->params()->ignoreTransforms && mMatrixSamplesStack.size() > 0)
      {
         std::vector<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
         
         instInfo->numMatrices = int(matrices.size());
         instInfo->matrices = new VR::TraceTransform[instInfo->numMatrices];
         
         for (int i=0; i<instInfo->numMatrices; ++i)
         {
            Alembic::Abc::M44d &M = matrices[i];
            
            instInfo->matrices[i].m = VR::Matrix(VR::Vector(M[0][0], M[0][1], M[0][2]),
                                                 VR::Vector(M[1][0], M[1][1], M[1][2]),
                                                 VR::Vector(M[2][0], M[2][1], M[2][2]));
            instInfo->matrices[i].offs = VR::Vector(M[3][0], M[3][1], M[3][2]);
         }
      }
      
      // Setup geometry
      
      if (info)
      {
         if (!info->updatedFrameGeometry)
         {
            // Read points
            Alembic::AbcGeom::IPointsSchema &schema = node.typedObject().getSchema();
            
            bool isConst = (info->constPositions != 0);
            double renderTime = mGeoSrc->renderTime();
            double renderFrame = mGeoSrc->renderFrame();
            
            float vscale = mGeoSrc->params()->fps;
            
            if (vscale > 0.0001f)
            {
               vscale = 1.0f / vscale;
            }
            else
            {
               vscale = 1.0f;
            }
            
            float ascale = vscale * vscale;
            
            // Collect user attributes (object and point attributes only)
            UserAttrsSet attrs;
            UserAttrsSet extraAttrs;
            std::string vname = "";
            std::string aname = "";
            
            // collectUserAttributes(schema.getUserProperties(), schema.getArbGeomParams(),
            //                       renderTime, false, attrs,
            //                       true, false, true, false, false);
            
            info->invalidFrame = false;
            
            // Read position, velocity and id
            if (isConst)
            {
               Alembic::AbcGeom::IPointsSchema::Sample theSample = schema.getValue();
               
               Alembic::Abc::P3fArraySamplePtr P = theSample.getPositions();
               Alembic::Abc::V3fArraySamplePtr V = theSample.getVelocities();
               Alembic::Abc::UInt64ArraySamplePtr ID = theSample.getIds();
               
               info->numPoints = P->size();
               info->sortParticles(ID->size(), ID->get());
               
               info->constPositions->setCount(info->numPoints + 1, renderFrame);
               info->velocities->setCount(info->numPoints + 1, renderFrame);
               info->accelerations->setCount(info->numPoints + 1, renderFrame);
               info->ids->setCount(info->numPoints + 1, renderFrame);
               
               VR::VectorList pl = info->constPositions->getVectorList(renderFrame);
               VR::VectorList vl = info->velocities->getVectorList(renderFrame);
               VR::ColorList al = info->accelerations->getColorList(renderFrame);
               VR::IntList il = info->ids->getIntList(renderFrame);
               
               collectUserAttributes(schema.getUserProperties(), schema.getArbGeomParams(),
                                     renderTime, false, attrs,
                                     true, false, true, false, false);
               
               // Get velocities and accelerations
               // for V-Ray: velocities, acceleration_pp
               const float *vel = 0;
               const float *acc = 0;
               
               if (V)
               {
                  vel = (const float*) V->getData();
               }
               else
               {
                  UserAttributes::iterator it = attrs.point.find("velocity");
                  
                  if (it == attrs.point.end() ||
                      it->second.dataType != Float_Type ||
                      it->second.dataDim != 3 ||
                      it->second.dataCount != info->numPoints)
                  {
                     it = attrs.point.find("v");
                     if (it != attrs.point.end() &&
                         (it->second.dataType != Float_Type ||
                          it->second.dataDim != 3 ||
                          it->second.dataCount != info->numPoints))
                     {
                        it = attrs.point.end();
                     }
                  }
                  if (it != attrs.point.end())
                  {
                     vname = it->first;
                     vel = (const float*) it->second.data;
                  }
               }
                  
               if (vel)
               {
                  UserAttributes::iterator it = attrs.point.find("acceleration");
                  
                  if (it == attrs.point.end() ||
                      it->second.dataType != Float_Type ||
                      it->second.dataDim != 3 ||
                      it->second.dataCount != info->numPoints)
                  {
                     it = attrs.point.find("accel");
                     if (it == attrs.point.end() ||
                         it->second.dataType != Float_Type ||
                         it->second.dataDim != 3 ||
                         it->second.dataCount != info->numPoints)
                     {
                        it = attrs.point.find("a");
                        if (it != attrs.point.end() &&
                            (it->second.dataType != Float_Type ||
                             it->second.dataDim != 3 ||
                             it->second.dataCount != info->numPoints))
                        {
                           it = attrs.point.end();
                        }
                     }
                  }
                  if (it != attrs.point.end())
                  {
                     aname = it->first;
                     acc = (const float*) it->second.data;
                  }
               }
               
               pl[0].set(0.0f, 0.0f, 0.0f);
               vl[0].set(0.0f, 0.0f, 0.0f);
               al[0].set(0.0f, 0.0f, 0.0f);
               il[0] = 0;
               
               for (size_t i=0, j=0; i<info->numPoints; ++i, j+=3)
               {
                  size_t k = 1 + info->particleOrder[i];
                  
                  Alembic::Abc::V3f p = P->get()[i];
                  Alembic::Abc::uint64_t id = ID->get()[i];
                  
                  pl[k].set(p.x, p.y, p.z);
                  il[k] = id;
                  
                  if (vel)
                  {
                     vl[k].set(vscale * vel[j], vscale * vel[j+1], vscale * vel[j+2]);
                  }
                  else
                  {
                     vl[k].set(0.0f, 0.0f, 0.0f);
                  }
                  
                  if (acc)
                  {
                     vl[k].set(ascale * acc[j], ascale * acc[j+1], ascale * acc[j+2]);
                  }
                  else
                  {
                     al[k].set(0.0f, 0.0f, 0.0f);
                  }
               }
            }
            else
            {
               bool singleSample = mGeoSrc->params()->ignoreDeformBlur;
               double *times = (singleSample ? &renderTime : mGeoSrc->sampleTimes());
               double *frames = (singleSample ? &renderFrame : mGeoSrc->sampleFrames());
               size_t ntimes = (singleSample ? 1 : mGeoSrc->numTimeSamples());
               double t;
               
               // When sampling the schema, make sure we sample 'renderTime'
               if (mGeoSrc->params()->verbose)
               {
                  std::cout << "[AlembicLoader] UpdateGeometry::enter: Sampling point schema" << std::endl;
               }
               for (size_t i=0; i<ntimes; ++i)
               {
                  t = times[i];
                  node.sampleSchema(t, t, i > 0);
               }
               
               node.sampleSchema(renderTime, renderTime, true);
               
               TimeSampleList<Alembic::AbcGeom::IPointsSchema> &samples = node.samples().schemaSamples;
               TimeSampleList<Alembic::AbcGeom::IPointsSchema>::ConstIterator samp0, samp1; // for animated mesh
               double a = 1.0;
               double b = 0.0;
               
               info->positions->clear();
               
               if (samples.size() == 0 || !samples.getSamples(renderTime, samp0, samp1, b))
               {
                  info->invalidFrame = true;
                  
                  info->ids->setCount(0, renderFrame);
                  info->velocities->setCount(0, renderFrame);
                  info->accelerations->setCount(0, renderFrame);
               }
               else
               {
                  Alembic::Abc::P3fArraySamplePtr P0 = samp0->data().getPositions();
                  Alembic::Abc::V3fArraySamplePtr V0 = samp0->data().getVelocities();
                  Alembic::Abc::UInt64ArraySamplePtr ID0 = samp0->data().getIds();
                  
                  Alembic::Abc::P3fArraySamplePtr P1;
                  Alembic::Abc::V3fArraySamplePtr V1;
                  Alembic::Abc::UInt64ArraySamplePtr ID1;
                  
                  // Build ID mappings
                  std::map<Alembic::Util::uint64_t, size_t> idmap0; // ID -> index in P0/ID0/V0
                  std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> > idmap1; // ID -> (index in P1/ID1/V1, index in final point list)
                  std::map<Alembic::Util::uint64_t, size_t> sharedids; // ID -> index in P1/ID1/V1
                  
                  std::vector<Alembic::Util::uint64_t> allids;
                  allids.reserve(2 * ID0->size());
                  
                  if (mGeoSrc->params()->verbose)
                  {
                     std::cout << "[AlembicLoader] UpdateGeometry::enter: Collect user attribute at t=" << samp0->time() << std::endl;
                  }
                  collectUserAttributes(schema.getUserProperties(), schema.getArbGeomParams(),
                                        samp0->time(), false, attrs,
                                        true, false, true, false, false);
                  
                  const float *vel0 = 0;
                  const float *vel1 = 0;
                  const float *acc0 = 0;
                  const float *acc1 = 0;
                  
                  if (mGeoSrc->params()->verbose)
                  {
                     std::cout << "[AlembicLoader] UpdateGeometry::enter: Initialize particle ID list" << std::endl;
                  }
                  for (size_t i=0; i<ID0->size(); ++i)
                  {
                     Alembic::Util::uint64_t id = ID0->get()[i];
                     
                     idmap0[id] = i;
                     allids.push_back(id);
                  }
                  
                  info->numPoints = P0->size();
                  
                  // Get velocities and accelerations
                  // for V-Ray: velocities, acceleration_pp
                  
                  if (mGeoSrc->params()->verbose)
                  {
                     std::cout << "[AlembicLoader] UpdateGeometry::enter: Looking for velocity and acceleration attributes" << std::endl;
                  }
                  
                  if (V0)
                  {
                     vel0 = (const float*) V0->getData();
                  }
                  else
                  {
                     UserAttributes::iterator it = attrs.point.find("velocity");
                     
                     if (it == attrs.point.end() ||
                         it->second.dataType != Float_Type ||
                         it->second.dataDim != 3 ||
                         it->second.dataCount != info->numPoints)
                     {
                        it = attrs.point.find("v");
                        if (it != attrs.point.end() &&
                            (it->second.dataType != Float_Type ||
                             it->second.dataDim != 3 ||
                             it->second.dataCount != info->numPoints))
                        {
                           it = attrs.point.end();
                        }
                     }
                     if (it != attrs.point.end())
                     {
                        vname = it->first;
                        vel0 = (const float*) it->second.data;
                     }
                  }
                  
                  if (vel0)
                  {
                     UserAttributes::iterator it = attrs.point.find("acceleration");
                     
                     if (it == attrs.point.end() ||
                         it->second.dataType != Float_Type ||
                         it->second.dataDim != 3 ||
                         it->second.dataCount != info->numPoints)
                     {
                        it = attrs.point.find("accel");
                        if (it == attrs.point.end() ||
                            it->second.dataType != Float_Type ||
                            it->second.dataDim != 3 ||
                            it->second.dataCount != info->numPoints)
                        {
                           it = attrs.point.find("a");
                           if (it != attrs.point.end() &&
                               (it->second.dataType != Float_Type ||
                                it->second.dataDim != 3 ||
                                it->second.dataCount != info->numPoints))
                           {
                              it = attrs.point.end();
                           }
                        }
                     }
                     if (it != attrs.point.end())
                     {
                        aname = it->first;
                        acc0 = (const float*) it->second.data;
                     }
                  }
                  
                  if (b > 0.0)
                  {
                     P1 = samp1->data().getPositions();
                     V1 = samp1->data().getVelocities();
                     ID1 = samp1->data().getIds();
                     
                     a = 1.0 - b;
                     
                     for (size_t i=0; i<ID1->size(); ++i)
                     {
                        Alembic::Util::uint64_t id = ID1->get()[i];
                        
                        if (idmap0.find(id) != idmap0.end())
                        {
                           sharedids[id] = i;
                        }
                        else
                        {
                           std::pair<size_t, size_t> idxs;
                           
                           idxs.first = i;
                           idxs.second = info->numPoints++;
                           
                           idmap1[id] = idxs;
                           allids.push_back(id);
                        }
                     }
                     
                     if (mGeoSrc->params()->verbose)
                     {
                        std::cout << "[AlembicLoader] UpdateGeometry::enter: Collect user point attribute(s) at t=" << samp1->time() << std::endl;
                     }
                     
                     collectUserAttributes(Alembic::Abc::ICompoundProperty(), schema.getArbGeomParams(),
                                           samp1->time(), false, extraAttrs,
                                           false, false, true, false, false);
                     
                     if (mGeoSrc->params()->verbose)
                     {
                        std::cout << "[AlembicLoader] UpdateGeometry::enter: Look for velocity and accelerations" << samp1->time() << std::endl;
                     }
                     
                     if (V1)
                     {
                        vel1 = (const float*) V1->getData();
                     }
                     else if (vname.length() > 0)
                     {
                        // Don't really have to check it as we are sampling the same schema, just at a different time
                        UserAttributes::iterator it = extraAttrs.point.find(vname);
                        if (it != extraAttrs.point.end())
                        {
                           vel1 = (const float*) it->second.data;
                        }
                     }
                     
                     if (aname.length() > 0)
                     {
                        // Don't really have to check it as we are sampling the same schema, just at a different time
                        UserAttributes::iterator it = extraAttrs.point.find(aname);
                        if (it != extraAttrs.point.end())
                        {
                           acc1 = (const float*) it->second.data;
                        }
                     }
                  }
                  
                  // Fill points
                  
                  if (mGeoSrc->params()->verbose)
                  {
                     std::cout << "[AlembicLoader] UpdateGeometry::enter: Sort particle IDs" << std::endl;
                  }
                  
                  info->sortParticles(allids.size(), &allids[0]);
                  
                  if (!vel0)
                  {
                     // Force single sample
                     ntimes = 1;
                     times = &renderTime;
                     frames = &renderFrame;
                  }
                  
                  info->ids->setCount(info->numPoints + 1, renderFrame);
                  info->velocities->setCount(info->numPoints + 1, renderFrame);
                  info->accelerations->setCount(info->numPoints + 1, renderFrame);
                  
                  VR::IntList il = info->ids->getIntList(renderFrame);
                  VR::VectorList vl = info->velocities->getVectorList(renderFrame);
                  VR::ColorList al = info->accelerations->getColorList(renderFrame);
                  
                  il[0] = 0;
                  vl[0].set(0.0f, 0.0f, 0.0f);
                  al[0].set(0.0f, 0.0f, 0.0f);
                  
                  std::map<Alembic::Util::uint64_t, size_t>::iterator idit;
                  
                  for (size_t i=0; i<ntimes; ++i)
                  {
                     t = times[i];
                     
                     if (mGeoSrc->params()->verbose)
                     {
                        std::cout << "[AlembicLoader] UpdateGeometry::enter: Compute positions for t=" << t << std::endl;
                     }
                     
                     VR::Table<VR::Vector> &pl = info->positions->at(frames[i]);
                     
                     pl.setCount(info->numPoints + 1, true);
                     
                     pl[0].set(0.0f, 0.0f, 0.0f);
                     
                     for (size_t j=0, voff=0; j<P0->size(); ++j, voff+=3)
                     {
                        // size_t k = 1 + j;
                        size_t k = 1 + info->particleOrder[j];
                        
                        Alembic::Util::uint64_t id = ID0->get()[j];
                        
                        il[k] = id;
                        
                        idit = sharedids.find(id);
                        
                        if (idit != sharedids.end())
                        {
                           // interpolate position and velocity
                           Alembic::Abc::V3f p = float(a) * P0->get()[j] + float(b) * P1->get()[idit->second];
                           
                           if (vel0 && vel1)
                           {
                              // Note: a * samp0->time() + b * samp1->time() == renderTime
                              double dt = t - renderTime;
                              
                              size_t off = 3 * idit->second;
                              
                              if (i == 0)
                              {
                                 vl[k].set(vscale * (a * vel0[voff+0] + b * vel1[off+0]),
                                           vscale * (a * vel0[voff+1] + b * vel1[off+1]),
                                           vscale * (a * vel0[voff+2] + b * vel1[off+2]));
                              }
                              
                              p.x += dt * (a * vel0[voff+0] + b * vel1[off+0]);
                              p.y += dt * (a * vel0[voff+1] + b * vel1[off+1]);
                              p.z += dt * (a * vel0[voff+2] + b * vel1[off+2]);
                              
                              if (acc0 && acc1)
                              {
                                 double hdt2 = 0.5 * dt * dt;
                                 
                                 if (i == 0)
                                 {
                                    al[k].set(ascale * (a * acc0[voff+0] + b * acc1[off+0]),
                                              ascale * (a * acc0[voff+1] + b * acc1[off+1]),
                                              ascale * (a * acc0[voff+2] + b * acc1[off+2]));
                                 }
                                 
                                 p.x += hdt2 * (a * acc0[voff+0] + b * acc1[off+0]);
                                 p.y += hdt2 * (a * acc0[voff+0] + b * acc1[off+0]);
                                 p.z += hdt2 * (a * acc0[voff+0] + b * acc1[off+0]);
                              }
                              else if (i == 0)
                              {
                                 al[k].set(0.0f, 0.0f, 0.0f);
                              }
                           }
                           else if (i == 0)
                           {
                              vl[k].set(0.0f, 0.0f, 0.0f);
                              al[k].set(0.0f, 0.0f, 0.0f);
                           }
                           
                           pl[k].set(p.x, p.y, p.z);
                        }
                        else
                        {
                           // Extrapolate using velocity and acceleration
                           
                           Alembic::Abc::V3f p = P0->get()[j];
                           
                           if (vel0)
                           {
                              double dt = t - samp0->time();
                              
                              if (i == 0)
                              {
                                 vl[k].set(vscale * vel0[voff], vscale * vel0[voff+1], vscale * vel0[voff+2]);
                              }
                              
                              p.x += dt * vel0[voff];
                              p.y += dt * vel0[voff+1];
                              p.z += dt * vel0[voff+2];
                              
                              if (acc0)
                              {
                                 double hdt2 = 0.5 * dt * dt;
                                 
                                 if (i == 0)
                                 {
                                    al[k].set(ascale * acc0[voff], ascale * acc0[voff+1], ascale * acc0[voff+2]);
                                 }
                                 
                                 p.x += hdt2 * acc0[voff];
                                 p.y += hdt2 * acc0[voff+1];
                                 p.z += hdt2 * acc0[voff+2];
                              }
                              else if (i == 0)
                              {
                                 al[k].set(0.0f, 0.0f, 0.0f);
                              }
                           }
                           else if (i == 0)
                           {
                              vl[k].set(0.0f, 0.0f, 0.0f);
                              al[k].set(0.0f, 0.0f, 0.0f);
                           }
                           
                           pl[k].set(p.x, p.y, p.z);
                        }
                     }
                     
                     std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator it;
                     
                     for (it=idmap1.begin(); it!=idmap1.end(); ++it)
                     {
                        //size_t k = 1 + it->second.second;
                        size_t k = 1 + info->particleOrder[it->second.second];
                        
                        // Extrapolate using velocity and acceleration
                        
                        Alembic::Abc::V3f p = P1->get()[it->second.first];
                        
                        if (vel1)
                        {
                           double dt = t - samp1->time();
                           
                           size_t off = it->second.first * 3;
                           
                           if (i == 0)
                           {
                              vl[k].set(vscale * vel1[off], vscale * vel1[off+1], vscale * vel1[off+2]);
                           }
                           
                           p.x += dt * vel1[off];
                           p.y += dt * vel1[off+1];
                           p.z += dt * vel1[off+2];
                           
                           if (acc1)
                           {
                              double hdt2 = 0.5 * dt * dt;
                              
                              if (i == 0)
                              {
                                 al[k].set(ascale * acc1[off], ascale * acc1[off+1], ascale * acc1[off+2]);
                              }
                              
                              p.x += hdt2 * acc1[off];
                              p.y += hdt2 * acc1[off+1];
                              p.z += hdt2 * acc1[off+2];
                           }
                           else if (i == 0)
                           {
                              al[k].set(0.0f, 0.0f, 0.0f);
                           }
                        }
                        else if (i == 0)
                        {
                           vl[k].set(0.0f, 0.0f, 0.0f);
                           al[k].set(0.0f, 0.0f, 0.0f);
                        }
                        
                        pl[k].set(p.x, p.y, p.z);
                        il[k] = ID1->get()[it->second.first];
                     }
                  }
                  
                  // Adjust attributes
                  
                  if (idmap1.size() > 0)
                  {
                     if (mGeoSrc->params()->verbose)
                     {
                        std::cout << "[AlembicLoader] UpdateGeometry::enter: Adjust user attributes size" << std::endl;
                     }
                     
                     for (UserAttributes::iterator it0 = attrs.point.begin(); it0 != attrs.point.end(); ++it0)
                     {
                        if (it0->first == vname || it0->first == aname)
                        {
                           continue;
                        }
                        
                        ResizeUserAttribute(it0->second, info->numPoints);
                        
                        UserAttributes::iterator it1 = extraAttrs.point.find(it0->first);
                        
                        if (it1 != extraAttrs.point.end())
                        {
                           std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator idit;
                           
                           for (idit = idmap1.begin(); idit != idmap1.end(); ++idit)
                           {
                              if (!CopyUserAttribute(it1->second, idit->second.first, 1,
                                                     it0->second, idit->second.second))
                              {
                                 std::cerr << "[AlembicLoader] Failed to copy extended user attribute data" << std::endl;
                              }
                           }
                        }
                     }
                  }
                  
                  DestroyUserAttributes(extraAttrs.point);
               }
            }
            
            if (info->invalidFrame)
            {
               instInfo->ignore = true;
            }
            else
            {
               // schema.getWidth?
               
               // Output user attributes (as color channels)
               
               if (vname.length() > 0)
               {
                  UserAttributes::iterator it = attrs.point.find(vname);
                  DestroyUserAttribute(it->second);
                  attrs.point.erase(it);
               }
               
               if (aname.length() > 0)
               {
                  UserAttributes::iterator it = attrs.point.find(aname);
                  DestroyUserAttribute(it->second);
                  attrs.point.erase(it);
               }
               
               if (mGeoSrc->params()->verbose)
               {
                  std::cout << "[AlembicLoader] UpdateGeometry::enter: Output user attribute(s)" << std::endl;
               }
               
               SetUserAttributes(info, attrs.object, renderFrame, mGeoSrc->params()->verbose);
               SetUserAttributes(info, attrs.point, renderFrame, mGeoSrc->params()->verbose);
               
               // Reset particle IDs
               if (!info->sortIDs)
               {
                  VR::IntList il = info->ids->getIntList(renderFrame);
                  il[0] = 0;
                  for (int i=1; i<il.count(); ++i)
                  {
                     il[i] = i - 1;
                  }
               }
               
               info->attachParams(mGeoSrc->factory());
            }
            
            // Clear user attributes
            DestroyUserAttributes(attrs.object);
            DestroyUserAttributes(attrs.point);
            
            info->updatedFrameGeometry = true;
         }
         else if (info->invalidFrame)
         {
            // No valid source geometry for instance
            instInfo->ignore = true;
         }
      }
      else
      {
         // No source geometry for instance
         instInfo->ignore = true;
      }
   }
   else if (instInfo)
   {
      instInfo->ignore = true;
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicCurves &, AlembicNode *)
{
   // TODO
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicNuPatch &, AlembicNode *)
{
   // TODO
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn UpdateGeometry::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mGeoSrc->params()->ignoreInstances)
      {
         return node.master()->enter(*this, &node);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}

void UpdateGeometry::leave(AlembicXform &node, AlembicNode *instance)
{
   bool visible = (instance ? instance->isVisible() : node.isVisible());
   
   if (visible && !node.isLocator() && !mGeoSrc->params()->ignoreTransforms)
   {
      mMatrixSamplesStack.pop_back();
   }
}

void UpdateGeometry::leave(AlembicNode &, AlembicNode *)
{
}
