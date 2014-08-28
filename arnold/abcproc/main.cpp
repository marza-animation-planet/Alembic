#include <ai.h>
#include "dso.h"
#include "visitors.h"
#include "globallock.h"


// ---

int ProcInit(AtNode *node, void **user_ptr)
{
   AiMsgDebug("[abcproc] ProcInit [thread %p]", AiThreadSelf());
   Dso *dso = new Dso(node);
   *user_ptr = dso;
   return 1;
}

int ProcNumNodes(void *user_ptr)
{
   AiMsgDebug("[abcproc] ProcNumNodes [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   
   return int(dso->numShapes());
}

AtNode* ProcGetNode(void *user_ptr, int i)
{
   // Can this be called from different threads with same procedural?
   // => should not no
   // ~? 
   
   AiMsgDebug("[abcproc] ProcGetNode [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   
   if (i == 0)
   {
      // generate the shape(s)
      if (dso->mode() == PM_multi)
      {
         // only generates new procedural nodes (read transform and bound information)
         MakeProcedurals visitor(dso);
         dso->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
         
         if (visitor.numNodes() != dso->numShapes())
         {
            AiMsgWarning("[abcproc] %lu procedural(s) generated (%lu expected)", visitor.numNodes(), dso->numShapes());
         }
         
         for (size_t i=0; i<dso->numShapes(); ++i)
         {
            dso->setGeneratedNode(i, visitor.node(i));
         }
      }
      else
      {
         AtNode *output = 0;
         
         // Access to Dso::msMasterNodes is not thread safe
         GlobalLock::Acquire();
         
         if (dso->isInstance())
         {
            GlobalLock::Release();
            
            // if (dso->ignoreInstances())
            // {
            // }
            
            AtNode *master = AiNodeLookUpByName(dso->masterNodeName().c_str());
            if (master)
            {
               if (dso->verbose())
               {
                  AiMsgInfo("[abcproc] Create a new instance of \"%s\"", AiNodeGetName(master));
               }
               
               output = AiNode("ginstance");
               
               // node name?
               
               AiNodeSetBool(output, "inherit_xform", false);
               AiNodeSetPtr(output, "node", master);
               
               // It seems that when ginstance node don't  inherit from the procedural
               //   attributes like standard shapes
               dso->transferInstanceParams(output);
               
               dso->transferUserParams(output);
            }
            else
            {
               AiMsgWarning("[abcproc] Master node not expanded");
            }
         }
         else
         {
            // Not yet generated, are there any instances of that node at all anyway?
            // Should check in order to release lock sooner...
            
            MakeShape visitor(dso);
            dso->scene()->visit(AlembicNode::VisitFilteredFlat, visitor);
            
            output = visitor.node();
            
            if (output)
            {
               dso->transferUserParams(output);
               dso->setMasterNodeName(AiNodeGetName(output));
            }
            
            GlobalLock::Release();
         }
         
         dso->setGeneratedNode(0, output);
      }
   }
   
   return dso->generatedNode(i);
}

int ProcCleanup(void *user_ptr)
{
   AiMsgDebug("[abcproc] ProcCleanup [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   delete dso;
   return 1;
}

// ---

proc_loader
{
   vtable->Init     = ProcInit;
   vtable->Cleanup  = ProcCleanup;
   vtable->NumNodes = ProcNumNodes;
   vtable->GetNode  = ProcGetNode;
   strcpy(vtable->version, AI_VERSION);
   return 1;
}

