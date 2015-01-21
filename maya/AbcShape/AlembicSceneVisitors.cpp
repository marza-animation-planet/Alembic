#include "AlembicSceneVisitors.h"

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

#include <iostream>



WorldUpdate::WorldUpdate(double t,
                         bool ignoreTransforms,
                         bool ignoreInstances,
                         bool ignoreVisibility)
   : mTime(t)
   , mIgnoreTransforms(ignoreTransforms)
   , mIgnoreInstances(ignoreInstances)
   , mIgnoreVisibility(ignoreVisibility)
   , mNumShapes(0)
{
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicXform &node, AlembicNode *instance)
{
   bool noData = true;
   bool updated = true;
   
   if (node.isLocator())
   {
      // sampleBounds utility function will sample locator property
      
      if (node.sampleBounds(mTime, mTime, false, &updated))
      {
         TimeSampleList<ILocatorProperty> &locatorSamples = node.samples().locatorSamples;
         
         if (updated || fabs(locatorSamples.lastEvaluationTime - mTime) > 0.0001)
         {
            TimeSampleList<ILocatorProperty>::ConstIterator samp0, samp1;
            double blend = 0.0;
            
            if (locatorSamples.getSamples(mTime, samp0, samp1, blend))
            {
               if (blend > 0.0)
               {
                  Alembic::Abc::V3d v0, v1;
                  
                  v0.setValue(samp0->data().T[0], samp0->data().T[1], samp0->data().T[2]);
                  v1.setValue(samp1->data().T[0], samp1->data().T[1], samp1->data().T[2]);
                  
                  node.setLocatorPosition((1.0 - blend) * v0 + blend * v1);
                  
                  v0.setValue(samp0->data().S[0], samp0->data().S[1], samp0->data().S[2]);
                  v1.setValue(samp1->data().S[0], samp1->data().S[1], samp1->data().S[2]);
                  
                  node.setLocatorScale((1.0 - blend) * v0 + blend * v1);
               }
               else
               {
                  node.setLocatorPosition(Alembic::Abc::V3d(samp0->data().T[0], samp0->data().T[1], samp0->data().T[2]));
                  node.setLocatorScale(Alembic::Abc::V3d(samp0->data().S[0], samp0->data().S[1], samp0->data().S[2]));
               }
               
               locatorSamples.lastEvaluationTime = mTime;
               
               noData = false;
            }
         }
         else
         {
            noData = false;
         }
      }
      
      if (noData)
      {
         node.setLocatorPosition(Alembic::Abc::V3d(0, 0, 0));
         node.setLocatorScale(Alembic::Abc::V3d(1, 1, 1));
      }
   }
   else
   {
      if (!mIgnoreTransforms)
      {
         if (node.sampleBounds(mTime, mTime, false, &updated))
         {
            TimeSampleList<Alembic::AbcGeom::IXformSchema> &xformSamples = node.samples().schemaSamples;
            
            if (updated || fabs(xformSamples.lastEvaluationTime - mTime) > 0.0001)
            {
               TimeSampleList<Alembic::AbcGeom::IXformSchema>::ConstIterator samp0, samp1;
               double blend = 0.0;
               
               if (xformSamples.getSamples(mTime, samp0, samp1, blend))
               {
                  if (blend > 0.0)
                  {
                     if (samp0->data().getInheritsXforms() != samp1->data().getInheritsXforms())
                     {
                        std::cout << node.path() << ": Animated inherits transform property found, use first sample value" << std::endl;
                        
                        node.setSelfMatrix(samp0->data().getMatrix());
                     }
                     else
                     {
                        node.setSelfMatrix((1.0 - blend) * samp0->data().getMatrix() +
                                           blend * samp1->data().getMatrix());
                     }
                  }
                  else
                  {
                     node.setSelfMatrix(samp0->data().getMatrix());
                  }
                     
                  node.setInheritsTransform(samp0->data().getInheritsXforms());
                  
                  xformSamples.lastEvaluationTime = mTime;
                  
                  noData = false;
               }
            }
            else
            {
               noData = false;
            }
         }
         
         if (noData)
         {
            Alembic::Abc::M44d id;
            
            node.setSelfMatrix(id);
            node.setInheritsTransform(true);
         }
      }
   }
   
   return anyEnter(node, instance);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicMesh &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicSubD &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicPoints &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicCurves &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mIgnoreInstances)
      {
         AlembicNode *m = node.master();
         return m->enter(*this, &node);
      }
      else
      {
         node.resetWorldMatrix();
         node.resetChildBounds();
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}

