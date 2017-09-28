#ifndef ABCPROC_VISITORS_MAKEPROCEDURALS_H_
#define ABCPROC_VISITORS_MAKEPROCEDURALS_H_

#include <visitors/common.h>

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
   inline const char* path(size_t i) const { return (i < numNodes() ? mPaths[i].c_str() : 0); }

private:

   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance);

private:

   class Dso *mDso;
   std::deque<std::deque<Alembic::Abc::M44d> > mMatrixSamplesStack;
   std::deque<AtNode*> mNodes;
   std::deque<std::string> mPaths;
};

template <class T>
AlembicNode::VisitReturn MakeProcedurals::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   Alembic::Util::bool_t visible = (mDso->ignoreVisibility() ? true : GetVisibility(node.object().getProperties(), mDso->renderTime()));
   
   node.setVisible(visible);
   
   if (visible)
   {
      AlembicNode *targetNode = (instance ? instance : &node);
      
      // Format name 'a la maya'
      AbcProcGlobalLock::Acquire();
      
      std::string targetName = targetNode->formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
      std::string name = mDso->uniqueName(targetName);

      AtNode *proc = AiNode("abcproc", name.c_str(), mDso->procNode());

      AbcProcGlobalLock::Release();

      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate new procedural node \"%s\"", name.c_str());
      }
      
      mDso->setSingleParams(proc, targetNode->path());
      
      if (!mDso->ignoreTransforms() && mMatrixSamplesStack.size() > 0)
      {
         std::deque<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
         
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
            
            AiArraySetMtx(ary, i, dst);
         }
         
         AiNodeSetArray(proc, Strings::matrix, ary);
      }
      
      mDso->transferUserParams(proc);
      
      // Add instance_num attribute
      if (AiNodeLookUpUserParameter(proc, Strings::instance_num) == 0)
      {
         AiNodeDeclare(proc, Strings::instance_num, "constant INT");
      }
      AiNodeSetInt(proc, Strings::instance_num, int(targetNode->instanceNumber()));
      
      mNodes.push_back(proc);
      mPaths.push_back(targetNode->path());
      
      return AlembicNode::ContinueVisit;
   }
   else
   {
      return AlembicNode::DontVisitChildren;
   }
}

#endif
