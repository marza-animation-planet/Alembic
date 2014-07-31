#include "AlembicSceneVisitors.h"

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

#include <iostream>


WorldUpdate::WorldUpdate(double t)
   : mTime(t)
{
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicXform &node)
{
   bool updated = true;
   
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
   
   return anyEnter(node);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicMesh &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicSubD &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicPoints &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicCurves &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicNuPatch &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn WorldUpdate::enter(AlembicNode &node)
{
   AlembicNode::VisitReturn rv = AlembicNode::ContinueVisit;
   
   if (node.isInstance())
   {
      AlembicNode *m = node.master();
      rv = m->enter(*this);
   }
   
   node.updateWorldMatrix();
   
   return rv;
}
   
void WorldUpdate::leave(AlembicNode &node)
{
   node.updateChildBounds();
}

// ---

CountShapes::CountShapes(bool ignoreInstances, bool ignoreVisibility)
   : mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
   , mCount(0)
{
}

AlembicNode::VisitReturn CountShapes::enter(AlembicXform &node)
{
   return ((mCheckVisibility && !node.isVisible()) ? AlembicNode::DontVisitChildren : AlembicNode::ContinueVisit);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicMesh &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicSubD &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicPoints &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicCurves &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicNuPatch &node)
{
   return shapeEnter(node);
}

AlembicNode::VisitReturn CountShapes::enter(AlembicNode &node)
{
   if (node.isInstance())
   {
      if (!mNoInstances)
      {
         return node.master()->enter(*this);
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

void CountShapes::leave(AlembicNode &)
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

AlembicNode::VisitReturn SampleGeometry::enter(AlembicXform &)
{
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicMesh &node)
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

AlembicNode::VisitReturn SampleGeometry::enter(AlembicSubD &node)
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

AlembicNode::VisitReturn SampleGeometry::enter(AlembicPoints &node)
{
   if (!hasBeenSampled(node) && node.sampleData(mTime))
   {
      // ToDo
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicCurves &node)
{
   if (!hasBeenSampled(node) && node.sampleData(mTime))
   {
      // ToDo
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicNuPatch &node)
{
   if (!hasBeenSampled(node) && node.sampleData(mTime))
   {
      // ToDo
      setSampled(node);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleGeometry::enter(AlembicNode &node)
{
   if (node.isInstance())
   {
      return node.master()->enter(*this);
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}
   
void SampleGeometry::leave(AlembicNode &)
{
}

// ---

DrawGeometry::DrawGeometry(const SceneGeometryData *sceneData, bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility)
   : mAsPoints(false)
   , mWireframe(false)
   , mWidth(0.0f)
   , mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
   , mSceneData(sceneData)
{
   // line width ?
   // point width ?
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicMesh &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mSceneData)
   {
      const MeshData *data = mSceneData->mesh(node);
      if (data)
      {
         if (mAsPoints)
         {
            data->drawPoints(mWidth);
         }
         else
         {
            data->draw(mWireframe, mWidth);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicSubD &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   
   if (mSceneData)
   {
      const MeshData *data = mSceneData->mesh(node);
      if (data)
      {
         if (mAsPoints)
         {
            data->drawPoints(mWidth);
         }
         else
         {
            data->draw(mWireframe, mWidth);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicPoints &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // ToDo
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicCurves &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // ToDo
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicNuPatch &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   
   // ToDo
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn DrawGeometry::enter(AlembicXform &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else if (!mNoTransforms)
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

AlembicNode::VisitReturn DrawGeometry::enter(AlembicNode &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else if (node.isInstance())
   {
      if (!mNoInstances)
      {
         return node.master()->enter(*this);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   
   return AlembicNode::ContinueVisit;
}
 
void DrawGeometry::leave(AlembicXform &node)
{
   if (!mNoTransforms && (!mCheckVisibility || node.isVisible()))
   {
      if (mMatrixStack.size() > 0)
      {
         glMatrixMode(GL_MODELVIEW);
         glLoadMatrixd(mMatrixStack.back().getValue());
         mMatrixStack.pop_back();
      }
   }
}
  
void DrawGeometry::leave(AlembicNode &node)
{
   if (node.isInstance() && !mNoInstances)
   {
      node.master()->leave(*this);
   }
}

// ---

DrawBounds::DrawBounds(bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility)
   : mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
   , mWidth(0.0f)
   , mAsPoints(false)
{
}

AlembicNode::VisitReturn DrawBounds::enter(AlembicXform &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else if (!mNoTransforms)
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

AlembicNode::VisitReturn DrawBounds::enter(AlembicNode &node)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else if (node.isInstance())
   {
      if (!mNoInstances)
      {
         return node.master()->enter(*this);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      DrawBox(node.selfBounds(), mAsPoints, mWidth);
      return AlembicNode::ContinueVisit;
   }
}
 
void DrawBounds::leave(AlembicXform &node)
{
   if (!mNoTransforms && (!mCheckVisibility || node.isVisible()))
   {
      if (mMatrixStack.size() > 0)
      {
         glMatrixMode(GL_MODELVIEW);
         glLoadMatrixd(mMatrixStack.back().getValue());
         mMatrixStack.pop_back();
      }
   }
}
  
void DrawBounds::leave(AlembicNode &node)
{
   if (node.isInstance() && !mNoInstances)
   {
      node.master()->leave(*this);
   }
}

// ---

Select::Select(const SceneGeometryData *scene,
               const Frustum &frustum,
               bool ignoreTransforms,
               bool ignoreInstances,
               bool ignoreVisibility)
   : mScene(scene)
   , mFrustum(frustum)
   , mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
   , mBounds(false)
   , mAsPoints(false)
   , mWireframe(false)
   , mWidth(0.0f)
{
}

AlembicNode::VisitReturn Select::enter(AlembicNode &node)
{
   if ((node.isInstance() && mNoInstances) ||
       !isVisible(node) ||
       !inFrustum(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   
   AlembicNode *master = (node.isInstance() ? node.master() : &node);
      
   switch (master->type())
   {
   case AlembicNode::TypeMesh:
   case AlembicNode::TypeSubD:
      {
         if (mBounds)
         {
            DrawBox(master->selfBounds(), mAsPoints, mWidth);
         }
         else
         {
            const MeshData *data = mScene->mesh(*master);
            if (data)
            {
               if (mAsPoints)
               {
                  data->drawPoints(mWidth);
               }
               else
               {
                  data->draw(mWireframe, mWidth);
               }
            }
         }
      }
      break;
   case AlembicNode::TypePoints:
   case AlembicNode::TypeCurves:
   case AlembicNode::TypeNuPatch:
      // Not yet supported
      break;
   case AlembicNode::TypeXform:
      {
         if (!mNoTransforms)
         {
            Alembic::Abc::M44d currentMatrix;
            glGetDoublev(GL_MODELVIEW_MATRIX, currentMatrix.getValue());
            mMatrixStack.push_back(currentMatrix);
            
            glMatrixMode(GL_MODELVIEW);
            if (master->inheritsTransform())
            {
               glMultMatrixd(master->selfMatrix().getValue());
            }
            else
            {
               glLoadMatrixd(master->selfMatrix().getValue());
            }
            
            mProcessedXforms.insert(&node);
         }
      }
      break;
   case AlembicNode::TypeGeneric:
      // Unsupported type
   default:
      break;
   }
   
   return AlembicNode::ContinueVisit;
}

void Select::leave(AlembicNode &node)
{
   std::set<AlembicNode*>::iterator it = mProcessedXforms.find(&node);
   
   if (it != mProcessedXforms.end())
   {
      if (mMatrixStack.size() > 0)
      {
         glMatrixMode(GL_MODELVIEW);
         glLoadMatrixd(mMatrixStack.back().getValue());
         mMatrixStack.pop_back();
      }
      
      mProcessedXforms.erase(it);
   }
}

bool Select::isVisible(AlembicNode &node) const
{
   return (!mCheckVisibility || node.isVisible());
}

bool Select::inFrustum(AlembicNode &node) const
{
   Alembic::Abc::Box3d bounds;
   
   if (!mNoTransforms)
   {
      bounds = node.childBounds();
   }
   else
   {
      if (node.isInstance())
      {
         if (node.master()->type() == AlembicNode::TypeXform)
         {
            return true;
         }
      }
      else if (node.type() == AlembicNode::TypeXform)
      {
         return true;
      }
      
      bounds = node.selfBounds();
   }
   
   return (!bounds.isEmpty() && !mFrustum.isBoxTotallyOutside(bounds));
}


// ---

PrintInfo::PrintInfo(bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility)
   : mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
{
}

AlembicNode::VisitReturn PrintInfo::enter(AlembicNode &node)
{
   if (!node.isInstance() || !mNoInstances || !mCheckVisibility || node.isVisible())
   {
      std::cout << node.path() << ":" << std::endl;
      std::cout << "  type: " << node.typeName() << std::endl;
      
      std::cout << "  inherits transform: " << node.inheritsTransform() << std::endl;
      std::cout << "  self matrix: " << node.selfMatrix() << std::endl;
      std::cout << "  self bounds: " << node.selfBounds().min << " - " << node.selfBounds().max << std::endl;
      
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

void PrintInfo::leave(AlembicNode &)
{
}
