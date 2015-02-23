#include "userattr.h"
#include <SampleUtils.h>

void InitUserAttribute(UserAttribute &attrib)
{
   attrib.arnoldCategory = AI_USERDEF_CONSTANT;
   attrib.arnoldType = AI_TYPE_UNDEFINED;
   attrib.arnoldTypeStr = "";
   attrib.abcType = Alembic::AbcCoreAbstract::DataType();
   attrib.isArray = false;
   attrib.dataDim = 0;
   attrib.dataCount = 0;
   attrib.data = 0;
   attrib.indicesCount = 0;
   attrib.indices = 0;
}

bool ResizeUserAttribute(UserAttribute &ua, unsigned int newSize)
{
   if (ua.arnoldType == AI_TYPE_UNDEFINED)
   {
      return false;
   }
   
   // Do not handled resizing for indexed user attribute
   if (ua.indicesCount > 0)
   {
      return false;
   }
   
   bool boolDef = false;
   AtByte byteDef = 0;
   int intDef = 0;
   unsigned int uintDef = 0;
   float floatDef = 0.0f;
   AtPoint2 pnt2Def = {0.0f, 0.0f};
   AtPoint pntDef = {0.0f, 0.0f, 0.0f};
   AtVector vecDef = {0.0f, 0.0f, 0.0f};
   AtColor rgbDef = {0.0f, 0.0f, 0.0f};
   AtRGBA rgbaDef = {0.0f, 0.0f, 0.0f, 1.0f};
   AtMatrix matrixDef;
   const char *strDef = 0;
   
   AiM4Identity(matrixDef);
   
   std::set<std::string>::iterator it = ua.strings.insert("").first;
   strDef = it->c_str();
   
   size_t oldBytesize = (ua.data ? ua.dataCount : 0) * ua.dataDim;
   size_t newBytesize = newSize * ua.dataDim;
   size_t podSize = 0;
   void *defaultValue = 0;
   
   switch (ua.arnoldType)
   {
   case AI_TYPE_BOOLEAN:
      podSize = sizeof(bool);
      defaultValue = &boolDef;
      break;
   case AI_TYPE_BYTE:
      podSize = sizeof(AtByte);
      defaultValue = &byteDef;
      break;
   case AI_TYPE_INT:
      defaultValue = &intDef;
      podSize = sizeof(int);
      break;
   case AI_TYPE_UINT:
      defaultValue = &uintDef;
      podSize = sizeof(unsigned int);
      break;
   case AI_TYPE_FLOAT:
      defaultValue = &floatDef;
      podSize = sizeof(float);
      break;
   case AI_TYPE_POINT2:
      defaultValue = &(pnt2Def.x);
      podSize = sizeof(float);
      break;
   case AI_TYPE_POINT:
      defaultValue = &(pntDef.x);
      podSize = sizeof(float);
      break;
   case AI_TYPE_VECTOR:
      defaultValue = &(vecDef.x);
      podSize = sizeof(float);
      break;
   case AI_TYPE_RGB:
      defaultValue = &(rgbDef.r);
      podSize = sizeof(float);
      break;
   case AI_TYPE_RGBA:
      defaultValue = &(rgbaDef.r);
      podSize = sizeof(float);
      break;
   case AI_TYPE_MATRIX:
      defaultValue = &(matrixDef[0][0]);
      podSize = sizeof(float);
      break;
   case AI_TYPE_STRING:
      defaultValue = &strDef;
      podSize = sizeof(const char*);
      break;
   default:
      break;
   }
   
   if (podSize == 0)
   {
      return false;
   }
   
   oldBytesize *= podSize;
   newBytesize *= podSize;
   
   if (newBytesize == 0)
   {
      if (ua.data)
      {
         AiFree(ua.data);
         ua.data = 0;
         ua.dataCount = 0;
      }
   }
   else
   {
      size_t copySize = (oldBytesize > newBytesize ? newBytesize : oldBytesize);
      size_t resetSize = (newBytesize > oldBytesize ? (newBytesize - oldBytesize) : 0);
      
      unsigned char *oldBytes = (unsigned char*) ua.data;
      unsigned char *newBytes = (unsigned char*) AiMalloc(newBytesize);
      
      if (oldBytes)
      {
         memcpy(newBytes, oldBytes, copySize);
         AiFree(oldBytes);
      }
      
      unsigned char *newElem = newBytes + oldBytesize;
      unsigned char *lastElem = newElem + resetSize;
      size_t elemSize = ua.dataDim * podSize;
      
      while (newElem < lastElem)
      {
         memcpy(newElem, defaultValue, elemSize);
         newElem += elemSize
      }
      
      ua.data = newBytes;
      ua.dataCount = newSize;
   }
   
   return true;
}

