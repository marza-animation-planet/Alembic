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
                                      std::map<std::string, Alembic::AbcGeom::IV2fGeomParam> *UVs,
                                      MakeShape::CollectFilter filter)
{
   if (userProps.valid() && objectAttrs && mDso->readObjectAttribs())
   {
      for (size_t i=0; i<userProps.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = userProps.getPropertyHeader(i);
         
         if (filter && !filter(header))
         {
            continue;
         }
         
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
      std::set<std::string> specialVertexNames;
      
      specialPointNames.insert("acceleration");
      specialPointNames.insert("accel");
      specialPointNames.insert("a");
      specialPointNames.insert("velocity");
      specialPointNames.insert("v");
      if (mDso->outputReference())
      {
         specialPointNames.insert("Pref");
         specialPointNames.insert("Nref");
         specialVertexNames.insert("Nref");
      }
      
      for (size_t i=0; i<geomParams.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = geomParams.getPropertyHeader(i);
         
         Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
         
         if (UVs &&
             scope == Alembic::AbcGeom::kFacevaryingScope &&
             Alembic::AbcGeom::IV2fGeomParam::matches(header) &&
             header.getMetaData().get("notUV") != "1")
             //Alembic::AbcGeom::isUV(header))
         {
            if (filter && !filter(header))
            {
               continue;
            }
            
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
               if (vertexAttrs && (mDso->readVertexAttribs() || specialVertexNames.find(ua.first) != specialVertexNames.end()))
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
               if (filter && !filter(header))
               {
                  DestroyUserAttribute(ua.second);
               }
               else
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
}


float* MakeShape::computeMeshSmoothNormals(MakeShape::MeshInfo &info,
                                           const float *P0,
                                           const float *P1,
                                           float blend)
{
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Compute smooth normals");
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
         
         const float *P0a = P0 + 3 * pi[0];
         const float *P0b = P0 + 3 * pi[1];
         const float *P0c = P0 + 3 * pi[2];
         
         if (P1)
         {
            const float *P1a = P1 + 3 * pi[0];
            const float *P1b = P1 + 3 * pi[1];
            const float *P1c = P1 + 3 * pi[2];
            
            float iblend = (1.0f - blend);
            
            p0.x = iblend * P0a[0] + blend * P1a[0];
            p0.y = iblend * P0a[1] + blend * P1a[1];
            p0.z = iblend * P0a[2] + blend * P1a[2];
            
            p1.x = iblend * P0b[0] + blend * P1b[0];
            p1.y = iblend * P0b[1] + blend * P1b[1];
            p1.z = iblend * P0b[2] + blend * P1b[2];
            
            p2.x = iblend * P0c[0] + blend * P1c[0];
            p2.y = iblend * P0c[1] + blend * P1c[1];
            p2.z = iblend * P0c[2] + blend * P1c[2];
         }
         else
         {
            p0.x = P0a[0];
            p0.y = P0a[1];
            p0.z = P0a[2];
            
            p1.x = P0b[0];
            p1.y = P0b[1];
            p1.z = P0b[2];
            
            p2.x = P0c[0];
            p2.y = P0c[1];
            p2.z = P0c[2];
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

bool MakeShape::FilterReferenceAttributes(const Alembic::AbcCoreAbstract::PropertyHeader &header)
{
   if (header.getName() == "Pref")
   {
      Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
      
      return (scope == Alembic::AbcGeom::kVaryingScope || scope == Alembic::AbcGeom::kVertexScope);
   }
   else if (header.getName() == "Nref")
   {
      Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
      
      return (scope != Alembic::AbcGeom::kConstantScope && scope != Alembic::AbcGeom::kUniformScope);
   }
   else
   {
      return false;
   }
}

bool MakeShape::checkReferenceNormals(MeshInfo &info,
                                      UserAttributes *pointAttrs,
                                      UserAttributes *vertexAttrs)
{
   bool hasNref = false;
   
   UserAttributes::iterator uait = vertexAttrs->find("Nref");
   
   if (uait != vertexAttrs->end())
   {
      if (isIndexedFloat3(info, uait->second))
      {
         hasNref = true;
      }
      else
      {
         DestroyUserAttribute(uait->second);
         vertexAttrs->erase(uait);
      }
   }
   
   if (!hasNref)
   {
      uait = pointAttrs->find("Nref");
      
      if (uait != pointAttrs->end())
      {
         if (isVaryingFloat3(info, uait->second))
         {
            hasNref = true;
         }
         else
         {
            DestroyUserAttribute(uait->second);
            pointAttrs->erase(uait);
         }
      }
   }
   
   return hasNref;
}

bool MakeShape::readReferenceNormals(AlembicMesh *refMesh,
                                     MeshInfo &info,
                                     const Alembic::Abc::M44d &Mref)
{
   Alembic::AbcGeom::IPolyMeshSchema meshSchema = refMesh->typedObject().getSchema();
   
   Alembic::AbcGeom::IN3fGeomParam Nref = meshSchema.getNormalsParam();
   
   if (Nref.valid() && (Nref.getScope() == Alembic::AbcGeom::kFacevaryingScope ||
                        Nref.getScope() == Alembic::AbcGeom::kVertexScope ||
                        Nref.getScope() == Alembic::AbcGeom::kVaryingScope))
   {
      Alembic::AbcGeom::IN3fGeomParam::Sample sample = Nref.getIndexedValue();
      
      Alembic::Abc::N3fArraySamplePtr vals = sample.getVals();
      Alembic::Abc::UInt32ArraySamplePtr idxs = sample.getIndices();
      
      bool NrefPerPoint = (Nref.getScope() != Alembic::AbcGeom::kFacevaryingScope);
      bool validSize = false;
      
      if (idxs)
      {
         validSize = (NrefPerPoint ? idxs->size() == info.pointCount : idxs->size() == info.vertexCount);
      }
      else
      {
         validSize = (NrefPerPoint ? vals->size() == info.pointCount : vals->size() == info.vertexCount);
      }
      
      if (validSize)
      {
         float *vl = 0;
         unsigned int *il = 0;
         
         if (NrefPerPoint)
         {
            vl = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
         }
         else
         {
            vl = (float*) AiMalloc(3 * vals->size() * sizeof(float));
         }
         
         if (!NrefPerPoint || !idxs)
         {
            // Can copy vector values straight from alembic property
            for (size_t i=0, j=0; i<vals->size(); ++i, j+=3)
            {
               Alembic::Abc::N3f val;
               
               Mref.multDirMatrix(vals->get()[i], val);
               val.normalize();
               
               vl[j+0] = val.x;
               vl[j+1] = val.y;
               vl[j+2] = val.z;
            }
         }
         
         if (idxs)
         {
            if (!NrefPerPoint)
            {
               il = (unsigned int*) AiMalloc(info.vertexCount * sizeof(unsigned int));
               
               for (unsigned int i=0; i<info.vertexCount; ++i)
               {
                  il[info.arnoldVertexIndex[i]] = idxs->get()[i];
               }
            }
            else
            {
               // no mapping
               for (unsigned int i=0, j=0; i<info.pointCount; ++i, j+=3)
               {
                  Alembic::Abc::N3f val;
                  
                  Mref.multDirMatrix(vals->get()[idxs->get()[i]], val);
                  val.normalize();
                  
                  vl[j+0] = val.x;
                  vl[j+1] = val.y;
                  vl[j+2] = val.z;
               }
            }
         }
         else
         {
            if (!NrefPerPoint)
            {
               // Build straight mapping 
               il = (unsigned int*) AiMalloc(info.vertexCount * sizeof(unsigned int));
               
               for (unsigned int i=0; i<info.vertexCount; ++i)
               {
                  il[info.arnoldVertexIndex[i]] = i;
               }
            }
         }
         
         if (mDso->verbose())
         {
            if (mDso->referenceScene())
            {
               AiMsgInfo("[abcproc] Use reference alembic world space normals as \"Nref\"");
            }
            else
            {
               AiMsgInfo("[abcproc] Use first sample world space normals as \"Nref\"");
            }
         }
         
         UserAttribute &ua = (NrefPerPoint ? info.pointAttrs["Nref"] : info.vertexAttrs["Nref"]);
   
         InitUserAttribute(ua);
         
         ua.arnoldCategory = (NrefPerPoint ? AI_USERDEF_VARYING : AI_USERDEF_INDEXED);
         ua.arnoldType = AI_TYPE_VECTOR;
         ua.arnoldTypeStr = "VECTOR";
         ua.isArray = true;
         ua.dataDim = 3;
         ua.dataCount = (NrefPerPoint ? info.pointCount : vals->size());
         ua.data = vl;
         ua.indicesCount = (NrefPerPoint ? 0 : info.vertexCount);
         ua.indices = (NrefPerPoint ? 0 : il);
         
         return true;
      }
      else
      {
         if (mDso->referenceScene())
         {
            AiMsgWarning("[abcproc] Cannot use reference alembic normals as \"Nref\"");
         }
         
         return false;
      }
   }
   else
   {
      if (mDso->referenceScene())
      {
         AiMsgWarning("[abcproc] Cannot use reference alembic normals as \"Nref\"");
      }
      
      return false;
   }
}

bool MakeShape::fillReferenceNormals(MeshInfo &info,
                                     UserAttributes *pointAttrs,
                                     UserAttributes *vertexAttrs)
{
   if (!pointAttrs || !vertexAttrs)
   {
      return false;
   }
   
   bool hasNref = checkReferenceNormals(info, pointAttrs, vertexAttrs);
   
   return fillReferenceNormals(info, !hasNref, pointAttrs, vertexAttrs);
}

bool MakeShape::fillReferenceNormals(MeshInfo &info,
                                     bool computeSmoothNormals,
                                     UserAttributes *pointAttrs,
                                     UserAttributes *vertexAttrs)
{
   if (!pointAttrs || !vertexAttrs)
   {
      return false;
   }
   
   if (computeSmoothNormals)
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Compute smooth \"Nref\" from \"Pref\"");
      }
      
      UserAttribute &ua = info.pointAttrs["Nref"];
      
      InitUserAttribute(ua);
               
      ua.arnoldCategory = AI_USERDEF_VARYING;
      ua.arnoldType = AI_TYPE_VECTOR;
      ua.arnoldTypeStr = "VECTOR";
      ua.isArray = true;
      ua.dataDim = 3;
      ua.dataCount = info.pointCount;
      ua.data = computeMeshSmoothNormals(info, (const float*) info.pointAttrs["Pref"].data, 0, 0.0f);
      
      return true;
   }
   else
   {
      bool NrefPerPoint = (vertexAttrs->find("Nref") == vertexAttrs->end());
      
      if (NrefPerPoint)
      {
         if (pointAttrs != &(info.pointAttrs))
         {
            UserAttribute &src = (*pointAttrs)["Nref"];
            UserAttribute &dst = info.pointAttrs["Nref"];
            
            dst = src;
            
            pointAttrs->erase(pointAttrs->find("Nref"));
         }
      }
      else
      {
         if (vertexAttrs != &(info.vertexAttrs))
         {
            UserAttribute &src = (*vertexAttrs)["Nref"];
            UserAttribute &dst = info.vertexAttrs["Nref"];
            
            dst = src;
            
            vertexAttrs->erase(vertexAttrs->find("Nref"));
         }
      }
      
      return true;
   }
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
   
   // Figure out if normals need to be read or computed 
   Alembic::AbcGeom::IN3fGeomParam N;
   std::vector<float*> _smoothNormals;
   std::vector<float*> *smoothNormals = 0;
   bool subd = false;
   bool smoothing = false;
   bool readNormals = false;
   bool NperPoint = false;
   
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
   
   readNormals = (!subd && smoothing);
   
   if (readNormals)
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Read normals data if available");
      }
      
      N = schema.getNormalsParam();
      
      if (N.valid() && (N.getScope() == Alembic::AbcGeom::kFacevaryingScope ||
                        N.getScope() == Alembic::AbcGeom::kVertexScope ||
                        N.getScope() == Alembic::AbcGeom::kVaryingScope))
      {
         NperPoint = (N.getScope() != Alembic::AbcGeom::kFacevaryingScope);
      }
      else
      {
         smoothNormals = &_smoothNormals;
         NperPoint = true;
      }
   }
   
   mNode = generateBaseMesh(node, info, smoothNormals);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output normals
   
   AtArray *nlist = 0;
   AtArray *nidxs = 0;
   
   if (readNormals)
   {
      if (smoothNormals)
      {
         // output smooth normals (per-point)
         
         // build vertex -> point mappings
         nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
         for (unsigned int k=0; k<info.vertexCount; ++k)
         {
            AiArraySetUInt(nidxs, k, info.vertexPointIndex[k]);
         }
         
         nlist = AiArrayAllocate(info.pointCount, smoothNormals->size(), AI_TYPE_VECTOR);
         
         unsigned int j = 0;
         AtVector n;
         
         for (std::vector<float*>::iterator nit=smoothNormals->begin(); nit!=smoothNormals->end(); ++nit)
         {
            float *N = *nit;
            
            for (unsigned int k=0; k<info.pointCount; ++k, ++j, N+=3)
            {
               n.x = N[0];
               n.y = N[1];
               n.z = N[2];
               
               AiArraySetVec(nlist, j, n);
            }
            
            AiFree(*nit);
         }
         
         smoothNormals->clear();
         
         AiNodeSetArray(mNode, "nlist", nlist);
         AiNodeSetArray(mNode, "nidxs", nidxs);
      }
      else
      {
         TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
         
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
      }
   }
   
   // Output UVs
   
   outputMeshUVs(node, mNode, info);
   
   // Output reference (Pref)
   
   if (mDso->outputReference())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate reference attributes");
      }
      
      UserAttributes refPointAttrs;
      UserAttributes refVertexAttrs;
      UserAttributes *pointAttrs = &refPointAttrs;
      UserAttributes *vertexAttrs = &refVertexAttrs;
      AlembicMesh *refMesh = 0;
      
      bool hasPref = getReferenceMesh(node, info, refMesh, pointAttrs, vertexAttrs);
      
      if (refMesh)
      {
         // Generate Pref
         
         Alembic::Abc::M44d Mref;  // World matrix to apply to points from reference (applies to both P and N)
         
         bool hadPref = hasPref;
         
         hasPref = fillReferencePositions(refMesh, info, pointAttrs, Mref);
         
         // Generate Nref
         
         if (hasPref && readNormals)
         {
            bool hasNref = checkReferenceNormals(info, pointAttrs, vertexAttrs);
            
            if (!hasNref)
            {
               if (hadPref)
               {
                  // Pref coming from user defined attribute, use it to compute Nref
                  // rather than reading a standard normal sample
               }
               else
               {
                  hasNref = readReferenceNormals(refMesh, info, Mref);
                  
                  if (hasNref)
                  {
                     // Nref is set in info's attribute stores
                     vertexAttrs = &(info.vertexAttrs);
                     pointAttrs = &(info.pointAttrs);
                  }
               }
            }
            
            fillReferenceNormals(info, !hasNref, pointAttrs, vertexAttrs);
         }
         
         // Tref?
         // Bref?
      }
      
      // Cleanup read reference attributes 
      DestroyUserAttributes(refPointAttrs);
      DestroyUserAttributes(refVertexAttrs);
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
   
   // Check if normals should be computed
   std::vector<float*> _smoothNormals;
   std::vector<float*> *smoothNormals = 0;
   bool subd = false;
   bool smoothing = false;
   bool readNormals = false;
   
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
   
   readNormals = (!subd && smoothing);
   
   if (readNormals)
   {
      smoothNormals = &_smoothNormals;
   }
   
   mNode = generateBaseMesh(node, info, smoothNormals);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output normals
   
   if (readNormals)
   {
      AtArray *nlist = 0;
      AtArray *nidxs = 0;
      
      // output smooth normals (per-point)
      
      // build vertex -> point mappings
      nidxs = AiArrayAllocate(info.vertexCount, 1, AI_TYPE_UINT);
      for (unsigned int k=0; k<info.vertexCount; ++k)
      {
         AiArraySetUInt(nidxs, k, info.vertexPointIndex[k]);
      }
      
      nlist = AiArrayAllocate(info.pointCount, smoothNormals->size(), AI_TYPE_VECTOR);
      
      unsigned int j = 0;
      AtVector n;
      
      for (std::vector<float*>::iterator nit=smoothNormals->begin(); nit!=smoothNormals->end(); ++nit)
      {
         float *N = *nit;
         
         for (unsigned int k=0; k<info.pointCount; ++k, ++j, N+=3)
         {
            n.x = N[0];
            n.y = N[1];
            n.z = N[2];
            
            AiArraySetVec(nlist, j, n);
         }
         
         AiFree(*nit);
      }
      
      smoothNormals->clear();
      
      AiNodeSetArray(mNode, "nlist", nlist);
      AiNodeSetArray(mNode, "nidxs", nidxs);
   }
   
   // Output UVs
   
   outputMeshUVs(node, mNode, info);
   
   // Output referece
   
   if (mDso->outputReference())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate reference attributes");
      }
      
      UserAttributes refPointAttrs;
      UserAttributes refVertexAttrs;
      UserAttributes *pointAttrs = &refPointAttrs;
      UserAttributes *vertexAttrs = &refVertexAttrs;
      AlembicSubD *refMesh = 0;
      
      bool hasPref = getReferenceMesh(node, info, refMesh, pointAttrs, vertexAttrs);
      
      if (refMesh)
      {
         // Generate Pref
         
         Alembic::Abc::M44d Mref;  // World matrix to apply to points from reference (applies to both P and N)
         
         hasPref = fillReferencePositions(refMesh, info, pointAttrs, Mref);
         
         // Generate Nref
         
         if (hasPref && readNormals)
         {
            fillReferenceNormals(info, pointAttrs, vertexAttrs);
         }
         
         // Tref?
         // Bref?
      }
      
      // Cleanup read reference attributes 
      DestroyUserAttributes(refPointAttrs);
      DestroyUserAttributes(refVertexAttrs);
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
