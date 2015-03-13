#include "userattr.h"

#include "userattr.h"
#include <SampleUtils.h>

static size_t DataTypeByteSize[Undefined_Type+1] = {sizeof(bool),
                                                    sizeof(int),
                                                    sizeof(unsigned int),
                                                    sizeof(float),
                                                    sizeof(const char*),
                                                    0};

void InitUserAttribute(UserAttribute &attrib)
{
   attrib.abcType = Alembic::AbcCoreAbstract::DataType();
   attrib.scope = Constant_Scope;
   attrib.usage = Undefined_Usage;
   attrib.isArray = false;
   attrib.dataType = Undefined_Type;
   attrib.dataDim = 0;
   attrib.dataCount = 0;
   attrib.data = 0;
   attrib.indicesCount = 0;
   attrib.indices = 0;
   attrib.strings.clear();
}

bool ResizeUserAttribute(UserAttribute &ua, unsigned int newSize)
{
   // Do not handled resizing for indexed user attribute
   if (ua.indicesCount > 0)
   {
      return false;
   }
   
   size_t elemSize = ua.dataDim * DataTypeByteSize[ua.dataType];
   size_t oldBytesize = elemSize * (ua.data ? ua.dataCount : 0);
   size_t newBytesize = elemSize * newSize;
   
   if (newBytesize == 0)
   {
      if (ua.data)
      {
         free(ua.data);
         ua.data = 0;
         ua.dataCount = 0;
      }
   }
   else
   {
      size_t copySize = (oldBytesize > newBytesize ? newBytesize : oldBytesize);
      size_t resetSize = (newBytesize > oldBytesize ? (newBytesize - oldBytesize) : 0);
      
      unsigned char *oldBytes = (unsigned char*) ua.data;
      unsigned char *newBytes = (unsigned char*) malloc(newBytesize);
      
      if (oldBytes)
      {
         memcpy(newBytes, oldBytes, copySize);
         free(oldBytes);
      }
      
      if (ua.dataType == String_Type)
      {
         const char** newElem = (const char**)(newBytes + oldBytesize);
         const char** lastElem = (const char**)(newBytes + newBytesize);
         
         std::set<std::string>::iterator it = ua.strings.insert("").first;
         const char *strDef = it->c_str();
         
         while (newElem < lastElem)
         {
            for (unsigned int i=0; i<ua.dataDim; ++i)
            {
               newElem[i] = strDef;
            }
            newElem += elemSize;
         }
      }
      else
      {
         memset(newBytes + oldBytesize, 0, resetSize);
      }
      
      ua.data = newBytes;
      ua.dataCount = newSize;
   }
   
   return true;
}

