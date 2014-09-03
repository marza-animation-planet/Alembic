#include "visitors.h"
#include "userattr.h"

// ---

GetTimeRange::GetTimeRange()
   : mStartTime(std::numeric_limits<double>::max())
   , mEndTime(-std::numeric_limits<double>::max())
{
}
   
AlembicNode::VisitReturn GetTimeRange::enter(AlembicXform &node, AlembicNode *)
{
   if (node.isLocator())
   {
      const Alembic::Abc::IScalarProperty &prop = node.locatorProperty();
      
      Alembic::AbcCoreAbstract::TimeSamplingPtr ts = prop.getTimeSampling();
      size_t nsamples = prop.getNumSamples();
      
      if (nsamples > 1)
      {
         mStartTime = std::min(ts->getSampleTime(0), mStartTime);
         mEndTime = std::max(ts->getSampleTime(nsamples-1), mEndTime);
      }
   }
   else
   {
      updateTimeRange(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetTimeRange::enter(AlembicMesh &node, AlembicNode *)
{
   updateTimeRange(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetTimeRange::enter(AlembicSubD &node, AlembicNode *)
{
   updateTimeRange(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetTimeRange::enter(AlembicPoints &, AlembicNode *)
{
   //updateTimeRange(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetTimeRange::enter(AlembicCurves &, AlembicNode *)
{
   //updateTimeRange(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetTimeRange::enter(AlembicNuPatch &, AlembicNode *)
{
   //updateTimeRange(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetTimeRange::enter(AlembicNode &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}
   
void GetTimeRange::leave(AlembicNode &, AlembicNode *)
{
   // Noop
}

bool GetTimeRange::getRange(double &start, double &end) const
{
   if (mStartTime < mEndTime)
   {
      start = mStartTime;
      end = mEndTime;
      return true;
   }
   else
   {
      return false;
   }
}

// ---

bool GetVisibility(Alembic::Abc::ICompoundProperty props, double t)
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

CountShapes::CountShapes(double renderTime, bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility)
   : mRenderTime(renderTime)
   , mIgnoreTransforms(ignoreTransforms)
   , mIgnoreInstances(ignoreInstances)
   , mIgnoreVisibility(ignoreVisibility)
   , mNumShapes(0)
{
}
   
AlembicNode::VisitReturn CountShapes::enter(AlembicXform &node, AlembicNode *)
{
   if (mIgnoreTransforms || isVisible(node))
   {
      return AlembicNode::ContinueVisit;
   }
   else
   {
      return AlembicNode::DontVisitChildren;
   }
}

AlembicNode::VisitReturn CountShapes::enter(AlembicMesh &node, AlembicNode *)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicSubD &node, AlembicNode *)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicPoints &, AlembicNode *)
{
   //return shapeEnter(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CountShapes::enter(AlembicCurves &, AlembicNode *)
{
   //return shapeEnter(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CountShapes::enter(AlembicNuPatch &, AlembicNode *)
{
   //return shapeEnter(node);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CountShapes::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mIgnoreInstances)
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
   
void CountShapes::leave(AlembicNode &, AlembicNode *)
{
   // Noop
}

// ---

MakeProcedurals::MakeProcedurals(Dso *dso)
   : mDso(dso)
{
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicXform &node, AlembicNode *instance)
{
   Alembic::Util::bool_t visible = (mDso->ignoreVisibility() ? true : GetVisibility(node.object().getProperties(), mDso->renderTime()));
   
   if (instance)
   {
      instance->setVisible(visible);
   }
   else
   {
      node.setVisible(visible);
   }
   
   if (!visible)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (!node.isLocator() && !mDso->ignoreTransforms())
   {
      TimeSampleList<Alembic::AbcGeom::IXformSchema> &xformSamples = node.samples().schemaSamples;
      
      // Beware of the inheritsXform when computing world
      mMatrixSamplesStack.push_back(std::vector<Alembic::Abc::M44d>());
      
      std::vector<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
      
      std::vector<Alembic::Abc::M44d> *parentMatrices = 0;
      if (mMatrixSamplesStack.size() >= 2)
      {
         parentMatrices = &(mMatrixSamplesStack[mMatrixSamplesStack.size()-2]);
      }
      
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
         double t = mDso->motionSampleTime(i);
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample xform \"%s\" at t=%lf", node.path().c_str(), t);
         }
         
         node.sampleBounds(t, t, (i > 0 || instance != 0));
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
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            Alembic::Abc::M44d matrix;
            bool inheritXforms = true;
            
            double t = mDso->motionSampleTime(i);
            
            TimeSampleList<Alembic::AbcGeom::IXformSchema>::ConstIterator samp0, samp1;
            double blend = 0.0;
            
            if (xformSamples.getSamples(t, samp0, samp1, blend))
            {
               if (blend > 0.0)
               {
                  if (samp0->data().getInheritsXforms() != samp1->data().getInheritsXforms())
                  {
                     AiMsgWarning("[abcproc] %s: Animated inherits transform property found, use first sample value", node.path().c_str());
                     
                     matrix = samp0->data().getMatrix();
                  }
                  else
                  {
                     matrix = (1.0 - blend) * samp0->data().getMatrix() + blend * samp1->data().getMatrix();
                  }
               }
               else
               {
                  matrix = samp0->data().getMatrix();
               }
               
               inheritXforms = samp0->data().getInheritsXforms();
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
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] %lu xform samples for \"%s\"", matrices.size(), node.path().c_str());
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicMesh &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicSubD &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicPoints &, AlembicNode *)
{
   //return shapeEnter(node, instance);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicCurves &, AlembicNode *)
{
   //return shapeEnter(node, instance);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicNuPatch &, AlembicNode *)
{
   //return shapeEnter(node, instance);
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mDso->ignoreInstances())
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

void MakeProcedurals::leave(AlembicXform &node, AlembicNode *instance)
{
   bool visible = (instance ? instance->isVisible() : node.isVisible());
   
   if (visible && !node.isLocator() && !mDso->ignoreTransforms())
   {
      mMatrixSamplesStack.pop_back();
   }
}

void MakeProcedurals::leave(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance() && !mDso->ignoreInstances())
   {
      AlembicNode *m = node.master();
      m->leave(*this, &node);
   }
}

// ---

CollectWorldMatrices::CollectWorldMatrices(Dso *dso)
   : mDso(dso)
{
}
   
AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicXform &node, AlembicNode *)
{
   if (!node.isLocator() && !mDso->ignoreTransforms())
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
      if (!mDso->ignoreInstances())
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
   if (!node.isLocator() && !mDso->ignoreTransforms())
   {
      mMatrixStack.pop_back();
   }
}

void CollectWorldMatrices::leave(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance() && !mDso->ignoreInstances())
   {
      AlembicNode *m = node.master();
      m->leave(*this, &node);
   }
}

// ---

MakeShape::MakeShape(Dso *dso)
   : mDso(dso)
   , mNode(0)
{
}

std::string MakeShape::arnoldNodeName(AlembicNode &node) const
{
   std::string baseName = node.formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
   return mDso->uniqueName(baseName);
}

void MakeShape::collectUserAttributes(Alembic::Abc::ICompoundProperty userProps,
                                      Alembic::Abc::ICompoundProperty geomParams,
                                      double t,
                                      bool interpolate,
                                      UserAttributes *objectAttrs,
                                      UserAttributes *primitiveAttrs,
                                      UserAttributes *pointAttrs,
                                      UserAttributes *vertexAttrs,
                                      std::map<std::string, Alembic::AbcGeom::IV2fGeomParam> *UVs)
{
   if (userProps.valid() && objectAttrs && mDso->readObjectAttribs())
   {
      for (size_t i=0; i<userProps.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = userProps.getPropertyHeader(i);
         
         std::pair<std::string, UserAttribute> ua;
         
         ua.first = header.getName();
         mDso->cleanAttribName(ua.first);
         InitUserAttribute(ua.second);
         
         ua.second.arnoldCategory = AI_USERDEF_CONSTANT;
         
         if (ReadUserAttribute(ua.second, userProps, header, t, false, interpolate))
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read \"%s\" object attribute as \"%s\"",
                         header.getName().c_str(), ua.first.c_str());
            }
            objectAttrs->insert(ua);
         }
         else
         {
            DestroyUserAttribute(ua.second);
         }
      }
   }
   
   if (geomParams.valid())
   {
      std::set<std::string> specialPointNames;
      
      specialPointNames.insert("acceleration");
      specialPointNames.insert("accel");
      specialPointNames.insert("a");
      specialPointNames.insert("velocity");
      specialPointNames.insert("v");
      
      for (size_t i=0; i<geomParams.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = geomParams.getPropertyHeader(i);
         
         Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
         
         if (UVs &&
             scope == Alembic::AbcGeom::kFacevaryingScope &&
             Alembic::AbcGeom::IV3fGeomParam::matches(header) &&
             Alembic::AbcGeom::isUV(header))
         {
            std::pair<std::string, Alembic::AbcGeom::IV2fGeomParam> UV;
            
            UV.first = header.getName();
            mDso->cleanAttribName(UV.first);
            UV.second = Alembic::AbcGeom::IV2fGeomParam(geomParams, header.getName());
            
            UVs->insert(UV);
         }
         else
         {
            std::pair<std::string, UserAttribute> ua;
            
            UserAttributes *attrs = 0;
            
            ua.first = header.getName();
            mDso->cleanAttribName(ua.first);
            InitUserAttribute(ua.second);
            
            switch (scope)
            {
            case Alembic::AbcGeom::kFacevaryingScope:
               if (vertexAttrs && mDso->readVertexAttribs())
               {
                  ua.second.arnoldCategory = AI_USERDEF_INDEXED;
                  attrs = vertexAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" vertex attribute as \"%s\"",
                               header.getName().c_str(), ua.first.c_str());
                  }
               }
               break;
            case Alembic::AbcGeom::kVaryingScope:
            case Alembic::AbcGeom::kVertexScope:
               if (pointAttrs && (mDso->readPointAttribs() || specialPointNames.find(ua.first) != specialPointNames.end()))
               {
                  ua.second.arnoldCategory = AI_USERDEF_VARYING;
                  attrs = pointAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" point attribute as \"%s\"",
                               header.getName().c_str(), ua.first.c_str());
                  }
               }
               break;
            case Alembic::AbcGeom::kUniformScope:
               if (primitiveAttrs && mDso->readPrimitiveAttribs())
               {
                  ua.second.arnoldCategory = AI_USERDEF_UNIFORM;
                  attrs = primitiveAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" primitive attribute as \"%s\"",
                               header.getName().c_str(), ua.first.c_str());
                  }
               }
               break;
            case Alembic::AbcGeom::kConstantScope:
               if (objectAttrs && mDso->readObjectAttribs())
               {
                  ua.second.arnoldCategory = AI_USERDEF_CONSTANT;
                  attrs = objectAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" object attribute as \"%s\"",
                               header.getName().c_str(), ua.first.c_str());
                  }
               }
               break;
            default:
               continue;
            }
            
            if (attrs)
            {
               if (ReadUserAttribute(ua.second, geomParams, header, t, true, interpolate))
               {
                  attrs->insert(ua);
               }
               else
               {
                  if (mDso->verbose())
                  {
                     AiMsgWarning("[abcproc] Failed");
                  }
                  DestroyUserAttribute(ua.second);
               }
            }
         }
      }
   }
}


