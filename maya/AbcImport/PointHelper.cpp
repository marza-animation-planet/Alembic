//-*****************************************************************************
//
// Copyright (c) 2009-2011,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include "util.h"
#include "PointHelper.h"
#include "NodeIteratorVisitorHelper.h"

#include <maya/MString.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MVectorArray.h>
#include <maya/MVector.h>
#include <maya/MGlobal.h>
#include <maya/MVectorArray.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MDagModifier.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MDistance.h>


double getWeightIndexAndTime(double iFrame,
    Alembic::AbcCoreAbstract::TimeSamplingPtr iTime, size_t numSamps,
    Alembic::AbcCoreAbstract::index_t & oIndex,
    Alembic::AbcCoreAbstract::index_t & oCeilIndex,
    double &oFrame,
    double &oCeilFrame)
{
    if (numSamps == 0)
    {
        numSamps = 1;
    }

    std::pair<Alembic::AbcCoreAbstract::index_t, double> floorTime = iTime->getFloorIndex(iFrame, numSamps);

    oIndex = floorTime.first;
    oFrame = floorTime.second;
    oCeilIndex = oIndex;
    oCeilFrame = oFrame;
    
    if (fabs(iFrame - floorTime.second) < 0.0001)
    {
        return 0.0;
    }

    std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilTime = iTime->getCeilIndex(iFrame, numSamps);

    if (oIndex == ceilTime.first)
    {
        return 0.0;
    }

    oCeilIndex = ceilTime.first;
    oCeilFrame = ceilTime.second;

    double blend = (iFrame - floorTime.second) / (ceilTime.second - floorTime.second);

    if (fabs(1.0 - blend) < 0.0001)
    {
        oIndex = oCeilIndex;
        oFrame = oCeilFrame;
        return 0.0;
    }
    else
    {
        return blend;
    }
}

template <typename MayaArrayType> struct MayaArrayElementType
{
    typedef void* T;
};

template <> struct MayaArrayElementType<MDoubleArray>
{
    typedef double T;
};

template <> struct MayaArrayElementType<MVectorArray>
{
    typedef MVector T;
};

template <typename AbcType, typename MayaType> struct AbcToMaya
{
    static void Set(const AbcType &src, MayaType &dst)
    {
    }
    
    static void Reset(MayaType &dst)
    {
    }
    
    static void Blend(const AbcType &src0, const AbcType &src1, double b, MayaType &dst)
    {
    }
};

template <typename AbcType> struct AbcToMaya<AbcType, double>
{
    static void Set(const AbcType &src, double &dst)
    {
        dst = double(src);
    }
    
    static void Reset(double &dst)
    {
        dst = 0.0;
    }
    
    static void Blend(const AbcType &src0, const AbcType &src1, double b, double &dst)
    {
        dst = (1.0 - b) * double(src0) + b * double(src1);
    }
};

template <typename AbcType> struct AbcToMaya<AbcType, MVector>
{
    static void Set(const AbcType &src, MVector &dst)
    {
        dst.x = double(src);
        dst.y = dst.x;
        dst.z = dst.x;
    }
    
    static void Reset(MVector &dst)
    {
        dst.x = 0.0;
        dst.y = 0.0;
        dst.z = 0.0;
    }
    
    static void Blend(const AbcType &src0, const AbcType &src1, double b, MVector &dst)
    {
        dst.x = (1.0 - b) * double(src0) + b * double(src1);
        dst.y = dst.x;
        dst.z = dst.x;
    }
};

template <> struct AbcToMaya<Alembic::Abc::bool_t, double>
{
    static void Set(const Alembic::Util::bool_t &src, double &dst)
    {
        dst = (src ? 1.0 : 0.0);
    }
    
    static void Reset(double &dst)
    {
        dst = 0.0;
    }
    
    static void Blend(const Alembic::Util::bool_t &src0, const Alembic::Util::bool_t &src1, double b, double &dst)
    {
        dst = (b > 0.5 ? (src1 ? 1.0 : 0.0) : (src0 ? 1.0 : 0.0));
    }
};

