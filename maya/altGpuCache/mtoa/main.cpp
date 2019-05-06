#include "gpuCacheTranslator.h"
#include "extension/Extension.h"

extern "C"
{

DLLEXPORT void initializeExtension(CExtension& extension)
{
   MStatus status;
   extension.Requires("altGpuCache");
   status = extension.RegisterTranslator("altGpuCache",
                                         "",
                                         CGpuCacheTranslator::Create,
                                         CGpuCacheTranslator::NodeInitializer);
}

DLLEXPORT void deinitializeExtension(CExtension& extension)
{
}

}