#include "userattr.h"
#include <SampleUtils.h>

void InitUserAttribute(UserAttribute &attrib)
{
   attrib.arnoldCategory = AI_USERDEF_CONSTANT;
   attrib.arnoldType = AI_TYPE_UNDEFINED;
   attrib.arnoldTypeStr = "";
   attrib.abcType = Alembic::AbcCoreAbstract::DataType();
   attrib.dataDim = 0;
   attrib.dataCount = 0; // Note: use dataCount == 0 for non-array attributes to differentiate with 1 element arrays
   attrib.data = 0;
   attrib.indicesCount = 0;
   attrib.indices = 0;
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
      size_t count = (ua.dataCount == 0 ? 1 : ua.dataCount) * ua.dataDim;
      ua.data = AiMalloc(count * sizeof(DstT));
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
      size_t count = (ua.dataCount == 0 ? 1 : ua.dataCount) * ua.dataDim;
      ua.data = AiMalloc(count * sizeof(T));
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
      size_t count = (ua.dataCount == 0 ? 1 : ua.dataCount) * ua.dataDim;
      ua.data = AiMalloc(count * sizeof(bool));
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
      size_t count = (ua.dataCount == 0 ? 1 : ua.dataCount) * ua.dataDim;
      ua.data = AiMalloc(count * sizeof(bool));
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
      size_t count = (ua.dataCount == 0 ? 1 : ua.dataCount) * ua.dataDim;
      ua.data = AiMalloc(count * sizeof(const char*));
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

template <typename SrcT, typename DstT, class ScalarProperty>
bool ReadScalarProperty(ScalarProperty prop, UserAttribute &ua, double t, bool interpolate)
{
   TimeSampleList<ScalarProperty> sampler; 
   typename TimeSampleList<ScalarProperty>::ConstIterator samp0, samp1;
   typename ScalarProperty::value_type val0, val1;
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
   
   ua.dataCount = 0;
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   val0 = samp0->data();
   // Not really no
   vals0 = (SrcT*) &val0;
   
   if (blend > 0.0 && interpolate)
   {
      val1 = samp0->data();
      // Not really no
      vals1 = (SrcT*) &val1;
      
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
   unsigned int count = 0;
   
   if (prop.isScalarLike())
   {
      // assert samp0->data()->size() == 1
      count = 1;
      ua.dataCount = 0;
   }
   else
   {
      count = samp0->data()->size();
      ua.dataCount = count;
   }
   
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   vals0 = (SrcT *) samp0->data()->getData();
   
   if (blend > 0.0 && interpolate && count != samp1->data()->size())
   {
      vals1 = (SrcT *) samp1->data()->getData();
      
      double a = 1.0 - blend;
      double b = blend;
      
      for (unsigned int i=0, k=0; i<count; ++i)
      {
         for (unsigned int j=0; j<ua.dataDim; ++j, ++k)
         {
            output[k] = TypeHelper<SrcT, DstT>::Blend(ua, vals0[k], vals1[k], a, b);
         }
      }
   }
   else
   {
      for (unsigned int i=0, k=0; i<count; ++i)
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
   
   ua.dataCount = (unsigned int) samp0->data().getVals()->size();
   TypeHelper<SrcT, DstT>::Alloc(ua);
   
   output = (DstT*) ua.data;
   
   vals0 = (SrcT *) samp0->data().getVals()->getData();
   
   if (blend > 0.0 && interpolate && ua.dataCount != samp1->data().getVals()->size())
   {
      vals1 = (SrcT *) samp1->data().getVals()->getData();
      
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
   
   if (param.isIndexed())
   {
      ua.indicesCount = samp0->data().getIndices()->size();
      ua.indices = (unsigned int*) AiMalloc(ua.indicesCount * sizeof(unsigned int));
      
      const Alembic::Util::uint32_t *indices = samp0->data().getIndices()->get();
      
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
      if (ua.arnoldCategory != AI_USERDEF_CONSTANT)
      {
         return false;
      }
      
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
            return false;
         }
      default:
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
            else if (interp.find("vector"))
            {
               return _ReadUserAttribute<Alembic::Abc::V3fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
            }
            else
            {
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
            return false;
         }
      case 16:
         ua.arnoldType = AI_TYPE_MATRIX;
         ua.arnoldTypeStr = "MATRIX";
         return _ReadUserAttribute<Alembic::Abc::M44fTPTraits, Alembic::Util::float32_t, float>(ua, parent, header, t, geoparam, interpolate);
      default:
         return false;
      }
   
   case Alembic::Util::kFloat64POD:
      switch (ua.dataDim)
      {
      case 1:
         ua.arnoldType = AI_TYPE_FLOAT;
         ua.arnoldTypeStr = "FLOAT";
         return _ReadUserAttribute<Alembic::Abc::Float32TPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
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
            else if (interp.find("vector"))
            {
               return _ReadUserAttribute<Alembic::Abc::V3dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
            }
            else
            {
               return false;
            }
         }
      case 4:
         // QuatdTPTraits no supported
         // no C4dTPTraits
         return false;
      case 16:
         ua.arnoldType = AI_TYPE_MATRIX;
         ua.arnoldTypeStr = "MATRIX";
         return _ReadUserAttribute<Alembic::Abc::M44dTPTraits, Alembic::Util::float64_t, float>(ua, parent, header, t, geoparam, interpolate);
      default:
         return false;
      }
   
   case Alembic::Util::kStringPOD:
      if (ua.dataDim != 1) return false;
      ua.arnoldType = AI_TYPE_STRING;
      ua.arnoldTypeStr = "STRING";
      return _ReadUserAttribute<Alembic::Abc::StringTPTraits, std::string, const char*>(ua, parent, header, t, geoparam, interpolate);
   
   default:
      return false;
   }
}

// ---

template <int ArnoldType, typename T>
void _NodeSet(AtNode *, const char *, T *)
{
}

template <int ArnoldType, typename T>
void _ArraySet(AtArray *, unsigned int, T *)
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
void _ArraySet<AI_TYPE_BOOLEAN, bool>(AtArray *ary, unsigned int count, bool *vals)
{
   for (unsigned int i=0; i<count; ++i)
   {
      AiArraySetBool(ary, i, vals[i]);
   }
}

template <>
void _ArraySet<AI_TYPE_BYTE, AtByte>(AtArray *ary, unsigned int count, AtByte *vals)
{
   for (unsigned int i=0; i<count; ++i)
   {
      AiArraySetByte(ary, i, vals[i]);
   }
}

template <>
void _ArraySet<AI_TYPE_INT, int>(AtArray *ary, unsigned int count, int *vals)
{
   for (unsigned int i=0; i<count; ++i)
   {
      AiArraySetInt(ary, i, vals[i]);
   }
}

template <>
void _ArraySet<AI_TYPE_UINT, unsigned int>(AtArray *ary, unsigned int count, unsigned int *vals)
{
   for (unsigned int i=0; i<count; ++i)
   {
      AiArraySetUInt(ary, i, vals[i]);
   }
}

template <>
void _ArraySet<AI_TYPE_FLOAT, float>(AtArray *ary, unsigned int count, float *vals)
{
   for (unsigned int i=0; i<count; ++i)
   {
      AiArraySetFlt(ary, i, vals[i]);
   }
}

template <>
void _ArraySet<AI_TYPE_POINT2, float>(AtArray *ary, unsigned int count, float *vals)
{
   AtPoint2 pnt;
   for (unsigned int i=0, j=0; i<count; ++i, j+=2)
   {
      pnt.x = vals[j+0];
      pnt.y = vals[j+1];
      AiArraySetPnt2(ary, i, pnt);
   }
}

template <>
void _ArraySet<AI_TYPE_POINT, float>(AtArray *ary, unsigned int count, float *vals)
{
   AtPoint pnt;
   for (unsigned int i=0, j=0; i<count; ++i, j+=3)
   {
      pnt.x = vals[j+0];
      pnt.y = vals[j+1];
      pnt.z = vals[j+2];
      AiArraySetPnt(ary, i, pnt);
   }
}

template <>
void _ArraySet<AI_TYPE_VECTOR, float>(AtArray *ary, unsigned int count, float *vals)
{
   AtVector vec;
   for (unsigned int i=0, j=0; i<count; ++i, j+=3)
   {
      vec.x = vals[j+0];
      vec.y = vals[j+1];
      vec.z = vals[j+2];
      AiArraySetVec(ary, i, vec);
   }
}

template <>
void _ArraySet<AI_TYPE_RGB, float>(AtArray *ary, unsigned int count, float *vals)
{
   AtRGB col;
   for (unsigned int i=0, j=0; i<count; ++i, j+=3)
   {
      col.r = vals[j+0];
      col.g = vals[j+1];
      col.b = vals[j+2];
      AiArraySetRGB(ary, i, col);
   }
}

template <>
void _ArraySet<AI_TYPE_RGBA, float>(AtArray *ary, unsigned int count, float *vals)
{
   AtRGBA col;
   for (unsigned int i=0, j=0; i<count; ++i, j+=4)
   {
      col.r = vals[j+0];
      col.g = vals[j+1];
      col.b = vals[j+2];
      col.a = vals[j+3];
      AiArraySetRGBA(ary, i, col);
   }
}

template <>
void _ArraySet<AI_TYPE_MATRIX, float>(AtArray *ary, unsigned int count, float *vals)
{
   AtMatrix mat;
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

template <>
void _ArraySet<AI_TYPE_STRING, const char*>(AtArray *ary, unsigned int count, const char* *vals)
{
   for (unsigned int i=0; i<count; ++i)
   {
      AiArraySetStr(ary, i, vals[i]);
   }
}


template <int ArnoldType, typename T>
void _SetUserAttribute(AtNode *node, const std::string &valName, const std::string &idxName, UserAttribute &ua)
{
   T *vals = (T*) ua.data;
   unsigned int *idxs = (idxName.length() > 0 ? ua.indices : 0);
   
   if (ua.dataCount >= 1 || idxs)
   {
      AtArray *valAry = AiArrayAllocate(ua.dataCount, 1, ArnoldType);
      
      _ArraySet<ArnoldType, T>(valAry, ua.dataCount, vals);
      
      AiNodeSetArray(node, valName.c_str(), valAry);
      
      if (idxs)
      {
         AtArray *idxAry = AiArrayAllocate(ua.indicesCount, 1, AI_TYPE_UINT);
         
         _ArraySet<AI_TYPE_UINT, unsigned int>(idxAry, ua.indicesCount, idxs);
         
         AiNodeSetArray(node, idxName.c_str(), idxAry);
      }
   }
   else
   {
      _NodeSet<ArnoldType, T>(node, valName.c_str(), vals);
   }
}

void SetUserAttribute(AtNode *node, const char *name, UserAttribute &ua)
{
   std::string decl;
   std::string valsName = name;
   std::string idxsName = "";
   
   if (ua.arnoldCategory == AI_USERDEF_CONSTANT)
   {
      decl += "constant ";
      if (ua.dataCount >= 1)
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
      valsName += "list"; // doesn't have to be
   }
   
   // For face-varying attributes, should I lookup for the suffixed name or the original one?
   
   const AtUserParamEntry *pe = AiNodeLookUpUserParameter(node, name);
   if (!pe && idxsName.length() > 0)
   {
      pe = AiNodeLookUpUserParameter(node, valsName.c_str());
   }
   
   if (pe == 0)
   {
      // To declare, we have to use the name without any suffix
      AiNodeDeclare(node, name, decl.c_str());
   }
   else
   {
      bool isArray = (AiUserParamGetType(pe) == AI_TYPE_ARRAY);
      int type = (isArray ? AiUserParamGetArrayType(pe) : AiUserParamGetType(pe));
      
      if (type != ua.arnoldType ||
          AiUserParamGetCategory(pe) != ua.arnoldCategory,
          (ua.dataDim >= 1 || ua.indices) != isArray)
      {
         // Attribute spec mismatch
         return;
      }
   }
   
   switch (ua.arnoldType)
   {
   case AI_TYPE_BOOLEAN:
      _SetUserAttribute<AI_TYPE_BOOLEAN, bool>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_BYTE:
      _SetUserAttribute<AI_TYPE_BYTE, AtByte>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_INT:
      _SetUserAttribute<AI_TYPE_INT, int>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_UINT:
      _SetUserAttribute<AI_TYPE_UINT, unsigned int>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_FLOAT:
      _SetUserAttribute<AI_TYPE_FLOAT, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_POINT2:
      _SetUserAttribute<AI_TYPE_POINT2, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_POINT:
      _SetUserAttribute<AI_TYPE_POINT, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_VECTOR:
      _SetUserAttribute<AI_TYPE_VECTOR, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_RGB:
      _SetUserAttribute<AI_TYPE_RGB, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_RGBA:
      _SetUserAttribute<AI_TYPE_RGBA, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_MATRIX:
      _SetUserAttribute<AI_TYPE_MATRIX, float>(node, valsName, idxsName, ua);
      break;
   case AI_TYPE_STRING:
      _SetUserAttribute<AI_TYPE_STRING, const char*>(node, valsName, idxsName, ua);
   default:
      break;
   }
}

void SetUserAttributes(AtNode *node, UserAttributes &attribs)
{
   UserAttributes::iterator it = attribs.begin();
   
   while (it != attribs.end())
   {
      SetUserAttribute(node, it->first.c_str(), it->second);
      ++it;
   }
}
