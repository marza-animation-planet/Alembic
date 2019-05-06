#ifndef GPUCACHE_GPUCACHETREE_H_
#define GPUCACHE_GPUCACHETREE_H_

#include <maya/MPxCommand.h>

class GpuCacheTree : public MPxCommand
{
public:
   
   GpuCacheTree();
   ~GpuCacheTree();

   virtual bool hasSyntax() const;
   virtual bool isUndoable() const;
   virtual MStatus doIt(const MArgList& args);

   static MSyntax createSyntax();
   static void* create();
};

#endif
