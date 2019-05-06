#ifndef GPUCACHE_ALEMBICSCENEVISITORS_H_
#define GPUCACHE_ALEMBICSCENEVISITORS_H_

#include "common.h"
#include <set>
#include <string>

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
