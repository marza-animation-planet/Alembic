#include <visitors/collectworldmatrices.h>

CollectWorldMatrices::CollectWorldMatrices(Dso *dso)
   : mDso(dso)
{
}
   
AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicXform &node, AlembicNode *)
{
   if (!node.isLocator() && !mDso->ignoreTransforms())
   {
      Alembic::AbcGeom::IXformSchema schema = node.typedObject().getSchema();
      
      Alembic::Abc::M44d parentWorldMatrix;
      
      if (mMatrixStack.size() > 0)
      {
         parentWorldMatrix = mMatrixStack.back();
      }
      
      Alembic::Abc::M44d localMatrix = schema.getValue().getMatrix();
      
      mMatrixStack.push_back(schema.getInheritsXforms() ? localMatrix * parentWorldMatrix : localMatrix);
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicMesh &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicSubD &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicPoints &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicCurves &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn CollectWorldMatrices::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (!mDso->ignoreInstances())
      {
         AlembicNode *m = node.master();
         return m->enter(*this, &node);
      }
      else
      {
         return AlembicNode::DontVisitChildren;
      }
   }
   else
   {
      return AlembicNode::ContinueVisit;
   }
}

void CollectWorldMatrices::leave(AlembicXform &node, AlembicNode *)
{
   if (!node.isLocator() && !mDso->ignoreTransforms())
   {
      mMatrixStack.pop_back();
   }
}

void CollectWorldMatrices::leave(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance() && !mDso->ignoreInstances())
   {
      AlembicNode *m = node.master();
      m->leave(*this, &node);
   }
}