bool CopyUserAttribute(UserAttribute &src, unsigned int srcIdx, unsigned int count,
                       UserAttribute &dst, unsigned int dstIdx)
{
   if (src.arnoldType != dst.arnoldType ||
       src.dataDim != dst.dataDim ||
       src.indicesCount > 0 ||
       dst.indicesCount > 0 ||
       src.data == 0 ||
       dst.data == 0 ||
       srcIdx >= src.dataCount ||
       dstIdx >= dst.dataCount ||
       srcIdx + count > src.dataCount ||
       dstIdx + count > dst.dataCount)
   {
      return false;
   }
   
   size_t podSize = 0;
   
   switch (src.arnoldType)
   {
   case AI_TYPE_BOOLEAN:
      podSize = sizeof(bool);
      break;
   case AI_TYPE_BYTE:
      podSize = sizeof(AtByte);
      break;
   case AI_TYPE_INT:
      podSize = sizeof(int);
      break;
   case AI_TYPE_UINT:
      podSize = sizeof(unsigned int);
      break;
   case AI_TYPE_FLOAT:
   case AI_TYPE_POINT2:
   case AI_TYPE_POINT:
   case AI_TYPE_VECTOR:
   case AI_TYPE_RGB:
   case AI_TYPE_RGBA:
   case AI_TYPE_MATRIX:
      podSize = sizeof(float);
      break;
   case AI_TYPE_STRING:
      podSize = sizeof(const char*);
      break;
   default:
      break;
   }
   
   if (podSize == 0)
   {
      return false;
   }
   
   if (src.arnoldType == AI_TYPE_STRING)
   {
      // For strings dataDim should always be 1
      
      const char** srcStrs = ((const char**) src.data) + (srcIdx * src.dataDim);
      const char** dstStrs = ((const char**) dst.data) + (dstIdx * dst.dataDim);
      
      for (unsigned int i=0; i<count; ++i, ++srcStrs, ++dstStrs)
      {
         *dstStrs = dst.strings.insert(*srcStrs).first->c_str();
      }
   }
   else
   {
      size_t elemSize = src.dataDim * podSize;
      
      unsigned char *srcBytes = ((unsigned char*) src.data) + srcIdx * elemSize;
      unsigned char *dstBytes = ((unsigned char*) dst.data) + dstIdx * elemSize;
      
      memcpy(dstBytes, srcBytes, count * elemSize);
   }
   
   return true;
}

void DestroyUserAttribute(UserAttribute &attrib)
{
   if (attrib.data)
   {
      AiFree(attrib.data);
   }
   
   if (attrib.indices)
   {
      AiFree(attrib.indices);
   }
   
   attrib.strings.clear();
   
   InitUserAttribute(attrib);
}

void DestroyUserAttributes(UserAttributes &attribs)
{
   UserAttributes::iterator it = attribs.begin();
   
   while (it != attribs.end())
   {
      DestroyUserAttribute(it->second);
      ++it;
   }
   
   attribs.clear();
}

// --

template <typename SrcT, typename DstT>
struct TypeHelper
{
   static inline void Alloc(UserAttribute &ua)
   {
      ua.data = AiMalloc(ua.dataCount * ua.dataDim * sizeof(DstT));
   }
   
   static inline DstT Convert(UserAttribute &, const SrcT &v)
   {
      return DstT(v);
   }
   
   static inline DstT Blend(UserAttribute &, const SrcT &v0, const SrcT &v1, double a, double b)
   {
      return DstT(a * double(v0) + b * double(v1));
   }
};

template <typename T>
struct TypeHelper<T, T>
{
   static inline void Alloc(UserAttribute &ua)
   {
      ua.data = AiMalloc(ua.dataCount * ua.dataDim * sizeof(T));
   }
   
   static inline T Convert(UserAttribute &, const T &v)
   {
      return v;
   }
   
   static inline T Blend(UserAttribute &, const T &v0, const T &v1, double a, double b)
   {
      return T(a * double(v0) + b * double(v1));
   }
};

template <typename SrcT>
struct TypeHelper<SrcT, bool>
{
   static inline void Alloc(UserAttribute &ua)
   {
      ua.data = AiMalloc(ua.dataCount * ua.dataDim * sizeof(bool));
   }
   
   static inline bool Convert(UserAttribute &, const SrcT &v)
   {
      return (v != SrcT(0));
   }
   
   static inline bool Blend(UserAttribute &, const SrcT &v0, const SrcT &v1, double a, double b)
   {
      return (a * double(v0) + b * double(v1) > 0.5);
   }
};

template <>
struct TypeHelper<Alembic::Util::bool_t, bool>
{
   static inline void Alloc(UserAttribute &ua)
   {
      ua.data = AiMalloc(ua.dataCount * ua.dataDim * sizeof(bool));
   }
   
   static inline bool Convert(UserAttribute &, const Alembic::Util::bool_t &v)
   {
      return v;
   }
   
   static inline bool Blend(UserAttribute &, const Alembic::Util::bool_t &v0, const Alembic::Util::bool_t &v1, double a, double)
   {
      return (a < 0.5 ? v1 : v0);
   }
};

template <>
struct TypeHelper<std::string, const char*>
{
   static inline void Alloc(UserAttribute &ua)
   {
      ua.data = AiMalloc(ua.dataCount * ua.dataDim * sizeof(const char*));
   }
   
   static inline const char* Convert(UserAttribute &ua, const std::string &v)
   {
      std::set<std::string>::iterator it = ua.strings.find(v);
      if (it != ua.strings.end())
      {
         return it->c_str();
      }
      else
      {
         it = ua.strings.insert(v).first;
         return it->c_str();
      }
   }
   
   static inline const char* Blend(UserAttribute &ua, const std::string &v0, const std::string &v1, double a, double)
   {
      return (a < 0.5 ? Convert(ua, v1) : Convert(ua, v0));
   }
};

template <typename T>
const T* GetPODPtr(const T &val)
{
   return &val;
}

template <typename T>
const T* GetPODPtr(const Imath::Vec2<T> &v)
{
   return &(v.x);
}

template <typename T>
const T* GetPODPtr(const Imath::Vec3<T> &v)
{
   return &(v.x);
}

template <typename T>
const T* GetPODPtr(const Imath::Vec4<T> &v)
{
   return &(v.x);
}

template <typename T>
const T* GetPODPtr(const Imath::Color3<T> &v)
{
   return &(v.x);
}

