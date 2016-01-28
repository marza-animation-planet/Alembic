#include "visitors.h"
#include "userattr.h"
#include "nurbs.h"

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

AlembicNode::VisitReturn GetTimeRange::enter(AlembicCurves &node, AlembicNode *)
{
   updateTimeRange(node);
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

AlembicNode::VisitReturn CountShapes::enter(AlembicCurves &node, AlembicNode *)
{
   return shapeEnter(node);
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
   
   node.setVisible(visible);
   
   if (!visible)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (!node.isLocator() && !mDso->ignoreTransforms())
   {
      TimeSampleList<Alembic::AbcGeom::IXformSchema> &xformSamples = node.samples().schemaSamples;
      
      // Beware of the inheritsXform when computing world
      mMatrixSamplesStack.push_back(std::deque<Alembic::Abc::M44d>());
      
      std::deque<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
      
      std::deque<Alembic::Abc::M44d> *parentMatrices = 0;
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
      Alembic::AbcGeom::IPointsSchema &schema = node.typedObject().getSchema();
      
      Alembic::AbcGeom::IFloatGeomParam widths = schema.getWidthsParam();
      
      UserAttribute attr;
      
      InitUserAttribute(attr);
      
      if (ReadSingleUserAttribute("radius", PointAttribute, mDso->renderTime(), schema.getUserProperties(), schema.getArbGeomParams(), attr))
      {
         if (attr.arnoldType != AI_TYPE_FLOAT || attr.dataDim != 1)
         {
            DestroyUserAttribute(attr);
         }
         else if (mDso->isPromotedToConstantAttrib("radius"))
         {
            attr.dataCount = (attr.dataCount > 0 ? 1 : 0);
         }
      }
      
      if (attr.data == 0 && ReadSingleUserAttribute("size", PointAttribute, mDso->renderTime(), schema.getUserProperties(), schema.getArbGeomParams(), attr))
      {
         if (attr.arnoldType != AI_TYPE_FLOAT || attr.dataDim != 1)
         {
            DestroyUserAttribute(attr);
         }
         else if (mDso->isPromotedToConstantAttrib("size"))
         {
            attr.dataCount = (attr.dataCount > 0 ? 1 : 0);
         }
      }
      
      if (attr.data != 0)
      {
         const float *ptr = (const float*) attr.data;
         
         for (unsigned int i=0; i<attr.dataCount; ++i)
         {
            float r = adjustRadius(ptr[i]);
            
            if (r > extraPadding)
            {
               extraPadding = r;
            }
         }
         
         DestroyUserAttribute(attr);
      }
      else if (widths.valid())
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
               float r = adjustRadius(vals->get()[i]);
               
               if (r > extraPadding)
               {
                  extraPadding = r;
               }
            }
         }
      }
      else
      {
         if (ReadSingleUserAttribute("radius", ObjectAttribute, mDso->renderTime(), schema.getUserProperties(), schema.getArbGeomParams(), attr))
         {
            if (attr.arnoldType != AI_TYPE_FLOAT || attr.dataDim != 1)
            {
               DestroyUserAttribute(attr);
            }
         }
         else if (mDso->isPromotedToConstantAttrib("radius"))
         {
            if (ReadSingleUserAttribute("radius", PrimitiveAttribute, mDso->renderTime(), schema.getUserProperties(), schema.getArbGeomParams(), attr))
            {
               if (attr.arnoldType != AI_TYPE_FLOAT || attr.dataDim != 1)
               {
                  DestroyUserAttribute(attr);
               }
            }
            else
            {
               attr.dataCount = (attr.dataCount > 0 ? 1 : 0);
            }
         }
         
         if (attr.data == 0 && ReadSingleUserAttribute("size", ObjectAttribute, mDso->renderTime(), schema.getUserProperties(), schema.getArbGeomParams(), attr))
         {
            if (attr.arnoldType != AI_TYPE_FLOAT || attr.dataDim != 1)
            {
               DestroyUserAttribute(attr);
            }
         }
         else if (mDso->isPromotedToConstantAttrib("size"))
         {
            if (ReadSingleUserAttribute("size", PrimitiveAttribute, mDso->renderTime(), schema.getUserProperties(), schema.getArbGeomParams(), attr))
            {
               if (attr.arnoldType != AI_TYPE_FLOAT || attr.dataDim != 1)
               {
                  DestroyUserAttribute(attr);
               }
            }
            else
            {
               attr.dataCount = (attr.dataCount > 0 ? 1 : 0);
            }
         }
         
         if (attr.data != 0)
         {
            extraPadding = *((const float*) attr.data);
            DestroyUserAttribute(attr);
         }
         else
         {
            extraPadding = mDso->radiusMin();
         }
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Points extra bounds padding: %lf", extraPadding);
      }
   }
   
   return shapeEnter(node, instance, false, extraPadding);
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicCurves &node, AlembicNode *instance)
{
   // Take into account the curve width in the computed bounds
   
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
               float r = adjustRadius(0.5 * vals->get()[i]);
               
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
         AiMsgInfo("[abcproc] Curves extra bounds padding: %lf", extraPadding);
      }
   }
   
   return shapeEnter(node, instance, false, extraPadding);
}

AlembicNode::VisitReturn MakeProcedurals::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   // Not supported yet
   node.setVisible(false);
   
   return AlembicNode::DontVisitChildren;
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
      if (mMatrixSamplesStack.size() > 0)
      {
         mMatrixSamplesStack.pop_back();
      }
      else
      {
         AiMsgWarning("[abcproc] Trying to pop empty matrix stack!");
      }
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

