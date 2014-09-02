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
         
         node.sampleBounds(t, t, i>0);
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

MakeShape::MakeShape(Dso *dso)
   : mDso(dso)
   , mNode(0)
{
}
   
AlembicNode::VisitReturn MakeShape::enter(AlembicMesh &node, AlembicNode *)
{
   Alembic::AbcGeom::IPolyMeshSchema schema = node.typedObject().getSchema();
   
   bool varyingTopology = (schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology);
   
   bool subd = false;
   bool smoothing = false;
   
   const AtUserParamEntry *pe;
   
   pe = AiNodeLookUpUserParameter(mDso->procNode(), "subdiv_type");
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
   
   // With smoothing off, nlist/nidxs have no influence
   bool readNormals = (!subd && smoothing);
   
   if (mDso->verbose() && readNormals)
   {
      AiMsgInfo("[abcproc] Read normals data if available");
   }
   
   TimeSampleList<Alembic::AbcGeom::IPolyMeshSchema>::ConstIterator samp0, samp1;
   TimeSampleList<Alembic::AbcGeom::IPolyMeshSchema> &meshSamples = node.samples().schemaSamples;
   
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> Nsamples;
   Alembic::AbcGeom::IN3fGeomParam N = schema.getNormalsParam();
   
   std::map<std::string, Alembic::AbcGeom::IV2fGeomParam> UVs;
   if (schema.getUVsParam().valid())
   {
      UVs[""] = schema.getUVsParam();
   }
   
   if (varyingTopology)
   {
      node.sampleSchema(mDso->renderTime(), mDso->renderTime(), false);
      
      if (readNormals && N.valid())
      {
         Nsamples.update(N, mDso->renderTime(), mDso->renderTime(), false);
      }
   }
   else
   {
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
         double t = mDso->motionSampleTime(i);
      
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample mesh \"%s\" at t=%lf", node.path().c_str(), t);
         }
      
         node.sampleSchema(t, t, i>0);
      
         if (readNormals && N.valid())
         {
            Nsamples.update(N, t, t, i>0);
         }
      }
   }
   
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Read %lu mesh samples", meshSamples.size());
      if (readNormals)
      {
         AiMsgInfo("[abcproc] Read %lu normal samples", Nsamples.size());
      }
   }
   
   if (meshSamples.size() == 0)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // Collect attributes
   
   Alembic::Abc::ICompoundProperty userProps = schema.getUserProperties();
   Alembic::Abc::ICompoundProperty geomParams = schema.getArbGeomParams();
   
   double attribsTime = (varyingTopology ? mDso->renderTime() : mDso->attribsTime(mDso->attribsFrame()));
   bool interpolateAttribs = !varyingTopology;
   
   UserAttributes objectAttrs;
   UserAttributes primitiveAttrs;
   UserAttributes pointAttrs;
   UserAttributes vertexAttrs;
   
   if (userProps.valid() && mDso->readObjectAttribs())
   {
      for (size_t i=0; i<geomParams.getNumProperties(); ++i)
      {
         const Alembic::AbcCoreAbstract::PropertyHeader &header = userProps.getPropertyHeader(i);
         
         std::string name = header.getName();
         mDso->cleanAttribName(name);
         
         UserAttribute ua;
         
         InitUserAttribute(ua);
         
         ua.arnoldCategory = AI_USERDEF_CONSTANT;
         
         if (ReadUserAttribute(ua, userProps, header, attribsTime, false, interpolateAttribs))
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read \"%s\" object attribute as \"%s\"", header.getName().c_str(), name.c_str());
            }
            objectAttrs[name] = ua;
         }
         else
         {
            DestroyUserAttribute(ua);
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
         
         if (scope == Alembic::AbcGeom::kFacevaryingScope &&
             Alembic::AbcGeom::IV3fGeomParam::matches(header) &&
             Alembic::AbcGeom::isUV(header))
         {
            UVs[header.getName()] = Alembic::AbcGeom::IV2fGeomParam(geomParams, header.getName());
         }
         else
         {
            UserAttributes *attrs = 0;
            
            std::string name = header.getName();
            mDso->cleanAttribName(name);
            
            UserAttribute ua;
               
            InitUserAttribute(ua);
            
            switch (scope)
            {
            case Alembic::AbcGeom::kFacevaryingScope:
               if (mDso->readVertexAttribs())
               {
                  ua.arnoldCategory = AI_USERDEF_INDEXED;
                  attrs = &vertexAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" vertex attribute as \"%s\"", header.getName().c_str(), name.c_str());
                  }
               }
               break;
            case Alembic::AbcGeom::kVaryingScope:
            case Alembic::AbcGeom::kVertexScope:
               if (mDso->readPointAttribs() || specialPointNames.find(name) != specialPointNames.end())
               {
                  ua.arnoldCategory = AI_USERDEF_VARYING;
                  attrs = &pointAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" point attribute as \"%s\"", header.getName().c_str(), name.c_str());
                  }
               }
               break;
            case Alembic::AbcGeom::kUniformScope:
               if (mDso->readPrimitiveAttribs())
               {
                  ua.arnoldCategory = AI_USERDEF_UNIFORM;
                  attrs = &primitiveAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" primitive attribute as \"%s\"", header.getName().c_str(), name.c_str());
                  }
               }
               break;
            case Alembic::AbcGeom::kConstantScope:
               if (mDso->readObjectAttribs())
               {
                  ua.arnoldCategory = AI_USERDEF_CONSTANT;
                  attrs = &objectAttrs;
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read \"%s\" object attribute as \"%s\"", header.getName().c_str(), name.c_str());
                  }
               }
               break;
            default:
               continue;
            }
            
            if (attrs)
            {
               if (ReadUserAttribute(ua, geomParams, header, attribsTime, true, interpolateAttribs))
               {
                  (*attrs)[name] = ua;
               }
               else
               {
                  if (mDso->verbose())
                  {
                     AiMsgWarning("[abcproc] Failed");
                  }
                  DestroyUserAttribute(ua);
               }
            }
         }
      }
   }
   
   std::string baseName = node.formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
   std::string name = mDso->uniqueName(baseName);
   
   unsigned int pointCount = 0;
   unsigned int vertexCount = 0;
   unsigned int polygonCount = 0;
   unsigned int *polygonVertexCount = 0;
   unsigned int *vertexPointIndices = 0;
   // alembic vertex index -> arnold vertex index (if winding is not reversed we have remapVertexIndex[i] == i)
   unsigned int *arnoldVertexIndex = 0;
   
   AtArray *nsides = 0;
   AtArray *vidxs = 0;
   AtArray *vlist = 0;
   AtArray *nlist = 0;
   AtArray *nidxs = 0;
   
   mNode = AiNode("polymesh");
   AiNodeSetStr(mNode, "name", name.c_str());
   
   if (meshSamples.size() == 1)
   {
      samp0 = meshSamples.begin();
      
      Alembic::Abc::P3fArraySamplePtr P = samp0->data().getPositions();
      Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
      Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
      
      pointCount = P->size();
      polygonCount = FC->size();
      polygonVertexCount = new unsigned int[FC->size()];
      vertexPointIndices = new unsigned int[FI->size()];
      arnoldVertexIndex = new unsigned int[FI->size()];
      
      vlist = AiArrayAllocate(P->size(), 1, AI_TYPE_POINT);
      nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
      vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
      
      for (size_t i=0, j=0; i<FC->size(); ++i)
      {
         unsigned int nv = FC->get()[i];
         
         polygonVertexCount[i] = nv;
         vertexCount += nv;
         
         if (nv == 0)
         {
            continue;
         }
         
         // Alembic winding is CW not CCW
         if (mDso->reverseWinding())
         {
            // Keep first vertex unchanged
            arnoldVertexIndex[j] = j;
            vertexPointIndices[j] = FI->get()[j];
            
            for (size_t k=1; k<nv; ++k)
            {
               arnoldVertexIndex[j+k] = j + nv - k;
               vertexPointIndices[j+nv-k] = FI->get()[j+k];
            }
            
            j += nv;
         }
         else
         {
            for (size_t k=0; k<nv; ++k, ++j)
            {
               arnoldVertexIndex[j] = j;
               vertexPointIndices[j] = FI->get()[j];
            }
         }
      }
      
      for (size_t i=0; i<P->size(); ++i)
      {
         Alembic::Abc::V3f _p = (*P)[i];
         AtPoint p = {_p.x, _p.y, _p.z};
         
         AiArraySetPnt(vlist, i, p);
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Read %lu faces", FC->size());
         AiMsgInfo("[abcproc] Read %lu points", P->size());
         AiMsgInfo("[abcproc] Read %lu vertices", FI->size());
      }
   }
   else
   {
      if (varyingTopology)
      {
         double blend = 0.0;
         
         meshSamples.getSamples(mDso->renderTime(), samp0, samp1, blend);
         
         Alembic::Abc::P3fArraySamplePtr P = samp0->data().getPositions();
         Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
         Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
         
         polygonCount = FC->size();
         pointCount = P->size();
         polygonVertexCount = new unsigned int[FC->size()];
         vertexPointIndices = new unsigned int[FI->size()];
         arnoldVertexIndex = new unsigned int[FI->size()];
         
         nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
         vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
         
         // Get velocity
         const float *vel = 0;
         if (samp0->data().getVelocities())
         {
            vel = (const float*) samp0->data().getVelocities()->getData();
         }
         else
         {
            UserAttributes::iterator it = pointAttrs.find("velocity");
            
            if (it == pointAttrs.end() || it->second.arnoldType != AI_TYPE_VECTOR || it->second.dataCount != pointCount)
            {
               it = pointAttrs.find("v");
               if (it != pointAttrs.end() && (it->second.arnoldType != AI_TYPE_VECTOR || it->second.dataCount != pointCount))
               {
                  it = pointAttrs.end();
               }
            }
            
            if (it != pointAttrs.end())
            {
               vel = (const float*) it->second.data;
            }
         }
         
         // Get acceleration
         const float *acc = 0;
         if (vel)
         {
            UserAttributes::iterator it = pointAttrs.find("acceleration");
            
            if (it == pointAttrs.end() || it->second.arnoldType != AI_TYPE_VECTOR || it->second.dataCount != pointCount)
            {
               it = pointAttrs.find("accel");
               if (it == pointAttrs.end() || it->second.arnoldType != AI_TYPE_VECTOR || it->second.dataCount != pointCount)
               {
                  it = pointAttrs.find("a");
                  if (it != pointAttrs.end() && (it->second.arnoldType != AI_TYPE_VECTOR || it->second.dataCount != pointCount))
                  {
                     it = pointAttrs.end();
                  }
               }
            }
            
            if (it != pointAttrs.end())
            {
               acc = (const float*) it->second.data;
            }
         }
         
         for (size_t i=0, j=0; i<FC->size(); ++i)
         {
            unsigned int nv = FC->get()[i];
            
            polygonVertexCount[i] = nv;
            vertexCount += nv;
            
            if (nv == 0)
            {
               continue;
            }
            
            if (mDso->reverseWinding())
            {
               arnoldVertexIndex[j] = j;
               vertexPointIndices[j] = FI->get()[j];
               
               for (size_t k=1; k<nv; ++k)
               {
                  arnoldVertexIndex[j+k] = j + nv - k;
                  vertexPointIndices[j+nv-k] = FI->get()[j+k];
               }
               
               j += nv;
            }
            else
            {
               for (size_t k=0; k<nv; ++k, ++j)
               {
                  arnoldVertexIndex[j] = j;
                  vertexPointIndices[j] = FI->get()[j];
               }
            }
         }
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Read %lu faces", FC->size());
            AiMsgInfo("[abcproc] Read %lu points", P->size());
            AiMsgInfo("[abcproc] Read %lu vertices", FI->size());
            
            if (vel)
            {
               AiMsgInfo("[abcproc] Use velocity to compute sample positions");
               if (acc)
               {
                  AiMsgInfo("[abcproc] Use velocity & acceleration to compute sample positions");
               }
            }
         }
         
         AtPoint pnt;
         
         if (!vel)
         {
            vlist = AiArrayAllocate(P->size(), 1, AI_TYPE_POINT);
            
            for (size_t i=0; i<P->size(); ++i)
            {
               Alembic::Abc::V3f p = P->get()[i];
               
               pnt.x = p.x;
               pnt.y = p.y;
               pnt.z = p.z;
               
               AiArraySetPnt(vlist, i, pnt);
            }
         }
         else
         {
            vlist = AiArrayAllocate(P->size(), mDso->numMotionSamples(), AI_TYPE_POINT);
            
            for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
            {
               double dt = mDso->motionSampleTime(i) - mDso->renderTime();
               
               if (acc)
               {
                  const float *vvel = vel;
                  const float *vacc = acc;
                  for (size_t k=0; k<P->size(); ++k, vvel+=3, vacc+=3)
                  {
                     Alembic::Abc::V3f p = P->get()[k];
                     
                     pnt.x = p.x + dt * (vvel[0] + 0.5 * dt * vacc[0]);
                     pnt.y = p.y + dt * (vvel[1] + 0.5 * dt * vacc[1]);
                     pnt.z = p.z + dt * (vvel[2] + 0.5 * dt * vacc[2]);
                     
                     AiArraySetPnt(vlist, j+k, pnt);
                  }
               }
               else
               {
                  const float *vvel = vel;
                  for (size_t k=0; k<P->size(); ++k, vvel+=3)
                  {
                     Alembic::Abc::V3f p = P->get()[k];
                     
                     pnt.x = p.x + dt * vvel[0];
                     pnt.y = p.y + dt * vvel[1];
                     pnt.z = p.z + dt * vvel[2];
                     
                     AiArraySetPnt(vlist, j+k, pnt);
                  }
               }
               
               j += pointCount;
            }
         }
      }
      else
      {
         AtPoint pnt;
         
         for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
         {
            double blend = 0.0;
            double t = mDso->motionSampleTime(i);
            
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read position samples [t=%lf]", t);
            }
            
            meshSamples.getSamples(t, samp0, samp1, blend);
            
            Alembic::Abc::P3fArraySamplePtr P0 = samp0->data().getPositions();
            
            if (pointCount > 0 && P0->size() != pointCount)
            {
               AiMsgWarning("[abcproc] Changing points count amongst sample");
               if (nsides) AiArrayDestroy(nsides);
               if (vidxs) AiArrayDestroy(vidxs);
               if (vlist) AiArrayDestroy(vlist);
               nsides = vidxs = vlist = 0;
               break;
            }
            
            pointCount = P0->size();
            
            if (!nsides)
            {
               Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
               Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
               
               polygonCount = FC->size();
               polygonVertexCount = new unsigned int[FC->size()];
               vertexPointIndices = new unsigned int[FI->size()];
               arnoldVertexIndex = new unsigned int[FI->size()];
               
               nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
               vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
               
               for (size_t k=0, l=0; k<FC->size(); ++k)
               {
                  unsigned int nv = FC->get()[k];
                  
                  polygonVertexCount[k] = nv;
                  vertexCount += nv;
                  
                  if (nv == 0)
                  {
                     continue;
                  }
                  
                  if (mDso->reverseWinding())
                  {
                     arnoldVertexIndex[l] = l;
                     vertexPointIndices[l] = FI->get()[l];
                     
                     for (size_t m=1; m<nv; ++m)
                     {
                        arnoldVertexIndex[l+m] = l + nv - m;
                        vertexPointIndices[l+nv-m] = FI->get()[l+m];
                     }
                     
                     l += nv;
                  }
                  else
                  {
                     for (size_t m=0; m<nv; ++m, ++l)
                     {
                        arnoldVertexIndex[l] = l;
                        vertexPointIndices[l] = FI->get()[l];
                     }
                  }
               }
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu faces", FC->size());
                  AiMsgInfo("[abcproc] Read %lu vertices", FI->size());
               }
            }
            
            if (!vlist)
            {
               vlist = AiArrayAllocate(P0->size(), mDso->numMotionSamples(), AI_TYPE_POINT);
            }
            
            if (blend > 0.0)
            {
               Alembic::Abc::P3fArraySamplePtr P1 = samp1->data().getPositions();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu points / %lu points [interpolate]", P0->size(), P1->size());
               }
               
               double a = 1.0 - blend;
               
               for (size_t k=0; k<P0->size(); ++k)
               {
                  Alembic::Abc::V3f p0 = P0->get()[k];
                  Alembic::Abc::V3f p1 = P1->get()[k];
                  
                  pnt.x = a * p0.x + blend * p1.x;
                  pnt.y = a * p0.y + blend * p1.y;
                  pnt.z = a * p0.z + blend * p1.z;
                  
                  AiArraySetPnt(vlist, j+k, pnt);
               }
            }
            else
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu points", P0->size());
               }
               
               for (size_t k=0; k<P0->size(); ++k)
               {
                  Alembic::Abc::V3f p = P0->get()[k];
                  
                  pnt.x = p.x;
                  pnt.y = p.y;
                  pnt.z = p.z;
                  
                  AiArraySetPnt(vlist, j+k, pnt);
               }
            }
            
            j += P0->size();
         }
      }
   }
   
   if (!nsides || !vidxs || !vlist || !polygonVertexCount || !vertexPointIndices)
   {
      AiNodeDestroy(mNode);
      mNode = 0;
      AiMsgWarning("[abcproc] Failed to generated mesh data");
      return AlembicNode::DontVisitChildren;
   }
   
   AiArraySetKey(nsides, 0, polygonVertexCount);
   AiArraySetKey(vidxs, 0, vertexPointIndices);
   
   AiNodeSetArray(mNode, "nsides", nsides);
   AiNodeSetArray(mNode, "vidxs", vidxs);
   AiNodeSetArray(mNode, "vlist", vlist);
   
   // Set mesh normals
   
   if (Nsamples.size() > 0)
   {
      TimeSampleList<Alembic::AbcGeom::IN3fGeomParam>::ConstIterator n0, n1;
      double blend = 0.0;
      AtVector vec;
      
      if (varyingTopology || Nsamples.size() == 1)
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
               AiArraySetUInt(nidxs, arnoldVertexIndex[i], idxs->get()[i]);
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
               AiArraySetUInt(nidxs, arnoldVertexIndex[i], i);
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
                        
                        AiArraySetVec(nlist, j + arnoldVertexIndex[k], vec);
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
                        
                        AiArraySetVec(nlist, j + arnoldVertexIndex[k], vec);
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
                        
                        AiArraySetVec(nlist, j + arnoldVertexIndex[k], vec);
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
                        
                        AiArraySetVec(nlist, j + arnoldVertexIndex[k], vec);
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
                     
                     AiArraySetVec(nlist, j + arnoldVertexIndex[k], vec);
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
                     
                     AiArraySetVec(nlist, j + arnoldVertexIndex[k], vec);
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
   
   // For all reamaining data, no motion blur
   
   // Set mesh UVs
   
   for (std::map<std::string, Alembic::AbcGeom::IV2fGeomParam>::iterator uvit=UVs.begin(); uvit!=UVs.end(); ++uvit)
   {
      TimeSampleList<Alembic::AbcGeom::IV2fGeomParam> sampler;
      TimeSampleList<Alembic::AbcGeom::IV2fGeomParam>::ConstIterator uv0, uv1;
      
      if (uvit->first == "uv")
      {
         AiMsgWarning("[abcproc] \"uv\" is a reserved UV set name");
         continue;
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Sample \"%s\" uvs", uvit->first.length() > 0 ? uvit->first.c_str() : "uv");
      }
      
      if (sampler.update(uvit->second, mDso->renderTime(), mDso->renderTime(), true))
      {
         double blend = 0.0;
         AtPoint2 pnt2;
         
         AtArray *uvlist = 0;
         AtArray *uvidxs = 0;
         
         if (sampler.getSamples(mDso->renderTime(), uv0, uv1, blend))
         {
            if (blend > 0.0 && !varyingTopology)
            {
               double a = 1.0 - blend;
               
               Alembic::Abc::V2fArraySamplePtr vals0 = uv0->data().getVals();
               Alembic::Abc::V2fArraySamplePtr vals1 = uv1->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs0 = uv0->data().getIndices();
               Alembic::Abc::UInt32ArraySamplePtr idxs1 = uv1->data().getIndices();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu uvs / %lu uvs [interpolate]", vals0->size(), vals1->size());
               }
               
               if (idxs0)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read %lu uv indices", idxs0->size());
                  }
                  
                  if (idxs1)
                  {
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu uv indices", idxs1->size());
                     }
                     
                     if (idxs0->size() != idxs1->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<idxs0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[idxs0->get()[i]];
                           Alembic::Abc::V2f val1 = vals1->get()[idxs1->get()[i]];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, arnoldVertexIndex[i], i);
                        }
                     }
                  }
                  else
                  {
                     // I don't think this should happend
                     if (vals1->size() != idxs0->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<idxs0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[idxs0->get()[i]];
                           Alembic::Abc::V2f val1 = vals1->get()[i];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, arnoldVertexIndex[i], i);
                        }
                     }
                  }
               }
               else
               {
                  if (idxs1)
                  {
                     Alembic::Abc::UInt32ArraySamplePtr idxs1 = uv1->data().getIndices();
                     
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu uv indices", idxs1->size());
                     }
                     
                     if (idxs1->size() != vals0->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(vals0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(vals0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<vals0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[i];
                           Alembic::Abc::V2f val1 = vals1->get()[idxs1->get()[i]];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, arnoldVertexIndex[i], i);
                        }
                     }
                  }
                  else
                  {
                     if (vals0->size() != vals1->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(vals0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(vals0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<vals0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[i];
                           Alembic::Abc::V2f val1 = vals1->get()[i];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, arnoldVertexIndex[i], i);
                        }
                     }
                  }
               }
            }
            else
            {
               Alembic::Abc::V2fArraySamplePtr vals = uv0->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs = uv0->data().getIndices();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu uvs", vals->size());
               }
               
               if (idxs)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read %lu uv indices", idxs->size());
                  }
                  
                  uvlist = AiArrayAllocate(vals->size(), 1, AI_TYPE_POINT2);
                  uvidxs = AiArrayAllocate(idxs->size(), 1, AI_TYPE_UINT);
                  
                  for (size_t i=0; i<vals->size(); ++i)
                  {
                     Alembic::Abc::V2f val = vals->get()[i];
                     
                     pnt2.x = val.x;
                     pnt2.y = val.y;
                     
                     AiArraySetPnt2(uvlist, i, pnt2);
                  }
                  
                  for (size_t i=0; i<idxs->size(); ++i)
                  {
                     AiArraySetUInt(uvidxs, arnoldVertexIndex[i], idxs->get()[i]);
                  }
               }
               else
               {
                  uvlist = AiArrayAllocate(vals->size(), 1, AI_TYPE_POINT2);
                  uvidxs = AiArrayAllocate(vals->size(), 1, AI_TYPE_UINT);
                  
                  for (size_t i=0; i<vals->size(); ++i)
                  {
                     Alembic::Abc::V2f val = vals->get()[i];
                     
                     pnt2.x = val.x;
                     pnt2.y = val.y;
                     
                     AiArraySetPnt2(uvlist, i, pnt2);
                     AiArraySetUInt(uvidxs, arnoldVertexIndex[i], i);
                  }
               }
            }
         }
         
         if (uvlist && uvidxs)
         {
            if (uvit->first.length() > 0)
            {
               std::string iname = uvit->first + "idxs";
               
               AiNodeDeclare(mNode, uvit->first.c_str(), "indexed POINT2");
               AiNodeSetArray(mNode, uvit->first.c_str(), uvlist);
               AiNodeSetArray(mNode, iname.c_str(), uvidxs);
            }
            else
            {
               AiNodeSetArray(mNode, "uvlist", uvlist);
               AiNodeSetArray(mNode, "uvidxs", uvidxs);
            }
         }
      }
      else
      {
         AiMsgWarning("[abcproc] Failed to output \"%s\" uvs", uvit->first.length() > 0 ? uvit->first.c_str() : "uv");
      }
   }
   
   // Set user defined attributes
   
   SetUserAttributes(mNode, objectAttrs);
   SetUserAttributes(mNode, primitiveAttrs);
   SetUserAttributes(mNode, pointAttrs);
   SetUserAttributes(mNode, vertexAttrs, arnoldVertexIndex);
   
   DestroyUserAttributes(objectAttrs);
   DestroyUserAttributes(primitiveAttrs);
   DestroyUserAttributes(pointAttrs);
   DestroyUserAttributes(vertexAttrs);
   
   delete[] polygonVertexCount;
   delete[] vertexPointIndices;
   delete[] arnoldVertexIndex;
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicSubD &node, AlembicNode *)
{
   // generate a polymesh
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn MakeShape::enter(AlembicNode &node, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}

void MakeShape::leave(AlembicNode &, AlembicNode *)
{
}