template <typename T>
const T* GetPODPtr(const Imath::Color4<T> &v)
{
   return &(v.r);
}

template <typename T>
const T* GetPODPtr(const Imath::Matrix33<T> &v)
{
   return &(v[0][0]);
}

template <typename T>
const T* GetPODPtr(const Imath::Matrix44<T> &v)
{
   return &(v[0][0]);
}


template <typename SrcT, typename DstT, class ScalarProperty>
bool ReadScalarProperty(ScalarProperty prop, UserAttribute &ua, double t, bool interpolate)
{
   TimeSampleList<ScalarProperty> sampler; 
   typename TimeSampleList<ScalarProperty>::ConstIterator samp0, samp1;
   const SrcT *vals0, *vals1;
   DstT *output = 0;
   double blend = 0.0;
   
   if (!sampler.update(prop, t, t, false))
   {
      return false;
   }
   
   if (!sampler.getSamples(t, samp0, samp1, blend))
   {
      return false;
   }
   
   ua.arnoldCategory = AI_USERDEF_CONSTANT;
   ua.dataCount = 1;
   ua.isArray = false;
   
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   vals0 = GetPODPtr(samp0->data());
   
   if (blend > 0.0 && interpolate)
   {
      vals1 = GetPODPtr(samp1->data());
      
      double a = 1.0 - blend;
      double b = blend;
      
      for (unsigned int j=0; j<ua.dataDim; ++j)
      {
         output[j] = TypeHelper<SrcT, DstT>::Blend(ua, vals0[j], vals1[j], a, b);
      }
   }
   else
   {
      for (unsigned int j=0; j<ua.dataDim; ++j)
      {
         output[j] = TypeHelper<SrcT, DstT>::Convert(ua, vals0[j]);
      }
   }
   
   return true;
}

template <typename SrcT, typename DstT, class ArrayProperty>
bool ReadArrayProperty(ArrayProperty prop, UserAttribute &ua, double t, bool interpolate)
{
   TimeSampleList<ArrayProperty> sampler;
   typename TimeSampleList<ArrayProperty>::ConstIterator samp0, samp1;
   const SrcT *vals0, *vals1;
   DstT *output = 0;
   double blend = 0.0;
   
   if (!sampler.update(prop, t, t, false))
   {
      return false;
   }
   
   if (!sampler.getSamples(t, samp0, samp1, blend))
   {
      return false;
   }
   
   // Note: Array may be used as scalar, in that case declare a scalar in arnold
   
   ua.arnoldCategory = AI_USERDEF_CONSTANT;
   
   if (prop.isScalarLike())
   {
      // assert samp0->data()->size() == 1
      ua.isArray = false;
      ua.dataCount = 1;
   }
   else
   {
      ua.isArray = true;
      ua.dataCount = samp0->data()->size();
   }
   
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   vals0 = (const SrcT *) samp0->data()->getData();
   
   if (blend > 0.0 && interpolate && ua.dataCount != samp1->data()->size())
   {
      vals1 = (const SrcT *) samp1->data()->getData();
      
      double a = 1.0 - blend;
      double b = blend;
      
      for (unsigned int i=0, k=0; i<ua.dataCount; ++i)
      {
         for (unsigned int j=0; j<ua.dataDim; ++j, ++k)
         {
            output[k] = TypeHelper<SrcT, DstT>::Blend(ua, vals0[k], vals1[k], a, b);
         }
      }
   }
   else
   {
      for (unsigned int i=0, k=0; i<ua.dataCount; ++i)
      {
         for (unsigned int j=0; j<ua.dataDim; ++j, ++k)
         {
            output[k] = TypeHelper<SrcT, DstT>::Convert(ua, vals0[k]);
         }
      }
   }
   
   return true;
}

template <typename SrcT, typename DstT, class GeomParam>
bool ReadGeomParam(GeomParam param, UserAttribute &ua, double t, bool interpolate)
{
   TimeSampleList<GeomParam> sampler; 
   typename TimeSampleList<GeomParam>::ConstIterator samp0, samp1;
   const SrcT *vals0, *vals1;
   DstT *output = 0;
   double blend = 0.0;
   
   if (!sampler.update(param, t, t, false))
   {
      return false;
   }
   
   if (!sampler.getSamples(t, samp0, samp1, blend))
   {
      return false;
   }
   
   switch (param.getScope())
   {
   case Alembic::AbcGeom::kFacevaryingScope:
      ua.arnoldCategory = AI_USERDEF_INDEXED;
      break;
   case Alembic::AbcGeom::kVertexScope:
   case Alembic::AbcGeom::kVaryingScope:
      ua.arnoldCategory = AI_USERDEF_VARYING;
      break;
   case Alembic::AbcGeom::kUniformScope:
      ua.arnoldCategory = AI_USERDEF_UNIFORM;
      break;
   default:
      ua.arnoldCategory = AI_USERDEF_CONSTANT;
      break;
   }
   
   ua.dataCount = (unsigned int) samp0->data().getVals()->size();
   ua.isArray = (ua.arnoldCategory != AI_USERDEF_CONSTANT ? true : ua.dataCount > 1);
   
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   vals0 = (const SrcT *) samp0->data().getVals()->getData();
   
   if (blend > 0.0 && interpolate && ua.dataCount == samp1->data().getVals()->size())
   {
      vals1 = (const SrcT *) samp1->data().getVals()->getData();
      
      double a = 1.0 - blend;
      double b = blend;
      
      for (unsigned int i=0, k=0; i<ua.dataCount; ++i)
      {
         for (unsigned int j=0; j<ua.dataDim; ++j, ++k)
         {
            output[k] = TypeHelper<SrcT, DstT>::Blend(ua, vals0[k], vals1[k], a, b);
         }
      }
   }
   else
   {
      for (unsigned int i=0, k=0; i<ua.dataCount; ++i)
      {
         for (unsigned int j=0; j<ua.dataDim; ++j, ++k)
         {
            output[k] = TypeHelper<SrcT, DstT>::Convert(ua, vals0[k]);
         }
      }
   }
   
   // SampleUtils only uses getIndexed with kFacevaryingScope geom param
   
   Alembic::Abc::UInt32ArraySamplePtr idxs = samp0->data().getIndices();
   
   if (idxs)
   {
      if (param.getScope() != Alembic::AbcGeom::kFacevaryingScope)
      {
         AiMsgWarning("[abcproc] Ignore non facevarying geo param using indices \"%s\"", param.getName().c_str());
         
         AiFree(ua.data);
         ua.data = 0;
         ua.dataCount = 0;
         ua.isArray = false;
         
         return false;
      }
      
      ua.indicesCount = idxs->size();
      ua.indices = (unsigned int*) AiMalloc(ua.indicesCount * sizeof(unsigned int));
      
      const Alembic::Util::uint32_t *indices = idxs->get();
      
      for (unsigned int i=0; i<ua.indicesCount; ++i)
      {
         ua.indices[i] = indices[i];
      }
   }
   
   return true;
}

