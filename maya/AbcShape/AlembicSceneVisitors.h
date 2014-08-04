#ifndef ABCSHAPE_ALEMBICSCENEVISITORS_H_
#define ABCSHAPE_ALEMBICSCENEVISITORS_H_

#include "AlembicScene.h"
#include "GeometryData.h"
#include "MathUtils.h"

#include <set>
#include <string>


class WorldUpdate
{
public:
   
   WorldUpdate(double t);
   
   AlembicNode::VisitReturn enter(AlembicXform &node);
   AlembicNode::VisitReturn enter(AlembicMesh &node);
   AlembicNode::VisitReturn enter(AlembicSubD &node);
   AlembicNode::VisitReturn enter(AlembicPoints &node);
   AlembicNode::VisitReturn enter(AlembicCurves &node);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node);
   AlembicNode::VisitReturn enter(AlembicNode &node);
      
   void leave(AlembicNode &node);
   
private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node)
   {
      bool updated = true;
      
      if (node.sampleBounds(mTime, &updated))
      {
         if (updated)
         {
            typename AlembicNodeT<T>::Sample &samp0 = node.firstSample();
            typename AlembicNodeT<T>::Sample &samp1 = node.secondSample();
            
            if (samp1.boundsWeight > 0.0)
            {
               
               Alembic::Abc::Box3d b0 = samp0.bounds;
               Alembic::Abc::Box3d b1 = samp1.bounds;
               
               node.setSelfBounds(Alembic::Abc::Box3d(samp0.boundsWeight * b0.min + samp1.boundsWeight * b1.min,
                                                      samp0.boundsWeight * b0.max + samp1.boundsWeight * b1.max));
            }
            else
            {
               node.setSelfBounds(samp0.bounds);
            }
         }
      }
      else
      {
         Alembic::Abc::Box3d box;
         node.setSelfBounds(box);
      }
      
      return anyEnter(node);
   }
   
   template <class T> 
   AlembicNode::VisitReturn anyEnter(AlembicNodeT<T> &node)
   {
      Alembic::Util::bool_t visible = true;
      
      Alembic::Abc::ICompoundProperty props = node.object().getProperties();
      
      if (props.valid())
      {
         const Alembic::Abc::PropertyHeader *header = props.getPropertyHeader("visible");
         
         if (header)
         {
            if (Alembic::Abc::ICharProperty::matches(*header))
            {
               Alembic::Abc::ICharProperty prop(props, "visible");
               
               Alembic::Util::int8_t v = 1;
               
               prop.get(v, Alembic::Abc::ISampleSelector(node.getSampleIndex(mTime, prop)));
               
               visible = (v != 0);
            }
            else if (Alembic::Abc::IBoolProperty::matches(*header))
            {
               Alembic::Abc::IBoolProperty prop(props, "visible");
               
               prop.get(visible, Alembic::Abc::ISampleSelector(node.getSampleIndex(mTime, prop)));
            }
         }
      }
      
      node.setVisible(visible);
      
      node.updateWorldMatrix();
         
      return AlembicNode::ContinueVisit;
   }

private:

   double mTime;
};


class CountShapes
{
public:
   
   CountShapes(bool ignoreInstances, bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node);
   AlembicNode::VisitReturn enter(AlembicMesh &node);
   AlembicNode::VisitReturn enter(AlembicSubD &node);
   AlembicNode::VisitReturn enter(AlembicPoints &node);
   AlembicNode::VisitReturn enter(AlembicCurves &node);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node);
   AlembicNode::VisitReturn enter(AlembicNode &node);
   
   void leave(AlembicNode &node);
   
   inline unsigned int count() const
   {
      return mCount;
   }

private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node)
   {
      if (mCheckVisibility && !node.isVisible())
      {
         return AlembicNode::DontVisitChildren;
      }
      else
      {
         ++mCount;
         return AlembicNode::ContinueVisit;
      }
   }

private:
   
   bool mNoInstances;
   bool mCheckVisibility;
   unsigned int mCount;
};


class ComputeSceneBounds
{
public:
   
   inline ComputeSceneBounds(bool ignoreTransforms,
                             bool ignoreInstances,
                             bool ignoreVisibility)
      : mNoTransforms(ignoreTransforms)
      , mNoInstances(ignoreInstances)
      , mCheckVisibility(!ignoreVisibility)
   {
   }
   
   inline AlembicNode::VisitReturn enter(AlembicXform &node)
   {
      // don't merge in transform bounds (childBounds only)
      if (mCheckVisibility && !node.isVisible())
      {
         return AlembicNode::DontVisitChildren;
      }
      else
      {
         return AlembicNode::ContinueVisit;
      }
   }
   
