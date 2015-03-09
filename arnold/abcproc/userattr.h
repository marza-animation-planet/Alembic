#ifndef ABCPROC_USERATTR_H_
#define ABCPROC_USERATTR_H_

#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <ai.h>
#include <string>
#include <map>
#include <set>

struct UserAttribute
{
   int arnoldCategory;
   int arnoldType;
   std::string arnoldTypeStr;
   Alembic::AbcCoreAbstract::DataType abcType;
   bool isArray; // only for constant attributes
   unsigned int dataDim;
   unsigned int dataCount;
   void *data;
   unsigned int indicesCount; // only for indexed attributes (facevarying)
   unsigned int *indices; // only for indexed attributes (facevarying)
   std::set<std::string> strings;
};

typedef std::map<std::string, UserAttribute> UserAttributes;

void InitUserAttribute(UserAttribute &);

bool ReadUserAttribute(UserAttribute &ua,
                       Alembic::Abc::ICompoundProperty parent,
                       const Alembic::AbcCoreAbstract::PropertyHeader &header,
                       double t,
                       bool geoparam,
                       bool interpolate);

bool ResizeUserAttribute(UserAttribute &ua, unsigned int newSize);

bool CopyUserAttribute(UserAttribute &src, unsigned int srcIdx, unsigned int count, UserAttribute &dst, unsigned int dstIdx);

void SetUserAttribute(AtNode *node, const char *name, UserAttribute &ua, unsigned int count, unsigned int *remapIndices=0);

void SetUserAttributes(AtNode *node, UserAttributes &attribs, unsigned int count, unsigned int *remapIndices=0);

void DestroyUserAttribute(UserAttribute &);

void DestroyUserAttributes(UserAttributes &);

#endif
