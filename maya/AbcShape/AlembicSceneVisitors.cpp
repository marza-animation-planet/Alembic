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
   if (node.isInstance())
   {
      AlembicNode *m = node.master();
      m->enter(*this);
   }
   
   node.updateWorldMatrix();
   
   return AlembicNode::ContinueVisit;
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
         node.master()->enter(*this);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   
   return AlembicNode::ContinueVisit;
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
      #ifdef _DEBUG
      std::cout << "Valid sample for mesh data (updated: " << updated << ")" << std::endl;
      #endif
      
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
      #ifdef _DEBUG
      std::cout << "Valid sample for mesh data (updated: " << updated << ")" << std::endl;
      #endif
      
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
      node.master()->enter(*this);
   }
   
   return AlembicNode::ContinueVisit;
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
         node.master()->enter(*this);
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
{
}

void DrawBounds::DrawBox(const Alembic::Abc::Box3d &bounds, float lineWidth)
{
   if (bounds.isEmpty() || bounds.isInfinite())
   {
      return;
   }
   
   if (lineWidth > 0.0f)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(lineWidth);
   }
   
   float min_x = bounds.min[0];
   float min_y = bounds.min[1];
   float min_z = bounds.min[2];
   float max_x = bounds.max[0];
   float max_y = bounds.max[1];
   float max_z = bounds.max[2];

   float w = max_x - min_x;
   float h = max_y - min_y;
   float d = max_z - min_z;

   glBegin(GL_LINES);

   glVertex3f(min_x, min_y, min_z);
   glVertex3f(min_x+w, min_y, min_z);
   glVertex3f(min_x, min_y, min_z);
   glVertex3f(min_x, min_y+h, min_z);
   glVertex3f(min_x, min_y, min_z);
   glVertex3f(min_x, min_y, min_z+d);
   glVertex3f(min_x+w, min_y, min_z);
   glVertex3f(min_x+w, min_y+h, min_z);
   glVertex3f(min_x+w, min_y+h, min_z);
   glVertex3f(min_x, min_y+h, min_z);
   glVertex3f(min_x, min_y, min_z+d);
   glVertex3f(min_x+w, min_y, min_z+d);
   glVertex3f(min_x+w, min_y, min_z+d);
   glVertex3f(min_x+w, min_y, min_z);
   glVertex3f(min_x, min_y, min_z+d);
   glVertex3f(min_x, min_y+h, min_z+d);
   glVertex3f(min_x, min_y+h, min_z+d);
   glVertex3f(min_x, min_y+h, min_z);
   glVertex3f(min_x+w, min_y+h, min_z);
   glVertex3f(min_x+w, min_y+h, min_z+d);
   glVertex3f(min_x+w, min_y, min_z+d);
   glVertex3f(min_x+w, min_y+h, min_z+d);
   glVertex3f(min_x, min_y+h, min_z+d);
   glVertex3f(min_x+w, min_y+h, min_z+d);

   float cx = bounds.center()[0];
   float cy = bounds.center()[1];
   float cz = bounds.center()[2];

   glVertex3f(cx-0.1, cy, cz);
   glVertex3f(cx+0.1, cy, cz);
   glVertex3f(cx, cy-0.1, cz);
   glVertex3f(cx, cy+0.1, cz);
   glVertex3f(cx, cy, cz-0.1);
   glVertex3f(cx, cy, cz+0.1);

   glEnd();
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
         node.master()->enter(*this);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      DrawBox(node.selfBounds(), mWidth);
   }
      
   return AlembicNode::ContinueVisit;
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
