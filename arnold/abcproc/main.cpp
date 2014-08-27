#include <ai.h>
#include "dso.h"


// ---

int ProcInit(AtNode *node, void **user_ptr)
{
   AiMsgDebug("[abcproc] ProcInit");
   Dso *dso = new Dso(node);
   *user_ptr = dso;
   return 1;
}

int ProcNumNodes(void *user_ptr)
{
   AiMsgDebug("[abcproc] ProcNumNodes");
   Dso *dso = (Dso*) user_ptr;
   
   //return dso->getNumShapes();
   
   return 0;
}

AtNode* ProcGetNode(void *user_ptr, int i)
{
   AiMsgDebug("[abcproc] ProcGetNode");
   Dso *dso = (Dso*) user_ptr;
   
   //check i validity
   
   return 0;
}

int ProcCleanup(void *user_ptr)
{
   AiMsgDebug("[abcproc] ProcCleanup");
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

