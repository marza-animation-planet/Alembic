#include "visitors.h"

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
         AiMsgInfo("[abcproc] %lu xform sample(s) for \"%s\"", matrices.size(), node.path().c_str());
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
   
   TimeSampleList<Alembic::AbcGeom::IPolyMeshSchema>::Iterator samp0, samp1;
   TimeSampleList<Alembic::AbcGeom::IPolyMeshSchema> &meshSamples = node.samples().schemaSamples;
   
   for (size_t i=0; i<mDso->numMotionSamples(); ++i)
   {
      double t = mDso->motionSampleTime(i);
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Sample xform \"%s\" at t=%lf", node.path().c_str(), t);
      }
      
      node.sampleSchema(t, t, i>0);
   }
   
   // Read attributes
   
   Alembic::Abc::ICompoundProperty userProps = schema.getUserProperties();
   Alembic::Abc::ICompoundProperty geomParams = schema.getArbGeomParams();
   
   // use attribsFrame(), blurAttrib() [read more that 1 sample and interpolate or extrapolate (point typed attribs only)]
   // don't forget ignoreMotionBlur() too
   
   if (mDso->readObjectAttribs())
   {
      // double t = mDso->attribsTime(mDso->attribsFrame());
      // userProps + constant geomParams
      //std::string attribName = ...;
      //
      //TimeSampleList<PropType> propSamples;
      //propSamples.update(t, t, true);
      //propSamples.getSamples(t, samp0, samp1, blend)
      //if (blend > 0.0 && !mDso->ignoreMotionBlur() && mDso->blurAttrib(attribName))
      //{  
      //  interpolate if topology isn't chaning
      //  extrapolate otherwise (only points can be)
      //}
      //else
      //{
      //  use samp0 only 
      //}
   }
   
   if (mDso->readPrimitiveAttribs())
   {
      // uniform geomParams
   }
   
   if (mDso->readPointAttribs())
   {
      // varying geomParams
   }
   
   if (mDso->readVertexAttribs())
   {
      // face-varying geomParams
   }
   
   if (meshSamples.size() == 0)
   {
      return AlembicNode::DontVisitChildren;
   }
   
   std::string baseName = node.formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
   std::string name = mDso->uniqueName(baseName);
   
   mNode = AiNode("polymesh");
   AiNodeSetStr(mNode, "name", name.c_str());
   
   // Least required parameters
   // 'nsides'
   // 'vlist' (mblur ok)
   // 'vidxs'
   
   // Normals: (only if both subd and smoothing are off)
   // 'nlist' (mblur ok)
   // 'nidxs'
   
   // UVs:
   // not part of the base schema
   
   //if (mDso->computeTangents())
   //{
   //  force read normals, if none, compute them
   //}
   
   if (meshSamples.size() == 1)
   {
      samp0 = meshSamples.begin();
      
      Alembic::Abc::P3fArraySamplePtr P = samp0->data().getPositions();
      Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
      Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
      
      AtArray *vlist = AiArrayAllocate(P->size(), 1, AI_TYPE_POINT);
      AtArray *nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
      AtArray *vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
      
      for (size_t i=0, j=0; i<FC->size(); ++i)
      {
         size_t nv = (*FC)[i];
         AiArraySetUInt(nsides, i, (*FC)[i]);
         for (size_t k=0; k<nv; ++k, ++j)
         {
            AiArraySetUInt(vidxs, j, (*FI)[j]);
         }
      }
      
      for (size_t i=0; i<P->size(); ++i)
      {
         Alembic::Abc::V3f _p = (*P)[i];
         AtPoint p = {_p.x, _p.y, _p.z};
         
         AiArraySetPnt(vlist, i, p);
      }
      
      AiNodeSetArray(mNode, "nsides", nsides);
      AiNodeSetArray(mNode, "vlist", vlist);
      AiNodeSetArray(mNode, "vidxs", vidxs);
   }
   else
   {
      // we have velocities in schema, but no acceleration
      
      // check for changing topology first
      // if not just do simple interpolation
      
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
      }
   }
         
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