bool CopyUserAttribute(UserAttribute &src, unsigned int srcIdx, unsigned int count,
                       UserAttribute &dst, unsigned int dstIdx)
{
   if (src.dataType != dst.dataType ||
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
   
   if (src.dataType == String_Type)
   {
      // Note: dataDim should always be 1 for strings
      const char** srcStrs = ((const char**) src.data) + (srcIdx * src.dataDim);
      const char** dstStrs = ((const char**) dst.data) + (dstIdx * dst.dataDim);
      
      for (unsigned int i=0; i<count; ++i, ++srcStrs, ++dstStrs)
      {
         for (unsigned int j=0; j<dst.dataDim; ++j)
         {
            dstStrs[j] = dst.strings.insert(srcStrs[j]).first->c_str();
         }
      }
   }
   else
   {
      size_t elemSize = src.dataDim * DataTypeByteSize[src.dataType];
      
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
      free(attrib.data);
   }
   
   if (attrib.indices)
   {
      free(attrib.indices);
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
      ua.data = malloc(ua.dataCount * ua.dataDim * sizeof(DstT));
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
      ua.data = malloc(ua.dataCount * ua.dataDim * sizeof(T));
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
      ua.data = malloc(ua.dataCount * ua.dataDim * sizeof(bool));
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
      ua.data = malloc(ua.dataCount * ua.dataDim * sizeof(bool));
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
      ua.data = malloc(ua.dataCount * ua.dataDim * sizeof(const char*));
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
bool ReadScalarProperty(ScalarProperty prop, UserAttribute &ua, double t, bool interpolate, bool verbose=false)
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
   
   ua.scope = Constant_Scope;
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
bool ReadArrayProperty(ArrayProperty prop, UserAttribute &ua, double t, bool interpolate, bool verbose=false)
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
   
   // Note: Array may be used as scalar, in that case declare as scalar
   
   ua.scope = Constant_Scope;
   
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
   
   if (verbose)
   {
      std::cout << "[AlembicLoader] ReadArrayProperty: " << ua.dataCount << " value(s)" << std::endl;
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
bool ReadGeomParam(GeomParam param, UserAttribute &ua, double t, bool interpolate, bool verbose=false)
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
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: Per vertex" << std::endl;
      }
      ua.scope = FaceVarying_Scope;
      break;
   case Alembic::AbcGeom::kVertexScope:
   case Alembic::AbcGeom::kVaryingScope:
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: Per point" << std::endl;
      }
      ua.scope = Varying_Scope;
      break;
   case Alembic::AbcGeom::kUniformScope:
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: Per face" << std::endl;
      }
      ua.scope = Uniform_Scope;
      break;
   default:
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: Constant" << std::endl;
      }
      ua.scope = Constant_Scope;
      break;
   }
   
   ua.dataCount = (unsigned int) samp0->data().getVals()->size();
   ua.isArray = (ua.scope != Constant_Scope ? true : ua.dataCount > 1);
   
   if (verbose)
   {
      std::cout << "[AlembicLoader] ReadGeomParam: " << ua.dataCount << " value(s)" << std::endl;
   }
   
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   vals0 = (const SrcT *) samp0->data().getVals()->getData();
   
   if (blend > 0.0 && interpolate && ua.dataCount == samp1->data().getVals()->size())
   {
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: Interpolate values: t0=" << samp0->time() << ", t1=" << samp1->time() << ", blend=" << blend << std::endl;
      }
      
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
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: Use values at t=" << samp0->time() << std::endl;
      }
      
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
         std::cout << "[AlembicLoader] ReadGeomParam: Ignore non facevarying geo param using indices \"" << param.getName() << "\"" << std::endl;
         
         free(ua.data);
         ua.data = 0;
         ua.dataCount = 0;
         ua.isArray = false;
         
         return false;
      }
      
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadGeomParam: " << idxs->size() << " indices" << std::endl;
      }
      
      ua.indicesCount = idxs->size();
      ua.indices = (unsigned int*) malloc(ua.indicesCount * sizeof(unsigned int));
      
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
                        bool interpolate,
                        bool verbose)
{
   if (geoparam)
   {
      Alembic::AbcGeom::ITypedGeomParam<Traits> param(parent, header.getName());
      
      if (!param.valid())
      {
         return false;
      }
      
      if (verbose)
      {
         std::cout << "[AlembicLoader] ReadUserAttribute: Read geometry param \"" << header.getName() << "\"..." << std::endl;
      }
      
      return ReadGeomParam<SrcT, DstT>(param, ua, t, interpolate, verbose);
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
         
         if (verbose)
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Read scalar property \"" << header.getName() << "\"..." << std::endl;
         }
         
         return ReadScalarProperty<SrcT, DstT>(prop, ua, t, interpolate, verbose);
      }
      else if (header.isArray())
      {
         Alembic::Abc::ITypedArrayProperty<Traits> prop(parent, header.getName());
         
         if (!prop.valid())
         {
            return false;
         }
         
         if (verbose)
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Read array property \"" << header.getName() << "\"..." << std::endl;
         }
         
         return ReadArrayProperty<SrcT, DstT>(prop, ua, t, interpolate, verbose);
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
                       bool interpolate,
                       bool verbose)
{
   std::string interp = "";
   
   if (geoparam && header.isCompound())
   {
      Alembic::Abc::ICompoundProperty cprop(parent, header.getName());
      
      const Alembic::AbcCoreAbstract::PropertyHeader *vh = cprop.getPropertyHeader(".vals");
      const Alembic::AbcCoreAbstract::PropertyHeader *ih = cprop.getPropertyHeader(".indices");
      
      if (!vh || !ih)
      {
         std::cout << "[AlembicLoader] ReadUserAttribute: \"" << header.getName() << "\" attribute is an unsupported compound" << std::endl;
         return false;
      }
      
      ua.abcType = vh->getDataType();
      interp = vh->getMetaData().get("interpretation");
   }
   else
   {
      ua.abcType = header.getDataType();
      interp = header.getMetaData().get("interpretation");
   }
   
   ua.usage = Undefined_Usage;
   ua.dataDim = ua.abcType.getExtent();
   
   switch (ua.abcType.getPod())
   {
   case Alembic::Util::kBooleanPOD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Bool_Type;
      return _ReadUserAttribute<Alembic::Abc::BooleanTPTraits, Alembic::Util::bool_t, bool>(ua, parent, header, t, geoparam, interpolate, verbose);
   
   case Alembic::Util::kUint8POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Uint_Type;
      return _ReadUserAttribute<Alembic::Abc::Uint8TPTraits, Alembic::Util::uint8_t, unsigned int>(ua, parent, header, t, geoparam, interpolate, verbose);
      
   case Alembic::Util::kUint16POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Uint_Type;
      return _ReadUserAttribute<Alembic::Abc::Uint16TPTraits, Alembic::Util::uint16_t, unsigned int>(ua, parent, header, t, geoparam, interpolate, verbose);
      
   case Alembic::Util::kUint32POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Uint_Type;
      return _ReadUserAttribute<Alembic::Abc::Uint32TPTraits, Alembic::Util::uint32_t, unsigned int>(ua, parent, header, t, geoparam, interpolate, verbose);
      
   case Alembic::Util::kUint64POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Uint_Type;
      return _ReadUserAttribute<Alembic::Abc::Uint64TPTraits, Alembic::Util::uint64_t, unsigned int>(ua, parent, header, t, geoparam, interpolate, verbose);
   
   case Alembic::Util::kInt8POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Int_Type;
      return _ReadUserAttribute<Alembic::Abc::Int8TPTraits, Alembic::Util::int8_t, int>(ua, parent, header, t, geoparam, interpolate, verbose);
   
   case Alembic::Util::kInt16POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Int_Type;
      return _ReadUserAttribute<Alembic::Abc::Int16TPTraits, Alembic::Util::int16_t, int>(ua, parent, header, t, geoparam, interpolate, verbose);
      
   case Alembic::Util::kInt32POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Int_Type;
      return _ReadUserAttribute<Alembic::Abc::Int32TPTraits, Alembic::Util::int32_t, int>(ua, parent, header, t, geoparam, interpolate, verbose);
      
   case Alembic::Util::kInt64POD:
      if (ua.dataDim != 1) return false;
      ua.dataType = Int_Type;
      return _ReadUserAttribute<Alembic::Abc::Int64TPTraits, Alembic::Util::int64_t, int>(ua, parent, header, t, geoparam, interpolate, verbose);
   
   case Alembic::Util::kFloat16POD:
      ua.dataType = Float_Type;
      switch (ua.dataDim)
      {
      case 1:
         return _ReadUserAttribute<Alembic::Abc::Float16TPTraits, Alembic::Util::float16_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);   
      case 3:
         if (interp.find("rgb") != std::string::npos)
         {
            ua.usage = Use_As_Color;
            return _ReadUserAttribute<Alembic::Abc::C3hTPTraits, Alembic::Util::float16_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);   
         }
         else
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float16[3] data for \"" << header.getName() << "\" attribute" << std::endl;
            return false;
         }
      case 4:
         if (interp.find("rgba") != std::string::npos)
         {
            ua.usage = Use_As_Color;
            return _ReadUserAttribute<Alembic::Abc::C4hTPTraits, Alembic::Util::float16_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);   
         }
         else
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float16[4] data for \"" << header.getName() << "\" attribute" << std::endl;
            return false;
         }
      default:
         std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled dimension " << ua.dataDim << " for Float16 POD data for \"" << header.getName() << "\" attribute" << std::endl;
         return false;
      }
      break;
   
   case Alembic::Util::kFloat32POD:
      ua.dataType = Float_Type;
      switch (ua.dataDim)
      {
      case 1:
         return _ReadUserAttribute<Alembic::Abc::Float32TPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
      case 2:
         if (interp.find("point") != std::string::npos)
         {
            ua.usage = Use_As_Point;
            return _ReadUserAttribute<Alembic::Abc::P2fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else if (interp.find("normal") != std::string::npos)
         {
            ua.usage = Use_As_Normal;
            return _ReadUserAttribute<Alembic::Abc::N2fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else if (interp.find("vector") != std::string::npos)
         {
            ua.usage = Use_As_Vector;
            return _ReadUserAttribute<Alembic::Abc::V2fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float32[2] data for \"" << header.getName() << "\" attribute" << std::endl;
            return false;
         }
      case 3:
         if (interp.find("point") != std::string::npos)
         {
            ua.usage = Use_As_Point;
            return _ReadUserAttribute<Alembic::Abc::P3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else if (interp.find("rgb") != std::string::npos)
         {
            ua.usage = Use_As_Color;
            return _ReadUserAttribute<Alembic::Abc::C3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else
         {
            if (interp.find("normal") != std::string::npos)
            {
               ua.usage = Use_As_Normal;
               return _ReadUserAttribute<Alembic::Abc::N3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
            }
            else if (interp.find("vector") != std::string::npos)
            {
               ua.usage = Use_As_Vector;
               return _ReadUserAttribute<Alembic::Abc::V3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
            }
            else
            {
               std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float32[3] data for \"" << header.getName() << "\" attribute" << std::endl;
               return false;
            }
         }
      case 4:
         if (interp.find("rgba") != std::string::npos)
         {
            ua.usage = Use_As_Color;
            return _ReadUserAttribute<Alembic::Abc::C4fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float32[4] data for \"" << header.getName() << "\" attribute" << std::endl;
            return false;
         }
      case 16:
         ua.usage = Use_As_Matrix;
         return _ReadUserAttribute<Alembic::Abc::M44fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
      default:
         std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled dimension " << ua.dataDim << " for Float32 POD data for \"" << header.getName() << "\" attribute" << std::endl;
         return false;
      }
      break;
   
   case Alembic::Util::kFloat64POD:
      ua.dataType = Float_Type;
      switch (ua.dataDim)
      {
      case 1:
         return _ReadUserAttribute<Alembic::Abc::Float64TPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
      case 2:
         if (interp.find("point") != std::string::npos)
         {
            ua.usage = Use_As_Point;
            return _ReadUserAttribute<Alembic::Abc::P2dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else if (interp.find("normal") != std::string::npos)
         {
            ua.usage = Use_As_Normal;
            return _ReadUserAttribute<Alembic::Abc::N2dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else if (interp.find("vector") != std::string::npos)
         {
            ua.usage = Use_As_Vector;
            return _ReadUserAttribute<Alembic::Abc::V2dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else
         {
            std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float64[2] data for \"" << header.getName() << "\" attribute" << std::endl;
            return false;
         }
      case 3:
         // no C3dTPTraits (rgb)
         if (interp.find("point") != std::string::npos)
         {
            ua.usage = Use_As_Point;
            return _ReadUserAttribute<Alembic::Abc::P3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
         }
         else
         {
            if (interp.find("normal") != std::string::npos)
            {
               ua.usage = Use_As_Normal;
               return _ReadUserAttribute<Alembic::Abc::N3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
            }
            else if (interp.find("vector") != std::string::npos)
            {
               ua.usage = Use_As_Vector;
               return _ReadUserAttribute<Alembic::Abc::V3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
            }
            else
            {
               std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled interpretation \"" << interp << "\" for Float64[3] data for \"" << header.getName() << "\" attribute" << std::endl;
               return false;
            }
         }
      case 16:
         ua.usage = Use_As_Matrix;
         return _ReadUserAttribute<Alembic::Abc::M44dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate, verbose);
      case 4:
         // QuatdTPTraits no supported
         // no C4dTPTraits
      default:
         std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled dimension " << ua.dataDim << " for Float64 POD data for \"" << header.getName() << "\" attribute" << std::endl;
         return false;
      }
      break;
   
   case Alembic::Util::kStringPOD:
      if (ua.dataDim != 1) return false;
      ua.dataType = String_Type;
      return _ReadUserAttribute<Alembic::Abc::StringTPTraits, std::string, const char*>(ua, parent, header, t, geoparam, interpolate, verbose);
   
   default:
      std::cout << "[AlembicLoader] ReadUserAttribute: Unhandled POD type (" << ua.abcType.getPod() << ") for \"" << header.getName() << "\" attribute" << std::endl;
      return false;
   }
   
   return false;
}

void SetUserAttributes(AlembicGeometrySource::GeomInfo *geom, UserAttributes &attribs, double t, bool verbose)
{
   //if (!geom || !geom->channelNames)
   if (!geom)
   {
      return;
   }
   
   std::set<std::string> usedNames;
   
   if (geom->channelNames)
   {
      for (int i=0; i<geom->channelNames->getCount(t); ++i)
      {
         tchar *channelName = geom->channelNames->getString(i, t);
         if (channelName)
         {
            usedNames.insert(std::string(channelName));
         }
      }
   }
   
   UserAttributes::iterator it = attribs.begin();
   
   while (it != attribs.end())
   {
      if (usedNames.find(it->first.c_str()) != usedNames.end())
      {
         std::cout << "[AlembicLoader] SetUserAttributes: User defined channel '" << it->first.c_str() << "' already defined" << std::endl;
         continue;
      }
      
      if (SetUserAttribute(geom, it->first.c_str(), it->second, t, usedNames, verbose) && it->second.scope != Constant_Scope)
      {
         if (geom->channelNames)
         {
            int idx = geom->channelNames->getCount(t);
            
            geom->channelNames->setCount(idx+1, t);
            geom->channelNames->setString(it->first.c_str(), idx, t);
            
            if (verbose)
            {
               std::cout << "[AlembicLoader] SetUserAttribtues: Added user defined channel \"" << it->first << "\"" << std::endl;
            }
         }
         
         usedNames.insert(it->first.c_str());
      }
      
      ++it;
   }
}

bool SetUserAttribute(AlembicGeometrySource::GeomInfo *geom, const char *name, UserAttribute &ua, double t, std::set<std::string> &usedNames, bool verbose)
{
   if (!geom || !ua.data)
   {
      return false;
   }
   
   // Type check
   if (ua.dataType < 0 || ua.dataType > String_Type)
   {
      std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Undefined user attribute type" << std::endl;
      return false;
   }
   else if (ua.dataType == String_Type)
   {
      std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" String user attribute not supported by V-Ray" << std::endl;
      return false;
   }
   
   // Dimension check
   if (ua.dataType != Float_Type)
   {
      if (ua.dataDim != 1)
      {
         std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Unsupported data dimension" << std::endl;
         return false;
      }
   }
   else
   {
      if (ua.dataDim < 1 || ua.dataDim > 4)
      {
         std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Unsupported data dimension" << std::endl;
         return false;
      }  
   }
   
   if (ua.scope == Constant_Scope)
   {
      if (ua.isArray)
      {
         return false;
      }
      
      char tmp[512];
      
      switch (ua.dataType)
      {
      case Bool_Type:
         {
            bool *bdata = (bool*) ua.data;
            
            if (geom->userAttr.length() > 0)
            {
               geom->userAttr += ";";
            }
            geom->userAttr += name;
            geom->userAttr += (*bdata ? "=1" : "=0");
         }
         break;
      case Int_Type:
         {
            int *idata = (int*) ua.data;
            sprintf(tmp, "%d", *idata);
            
            if (geom->userAttr.length() > 0)
            {
               geom->userAttr += ";";
            }
            geom->userAttr += name;
            geom->userAttr += "=";
            geom->userAttr += tmp;
         }
         break;
      case Uint_Type:
         {
            unsigned int *udata = (unsigned int*) ua.data;
            sprintf(tmp, "%u", *udata);
            
            if (geom->userAttr.length() > 0)
            {
               geom->userAttr += ";";
            }
            geom->userAttr += name;
            geom->userAttr += "=";
            geom->userAttr += tmp;
         }
         break;
      case Float_Type:
         {
            float *fdata = (float*) ua.data;
            switch (ua.dataDim)
            {
            case 4:
               sprintf(tmp, "%f,%f,%f,%f", fdata[0], fdata[1], fdata[2], fdata[3]);
               break;
            case 3:
               sprintf(tmp, "%f,%f,%f", fdata[0], fdata[1], fdata[2]);
               break;
            case 2:
               sprintf(tmp, "%f,%f", fdata[0], fdata[1]);
               break;
            default:
               sprintf(tmp, "%f", *fdata);
            }
            
            if (geom->userAttr.length() > 0)
            {
               geom->userAttr += ";";
            }
            geom->userAttr += name;
            geom->userAttr += "=";
            geom->userAttr += tmp;
         }
         break;
      default:
         std::cerr << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Unsupported data type" << std::endl;
         return false;
      }
      
      #ifdef _DEBUG
      std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Updated user attribute string to \"" << geom->userAttr << "\"" << std::endl;
      #endif
      
      return true;
   }
   else if (ua.scope >= Uniform_Scope && ua.scope <= FaceVarying_Scope)
   {
      if (geom->channels)
      {
         if (ua.scope == Uniform_Scope)
         {
            if (ua.dataCount != geom->numFaces)
            {
               std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Invalid values count (" << ua.dataCount << ", expected " << geom->numFaces << ")" << std::endl;
               return false;
            }
         }
         else if (ua.scope == Varying_Scope)
         {
            if (ua.dataCount != geom->numPoints)
            {
               std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Invalid values count (" << ua.dataCount << ", expected " << geom->numPoints << ")" << std::endl;
               return false;
            }
         }
         else
         {
            if (ua.indices)
            {
               if (ua.indicesCount != geom->numFaceVertices)
               {
                  std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Invalid indices count (" << ua.indicesCount << ", expected " << geom->numFaceVertices << ")" << std::endl;
                  return false;
               }
            }
            else
            {
               if (ua.dataCount != geom->numFaceVertices)
               {
                  std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Invalid values count (" << ua.dataCount << ", expected " << geom->numFaceVertices << ")" << std::endl;
                  return false;
               }
            }
         }
         
         // mesh / subd
         VR::DefMapChannelsParam::MapChannelList &channels = geom->channels->getMapChannels();
         
         int channelIdx = channels.count();
         
         channels.setCount(channelIdx + 1, t);
         
         VR::DefMapChannelsParam::MapChannel &channel = channels[channelIdx];
         
         channel.idx = channelIdx;
         channel.verts.setCount(ua.dataCount);
         channel.faces.setCount(geom->numTriangles * 3);
         
         switch (ua.dataType)
         {
         case Bool_Type:
            {
               bool *bdata = (bool*) ua.data;
               for (unsigned int i=0; i<ua.dataCount; ++i)
               {
                  channel.verts[i].set(float(bdata[i]), 0.0f, 0.0f);
               }
            }
            break;
         case Int_Type:
            {
               int *idata = (int*) ua.data;
               for (unsigned int i=0; i<ua.dataCount; ++i)
               {
                  channel.verts[i].set(float(idata[i]), 0.0f, 0.0f);
               }
            }
            break;
         case Uint_Type:
            {
               unsigned int *udata = (unsigned int*) ua.data;
               for (unsigned int i=0; i<ua.dataCount; ++i)
               {
                  channel.verts[i].set(float(udata[i]), 0.0f, 0.0f);
               }
            }
            break;
         case Float_Type:
            {
               float *fdata = (float*) ua.data;
               
               switch (ua.dataDim)
               {
               case 4:
                  for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=4)
                  {
                     channel.verts[i].set(fdata[j], fdata[j+1], fdata[j+2]);
                  }
                  break;
               case 3:
                  for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=3)
                  {
                     channel.verts[i].set(fdata[j], fdata[j+1], fdata[j+2]);
                  }
                  break;
               case 2:
                  for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=2)
                  {
                     channel.verts[i].set(fdata[j], fdata[j+1], 0.0f);
                  }
                  break;
               default:
                  for (unsigned int i=0, j=0; i<ua.dataCount; ++i, ++j)
                  {
                     channel.verts[i].set(fdata[j], 0.0f, 0.0f);
                  }
               }
            }
            break;
         default:
            std::cerr << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Unsupported data type" << std::endl;
            return false;
         }
         
         if (ua.scope == Uniform_Scope)
         {
            for (size_t i=0, j=0; i<geom->numTriangles; ++i)
            {
               for (size_t k=0; k<3; ++k, ++j)
               {
                  channel.faces[j] = geom->toFaceIndex[j];
               }
            }
            
            return true;
         }
         else if (ua.scope == Varying_Scope)
         {
            for (size_t i=0, j=0; i<geom->numTriangles; ++i)
            {
               for (size_t k=0; k<3; ++k, ++j)
               {
                  channel.faces[j] = geom->toPointIndex[j];
               }
            }
            
            return true;
         }
         else // FaceVarying_Scope
         {
            if (ua.indices)
            {
               for (size_t i=0, j=0; i<geom->numTriangles; ++i)
               {
                  for (size_t k=0; k<3; ++k, ++j)
                  {
                     unsigned int vi = geom->toVertexIndex[j];
                     if (vi < ua.indicesCount)
                     {
                        channel.faces[j] = ua.indices[vi];
                     }
                  }
               }
            }
            else
            {
               for (size_t i=0, j=0; i<geom->numTriangles; ++i)
               {
                  for (size_t k=0; k<3; ++k, ++j)
                  {
                     channel.faces[j] = geom->toVertexIndex[j];
                  }
               }
            }
            return true;
         }
      }
      else if (geom->ids)
      {
         // particles
         if (ua.scope != Varying_Scope)
         {
            std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Only constant and varying attributes supported on points primitives" << std::endl;
            return false;
         }
         
         if (geom->numPoints != ua.dataCount)
         {
            std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Invalid size for point attribute \"" << name << "\" (got " << ua.dataCount << " expected " << geom->numPoints << ")" << std::endl;
            return false;
         }
         
         if (!geom->particleOrder)
         {
            std::cout << "[AlembicLoader] SetUserAttribute: \"" << name << "\" No particle order defined" << std::endl;
            return false;
         }
         
         std::string paramName = geom->remapParamName(name);
         
         if (!geom->isValidParamName(paramName))
         {
            std::cout << "[AlembicLoader] SetUserAttribute: Invalid parameter name \"" << paramName << "\" (\"" << name << "\")" << std::endl;
            return false;
         }
         
         switch (ua.dataType)
         {
         case Bool_Type:
            {
               if (!geom->isFloatParam(paramName))
               {
                  return false;
               }
               
               std::map<std::string, VR::DefFloatListParam*>::iterator pit = geom->floatParams.find(paramName);
               
               VR::DefFloatListParam *param = 0;
               
               if (pit == geom->floatParams.end())
               {
                  param = new VR::DefFloatListParam(paramName.c_str(), 1);
                  geom->floatParams[paramName] = param;
               }
               else
               {
                  param = pit->second;
               }
                  
               param->setCount(geom->numPoints + 1);
               VR::FloatList fl = param->getFloatList(t);
               fl[0] = 0.0f;
               
               bool *bdata = (bool*) ua.data;
               for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=ua.dataDim)
               {
                  fl[geom->particleOrder[i]+1] = (bdata[j] ? 1.0f : 0.0f);
               }
            }
            break;
         case Int_Type:
            {
               if (!geom->isFloatParam(paramName))
               {
                  return false;
               }
               
               std::map<std::string, VR::DefFloatListParam*>::iterator pit = geom->floatParams.find(paramName);
               
               VR::DefFloatListParam *param = 0;
               
               if (pit == geom->floatParams.end())
               {
                  param = new VR::DefFloatListParam(paramName.c_str(), 1);
                  geom->floatParams[paramName] = param;
               }
               else
               {
                  param = pit->second;
               }
                  
               param->setCount(geom->numPoints + 1);
               VR::FloatList fl = param->getFloatList(t);
               fl[0] = 0.0f;
               
               int *idata = (int*) ua.data;
               for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=ua.dataDim)
               {
                  fl[geom->particleOrder[i]+1] = float(idata[j]);
               }
            }
            break;
         case Uint_Type:
            {
               if (!geom->isFloatParam(paramName))
               {
                  return false;
               }
               
               std::map<std::string, VR::DefFloatListParam*>::iterator pit = geom->floatParams.find(paramName);
               
               VR::DefFloatListParam *param = 0;
               
               if (pit == geom->floatParams.end())
               {
                  param = new VR::DefFloatListParam(paramName.c_str(), 1);
                  geom->floatParams[paramName] = param;
               }
               else
               {
                  param = pit->second;
               }
                  
               param->setCount(geom->numPoints + 1);
               VR::FloatList fl = param->getFloatList(t);
               fl[0] = 0.0f;
               
               unsigned int *udata = (unsigned int*) ua.data;
               for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=ua.dataDim)
               {
                  fl[geom->particleOrder[i]+1] = float(udata[j]);
               }
            }
            break;
         case Float_Type:
            if (ua.dataDim == 3 || ua.dataDim == 4)
            {
               if (!geom->isColorParam(paramName))
               {
                  return false;
               }
               
               std::map<std::string, VR::DefColorListParam*>::iterator pit = geom->colorParams.find(paramName);
               
               VR::DefColorListParam *param = 0;
               
               if (pit == geom->colorParams.end())
               {
                  param = new VR::DefColorListParam(paramName.c_str(), 1);
                  geom->colorParams[paramName] = param;
               }
               else
               {
                  param = pit->second;
               }
                  
               param->setCount(geom->numPoints + 1);
               VR::ColorList cl = param->getColorList(t);
               cl[0].set(0.0f, 0.0f, 0.0f);
               
               float *fdata = (float*) ua.data;
               for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=ua.dataDim)
               {
                  cl[geom->particleOrder[i]+1].set(fdata[j], fdata[j+1], fdata[j+2]);
               }
            }
            else
            {
               if (!geom->isFloatParam(paramName))
               {
                  return false;
               }
               
               std::map<std::string, VR::DefFloatListParam*>::iterator pit = geom->floatParams.find(paramName);
               
               VR::DefFloatListParam *param = 0;
               
               if (pit == geom->floatParams.end())
               {
                  param = new VR::DefFloatListParam(paramName.c_str(), 1);
                  geom->floatParams[paramName] = param;
               }
               else
               {
                  param = pit->second;
               }
                  
               param->setCount(geom->numPoints + 1);
               VR::FloatList fl = param->getFloatList(t);
               fl[0] = 0.0f;
               
               float *fdata = (float*) ua.data;
               for (unsigned int i=0, j=0; i<ua.dataCount; ++i, j+=ua.dataDim)
               {
                  fl[geom->particleOrder[i]+1] = fdata[j];
               }
            }
            break;
         default:
            std::cerr << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Unsupported data type" << std::endl;
            return false;
         }
         
         return true;
      }
      else
      {
         std::cerr << "[AlembicLoader] SetUserAttributes: \"" << name << "\" Unsupported target primitive" << std::endl;
         return false;
      }
   }
   else
   {
      std::cerr << "[AlembicLoader] SetUserAttribute: \"" << name << "\" Invalid user attribute scope" << std::endl;
      return false;
   }
}