template <> struct AbcToMaya<Alembic::Abc::C3h, MVector>
{
    static void Set(const Alembic::Abc::C3h &src, MVector &dst)
    {
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
    }
    
    static void Reset(MVector &dst)
    {
        dst.x = 0.0;
        dst.y = 0.0;
        dst.z = 0.0;
    }
    
    static void Blend(const Alembic::Abc::C3h &src0, const Alembic::Abc::C3h &src1, double b, MVector &dst)
    {
        MVector v0(src0.x, src0.y, src0.z);
        MVector v1(src1.x, src1.y, src1.z);
        
        dst = (1.0 - b) * v0 + b * v1;
    }
};

template <> struct AbcToMaya<Alembic::Abc::V3f, MVector>
{
    static void Set(const Alembic::Abc::V3f &src, MVector &dst)
    {
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
    }
    
    static void Reset(MVector &dst)
    {
        dst.x = 0.0;
        dst.y = 0.0;
        dst.z = 0.0;
    }
    
    static void Blend(const Alembic::Abc::V3f &src0, const Alembic::Abc::V3f &src1, double b, MVector &dst)
    {
        MVector v0(src0.x, src0.y, src0.z);
        MVector v1(src1.x, src1.y, src1.z);
        
        dst = (1.0 - b) * v0 + b * v1;
    }
};

template <> struct AbcToMaya<Alembic::Abc::V3d, MVector>
{
    static void Set(const Alembic::Abc::V3d &src, MVector &dst)
    {
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
    }
    
    static void Reset(MVector &dst)
    {
        dst.x = 0.0;
        dst.y = 0.0;
        dst.z = 0.0;
    }
    
    static void Blend(const Alembic::Abc::V3d &src0, const Alembic::Abc::V3d &src1, double b, MVector &dst)
    {
        MVector v0(src0.x, src0.y, src0.z);
        MVector v1(src1.x, src1.y, src1.z);
        
        dst = (1.0 - b) * v0 + b * v1;
    }
};

template <typename Traits, typename MayaArrayType>
void FillValues(Alembic::Abc::ICompoundProperty geomParams, const std::string &name, double t, MayaArrayType &out,
                size_t sz0, size_t sz1=0, const Alembic::Abc::uint64_t *ids0=0, const std::map<size_t, size_t> *idmap=0)
{
    Alembic::AbcGeom::ITypedGeomParam<Traits> prop(geomParams, name);
    
    Alembic::AbcCoreAbstract::index_t iprev = 0;
    Alembic::AbcCoreAbstract::index_t inext = 0;
    double tprev = 0.0;
    double tnext = 0.0;
    
    double blend = getWeightIndexAndTime(t, prop.getTimeSampling(), prop.getNumSamples(), iprev, inext, tprev, tnext);
    
    if (blend > 0.0)
    {
        typename Alembic::AbcGeom::ITypedGeomParam<Traits>::Sample samp0 = prop.getExpandedValue(iprev);
        typename Alembic::AbcGeom::ITypedGeomParam<Traits>::Sample samp1 = prop.getExpandedValue(inext);
        
        size_t count = samp0.getVals()->size();
        
        if (count != sz0)
        {
            for (size_t i=0; i<sz0; ++i)
            {
                AbcToMaya<typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type, typename MayaArrayElementType<MayaArrayType>::T>::Reset(out[i]);
            }
        }
        else
        {
            const typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type *vptr0 = samp0.getVals()->get();
            
            if (samp1.getVals()->size() != sz1 || !ids0 || !idmap)
            {
                for (size_t i=0; i<sz0; ++i)
                {
                    AbcToMaya<typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type, typename MayaArrayElementType<MayaArrayType>::T>::Set(vptr0[i], out[i]);
                }
            }
            else
            {
                const typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type *vptr1 = samp1.getVals()->get();
                
                std::map<size_t, size_t>::const_iterator idit;
                
                for (size_t i=0; i<sz0; ++i)
                {
                    idit = idmap->find(ids0[i]);
                    
                    if (idit != idmap->end())
                    {
                        AbcToMaya<typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type, typename MayaArrayElementType<MayaArrayType>::T>::Blend(vptr0[i], vptr1[idit->second], blend, out[i]);
                    }
                    else
                    {
                        AbcToMaya<typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type, typename MayaArrayElementType<MayaArrayType>::T>::Set(vptr0[i], out[i]);
                    }
                }
            }
        }
    }
    else
    {
        typename Alembic::AbcGeom::ITypedGeomParam<Traits>::Sample samp = prop.getExpandedValue(iprev);
        
        size_t count = samp.getVals()->size();
        
        if (count != sz0)
        {
            for (size_t i=0; i<sz0; ++i)
            {
                AbcToMaya<typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type, typename MayaArrayElementType<MayaArrayType>::T>::Reset(out[i]);
            }
        }
        else
        {
            const typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type *vptr = samp.getVals()->get();
            
            for (size_t i=0; i<sz0; ++i)
            {
                AbcToMaya<typename Alembic::AbcGeom::ITypedGeomParam<Traits>::value_type, typename MayaArrayElementType<MayaArrayType>::T>::Set(vptr[i], out[i]);
            }
        }
    }
}