AtNode* MakeShape::createArnoldNode(const char *nodeType, AlembicNode &node, bool useProcName) const
{
   GlobalLock::Acquire();

   AtNode *anode = AiNode(nodeType);

   std::string name;

   if (useProcName)
   {
      name = AiNodeGetName(mDso->procNode());

      // rename source procedural node
      AiNodeSetStr(mDso->procNode(), "name", mDso->uniqueName("_"+name).c_str());

      // use procedural name for newly generated node
      AiNodeSetStr(anode, "name", name.c_str());
   }
   else
   {
      name = node.formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
      AiNodeSetStr(anode, "name", mDso->uniqueName(name).c_str());
   }

   GlobalLock::Release();

   return anode;
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
      specialPointNames.insert("vel");
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
            
            bool promoteToConstant = mDso->isPromotedToConstantAttrib(ua.first);
            
            switch (scope)
            {
            case Alembic::AbcGeom::kFacevaryingScope:
               if (promoteToConstant)
               {
                  if (objectAttrs && mDso->readObjectAttribs())
                  {
                     attrs = objectAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" vertex attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               else
               {
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
               }
               break;
            case Alembic::AbcGeom::kVaryingScope:
            case Alembic::AbcGeom::kVertexScope:
               if (promoteToConstant)
               {
                  if (objectAttrs && mDso->readObjectAttribs())
                  {
                     attrs = objectAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" point attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               else
               {
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
               }
               break;
            case Alembic::AbcGeom::kUniformScope:
               if (promoteToConstant)
               {
                  if (objectAttrs && mDso->readObjectAttribs())
                  {
                     attrs = objectAttrs;
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read \"%s\" primitive attribute as \"%s\"",
                                  header.getName().c_str(), ua.first.c_str());
                     }
                  }
               }
               else
               {
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
                     if (!promoteToConstant)
                     {
                        attrs->insert(ua);
                     }
                     else
                     {
                        std::pair<std::string, UserAttribute> pua;
                        
                        bool promoted =  PromoteToConstantAttrib(ua.second, pua.second);
                        
                        DestroyUserAttribute(ua.second);
                        
                        if (!promoted)
                        {
                           if (mDso->verbose())
                           {
                              AiMsgWarning("[abcproc] Failed (could not promote to constant)");
                           }
                        }
                        else
                        {
                           pua.first = ua.first;
                           
                           if (mDso->verbose())
                           {
                              AiMsgInfo("[abcproc] \"%s\" promoted to constant attribute", pua.first.c_str());
                           }
                           
                           attrs->insert(pua);
                        }
                     }
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
   
   mNode = generateBaseMesh(node, instance, info, smoothNormals);
   
   if (!mNode)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Output normals
   
   AtArray *nlist = 0;
   AtArray *nidxs = 0;
   
   if (readNormals)
   {
      // Arnold requires as many normal samples as position samples
      if (smoothNormals)
      {
         // Output smooth normals (per-point)
         // As normals are computed from positions, sample count will match
         
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
            
            AtArray *vlist = AiNodeGetArray(mNode, "vlist");
            size_t requiredSamples = size_t(vlist->nkeys);
            
            // requiredSamples is either 1 or numMotionSamples()
            bool unexpectedSampleCount = (requiredSamples != 1 && requiredSamples != mDso->numMotionSamples());
            if (unexpectedSampleCount)
            {
               AiMsgWarning("[abcproc] Unexpected mesh sample count %lu.", requiredSamples);
            }
            
            if (info.varyingTopology || unexpectedSampleCount)
            {
               Nsamples.getSamples(mDso->renderTime(), n0, n1, blend);
               
               Alembic::Abc::N3fArraySamplePtr vals = n0->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs = n0->data().getIndices();
               
               nlist = AiArrayAllocate(vals->size(), requiredSamples, AI_TYPE_VECTOR);
               
               size_t ncount = vals->size();
               
               for (size_t i=0; i<ncount; ++i)
               {
                  Alembic::Abc::N3f val = vals->get()[i];
                  
                  vec.x = val.x;
                  vec.y = val.y;
                  vec.z = val.z;
                  
                  for (size_t j=0, k=0; j<requiredSamples; ++j, k+=ncount)
                  {
                     AiArraySetVec(nlist, i+k, vec);
                  }
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
               for (size_t i=0, j=0; i<requiredSamples; ++i)
               {
                  double t = (requiredSamples == 1 ? mDso->renderTime() : mDso->motionSampleTime(i));
                  
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read normal samples [t=%lf]", t);
                  }
                  
                  Nsamples.getSamples(t, n0, n1, blend);
                  
                  if (!nlist)
                  {
                     nlist = AiArrayAllocate(info.vertexCount, requiredSamples, AI_TYPE_VECTOR);
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
   
   mNode = generateBaseMesh(node, instance, info, smoothNormals);
   
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
   
   mNode = createArnoldNode("points", (instance ? *instance : node), true);
   
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
   
   UserAttributes::iterator it = info.pointAttrs.find("velocity");
   if (it == info.pointAttrs.end() ||
       it->second.arnoldType != AI_TYPE_VECTOR ||
       it->second.dataCount != info.pointCount)
   {
      it = info.pointAttrs.find("vel");
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
   }
   
   if (it != info.pointAttrs.end())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Using user defined attribute \"%s\" for point velocities", it->first.c_str());
      }
      vname = it->first;
      vel0 = (const float*) it->second.data;
   }
   else if (V0)
   {
      if (V0->size() != P0->size())
      {
         AiMsgWarning("[abcproc] (1) Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V0->size(), P0->size());
      }
      else
      {
         vel0 = (const float*) V0->getData();
      }
   }
   
   if (vel0)
   {
      it = info.pointAttrs.find("acceleration");
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
      
      if (vname.length() > 0)
      {
         it = extraPointAttrs.find(vname);
         if (it != extraPointAttrs.end() && it->second.dataCount == P1->size())
         {
            vel1 = (const float*) it->second.data;
         }
      }
      else if (V1)
      {
         if (V1->size() != P1->size())
         {
            AiMsgWarning("[abcproc] (2) Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V1->size(), P1->size());
         }
         else
         {
            vel1 = (const float*) V1->getData();
         }
      }
      
      if (aname.length() > 0)
      {
         it = extraPointAttrs.find(aname);
         if (it != extraPointAttrs.end() && it->second.dataCount == P1->size())
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
   bool radiusSet = false;
   
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
      else if (info.pointAttrs.find("size") != info.pointAttrs.end())
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Ignore alembic \"widths\" property: \"size\" attribute set");
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
         
         it = info.objectAttrs.find("size");
         
         if (it != info.objectAttrs.end())
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Ignore \"size\" object attribute");
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
                        
                        radius[i] = adjustRadius(r);
                     }
                     
                     std::map<Alembic::Util::uint64_t, std::pair<size_t, size_t> >::iterator idit1;
                     
                     for (idit1 = idmap1.begin(); idit1 != idmap1.end(); ++idit1)
                     {
                        r = R1->get()[idit1->second.first];
                        
                        radius[idit1->second.second] = adjustRadius(r);
                     }
                  }
                  else
                  {
                     // asset(R0->size() == info.pointCount);
                     
                     for (size_t i=0; i<R0->size(); ++i)
                     {
                        r = R0->get()[i];
                        
                        radius[i] = adjustRadius(r);
                     }
                  }
                  
                  AiNodeSetArray(mNode, "radius", AiArrayConvert(info.pointCount, 1, AI_TYPE_FLOAT, radius));
                  
                  AiFree(radius);
                  
                  radiusSet = true;
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
   if (!radiusSet)
   {
      UserAttribute *ra = 0;
      UserAttributes *attrs = 0;
      
      UserAttributes::iterator ait = info.pointAttrs.find("radius");
      
      if (ait == info.pointAttrs.end())
      {
         ait = info.pointAttrs.find("size");
      }
      
      if (ait != info.pointAttrs.end())
      {
         ra = &(ait->second);
         attrs = &(info.pointAttrs);
      }
      else
      {
         ait = info.objectAttrs.find("radius");
         
         if (ait == info.objectAttrs.end())
         {
            ait = info.objectAttrs.find("size");
         }
         
         if (ait != info.objectAttrs.end())
         {
            ra = &(ait->second);
            attrs = &(info.objectAttrs);
         }
      }
      
      if (ra && ra->data && ra->arnoldType == AI_TYPE_FLOAT)
      {
         float *r = (float*) ra->data;
            
         for (unsigned int i=0; i<ra->dataCount; ++i)
         {
            for (unsigned int j=0; j<ra->dataDim; ++j, ++r)
            {
               *r = adjustRadius(*r);
            }
         }
         
         AtArray *radius = AiArrayConvert(ra->dataCount, 1, AI_TYPE_FLOAT, ra->data);
         
         AiNodeSetArray(mNode, "radius", radius);
         
         DestroyUserAttribute(ait->second);
         attrs->erase(ait);
      }
      else
      {
         AiMsgInfo("[abcproc] No radius set for points in alembic archive. Create particles with constant radius %lf (can be changed using dso '-radiusmin' data flag or 'abc_radiusmin' user attribute)", mDso->radiusMin());
         
         AtArray *radius = AiArrayAllocate(1, 1, AI_TYPE_FLOAT);
         AiArraySetFlt(radius, 0, mDso->radiusMin());
         
         AiNodeSetArray(mNode, "radius", radius);
      }
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

bool MakeShape::getReferenceCurves(AlembicCurves &node,
                                   CurvesInfo &info,
                                   AlembicCurves* &refCurves,
                                   UserAttributes* &pointAttrs)
{
   if (!pointAttrs)
   {
      // pointAttrs must be set
      refCurves = 0;
      return false;
   }
   
   if (pointAttrs == &(info.pointAttrs))
   {
      // pointAttrs should point to attribute storage other than current scene's one
      refCurves = 0;
      return false;
   }
   
   // Priority
   //   1/ "Pref" attribute in current alembic scene
   //   2/ "Pref" attribute in reference alembic scene
   //   3/ Positions from reference alembic scene
   //   4/ Positions from current alembic scene's first frame
   
   UserAttributes::iterator uait;
   bool hasPref = false;
   
   uait = info.pointAttrs.find("Pref");
   
   if (uait != info.pointAttrs.end())
   {
      if (isVaryingFloat3(info, uait->second))
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Using 'Pref' attribute found in alembic file");
         }
         
         hasPref = true;
      }
      else
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Ignore 'Pref' attribute found in alembic file");
         }
         
         // Pref exists but doesn't match requirements, get rid of it
         DestroyUserAttribute(uait->second);
         info.pointAttrs.erase(uait);
      }
   }
   
   if (hasPref)
   {
      refCurves = &node;
      pointAttrs = &(info.pointAttrs);
   }
   else
   {
      AlembicScene *refScene = mDso->referenceScene();
      
      if (!refScene)
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Using current alembic scene first sample as reference.");
         }
         
         refCurves = &node;
         pointAttrs = &(info.pointAttrs);
      }
      else
      {
         AlembicNode *refNode = refScene->find(node.path());
         
         if (refNode)
         {
            refCurves = dynamic_cast<AlembicCurves*>(refNode);
            
            if (!refCurves)
            {
               AiMsgWarning("[abcproc] Node in reference alembic doesn't have the same type. Using current alembic scene first sample as reference");
               
               refCurves = &node;
               pointAttrs = &(info.pointAttrs);
            }
            else
            {
               Alembic::AbcGeom::ICurvesSchema schema = refCurves->typedObject().getSchema();
               
               double reftime = schema.getTimeSampling()->getSampleTime(0);
               
               collectUserAttributes(schema.getUserProperties(),
                                     schema.getArbGeomParams(),
                                     reftime, false,
                                     0, 0, pointAttrs, 0, 0,
                                     FilterReferenceAttributes);
               
               uait = pointAttrs->find("Pref");
               
               if (uait != pointAttrs->end())
               {
                  if (isVaryingFloat3(info, uait->second))
                  {
                     hasPref = true;
                  }
                  else
                  {
                     // Pref exists but doesn't match requirements, get rid of it
                     DestroyUserAttribute(uait->second);
                     pointAttrs->erase(uait);
                  }
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Couldn't find node in reference alembic scene. Using current alembic scene first sample as reference");
            
            refCurves = &node;
            pointAttrs = &(info.pointAttrs);
         }
      }
   }
   
   return hasPref;
}

bool MakeShape::initCurves(CurvesInfo &info,
                           const Alembic::AbcGeom::ICurvesSchema::Sample &sample,
                           std::string &arnoldBasis)
{
   if (sample.getType() == Alembic::AbcGeom::kVariableOrder)
   {
      AiMsgWarning("[abcproc] Variable order curves not supported");
      return false;
   }
   
   bool nurbs = false;
   int degree = (sample.getType() == Alembic::AbcGeom::kCubic ? 3 : 1);
   bool periodic = (sample.getWrap() == Alembic::AbcGeom::kPeriodic);
   
   if (info.degree != 1)
   {
      switch (sample.getBasis())
      {
      case Alembic::AbcGeom::kNoBasis:
         arnoldBasis = "linear";
         degree = 1;
         break;
      
      case Alembic::AbcGeom::kBezierBasis:
         arnoldBasis = "bezier";
         break;
      
      case Alembic::AbcGeom::kBsplineBasis:
         nurbs = true;
         arnoldBasis = "catmull-rom";
         break;
      
      case Alembic::AbcGeom::kCatmullromBasis:
         arnoldBasis = "catmull-rom";
         break;
      
      case Alembic::AbcGeom::kPowerBasis:
         AiMsgWarning("[abcproc] Curve 'Power' basis not supported");
         return false;
      
      case Alembic::AbcGeom::kHermiteBasis:
         AiMsgWarning("[abcproc] Curve 'Hermite' basis not supported");
         return false;
      
      default:
         AiMsgWarning("[abcproc] Unknown curve basis");
         return false;
      }
   }
   
   if (info.degree != -1)
   {
      if (degree != info.degree ||
          nurbs != info.nurbs ||
          periodic != info.periodic)
      {
         return false;
      }
   }
   
   info.degree = degree;
   info.periodic = periodic;
   info.nurbs = nurbs;
   
   return true;
}

bool MakeShape::fillCurvesPositions(CurvesInfo &info,
                                    size_t motionStep,
                                    size_t numMotionSteps,
                                    Alembic::Abc::Int32ArraySamplePtr Nv,
                                    Alembic::Abc::P3fArraySamplePtr P0,
                                    Alembic::Abc::FloatArraySamplePtr W0,
                                    Alembic::Abc::P3fArraySamplePtr P1,
                                    Alembic::Abc::FloatArraySamplePtr W1,
                                    const float *vel,
                                    const float *acc,
                                    float blend,
                                    Alembic::Abc::FloatArraySamplePtr K,
                                    AtArray* &num_points,
                                    AtArray* &points)
{
   unsigned int extraPoints = (info.degree == 3 ? 2 : 0);
   
   if (!num_points)
   {
      info.curveCount = (unsigned int) Nv->size();
      info.pointCount = 0;
      info.cvCount = 0;
      
      num_points = AiArrayAllocate(info.curveCount, 1, AI_TYPE_UINT);
      
      if (info.nurbs)
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            // Num Spans + Degree = Num CVs
            // -> Num Spans = Num CVs - degree
            unsigned int curvePointCount = 1 + (Nv->get()[ci] - info.degree) * mDso->nurbsSampleRate();
            
            AiArraySetUInt(num_points, ci, curvePointCount + extraPoints);
            
            info.pointCount += curvePointCount;
         }
         
         info.cvCount = (unsigned int) P0->size();
      }
      else
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            AiArraySetUInt(num_points, ci, (unsigned int) (Nv->get()[ci] + extraPoints));
         }
         
         info.pointCount = (unsigned int) P0->size();
      }
   }
   else if (num_points->nelements != info.curveCount)
   {
      AiMsgWarning("[abcproc] Bad curve num_points array size");
      return false;
   }
   
   if (info.pointCount == 0)
   {
      if (info.nurbs)
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            info.pointCount += (1 + (Nv->get()[ci] - info.degree) * mDso->nurbsSampleRate());
         }
      }
      else
      {
         info.pointCount = (unsigned int) P0->size();
      }
   }
   
   unsigned int allocPointCount = info.pointCount + (extraPoints * info.curveCount);
   
   if (!points)
   {
      points = AiArrayAllocate(allocPointCount, numMotionSteps, AI_TYPE_POINT);
   }
   else
   {
      if (points->nelements != allocPointCount)
      {
         AiMsgWarning("[abcproc] Bad curve points array size");
         return false;
      }
      else if (motionStep >= points->nkeys)
      {
         AiMsgWarning("[abcproc] Bad motion key for curve points");
         return false;
      }
   }
   
   size_t pi = motionStep * allocPointCount;
   unsigned int po = 0;
   Alembic::Abc::V3f p0, p1;
   AtPoint pnt;
   const float *cvel = vel;
   const float *cacc = acc;
   
   if (info.nurbs)
   {
      NURBS<3> curve;
      NURBS<3>::CV cv;
      NURBS<3>::Pnt pt;
      unsigned int ko = 0;
      float w0, w1;
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Allocate %u point(s) for %u curve(s), from %lu cv(s)", info.pointCount, info.curveCount, P0->size());
      }
      
      for (unsigned int ci=0; ci<info.curveCount; ++ci)
      {
         // Build NURBS curve
         
         long np = Nv->get()[ci];
         long ns = np - info.degree;
         long nk = ns + 2 * info.degree + 1;
         
         curve.setNumCVs(np);
         curve.setNumKnots(nk);
         
         if (blend > 0.0f && (P1 || vel))
         {
            if (P1)
            {
               float a = 1.0f - blend;
               
               for (long i=0; i<np; ++i)
               {
                  p0 = P0->get()[po + i];
                  p1 = P1->get()[po + i];
                  
                  w0 = (W0 ? W0->get()[po + i] : 1.0f);
                  w1 = (W1 ? W1->get()[po + i] : 1.0f);
                  
                  cv[0] = a * p0.x + blend * p1.x;
                  cv[1] = a * p0.y + blend * p1.y;
                  cv[2] = a * p0.z + blend * p1.z;
                  cv[3] = a * w0 + blend * w1;
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
                  // }
                  
                  curve.setCV(i, cv);
               }
            }
            else if (acc)
            {
               for (long i=0; i<np; ++i, cvel+=3, cacc+=3)
               {
                  p0 = P0->get()[po + i];
                  w0 = (W0 ? W0->get()[po + i] : 1.0f);
                  
                  cv[0] = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  cv[1] = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  cv[2] = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  cv[3] = w0;
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
                  // }
                  
                  curve.setCV(i, cv);
               }
            }
            else
            {
               for (long i=0; i<np; ++i, cvel+=3)
               {
                  p0 = P0->get()[po + i];
                  w0 = (W0 ? W0->get()[po + i] : 1.0f);
                  
                  cv[0] = p0.x + blend * cvel[0];
                  cv[1] = p0.y + blend * cvel[1];
                  cv[2] = p0.z + blend * cvel[2];
                  cv[3] = w0;
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
                  // }
                  
                  curve.setCV(i, cv);
               }
            }
         }
         else
         {
            for (long i=0; i<np; ++i)
            {
               p0 = P0->get()[po + i];
               w0 = (W0 ? W0->get()[po + i] : 1.0f);
               
               cv[0] = p0.x;
               cv[1] = p0.y;
               cv[2] = p0.z;
               cv[3] = w0;
               
               // if (mDso->verbose())
               // {
               //    AiMsgInfo("[abcproc] curve[%u].cv[%ld] = (%f, %f, %f, %f)", ci, i, cv[0], cv[1], cv[2], cv[3]);
               // }
               
               curve.setCV(i, cv);
            }
         }
         
         for (long i=0; i<nk; ++i)
         {
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] curve[%u].knot[%ld] = %f", ci, i, K->get()[ko + i]);
            // }
            
            curve.setKnot(i, K->get()[ko + i]);
         }
         
         po += np;
         ko += nk;
         
         // Evaluate NURBS
         
         float ustart = 0.0f;
         float uend = 0.0f;
         
         curve.getDomain(ustart, uend);
         
         float ustep = (uend - ustart) / float(ns * mDso->nurbsSampleRate());
         
         long numSamples = 1 + ns * mDso->nurbsSampleRate();
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] curve[%u] domain = [%f, %f], step = %f, points = %ld", ci, ustart, uend, ustep, numSamples);
         }
         
         if (extraPoints > 0)
         {
            curve.eval(ustart, pt);
            pnt.x = pt[0];
            pnt.y = pt[1];
            pnt.z = pt[2];
            AiArraySetPnt(points, pi++, pnt);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] first point (u=%f, idx=%lu): (%f, %f, %f)", ustart, pi-1, pnt.x, pnt.y, pnt.z);
            // }
         }
         
         float u = ustart;
         
         for (long i=0; i<numSamples; ++i)
         {
            curve.eval(u, pt);
            pnt.x = pt[0];
            pnt.y = pt[1];
            pnt.z = pt[2];
            AiArraySetPnt(points, pi++, pnt);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] point(u=%f, idx=%lu): (%f, %f, %f)", u, pi-1, pnt.x, pnt.y, pnt.z);
            // }
         
            u += ustep;
         }
         
         if (extraPoints > 0)
         {
            curve.eval(uend, pt);
            pnt.x = pt[0];
            pnt.y = pt[1];
            pnt.z = pt[2];
            AiArraySetPnt(points, pi++, pnt);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] last point(u=%f, idx=%lu): (%f, %f, %f)", uend, pi-1, pnt.x, pnt.y, pnt.z);
            // }
         }
      }
   }
   else
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] %u curve(s), %u point(s)", info.curveCount, info.pointCount);
      }
      
      if (blend > 0.0f && (P1 || vel))
      {
         if (P1)
         {
            float a = 1.0f - blend;
            
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               int np = Nv->get()[ci];
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po];
                  p1 = P1->get()[po]; 
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               for (int i=0; i<np; ++i)
               {
                  p0 = P0->get()[po + i];
                  p1 = P1->get()[po + i];
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po + np - 1];
                  p1 = P1->get()[po + np - 1];
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               po += np;
            }
         }
         else if (acc)
         {
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               int np = Nv->get()[ci];
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po];
                  pnt.x = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  pnt.y = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  pnt.z = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               for (int i=0; i<np; ++i, cvel+=3, cacc+=3)
               {
                  p0 = P0->get()[po + i];
                  pnt.x = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  pnt.y = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  pnt.z = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po + np - 1];
                  pnt.x = p0.x + blend * (cvel[0] + 0.5f * blend * cacc[0]);
                  pnt.y = p0.y + blend * (cvel[1] + 0.5f * blend * cacc[1]);
                  pnt.z = p0.z + blend * (cvel[2] + 0.5f * blend * cacc[2]);
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               po += np;
            }
         }
         else
         {
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               int np = Nv->get()[ci];
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po];
                  pnt.x = p0.x + blend * cvel[0];
                  pnt.y = p0.y + blend * cvel[1];
                  pnt.z = p0.z + blend * cvel[2];
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               for (int i=0; i<np; ++i, cvel+=3)
               {
                  p0 = P0->get()[po + i];
                  pnt.x = p0.x + blend * cvel[0];
                  pnt.y = p0.y + blend * cvel[1];
                  pnt.z = p0.z + blend * cvel[2];
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               if (extraPoints > 0)
               {
                  p0 = P0->get()[po + np - 1];
                  pnt.x = p0.x + blend * cvel[0];
                  pnt.y = p0.y + blend * cvel[1];
                  pnt.z = p0.z + blend * cvel[2];
                  AiArraySetPnt(points, pi++, pnt);
               }
               
               po += np;
            }
         }
      }
      else
      {
         for (unsigned int ci=0; ci<info.curveCount; ++ci)
         {
            int np = Nv->get()[ci];
            
            if (extraPoints > 0)
            {
               p0 = P0->get()[po];
               pnt.x = p0.x;
               pnt.y = p0.y;
               pnt.z = p0.z;
               AiArraySetPnt(points, pi++, pnt);
            }
            
            for (int i=0; i<np; ++i, cvel+=3, cacc+=3)
            {
               p0 = P0->get()[po + i];
               pnt.x = p0.x;
               pnt.y = p0.y;
               pnt.z = p0.z;
               AiArraySetPnt(points, pi++, pnt);
            }
            
            if (extraPoints > 0)
            {
               p0 = P0->get()[po + np - 1];
               pnt.x = p0.x;
               pnt.y = p0.y;
               pnt.z = p0.z;
               AiArraySetPnt(points, pi++, pnt);
            }
            
            po += np;
         }
      }
   }
   
   return true;
}

