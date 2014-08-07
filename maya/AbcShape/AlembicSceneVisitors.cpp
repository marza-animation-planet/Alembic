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
{
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicXform &node, AlembicNode *instance)
{
   bool updated = true;
   
   if (node.isLocator())
   {
      if (node.sampleBounds(mTime, &updated))
      {
         if (updated)
         {
            AlembicXform::Sample &samp0 = node.firstSample();
            AlembicXform::Sample &samp1 = node.secondSample();
            
            if (samp1.dataWeight > 0.0)
            {
               Alembic::Abc::V3d v0, v1;
               
               v0.setValue(samp0.locator[0], samp0.locator[1], samp0.locator[2]);
               v1.setValue(samp1.locator[0], samp1.locator[1], samp1.locator[2]);
               
               node.setLocatorPosition(samp0.dataWeight * v0 + samp1.dataWeight * v1);
               
               v0.setValue(samp0.locator[3], samp0.locator[4], samp0.locator[5]);
               v1.setValue(samp1.locator[3], samp1.locator[4], samp1.locator[5]);
               
               node.setLocatorScale(samp0.dataWeight * v0 + samp1.dataWeight * v1);
            }
            else
            {
               node.setLocatorPosition(Alembic::Abc::V3d(samp0.locator[0], samp0.locator[1], samp0.locator[2]));
               node.setLocatorScale(Alembic::Abc::V3d(samp0.locator[3], samp0.locator[4], samp0.locator[5]));
            }
         }
      }
      else
      {
         node.setLocatorPosition(Alembic::Abc::V3d(0, 0, 0));
         node.setLocatorScale(Alembic::Abc::V3d(1, 1, 1));
      }
   }
   else
   {
      if (!mIgnoreTransforms)
      {
         if (node.sampleBounds(mTime, &updated))
         {
            if (updated)
            {
               AlembicXform::Sample &samp0 = node.firstSample();
               AlembicXform::Sample &samp1 = node.secondSample();
               
               if (samp1.dataWeight > 0.0)
               {
                  if (samp0.data.getInheritsXforms() != samp1.data.getInheritsXforms())
                  {
                     std::cout << node.path() << ": Animated inherits transform property found, use first sample value" << std::endl;
                     node.setSelfMatrix(samp0.data.getMatrix());
                  }
                  else
                  {
                     node.setSelfMatrix(samp0.dataWeight * samp0.data.getMatrix() +
                                        samp1.dataWeight * samp1.data.getMatrix());
                  }
               }
               else
               {
                  node.setSelfMatrix(samp0.data.getMatrix());
               }
                  
               node.setInheritsTransform(samp0.data.getInheritsXforms());
            }
         }
         else
         {
            Alembic::Abc::M44d id;
            node.setSelfMatrix(id);
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
   AlembicNode::VisitReturn rv = AlembicNode::ContinueVisit;
   
   if (node.isInstance())
   {
      if (!mIgnoreInstances)
      {
         AlembicNode *m = node.master();
         rv = m->enter(*this, &node);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   
   node.updateWorldMatrix();
   
   return rv;
}
   
void WorldUpdate::leave(AlembicNode &node, AlembicNode *)
{
   if (!node.isInstance() || !mIgnoreInstances)
   {
      node.updateChildBounds();
   }
}

// ---

GetFrameRange::GetFrameRange()
   : mStartFrame(std::numeric_limits<double>::max())
   , mEndFrame(-std::numeric_limits<double>::max())
{
}
   
AlembicNode::VisitReturn GetFrameRange::enter(AlembicXform &node, AlembicNode *instance)
{
   if (!instance)
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

AlembicNode::VisitReturn GetFrameRange::enter(AlembicMesh &node, AlembicNode *instance)
{
   if (!instance)
   {
      updateFrameRange(node);
   }
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicSubD &node, AlembicNode *instance)
{
   if (!instance)
   {
      updateFrameRange(node);
   }
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicPoints &node, AlembicNode *instance)
{
   if (!instance)
   {
      updateFrameRange(node);
   }
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicCurves &node, AlembicNode *instance)
{
   if (!instance)
   {
      updateFrameRange(node);
   }
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   if (!instance)
   {
      updateFrameRange(node);
   }
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicNode &, AlembicNode *)
{
   return AlembicNode::ContinueVisit;
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
   bool updated = true;
   
   if (!hasBeenSampled(node) && node.sampleData(mTime, &updated))
   {
      if (mSceneData)
      {
         MeshData *data = mSceneData->mesh(node);
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
      
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicSubD &node, AlembicNode *)
{
   bool updated = true;
   
   if (!hasBeenSampled(node) && node.sampleData(mTime, &updated))
   {
      if (mSceneData)
      {
         MeshData *data = mSceneData->mesh(node);
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
      
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicPoints &node, AlembicNode *)
{
   bool updated = true;
   
   if (!hasBeenSampled(node) && node.sampleData(mTime, &updated))
   {
      if (mSceneData)
      {
         PointsData *data = mSceneData->points(node);
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
      
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicCurves &node, AlembicNode *)
{
   bool updated = true;
   
   if (!hasBeenSampled(node) && node.sampleData(mTime, &updated))
   {
      if (mSceneData)
      {
         CurvesData *data = mSceneData->curves(node);
         if (data && (!data->isValid() || updated))
         {
            data->update(node);
         }
      }
      
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicNuPatch &node, AlembicNode *)
{
   if (!hasBeenSampled(node) && node.sampleData(mTime))
   {
      // ToDo
      setSampled(node);
   }
   
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

DrawGeometry::DrawGeometry(const SceneGeometryData *sceneData,
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

bool DrawGeometry::isVisible(AlembicNode &node) const
{
   return (!mCheckVisibility || node.isVisible());
}

bool DrawGeometry::cull(AlembicNode &node)
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
            std::cout << "[AbcShape] " << node.path() << " culled" << std::endl;
         }
         #endif
         mCulledNodes.insert(&node);
      }
      
      return culled;
   }
}

bool DrawGeometry::culled(AlembicNode &node) const
{
   return (mCulledNodes.find(&node) != mCulledNodes.end());
}

void DrawGeometry::setViewMatrix(const Alembic::Abc::M44d &view)
{
   mViewMatrixInv = view.inverse();
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicMesh &node, AlembicNode *)
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicSubD &node, AlembicNode *)
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicPoints &node, AlembicNode *)
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicCurves &node, AlembicNode *)
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicNuPatch &node, AlembicNode *)
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicXform &node, AlembicNode *instance)
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
      glLoadMatrixd(mViewMatrixInv.getValue());
      
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicNode &node, AlembicNode *)
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
 
void DrawGeometry::leave(AlembicXform &node, AlembicNode *instance)
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
  
void DrawGeometry::leave(AlembicNode &node, AlembicNode *)
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