void WorldUpdate::leave(AlembicXform &node, AlembicNode *)
{
   node.updateChildBounds();
}

void WorldUpdate::leave(AlembicNode &node, AlembicNode *)
{
   if (!node.isInstance() || !mIgnoreInstances)
   {
      node.updateChildBounds();
      
      Alembic::Abc::Box3d bounds;
      
      if (!mIgnoreTransforms)
      {
         bounds = node.childBounds();
      }
      else if (!node.isInstance())
      {
         bounds = node.selfBounds();
      }
      
      if (!bounds.isInfinite())
      {
         mBounds.extendBy(bounds);
      }
   }
}

// ---

GetFrameRange::GetFrameRange(bool ignoreTransforms,
                             bool ignoreInstances,
                             bool ignoreVisibility)
   : mStartFrame(std::numeric_limits<double>::max())
   , mEndFrame(-std::numeric_limits<double>::max())
   , mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
{
}
   
AlembicNode::VisitReturn GetFrameRange::enter(AlembicXform &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      if (!mNoTransforms)
      {
         if (node.isLocator())
         {
            const Alembic::Abc::IScalarProperty &prop = node.locatorProperty();
            
            Alembic::AbcCoreAbstract::TimeSamplingPtr ts = prop.getTimeSampling();
            size_t nsamples = prop.getNumSamples();
            
            if (nsamples > 1)
            {
               mStartFrame = std::min(ts->getSampleTime(0), mStartFrame);
               mEndFrame = std::max(ts->getSampleTime(nsamples-1), mEndFrame);
            }
         }
         else
         {
            updateFrameRange(node);
         }
      }
      
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicMesh &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicSubD &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicPoints &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicCurves &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicNuPatch &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicNode &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      if (node.isInstance())
      {
         if (!mNoInstances)
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
}
   
void GetFrameRange::leave(AlembicNode &, AlembicNode *)
{
}

bool GetFrameRange::getFrameRange(double &start, double &end) const
{
   if (mStartFrame < mEndFrame)
   {
      start = mStartFrame;
      end = mEndFrame;
      return true;
   }
   else
   {
      return false;
   }
}

// ---

CountShapes::CountShapes(bool ignoreInstances, bool ignoreVisibility)
   : mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
   , mCount(0)
{
}

AlembicNode::VisitReturn CountShapes::enter(AlembicXform &node, AlembicNode *)
{
   return ((mCheckVisibility && !node.isVisible()) ? AlembicNode::DontVisitChildren : AlembicNode::ContinueVisit);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicMesh &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicSubD &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicPoints &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicCurves &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mNoInstances)
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
}

// ---

SampleGeometry::SampleGeometry(double t, SceneGeometryData *sceneData)
   : mTime(t)
   , mSceneData(sceneData)
{
}

bool SampleGeometry::hasBeenSampled(AlembicNode &node) const
{
   return (mSampledNodes.find(node.path()) != mSampledNodes.end());
}

