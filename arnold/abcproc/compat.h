#ifndef ABCPROC_COMPAT_H_
#define ABCPROC_COMPAT_H_

#include <ai.h>

#if AI_VERSION_ARCH_NUM < 5

#define ARNOLD4_API

#define procedural_init      int ProcInit(AtNode *node, void **user_ptr)
#define procedural_num_nodes int ProcNumNodes(void *user_ptr)
#define procedural_get_node  AtNode* ProcGetNode(void *user_ptr, int i)
#define procedural_cleanup   int ProcCleanup(void *user_ptr)

#ifndef uint8_t
typedef AtByte uint8_t;
#endif

inline AtNode* AiNode(const char *type, const char *name, AtNode *parent)
{
   AtNode *n = AiNode(type);
   AiNodeSetStr(n, "name", name);
   return n;
}

inline unsigned int AiArrayGetNumKeys(AtArray *ary)
{
   return ary->nkeys;
}

inline unsigned int AiArrayGetNumElements(AtArray *ary)
{
   return ary->nelements;
}

inline int AiArrayGetType(AtArray *ary)
{
   return ary->type;
}

typedef AtPoint2 AtVector2;
#define AI_TYPE_VECTOR2 AI_TYPE_POINT2

inline AtVector2 AiArrayGetVec2(AtArray *ary, unsigned int idx)
{
   return AiArrayGetPnt2(ary, idx);
}

inline void AiArraySetVec2(AtArray *ary, unsigned int idx, AtVector2 val)
{
   AiArraySetPnt2(ary, idx, val);
}

inline AtVector2 AiNodeGetVec2(AtNode *node, const char *pname)
{
   return AiNodeGetPnt2(node, pname);
}

inline void AiNodeSetVec2(AtNode *node, const char *pname, float x, float y)
{
   AiNodeSetPnt2(node, pname, x, y);
}

#define POINT_TYPE_STR        "POINT"
#define POINTARRAY_TYPE_STR   "ARRAY POINT"
#define VECTOR2_TYPE_STR      "POINT2"
#define VECTOR2ARRAY_TYPE_STR "ARRAY POINT2"

#else

typedef AtVector AtPoint;
#define AI_TYPE_POINT AI_TYPE_VECTOR
#define AiNodeSetPnt  AiNodeSetVec
#define AiNodeGetPnt  AiNodeGetVec
#define AiArraySetPnt AiArraySetVec
#define AiArrayGetPnt AiArrayGetVec

#define POINT_TYPE_STR        "VECTOR"
#define POINTARRAY_TYPE_STR   "ARRAY VECTOR"
#define VECTOR2_TYPE_STR      "VECTOR2"
#define VECTOR2ARRAY_TYPE_STR "ARRAY VECTOR2"

#endif

#endif
