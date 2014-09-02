#ifndef ABCPROC_VISITORS_H_
#define ABCPROC_VISITORS_H_

#include <AlembicNode.h>
#include <ai.h>
#include "dso.h"

// ---

class GetTimeRange
{
public:
   
   GetTimeRange();
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &node, AlembicNode *instance=0);

   bool getRange(double &start, double &end) const;

private:
   
   template <class T>
   void updateTimeRange(AlembicNodeT<T> &node);

private:
   
   double mStartTime;
   double mEndTime;
};

template <class T>
void GetTimeRange::updateTimeRange(AlembicNodeT<T> &node)
{
   Alembic::AbcCoreAbstract::TimeSamplingPtr ts = node.typedObject().getSchema().getTimeSampling();
   size_t nsamples = node.typedObject().getSchema().getNumSamples();
   if (nsamples > 1)
   {
      mStartTime = std::min(ts->getSampleTime(0), mStartTime);
      mEndTime = std::max(ts->getSampleTime(nsamples-1), mEndTime);
   }
}

// ---

extern bool GetVisibility(Alembic::Abc::ICompoundProperty props, double t);

// ---

class CountShapes
{
public:
   
   CountShapes(double renderTime, bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &node, AlembicNode *instance=0);

   inline size_t numShapes() const { return mNumShapes; }

private:
   
   template <class T>
   bool isVisible(AlembicNodeT<T> &node);
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node);

private:
   
   double mRenderTime;
   bool mIgnoreTransforms;
   bool mIgnoreInstances;
   bool mIgnoreVisibility;
   size_t mNumShapes;
};

template <class T>
AlembicNode::VisitReturn CountShapes::shapeEnter(AlembicNodeT<T> &node)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      ++mNumShapes;
      return AlembicNode::ContinueVisit;
   }
}

template <class T>
bool CountShapes::isVisible(AlembicNodeT<T> &node)
{
   return (mIgnoreVisibility ? true : GetVisibility(node.object().getProperties(), mRenderTime));
}

// ---

class MakeProcedurals
{
public:
   
   MakeProcedurals(class Dso *dso);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicXform &node, AlembicNode *instance=0);
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   inline size_t numNodes() const { return mNodes.size(); }
   inline AtNode* node(size_t i) const { return (i < numNodes() ? mNodes[i] : 0); }
   
private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);

private:

   class Dso *mDso;
   std::vector<std::vector<Alembic::Abc::M44d> > mMatrixSamplesStack;
   std::vector<AtNode*> mNodes;
};

