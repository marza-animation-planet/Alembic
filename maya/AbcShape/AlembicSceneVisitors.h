#ifndef ABCSHAPE_ALEMBICSCENEVISITORS_H_
#define ABCSHAPE_ALEMBICSCENEVISITORS_H_

#include "common.h"

#include <set>
#include <string>


class WorldUpdate
{
public:
   
   WorldUpdate(double t,
               bool ignoreTransforms,
               bool ignoreInstances,
               bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicXform &node, AlembicNode *instance=0);
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   const Alembic::Abc::Box3d& bounds() const;
   unsigned int numShapes(bool visibleOnly) const;
   
private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
   template <class T> 
   AlembicNode::VisitReturn anyEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);

private:

   double mTime;
   bool mIgnoreTransforms;
   bool mIgnoreInstances;
   bool mIgnoreVisibility;
   Alembic::Abc::Box3d mBounds;
   unsigned int mNumShapes;
   unsigned int mNumVisibleShapes;
   std::vector<bool> mVisibilityStack;
};

inline const Alembic::Abc::Box3d& WorldUpdate::bounds() const
{
   return mBounds;
}

inline unsigned int WorldUpdate::numShapes(bool visibleOnly) const
{
   return (visibleOnly ? mNumVisibleShapes : mNumShapes);
}

template <class T>
AlembicNode::VisitReturn WorldUpdate::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   bool noData = true;
   bool updated = true;
   
   TimeSampleList<Alembic::Abc::IBox3dProperty> &boundsSamples = node.samples().boundsSamples;
   
   if (node.sampleBounds(mTime, mTime, false, &updated))
   {
      if (updated || fabs(boundsSamples.lastEvaluationTime - mTime) > 0.0001)
      {
         TimeSampleList<Alembic::Abc::IBox3dProperty>::ConstIterator samp0, samp1;
         double blend = 0.0;
         
         if (boundsSamples.getSamples(mTime, samp0, samp1, blend))
         {
            if (blend > 0.0)
            {
               Alembic::Abc::Box3d b0 = samp0->data();
               Alembic::Abc::Box3d b1 = samp1->data();
               
               node.setSelfBounds(Alembic::Abc::Box3d((1.0 - blend) * b0.min + blend * b1.min,
                                                      (1.0 - blend) * b0.max + blend * b1.max));
            }
            else
            {
               node.setSelfBounds(samp0->data());
            }
            
            noData = false;
            
            boundsSamples.lastEvaluationTime = mTime;
         }
      }
      else
      {
         noData = false;
      }
   }
   
   if (noData)
   {
      Alembic::Abc::Box3d box;
      node.setSelfBounds(box);
   }
   
   return anyEnter(node, instance);
}

template <class T> 
AlembicNode::VisitReturn WorldUpdate::anyEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   Alembic::Util::bool_t visible = true;
   
   if (!mIgnoreVisibility)
   {
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
   }
   
   node.setVisible(visible);
   
   if (!instance)
   {
      node.updateWorldMatrix();
   }
   else
   {
      instance->updateWorldMatrix();
   }
   
   if (node.type() != AlembicNode::TypeXform)
   {
      ++mNumShapes;
      
      // Shape is visible if:
      //   we ignore visibility information
      //   OR
      //   the shape itself is visible AND EITHER we ignore transforms OR its parent (if any) is visible too
      if (mIgnoreVisibility)
      {
         ++mNumVisibleShapes;
      }
      else if (visible)
      {
         if (mIgnoreTransforms)
         {
            ++mNumVisibleShapes;
         }
         else if (mVisibilityStack.size() == 0)
         {
            ++mNumVisibleShapes;
         }
         else if (mVisibilityStack.back())
         {
            ++mNumVisibleShapes;
         }
      }
   }
   else
   {
      if (!mIgnoreTransforms && !mIgnoreVisibility)
      {
         if (mVisibilityStack.size() == 0)
         {
            mVisibilityStack.push_back(visible);
         }
         else
         {
            mVisibilityStack.push_back(mVisibilityStack.back() && visible);
         }
      }
   }
   
   return AlembicNode::ContinueVisit;
}

// ---

class GetFrameRange
{
public:
   
   GetFrameRange(bool ignoreTransforms,
                 bool ignoreInstances,
                 bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &node, AlembicNode *instance=0);

   bool getFrameRange(double &start, double &end) const;

private:
   
   template <class T>
   void updateFrameRange(AlembicNodeT<T> &node);

private:
   
   double mStartFrame;
   double mEndFrame;
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
};

