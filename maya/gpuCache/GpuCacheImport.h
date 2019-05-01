#ifndef GPUCACHE_GPUCACHEIMPORT_H_
#define GPUCACHE_GPUCACHEIMPORT_H_

#include <maya/MPxCommand.h>

class GpuCacheImport : public MPxCommand
{
public:
   
   GpuCacheImport();
   ~GpuCacheImport();

   virtual bool hasSyntax() const;
   virtual bool isUndoable() const;
   virtual MStatus doIt(const MArgList& args);

   static MSyntax createSyntax();
   static void* create();
};

#endif
