#include "params.h"
#include "plugin.h"

#undef VRAYPLUG_DLL_EXPORT
#ifdef _WIN32
#  define VRAYPLUG_DLL_EXPORT __declspec(dllexport)
#else
#  define VRAYPLUG_DLL_EXPORT __attribute__((visibility("default")))
#endif

using namespace VUtils;
SIMPLE_PLUGIN_LIBRARY(PluginID(LARGE_CONST(0xFFFF201502250001)), EXT_STATIC_GEOM_SOURCE, "AlembicLoader", "Alembic geometry provider", AlembicLoader, AlembicLoaderParams);
