#ifndef __abc_vray_userattr_h__
#define __abc_vray_userattr_h__

#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>
#include <string>
#include <map>
#include <set>
#include "geomsrc.h"

enum Usage
{
   Use_As_Color = 0,
   Use_As_Point,
   Use_As_Vector,
   Use_As_Normal,
   Use_As_Matrix,
   Undefined_Usage
};

enum Scope
{
   Constant_Scope = 0,
   Uniform_Scope,
   Varying_Scope,
   FaceVarying_Scope
};

// Simplify types (makes my life easier)
enum DataType
{
   Bool_Type = 0,
   Int_Type,
   Uint_Type,
   Float_Type,
   String_Type,
   Undefined_Type
};

struct UserAttribute
{
   Alembic::AbcCoreAbstract::DataType abcType;
   Usage usage;
   Scope scope;
   bool isArray; // only for constant attributes
   DataType dataType;
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
                       bool interpolate,
                       bool verbose=false);

bool ResizeUserAttribute(UserAttribute &ua, unsigned int newSize);

bool CopyUserAttribute(UserAttribute &src, unsigned int srcIdx, unsigned int count, UserAttribute &dst, unsigned int dstIdx);

void DestroyUserAttribute(UserAttribute &);
void DestroyUserAttributes(UserAttributes &);

bool SetUserAttribute(AlembicGeometrySource::GeomInfo *geom, const char *name, UserAttribute &ua, double t, std::set<std::string> &usedNames, bool verbose=false);
void SetUserAttributes(AlembicGeometrySource::GeomInfo *geom, UserAttributes &attribs, double t, bool verbose=false);

#endif
