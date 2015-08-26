#include "AbcTranslator.h"
#include "extension/Extension.h"

extern "C"
{

DLLEXPORT void initializeExtension(CExtension& extension)
{
   MStatus status;
   extension.Requires(NAME_PREFIX "AbcShape");
   status = extension.RegisterTranslator(NAME_PREFIX "AbcShape",
                                         "",
                                         CAbcTranslator::Create,
                                         CAbcTranslator::NodeInitializer);
   // and gpuCache too?
}

DLLEXPORT void deinitializeExtension(CExtension& extension)
{
}

}