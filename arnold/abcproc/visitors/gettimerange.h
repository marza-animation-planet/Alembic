#ifndef ABCPROC_VISITORS_GETTIMERANGE_H_
#define ABCPROC_VISITORS_GETTIMERANGE_H_

#include <visitors/common.h>

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

#endif
