#ifndef ABCSHAPE_COMMON_H_
#define ABCSHAPE_COMMON_H_

#ifdef NAME_PREFIX
#   define PREFIX_NAME(s) NAME_PREFIX s
#else
#   define PREFIX_NAME(s) s
#endif

#include "AlembicScene.h"
#include "GeometryData.h"
#include "MathUtils.h"

#ifdef ABCSHAPE_VRAY_SUPPORT
#   include <vrayplugins.h>
#   include <vrayinterface.h>
#   include <plugman.h>
#   include <vraymayageom.h>
#   include <defparams.h>
#   include <vraypluginsexporter.h>
#endif

#endif
