#ifndef ABCSHAPE_ABCSHAPEIMPORT_H_
#define ABCSHAPE_ABCSHAPEIMPORT_H_

#include <maya/MPxCommand.h>

class AbcShapeImport : public MPxCommand
{
public:
   
   AbcShapeImport();
   ~AbcShapeImport();

   virtual bool hasSyntax() const;
   virtual bool isUndoable() const;
   virtual MStatus doIt(const MArgList& args);

   static MSyntax createSyntax();
   static void* create();
};

#endif
