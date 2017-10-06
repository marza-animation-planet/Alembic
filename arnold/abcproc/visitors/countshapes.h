#ifndef ABCPROC_VISITORS_COUNTSHAPES_H_
#define ABCPROC_VISITORS_COUNTSHAPES_H_

#include <visitors/common.h>

class CountShapes
{
public:
   
   CountShapes(double renderTime, bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility, bool ignoreNurbs);
   
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
   bool mIgnoreNurbs;
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

#endif
