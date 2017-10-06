#ifndef ABCPROC_VISITORS_COLLECTWORLDMATRICES_H_
#define ABCPROC_VISITORS_COLLECTWORLDMATRICES_H_

#include <visitors/common.h>

class CollectWorldMatrices
{
public:
   
   CollectWorldMatrices(class Dso *dso);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicXform &node, AlembicNode *instance=0);
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   inline bool getWorldMatrix(const std::string &objectPath, Alembic::Abc::M44d &W) const
   {
      std::map<std::string, Alembic::Abc::M44d>::const_iterator it = mMatrices.find(objectPath);
      if (it != mMatrices.end())
      {
         W = it->second;
         return true;
      }
      else
      {
         return false;
      }
   }
   
private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
private:
   
   class Dso *mDso;
   std::vector<Alembic::Abc::M44d> mMatrixStack;
   std::map<std::string, Alembic::Abc::M44d> mMatrices;
};

template <class T>
AlembicNode::VisitReturn CollectWorldMatrices::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   Alembic::Abc::M44d W;
   
   if (mMatrixStack.size() > 0)
   {
      W = mMatrixStack.back();
   }
   
   std::string path = (instance ? instance->path() : node.path());
   
   mMatrices[path] = W;
   
   return AlembicNode::ContinueVisit;
}

#endif