template <class Traits, typename SrcT, typename DstT>
bool _ReadUserAttribute(UserAttribute &ua,
                        Alembic::Abc::ICompoundProperty parent,
                        const Alembic::AbcCoreAbstract::PropertyHeader &header,
                        double t,
                        bool geoparam,
                        bool interpolate)
{
   if (geoparam)
   {
      Alembic::AbcGeom::ITypedGeomParam<Traits> param(parent, header.getName());
      
      if (!param.valid())
      {
         return false;
      }
      
      return ReadGeomParam<SrcT, DstT>(param, ua, t, interpolate);
   }
   else
   {
      if (header.isScalar())
      {
         Alembic::Abc::ITypedScalarProperty<Traits> prop(parent, header.getName());
         
         if (!prop.valid())
         {
            return false;
         }
         
         return ReadScalarProperty<SrcT, DstT>(prop, ua, t, interpolate);
      }
      else if (header.isArray())
      {
         Alembic::Abc::ITypedArrayProperty<Traits> prop(parent, header.getName());
         
         if (!prop.valid())
         {
            return false;
         }
         
         return ReadArrayProperty<SrcT, DstT>(prop, ua, t, interpolate);
      }
      else
      {
         return false;
      }
   }
}

bool ReadUserAttribute(UserAttribute &ua,
                       Alembic::Abc::ICompoundProperty parent,
                       const Alembic::AbcCoreAbstract::PropertyHeader &header,
                       double t,
                       bool geoparam,
                       bool interpolate)
{
   ua.abcType = header.getDataType();
   ua.dataDim = ua.abcType.getExtent();
   std::string interp = header.getMetaData().get("interpretation");
   
   switch (ua.abcType.getPod())
   {
   case Alembic::Util::kBooleanPOD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_BOOLEAN;
      ua.arnoldTypeStr = "BOOL";
      return _ReadUserAttribute<Alembic::Abc::BooleanTPTraits, Alembic::Util::bool_t, bool>(ua, parent, header, t, geoparam, interpolate);
   
   case Alembic::Util::kInt8POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_BYTE;
      ua.arnoldTypeStr = "BYTE";
      return _ReadUserAttribute<Alembic::Abc::Int8TPTraits, Alembic::Util::int8_t, AtByte>(ua, parent, header, t, geoparam, interpolate);
   
   case Alembic::Util::kUint8POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_UINT;
      ua.arnoldTypeStr = "UINT";
      return _ReadUserAttribute<Alembic::Abc::Uint8TPTraits, Alembic::Util::uint8_t, unsigned int>(ua, parent, header, t, geoparam, interpolate);
      
   case Alembic::Util::kUint16POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_UINT;
      ua.arnoldTypeStr = "UINT";
      return _ReadUserAttribute<Alembic::Abc::Uint16TPTraits, Alembic::Util::uint16_t, unsigned int>(ua, parent, header, t, geoparam, interpolate);
      
   case Alembic::Util::kUint32POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_UINT;
      ua.arnoldTypeStr = "UINT";
      return _ReadUserAttribute<Alembic::Abc::Uint32TPTraits, Alembic::Util::uint32_t, unsigned int>(ua, parent, header, t, geoparam, interpolate);
      
   case Alembic::Util::kUint64POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_UINT;
      ua.arnoldTypeStr = "UINT";
      return _ReadUserAttribute<Alembic::Abc::Uint64TPTraits, Alembic::Util::uint64_t, unsigned int>(ua, parent, header, t, geoparam, interpolate);
   
   case Alembic::Util::kInt16POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_INT;
      ua.arnoldTypeStr = "INT";
      return _ReadUserAttribute<Alembic::Abc::Int16TPTraits, Alembic::Util::int16_t, int>(ua, parent, header, t, geoparam, interpolate);
      
   case Alembic::Util::kInt32POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_INT;
      ua.arnoldTypeStr = "INT";
      return _ReadUserAttribute<Alembic::Abc::Int32TPTraits, Alembic::Util::int32_t, int>(ua, parent, header, t, geoparam, interpolate);
      
   case Alembic::Util::kInt64POD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_INT;
      ua.arnoldTypeStr = "INT";
      return _ReadUserAttribute<Alembic::Abc::Int64TPTraits, Alembic::Util::int64_t, int>(ua, parent, header, t, geoparam, interpolate);
   
   case Alembic::Util::kFloat16POD:
      switch (ua.dataDim)
      {
      case 1:
         ua.arnoldType = AI_TYPE_FLOAT;
         ua.arnoldTypeStr = "FLOAT";
         return _ReadUserAttribute<Alembic::Abc::Float16TPTraits, Alembic::Util::float16_t, float>(ua, parent, header, t, geoparam, interpolate);   
      case 3:
         if (interp.find("rgb") != std::string::npos)
         {
            ua.arnoldType = AI_TYPE_RGB;
            ua.arnoldTypeStr = "RGB";
            return _ReadUserAttribute<Alembic::Abc::C3hTPTraits, Alembic::Util::float16_t, float>(ua, parent, header, t, geoparam, interpolate);   
         }
         else
         {
            AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float16[3] data", interp.c_str());
            return false;
         }
      case 4:
         if (interp.find("rgba") != std::string::npos)
         {
            ua.arnoldType = AI_TYPE_RGBA;
            ua.arnoldTypeStr = "RGBA";
            return _ReadUserAttribute<Alembic::Abc::C4hTPTraits, Alembic::Util::float16_t, float>(ua, parent, header, t, geoparam, interpolate);   
         }
         else
         {
            AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float16[4] data", interp.c_str());
            return false;
         }
      default:
         AiMsgWarning("[abcproc] Unhandled dimension %u for Float16 POD data", ua.dataDim);
         return false;
      }
   
   case Alembic::Util::kFloat32POD:
      switch (ua.dataDim)
      {
      case 1:
         ua.arnoldType = AI_TYPE_FLOAT;
         ua.arnoldTypeStr = "FLOAT";
         return _ReadUserAttribute<Alembic::Abc::Float32TPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
      case 2:
         ua.arnoldType = AI_TYPE_POINT2;
         ua.arnoldTypeStr = "POINT2";
         if (interp.find("point") != std::string::npos)
         {
            return _ReadUserAttribute<Alembic::Abc::P2fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else if (interp.find("normal") != std::string::npos)
         {
            return _ReadUserAttribute<Alembic::Abc::N2fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else if (interp.find("vector") != std::string::npos)
         {
            return _ReadUserAttribute<Alembic::Abc::V2fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else
         {
            AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float32[2] data", interp.c_str());
            return false;
         }
      case 3:
         if (interp.find("point") != std::string::npos)
         {
            ua.arnoldType = AI_TYPE_POINT;
            ua.arnoldTypeStr = "POINT";
            return _ReadUserAttribute<Alembic::Abc::P3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else if (interp.find("rgb") != std::string::npos)
         {
            ua.arnoldType = AI_TYPE_RGB;
            ua.arnoldTypeStr = "RGB";
            return _ReadUserAttribute<Alembic::Abc::C3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else
         {
            ua.arnoldType = AI_TYPE_VECTOR;
            ua.arnoldTypeStr = "VECTOR";
            if (interp.find("normal") != std::string::npos)
            {
               return _ReadUserAttribute<Alembic::Abc::N3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
            }
            else if (interp.find("vector") != std::string::npos)
            {
               return _ReadUserAttribute<Alembic::Abc::V3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
            }
            else
            {
               AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float32[3] data", interp.c_str());
               return false;
            }
         }
      case 4:
         if (interp.find("rgba") != std::string::npos)
         {
            ua.arnoldType = AI_TYPE_RGBA;
            ua.arnoldTypeStr = "RGBA";
            return _ReadUserAttribute<Alembic::Abc::C4fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else
         {
            AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float32[4] data", interp.c_str());
            return false;
         }
      case 16:
         ua.arnoldType = AI_TYPE_MATRIX;
         ua.arnoldTypeStr = "MATRIX";
         return _ReadUserAttribute<Alembic::Abc::M44fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
      default:
         AiMsgWarning("[abcproc] Unhandled dimension %u for Float32 POD data", ua.dataDim);
         return false;
      }
   
   case Alembic::Util::kFloat64POD:
      switch (ua.dataDim)
      {
      case 1:
         ua.arnoldType = AI_TYPE_FLOAT;
         ua.arnoldTypeStr = "FLOAT";
         return _ReadUserAttribute<Alembic::Abc::Float64TPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
      case 2:
         ua.arnoldType = AI_TYPE_POINT2;
         ua.arnoldTypeStr = "POINT2";
         if (interp.find("point") != std::string::npos)
         {
            return _ReadUserAttribute<Alembic::Abc::P2dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else if (interp.find("normal") != std::string::npos)
         {
            return _ReadUserAttribute<Alembic::Abc::N2dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else if (interp.find("vector") != std::string::npos)
         {
            return _ReadUserAttribute<Alembic::Abc::V2dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else
         {
            AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float64[2] data", interp.c_str());
            return false;
         }
      case 3:
         // no C3dTPTraits (rgb)
         if (interp.find("point") != std::string::npos)
         {
            ua.arnoldType = AI_TYPE_POINT;
            ua.arnoldTypeStr = "POINT";
            return _ReadUserAttribute<Alembic::Abc::P3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
         }
         else
         {
            ua.arnoldType = AI_TYPE_VECTOR;
            ua.arnoldTypeStr = "VECTOR";
            if (interp.find("normal") != std::string::npos)
            {
               return _ReadUserAttribute<Alembic::Abc::N3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
            }
            else if (interp.find("vector") != std::string::npos)
            {
               return _ReadUserAttribute<Alembic::Abc::V3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
            }
            else
            {
               AiMsgWarning("[abcproc] Unhandled interpretation \"%s\" for Float64[3] data", interp.c_str());
               return false;
            }
         }
      case 16:
         ua.arnoldType = AI_TYPE_MATRIX;
         ua.arnoldTypeStr = "MATRIX";
         return _ReadUserAttribute<Alembic::Abc::M44dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
      case 4:
         // QuatdTPTraits no supported
         // no C4dTPTraits
      default:
         AiMsgWarning("[abcproc] Unhandled dimension %u for Float64 POD data", ua.dataDim);
         return false;
      }
   
   case Alembic::Util::kStringPOD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_STRING;
      ua.arnoldTypeStr = "STRING";
      return _ReadUserAttribute<Alembic::Abc::StringTPTraits, std::string, const char*>(ua, parent, header, t, geoparam, interpolate);
   
   default:
      AiMsgWarning("[abcproc] Unhandled POD type");
      return false;
   }
}

// ---

template <int ArnoldType, typename T>
void _NodeSet(AtNode *, const char *, T *)
{
}

template <int ArnoldType, typename T>
void _ArraySet(AtArray *, unsigned int, T *, unsigned int *)
{
}


template <>
void _NodeSet<AI_TYPE_BOOLEAN, bool>(AtNode *node, const char *name, bool *vals)
{
   AiNodeSetBool(node, name, vals[0]);
}

template <>
void _NodeSet<AI_TYPE_BYTE, AtByte>(AtNode *node, const char *name, AtByte *vals)
{
   AiNodeSetBool(node, name, vals[0]);
}

template <>
void _NodeSet<AI_TYPE_INT, int>(AtNode *node, const char *name, int *vals)
{
   AiNodeSetInt(node, name, vals[0]);
}

template <>
void _NodeSet<AI_TYPE_INT, unsigned int>(AtNode *node, const char *name, unsigned int *vals)
{
   AiNodeSetUInt(node, name, vals[0]);
}

template <>
void _NodeSet<AI_TYPE_FLOAT, float>(AtNode *node, const char *name, float *vals)
{
   AiNodeSetFlt(node, name, vals[0]);
}

template <>
void _NodeSet<AI_TYPE_POINT2, float>(AtNode *node, const char *name, float *vals)
{
   AiNodeSetPnt2(node, name, vals[0], vals[1]);
}

template <>
void _NodeSet<AI_TYPE_POINT, float>(AtNode *node, const char *name, float *vals)
{
   AiNodeSetPnt(node, name, vals[0], vals[1], vals[2]);
}

template <>
void _NodeSet<AI_TYPE_VECTOR, float>(AtNode *node, const char *name, float *vals)
{
   AiNodeSetVec(node, name, vals[0], vals[1], vals[2]);
}

template <>
void _NodeSet<AI_TYPE_RGB, float>(AtNode *node, const char *name, float *vals)
{
   AiNodeSetRGB(node, name, vals[0], vals[1], vals[2]);
}

template <>
void _NodeSet<AI_TYPE_RGBA, float>(AtNode *node, const char *name, float *vals)
{
   AiNodeSetRGBA(node, name, vals[0], vals[1], vals[2], vals[3]);
}

template <>
void _NodeSet<AI_TYPE_MATRIX, float>(AtNode *node, const char *name, float *vals)
{
   AtMatrix mat;
   mat[0][0] = vals[0];
   mat[0][1] = vals[1];
   mat[0][2] = vals[2];
   mat[0][3] = vals[3];
   mat[1][0] = vals[4];
   mat[1][1] = vals[5];
   mat[1][2] = vals[6];
   mat[1][3] = vals[7];
   mat[2][0] = vals[8];
   mat[2][1] = vals[9];
   mat[2][2] = vals[10];
   mat[2][3] = vals[11];
   mat[3][0] = vals[12];
   mat[3][1] = vals[13];
   mat[3][2] = vals[14];
   mat[3][3] = vals[15];
   AiNodeSetMatrix(node, name, mat);
}

template <>
void _NodeSet<AI_TYPE_STRING, const char*>(AtNode *node, const char *name, const char* *vals)
{
   AiNodeSetStr(node, name, vals[0]);
}


template <>
void _ArraySet<AI_TYPE_BOOLEAN, bool>(AtArray *ary, unsigned int count, bool *vals, unsigned int *idxs)
{
   if (idxs)
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetBool(ary, i, vals[idxs[i]]);
      }
   }
   else
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetBool(ary, i, vals[i]);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_BYTE, AtByte>(AtArray *ary, unsigned int count, AtByte *vals, unsigned int *idxs)
{
   if (idxs)
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetByte(ary, i, vals[idxs[i]]);
      }
   }
   else
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetByte(ary, i, vals[i]);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_INT, int>(AtArray *ary, unsigned int count, int *vals, unsigned int *idxs)
{
   if (idxs)
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetInt(ary, i, vals[idxs[i]]);
      }
   }
   else
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetInt(ary, i, vals[i]);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_UINT, unsigned int>(AtArray *ary, unsigned int count, unsigned int *vals, unsigned int *idxs)
{
   if (idxs)
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetUInt(ary, i, vals[idxs[i]]);
      }
   }
   else
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetUInt(ary, i, vals[i]);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_FLOAT, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   if (idxs)
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetFlt(ary, i, vals[idxs[i]]);
      }
   }
   else
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetFlt(ary, i, vals[i]);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_POINT2, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   AtPoint2 pnt;
   if (idxs)
   {
      for (unsigned int i=0, j=0; i<count; ++i)
      {
         j = idxs[i] * 2;
         pnt.x = vals[j+0];
         pnt.y = vals[j+1];
         AiArraySetPnt2(ary, i, pnt);
      }
   }
   else
   {
      for (unsigned int i=0, j=0; i<count; ++i, j+=2)
      {
         pnt.x = vals[j+0];
         pnt.y = vals[j+1];
         AiArraySetPnt2(ary, i, pnt);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_POINT, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   AtPoint pnt;
   if (idxs)
   {
      for (unsigned int i=0, j=0; i<count; ++i)
      {
         j = idxs[i] * 3;
         pnt.x = vals[j+0];
         pnt.y = vals[j+1];
         pnt.z = vals[j+2];
         AiArraySetPnt(ary, i, pnt);
      }
   }
   else
   {
      for (unsigned int i=0, j=0; i<count; ++i, j+=3)
      {
         pnt.x = vals[j+0];
         pnt.y = vals[j+1];
         pnt.z = vals[j+2];
         AiArraySetPnt(ary, i, pnt);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_VECTOR, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   AtVector vec;
   if (idxs)
   {
      for (unsigned int i=0, j=0; i<count; ++i)
      {
         j = idxs[i] * 3;
         vec.x = vals[j+0];
         vec.y = vals[j+1];
         vec.z = vals[j+2];
         AiArraySetVec(ary, i, vec);
      }
   }
   else
   {
      for (unsigned int i=0, j=0; i<count; ++i, j+=3)
      {
         vec.x = vals[j+0];
         vec.y = vals[j+1];
         vec.z = vals[j+2];
         AiArraySetVec(ary, i, vec);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_RGB, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   AtRGB col;
   if (idxs)
   {
      for (unsigned int i=0, j=0; i<count; ++i)
      {
         j = idxs[i] * 3;
         col.r = vals[j+0];
         col.g = vals[j+1];
         col.b = vals[j+2];
         AiArraySetRGB(ary, i, col);
      }
   }
   else
   {
      for (unsigned int i=0, j=0; i<count; ++i, j+=3)
      {
         col.r = vals[j+0];
         col.g = vals[j+1];
         col.b = vals[j+2];
         AiArraySetRGB(ary, i, col);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_RGBA, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   AtRGBA col;
   if (idxs)
   {
      for (unsigned int i=0, j=0; i<count; ++i)
      {
         j = idxs[i] * 4;
         col.r = vals[j+0];
         col.g = vals[j+1];
         col.b = vals[j+2];
         col.a = vals[j+3];
         AiArraySetRGBA(ary, i, col);
      }
   }
   else
   {
      for (unsigned int i=0, j=0; i<count; ++i, j+=4)
      {
         col.r = vals[j+0];
         col.g = vals[j+1];
         col.b = vals[j+2];
         col.a = vals[j+3];
         AiArraySetRGBA(ary, i, col);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_MATRIX, float>(AtArray *ary, unsigned int count, float *vals, unsigned int *idxs)
{
   AtMatrix mat;
   if (idxs)
   {
      for (unsigned int i=0, j=0; i<count; ++i)
      {
         j = idxs[i] * 16;
         mat[0][0] = vals[j+0];
         mat[0][1] = vals[j+1];
         mat[0][2] = vals[j+2];
         mat[0][3] = vals[j+3];
         mat[1][0] = vals[j+4];
         mat[1][1] = vals[j+5];
         mat[1][2] = vals[j+6];
         mat[1][3] = vals[j+7];
         mat[2][0] = vals[j+8];
         mat[2][1] = vals[j+9];
         mat[2][2] = vals[j+10];
         mat[2][3] = vals[j+11];
         mat[3][0] = vals[j+12];
         mat[3][1] = vals[j+13];
         mat[3][2] = vals[j+14];
         mat[3][3] = vals[j+15];
         AiArraySetMtx(ary, i, mat);
      }
   }
   else
   {
      for (unsigned int i=0, j=0; i<count; ++i, j+=16)
      {
         mat[0][0] = vals[j+0];
         mat[0][1] = vals[j+1];
         mat[0][2] = vals[j+2];
         mat[0][3] = vals[j+3];
         mat[1][0] = vals[j+4];
         mat[1][1] = vals[j+5];
         mat[1][2] = vals[j+6];
         mat[1][3] = vals[j+7];
         mat[2][0] = vals[j+8];
         mat[2][1] = vals[j+9];
         mat[2][2] = vals[j+10];
         mat[2][3] = vals[j+11];
         mat[3][0] = vals[j+12];
         mat[3][1] = vals[j+13];
         mat[3][2] = vals[j+14];
         mat[3][3] = vals[j+15];
         AiArraySetMtx(ary, i, mat);
      }
   }
}

template <>
void _ArraySet<AI_TYPE_STRING, const char*>(AtArray *ary, unsigned int count, const char* *vals, unsigned int *idxs)
{
   if (idxs)
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetStr(ary, i, vals[idxs[i]]);
      }
   }
   else
   {
      for (unsigned int i=0; i<count; ++i)
      {
         AiArraySetStr(ary, i, vals[i]);
      }
   }
}


template <int ArnoldType, typename T>
void _SetUserAttribute(AtNode *node, const std::string &valName, const std::string &idxName, UserAttribute &ua, unsigned int *remapIndices)
{
   T *vals = (T*) ua.data;
   
   //if (ua.dataCount >= 1 || ua.indices)
   if (ua.arnoldCategory != AI_USERDEF_CONSTANT || ua.isArray)
   {
      if (idxName.length() > 0)
      {
         // AI_USERDEF_INDEXED case
         
         AtArray *valAry = AiArrayAllocate(ua.dataCount, 1, ArnoldType);
         
         _ArraySet<ArnoldType, T>(valAry, ua.dataCount, vals, 0);
         
         AiNodeSetArray(node, valName.c_str(), valAry);
         
         if (!ua.indices)
         {
            // geom param was fully expanded, build indices
            
            ua.indicesCount = ua.dataCount;
            ua.indices = (unsigned int*) AiMalloc(ua.indicesCount * sizeof(unsigned int));
            
            if (remapIndices)
            {
               for (unsigned int i=0; i<ua.indicesCount; ++i)
               {
                  ua.indices[remapIndices[i]] = i;
               }
            }
            else
            {
               for (unsigned int i=0; i<ua.indicesCount; ++i)
               {
                  ua.indices[i] = i;
               }
            }
         }
         else if (remapIndices)
         {
            unsigned int *indices = (unsigned int*) AiMalloc(ua.indicesCount * sizeof(unsigned int));
            
            for (unsigned int i=0; i<ua.indicesCount; ++i)
            {
               indices[remapIndices[i]] = ua.indices[i];
            }
            
            AiFree(ua.indices);
            ua.indices = indices;
         }
         
         AtArray *idxAry = AiArrayAllocate(ua.indicesCount, 1, AI_TYPE_UINT);
         
         _ArraySet<AI_TYPE_UINT, unsigned int>(idxAry, ua.indicesCount, ua.indices, 0);
         
         AiNodeSetArray(node, idxName.c_str(), idxAry);
      }
      else
      {
         if (ua.indices)
         {
            // Non facevarying attribute with indices
            // This should not happen in theory as SampleUtils only query indices for kFacevaryingScope
            // (Do no apply remapping in this case)
            AiMsgWarning("[abcproc] Setting non facevarying attribute with indexed values");
            
            AtArray *valAry = AiArrayAllocate(ua.indicesCount, 1, ArnoldType);
            
            _ArraySet<ArnoldType, T>(valAry, ua.indicesCount, vals, ua.indices);
            
            AiNodeSetArray(node, valName.c_str(), valAry);
         }
         else
         {
            AtArray *valAry = AiArrayAllocate(ua.dataCount, 1, ArnoldType);
            
            _ArraySet<ArnoldType, T>(valAry, ua.dataCount, vals, 0);
            
            AiNodeSetArray(node, valName.c_str(), valAry);
         }
      }
   }
   else
   {
      _NodeSet<ArnoldType, T>(node, valName.c_str(), vals);
   }
}

void SetUserAttribute(AtNode *node, const char *name, UserAttribute &ua, unsigned int *remapIndices)
{
   std::string decl;
   std::string valsName = name;
   std::string idxsName = "";
   
   if (ua.arnoldCategory == AI_USERDEF_CONSTANT)
   {
      decl += "constant ";
      if (ua.isArray)
      {
         decl += "ARRAY ";
      }
      decl += ua.arnoldTypeStr;
   }
   else if (ua.arnoldCategory == AI_USERDEF_UNIFORM)
   {
      decl = "uniform " + ua.arnoldTypeStr;
   }
   else if (ua.arnoldCategory == AI_USERDEF_VARYING)
   {
      decl = "varying " + ua.arnoldTypeStr;
   }
   else
   {
      decl = "indexed " + ua.arnoldTypeStr;
      idxsName = valsName + "idxs";
   }
   
   const AtParamEntry *pe = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(node), name);
   if (pe == 0)
   {
      const AtUserParamEntry *upe = AiNodeLookUpUserParameter(node, name);
      
      if (upe == 0)
      {
         AiNodeDeclare(node, name, decl.c_str());
      }
      else
      {
         bool isArray = (AiUserParamGetType(upe) == AI_TYPE_ARRAY);
         int type = (isArray ? AiUserParamGetArrayType(upe) : AiUserParamGetType(upe));
         
         if (type != ua.arnoldType ||
             AiUserParamGetCategory(upe) != ua.arnoldCategory,
             ua.isArray != isArray)
         {
            // Attribute spec mismatch
            return;
         }
      }
   }
   
   switch (ua.arnoldType)
   {
   case AI_TYPE_BOOLEAN:
      _SetUserAttribute<AI_TYPE_BOOLEAN, bool>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_BYTE:
      _SetUserAttribute<AI_TYPE_BYTE, AtByte>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_INT:
      _SetUserAttribute<AI_TYPE_INT, int>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_UINT:
      _SetUserAttribute<AI_TYPE_UINT, unsigned int>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_FLOAT:
      _SetUserAttribute<AI_TYPE_FLOAT, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_POINT2:
      _SetUserAttribute<AI_TYPE_POINT2, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_POINT:
      _SetUserAttribute<AI_TYPE_POINT, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_VECTOR:
      _SetUserAttribute<AI_TYPE_VECTOR, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_RGB:
      _SetUserAttribute<AI_TYPE_RGB, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_RGBA:
      _SetUserAttribute<AI_TYPE_RGBA, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_MATRIX:
      _SetUserAttribute<AI_TYPE_MATRIX, float>(node, valsName, idxsName, ua, remapIndices);
      break;
   case AI_TYPE_STRING:
      _SetUserAttribute<AI_TYPE_STRING, const char*>(node, valsName, idxsName, ua, remapIndices);
   default:
      break;
   }
}

void SetUserAttributes(AtNode *node, UserAttributes &attribs, unsigned int *remapIndices)
{
   UserAttributes::iterator it = attribs.begin();
   
   while (it != attribs.end())
   {
      SetUserAttribute(node, it->first.c_str(), it->second, remapIndices);
      ++it;
   }
}