bool MakeShape::fillReferencePositions(AlembicCurves *refCurves,
                                       CurvesInfo &info,
                                       UserAttributes *pointAttrs)
{
   if (!refCurves || !pointAttrs)
   {
      return false;
   }
   
   Alembic::AbcGeom::ICurvesSchema schema = refCurves->typedObject().getSchema();
         
   Alembic::AbcGeom::ICurvesSchema::Sample sample = schema.getValue();
   
   Alembic::Abc::P3fArraySamplePtr Pref = sample.getPositions();
      
   bool hasPref = (pointAttrs->find("Pref") != pointAttrs->end());
   
   if (hasPref)
   {
      if (pointAttrs != &(info.pointAttrs))
      {
         bool destroyDst = (info.pointAttrs.find("Pref") != info.pointAttrs.end());
         
         // Transfer from reference shape attributes to current shape attribtues
         UserAttribute &src = (*pointAttrs)["Pref"];
         UserAttribute &dst = info.pointAttrs["Pref"];
         
         if (destroyDst)
         {
            DestroyUserAttribute(dst);
         }
         
         dst = src;
         
         pointAttrs->erase(pointAttrs->find("Pref"));
      }
      
      UserAttribute &ua = info.pointAttrs["Pref"];
      
      unsigned int count = (info.nurbs ? info.cvCount : info.pointCount);
      
      // Little type aliasing: float[1][3n] -> float[3][n]
      if (ua.dataDim == 1 && ua.dataCount == (3 * count))
      {
         AiMsgWarning("[abcproc] \"Pref\" exported with wrong base type: float instead if float[3]");
         
         ua.dataDim = 3;
         ua.dataCount = count;
         ua.arnoldType = AI_TYPE_POINT;
         ua.arnoldTypeStr = "POINT";
      }
      
      // Check valid point count
      if (ua.dataCount != count || ua.dataDim != 3)
      {
         AiMsgWarning("[abcproc] \"Pref\" exported with incompatible size and/or dimension");
         
         DestroyUserAttribute(ua);
         info.pointAttrs.erase(info.pointAttrs.find("Pref"));
         return false;
      }
      
      if (info.nurbs)
      {
         Alembic::Abc::Int32ArraySamplePtr Nv = sample.getCurvesNumVertices();
         
         if (Nv->size() != info.curveCount)
         {
            AiMsgWarning("[abcproc] Incompatible curve count in reference object");
            
            DestroyUserAttribute(ua);
            info.pointAttrs.erase(info.pointAttrs.find("Pref"));
            return false;
         }
         
         if (Pref->size() != count)
         {
            AiMsgWarning("[abcproc] Incompatible point count in reference object");
            
            DestroyUserAttribute(ua);
            info.pointAttrs.erase(info.pointAttrs.find("Pref"));
            return false;
         }
         
         Alembic::Abc::FloatArraySamplePtr W = sample.getPositionWeights();
         Alembic::Abc::FloatArraySamplePtr K;
         schema.getKnotsProperty().get(K);
         
         NURBS<3> curve;
         NURBS<3>::CV cv;
         NURBS<3>::Pnt pnt;
         Alembic::Abc::V3f p;
         const float *P = (const float*) ua.data;
         float *newP = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
         long ko = 0;
         long po = 0;
         size_t off = 0;
         
         for (size_t ci=0, pi=0; ci<Nv->size(); ++ci)
         {
            // Build NURBS curve
            
            long np = Nv->get()[ci];
            long ns = np - info.degree;
            long nk = ns + 2 * info.degree + 1;
            
            curve.setNumCVs(np);
            curve.setNumKnots(nk);
            
            for (int pi=0; pi<np; ++pi, P+=3)
            {
               cv[0] = P[0];
               cv[1] = P[1];
               cv[2] = P[2];
               cv[3] = (W ? W->get()[po + pi] : 1.0f);
               curve.setCV(pi, cv);
            }
            
            for (int ki=0; ki<nk; ++ki)
            {
               curve.setKnot(ki, K->get()[ko + ki]);
            }
            
            po += np;
            ko += nk;
            
            // Evaluate NURBS curve
            
            float ustart = 0.0f;
            float uend = 0.0f;
            
            curve.getDomain(ustart, uend);
            
            float ustep = (uend - ustart) / (ns * mDso->nurbsSampleRate());
            
            long n = 1 + (ns * mDso->nurbsSampleRate());
            
            float u = ustart;
            
            for (int i=0; i<n; ++i, off+=3)
            {
               curve.eval(u, pnt);
               
               // check off < 3 * info.pointCount?
               newP[off + 0] = pnt[0];
               newP[off + 1] = pnt[1];
               newP[off + 2] = pnt[2];
               
               u += ustep;
            }
         }
         
         AiFree(ua.data);
         ua.data = newP;
         ua.dataDim = 3;
         ua.dataCount = info.pointCount;
      }
      
      if (mDso->referenceScene())
      {
         AiMsgWarning("[abcproc] \"Pref\" read from user attribute, ignore values from reference alembic");
      }
      else if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] \"Pref\" read from user attribute");
      }
   }
   else
   {
      Alembic::Abc::Int32ArraySamplePtr Nv = sample.getCurvesNumVertices();
      
      if (Nv->size() == info.curveCount &&
          Pref->size() == (info.nurbs ? info.cvCount : info.pointCount))
      {
         // Figure out world matrix
         
         Alembic::Abc::M44d Mref;
         
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
                  Mref[r][c] = mtx[r][c];
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
         
            AlembicXform *refParent = dynamic_cast<AlembicXform*>(refCurves->parent());
         
            while (refParent)
            {
               Alembic::AbcGeom::IXformSchema xformSchema = refParent->typedObject().getSchema();
            
               Alembic::AbcGeom::XformSample xformSample = xformSchema.getValue();
            
               Mref = Mref * xformSample.getMatrix();
            
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
         
         // Compute reference positions
         
         float *vals = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
         
         if (info.nurbs)
         {
            Alembic::Abc::FloatArraySamplePtr W = sample.getPositionWeights();
            Alembic::Abc::FloatArraySamplePtr K;
            
            schema.getKnotsProperty().get(K);
            
            NURBS<3> curve;
            NURBS<3>::CV cv;
            NURBS<3>::Pnt pnt;
            Alembic::Abc::V3f p;
            long ko = 0;
            long po = 0;
            
            for (size_t ci=0, off=0; ci<Nv->size(); ++ci)
            {
               // Build NURBS curve
               
               long np = Nv->get()[ci];
               long ns = np - info.degree;
               long nk = ns + 2 * info.degree + 1;
               
               curve.setNumCVs(np);
               curve.setNumKnots(nk);
               
               for (int pi=0; pi<np; ++pi)
               {
                  p = Pref->get()[po + pi] * Mref;
                  cv[0] = p.x;
                  cv[1] = p.y;
                  cv[2] = p.z;
                  cv[3] = (W ? W->get()[po + pi] : 1.0f);
                  curve.setCV(pi, cv);
               }
               
               for (int ki=0; ki<nk; ++ki)
               {
                  curve.setKnot(ki, K->get()[ko + ki]);
               }
               
               po += np;
               ko += nk;
               
               // Evaluate curve
               
               float ustart = 0.0f;
               float uend = 0.0f;
               
               curve.getDomain(ustart, uend);
               
               float ustep = (uend - ustart) / (ns * mDso->nurbsSampleRate());
               
               long n = 1 + (ns * mDso->nurbsSampleRate());
               
               float u = ustart;
               
               for (int i=0; i<n; ++i, off+=3)
               {
                  curve.eval(u, pnt);
                  
                  vals[off + 0] = pnt[0];
                  vals[off + 1] = pnt[1];
                  vals[off + 2] = pnt[2];
                  
                  u += ustep;
               }
            }
         }
         else
         {
            for (unsigned int i=0, off=0; i<info.pointCount; ++i, off+=3)
            {
               Alembic::Abc::V3f p = Pref->get()[i] * Mref;
               
               vals[off + 0] = p.x;
               vals[off + 1] = p.y;
               vals[off + 2] = p.z;
            }
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
         
         hasPref = true;
      }
      else
      {
         AiMsgWarning("[abcproc] Could not generate \"Pref\" for curves \"%s\"", refCurves->path().c_str());
      }
   }
   
   return hasPref;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicCurves &node, AlembicNode *instance)
{
   Alembic::AbcGeom::ICurvesSchema schema = node.typedObject().getSchema();
   
   if (mDso->isVolume())
   {
      mNode = generateVolumeBox(schema);
      outputInstanceNumber(node, instance);
      return AlembicNode::ContinueVisit;
   }
   
   CurvesInfo info;
   
   info.varyingTopology = (schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   TimeSampleList<Alembic::AbcGeom::ICurvesSchema> &samples = node.samples().schemaSamples;
   TimeSampleList<Alembic::AbcGeom::IFloatGeomParam> Wsamples;
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
   TimeSampleList<Alembic::AbcGeom::ICurvesSchema>::ConstIterator samp0, samp1;
   TimeSampleList<Alembic::AbcGeom::IFloatGeomParam>::ConstIterator Wsamp0, Wsamp1;
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam>::ConstIterator Nsamp0, Nsamp1;
   double a = 1.0;
   double b = 0.0;
   double t = 0.0;
   
   Alembic::AbcGeom::IFloatGeomParam widths = schema.getWidthsParam();
   Alembic::AbcGeom::IN3fGeomParam normals = schema.getNormalsParam();
   Alembic::AbcGeom::IV2fGeomParam uvs = schema.getUVsParam();
   
   if (info.varyingTopology)
   {
      t = mDso->renderTime();
      
      node.sampleSchema(t, t, false);
      
      if (widths)
      {
         Wsamples.update(widths, t, t, false);
      }
      
      if (normals)
      {
         Nsamples.update(normals, t, t, false);
      }
   }
   else
   {
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
         t = mDso->motionSampleTime(i);
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample curves \"%s\" at t=%lf", node.path().c_str(), t);
         }
      
         node.sampleSchema(t, t, i > 0);
         
         if (widths)
         {
            Wsamples.update(widths, t, t, i > 0);
         }
         
         if (normals)
         {
            Nsamples.update(normals, t, t, i > 0);
         }
      }
      
      // renderTime() may not be in the sample list, let's at it just in case
      node.sampleSchema(mDso->renderTime(), mDso->renderTime(), true);
   }
   
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Read %lu curves samples", samples.size());
   }
   
   if (samples.size() == 0)
   {
      return AlembicNode::DontVisitChildren;
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
                         0,
                         0);
   
   // Process point positions
   
   AtArray *num_points = 0;
   AtArray *points = 0;
   std::string basis = "linear";
   Alembic::Abc::Int32ArraySamplePtr Nv;
   Alembic::Abc::P3fArraySamplePtr P0;
   Alembic::Abc::P3fArraySamplePtr P1;
   Alembic::Abc::FloatArraySamplePtr W0;
   Alembic::Abc::FloatArraySamplePtr W1;
   Alembic::Abc::FloatArraySamplePtr K;
   bool success = false;
   
   if (samples.size() == 1 && !info.varyingTopology)
   {
      samp0 = samples.begin();
      
      Nv = samp0->data().getCurvesNumVertices();
      P0 = samp0->data().getPositions();
      
      if (initCurves(info, samp0->data(), basis))
      {
         if (info.nurbs)
         {
            W0 = samp0->data().getPositionWeights();
            
            // getKnots() is not 'const'! WTF!
            //K = samp0->data().getKnots();
            
            Alembic::Abc::ISampleSelector iss(samp0->time(), Alembic::Abc::ISampleSelector::kNearIndex);
            schema.getKnotsProperty().get(K, iss);
         }
         
         success = fillCurvesPositions(info, 0, 1, Nv, P0, W0, P1, W1, 0, 0, 0.0f, K, num_points, points);
      }
   }
   else
   {
      if (info.varyingTopology)
      {
         samples.getSamples(mDso->renderTime(), samp0, samp1, b);
         
         Nv = samp0->data().getCurvesNumVertices();
         P0 = samp0->data().getPositions();
         
         if (initCurves(info, samp0->data(), basis))
         {
            if (info.nurbs)
            {
               W0 = samp0->data().getPositionWeights();
               
               Alembic::Abc::ISampleSelector iss(samp0->time(), Alembic::Abc::ISampleSelector::kNearIndex);
               schema.getKnotsProperty().get(K, iss);
            }
            
            // Get velocity
            
            const float *vel = 0;
            
            UserAttributes::iterator it = info.pointAttrs.find("velocity");
            
            if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
            {
               it = info.pointAttrs.find("v");
               if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.end();
               }
            }
            
            if (it != info.pointAttrs.end())
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Using user defined attribute \"%s\" for point velocities", it->first.c_str());
               }
               vel = (const float*) it->second.data;
            }
            else if (samp0->data().getVelocities())
            {
               Alembic::Abc::V3fArraySamplePtr V0 = samp0->data().getVelocities();
               if (V0->size() != P0->size())
               {
                  AiMsgWarning("[abcproc] Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V0->size(), P0->size());
               }
               else
               {
                  vel = (const float*) V0->getData();
               }
            }
            
            // Get acceleration
            
            const float *acc = 0;
            
            if (vel)
            {
               UserAttributes::iterator it = info.pointAttrs.find("acceleration");
               
               if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.find("accel");
                  if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.find("a");
                     if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                     {
                        it = info.pointAttrs.end();
                     }
                  }
               }
               
               if (it != info.pointAttrs.end())
               {
                  acc = (const float*) it->second.data;
               }
            }
            
            // Compute positions
            
            if (vel)
            {
               success = true;
               
               for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
               {
                  t = mDso->motionSampleTime(i) - mDso->renderTime();
                  
                  if (!fillCurvesPositions(info, i, mDso->numMotionSamples(), Nv, P0, W0, P1, W1, vel, acc, t, K, num_points, points))
                  {
                     success = false;
                     break;
                  }
               }
            }
            else
            {
               // only one sample
               success = fillCurvesPositions(info, 0, 1, Nv, P0, W0, P1, W1, 0, 0, 0.0f, K, num_points, points);
            }
         }
      }
      else
      {
         success = true;
         
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            b = 0.0;
            t = mDso->motionSampleTime(i);
            
            samples.getSamples(t, samp0, samp1, b);
            
            if (!initCurves(info, samp0->data(), basis))
            {
               success = false;
               break;
            }
            
            Nv = samp0->data().getCurvesNumVertices();
            P0 = samp0->data().getPositions();
            
            if (info.nurbs)
            {
               W0 = samp0->data().getPositionWeights();
               
               Alembic::Abc::ISampleSelector iss(samp0->time(), Alembic::Abc::ISampleSelector::kNearIndex);
               schema.getKnotsProperty().get(K, iss);
            }
            
            if (b > 0.0)
            {
               if (!initCurves(info, samp1->data(), basis))
               {
                  success = false;
                  break;
               }
               
               P1 = samp1->data().getPositions();
               
               if (info.nurbs)
               {
                  W1 = samp0->data().getPositionWeights();
               }
            }
            else
            {
               P1.reset();
               W1.reset();
            }
            
            if (!fillCurvesPositions(info, i, mDso->numMotionSamples(), Nv, P0, W0, P1, W1, 0, 0, b, K, num_points, points))
            {
               success = false;
               break;
            }
         }
      }
   }
   
   if (!success)
   {
      if (num_points)
      {
         AiArrayDestroy(num_points);
      }
      
      if (points)
      {
         AiArrayDestroy(points);
      }
      
      return AlembicNode::DontVisitChildren;
   }
   
   mNode = createArnoldNode("curves", (instance ? *instance : node), true);
   AiNodeSetStr(mNode, "basis", basis.c_str());
   AiNodeSetArray(mNode, "num_points", num_points);
   AiNodeSetArray(mNode, "points", points);
   AiNodeSetStr(mNode, "mode", "ribbon");
   // mode
   // min_pixel_width
   // invert_normals
   
   // Process radius
   AtArray *radius = 0;
   
   if (Wsamples.size() > 0)
   {
      Alembic::Abc::FloatArraySamplePtr W0, W1;
      
      if (Wsamples.size() > 1 && !info.varyingTopology)
      {
         radius = AiArrayAllocate(info.pointCount, mDso->numMotionSamples(), AI_TYPE_FLOAT);
         
         unsigned int po = 0;
         
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            b = 0.0;
            
            Wsamples.getSamples(mDso->motionSampleTime(i), Wsamp0, Wsamp1, b);
            
            W0 = Wsamp0->data().getVals();
            
            if (W0->size() != 1 &&
                W0->size() != info.curveCount &&
                W0->size() != info.pointCount) // W0->size() won't match pointCount for NURBS
            {
               AiArrayDestroy(radius);
               radius = 0;
               break;
            }
            
            unsigned int Wcount = (unsigned int) W0->size();
            unsigned int pi = 0;
            
            if (b > 0.0)
            {
               a = 1.0 - b;
               
               W1 = Wsamp1->data().getVals();
               
               if (W1->size() != W0->size())
               {
                  AiArrayDestroy(radius);
                  radius = 0;
                  break;
               }
               
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int wi = (Wcount == 1 ? 0 : (Wcount == info.curveCount ? ci : pi));
                     
                     float w0 = W0->get()[wi];
                     float w1 = W1->get()[wi];
                     
                     AiArraySetFlt(radius, po + pi, adjustRadius(0.5f * (a * w0 + b * w1)));
                  }
               }
            }
            else
            {
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int wi = (Wcount == 1 ? 0 : (Wcount == info.curveCount ? ci : pi));
                     
                     AiArraySetFlt(radius, po + pi, adjustRadius(0.5f * W0->get()[wi]));
                  }
               }
            }
            
            po += info.pointCount;
         }
      }
      else
      {
         Wsamples.getSamples(mDso->renderTime(), Wsamp0, Wsamp1, b);
            
         W0 = Wsamp0->data().getVals();
         
         if (W0->size() == 1 ||
             W0->size() == info.curveCount ||
             W0->size() == info.pointCount)
         {
            unsigned int Wcount = (unsigned int) W0->size();
         
            radius = AiArrayAllocate(info.pointCount, 1, AI_TYPE_FLOAT);
            
            unsigned int pi = 0;
            
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
               
               for (unsigned int j=0; j<np; ++j, ++pi)
               {
                  unsigned int wi = (Wcount == 1 ? 0 : (Wcount == info.curveCount ? ci : pi));
                  
                  AiArraySetFlt(radius, pi, adjustRadius(0.5f * W0->get()[wi]));
               }
            }
         }
      }
   }
   
   if (!radius)
   {
      AiMsgWarning("[abcproc] Defaulting curve radius to 0.01");
      
      float defaultWidth = 0.01f; // maybe an other option for this
      
      radius = AiArrayAllocate(info.pointCount, 1, AI_TYPE_FLOAT);
      
      for (unsigned int i=0; i<info.pointCount; ++i)
      {
         AiArraySetFlt(radius, i, adjustRadius(0.5f * defaultWidth));
      }
   }
   
   AiNodeSetArray(mNode, "radius", radius);
   
   // Process normals (orientation)
   
   AtArray *orientations = 0;
   
   if (Nsamples.size() > 0)
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Reading curve normal(s)...");
      }
      
      Alembic::Abc::N3fArraySamplePtr N0, N1;
      Alembic::Abc::V3f n0, n1;
      AtVector nrm;
         
      if (Nsamples.size() > 1 && !info.varyingTopology)
      {
         orientations = AiArrayAllocate(info.pointCount, mDso->numMotionSamples(), AI_TYPE_VECTOR);
         
         unsigned int po = 0;
         
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            b = 0.0;
            
            Nsamples.getSamples(mDso->motionSampleTime(i), Nsamp0, Nsamp1, b);
            
            N0 = Nsamp0->data().getVals();
            
            if (N0->size() != 1 &&
                N0->size() != info.curveCount &&
                N0->size() != info.pointCount)
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Curve normals count mismatch");
               }
               
               AiArrayDestroy(orientations);
               orientations = 0;
               
               break;
            }
            
            unsigned int Ncount = (unsigned int) N0->size();
            unsigned int pi = 0;
            
            if (b > 0.0)
            {
               a = 1.0 - b;
               
               N1 = Nsamp1->data().getVals();
               
               if (N1->size() != N0->size())
               {
                  AiArrayDestroy(orientations);
                  orientations = 0;
                  break;
               }
               
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int ni = (Ncount == 1 ? 0 : (Ncount == info.curveCount ? ci : pi));
                     
                     n0 = N0->get()[ni];
                     n1 = N1->get()[ni];
                     nrm.x = a * n0.x + b * n1.x;
                     nrm.y = a * n0.y + b * n1.y;
                     nrm.z = a * n0.z + b * n1.z;
                     AiArraySetVec(orientations, po + pi, nrm);
                  }
               }
            }
            else
            {
               for (unsigned int ci=0; ci<info.curveCount; ++ci)
               {
                  unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
                  
                  for (unsigned int j=0; j<np; ++j, ++pi)
                  {
                     unsigned int ni = (Ncount == 1 ? 0 : (Ncount == info.curveCount ? ci : pi));
                     
                     n0 = N0->get()[ni];
                     nrm.x = n0.x;
                     nrm.y = n0.y;
                     nrm.z = n0.z;
                     AiArraySetVec(orientations, po + pi, nrm);
                  }
               }
            }
            
            po += info.pointCount;
         }
      }
      else
      {
         Nsamples.getSamples(mDso->renderTime(), Nsamp0, Nsamp0, b);
            
         N0 = Nsamp0->data().getVals();
         
         if (N0->size() == 1 ||
             N0->size() == info.curveCount ||
             N0->size() == info.pointCount)
         {
            unsigned int Ncount = (unsigned int) N0->size();
         
            orientations = AiArrayAllocate(info.pointCount, 1, AI_TYPE_VECTOR);
            
            unsigned int pi = 0;
            
            for (unsigned int ci=0; ci<info.curveCount; ++ci)
            {
               unsigned int np = AiArrayGetUInt(num_points, ci) - 2;
               
               for (unsigned int j=0; j<np; ++j, ++pi)
               {
                  unsigned int ni = (Ncount == 1 ? 0 : (Ncount == info.curveCount ? ci : pi));
                  
                  n0 = N0->get()[ni];
                  nrm.x = n0.x;
                  nrm.y = n0.y;
                  nrm.z = n0.z;
                  AiArraySetVec(orientations, pi, nrm);
               }
            }
         }
         else if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Curve normals count mismatch");
         }
      }
   }
   
   if (orientations)
   {
      AiNodeSetStr(mNode, "mode", "oriented");
      AiNodeSetArray(mNode, "orientations", orientations);
   }
   
   // Process uvs
   
   if (uvs)
   {
      if (info.pointAttrs.find("uv") != info.pointAttrs.end() ||
          info.primitiveAttrs.find("uv") != info.primitiveAttrs.end() ||
          info.objectAttrs.find("uv") != info.objectAttrs.end())
      {
         AiMsgWarning("[abcproc] Curves node already has a 'uv' user attribute, ignore alembic's");
      }
      else
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Reading curve uv(s)...");
         }
         
         Alembic::Abc::ICompoundProperty uvsProp = uvs.getValueProperty().getParent();
         
         UserAttribute uvsAttr;
         
         InitUserAttribute(uvsAttr);
         
         if (ReadUserAttribute(uvsAttr, uvsProp.getParent(), uvsProp.getHeader(), attribsTime, true, interpolateAttribs))
         {
            switch (uvsAttr.arnoldCategory)
            {
            case AI_USERDEF_CONSTANT:
               SetUserAttribute(mNode, "uv", uvsAttr, 0);
               break;
            case AI_USERDEF_UNIFORM:
               SetUserAttribute(mNode, "uv", uvsAttr, info.curveCount);
               break;
            case AI_USERDEF_VARYING:
               SetUserAttribute(mNode, "uv", uvsAttr, info.pointCount);
               break;
            }
         }
         
         DestroyUserAttribute(uvsAttr);
      }
   }
   
   // Reference positions
   
   if (mDso->outputReference())
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate reference attributes");
      }
      
      UserAttributes refPointAttrs;
      UserAttributes *pointAttrs = &refPointAttrs;
      AlembicCurves *refCurves = 0;
      
      getReferenceCurves(node, info, refCurves, pointAttrs);
      
      if (refCurves)
      {
         fillReferencePositions(refCurves, info, pointAttrs);
      }
      else
      {
         AiMsgWarning("[abcproc] No valid reference curve found");
      }
      
      DestroyUserAttributes(refPointAttrs);
   }
   
   SetUserAttributes(mNode, info.objectAttrs, 0);
   SetUserAttributes(mNode, info.primitiveAttrs, info.curveCount);
   SetUserAttributes(mNode, info.pointAttrs, info.pointCount);
      
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