template <class T>
AlembicNode::VisitReturn MakeProcedurals::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   // Bounds padding? [check disp_padding]
   
   Alembic::Util::bool_t visible = (mDso->ignoreVisibility() ? true : GetVisibility(node.object().getProperties(), mDso->renderTime()));
   
   if (visible)
   {
      TimeSampleList<Alembic::Abc::IBox3dProperty> &boundsSamples = node.samples().boundsSamples;
      
      Alembic::Abc::Box3d box;
      
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
         double t = mDso->motionSampleTime(i);
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample bounds \"%s\" at t=%lf", node.path().c_str(), t);
         }
         
         node.sampleBounds(t, t, i>0);
      }
      
      if (boundsSamples.size() == 0)
      {
         // no data... -> empty box, don't generate node
         mNodes.push_back(0);
         AiMsgWarning("[abcproc] No valid bounds for \"%s\"", node.path().c_str());
         return AlembicNode::ContinueVisit;
      }
      else if (boundsSamples.size() == 1)
      {
         box = boundsSamples.begin()->data();
      }
      else
      {
         for (size_t i=0; i<mDso->numMotionSamples(); ++i)
         {
            double t = mDso->motionSampleTime(i);
            
            TimeSampleList<Alembic::Abc::IBox3dProperty>::ConstIterator samp0, samp1;
            double blend = 0.0;
            
            if (boundsSamples.getSamples(t, samp0, samp1, blend))
            {
               if (blend > 0.0)
               {
                  Alembic::Abc::Box3d b0 = samp0->data();
                  Alembic::Abc::Box3d b1 = samp1->data();
                  Alembic::Abc::Box3d b2((1.0 - blend) * b0.min + blend * b1.min,
                                         (1.0 - blend) * b0.max + blend * b1.max);
                  box.extendBy(b2);
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] Interpolated bounds at t=%lf: (%lf, %lf, %lf) -> (%lf, %lf, %lf)",
                  //              t, b2.min.x, b2.min.y, b2.min.z, b2.max.x, b2.max.y, b2.max.z);
                  // }
               }
               else
               {
                  Alembic::Abc::Box3d b = samp0->data();
                  
                  box.extendBy(b);
                  
                  // if (mDso->verbose())
                  // {
                  //    AiMsgInfo("[abcproc] Bounds at t=%lf: (%lf, %lf, %lf) -> (%lf, %lf, %lf)",
                  //              t, b.min.x, b.min.y, b.min.z, b.max.x, b.max.y, b.max.z);
                  // }
               }
            }
         }
      }
      
      const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mDso->procNode(), "disp_padding");
      if (pe && mDso->overrideAttrib("disp_padding"))
      {
         float padding = AiNodeGetFlt(mDso->procNode(), "disp_padding");
         
         if (padding > 0.0f && mDso->verbose())
         {
            AiMsgInfo("[abcproc] \"disp_padding\" attribute found on procedural, pad bounds by %f", padding);
         }
         
         box.min.x -= padding;
         box.min.y -= padding;
         box.min.z -= padding;
         
         box.max.x += padding;
         box.max.y += padding;
         box.max.z += padding;
      }
      
      AlembicNode *targetNode = (instance ? instance : &node);
      
      // Format name 'a la maya'
      std::string targetName = targetNode->formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
      std::string name = mDso->uniqueName(targetName);
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate new procedural node \"%s\"", name.c_str());
         AiMsgInfo("[abcproc]   Bounds: (%lf, %lf, %lf) -> (%lf, %lf, %lf)",
                   box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z);
      }
      
      AtNode *proc = AiNode("procedural");
      AiNodeSetStr(proc, "name", name.c_str());
      AiNodeSetStr(proc, "dso", AiNodeGetStr(mDso->procNode(), "dso"));
      AiNodeSetStr(proc, "data", mDso->dataString(targetNode->path().c_str()).c_str());
      AiNodeSetBool(proc, "load_at_init", false);
      AiNodeSetPnt(proc, "min", box.min.x, box.min.y, box.min.z);
      AiNodeSetPnt(proc, "max", box.max.x, box.max.y, box.max.z);
      
      if (!mDso->ignoreTransforms() && mMatrixSamplesStack.size() > 0)
      {
         std::vector<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] %lu xform sample(s) for \"%s\"", matrices.size(), node.path().c_str());
         }
         
         AtArray *ary = AiArrayAllocate(1, matrices.size(), AI_TYPE_MATRIX);
         
         for (size_t i=0; i<matrices.size(); ++i)
         {
            Alembic::Abc::M44d &src = matrices[i];
            
            AtMatrix dst;
            for (int r=0; r<4; ++r)
               for (int c=0; c<4; ++c)
                  dst[r][c] = float(src[r][c]);
            
            // if (mDso->verbose())
            // {
            //    AiMsgInfo("[abcproc] [[%lf, %lf, %lf, %lf],\n[%lf, %lf, %lf, %lf],\n[%lf, %lf, %lf, %lf],\n[%lf, %lf, %lf, %lf]]",
            //              src[0][0], src[0][1], src[0][2], src[0][3],
            //              src[1][0], src[1][1], src[1][2], src[1][3],
            //              src[2][0], src[2][1], src[2][2], src[2][3],
            //              src[3][0], src[3][1], src[3][2], src[3][3]);
            // }
            
            AiArraySetMtx(ary, i, dst);
         }
         
         AiNodeSetArray(proc, "matrix", ary);
      }
      
      mDso->transferUserParams(proc);
      
      // Add instance_num attribute
      if (AiNodeLookUpUserParameter(proc, "instance_num") == 0)
      {
         AiNodeDeclare(proc, "instance_num", "constant INT");
      }
      AiNodeSetInt(proc, "instance_num", int(node.instanceNumber()));
      
      mNodes.push_back(proc);
      
      return AlembicNode::ContinueVisit;
   }
   else
   {
      return AlembicNode::DontVisitChildren;
   }
}

// ---

class MakeShape
{
public:
   
   MakeShape(class Dso *dso);
   
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   inline AtNode* node() const { return mNode; }
   
private:

   class Dso *mDso;
   AtNode *mNode;
};

#endif