   inline AlembicNode::VisitReturn enter(AlembicNode &node)
   {
      if (mCheckVisibility && !node.isVisible())
      {
         return AlembicNode::DontVisitChildren;
      }
      else
      {
         Alembic::Abc::Box3d bounds;
         
         if (!mNoTransforms)
         {
            if (!node.isInstance() || !mNoInstances)
            {
               bounds = node.childBounds();
            }
         }
         else if (!node.isInstance())
         {
            // without no transforms, all instances have the same bounds
            bounds = node.selfBounds();
         }
         
         if (!bounds.isEmpty() && !bounds.isInfinite())
         {
            mBounds.extendBy(bounds);
         }
         
         return AlembicNode::ContinueVisit;
      }
   }
   
   inline void leave(AlembicNode &)
   {
   }
   
   inline const Alembic::Abc::Box3d bounds() const
   {
      return mBounds;
   }
   
private:
   
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   Alembic::Abc::Box3d mBounds;
};


class SampleGeometry
{
public:
   
   SampleGeometry(double t, SceneGeometryData *sceneData);
   
   AlembicNode::VisitReturn enter(AlembicXform &node);
   AlembicNode::VisitReturn enter(AlembicMesh &node);
   AlembicNode::VisitReturn enter(AlembicSubD &node);
   AlembicNode::VisitReturn enter(AlembicPoints &node);
   AlembicNode::VisitReturn enter(AlembicCurves &node);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node);
   AlembicNode::VisitReturn enter(AlembicNode &node);
      
   void leave(AlembicNode &);
   
private:

   bool hasBeenSampled(AlembicNode &node) const;
   void setSampled(AlembicNode &node);

private:
   
   double mTime;
   std::set<std::string> mSampledNodes;
   SceneGeometryData *mSceneData;
};


class DrawGeometry
{
public:
   
   DrawGeometry(const SceneGeometryData *sceneData,
                bool ignoreTransforms,
                bool ignoreInstances,
                bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node);
   AlembicNode::VisitReturn enter(AlembicMesh &node);
   AlembicNode::VisitReturn enter(AlembicSubD &node);
   AlembicNode::VisitReturn enter(AlembicPoints &node);
   AlembicNode::VisitReturn enter(AlembicCurves &node);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node);
   AlembicNode::VisitReturn enter(AlembicNode &node);
   
   void leave(AlembicXform &node);
   void leave(AlembicNode &);
   
   inline void drawAsPoints(bool on) { mAsPoints = on; }
   inline void drawWireframe(bool on) { mWireframe = on; }
   inline void setLineWidth(float w) { mLineWidth = w; }
   inline void setPointWidth(float w) { mPointWidth = w; }
   
private:
   
   bool mAsPoints;
   bool mWireframe;
   float mLineWidth;
   float mPointWidth;
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   const SceneGeometryData *mSceneData;
   std::deque<Alembic::Abc::M44d> mMatrixStack;
};


class DrawBounds
{
public:
   
   DrawBounds(bool ignoreTransforms,
              bool ignoreInstances,
              bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node);
   AlembicNode::VisitReturn enter(AlembicNode &node);
   
   void leave(AlembicXform &node);
   void leave(AlembicNode &);
   
   inline void drawAsPoints(bool on) { mAsPoints = on; }
   inline void setWidth(float w) { mWidth = w; }
   
private:
   
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   float mWidth;
   bool mAsPoints;
   std::deque<Alembic::Abc::M44d> mMatrixStack;
};


class Select
{
public:
   
   Select(const SceneGeometryData *scene,
          const Frustum &frustum,
          bool ignoreTransforms,
          bool ignoreInstances,
          bool ignoreVisibility);
   
   inline void setWidth(float w) { mWidth = w; }
   inline void drawAsPoints(bool on) { mAsPoints = on; }
   inline void drawWireframe(bool on) { mWireframe = on; }
   inline void drawBounds(bool on) { mBounds = on; }
   
   AlembicNode::VisitReturn enter(AlembicNode &node);
   void leave(AlembicNode &node);
   
private:
   
   bool isVisible(AlembicNode &node) const;
   bool inFrustum(AlembicNode &node) const;

private:
   
   const SceneGeometryData *mScene;
   Frustum mFrustum;
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   bool mBounds;
   bool mAsPoints;
   bool mWireframe;
   float mWidth;
   std::deque<Alembic::Abc::M44d> mMatrixStack;
   std::set<AlembicNode*> mProcessedXforms;
};


class PrintInfo
{
public:
   
   PrintInfo(bool ignoreTransforms,
             bool ignoreInstances,
             bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicNode &node);
   void leave(AlembicNode &node);

private:
   
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
};

#endif
