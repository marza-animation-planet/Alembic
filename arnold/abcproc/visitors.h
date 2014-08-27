#ifndef ABCPROC_VISITORS_H_
#define ABCPROC_VISITORS_H_

#include <AlembicNode.h>

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
               
               if (prop.isConstant())
               {
                  prop.get(v);
               }
               else
               {
                  size_t nsamples = prop.getNumSamples();
                  Alembic::Abc::TimeSamplingPtr timeSampling = prop.getTimeSampling();
                  std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime = timeSampling->getNearIndex(mRenderTime, nsamples);
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
                  std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime = timeSampling->getNearIndex(mRenderTime, nsamples);
                  prop.get(visible, Alembic::Abc::ISampleSelector(indexTime.first));
               }
            }
         }
      }
   }
   
   return visible;
}

// ---

#endif
