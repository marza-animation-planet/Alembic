#include "AlembicSceneVisitors.h"
#include <iostream>

// ---

GetFrameRange::GetFrameRange(bool ignoreTransforms,
                             bool ignoreInstances,
                             bool ignoreVisibility)
   : mStartFrame(std::numeric_limits<double>::max())
   , mEndFrame(-std::numeric_limits<double>::max())
   , mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
{
}
   
AlembicNode::VisitReturn GetFrameRange::enter(AlembicXform &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      if (!mNoTransforms)
      {
         if (node.isLocator())
         {
            const Alembic::Abc::IScalarProperty &prop = node.locatorProperty();
            
            Alembic::AbcCoreAbstract::TimeSamplingPtr ts = prop.getTimeSampling();
            size_t nsamples = prop.getNumSamples();
            
            if (nsamples > 1)
            {
               mStartFrame = std::min(ts->getSampleTime(0), mStartFrame);
               mEndFrame = std::max(ts->getSampleTime(nsamples-1), mEndFrame);
            }
         }
         else
         {
            updateFrameRange(node);
         }
      }
      
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicMesh &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicSubD &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicPoints &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicCurves &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicNuPatch &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      updateFrameRange(node);
      return AlembicNode::ContinueVisit;
   }
}

AlembicNode::VisitReturn GetFrameRange::enter(AlembicNode &node, AlembicNode *)
{
   if (mCheckVisibility && !node.isVisible())
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      if (node.isInstance())
      {
         if (!mNoInstances)
         {
            return node.master()->enter(*this, &node);
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
}
   
void GetFrameRange::leave(AlembicNode &, AlembicNode *)
{
}

bool GetFrameRange::getFrameRange(double &start, double &end) const
{
   if (mStartFrame < mEndFrame)
   {
      start = mStartFrame;
      end = mEndFrame;
      return true;
   }
   else
   {
      return false;
   }
}

// ---

PrintInfo::PrintInfo(bool ignoreTransforms,
                     bool ignoreInstances,
                     bool ignoreVisibility)
   : mNoTransforms(ignoreTransforms)
   , mNoInstances(ignoreInstances)
   , mCheckVisibility(!ignoreVisibility)
{
}

AlembicNode::VisitReturn PrintInfo::enter(AlembicNode &node, AlembicNode *)
{
   if (!node.isInstance() || !mNoInstances || !mCheckVisibility || node.isVisible())
   {
      std::cout << node.path() << ":" << std::endl;
      std::cout << "  type: " << node.typeName() << std::endl;
      
      std::cout << "  inherits transform: " << node.inheritsTransform() << std::endl;
      std::cout << "  self matrix: " << node.selfMatrix() << std::endl;
      std::cout << "  self bounds: " << node.selfBounds().min << " - " << node.selfBounds().max << std::endl;
      std::cout << "  locator: " << node.isLocator() << std::endl;
      
      if (!mNoTransforms)
      {
         std::cout << "  child bounds: " << node.childBounds().min << " - " << node.childBounds().max << std::endl;
         std::cout << "  world matrix: " << node.worldMatrix() << std::endl;
      }
      
      if (!mNoInstances)
      {
         std::cout << "  instance number: " << node.instanceNumber() << std::endl;
         std::cout << "  master instance: " << node.masterPath() << std::endl;
      }
      
      if (!mCheckVisibility)
      {
         std::cout << "  visibility: " << node.isVisible() << std::endl;
      }
      
      std::cout << std::endl;
      
      return AlembicNode::ContinueVisit;
   }
   else
   {
      return AlembicNode::DontVisitChildren;
   }
}

void PrintInfo::leave(AlembicNode &, AlembicNode *)
{
}