void ReadAttributes(double t, const Alembic::AbcGeom::IPoints & iNode, MFnArrayAttrsData & fnAttrs,
                    size_t sz0, size_t sz1=0, const Alembic::Abc::uint64_t *ids0=0, const std::map<size_t, size_t> *idmap=0)
{
    Alembic::AbcGeom::IPointsSchema schema = iNode.getSchema();
    
    Alembic::Abc::ICompoundProperty geomParams = schema.getArbGeomParams();
    
    Alembic::AbcCoreAbstract::index_t iprev, inext;
    double tprev, tnext, blend;
    MStatus stat;
    
    for (size_t i=0; i<geomParams.getNumProperties(); ++i)
    {
        const Alembic::AbcCoreAbstract::PropertyHeader &header = geomParams.getPropertyHeader(i);
        
        Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
        
        if (scope != Alembic::AbcGeom::kVaryingScope)
        {
            continue;
        }
        
        Alembic::AbcCoreAbstract::DataType abcType = header.getDataType();
        std::string usage = header.getMetaData().get("interpretation");
        unsigned int dim = abcType.getExtent();
        
        MString attrName = header.getName().c_str();
        MFnArrayAttrsData::Type attrType;
        
        switch (abcType.getPod())
        {
        case Alembic::Util::kBooleanPOD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::BooleanTPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kInt8POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Int8TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kInt16POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Int16TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kInt32POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Int32TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kInt64POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Int64TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kUint8POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Uint8TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kUint16POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Uint16TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kUint32POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Uint32TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kUint64POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Uint64TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kFloat16POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Float16TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            else if (dim == 3)
            {
                MVectorArray vArray = fnAttrs.vectorArray(attrName);
                vArray.setLength(sz0);
                FillValues<Alembic::Abc::C3hTPTraits>(geomParams, header.getName(), t, vArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kFloat32POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Float32TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            else if (dim == 3)
            {
                MVectorArray vArray = fnAttrs.vectorArray(attrName);
                vArray.setLength(sz0);
                FillValues<Alembic::Abc::V3fTPTraits>(geomParams, header.getName(), t, vArray, sz0, sz1, ids0, idmap);
            }
            break;
        case Alembic::Util::kFloat64POD:
            if (dim == 1)
            {
                MDoubleArray dArray = fnAttrs.doubleArray(attrName);
                dArray.setLength(sz0);
                FillValues<Alembic::Abc::Float64TPTraits>(geomParams, header.getName(), t, dArray, sz0, sz1, ids0, idmap);
            }
            else if (dim == 3)
            {
                MVectorArray vArray = fnAttrs.vectorArray(attrName);
                vArray.setLength(sz0);
                FillValues<Alembic::Abc::V3dTPTraits>(geomParams, header.getName(), t, vArray, sz0, sz1, ids0, idmap);
            }
            break;
        default:
            break;
        }   
    }
}

void CreateArrayAttr(const MString &nodeName, const MString &attrType, const MString &attrName)
{
    MString scr;
    
    scr += "if (!`attributeExists " + attrName + " " + nodeName + "`) {\n";
    scr += "  addAttr -ln " + attrName + "0 -dt " + attrType + "Array " + nodeName + ";\n";
    scr += "  addAttr -ln " + attrName + " -dt " + attrType + "Array " + nodeName + ";\n";
    scr += "  setAttr -e -keyable true " + nodeName + "." + attrName + ";\n";
    scr += "}";
    
    MGlobal::executeCommand(scr);
}

void CreateAttributes(double t, const Alembic::AbcGeom::IPoints & iNode, MFnParticleSystem & fnParticle, size_t sz)
{
    Alembic::AbcGeom::IPointsSchema schema = iNode.getSchema();
    
    Alembic::Abc::ICompoundProperty geomParams = schema.getArbGeomParams();
    
    MDoubleArray dArray;
    MVectorArray vArray;
    Alembic::AbcCoreAbstract::index_t iprev, inext;
    double tprev, tnext, blend;
    MStatus stat;
    
    dArray.setLength(sz);
    vArray.setLength(sz);
    
    MString nodeName = fnParticle.name();
    
    for (size_t i=0; i<geomParams.getNumProperties(); ++i)
    {
        const Alembic::AbcCoreAbstract::PropertyHeader &header = geomParams.getPropertyHeader(i);
        
        Alembic::AbcGeom::GeometryScope scope = Alembic::AbcGeom::GetGeometryScope(header.getMetaData());
        
        if (scope != Alembic::AbcGeom::kVaryingScope)
        {
            continue;
        }
        
        Alembic::AbcCoreAbstract::DataType abcType = header.getDataType();
        std::string usage = header.getMetaData().get("interpretation");
        unsigned int dim = abcType.getExtent();
        
        MString attrName = header.getName().c_str();
        
        switch (abcType.getPod())
        {
        case Alembic::Util::kBooleanPOD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::BooleanTPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kInt8POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Int8TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kInt16POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Int16TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kInt32POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Int32TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kInt64POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Int64TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kUint8POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Uint8TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kUint16POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Uint16TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kUint32POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Uint32TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kUint64POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Uint64TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            break;
        case Alembic::Util::kFloat16POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Float16TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            else if (dim == 3)
            {
                FillValues<Alembic::Abc::C3hTPTraits>(geomParams, header.getName(), t, vArray, sz);
                CreateArrayAttr(nodeName, "vector", attrName);
                fnParticle.setPerParticleAttribute(attrName, vArray);
            }
            break;
        case Alembic::Util::kFloat32POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Float32TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            else if (dim == 3)
            {
                FillValues<Alembic::Abc::V3fTPTraits>(geomParams, header.getName(), t, vArray, sz);
                CreateArrayAttr(nodeName, "vector", attrName);
                fnParticle.setPerParticleAttribute(attrName, vArray);
            }
            break;
        case Alembic::Util::kFloat64POD:
            if (dim == 1)
            {
                FillValues<Alembic::Abc::Float64TPTraits>(geomParams, header.getName(), t, dArray, sz);
                CreateArrayAttr(nodeName, "double", attrName);
                fnParticle.setPerParticleAttribute(attrName, dArray);
            }
            else if (dim == 3)
            {
                FillValues<Alembic::Abc::V3dTPTraits>(geomParams, header.getName(), t, vArray, sz);
                CreateArrayAttr(nodeName, "vector", attrName);
                fnParticle.setPerParticleAttribute(attrName, vArray);
            }
            break;
        default:
            break;
        }   
    }
}

MStatus read(double iFrame, const Alembic::AbcGeom::IPoints & iNode, MObject & iObject )
{
    MStatus status = MS::kSuccess;
    
    MFnArrayAttrsData fnAttrs(iObject);
    
    Alembic::AbcGeom::IPointsSchema schema = iNode.getSchema();
    
    if (schema.getNumSamples() == 0)
    {
        fnAttrs.clear();
        MDoubleArray outCount = fnAttrs.doubleArray("count", &status);
        outCount.setLength(1);
        outCount[0] = 0.0;
        
        return status;
    }
    
    Alembic::AbcCoreAbstract::index_t floorIndex, ceilIndex;
    double floorTime, ceilTime, blend;
    
    blend = getWeightIndexAndTime(iFrame, schema.getTimeSampling(), schema.getNumSamples(),
                                  floorIndex, ceilIndex, floorTime, ceilTime);
    
    Alembic::Abc::ISampleSelector floorSampSel(floorIndex);
    Alembic::Abc::ISampleSelector ceilSampSel(ceilIndex);
    
    Alembic::AbcGeom::IPointsSchema::Sample floorSamp, ceilSamp;
    schema.get(floorSamp, floorSampSel);
    
    Alembic::Abc::P3fArraySamplePtr inFloorPos = floorSamp.getPositions();
    Alembic::Abc::V3fArraySamplePtr inFloorVel = floorSamp.getVelocities();
    Alembic::Abc::UInt64ArraySamplePtr inFloorId = floorSamp.getIds();
    
    MVectorArray outPos = fnAttrs.vectorArray("position", &status);
    MVectorArray outVel = fnAttrs.vectorArray("velocity", &status);
    MDoubleArray outId = fnAttrs.doubleArray("particleId", &status);
    MDoubleArray outCount = fnAttrs.doubleArray("count", &status);
    
    size_t pSize = inFloorPos->size();
    
    outCount.setLength(1);
    outCount[0] = double(pSize);
    
    outPos.setLength(pSize);
    outVel.setLength(pSize);
    outId.setLength(pSize);
    
    float scl = 1.0f;
    if (MDistance::uiUnit() != MDistance::kCentimeters)
    {
        scl = MDistance(1.0, MDistance::uiUnit()).as(MDistance::kCentimeters);
    }

    if (blend > 0.0)
    {
        schema.get(ceilSamp, ceilSampSel);
                
        Alembic::Abc::P3fArraySamplePtr inCeilPos = ceilSamp.getPositions();
        Alembic::Abc::V3fArraySamplePtr inCeilVel = ceilSamp.getVelocities();
        Alembic::Abc::UInt64ArraySamplePtr inCeilId = ceilSamp.getIds();
        
        std::map<size_t, size_t> idmap;
        std::map<size_t, size_t>::iterator idit;
        
        for (size_t j=0; j<inCeilId->size(); ++j)
        {
            idmap[(*inCeilId)[j]] = j;
        }
        
        double dt = blend * (ceilTime - floorTime);
        
        for (unsigned int pId = 0; pId < pSize; pId++)
        {
            MVector p0(scl * (*inFloorPos)[pId].x,
                       scl * (*inFloorPos)[pId].y,
                       scl * (*inFloorPos)[pId].z);
            
            MVector v0(scl * (*inFloorVel)[pId].x,
                       scl * (*inFloorVel)[pId].y,
                       scl * (*inFloorVel)[pId].z);
            
            outId[pId] = (*inFloorId)[pId];
            
            idit = idmap.find(outId[pId]);
            
            if (idit == idmap.end())
            {
                outPos[pId] = p0 + dt * v0;
                outVel[pId] = v0;
            }
            else
            {
                MVector p1(scl * (*inCeilPos)[idit->second].x,
                           scl * (*inCeilPos)[idit->second].y,
                           scl * (*inCeilPos)[idit->second].z);
                
                MVector v1(scl * (*inCeilVel)[idit->second].x,
                           scl * (*inCeilVel)[idit->second].y,
                           scl * (*inCeilVel)[idit->second].z);
                
                outPos[pId] = (1.0 - blend) * p0 + blend * p1;
                outVel[pId] = (1.0 - blend) * v0 + blend * v1;
            }
        }
        
        ReadAttributes(iFrame, iNode, fnAttrs, pSize, inCeilPos->size(), inFloorId->get(), &idmap);
    }
    else
    {
        for (unsigned int pId = 0; pId < pSize; pId++)
        {
            outPos[pId] = MVector(scl * (*inFloorPos)[pId].x,
                                  scl * (*inFloorPos)[pId].y,
                                  scl * (*inFloorPos)[pId].z);
            
            outVel[pId] = MVector(scl * (*inFloorVel)[pId].x,
                                  scl * (*inFloorVel)[pId].y,
                                  scl * (*inFloorVel)[pId].z);
            
            outId[pId] = (*inFloorId)[pId];
        }
        
        ReadAttributes(iFrame, iNode, fnAttrs, inFloorPos->size());
    }

    return status;
}

MStatus create(double iFrame, const Alembic::AbcGeom::IPoints & iNode,
    MObject & iParent, MObject & iObject)
{
    MStatus status = MS::kSuccess;
    Alembic::AbcGeom::IPointsSchema schema = iNode.getSchema();

    // object has no samples, bail early
    if (schema.getNumSamples() == 0)
    {
        return status;
    }

    Alembic::AbcGeom::IPointsSchema::Sample samp;
    Alembic::AbcCoreAbstract::index_t index =
        schema.getTimeSampling()->getNearIndex(
            iFrame, schema.getNumSamples()).first;

    schema.get(samp, index);

    size_t pSize = samp.getPositions()->size();

    // // bail early if there's no particle data at this frame
    // if (pSize == 0)
    // {
    //     return status;
    // }

    // convert the data to Maya format
    MFnParticleSystem fnParticle;
    MDagModifier dagMod;

    iObject = dagMod.createNode("nParticle", iParent, &status);
    status = dagMod.doIt();
    fnParticle.setObject(iObject);
    fnParticle.setName(iNode.getName().c_str());

    MGlobal::executeCommand("setupNParticleConnections " + fnParticle.fullPathName());
    MGlobal::executeCommand("connectAttr time1.outTime " + fnParticle.fullPathName() + ".currentTime");

    if (pSize > 0)
    {
        MPointArray pArray;
        MDoubleArray iArray;
        MVectorArray vArray;
        
        Alembic::Abc::P3fArraySamplePtr posPtr = samp.getPositions();
        Alembic::Abc::V3fArraySamplePtr velPtr = samp.getVelocities();
        Alembic::Abc::UInt64ArraySamplePtr idPtr = samp.getIds();

        float scl = 1.0f;
        if (MDistance::uiUnit() != MDistance::kCentimeters)
        {
            scl = MDistance(1.0, MDistance::uiUnit()).as(MDistance::kCentimeters);
        }

        for (unsigned int pId = 0; pId < pSize; pId++)
        {
            pArray.append(scl * (*posPtr)[pId].x,
                          scl * (*posPtr)[pId].y,
                          scl * (*posPtr)[pId].z);
            
            vArray.append(MVector(scl * (*velPtr)[pId].x,
                                  scl * (*velPtr)[pId].y,
                                  scl * (*velPtr)[pId].z));
            
            iArray.append((*idPtr)[pId]);
        }

        // velocities? ids? other attributes?
        status = fnParticle.emit(pArray, vArray);
        fnParticle.setPerParticleAttribute("particleId", iArray, &status);
    }
    
    CreateAttributes(iFrame, iNode, fnParticle, pSize);
    
    status = fnParticle.saveInitialState();
    
    MPlug plug;
    plug = fnParticle.findPlug("isDynamic");
    plug.setValue(false);
    
    plug = fnParticle.findPlug("playFromCache");
    plug.setValue(true);
    
    plug = fnParticle.findPlug("particleRenderType");
    plug.setValue(3);

    return status;
}
