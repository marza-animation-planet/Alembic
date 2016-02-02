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
   // This function won't get call for the same procedural node from difference threads
   
   AiMsgDebug("[abcproc] ProcGetNode [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   
   if (i == 0)
   {
      // generate the shape(s)
      if (dso->mode() == PM_multi)
      {
         // only generates new procedural nodes (read transform and bound information)
         MakeProcedurals procNodes(dso);
         dso->scene()->visit(AlembicNode::VisitDepthFirst, procNodes);
         
         if (procNodes.numNodes() != dso->numShapes())
         {
            AiMsgWarning("[abcproc] %lu procedural(s) generated (%lu expected)", procNodes.numNodes(), dso->numShapes());
         }
         
         CollectWorldMatrices refMatrices(dso);
         if (dso->referenceScene() && !dso->ignoreTransforms())
         {
            dso->referenceScene()->visit(AlembicNode::VisitDepthFirst, refMatrices);
         }
         
         for (size_t i=0; i<dso->numShapes(); ++i)
         {
            AtNode *node = procNodes.node(i);
            Alembic::Abc::M44d W;
            AtMatrix mtx;
            
            if (node && refMatrices.getWorldMatrix(procNodes.path(i), W))
            {
               for (int r=0; r<4; ++r)
               {
                  for (int c=0; c<4; ++c)
                  {
                     mtx[r][c] = W[r][c];
                  }
               }
               
               // Store reference object world matrix to avoid having to re-compute it later
               AiNodeDeclare(node, "Mref", "constant MATRIX");
               AiNodeSetMatrix(node, "Mref", mtx);
            }
            
            dso->setGeneratedNode(i, node);
         }
      }
      else
      {
         AtNode *output = 0;
         std::string masterNodeName;
         
         GlobalLock::Acquire();
         bool isInstance = dso->isInstance(&masterNodeName);
         GlobalLock::Release();
         
         if (!isInstance)
         {
            MakeShape visitor(dso);
            // dso->scene()->visit(AlembicNode::VisitFilteredFlat, visitor);
            dso->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
            
            output = visitor.node();
            
            if (output)
            {
               dso->transferUserParams(output);
               
               GlobalLock::Acquire();
               
               if (dso->isInstance(&masterNodeName))
               {
                  GlobalLock::Release();
                  
                  AiMsgWarning("[abcproc] Master node '%s' created in another thread. Ignore %s node '%s'.",
                               masterNodeName.c_str(),
                               AiNodeEntryGetName(AiNodeGetNodeEntry(output)),
                               AiNodeGetName(output));
                  
                  // reset name to avoid clashes
                  AiNodeSetStr(output, "name", "");
                  AiNodeSetByte(output, "visibility", 0);
#if AI_VERSION_ARCH_NUM > 4 || (AI_VERSION_ARCH_NUM == 4 && AI_VERSION_MAJOR_NUM > 2 || (AI_VERSION_MAJOR_NUM == 2 && AI_VERSION_MINOR_NUM >= 3))
                  AiNodeSetDisabled(output, true);
#endif

                  isInstance = true;
               }
               else
               {
                  dso->setMasterNodeName(AiNodeGetName(output));
                  
                  GlobalLock::Release();
               }
            }
         }
         
         if (isInstance)
         {
            AtNode *master = AiNodeLookUpByName(masterNodeName.c_str());
            
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
               AiMsgWarning("[abcproc] Master node '%s' not yet expanded. Ignore instance.", masterNodeName.c_str());
            }
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

#include <H5pubconf.h>

#ifdef H5_HAVE_THREADSAFE

#ifdef _WIN32

// ToDo

#else

#include <pthread.h>
#include <H5public.h>

static int __attribute__((constructor)) on_dlopen(void)
{
   H5dont_atexit();
}

static int __attribute__((destructor)) on_dlclose(void)
{
   // Hack around thread termination segfault
   // -> alembic procedural is unloaded before thread finishes
   //    resulting on HDF5 keys to leak
   
   extern pthread_key_t H5TS_errstk_key_g;
   extern pthread_key_t H5TS_funcstk_key_g;
   extern pthread_key_t H5TS_cancel_key_g;
   
   pthread_key_delete(H5TS_cancel_key_g);
   pthread_key_delete(H5TS_funcstk_key_g);
   pthread_key_delete(H5TS_errstk_key_g);
   
   return 0;
}

#endif

#endif

proc_loader
{
   vtable->Init     = ProcInit;
   vtable->Cleanup  = ProcCleanup;
   vtable->NumNodes = ProcNumNodes;
   vtable->GetNode  = ProcGetNode;
   strcpy(vtable->version, AI_VERSION);
   return 1;
}

