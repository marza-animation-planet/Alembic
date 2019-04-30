#ifndef GPUCACHE_COMMON_H_
#define GPUCACHE_COMMON_H_

#include "AlembicScene.h"

enum CycleType
{
   CT_hold = 0,
   CT_loop,
   CT_reverse,
   CT_bounce,
   CT_clip
};

extern void AssignDefaultShader(class MObject &obj);

#endif