void SampleGeometry::setSampled(AlembicNode &node)
{
   mSampledNodes.insert(node.path());
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicXform &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicMesh &node, AlembicNode *)
{
   bool noData = true;
   bool updated = true;
   bool alreadySampled = hasBeenSampled(node);
   
   if (!alreadySampled && node.sampleSchema(mTime, mTime, false, &updated))
   {
      TimeSampleList<Alembic::AbcGeom::IPolyMeshSchema> &samples = node.samples().schemaSamples;
      
      if (updated || fabs(samples.lastEvaluationTime - mTime) > 0.0001)
      {
         // set updated flag to force a MeshData::update even if current state is valid
         updated = true;
         
         TimeSampleList<Alembic::AbcGeom::IPolyMeshSchema>::ConstIterator samp0, samp1;
         double blend = 0.0;
         
         if (samples.getSamples(mTime, samp0, samp1, blend))
         {
            samples.lastEvaluationTime = mTime;
            noData = false;
            setSampled(node);
         }
      }
      else
      {
         noData = false;
         setSampled(node);
      }
   }
   else
   {
      updated = false;
      noData = !alreadySampled;
   }
   
   if (!noData)
   {
      if (mSceneData)
      {
         MeshData *data = mSceneData->mesh(node);
         
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicSubD &node, AlembicNode *)
{
   bool noData = true;
   bool updated = true;
   bool alreadySampled = hasBeenSampled(node);
   
   if (!alreadySampled && node.sampleSchema(mTime, mTime, false, &updated))
   {
      TimeSampleList<Alembic::AbcGeom::ISubDSchema> &samples = node.samples().schemaSamples;
      
      if (updated || fabs(samples.lastEvaluationTime - mTime) > 0.0001)
      {
         // set updated flag to force a MeshData::update even if current state is valid
         updated = true;
         
         TimeSampleList<Alembic::AbcGeom::ISubDSchema>::ConstIterator samp0, samp1;
         double blend = 0.0;
         
         if (samples.getSamples(mTime, samp0, samp1, blend))
         {
            samples.lastEvaluationTime = mTime;
            noData = false;
            setSampled(node);
         }
      }
      else
      {
         noData = false;
         setSampled(node);
      }
   }
   else
   {
      noData = !alreadySampled;
      updated = false;
   }
   
   if (!noData)
   {
      if (mSceneData)
      {
         MeshData *data = mSceneData->mesh(node);
         
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicPoints &node, AlembicNode *)
{
   bool noData = true;
   bool updated = true;
   bool alreadySampled = hasBeenSampled(node);
   
   if (!alreadySampled && node.sampleSchema(mTime, mTime, false, &updated))
   {
      TimeSampleList<Alembic::AbcGeom::IPointsSchema> &samples = node.samples().schemaSamples;
      
      if (updated || fabs(samples.lastEvaluationTime - mTime) > 0.0001)
      {
         // set updated flag to force a MeshData::update even if current state is valid
         updated = true;
         
         TimeSampleList<Alembic::AbcGeom::IPointsSchema>::ConstIterator samp0, samp1;
         double blend = 0.0;
         
         if (samples.getSamples(mTime, samp0, samp1, blend))
         {
            samples.lastEvaluationTime = mTime;
            noData = false;
            setSampled(node);
         }
      }
      else
      {
         noData = false;
         setSampled(node);
      }
   }
   else
   {
      noData = !alreadySampled;
      updated = false;
   }
   
   if (!noData)
   {
      if (mSceneData)
      {
         PointsData *data = mSceneData->points(node);
         
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicCurves &node, AlembicNode *)
{
   bool noData = true;
   bool updated = true;
   bool alreadySampled = hasBeenSampled(node);
   
   if (!alreadySampled && node.sampleSchema(mTime, mTime, false, &updated))
   {
      TimeSampleList<Alembic::AbcGeom::ICurvesSchema> &samples = node.samples().schemaSamples;
      
      if (updated || fabs(samples.lastEvaluationTime - mTime) > 0.0001)
      {
         // set updated flag to force a MeshData::update even if current state is valid
         updated = true;
         
         TimeSampleList<Alembic::AbcGeom::ICurvesSchema>::ConstIterator samp0, samp1;
         double blend = 0.0;
         
         if (samples.getSamples(mTime, samp0, samp1, blend))
         {
            samples.lastEvaluationTime = mTime;
            noData = false;
            setSampled(node);
         }
      }
      else
      {
         noData = false;
         setSampled(node);
      }
   }
   else
   {
      noData = !alreadySampled;
      updated = false;
   }
   
   if (!noData)
   {
      if (mSceneData)
      {
         CurvesData *data = mSceneData->curves(node);
         
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicNuPatch &node, AlembicNode *)
{
   // ToDo
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      return node.master()->enter(*this, &node);
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}
   
void SampleGeometry::leave(AlembicNode &, AlembicNode *)
{
}

// ---

DrawScene::DrawScene(const SceneGeometryData *sceneData,
                     bool ignoreTransforms,
                     bool ignoreInstances,
                     bool ignoreVisibility)
   : mBounds(sceneData == 0)
   , mAsPoints(false)
   , mWireframe(false)
   , mLineWidth(0.0f)
   , mPointWidth(0.0f)
   , mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
   , mCull(false)
   , mSceneData(sceneData)
   , mTransformBounds(false)
   , mLocators(false)
{
}

bool DrawScene::isVisible(AlembicNode &node) const
{
   return (!mCheckVisibility || node.isVisible());
}

bool DrawScene::cull(AlembicNode &node)
{
   if (!mCull)
   {
      return false;
   }
   else
   {
      Alembic::Abc::Box3d bounds = (mNoTransforms ? node.selfBounds() : node.childBounds());
      
      if (bounds.isInfinite())
      {
         return false;
      }
      
      bool culled = (bounds.isEmpty() || mFrustum.isBoxTotallyOutside(bounds));
      
      if (culled)
      {
         #ifdef _DEBUG
         if (!bounds.isEmpty())
         {
            std::cout << "[" << PREFIX_NAME("AbcShape") << "] " << node.path() << " culled" << std::endl;
         }
         #endif
         mCulledNodes.insert(&node);
      }
      
      return culled;
   }
}

bool DrawScene::culled(AlembicNode &node) const
{
   return (mCulledNodes.find(&node) != mCulledNodes.end());
}

void DrawScene::setWorldMatrix(const Alembic::Abc::M44d &world)
{
   mWorldMatrix = world;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicMesh &node, AlembicNode *)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mBounds)
   {
      DrawBox(node.selfBounds(), mAsPoints, (mAsPoints ? mPointWidth : mLineWidth));
   }
   else
   {
      if (mSceneData)
      {
         const MeshData *data = mSceneData->mesh(node);
         if (data)
         {
            if (mAsPoints)
            {
               data->drawPoints(mPointWidth);
            }
            else
            {
               data->draw(mWireframe, mLineWidth);
            }
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "No geometry data for " << node.path() << std::endl;
            #endif
         }
      }
      else
      {
         #ifdef _DEBUG
         std::cout << "No scene geometry data at all" << std::endl;
         #endif
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicSubD &node, AlembicNode *)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mBounds)
   {
      DrawBox(node.selfBounds(), mAsPoints, (mAsPoints ? mPointWidth : mLineWidth));
   }
   else
   {
      if (mSceneData)
      {
         const MeshData *data = mSceneData->mesh(node);
         if (data)
         {
            if (mAsPoints)
            {
               data->drawPoints(mPointWidth);
            }
            else
            {
               data->draw(mWireframe, mLineWidth);
            }
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "No geometry data for " << node.path() << std::endl;
            #endif
         }
      }
      else
      {
         #ifdef _DEBUG
         std::cout << "No scene geometry data at all" << std::endl;
         #endif
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicPoints &node, AlembicNode *)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mBounds)
   {
      DrawBox(node.selfBounds(), mAsPoints, (mAsPoints ? mPointWidth : mLineWidth));
   }
   else
   {
      if (mSceneData)
      {
         const PointsData *data = mSceneData->points(node);
         if (data)
         {
            data->draw(mPointWidth);
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "No geometry data for " << node.path() << std::endl;
            #endif
         }
      }
      else
      {
         #ifdef _DEBUG
         std::cout << "No scene geometry data at all" << std::endl;
         #endif
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicCurves &node, AlembicNode *)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mBounds)
   {
      DrawBox(node.selfBounds(), mAsPoints, (mAsPoints ? mPointWidth : mLineWidth));
   }
   else
   {
      if (mSceneData)
      {
         const CurvesData *data = mSceneData->curves(node);
         if (data)
         {
            if (mAsPoints)
            {
               data->drawPoints(mPointWidth);
            }
            else
            {
               data->draw(mLineWidth);
            }
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "No geometry data for " << node.path() << std::endl;
            #endif
         }
      }
      else
      {
         #ifdef _DEBUG
         std::cout << "No scene geometry data at all" << std::endl;
         #endif
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicNuPatch &node, AlembicNode *)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mBounds)
   {
      DrawBox(node.selfBounds(), mAsPoints, (mAsPoints ? mPointWidth : mLineWidth));
   }
   else
   {
      #ifdef _DEBUG
      std::cout << "No geometry data for patches yet (" << node.path() << ")" << std::endl;
      #endif
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicXform &node, AlembicNode *instance)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (cull(instance ? *instance : node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (node.isLocator())
   {
      if (mLocators)
      {
         DrawLocator(node.locatorPosition(), node.locatorScale(), mLineWidth);
      }
      return AlembicNode::ContinueVisit;
   }
   
   if (mTransformBounds)
   {
      Alembic::Abc::M44d currentMatrix;
      GLdouble currentColor[4] = {0, 0, 0, 1};
      
      bool lightingOn = glIsEnabled(GL_LIGHTING);
      glGetDoublev(GL_MODELVIEW_MATRIX, currentMatrix.getValue());
      glGetDoublev(GL_CURRENT_COLOR, currentColor);
      
      if (lightingOn)
      {
         glDisable(GL_LIGHTING);
      }
      
      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixd(mWorldMatrix.getValue());
      
      glColor3d(0, 0, 0);
      if (!mNoTransforms)
      {
         DrawBox(instance ? instance->childBounds() : node.childBounds(), false, mLineWidth);
      }
      else
      {
         DrawBox(node.selfBounds(), false, mLineWidth);
      }
      
      glLoadMatrixd(currentMatrix.getValue());
      glColor4dv(currentColor);
      
      if (lightingOn)
      {
         glEnable(GL_LIGHTING);
      }
   }
   
   if (!mNoTransforms)
   {
      // Do not use gl matrix stack, won't work with deep hierarchies
      // => Make our own stack
      Alembic::Abc::M44d currentMatrix;
      glGetDoublev(GL_MODELVIEW_MATRIX, currentMatrix.getValue());
      mMatrixStack.push_back(currentMatrix);
      
      glMatrixMode(GL_MODELVIEW);
      if (node.inheritsTransform())
      {
         glMultMatrixd(node.selfMatrix().getValue());
      }
      else
      {
         glLoadMatrixd(node.selfMatrix().getValue());
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawScene::enter(AlembicNode &node, AlembicNode *)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   else if (node.isInstance())
   {
      if (!mNoInstances)
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
 
void DrawScene::leave(AlembicXform &node, AlembicNode *instance)
{
   if (isVisible(node) && !culled(instance ? *instance : node) && !node.isLocator() && !mNoTransforms)
   {
      if (mMatrixStack.size() > 0)
      {
         glMatrixMode(GL_MODELVIEW);
         glLoadMatrixd(mMatrixStack.back().getValue());
         mMatrixStack.pop_back();
      }
   }
}
  
void DrawScene::leave(AlembicNode &node, AlembicNode *)
{
   if (isVisible(node) && node.isInstance() && !mNoInstances)
   {
      node.master()->leave(*this, &node);
   }
}

// ---

PrintInfo::PrintInfo(bool ignoreTransforms,
                     bool ignoreInstances,
                     bool ignoreVisibility)
   : mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
{
}

AlembicNode::VisitReturn PrintInfo::enter(AlembicNode &node, AlembicNode *)
{
   if (!node.isInstance() || !mNoInstances || !mCheckVisibility || node.isVisible())
   {
      std::cout << node.path() << ":" << std::endl;
      std::cout << "  type: " << node.typeName() << std::endl;
      
      std::cout << "  inherits transform: " << node.inheritsTransform() << std::endl;
      std::cout << "  self matrix: " << node.selfMatrix() << std::endl;
      std::cout << "  self bounds: " << node.selfBounds().min << " - " << node.selfBounds().max << std::endl;
      std::cout << "  locator: " << node.isLocator() << std::endl;
      
      if (!mNoTransforms)
      {
         std::cout << "  child bounds: " << node.childBounds().min << " - " << node.childBounds().max << std::endl;
         std::cout << "  world matrix: " << node.worldMatrix() << std::endl;
      }
      
      if (!mNoInstances)
      {
         std::cout << "  instance number: " << node.instanceNumber() << std::endl;
         std::cout << "  master instance: " << node.masterPath() << std::endl;
      }
      
      if (!mCheckVisibility)
      {
         std::cout << "  visibility: " << node.isVisible() << std::endl;
      }
      
      std::cout << std::endl;
      
      return AlembicNode::ContinueVisit;
   }
   else
   {
      return AlembicNode::DontVisitChildren;
   }
}

void PrintInfo::leave(AlembicNode &, AlembicNode *)
{
}
