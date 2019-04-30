#include "AbcTranslator.h"
#include "extension/Extension.h"

extern "C"
{

DLLEXPORT void initializeExtension(CExtension& extension)
{
   MStatus status;
   extension.Requires("gpuCache");
   status = extension.RegisterTranslator("gpuCache",
                                         "",
                                         CAbcTranslator::Create,
                                         CAbcTranslator::NodeInitializer);
}

DLLEXPORT void deinitializeExtension(CExtension& extension)
{
}

}