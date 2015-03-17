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

AlembicNode::VisitReturn GetTimeRange::enter(AlembicPoints &node, AlembicNode *)
{
   updateTimeRange(node);
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

AlembicNode::VisitReturn CountShapes::enter(AlembicPoints &node, AlembicNode *)
{
   return shapeEnter(node);
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
      
      double renderTime = mDso->renderTime();
      const double *sampleTimes = 0;
      size_t sampleTimesCount = 0;
      
      if (mDso->ignoreTransformBlur())
      {
         sampleTimes = &renderTime;
         sampleTimesCount = 1;
      }
      else
      {
         sampleTimes = &(mDso->motionSampleTimes()[0]);
         sampleTimesCount = mDso->numMotionSamples();
      }
      
      for (size_t i=0; i<sampleTimesCount; ++i)
      {
         double t = sampleTimes[i];
         
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
         for (size_t i=0; i<sampleTimesCount; ++i)
         {
            Alembic::Abc::M44d matrix;
            bool inheritXforms = true;
            
            double t = sampleTimes[i];
            
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
   bool interpolate = (node.typedObject().getSchema().getTopologyVariance() != Alembic::AbcGeom::kHeterogenousTopology);
   return shapeEnter(node, instance, interpolate, 0.0);
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicSubD &node, AlembicNode *instance)
{
   bool interpolate = (node.typedObject().getSchema().getTopologyVariance() != Alembic::AbcGeom::kHeterogenousTopology);
   return shapeEnter(node, instance, interpolate, 0.0);
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicPoints &node, AlembicNode *instance)
{
   // Take into account to point size in the computed bounds
   
   Alembic::Util::bool_t visible = (mDso->ignoreVisibility() ? true : GetVisibility(node.object().getProperties(), mDso->renderTime()));
   
   double extraPadding = 0.0;
   
   if (visible)
   {
      Alembic::AbcGeom::IFloatGeomParam widths = node.typedObject().getSchema().getWidthsParam();
      
      if (widths.valid())
      {
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam> wsamples;
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam>::ConstIterator wsample;
            
         double renderTime = mDso->renderTime();
         
         const double *sampleTimes = 0;
         size_t sampleTimesCount = 0;
         
         if (mDso->ignoreDeformBlur())
         {
            sampleTimes = &renderTime;
            sampleTimesCount = 1;
         }
         else
         {
            sampleTimes = &(mDso->motionSampleTimes()[0]);
            sampleTimesCount = mDso->numMotionSamples();
         }
            
         for (size_t i=0; i<sampleTimesCount; ++i)
         {
            double t = sampleTimes[i];
            
            wsamples.update(widths, t, t, (i > 0 || instance != 0));
         }
         
         for (wsample=wsamples.begin(); wsample!=wsamples.end(); ++wsample)
         {
            Alembic::Abc::FloatArraySamplePtr vals = wsample->data().getVals();
            
            for (size_t i=0; i<vals->size(); ++i)
            {
               float r = vals->get()[i];
               if (r > extraPadding)
               {
                  extraPadding = r;
               }
            }
         }
      }
      else
      {
         extraPadding = mDso->radiusMin();
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Points extra bounds padding: %lf", extraPadding);
      }
   }
   
   return shapeEnter(node, instance, false, extraPadding);
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
         
         //ua.second.arnoldCategory = AI_USERDEF_CONSTANT;
         
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
                  //ua.second.arnoldCategory = AI_USERDEF_INDEXED;
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
                  //ua.second.arnoldCategory = AI_USERDEF_VARYING;
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
                  //ua.second.arnoldCategory = AI_USERDEF_UNIFORM;
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
                  //ua.second.arnoldCategory = AI_USERDEF_CONSTANT;
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
   bool NperPoint = false;
   
   if (readNormals && N.valid() &&
       (N.getScope() == Alembic::AbcGeom::kFacevaryingScope ||
        N.getScope() == Alembic::AbcGeom::kVaryingScope))
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
      
      NperPoint = (N.getScope() == Alembic::AbcGeom::kVaryingScope);
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
         
         if (NperPoint)
         {
            if (idxs)
            {
               nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
               
               for (unsigned int i=0; i<info.vertexCount; ++i)
               {
                  unsigned int vidx = info.arnoldVertexIndex[i];
                  unsigned int pidx = info.vertexPointIndex[vidx];
                  
                  if (pidx >= idxs->size())
                  {
                     AiMsgWarning("[abcproc] Invalid normal index");
                     AiArraySetUInt(nidxs, vidx, 0);
                  }
                  else
                  {
                     AiArraySetUInt(nidxs, vidx, idxs->get()[pidx]);
                  }
               }
            }
            else
            {
               nidxs = AiArrayCopy(AiNodeGetArray(mNode, "vidxs"));
            }
         }
         else
         {     
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
      }
      else
      {
         for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
         {
            double t = mDso->motionSampleTime(i);
            
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read normal samples [t=%lf]", t);
            }
            
            Nsamples.getSamples(t, n0, n1, blend);
            
            if (!nlist)
            {
               nlist = AiArrayAllocate(info.vertexCount, mDso->numMotionSamples(), AI_TYPE_VECTOR);
            }
            
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
                     
                     if (idxs0->size() != idxs1->size() ||
                         ( NperPoint && idxs0->size() != info.pointCount) ||
                         (!NperPoint && idxs0->size() != info.vertexCount))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     if (NperPoint)
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
                        {
                           unsigned int vidx = info.arnoldVertexIndex[k];
                           unsigned int pidx = info.vertexPointIndex[vidx];
                           
                           if (pidx >= idxs0->size() || pidx >= idxs1->size())
                           {
                              AiMsgWarning("[abcproc] Invalid normal index");
                              AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                           }
                           else
                           {
                              Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[pidx]];
                              Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[pidx]];
                              
                              vec.x = a * val0.x + blend * val1.x;
                              vec.y = a * val0.y + blend * val1.y;
                              vec.z = a * val0.z + blend * val1.z;
                              
                              AiArraySetVec(nlist, j + vidx, vec);
                           }
                        }
                     }
                     else
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
                        {
                           Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[k]];
                           Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[k]];
                           
                           vec.x = a * val0.x + blend * val1.x;
                           vec.y = a * val0.y + blend * val1.y;
                           vec.z = a * val0.z + blend * val1.z;
                           
                           AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                        }
                     }
                  }
                  else
                  {
                     if (vals1->size() != idxs0->size() ||
                         ( NperPoint && vals1->size() != info.pointCount) ||
                         (!NperPoint && vals1->size() != info.vertexCount))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     if (NperPoint)
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
                        {
                           unsigned int vidx = info.arnoldVertexIndex[k];
                           unsigned int pidx = info.vertexPointIndex[vidx];
                           
                           if (pidx >= vals1->size() || pidx >= idxs0->size())
                           {
                              AiMsgWarning("[abcproc] Invalid normal index");
                              AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                           }
                           else
                           {
                              Alembic::Abc::N3f val0 = vals0->get()[idxs0->get()[pidx]];
                              Alembic::Abc::N3f val1 = vals1->get()[pidx];
                              
                              vec.x = a * val0.x + blend * val1.x;
                              vec.y = a * val0.y + blend * val1.y;
                              vec.z = a * val0.z + blend * val1.z;
                              
                              AiArraySetVec(nlist, j + vidx, vec);
                           }
                        }
                     }
                     else
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
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
               }
               else
               {
                  if (idxs1)
                  {
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu normal indices", idxs1->size());
                     }
                     
                     if (vals0->size() != idxs1->size() ||
                         ( NperPoint && vals0->size() != info.pointCount) ||
                         (!NperPoint && vals0->size() != info.vertexCount))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     if (NperPoint)
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
                        {
                           unsigned int vidx = info.arnoldVertexIndex[k];
                           unsigned int pidx = info.vertexPointIndex[vidx];
                           
                           if (pidx >= vals0->size() || pidx >= idxs1->size())
                           {
                              AiMsgWarning("[abcproc] Invalid normal index");
                              AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                           }
                           else
                           {
                              Alembic::Abc::N3f val0 = vals0->get()[pidx];
                              Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[pidx]];
                              
                              vec.x = a * val0.x + blend * val1.x;
                              vec.y = a * val0.y + blend * val1.y;
                              vec.z = a * val0.z + blend * val1.z;
                              
                              AiArraySetVec(nlist, j + vidx, vec);
                           }
                        }
                     }
                     else
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
                        {
                           Alembic::Abc::N3f val0 = vals0->get()[k];
                           Alembic::Abc::N3f val1 = vals1->get()[idxs1->get()[k]];
                           
                           vec.x = a * val0.x + blend * val1.x;
                           vec.y = a * val0.y + blend * val1.y;
                           vec.z = a * val0.z + blend * val1.z;
                           
                           AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                        }
                     }
                  }
                  else
                  {
                     if (vals1->size() != vals0->size() ||
                         ( NperPoint && vals0->size() != info.pointCount) ||
                         (!NperPoint && vals0->size() != info.vertexCount))
                     {
                        if (nidxs) AiArrayDestroy(nidxs);
                        if (nlist) AiArrayDestroy(nlist);
                        nidxs = 0;
                        nlist = 0;
                        AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                        break;
                     }
                     
                     if (NperPoint)
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
                        {
                           unsigned int vidx = info.arnoldVertexIndex[k];
                           unsigned int pidx = info.vertexPointIndex[vidx];
                           
                           if (pidx >= vals0->size())
                           {
                              AiMsgWarning("[abcproc] Invalid normal index");
                              AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                           }
                           else
                           {
                              Alembic::Abc::N3f val0 = vals0->get()[pidx];
                              Alembic::Abc::N3f val1 = vals1->get()[pidx];
                              
                              vec.x = a * val0.x + blend * val1.x;
                              vec.y = a * val0.y + blend * val1.y;
                              vec.z = a * val0.z + blend * val1.z;
                              
                              AiArraySetVec(nlist, j + vidx, vec);
                           }
                        }
                     }
                     else
                     {
                        for (unsigned int k=0; k<info.vertexCount; ++k)
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
                  
                  if (( NperPoint && idxs->size() != info.pointCount ) ||
                      (!NperPoint && idxs->size() != info.vertexCount))
                  {
                     AiArrayDestroy(nidxs);
                     AiArrayDestroy(nlist);
                     nidxs = 0;
                     nlist = 0;
                     AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                     break;
                  }
                  
                  if (NperPoint)
                  {
                     for (unsigned int k=0; k<info.vertexCount; ++k)
                     {
                        unsigned int vidx = info.arnoldVertexIndex[k];
                        unsigned int pidx = info.vertexPointIndex[vidx];
                        
                        if (pidx >= idxs->size())
                        {
                           AiMsgWarning("[abcproc] Invalid normal index");
                           AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                        }
                        else
                        {
                           Alembic::Abc::N3f val = vals->get()[idxs->get()[pidx]];
                           
                           vec.x = val.x;
                           vec.y = val.y;
                           vec.z = val.z;
                           
                           AiArraySetVec(nlist, j + vidx, vec);
                        }
                     }
                  }
                  else
                  {
                     for (unsigned int k=0; k<info.vertexCount; ++k)
                     {
                        Alembic::Abc::N3f val = vals->get()[idxs->get()[k]];
                        
                        vec.x = val.x;
                        vec.y = val.y;
                        vec.z = val.z;
                        
                        AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                     }
                  }
               }
               else
               {
                  if (( NperPoint && vals->size() != info.pointCount ) ||
                      (!NperPoint && vals->size() != info.vertexCount))
                  {
                     AiArrayDestroy(nidxs);
                     if (nlist) AiArrayDestroy(nlist);
                     nidxs = 0;
                     nlist = 0;
                     AiMsgWarning("[abcproc] Ignore normals: non uniform samples");
                     break;
                  }
                  
                  if (NperPoint)
                  {
                     for (unsigned int k=0; k<info.vertexCount; ++k)
                     {
                        unsigned int vidx = info.arnoldVertexIndex[k];
                        unsigned int pidx = info.vertexPointIndex[vidx];
                        
                        if (pidx >= vals->size())
                        {
                           AiMsgWarning("[abcproc] Invalid normal index");
                           AiArraySetVec(nlist, j + vidx, AI_V3_ZERO);
                        }
                        else
                        {
                           Alembic::Abc::N3f val = vals->get()[pidx];
                        
                           vec.x = val.x;
                           vec.y = val.y;
                           vec.z = val.z;
                           
                           AiArraySetVec(nlist, j + vidx, vec);
                        }
                     }
                  }
                  else
                  {
                     for (unsigned int k=0; k<info.vertexCount; ++k)
                     {
                        Alembic::Abc::N3f val = vals->get()[k];
                        
                        vec.x = val.x;
                        vec.y = val.y;
                        vec.z = val.z;
                        
                        AiArraySetVec(nlist, j + info.arnoldVertexIndex[k], vec);
                     }
                  }
               }
            }
            
            if (!nidxs)
            {
               // vertex remapping already happened at values level
               nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
               
               for (unsigned int k=0; k<info.vertexCount; ++k)
               {
                  AiArraySetUInt(nidxs, k, k);
               }
            }
            
            j += info.vertexCount;
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
               ua.isArray = true;
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
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.primitiveAttrs, info.polygonCount);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
   SetUserAttributes(mNode, info.vertexAttrs, info.vertexCount, info.arnoldVertexIndex);
   
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
               ua.isArray = true;
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
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.primitiveAttrs, info.polygonCount);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
   SetUserAttributes(mNode, info.vertexAttrs, info.vertexCount, info.arnoldVertexIndex);
   
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
   
   // renderTime() may not be in the sample list, let's at it just in case
   node.sampleSchema(mDso->renderTime(), mDso->renderTime(), true);
   
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
   
   // Collect attributes
   
   collectUserAttributes(schema.getUserProperties(),
                         schema.getArbGeomParams(),
                         samp0->time(),
                         false,
                         &info.objectAttrs,
                         0,
                         &info.pointAttrs,
                         0,
                         0);
   
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
   std::string vname, aname;
   
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
         vname = it->first;
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
         aname = it->first;
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
      
      // Collect point attributes
      
      collectUserAttributes(Alembic::Abc::ICompoundProperty(), schema.getArbGeomParams(),
                            samp1->time(), false, 0, 0, &extraPointAttrs, 0, 0);
      
      // Get velocities and accelerations
      
      if (V1)
      {
         vel1 = (const float*) V1->getData();
      }
      else if (vname.length() > 0)
      {
         // Don't really have to check it as we are sampling the same schema, just at a different time
         UserAttributes::iterator it = extraPointAttrs.find(vname);
         if (it != extraPointAttrs.end())
         {
            vel1 = (const float*) it->second.data;
         }
      }
      
      if (aname.length() > 0)
      {
         // Don't really have to check it as we are sampling the same schema, just at a different time
         UserAttributes::iterator it = extraPointAttrs.find(aname);
         if (it != extraPointAttrs.end())
         {
            acc1 = (const float*) it->second.data;
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
            
            if (vel0 && vel1)
            {
               // Note: a * samp0->time() + b * samp1->time() == renderTime
               double dt = t - mDso->renderTime();
               
               size_t off = idit->second * 3;
               
               pnt.x += dt * (a * vel0[voff  ] + b * vel1[off  ]);
               pnt.y += dt * (a * vel0[voff+1] + b * vel1[off+1]);
               pnt.z += dt * (a * vel0[voff+2] + b * vel1[off+2]);
               
               if (acc0 && acc1)
               {
                  double hdt2 = 0.5 * dt * dt;
                  
                  pnt.x += hdt2 * (a * acc0[voff  ] + b * acc1[off  ]);
                  pnt.y += hdt2 * (a * acc0[voff+1] + b * acc1[off+1]);
                  pnt.z += hdt2 * (a * acc0[voff+2] + b * acc1[off+2]);
               }
               
               // Note: should adjust velocity and acceleration attribute values in final output too
            }
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
               
               pnt.x += dt * vel0[voff  ];
               pnt.y += dt * vel0[voff+1];
               pnt.z += dt * vel0[voff+2];
               
               if (acc0)
               {
                  double hdt2 = 0.5 * dt * dt;
                  
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
            
            unsigned int voff = 3 * it->second.first;
            
            pnt.x += dt * vel1[voff  ];
            pnt.y += dt * vel1[voff+1];
            pnt.z += dt * vel1[voff+2];
            
            if (acc1)
            {
               double hdt2 = 0.5 * dt * dt;
               
               pnt.x += hdt2 * acc1[voff  ];
               pnt.y += hdt2 * acc1[voff+1];
               pnt.z += hdt2 * acc1[voff+2];
            }
         }
         
         AiArraySetPnt(points, koff + it->second.second, pnt);
      }
   }
   
   AiNodeSetArray(mNode, "points", points);
   
   // mode: disk|sphere|quad
   // radius[]
   // aspect[]    (only for quad)
   // rotation[]  (only for quad)
   //
   // Use alternative attribtues for mode, aspect, rotation and radius?
   //
   // radius is handled by Alembic as 'widths' property
   // give priority to user defined 'radius' 
   
   Alembic::AbcGeom::IFloatGeomParam widths = schema.getWidthsParam();
   if (widths.valid())
   {
      // Convert to point attribute 'radius'
      
      if (info.pointAttrs.find("radius") != info.pointAttrs.end())
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Ignore alembic \"widths\" property: \"radius\" attribute set");
         }
      }
      else
      {
         UserAttributes::iterator it = info.objectAttrs.find("radius");
         
         if (it != info.objectAttrs.end())
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Ignore \"radius\" object attribute");
            }
            DestroyUserAttribute(it->second);
            info.objectAttrs.erase(it);
         }
         
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam> wsamples;
         TimeSampleList<Alembic::AbcGeom::IFloatGeomParam>::ConstIterator wsamp0, wsamp1;
         
         double br = 0.0;
         
         if (!wsamples.update(widths, mDso->renderTime(), mDso->renderTime(), false))
         {
            AiMsgWarning("[abcproc] Could not read alembic points \"widths\" property for t = %lf", mDso->renderTime());
            // default to 1?
         }
         else if (!wsamples.getSamples(mDso->renderTime(), wsamp0, wsamp1, br))
         {
            AiMsgWarning("[abcproc] Could not read alembic points \"widths\" property for t = %lf", mDso->renderTime());
            // default to 1?
         }
         else
         {
            Alembic::Abc::FloatArraySamplePtr R0 = wsamp0->data().getVals();
            Alembic::Abc::FloatArraySamplePtr R1;
            
            if (wsamp0->time() != samp0->time())
            {
               AiMsgWarning("[abcproc] \"widths\" property sample time doesn't match points schema sample time (%lf and %lf respectively)",
                            samp0->time(), wsamp0->time());
            }
            else if (R0->size() != P0->size())
            {
               AiMsgWarning("[abcproc] \"widths\" property size doesn't match render time point count");
            }
            else
            {
               bool process = true;
               
               if (br > 0.0)
               {
                  R1 = wsamp1->data().getVals();
                  
                  if (wsamp1->time() != samp1->time())
                  {
                     AiMsgWarning("[abcproc] \"widths\" property sample time doesn't match points schema sample time (%lf and %lf respectively)",
                                  samp1->time(), wsamp1->time());
                     process = false;
                  }
                  else if (R1->size() != P1->size())
                  {
                     AiMsgWarning("[abcproc] \"widths\" property size doesn't match render time point count");
                     process = false;
                  }
               }
               
               if (process)
               {
                  float *radius = (float*) AiMalloc(info.pointCount * sizeof(float));
                  float r;
                  
                  if (R1)
                  {
                     for (size_t i=0; i<R0->size(); ++i)
                     {
                        Alembic::Util::uint64_t id = ID0->get()[i];
                        
                        idit = sharedids.find(id);
                        
                        if (idit != sharedids.end())
                        {
                           r = (1 - br) * R0->get()[i] + br * R1->get()[idit->second];
                        }
                        else
                        {
                           r = R0->get()[i];
                        }
                        
                        radius[i] = adjustPointRadius(r);
                     }
                     
                     std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator idit1;
                     
                     for (idit1 = idmap1.begin(); idit1 != idmap1.end(); ++idit1)
                     {
                        r = R1->get()[idit1->second.first];
                        
                        radius[idit1->second.second] = adjustPointRadius(r);
                     }
                  }
                  else
                  {
                     // asset(R0->size() == info.pointCount);
                     
                     for (size_t i=0; i<R0->size(); ++i)
                     {
                        r = R0->get()[i];
                        
                        radius[i] = adjustPointRadius(r);
                     }
                  }
                  
                  AiNodeSetArray(mNode, "radius", AiArrayConvert(info.pointCount, 1, AI_TYPE_FLOAT, radius));
               }
            }
         }
      }
   }
   
   // Output user defined attributes
   
   // Extend existing attributes
   
   if (idmap1.size() > 0)
   {
      for (UserAttributes::iterator it0 = info.pointAttrs.begin(); it0 != info.pointAttrs.end(); ++it0)
      {
         ResizeUserAttribute(it0->second, info.pointCount);
         
         UserAttributes::iterator it1 = extraPointAttrs.find(it0->first);
         
         if (it1 != extraPointAttrs.end())
         {
            std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator idit;
            
            for (idit = idmap1.begin(); idit != idmap1.end(); ++idit)
            {
               if (!CopyUserAttribute(it1->second, idit->second.first, 1,
                                      it0->second, idit->second.second))
               {
                  AiMsgWarning("[abcproc] Failed to copy extended user attribute data");
               }
            }
         }
      }
   }
   
   // Adjust radiuses
   
   UserAttribute *ra = 0;
   
   UserAttributes::iterator ait = info.pointAttrs.find("radius");
   
   if (ait == info.pointAttrs.end())
   {
      ait = info.objectAttrs.find("radius");
      
      if (ait != info.objectAttrs.end())
      {
         ra = &(ait->second);
      }
   }
   else
   {
      ra = &(ait->second);
   }
   
   if (ra && ra->data && ra->arnoldType == AI_TYPE_FLOAT)
   {
      float *r = (float*) ra->data;
         
      for (unsigned int i=0; i<ra->dataCount; ++i)
      {
         for (unsigned int j=0; i<ra->dataDim; ++j, ++r)
         {
            *r = adjustPointRadius(*r);
         }
      }
   }
   else
   {
      AiMsgInfo("[abcproc] No radius set for points in alembic archive. Create particles with constant radius %lf (can be changed using dso '-radiusmin' data flag or 'abc_radiusmin' user attribute)", mDso->radiusMin());
      
      AtArray *radius = AiArrayAllocate(1, 1, AI_TYPE_FLOAT);
      AiArraySetFlt(radius, 0, mDso->radiusMin());
      
      AiNodeSetArray(mNode, "radius", radius);
   }
   
   // Note: arnold want particle point attributes as uniform attributes, not varying
   for (UserAttributes::iterator it = info.pointAttrs.begin(); it != info.pointAttrs.end(); ++it)
   {
      it->second.arnoldCategory = AI_USERDEF_UNIFORM;
   }
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
   
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