template <class T>
void GetFrameRange::updateFrameRange(AlembicNodeT<T> &node)
{
   Alembic::AbcCoreAbstract::TimeSamplingPtr ts = node.typedObject().getSchema().getTimeSampling();
   size_t nsamples = node.typedObject().getSchema().getNumSamples();
   if (nsamples > 1)
   {
      mStartFrame = std::min(ts->getSampleTime(0), mStartFrame);
      mEndFrame = std::max(ts->getSampleTime(nsamples-1), mEndFrame);
   }
}

// ---

class CountShapes
{
public:
   
   // Note: doesn't count locators as shapes
   
   CountShapes(bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   unsigned int count() const;

private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);

private:
   
   bool mIgnoreTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   unsigned int mCount;
};

inline unsigned int CountShapes::count() const
{
   return mCount;
}

template <class T>
AlembicNode::VisitReturn CountShapes::shapeEnter(AlembicNodeT<T> &node, AlembicNode *)
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

// ---

class ComputeSceneBounds
{
public:
   
   ComputeSceneBounds(bool ignoreTransforms,
                      bool ignoreInstances,
                      bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicNode &, AlembicNode *instance=0);
   
   const Alembic::Abc::Box3d bounds() const;
   
private:
   
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   Alembic::Abc::Box3d mBounds;
};

inline ComputeSceneBounds::ComputeSceneBounds(bool ignoreTransforms,
                                              bool ignoreInstances,
                                              bool ignoreVisibility)
   : mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
{
}

inline AlembicNode::VisitReturn ComputeSceneBounds::enter(AlembicXform &node, AlembicNode *)
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

inline AlembicNode::VisitReturn ComputeSceneBounds::enter(AlembicNode &node, AlembicNode *)
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
         // without transforms, all instances have the same bounds
         bounds = node.selfBounds();
      }
      
      if (!bounds.isEmpty() && !bounds.isInfinite())
      {
         mBounds.extendBy(bounds);
      }
      
      return AlembicNode::ContinueVisit;
   }
}

inline void ComputeSceneBounds::leave(AlembicNode &, AlembicNode *)
{
}

inline const Alembic::Abc::Box3d ComputeSceneBounds::bounds() const
{
   return mBounds;
}

// ---

class SampleGeometry
{
public:
   
   SampleGeometry(double t, SceneGeometryData *sceneData);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &, AlembicNode *instance=0);
   
private:

   bool hasBeenSampled(AlembicNode &node) const;
   void setSampled(AlembicNode &node);

private:
   
   double mTime;
   std::set<std::string> mSampledNodes;
   SceneGeometryData *mSceneData;
};

// ---

class DrawScene
{
public:
   
   DrawScene(const SceneGeometryData *sceneData,
             bool ignoreTransforms,
             bool ignoreInstances,
             bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicXform &node, AlembicNode *instance=0);
   void leave(AlembicNode &, AlembicNode *instance=0);
   
   inline void drawBounds(bool on) { if (!on || mSceneData) mBounds = on; }
   inline void drawAsPoints(bool on) { mAsPoints = on; }
   inline void drawWireframe(bool on) { mWireframe = on; }
   inline void setLineWidth(float w) { mLineWidth = w; }
   inline void setPointWidth(float w) { mPointWidth = w; }
   inline void doCull(const Frustum &f) { mCull = true; mFrustum = f; }
   inline void dontCull() { mCull = false; }
   inline void drawTransformBounds(bool on, const Alembic::Abc::M44d &world) { mTransformBounds = on; setWorldMatrix(world); }
   inline void drawLocators(bool on) { mLocators = on; }
   void setWorldMatrix(const Alembic::Abc::M44d &world);
   
private:
   
   bool isVisible(AlembicNode &node) const;
   bool cull(AlembicNode &node);
   bool culled(AlembicNode &node) const;
   
private:
   
   bool mBounds;
   bool mAsPoints;
   bool mWireframe;
   float mLineWidth;
   float mPointWidth;
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
   bool mCull;
   Frustum mFrustum;
   const SceneGeometryData *mSceneData;
   bool mTransformBounds;
   bool mLocators;
   Alembic::Abc::M44d mWorldMatrix;
   std::deque<Alembic::Abc::M44d> mMatrixStack;
   std::set<AlembicNode*> mCulledNodes;
};

// ---

class PrintInfo
{
public:
   
   PrintInfo(bool ignoreTransforms,
             bool ignoreInstances,
             bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   void leave(AlembicNode &node, AlembicNode *instance=0);

private:
   
   bool mNoTransforms;
   bool mNoInstances;
   bool mCheckVisibility;
};

#endif