float* MakeShape::computeMeshSmoothNormals(MakeShape::MeshInfo &info,
                                           Alembic::Abc::P3fArraySamplePtr P0,
                                           Alembic::Abc::P3fArraySamplePtr P1,
                                           float blend)
{
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Compute smooth point normals");
   }
   
   size_t bytesize = 3 * info.pointCount * sizeof(float);
   
   float *smoothNormals = (float*) AiMalloc(bytesize);
   
   memset(smoothNormals, 0, bytesize);
   
   Alembic::Abc::V3f fN, p0, p1, p2, e0, e1;
   
   for (unsigned int f=0, v=0; f<info.polygonCount; ++f)
   {
      unsigned int nfv = info.polygonVertexCount[f];
      
      for (unsigned int fv=2; fv<nfv; ++fv)
      {
         unsigned int pi[3] = {info.vertexPointIndex[v         ],
                               info.vertexPointIndex[v + fv - 1],
                               info.vertexPointIndex[v + fv    ]};
         
         if (P1)
         {
            p0 = (1.0f - blend) * P0->get()[pi[0]] + blend * P1->get()[pi[0]];
            p1 = (1.0f - blend) * P0->get()[pi[1]] + blend * P1->get()[pi[1]];
            p2 = (1.0f - blend) * P0->get()[pi[2]] + blend * P1->get()[pi[2]];
         }
         else
         {
            p0 = P0->get()[pi[0]];
            p1 = P0->get()[pi[1]];
            p2 = P0->get()[pi[2]];
         }
         
         e0 = p1 - p0;
         e1 = p2 - p1;
         
         // reverseWinding means CCW, CW otherwise, decides normal orientation
         fN = (mDso->reverseWinding() ? e0.cross(e1) : e1.cross(e0));
         fN.normalize();
         
         for (int i=0; i<3; ++i)
         {
            unsigned int off = 3 * pi[i];
            for (int j=0; j<3; ++j)
            {
               smoothNormals[off+j] += fN[j];
            }
         }
      }
      
      v += nfv;
   }
   
   for (unsigned int p=0, off=0; p<info.pointCount; ++p, off+=3)
   {
      float l = smoothNormals[off  ] * smoothNormals[off  ] +
                smoothNormals[off+1] * smoothNormals[off+1] +
                smoothNormals[off+2] * smoothNormals[off+2];
      
      if (l > 0.0f)
      {
         l = 1.0f / sqrtf(l);
         
         smoothNormals[off  ] *= l;
         smoothNormals[off+1] *= l;
         smoothNormals[off+2] *= l;
      }
   }
   
   return smoothNormals;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicMesh &node, AlembicNode *instance)
{
   Alembic::AbcGeom::IPolyMeshSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(schema);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   MeshInfo info;
   
   info.varyingTopology = (schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   if (schema.getUVsParam().valid())
   {
      info.UVs[""] = schema.getUVsParam();
   }
   
   // Collect attributes
   
   double attribsTime = (info.varyingTopology ? mDso->renderTime() : mDso->attribsTime(mDso->attribsFrame()));
   
   bool interpolateAttribs = !info.varyingTopology;
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         attribsTime,
                         interpolateAttribs,
                         &info.objectAttrs,
                         &info.primitiveAttrs,
                         &info.pointAttrs,
                         &info.vertexAttrs,
                         &info.UVs);
   
   // Create base mesh
   
   mNode = generateBaseMesh(node, info);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output normals
   
   AtArray *nlist = 0;
   AtArray *nidxs = 0;
   
   bool subd = false;
   bool smoothing = false;
   
   const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mDso->procNode(), "subdiv_type");
   
   if (pe)
   {
      subd = (AiNodeGetInt(mDso->procNode(), "subdiv_type") > 0);
   }
   
   if (!subd)
   {
      pe = AiNodeLookUpUserParameter(mDso->procNode(), "smoothing");
      if (pe)
      {
         smoothing = AiNodeGetBool(mDso->procNode(), "smoothing");
      }
   }
   
   bool readNormals = (!subd && smoothing);
   
   if (mDso->verbose() && readNormals)
   {
      AiMsgInfo("[abcproc] Read normals data if available");
   }
   
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
   Alembic::AbcGeom::IN3fGeomParam N = schema.getNormalsParam();
   
   if (readNormals && N.valid())
   {
      if (info.varyingTopology)
      {
         Nsamples.update(N, mDso->renderTime(), mDso->renderTime(), false);
      }
      else
      {
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            double t = mDso->motionSampleTime(i);
         
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Sample normals \"%s\" at t=%lf", node.path().c_str(), t);
            }
         
            Nsamples.update(N, t, t, i>0);
         }
      }
   }
   
   if (Nsamples.size() > 0)
   {
      TimeSampleList<Alembic::AbcGeom::IN3fGeomParam>::ConstIterator n0, n1;
      double blend = 0.0;
      AtVector vec;
      
      if (info.varyingTopology || Nsamples.size() == 1)
      {
         // Only output one sample
         
         Nsamples.getSamples(mDso->renderTime(), n0, n1, blend);
         
         Alembic::Abc::N3fArraySamplePtr vals = n0->data().getVals();
         Alembic::Abc::UInt32ArraySamplePtr idxs = n0->data().getIndices();
         
         nlist = AiArrayAllocate(vals->size(), 1, AI_TYPE_VECTOR);
         
         for (size_t i=0; i<vals->size(); ++i)
         {
            Alembic::Abc::N3f val = vals->get()[i];
            
            vec.x = val.x;
            vec.y = val.y;
            vec.z = val.z;
            
            AiArraySetVec(nlist, i, vec);
         }
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Read %lu normals", vals->size());
         }
         
         if (idxs)
         {
            nidxs = AiArrayAllocate(idxs->size(), 1, AI_TYPE_UINT);
            
            for (size_t i=0; i<idxs->size(); ++i)
            {
               AiArraySetUInt(nidxs, info.arnoldVertexIndex[i], idxs->get()[i]);
            }
            
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read %lu normal indices", idxs->size());
            }
         }
         else
         {
            nidxs = AiArrayAllocate(vals->size(), 1, AI_TYPE_UINT);
            
            for (size_t i=0; i<vals->size(); ++i)
            {
               AiArraySetUInt(nidxs, info.arnoldVertexIndex[i], i);
            }
         }
      }
      else
      {
         size_t numNormals = 0;
         
         for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
         {
            double t = mDso->motionSampleTime(i);
            
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read normal samples [t=%lf]", t);
            }
            
            Nsamples.getSamples(t, n0, n1, blend);
            
            if (blend > 0.0)
            {
               Alembic::Abc::N3fArraySamplePtr vals0 = n0->data().getVals();
               Alembic::Abc::N3fArraySamplePtr vals1 = n1->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs0 = n0->data().getIndices();
               Alembic::Abc::UInt32ArraySamplePtr idxs1 = n1->data().getIndices();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu normals / %lu normals [interpolate]", vals0->size(), vals1->size());
               }
               
               double a = 1.0 - blend;
               
               if (idxs0)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read %lu normal indices", idxs0->size());
                  }
                  
                  if (idxs1)
                  {
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu normal indices", idxs0->size());
                     }
                     
                     if (idxs1->size() != idxs0->size() || (numNormals > 0 && idxs0->size() != numNormals))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     numNormals = idxs0->size();
                     
                     if (!nlist)
                     {
                        nlist = AiArrayAllocate(numNormals, mDso->numMotionSamples(), AI_TYPE_VECTOR);
                     }
                     
                     for (size_t k=0; k<numNormals; ++k)
                     {
                        Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[k]];
                        Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[k]];
                        
                        vec.x = a * val0.x + blend * val1.x;
                        vec.y = a * val0.y + blend * val1.y;
                        vec.z = a * val0.z + blend * val1.z;
                        
                        AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                     }
                  }
                  else
                  {
                     if (vals1->size() != idxs0->size() || (numNormals > 0 && idxs0->size() != numNormals))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     numNormals = idxs0->size();
                     
                     if (!nlist)
                     {
                        nlist = AiArrayAllocate(numNormals, mDso->numMotionSamples(), AI_TYPE_VECTOR);
                     }
                     
                     for (size_t k=0; k<numNormals; ++k)
                     {
                        Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[k]];
                        Alembic::Abc::N3f val1 = vals1->get()[k];
                        
                        vec.x = a * val0.x + blend * val1.x;
                        vec.y = a * val0.y + blend * val1.y;
                        vec.z = a * val0.z + blend * val1.z;
                        
                        AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                     }
                  }
               }
               else
               {
                  if (idxs1)
                  {
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu normal indices", idxs1->size());
                     }
                     
                     if (idxs1->size() != vals0->size() || (numNormals > 0 && vals0->size() != numNormals))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     numNormals = vals0->size();
                     
                     if (!nlist)
                     {
                        nlist = AiArrayAllocate(numNormals, mDso->numMotionSamples(), AI_TYPE_VECTOR);
                     }
                     
                     for (size_t k=0; k<numNormals; ++k)
                     {
                        Alembic::Abc::N3f val0 = vals0->get()[k];
                        Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[k]];
                        
                        vec.x = a * val0.x + blend * val1.x;
                        vec.y = a * val0.y + blend * val1.y;
                        vec.z = a * val0.z + blend * val1.z;
                        
                        AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                     }
                  }
                  else
                  {
                     if (vals1->size() != vals0->size() || (numNormals > 0 && vals0->size() != numNormals))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     numNormals = vals0->size();
                     
                     if (!nlist)
                     {
                        nlist = AiArrayAllocate(numNormals, mDso->numMotionSamples(), AI_TYPE_VECTOR);
                     }
                     
                     for (size_t k=0; k<numNormals; ++k)
                     {
                        Alembic::Abc::N3f val0 = vals0->get()[k];
                        Alembic::Abc::N3f val1 = vals1->get()[k];
                        
                        vec.x = a * val0.x + blend * val1.x;
                        vec.y = a * val0.y + blend * val1.y;
                        vec.z = a * val0.z + blend * val1.z;
                        
                        AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                     }
                  }
               }
            }
            else
            {
               Alembic::Abc::N3fArraySamplePtr vals = n0->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs = n0->data().getIndices();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu normals", vals->size());
               }
            
               if (idxs)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read %lu normal indices", idxs->size());
                  }
                  
                  if (numNormals > 0 && idxs->size() != numNormals)
                  {
                     AiArrayDestroy(nidxs);
                     AiArrayDestroy(nlist);
                     nidxs = 0;
                     nlist = 0;
                     AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                     break;
                  }
                  
                  numNormals = idxs->size();
                  
                  if (!nlist)
                  {
                     nlist = AiArrayAllocate(numNormals, mDso->numMotionSamples(), AI_TYPE_VECTOR);
                  }
                  
                  for (size_t k=0; k<idxs->size(); ++k)
                  {
                     Alembic::Abc::N3f val = vals->get()[idxs->get()[k]];
                     
                     vec.x = val.x;
                     vec.y = val.y;
                     vec.z = val.z;
                     
                     AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                  }
               }
               else
               {
                  if (numNormals > 0 && vals->size() != numNormals)
                  {
                     AiArrayDestroy(nidxs);
                     if (nlist) AiArrayDestroy(nlist);
                     nidxs = 0;
                     nlist = 0;
                     AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                     break;
                  }
                  
                  numNormals = vals->size();
                  
                  if (!nlist)
                  {
                     nlist = AiArrayAllocate(numNormals, mDso->numMotionSamples(), AI_TYPE_VECTOR);
                  }
                  
                  for (size_t k=0; k<vals->size(); ++k)
                  {
                     Alembic::Abc::N3f val = vals->get()[k];
                     
                     vec.x = val.x;
                     vec.y = val.y;
                     vec.z = val.z;
                     
                     AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                  }
               }
            }
            
            if (!nidxs)
            {
               // vertex remapping already happened at values level
               nidxs = AiArrayAllocate(numNormals, 1, AI_TYPE_UINT);
               
               for (size_t k=0; k<numNormals; ++k)
               {
                  AiArraySetUInt(nidxs, k, k);
               }
            }
            
            j += numNormals;
         }
      }
      
      if (nlist && nidxs)
      {
         AiNodeSetArray(mNode, "nlist", nlist);
         AiNodeSetArray(mNode, "nidxs", nidxs);
      }
      else
      {
         AiMsgWarning("[abcproc] Ignore normals: No valid data");
      }
   }
   
   // Output UVs
   
   outputMeshUVs(node, mNode, info);
   
   // Output reference (Pref)
   
   if (mDso->referenceScene())
   {
      AlembicNode *refNode = mDso->referenceScene()->find(node.path());
      
      if (refNode)
      {
         AlembicMesh *refMesh = dynamic_cast<AlembicMesh*>(refNode);
         
         if (refMesh)
         {
            Alembic::AbcGeom::IPolyMeshSchema meshSchema = refMesh->typedObject().getSchema();
            
            Alembic::AbcGeom::IPolyMeshSchema::Sample meshSample = meshSchema.getValue();
            
            Alembic::Abc::P3fArraySamplePtr Pref = meshSample.getPositions();
            
            // Output Pref
            
            if (Pref->size() == info.pointCount &&
                info.pointAttrs.find("Pref") == info.pointAttrs.end())
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Ouptut Pref");
               }
               
               float *vals = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
               
               Alembic::Abc::M44d W;
               
               const AtUserParamEntry *upe = AiNodeLookUpUserParameter(mDso->procNode(), "Mref");
               if (upe != 0 &&
                   AiUserParamGetCategory(upe) == AI_USERDEF_CONSTANT &&
                   AiUserParamGetType(upe) == AI_TYPE_MATRIX)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Using provided Mref");
                  }
                  
                  AtMatrix mtx;
                  AiNodeGetMatrix(mDso->procNode(), "Mref", mtx);
                  for (int r=0; r<4; ++r)
                  {
                     for (int c=0; c<4; ++c)
                     {
                        W[r][c] = mtx[r][c];
                     }
                  }
               }
               else
               {
                  // recompute matrix
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Compute reference world matrix Mref");
                  }
                  
                  AlembicXform *refParent = dynamic_cast<AlembicXform*>(refNode->parent());
                  
                  while (refParent)
                  {
                     Alembic::AbcGeom::IXformSchema xformSchema = refParent->typedObject().getSchema();
                     
                     Alembic::AbcGeom::XformSample xformSample = xformSchema.getValue();
                     
                     W = W * xformSample.getMatrix();
                     
                     if (xformSchema.getInheritsXforms())
                     {
                        refParent = dynamic_cast<AlembicXform*>(refParent->parent());
                     }
                     else
                     {
                        refParent = 0;
                     }
                  }
               }
               
               for (unsigned int p=0, off=0; p<info.pointCount; ++p, off+=3)
               {
                  Alembic::Abc::V3f P = Pref->get()[p] * W;
                  
                  vals[off] = P.x;
                  vals[off+1] = P.y;
                  vals[off+2] = P.z;
               }
               
               UserAttribute &ua = info.pointAttrs["Pref"];
               
               InitUserAttribute(ua);
               
               ua.arnoldCategory = AI_USERDEF_VARYING;
               ua.arnoldType = AI_TYPE_POINT;
               ua.arnoldTypeStr = "POINT";
               ua.dataDim = 3;
               ua.dataCount = info.pointCount;
               ua.data = vals;
            }
            else
            {
               if (Pref->size() != info.pointCount)
               {
                  AiMsgWarning("[abcproc] Point count mismatch in reference alembic for mesh \"%s\"", node.path().c_str());
               }
               else
               {
                  AiMsgWarning("[abcproc] \"Pref\" user attribute already exists, ignore values from reference alembic");
               }
            }
            
            // Nref?
            // Tref?
            // Bref?
         }
      }
   }
   
   // Output user defined attributes
   
   SetUserAttributes(mNode, info.objectAttrs);
   SetUserAttributes(mNode, info.primitiveAttrs);
   SetUserAttributes(mNode, info.pointAttrs);
   SetUserAttributes(mNode, info.vertexAttrs, info.arnoldVertexIndex);
   
   // Make sure we have the 'instance_num' attribute
   
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicSubD &node, AlembicNode *instance)
{
   // generate a polymesh
   Alembic::AbcGeom::ISubDSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(schema);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   MeshInfo info;
   
   info.varyingTopology = (schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   if (schema.getUVsParam().valid())
   {
      info.UVs[""] = schema.getUVsParam();
   }
   
   // Collect attributes
   
   double attribsTime = (info.varyingTopology ? mDso->renderTime() : mDso->attribsTime(mDso->attribsFrame()));
   
   bool interpolateAttribs = !info.varyingTopology;
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         attribsTime,
                         interpolateAttribs,
                         &info.objectAttrs,
                         &info.primitiveAttrs,
                         &info.pointAttrs,
                         &info.vertexAttrs,
                         &info.UVs);
   
   // Create base mesh
   
   mNode = generateBaseMesh(node, info);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output UVs
   
   outputMeshUVs(node, mNode, info);
   
   // Output referece
   
   if (mDso->referenceScene())
   {
      AlembicNode *refNode = mDso->referenceScene()->find(node.path());
      
      if (refNode)
      {
         AlembicSubD *refMesh = dynamic_cast<AlembicSubD*>(refNode);
         
         if (refMesh)
         {
            Alembic::AbcGeom::ISubDSchema meshSchema = refMesh->typedObject().getSchema();
            
            Alembic::AbcGeom::ISubDSchema::Sample meshSample = meshSchema.getValue();
            
            Alembic::Abc::P3fArraySamplePtr Pref = meshSample.getPositions();
            
            // Output Pref
            
            if (Pref->size() == info.pointCount &&
                info.pointAttrs.find("Pref") == info.pointAttrs.end())
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Ouptut Pref");
               }
               
               float *vals = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
               
               Alembic::Abc::M44d W;
               
               const AtUserParamEntry *upe = AiNodeLookUpUserParameter(mDso->procNode(), "Mref");
               if (upe != 0 &&
                   AiUserParamGetCategory(upe) == AI_USERDEF_CONSTANT &&
                   AiUserParamGetType(upe) == AI_TYPE_MATRIX)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Using provided Mref");
                  }
                  
                  AtMatrix mtx;
                  AiNodeGetMatrix(mDso->procNode(), "Mref", mtx);
                  for (int r=0; r<4; ++r)
                  {
                     for (int c=0; c<4; ++c)
                     {
                        W[r][c] = mtx[r][c];
                     }
                  }
               }
               else
               {
                  // recompute matrix
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Compute reference world matrix Mref");
                  }
                  
                  AlembicXform *refParent = dynamic_cast<AlembicXform*>(refNode->parent());
                  
                  while (refParent)
                  {
                     Alembic::AbcGeom::IXformSchema xformSchema = refParent->typedObject().getSchema();
                     
                     Alembic::AbcGeom::XformSample xformSample = xformSchema.getValue();
                     
                     W = W * xformSample.getMatrix();
                     
                     if (xformSchema.getInheritsXforms())
                     {
                        refParent = dynamic_cast<AlembicXform*>(refParent->parent());
                     }
                     else
                     {
                        refParent = 0;
                     }
                  }
               }
               
               for (unsigned int p=0, off=0; p<info.pointCount; ++p, off+=3)
               {
                  Alembic::Abc::V3f P = Pref->get()[p] * W;
                  
                  vals[off] = P.x;
                  vals[off+1] = P.y;
                  vals[off+2] = P.z;
               }
               
               UserAttribute &ua = info.pointAttrs["Pref"];
               
               InitUserAttribute(ua);
               
               ua.arnoldCategory = AI_USERDEF_VARYING;
               ua.arnoldType = AI_TYPE_POINT;
               ua.arnoldTypeStr = "POINT";
               ua.dataDim = 3;
               ua.dataCount = info.pointCount;
               ua.data = vals;
            }
            else
            {
               if (Pref->size() != info.pointCount)
               {
                  AiMsgWarning("[abcproc] Point count mismatch in reference alembic for mesh \"%s\"", node.path().c_str());
               }
               else
               {
                  AiMsgWarning("[abcproc] \"Pref\" user attribute already exists, ignore values from reference alembic");
               }
            }
            
            // Nref?
            // Tref?
            // Bref?
         }
      }
   }
   
   // Output user defined attributes
   
   SetUserAttributes(mNode, info.objectAttrs);
   SetUserAttributes(mNode, info.primitiveAttrs);
   SetUserAttributes(mNode, info.pointAttrs);
   SetUserAttributes(mNode, info.vertexAttrs, info.arnoldVertexIndex);
   
   // Make sure we have the 'instance_num' attribute
   
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicPoints &node, AlembicNode *instance)
{
   Alembic::AbcGeom::IPointsSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(schema);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   PointsInfo info;
   UserAttributes extraPointAttrs;
   
   // Collect attributes
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         mDso->renderTime(),
                         false,
                         &info.objectAttrs,
                         0,
                         &info.pointAttrs,
                         0,
                         0);
   
   // Generate base points
   
   TimeSampleList<Alembic::AbcGeom::IPointsSchema> &samples = node.samples().schemaSamples;
   TimeSampleList<Alembic::AbcGeom::IPointsSchema>::ConstIterator samp0, samp1;
   double a = 1.0;
   double b = 0.0;
   
   for (size_t i=0; i<mDso->numMotionSamples(); ++i)
   {
      double t = mDso->motionSampleTime(i);
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Sample points \"%s\" at t=%lf", node.path().c_str(), t);
      }
   
      node.sampleSchema(t, t, i > 0);
   }
   
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Read %lu points samples", samples.size());
   }
   
   if (samples.size() == 0 ||
       !samples.getSamples(mDso->renderTime(), samp0, samp1, b))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   std::string name = arnoldNodeName(node);
   
   mNode = AiNode("points");
   AiNodeSetStr(mNode, "name", name.c_str());
   
   // Build IDmap
   Alembic::Abc::P3fArraySamplePtr P0 = samp0->data().getPositions();
   Alembic::Abc::V3fArraySamplePtr V0 = samp0->data().getVelocities();
   Alembic::Abc::UInt64ArraySamplePtr ID0 = samp0->data().getIds();
   
   Alembic::Abc::P3fArraySamplePtr P1;
   Alembic::Abc::UInt64ArraySamplePtr ID1;
   Alembic::Abc::V3fArraySamplePtr V1;
   
   std::map<Alembic::Util::uint64_t, size_t> idmap0; // ID -> index in P0/ID0/V0
   std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> > idmap1; // ID -> (index in P1/ID1/V1, index in final point list)
   std::map<Alembic::Util::uint64_t, size_t> sharedids; // ID -> index in P1/ID1/V1
   
   const float *vel0 = 0;
   const float *vel1 = 0;
   const float *acc0 = 0;
   const float *acc1 = 0;
   
   for (size_t i=0; i<ID0->size(); ++i)
   {
      idmap0[ID0->get()[i]] = i;
   }
   
   info.pointCount = idmap0.size();
   
   // Get velocities and accelerations
   
   if (V0)
   {
      vel0 = (const float*) V0->getData();
   }
   else
   {
      UserAttributes::iterator it = info.pointAttrs.find("velocity");
      if (it == info.pointAttrs.end() ||
          it->second.arnoldType != AI_TYPE_VECTOR ||
          it->second.dataCount != info.pointCount)
      {
         it = info.pointAttrs.find("v");
         if (it != info.pointAttrs.end() &&
             (it->second.arnoldType != AI_TYPE_VECTOR ||
              it->second.dataCount != info.pointCount))
         {
            it = info.pointAttrs.end();
         }
      }
      if (it != info.pointAttrs.end())
      {
         vel0 = (const float*) it->second.data;
      }
   }
   
   if (vel0)
   {
      UserAttributes::iterator it = info.pointAttrs.find("acceleration");
      if (it == info.pointAttrs.end() ||
          it->second.arnoldType != AI_TYPE_VECTOR ||
          it->second.dataCount != info.pointCount)
      {
         it = info.pointAttrs.find("accel");
         if (it == info.pointAttrs.end() ||
             it->second.arnoldType != AI_TYPE_VECTOR ||
             it->second.dataCount != info.pointCount)
         {
            it = info.pointAttrs.find("a");
            if (it != info.pointAttrs.end() &&
                (it->second.arnoldType != AI_TYPE_VECTOR ||
                 it->second.dataCount != info.pointCount))
            {
               it = info.pointAttrs.end();
            }
         }
      }
      if (it != info.pointAttrs.end())
      {
         acc0 = (const float*) it->second.data;
      }
   }
   
   if (b > 0.0)
   {
      P1 = samp1->data().getPositions();
      ID1 = samp1->data().getIds();
      V1 = samp1->data().getVelocities();
      
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
            idxs.second = info.pointCount++;
            
            idmap1[id] = idxs;
         }
      }
      
      if (idmap1.size() > 0)
      {
         // Collect point attributes
         
         collectUserAttributes(Alembic::Abc::ICompoundProperty(), schema.getArbGeomParams(),
                               samp1->time(), false, 0, 0, &extraPointAttrs, 0, 0);
         
         // Get velocities and accelerations
         
         if (V1)
         {
            vel1 = (const float*) V1->getData();
         }
         else
         {
            UserAttributes::iterator it = extraPointAttrs.find("velocity");
            if (it == extraPointAttrs.end() ||
                it->second.arnoldType != AI_TYPE_VECTOR ||
                it->second.dataCount != P1->size())
            {
               it = extraPointAttrs.find("v");
               if (it != extraPointAttrs.end() &&
                   (it->second.arnoldType != AI_TYPE_VECTOR ||
                    it->second.dataCount != P1->size()))
               {
                  it = extraPointAttrs.end();
               }
            }
            if (it != extraPointAttrs.end())
            {
               vel1 = (const float*) it->second.data;
            }
         }
         
         if (vel1)
         {
            UserAttributes::iterator it = extraPointAttrs.find("acceleration");
            if (it == extraPointAttrs.end() ||
                it->second.arnoldType != AI_TYPE_VECTOR ||
                it->second.dataCount != info.pointCount)
            {
               it = extraPointAttrs.find("accel");
               if (it == extraPointAttrs.end() ||
                   it->second.arnoldType != AI_TYPE_VECTOR ||
                   it->second.dataCount != info.pointCount)
               {
                  it = extraPointAttrs.find("a");
                  if (it != extraPointAttrs.end() &&
                      (it->second.arnoldType != AI_TYPE_VECTOR ||
                       it->second.dataCount != info.pointCount))
                  {
                     it = extraPointAttrs.end();
                  }
               }
            }
            if (it != extraPointAttrs.end())
            {
               acc1 = (const float*) it->second.data;
            }
         }
      }
   }
   
   // Fill points
   
   unsigned int nkeys = (vel0 ? mDso->numMotionSamples() : 1);
   
   AtArray *points = AiArrayAllocate(info.pointCount, nkeys, AI_TYPE_POINT);
   
   std::map<Alembic::Util::uint64_t, size_t>::iterator idit;
   AtPoint pnt;
   
   for (unsigned int i=0, koff=0; i<nkeys; ++i, koff+=info.pointCount)
   {
      double t = (vel0 ? mDso->motionSampleTime(i) : mDso->renderTime());
      
      for (size_t j=0, voff=0; j<P0->size(); ++j, voff+=3)
      {
         Alembic::Util::uint64_t id = ID0->get()[j];
         
         idit = sharedids.find(id);
         
         if (idit != sharedids.end())
         {
            Alembic::Abc::V3f P = float(a) * P0->get()[j] + float(b) * P1->get()[idit->second];
            
            pnt.x = P.x;
            pnt.y = P.y;
            pnt.z = P.z;
         }
         else
         {
            Alembic::Abc::V3f P = P0->get()[j];
            
            pnt.x = P.x;
            pnt.y = P.y;
            pnt.z = P.z;
            
            if (vel0)
            {
               double dt = t - samp0->time();
               double hdt2 = 0.5 * dt * dt;
               
               pnt.x += dt * vel0[voff  ];
               pnt.y += dt * vel0[voff+1];
               pnt.z += dt * vel0[voff+2];
               
               if (acc0)
               {
                  pnt.x += hdt2 * acc0[voff  ];
                  pnt.y += hdt2 * acc0[voff+1];
                  pnt.z += hdt2 * acc0[voff+2];
               }
            }
         }
         
         AiArraySetPnt(points, koff+j, pnt);
      }
      
      std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator it;
      
      for (it=idmap1.begin(); it!=idmap1.end(); ++it)
      {
         Alembic::Abc::V3f P = P1->get()[it->second.first];
         
         pnt.x = P.x;
         pnt.y = P.y;
         pnt.z = P.z;
         
         if (vel1)
         {
            double dt = t - samp1->time();
            double hdt2 = 0.5 * dt * dt;
            
            unsigned int voff = 3 * it->second.first;
            
            pnt.x += dt * vel1[voff  ];
            pnt.y += dt * vel1[voff+1];
            pnt.z += dt * vel1[voff+2];
            
            if (acc1)
            {
               pnt.x += hdt2 * acc1[voff  ];
               pnt.y += hdt2 * acc1[voff+1];
               pnt.z += hdt2 * acc1[voff+2];
            }
         }
         
         AiArraySetPnt(points, koff + it->second.second, pnt);
      }
   }
   
   // mode: disk|sphere|quad
   // radius[]
   // aspect[]    (only for quad)
   // rotation[]  (only for quad)
   //
   // Use alternatives for mode, aspect, rotation and radius?
   // radius is handled by Alembic as 'widths' property
   
   Alembic::AbcGeom::IFloatGeomParam widths = schema.getWidthsParam();
   if (widths.valid())
   {
      // Convert to point attribute 'radius'
      
      if (info.pointAttrs.find("radius") != info.pointAttrs.end() ||
          info.objectAttrs.find("radius") != info.objectAttrs.end())
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Ignore alembic \"widths\" property: \"radius\" attribute set");
         }
      }
      else
      {
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam> wsamples;
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam>::ConstIterator wsamp0, wsamp1;
         
         if (!wsamples.update(widths, mDso->renderTime(), mDso->renderTime(), false))
         {
            // default to 1?
         }
         else if (!wsamples.getSamples(mDso->renderTime(), wsamp0, wsamp1, b))
         {
            // default to 1?
         }
         else
         {
            float *radius = (float*) AiMalloc(info.pointCount * sizeof(float));
            
            for (size_t i=0; i<P0->size(); ++i)
            {
               Alembic::Abc::FloatArraySamplePtr r0 = wsamp0->data().getVals();
            }
            
            // 
         }
      }
   }
   
   // Output user defined attributes
   
   // Apply dso radiusmin/radiusmax/radiusscale to 'radius' point attribute
   
   if (idmap1.size() > 0)
   {
      // Extend point attributes data (skip radius already processed above)
      
      for (UserAttributes::iterator it = info.pointAttrs.begin(); it != info.pointAttrs.end(); ++it)
      {
         if (extraPointAttrs.find(it->first) == extraPointAttrs.end())
         {
            // Fill with adhoc values?
         }
         else
         {
            // Use idmap1 to extend dataCount to info.pointCount
         }
      }
   }
   
   SetUserAttributes(mNode, info.objectAttrs);
   SetUserAttributes(mNode, info.pointAttrs);
   
   // Make sure we have the 'instance_num' attribute
   
   outputInstanceNumber(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicNode &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}

void MakeShape::leave(AlembicNode &, AlembicNode *)
{
}
