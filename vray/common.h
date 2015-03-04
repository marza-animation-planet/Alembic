#ifndef __abc_vray_common_h__
#define __abc_vray_common_h__

#include <vrayplugins.h>
#include <vrayinterface.h>
#include <plugman.h>
#include <defparams.h>
#include <charstring.h>
#include <factory.h>
#include <vraypluginsexporter.h>

#include <AlembicSceneCache.h>

#if 0
#  define _TRACELOG
#endif

#ifdef _TRACELOG
#  define TRACEF(func) { std::cout << "[AlembicLoader] " << #func << std::endl; }
#  define TRACEF_MSG(func, msg) { std::cout << "[AlembicLoader] " << #func << ": " << msg << std::endl; }
#  define TRACEM(klass, func) { std::cout << "[AlembicLoader] " << #klass << "::" << #func << std::endl; }
#  define TRACEM_MSG(klass, func, msg) { std::cout << "[AlembicLoader] " << #klass << "::" << #func << ": " << msg << std::endl; }
#else
#  define TRACEF(func)
#  define TRACEF_MSG(func, msg)
#  define TRACEM(klass, func)
#  define TRACEM_MSG(klass, func, msg)
#endif

#endif
