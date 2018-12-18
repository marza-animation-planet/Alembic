//-*****************************************************************************
//
// Copyright (c) 2009-2014,
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
#include "MeshHelper.h"
#include "NodeIteratorVisitorHelper.h"

#include <maya/MTypes.h>
#include <maya/MString.h>
#include <maya/MFloatPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MUintArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MVector.h>


namespace
{
    // utility to determine if a string is in the string array
    bool inStrArray( const MStringArray & iArray, const MString & iStr )
    {
        unsigned int arrLength = iArray.length();

        for (unsigned int i = 0; i < arrLength; ++i)
        {
            if (iArray[i] == iStr)
            {
                return true;
            }
        }
        return false;
    }

    // utility to clear pt when doing a swap otherwise
    // the new swap position could get messed up
    void clearPt(MFnMesh & ioMesh)
    {
        MPlug ptPlug = ioMesh.findPlug("pt", true);
        unsigned int numElements = ptPlug.numElements();
        if (ptPlug.isArray() && (numElements > 0))
        {
            for (unsigned int i = 0; i < numElements; i++)
            {
                MPlug elementPlug = ptPlug[i];
                MPlug childx = elementPlug.child(0);
                childx.setValue(0.0);
                MPlug childy = elementPlug.child(1);
                childy.setValue(0.0);
                MPlug childz = elementPlug.child(2);
                childz.setValue(0.0);
            }
        }
    }

    void setPolySmoothEdges(double iFrame, MFnMesh & ioMesh, Alembic::Abc::IV2iArrayProperty iHardEdges)
    {
        if (!iHardEdges)
        {
            return;
        }

        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        double alpha = getWeightAndIndex(iFrame,
            iHardEdges.getTimeSampling(), iHardEdges.getNumSamples(),
            index, ceilIndex);

        Alembic::Abc::V2iArraySamplePtr samp = iHardEdges.getValue(Alembic::Abc::ISampleSelector(index));
        // ceilSamp?

        // Build set of hard edges
        std::pair<int, int> edge;
        std::set<std::pair<int, int> > hardEdgesSet;

        for (size_t i=0; i<samp->size(); ++i)
        {
            int v0 = (*samp)[i].x;
            int v1 = (*samp)[i].y;
            if (v0 < v1)
            {
                edge.first = v0;
                edge.second = v1;
            }
            else
            {
                edge.first = v1;
                edge.second = v0;
            }
            hardEdgesSet.insert(edge);
        }

        // Loop edges to set 'smooth' flag
        for (unsigned int e=0; e<ioMesh.numEdges(); ++e)
        {
            int2 i2;
            ioMesh.getEdgeVertices(e, i2);
            edge.first = i2[0];
            edge.second = i2[1];
            if (hardEdgesSet.find(edge) != hardEdgesSet.end())
            {
                ioMesh.setEdgeSmoothing(e, false);
            }
            else
            {
                edge.first = i2[1];
                edge.second = i2[0];
                if (hardEdgesSet.find(edge) != hardEdgesSet.end())
                {
                    ioMesh.setEdgeSmoothing(e, false);
                }
                else
                {
                    ioMesh.setEdgeSmoothing(e, true);
                }
            }
        }
    }

    // normal vector is packed differently in file
    // from the format Maya accepts directly
    void setPolyNormals(double iFrame, MFnMesh & ioMesh,
        Alembic::AbcGeom::IN3fGeomParam iNormals,
        bool remapIndices, const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices)
    {
        // no normals to set?  bail early
        if (!iNormals)
            return;

        if (iNormals.getScope() != Alembic::AbcGeom::kVertexScope &&
            iNormals.getScope() != Alembic::AbcGeom::kVaryingScope &&
            iNormals.getScope() != Alembic::AbcGeom::kFacevaryingScope)
        {
            printWarning(ioMesh.fullPathName() +
                " normal vector has an unsupported scope, skipping normals");
            return;
        }

        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        double alpha = getWeightAndIndex(iFrame,
            iNormals.getTimeSampling(), iNormals.getNumSamples(),
            index, ceilIndex);

        Alembic::AbcGeom::IN3fGeomParam::Sample samp;
        iNormals.getExpanded(samp, Alembic::Abc::ISampleSelector(index));

        MVectorArray normalsIn;

        Alembic::Abc::N3fArraySamplePtr sampVal = samp.getVals();
        size_t sampSize = sampVal->size();
        bool faceVarying = (iNormals.getScope() == Alembic::AbcGeom::kFacevaryingScope);
        bool valid = false;

        if (faceVarying)
        {
            if (remapIndices)
            {
                valid = (size_t(ioMesh.numFaceVertices()) <= sampSize);
            }
            else
            {
                valid = (size_t(ioMesh.numFaceVertices()) == sampSize);
            }
        }
        else if (iNormals.getScope() == Alembic::AbcGeom::kVertexScope ||
                 iNormals.getScope() == Alembic::AbcGeom::kVaryingScope)
        {
            valid = (size_t(ioMesh.numVertices()) == sampSize);
        }

        if (valid)
        {
            Alembic::AbcGeom::IN3fGeomParam::Sample ceilSamp;
            Alembic::Abc::N3fArraySamplePtr ceilVal;

            if (alpha != 0 && index != ceilIndex)
            {
                iNormals.getExpanded(ceilSamp,
                    Alembic::Abc::ISampleSelector(ceilIndex));
                ceilVal = ceilSamp.getVals();
            }

            if (ceilVal && ceilVal->size() == sampSize)
            {
                if (faceVarying && remapIndices)
                {
                    normalsIn.setLength(ioMesh.numFaceVertices());

                    for (unsigned int i=0; i<mayaVertexIndices.length(); ++i)
                    {
                        int mayaIndex = mayaVertexIndices[i];
                        int abcIndex = abcVertexIndices[i];

                        MVector normal(
                            simpleLerp<float>(alpha, (*sampVal)[abcIndex].x,
                                (*ceilVal)[abcIndex].x),
                            simpleLerp<float>(alpha, (*sampVal)[abcIndex].y,
                                (*ceilVal)[abcIndex].y),
                            simpleLerp<float>(alpha, (*sampVal)[abcIndex].z,
                                (*ceilVal)[abcIndex].z));

                        normalsIn[mayaIndex] = normal;
                    }
                }
                else
                {
                    for (size_t i = 0; i < sampSize; ++i)
                    {
                        MVector normal(
                            simpleLerp<float>(alpha, (*sampVal)[i].x,
                                (*ceilVal)[i].x),
                            simpleLerp<float>(alpha, (*sampVal)[i].y,
                                (*ceilVal)[i].y),
                            simpleLerp<float>(alpha, (*sampVal)[i].z,
                                (*ceilVal)[i].z));
                        normalsIn.append(normal);
                    }
                }
            }
            else
            {
                if (faceVarying && remapIndices)
                {
                    normalsIn.setLength(ioMesh.numFaceVertices());

                    for (unsigned int i=0; i<mayaVertexIndices.length(); ++i)
                    {
                        int mayaIndex = mayaVertexIndices[i];
                        int abcIndex = abcVertexIndices[i];

                        MVector normal((*sampVal)[abcIndex].x, (*sampVal)[abcIndex].y,
                            (*sampVal)[abcIndex].z);

                        normalsIn[mayaIndex] = normal;
                    }
                }
                else
                {
                    for (size_t i = 0; i < sampSize; ++i)
                    {
                        MVector normal((*sampVal)[i].x, (*sampVal)[i].y,
                            (*sampVal)[i].z);
                        normalsIn.append(normal);
                    }
                }
            }

            if (!faceVarying)
            {
                MIntArray vertexList;
                int iEnd = static_cast<int>(sampSize);
                for (int i = 0; i < iEnd; ++i)
                {
                    vertexList.append(i);
                }

                ioMesh.setVertexNormals(normalsIn, vertexList);

            }
            else
            {
                MIntArray faceList(ioMesh.numFaceVertices());
                MIntArray vertexList(ioMesh.numFaceVertices());

                // per vertex per-polygon normal
                int numFaces = ioMesh.numPolygons();
                int nIndex = 0;

                if (faceVarying && remapIndices)
                {
                    // values already re-order to match maya face winding
                    for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
                    {
                        MIntArray polyVerts;
                        ioMesh.getPolygonVertices(faceIndex, polyVerts);
                        int numVertices = polyVerts.length();
                        for (int v = 0; v < numVertices; ++v, ++nIndex)
                        {
                            faceList[nIndex] = faceIndex;
                            vertexList[nIndex] = polyVerts[v];
                        }
                    }
                }
                else
                {
                    for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
                    {
                        MIntArray polyVerts;
                        ioMesh.getPolygonVertices(faceIndex, polyVerts);
                        int numVertices = polyVerts.length();
                        for (int v = numVertices - 1; v >= 0; v--, ++nIndex)
                        {
                            faceList[nIndex] = faceIndex;
                            vertexList[nIndex] = polyVerts[v];
                        }
                    }
                }

                ioMesh.setFaceVertexNormals(normalsIn, faceList, vertexList);
            }
        }
        else if (sampSize != 0 && ioMesh.numVertices() != 0)
        {
            printWarning(ioMesh.fullPathName() +
                " normal vector scope does not match size of data, " +
                "skipping normals");
        }
    }

    void fillPoints(MFloatPointArray & oPointArray,
        Alembic::Abc::P3fArraySamplePtr iPoints,
        Alembic::Abc::P3fArraySamplePtr iCeilPoints, double alpha)
    {
       if(!iPoints)
           return;

        unsigned int numPoints = static_cast<unsigned int>(iPoints->size());

        if(!numPoints)
           return;

        oPointArray.setLength(numPoints);

        if (alpha == 0 || iCeilPoints == NULL)
        {
            for (unsigned int i = 0; i < numPoints; ++i)
            {
                oPointArray.set(i,
                    (*iPoints)[i].x, (*iPoints)[i].y, (*iPoints)[i].z);
            }
        }
        else
        {
            for (unsigned int i = 0; i < numPoints; ++i)
            {
                oPointArray.set(i,
                    simpleLerp<float>(alpha,
                        (*iPoints)[i].x, (*iCeilPoints)[i].x),
                    simpleLerp<float>(alpha,
                        (*iPoints)[i].y, (*iCeilPoints)[i].y),
                    simpleLerp<float>(alpha,
                        (*iPoints)[i].z, (*iCeilPoints)[i].z));
            }
        }

    }

    void fillTopology(MFnMesh & ioMesh, MObject & iParent,
        MFloatPointArray & iPoints,
        Alembic::Abc::Int32ArraySamplePtr iIndices,
        Alembic::Abc::Int32ArraySamplePtr iCounts,
        bool varyingTopology, bool &remapIndices,
        MIntArray &mayaVertexIndices, MIntArray &abcVertexIndices)
    {
        // since we are changing the topology we will be creating a new mesh

        // Get face count info
        unsigned int numPolys = iCounts ? static_cast<unsigned int>(iCounts->size()) : 0;
        MIntArray polyCounts;
        polyCounts.setLength(numPolys);

        for (unsigned int i = 0; i < numPolys; ++i)
        {
            polyCounts[i] = (*iCounts)[i];
        }

        unsigned int numConnects = iIndices ? static_cast<unsigned int>(iIndices->size()) : 0;

        MIntArray polyConnects;
        polyConnects.setLength(numConnects);

        unsigned int facePointIndex = 0;
        unsigned int base = 0;
        MIntArray faceIndices;
        MIntArray invalidFaces;
        for (unsigned int i = 0; i < numPolys; ++i)
        {
            // reverse the order of the faces
            int curNum = polyCounts[i];
            bool invalid = false;
            
            if (curNum > int(faceIndices.length()))
                faceIndices.setLength(curNum);
            
            for (int j = 0; j < curNum; ++j, ++facePointIndex)
            {
                int faceIndex = (*iIndices)[base+curNum-j-1];
                
                if (!invalid)
                {
                    for (int k=j-1; k>=0; --k)
                    {
                        if (faceIndices[k] == faceIndex)
                        {
                            invalid = true;
                            break;
                        }
                    }
                    
                    if (invalid)
                    {
                        invalidFaces.append(int(i));
                    }
                    else
                    {
                        faceIndices[j] = faceIndex;
                    }
                }
                
                polyConnects[facePointIndex] = faceIndex;
            }

            base += curNum;
        }

        if (ioMesh.parentCount() != 0)
        {
            ioMesh.createInPlace(iPoints.length(), numPolys, iPoints,
                polyCounts, polyConnects);
        }
        else
        {
            ioMesh.create(iPoints.length(), numPolys, iPoints,
               polyCounts, polyConnects, iParent);
        }

        if (invalidFaces.length() > 0)
        {
            MGlobal::displayWarning("AbcImport: Mesh has invalid face(s). Remapping vertex indices.");

            unsigned int invalidFaceIndex = 0;
            unsigned int abcVertexIndex = 0;
            unsigned int mayaVertexIndex = 0;
            unsigned int mayaFaceIndex = 0;
            int nextInvalidFace = invalidFaces[invalidFaceIndex];
            
            remapIndices = true;
            
            if (!varyingTopology)
            {
                MStringArray bdLongNames;
                MStringArray bdShortNames;
                MStringArray bdTypes;
                
                // Create blind data types
                bdLongNames.append("abcVertexIndex");
                bdShortNames.append("abcvi");
                bdTypes.append("int");
                
                ioMesh.createBlindDataType(1001, bdLongNames, bdShortNames, bdTypes);
            }
            
            // Initialize blind data ids and values
            mayaVertexIndices.setLength(ioMesh.numFaceVertices());
            abcVertexIndices.setLength(ioMesh.numFaceVertices());
            
            // Fill blind data (maya index -> abc index for both face and vertex)
            for (int abcFaceIndex = 0; abcFaceIndex < int(numPolys); ++abcFaceIndex)
            {
                int vertexCount = polyCounts[abcFaceIndex];
                
                if (abcFaceIndex == nextInvalidFace)
                {
                    if (++invalidFaceIndex >= invalidFaces.length())
                    {
                        nextInvalidFace = -1;
                    }
                    else
                    {
                        nextInvalidFace = invalidFaces[invalidFaceIndex];
                    }
                }
                else
                {
                    for (int vertexIndex=0; vertexIndex<vertexCount; ++vertexIndex, ++mayaVertexIndex)
                    {
                        mayaVertexIndices[mayaVertexIndex] = mayaVertexIndex;
                        abcVertexIndices[mayaVertexIndex] = abcVertexIndex + vertexCount - vertexIndex - 1;
                    }
                }
                
                abcVertexIndex += vertexCount;
            }
            
            if (!varyingTopology)
            {
                ioMesh.setIntBlindData(mayaVertexIndices, MFn::kMeshFaceVertComponent, 1001, "abcvi", abcVertexIndices);
            }
        }
        else
        {
            remapIndices = false;
        }
    }

    void setUVSet(MFnMesh & ioMesh,
        const MFloatArray & iUlist, const MFloatArray & iVlist,
        const Alembic::Abc::UInt32ArraySamplePtr & iSampIndices, bool faceVarying,
        const MString & iUVSetName, bool remapIndices,
        const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices)
    {
        // per vertex per-polygon uv
        int numFaces = ioMesh.numPolygons();
        int nIndex = 0;

        MIntArray uvCounts(ioMesh.numPolygons(), 0);
        MIntArray uvIds(ioMesh.numFaceVertices(), 0);
        int uvCountsIndex = 0;

        // per-polygon per-vertex
        if (faceVarying)
        {
            if (remapIndices)
            {
                for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex)
                {
                    uvCounts[faceIndex] = ioMesh.polygonVertexCount(faceIndex);
                }
                
                for (unsigned int vertexIndex=0; vertexIndex<mayaVertexIndices.length(); ++vertexIndex)
                {
                    int mayaIndex = mayaVertexIndices[vertexIndex];
                    int abcIndex = abcVertexIndices[vertexIndex];
                    uvIds[mayaIndex] = (int)(*iSampIndices)[abcIndex];
                }
            }
            else
            {
                for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
                {
                    int numVertices = ioMesh.polygonVertexCount(faceIndex);
                    uvCounts[uvCountsIndex++] = numVertices;
                    int curIndex = nIndex;
                    for (int v = numVertices - 1; v >= 0; v--, ++nIndex)
                    {
                        uvIds[nIndex] = (int)(*iSampIndices)[curIndex + v];
                    }
                }
            }
        }
        // per-vertex
        else
        {
            for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
            {
                int numVertices = ioMesh.polygonVertexCount(faceIndex);
                uvCounts[uvCountsIndex++] = numVertices;
                MIntArray vertexList;
                ioMesh.getPolygonVertices (faceIndex, vertexList);
                for (int v = 0; v < numVertices; ++v, ++nIndex)
                {
                    uvIds[nIndex] = (int)(*iSampIndices)[vertexList[v]];
                }
            }
        }

        // clear out the UVs if we need to shrink the size of the UV table
        if (ioMesh.numUVs(iUVSetName) > (int) iUlist.length())
        {
            ioMesh.clearUVs(&iUVSetName);
        }

        MStatus status = ioMesh.setUVs(iUlist, iVlist, &iUVSetName);
        if (status != MStatus::kSuccess)
        {
            MGlobal::displayWarning("Couldnt set UVs for " + iUVSetName +
                status.errorString());
            return;
        }
        else
        {
            status = ioMesh.assignUVs(uvCounts, uvIds, &iUVSetName);
        }

        if (status != MStatus::kSuccess)
        {
            MGlobal::displayWarning("Couldnt assign UVs for " + iUVSetName +
                status.errorString());
        }
    }

    void setUV2f(double iFrame, MFnMesh & ioMesh,
        const Alembic::AbcGeom::IV2fGeomParam & iV2f,
        const Alembic::AbcGeom::IUInt32ArrayProperty & indexProperty,
        const MString & iUVSetName, bool remapIndices,
        const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices)
    {
        //Get the floor sample values
        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        double alpha = getWeightAndIndex(iFrame, iV2f.getTimeSampling(),
            iV2f.getNumSamples(), index, ceilIndex);

        Alembic::AbcGeom::IV2fGeomParam::Sample samp;
        iV2f.getIndexed(samp, Alembic::Abc::ISampleSelector(index));
        Alembic::Abc::V2fArraySamplePtr sampVal = samp.getVals();
        size_t sampSize = sampVal->size();

        if (ioMesh.numVertices() == 0)
        {
            return;
        }

        bool valid = false;
        bool faceVarying = (iV2f.getScope() == Alembic::AbcGeom::kFacevaryingScope);
        int indicesCount = samp.getIndices()->size();

        if (faceVarying)
        {
            if (remapIndices)
            {
                // maya has pruned some faces, be less struct about face vertices count
                valid = (ioMesh.numFaceVertices() <= indicesCount);
            }
            else
            {
                valid = (ioMesh.numFaceVertices() == indicesCount);
            }
        }
        else if (iV2f.getScope() == Alembic::AbcGeom::kVaryingScope ||
                 iV2f.getScope() == Alembic::AbcGeom::kVertexScope)
        {
            valid = (ioMesh.numVertices() == indicesCount);
        }
        else
        {
            return;
        }

        if (!valid)
        {
            MString msg = "UV set sample size is: ";
            msg += (int) samp.getIndices()->size();
            msg += " expecting: ";
            msg += ioMesh.numVertices();
            msg += " or ";
            msg += ioMesh.numFaceVertices();
            MGlobal::displayWarning(msg);
            return;
        }

        MFloatArray uList;
        MFloatArray vList;

        //Interpolate between 2 samples (as long as the indices are constant)
        //(non index is considered constant)
        if ( alpha != 0 && index != ceilIndex &&
            (!indexProperty || indexProperty.isConstant()) )
        {
            Alembic::AbcGeom::IV2fGeomParam::Sample ceilSamp;
            iV2f.getIndexed(ceilSamp,
                    Alembic::Abc::ISampleSelector(ceilIndex));
            Alembic::Abc::V2fArraySamplePtr ceilVal = ceilSamp.getVals();
            // Make sure the point count hasn't changed
            if (ceilVal->size() == sampSize)
            {
                for (size_t i = 0; i < sampSize; ++i)
                {
                    uList.append(
                            simpleLerp<float>(alpha, (*sampVal)[i].x,
                                    (*ceilVal)[i].x));
                    vList.append(
                            simpleLerp<float>(alpha, (*sampVal)[i].y,
                                    (*ceilVal)[i].y));
                }
            }
            else
            {
                for (size_t i = 0; i < sampSize; ++i)
                {
                    uList.append((*sampVal)[i].x);
                    vList.append((*sampVal)[i].y);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < sampSize; ++i)
            {
                uList.append((*sampVal)[i].x);
                vList.append((*sampVal)[i].y);
            }
        }

        setUVSet(ioMesh, uList, vList, samp.getIndices(), faceVarying, iUVSetName,
                 remapIndices, mayaVertexIndices, abcVertexIndices);
    }

    void createUVset(MFnMesh & meshIO, MString & iSetName)
    {
        if (meshIO.createUVSetDataMesh(iSetName) != MS::kSuccess)
        {
            meshIO.createUVSet(iSetName, NULL, NULL);
        }
    }

    void createColorSet(MFnMesh & meshIO, MString & iSetName,
        const Alembic::AbcCoreAbstract::MetaData & iMetaData)
    {

        MStatus status = meshIO.createColorSetDataMesh(iSetName);

        if (status != MS::kSuccess)
        {
            meshIO.createColorSetWithName(iSetName, NULL, NULL, NULL);
            if (iMetaData.get("mayaColorSet") == "1")
            {
                meshIO.setCurrentColorSetName(iSetName);
            }
// #if MAYA_API_VERSION > 201200
//             meshIO.setDisplayColors(true);
// #endif
        }

    }
    
    void setColor(MFnMesh & ioMesh, MColorArray & iColorList,
        Alembic::Abc::UInt32ArraySamplePtr & iSampIndices,
        bool faceVarying, MString & iColorSet,
        MFnMesh::MColorRepresentation iRepr, bool remapIndices,
        const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices)
    {
        // per vertex per-polygon color
        int numFaces = ioMesh.numPolygons();
        int nIndex = 0;

        MIntArray assignmentList(ioMesh.numFaceVertices());
        if (faceVarying)
        {
            if (remapIndices)
            {
                for (unsigned int i=0; i<mayaVertexIndices.length(); ++i)
                {
                    int mayaIndex = mayaVertexIndices[i];
                    int abcIndex = abcVertexIndices[i];
                    assignmentList[mayaIndex] = (int) (*iSampIndices)[abcIndex];
                }
            }
            else
            {
                for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
                {
                    int numVertices = ioMesh.polygonVertexCount(faceIndex);
                    int curIndex = nIndex;
                    for (int v = numVertices - 1; v >= 0; v--, ++nIndex)
                    {
                        assignmentList[nIndex] = (int)(*iSampIndices)[curIndex + v];
                    }
                }
            }
        }
        else
        {
            for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
            {
                int numVertices = ioMesh.polygonVertexCount(faceIndex);
                MIntArray vertexList;
                ioMesh.getPolygonVertices (faceIndex, vertexList);
                for (int v = 0; v < numVertices; ++v, ++nIndex)
                {
                    assignmentList[nIndex] =
                        (int)(*iSampIndices)[vertexList[v]];
                }
            }
        }
        
        // clear out the colors if we need to shrink the size of the color table
        if (ioMesh.numColors(iColorSet) > (int) iColorList.length())
        {
            ioMesh.clearColors(&iColorSet);
        }
        
        MStatus status = ioMesh.setColors(iColorList, &iColorSet, iRepr);
        if (status != MStatus::kSuccess)
        {
            MGlobal::displayWarning("Couldnt set colors for " + iColorSet +
                status.errorString());
            return;
        }
        else
        {
            status = ioMesh.assignColors(assignmentList, &iColorSet);
        }

        if (status != MStatus::kSuccess)
        {
            MGlobal::displayWarning("Couldnt assign colors for " + iColorSet +
                status.errorString());
        }
    }

    void setColor3f(double iFrame, MFnMesh & ioMesh,
        Alembic::AbcGeom::IC3fGeomParam & iC3f, bool remapIndices,
        const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices)
    {
        //Get the floor sample values
        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        double alpha = getWeightAndIndex(iFrame, iC3f.getTimeSampling(),
            iC3f.getNumSamples(), index, ceilIndex);

        Alembic::AbcGeom::IC3fGeomParam::Sample samp;
        iC3f.getIndexed(samp, Alembic::Abc::ISampleSelector(index));
        Alembic::Abc::C3fArraySamplePtr sampVal = samp.getVals();
        size_t sampSize = sampVal->size();

        if (ioMesh.numVertices() == 0)
        {
            return;
        }

        bool valid = false;
        bool faceVarying = (iC3f.getScope() == Alembic::AbcGeom::kFacevaryingScope);
        int indicesCount = (int) samp.getIndices()->size();

        if (faceVarying)
        {
            if (remapIndices)
            {
                valid = (ioMesh.numFaceVertices() <= indicesCount);
            }
            else
            {
                valid = (ioMesh.numFaceVertices() == indicesCount);
            }
        }
        else if (iC3f.getScope() == Alembic::AbcGeom::kVertexScope ||
                 iC3f.getScope() == Alembic::AbcGeom::kVaryingScope)
        {
            valid = (ioMesh.numVertices() == indicesCount);
        }
        else
        {
            return;
        }
        
        if (!valid)
        {
            MString msg = "Color sample size is: ";
            msg += (int) samp.getIndices()->size();
            msg += " expecting: ";
            msg += ioMesh.numVertices();
            msg += " or ";
            msg += ioMesh.numFaceVertices();
            MGlobal::displayWarning(msg);
            return;
        }

        MColorArray colorList;

        //Interpolate between 2 samples (as long as the indices are constant)
        //(non index is considered constant)
        if ( alpha != 0 && index != ceilIndex &&
            (!iC3f.getIndexProperty() || iC3f.getIndexProperty().isConstant()) )
        {
            Alembic::AbcGeom::IC3fGeomParam::Sample ceilSamp;
            iC3f.getIndexed(ceilSamp,
                    Alembic::Abc::ISampleSelector(ceilIndex));
            Alembic::Abc::C3fArraySamplePtr ceilVal = ceilSamp.getVals();
            // Make sure the point count hasn't changed
            if (sampSize == ceilVal->size())
            {
                for (size_t i = 0; i < sampSize; ++i)
                {
                    colorList.append(
                            simpleLerp<float>(alpha, (*sampVal)[i].x,
                                    (*ceilVal)[i].x),
                            simpleLerp<float>(alpha, (*sampVal)[i].y,
                                    (*ceilVal)[i].y),
                            simpleLerp<float>(alpha, (*sampVal)[i].z,
                                    (*ceilVal)[i].z));
                }
            }
            else
            {
                for (size_t i = 0; i < sampSize; ++i)
                {
                    colorList.append((*sampVal)[i].x, (*sampVal)[i].y,
                            (*sampVal)[i].z);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < sampSize; ++i)
            {
                colorList.append((*sampVal)[i].x, (*sampVal)[i].y,
                        (*sampVal)[i].z);
            }
        }

        MString colorSetName(iC3f.getName().c_str());
        Alembic::Abc::UInt32ArraySamplePtr indices = samp.getIndices();
        setColor(ioMesh, colorList, indices, faceVarying, colorSetName, MFnMesh::kRGB,
                 remapIndices, mayaVertexIndices, abcVertexIndices);
    }

    void setColor4f(double iFrame, MFnMesh & ioMesh,
        Alembic::AbcGeom::IC4fGeomParam & iC4f, bool remapIndices,
        const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices)
    {
        //Get the floor sample values
        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        double alpha = getWeightAndIndex(iFrame, iC4f.getTimeSampling(),
            iC4f.getNumSamples(), index, ceilIndex);

        Alembic::AbcGeom::IC4fGeomParam::Sample samp;
        iC4f.getIndexed(samp, Alembic::Abc::ISampleSelector(index));
        Alembic::Abc::C4fArraySamplePtr sampVal = samp.getVals();
        size_t sampSize = sampVal->size();

        if (ioMesh.numVertices() == 0)
        {
            return;
        }

        bool valid = false;
        bool faceVarying = (iC4f.getScope() == Alembic::AbcGeom::kFacevaryingScope);
        int indicesCount = (int) samp.getIndices()->size();

        if (faceVarying)
        {
            if (remapIndices)
            {
                valid = (ioMesh.numFaceVertices() <= indicesCount);
            }
            else
            {
                valid = (ioMesh.numFaceVertices() == indicesCount);
            }
        }
        else if (iC4f.getScope() == Alembic::AbcGeom::kVertexScope ||
                 iC4f.getScope() == Alembic::AbcGeom::kVaryingScope)
        {
            valid = (ioMesh.numVertices() == indicesCount);
        }
        else
        {
            return;
        }
        
        if (!valid)
        {
            MString msg = "Color sample size is: ";
            msg += (int) samp.getIndices()->size();
            msg += " expecting: ";
            msg += ioMesh.numVertices();
            msg += " or ";
            msg += ioMesh.numFaceVertices();
            MGlobal::displayWarning(msg);
            return;
        }

        MColorArray colorList;

        //Interpolate between 2 samples
        if ( alpha != 0 && index != ceilIndex &&
            (!iC4f.getIndexProperty() || iC4f.getIndexProperty().isConstant()) )
        {
            Alembic::AbcGeom::IC4fGeomParam::Sample ceilSamp;
            iC4f.getIndexed(ceilSamp,
                    Alembic::Abc::ISampleSelector(ceilIndex));
            Alembic::Abc::C4fArraySamplePtr ceilVal = ceilSamp.getVals();
            // Make sure the point count hasn't changed
            if (sampSize == ceilVal->size())
            {
                for (size_t i = 0; i < sampSize; ++i)
                {
                    colorList.append(
                            simpleLerp<float>(alpha, (*sampVal)[i].r,
                                    (*ceilVal)[i].r),
                            simpleLerp<float>(alpha, (*sampVal)[i].g,
                                    (*ceilVal)[i].g),
                            simpleLerp<float>(alpha, (*sampVal)[i].b,
                                    (*ceilVal)[i].b),
                            simpleLerp<float>(alpha, (*sampVal)[i].a,
                                    (*ceilVal)[i].a)
                                    );
                }
            }
            else
            {
                for (size_t i = 0; i < sampSize; ++i)
                {
                    colorList.append((*sampVal)[i].r, (*sampVal)[i].g,
                            (*sampVal)[i].b, (*sampVal)[i].a);
                }
            }
        }
        else
        {
            for (size_t i = 0; i < sampSize; ++i)
            {
                colorList.append((*sampVal)[i].r, (*sampVal)[i].g,
                        (*sampVal)[i].b, (*sampVal)[i].a);
            }
        }

        MString colorSetName(iC4f.getName().c_str());
        Alembic::Abc::UInt32ArraySamplePtr indices = samp.getIndices();
        setColor(ioMesh, colorList, indices, faceVarying, colorSetName, MFnMesh::kRGBA,
                 remapIndices, mayaVertexIndices, abcVertexIndices);
    }

    typedef std::vector< Alembic::AbcGeom::IV2fGeomParam > IV2fGPVec;
    typedef std::vector< Alembic::AbcGeom::IC3fGeomParam > IC3fGPVec;
    typedef std::vector< Alembic::AbcGeom::IC4fGeomParam > IC4fGPVec;

    void setColorsAndUVs(double iFrame, MFnMesh & ioMesh,
        Alembic::AbcGeom::IV2fGeomParam iPrimaryV2f,
        IV2fGPVec iV2s, IC3fGPVec iC3s, IC4fGPVec iC4s,
        bool remapIndices, const MIntArray &mayaVertexIndices, const MIntArray &abcVertexIndices,
        bool iSetStatic)
    {
        if (iPrimaryV2f.getNumSamples() < 1 && iV2s.empty() && iC3s.empty() &&
            iC4s.empty())
        {
            return;
        }

        MStringArray uvSetNames;
        ioMesh.getUVSetNames(uvSetNames);

        if (iPrimaryV2f.getNumSamples() > 0 &&
            (iSetStatic || !iPrimaryV2f.isConstant()))
        {
            MString uvSetName =
                Alembic::Abc::GetSourceName(iPrimaryV2f.getMetaData()).c_str();

            if (uvSetName.length() == 0)
            {
                uvSetName = "map1";
            }

            if (!inStrArray(uvSetNames, uvSetName))
            {
                createUVset(ioMesh, uvSetName);
                ioMesh.setCurrentUVSetName(uvSetName);
                uvSetNames.append(uvSetName);
            }
            setUV2f(iFrame, ioMesh, iPrimaryV2f,
                iPrimaryV2f.getIndexProperty(), uvSetName,
                remapIndices, mayaVertexIndices, abcVertexIndices);
        }

        IV2fGPVec::const_iterator v2sEnd = iV2s.end();
        for (IV2fGPVec::iterator v2s = iV2s.begin(); v2s != v2sEnd; ++v2s)
        {
            if (v2s->getNumSamples() > 0 && (iSetStatic || !v2s->isConstant()))
            {
                MString uvSetName(v2s->getName().c_str());
                if (!inStrArray(uvSetNames, uvSetName))
                {
                    createUVset(ioMesh, uvSetName);
                    uvSetNames.append(uvSetName);
                }
                setUV2f(iFrame, ioMesh, *v2s, v2s->getIndexProperty(), uvSetName,
                        remapIndices, mayaVertexIndices, abcVertexIndices);
            }
        }

        MStringArray colorSetNames;
        ioMesh.getColorSetNames(colorSetNames);

        IC3fGPVec::const_iterator c3sEnd = iC3s.end();
        for (IC3fGPVec::iterator c3s = iC3s.begin(); c3s != c3sEnd; ++c3s)
        {
            if (c3s->getNumSamples() > 0 && (iSetStatic || !c3s->isConstant()))
            {
                MString colorSetName(c3s->getName().c_str());
                if (!inStrArray(colorSetNames, colorSetName))
                {
                    createColorSet(ioMesh, colorSetName, c3s->getMetaData());
                    colorSetNames.append(colorSetName);
                }
                setColor3f(iFrame, ioMesh, *c3s,
                           remapIndices, mayaVertexIndices, abcVertexIndices);
            }
        }

        IC4fGPVec::const_iterator c4sEnd = iC4s.end();
        for (IC4fGPVec::iterator c4s = iC4s.begin(); c4s != c4sEnd; ++c4s)
        {
            if (c4s->getNumSamples() > 0 && (iSetStatic || !c4s->isConstant()))
            {
                MString colorSetName(c4s->getName().c_str());
                if (!inStrArray(colorSetNames, colorSetName))
                {
                    createColorSet(ioMesh, colorSetName, c4s->getMetaData());
                    colorSetNames.append(colorSetName);
                }
                setColor4f(iFrame, ioMesh, *c4s,
                           remapIndices, mayaVertexIndices, abcVertexIndices);
            }
        }
    }

    void fillCreases(MFnMesh & ioMesh, SubDAndFriends & iNode,
                     Alembic::Abc::FloatArraySamplePtr creases,
                     Alembic::Abc::Int32ArraySamplePtr indices,
                     Alembic::Abc::Int32ArraySamplePtr lengths)
    {
        if (!creases || creases->size() == 0)
        {
            return;
        }

        // based on logic from the gpuCache, create edge map for edgeId lookup
        // unordered_map might be a better choice for this, but it isn't quite
        // portable yet.  Hopefully when everyone is mostly on good C++11
        // compliant compilers and beyond we can switch it.
        typedef std::map<std::pair<int, int>, int> EdgeMap;
        EdgeMap edgeMap;
        int numEdges = ioMesh.numEdges();
        for (int i = 0; i < numEdges; i++)
        {
            int vertexList[2];
            ioMesh.getEdgeVertices(i, vertexList);

            if (vertexList[0] > vertexList[1])
            {
                std::swap(vertexList[0], vertexList[1]);
            }
            edgeMap.insert(std::make_pair(
                std::make_pair(vertexList[0], vertexList[1]), i));
        }

        std::size_t numLengths = lengths->size();

        MUintArray edgeIds;
        MDoubleArray creaseData;

        std::size_t curIndex = 0;

        // curIndex incremented here to move on to the next crease length
        for (std::size_t i = 0; i < numLengths; ++i, ++curIndex)
        {
            std::size_t len = (*lengths)[i] - 1;
            float creaseSharpness = (*creases)[i];

            // curIndex incremented here to go between all the edges that make
            // up a given length
            for (std::size_t j = 0; j < len; ++j, ++curIndex)
            {
                Alembic::Util::int32_t vertA = (*indices)[curIndex];
                Alembic::Util::int32_t vertB = (*indices)[curIndex+1];
                std::pair<int, int> edge = std::make_pair(vertA, vertB);
                if (edge.first > edge.second)
                {
                    std::swap(edge.first, edge.second);
                }
                EdgeMap::iterator iter = edgeMap.find(edge);
                if (iter != edgeMap.end() && iter->second < numEdges)
                {
                    creaseData.append(creaseSharpness);
                    edgeIds.append(iter->second);
                }
            }
        }

        if (ioMesh.setCreaseEdges(edgeIds, creaseData) != MS::kSuccess)
        {
            MString warn = "Failed to set creases on: ";
            warn += iNode.mMesh.getName().c_str();
            printWarning(warn);
        }
    }

    void fillCorners(MFnMesh & ioMesh, SubDAndFriends & iNode,
                     Alembic::Abc::FloatArraySamplePtr corners,
                     Alembic::Abc::Int32ArraySamplePtr cornerVerts)
    {
        if (!corners || corners->size() == 0)
        {
            return;
        }

        unsigned int numCorners = static_cast<unsigned int>(corners->size());
        MUintArray vertIds(numCorners);
        MDoubleArray cornerData(numCorners);

        for (unsigned int i = 0; i < numCorners; ++i)
        {
            cornerData[i] = (*corners)[i];
            vertIds[i] = (*cornerVerts)[i];
        }

        if (ioMesh.setCreaseVertices(vertIds, cornerData) != MS::kSuccess)
        {
            MString warn = "Failed to set corners on: ";
            warn += iNode.mMesh.getName().c_str();
            printWarning(warn);
        }
    }

    void fillHoles(MFnMesh & ioMesh, SubDAndFriends & iNode,
                   Alembic::Abc::Int32ArraySamplePtr holes)
    {
    #if MAYA_API_VERSION >= 201100
        if (!holes || holes->size() == 0)
        {
            return;
        }

        unsigned int numHoles = (unsigned int)holes->size();
        MUintArray holeData(numHoles);
        for (unsigned int i = 0; i < numHoles; ++i)
        {
            holeData[i] = (*holes)[i];
        }

        if (ioMesh.setInvisibleFaces(holeData) != MS::kSuccess)
        {
            MString warn = "Failed to set holes on: ";
            warn += iNode.mMesh.getName().c_str();
            printWarning(warn);
        }
    #endif
    }

    void fillCreasesCornersAndHoles(MFnMesh & ioMesh, SubDAndFriends & iNode,
        Alembic::AbcGeom::ISubDSchema::Sample &samp)
    {
        fillCreases(ioMesh, iNode, samp.getCreaseSharpnesses(),
                    samp.getCreaseIndices(), samp.getCreaseLengths());
        fillCorners(ioMesh, iNode, samp.getCornerSharpnesses(),
                    samp.getCornerIndices());
        fillHoles(ioMesh, iNode, samp.getHoles());
    }

}  // namespace

void readPoly(double iFrame, bool iReadNormals, MFnMesh & ioMesh, MObject & iParent,
    PolyMeshAndFriends & iNode, bool iInitialized)
{
    Alembic::AbcGeom::IPolyMeshSchema schema = iNode.mMesh.getSchema();
    Alembic::AbcGeom::MeshTopologyVariance ttype = schema.getTopologyVariance();

    Alembic::AbcCoreAbstract::index_t index, ceilIndex;
    double alpha = getWeightAndIndex(iFrame,
        schema.getTimeSampling(), schema.getNumSamples(), index, ceilIndex);

    MFloatPointArray pointArray;
    Alembic::Abc::P3fArraySamplePtr ceilPoints;

    MIntArray mayaVertexIndices;
    MIntArray abcVertexIndices;
    bool remapIndices = false;

    // we can just read the points
    if (ttype != Alembic::AbcGeom::kHeterogenousTopology && iInitialized)
    {
        if (ioMesh.hasBlindData(MFn::kMeshFaceVertComponent, 1001))
        {
            remapIndices = true,
            ioMesh.getIntBlindData(MFn::kMeshFaceVertComponent, 1001, "abcvi", mayaVertexIndices, abcVertexIndices);
        }

        Alembic::Abc::P3fArraySamplePtr points = schema.getPositionsProperty(
            ).getValue(Alembic::Abc::ISampleSelector(index));

        if (alpha != 0.0)
        {
            ceilPoints = schema.getPositionsProperty().getValue(
                Alembic::Abc::ISampleSelector(ceilIndex) );
        }

       fillPoints(pointArray, points, ceilPoints, alpha);
       if(pointArray.length() > 0)
       {
           ioMesh.setPoints(pointArray, MSpace::kObject);
       }

        setColorsAndUVs(iFrame, ioMesh, schema.getUVsParam(),
            iNode.mV2s, iNode.mC3s, iNode.mC4s,
            remapIndices, mayaVertexIndices, abcVertexIndices,
            !iInitialized);

        if (!iInitialized && iReadNormals)
        {
            Alembic::Abc::ICompoundProperty up = schema.getUserProperties();
            if (up && up.getPropertyHeader("mayaHardEdges") != NULL)
            {
                Alembic::Abc::IV2iArrayProperty hardEdgesProp(up, "mayaHardEdges");
                setPolySmoothEdges(iFrame, ioMesh, hardEdgesProp);
            }
        }

        if (iReadNormals && schema.getNormalsParam().getNumSamples() > 1)
        {
            setPolyNormals(iFrame, ioMesh, schema.getNormalsParam(),
                           remapIndices, mayaVertexIndices, abcVertexIndices);
        }

        return;
    }

    // we need to read the topology
    Alembic::AbcGeom::IPolyMeshSchema::Sample samp;
    schema.get(samp, Alembic::Abc::ISampleSelector(index));

    if (alpha != 0.0 && ttype != Alembic::AbcGeom::kHeterogenousTopology)
    {
        ceilPoints = schema.getPositionsProperty().getValue(
            Alembic::Abc::ISampleSelector(ceilIndex) );
    }

    fillPoints(pointArray, samp.getPositions(), ceilPoints, alpha);

    fillTopology(ioMesh, iParent, pointArray, samp.getFaceIndices(),
        samp.getFaceCounts(), ttype == Alembic::AbcGeom::kHeterogenousTopology,
        remapIndices, mayaVertexIndices, abcVertexIndices);

    if (iReadNormals)
    {
        setPolyNormals(iFrame, ioMesh, schema.getNormalsParam(),
                       remapIndices, mayaVertexIndices, abcVertexIndices);
    }
    
    setColorsAndUVs(iFrame, ioMesh, schema.getUVsParam(),
        iNode.mV2s, iNode.mC3s, iNode.mC4s,
        remapIndices, mayaVertexIndices, abcVertexIndices,
        !iInitialized);
}

void readSubD(double iFrame, MFnMesh & ioMesh, MObject & iParent,
    SubDAndFriends & iNode, bool iInitialized)
{
    Alembic::AbcGeom::ISubDSchema schema = iNode.mMesh.getSchema();
    Alembic::AbcGeom::MeshTopologyVariance tv = schema.getTopologyVariance();

    Alembic::AbcCoreAbstract::index_t index, ceilIndex;
    double alpha = getWeightAndIndex(iFrame,
        schema.getTimeSampling(), schema.getNumSamples(), index, ceilIndex);

    MFloatPointArray pointArray;
    Alembic::Abc::P3fArraySamplePtr ceilPoints;

    MIntArray mayaVertexIndices;
    MIntArray abcVertexIndices;
    bool remapIndices = false;

    // we can just read the points
    if (tv != Alembic::AbcGeom::kHeterogenousTopology && iInitialized)
    {
        if (ioMesh.hasBlindData(MFn::kMeshFaceVertComponent, 1001))
        {
            remapIndices = true,
            ioMesh.getIntBlindData(MFn::kMeshFaceVertComponent, 1001, "abcvi", mayaVertexIndices, abcVertexIndices);
        }
        
        Alembic::Abc::ISampleSelector sampSel(index);
        Alembic::Abc::P3fArraySamplePtr points =
            schema.getPositionsProperty().getValue(sampSel);

        if (alpha != 0.0)
        {
            ceilPoints = schema.getPositionsProperty().getValue(
                Alembic::Abc::ISampleSelector(ceilIndex) );
        }

        fillPoints(pointArray, points, ceilPoints, alpha);
        ioMesh.setPoints(pointArray, MSpace::kObject);

        setColorsAndUVs(iFrame, ioMesh, schema.getUVsParam(), iNode.mV2s,
            iNode.mC3s, iNode.mC4s,
            remapIndices, mayaVertexIndices, abcVertexIndices,
            !iInitialized);

        return;
    }

    // we need to read the topology
    Alembic::AbcGeom::ISubDSchema::Sample samp;
    schema.get(samp, Alembic::Abc::ISampleSelector(index));

    if (alpha != 0.0 && tv != Alembic::AbcGeom::kHeterogenousTopology)
    {
        ceilPoints = schema.getPositionsProperty().getValue(
            Alembic::Abc::ISampleSelector(ceilIndex) );
    }

    fillPoints(pointArray, samp.getPositions(), ceilPoints, alpha);

    fillTopology(ioMesh, iParent, pointArray, samp.getFaceIndices(),
        samp.getFaceCounts(), tv == Alembic::AbcGeom::kHeterogenousTopology,
        remapIndices, mayaVertexIndices, abcVertexIndices);

    setColorsAndUVs(iFrame, ioMesh, schema.getUVsParam(),
        iNode.mV2s, iNode.mC3s, iNode.mC4s,
        remapIndices, mayaVertexIndices, abcVertexIndices,
        !iInitialized);
    
    fillCreasesCornersAndHoles(ioMesh, iNode, samp);
}

void disconnectMesh(MObject & iMeshObject,
    std::vector<Prop> & iSampledPropList,
    std::size_t iFirstProp)
{
    MFnMesh fnMesh;
    fnMesh.setObject(iMeshObject);

    // disconnect old connection from AlembicNode or some other nodes
    // to inMesh if one such connection exist
    MPlug dstPlug = fnMesh.findPlug("inMesh", true);
    disconnectAllPlugsTo(dstPlug);

    disconnectProps(fnMesh, iSampledPropList, iFirstProp);

    clearPt(fnMesh);

    return;

}

MObject createPoly(double iFrame, bool iReadNormals, PolyMeshAndFriends & iNode, MObject & iParent)
{
    Alembic::AbcGeom::IPolyMeshSchema &schema = iNode.mMesh.getSchema();
    MString name(iNode.mMesh.getName().c_str());

    MObject obj;

    MIntArray mayaVertexIndices;
    MIntArray abcVertexIndices;
    bool remapIndices = false;

    // add other properties
    if (!schema.isConstant())
    {
        MFloatPointArray emptyPt;
        MIntArray emptyInt;
        MFnMesh fnMesh;
        obj = fnMesh.create(0, 0, emptyPt, emptyInt, emptyInt, iParent);
        fnMesh.setName(name);
    }
    else
    {
        Alembic::AbcCoreAbstract::index_t index, ceilIndex;
        double alpha = getWeightAndIndex(iFrame, schema.getTimeSampling(),
            schema.getNumSamples(), index, ceilIndex);

        Alembic::AbcGeom::IPolyMeshSchema::Sample samp;
        schema.get(samp, Alembic::Abc::ISampleSelector(index));

        MFloatPointArray ptArray;
        Alembic::Abc::P3fArraySamplePtr ceilPoints;
        if (index != ceilIndex)
        {
            Alembic::AbcGeom::IPolyMeshSchema::Sample ceilSamp;
            schema.get(ceilSamp, Alembic::Abc::ISampleSelector(ceilIndex));
            ceilPoints = ceilSamp.getPositions();
        }

        fillPoints(ptArray, samp.getPositions(), ceilPoints, alpha);

        MFnMesh fnMesh;

        fillTopology(fnMesh, iParent, ptArray, samp.getFaceIndices(),
            samp.getFaceCounts(), schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology,
            remapIndices, mayaVertexIndices, abcVertexIndices);
        
        fnMesh.setName(iNode.mMesh.getName().c_str());

        if (iReadNormals)
        {
            // setPolyNormals(iFrame, fnMesh, schema.getNormalsParam(),
            //                remapIndices, mayaVertexIndices, abcVertexIndices);
            Alembic::Abc::ICompoundProperty up = schema.getUserProperties();
            if (up && up.getPropertyHeader("mayaHardEdges") != NULL)
            {
                Alembic::Abc::IV2iArrayProperty hardEdgesProp(up, "mayaHardEdges");
                setPolySmoothEdges(iFrame, fnMesh, hardEdgesProp);
            }
        }

        obj = fnMesh.object();
    }

    MFnMesh fnMesh(obj);

    MString pathName = fnMesh.partialPathName();
    setInitialShadingGroup(pathName);

    setColorsAndUVs(iFrame, fnMesh, schema.getUVsParam(),
        iNode.mV2s, iNode.mC3s, iNode.mC4s,
        remapIndices, mayaVertexIndices, abcVertexIndices,
        true);

    // if ( !schema.getNormalsParam().valid() )
    // {
    //     MFnNumericAttribute attr;
    //     MString attrName("noNormals");
    //     MObject attrObj = attr.create(attrName, attrName,
    //     MFnNumericData::kBoolean, true);
    //     attr.setKeyable(true);
    //     attr.setHidden(false);
    //     fnMesh.addAttribute(attrObj);
    // }

    return obj;
}

MObject createSubD(double iFrame, SubDAndFriends & iNode, MObject & iParent)
{
    Alembic::AbcGeom::ISubDSchema schema = iNode.mMesh.getSchema();

    Alembic::AbcCoreAbstract::index_t index, ceilIndex;
    getWeightAndIndex(iFrame, schema.getTimeSampling(),
        schema.getNumSamples(), index, ceilIndex);

    Alembic::AbcGeom::ISubDSchema::Sample samp;
    schema.get(samp, Alembic::Abc::ISampleSelector(index));

    MString name(iNode.mMesh.getName().c_str());

    MFnMesh fnMesh;

    MIntArray mayaVertexIndices;
    MIntArray abcVertexIndices;
    bool remapIndices = false;

    MFloatPointArray pointArray;
    Alembic::Abc::P3fArraySamplePtr emptyPtr;
    fillPoints(pointArray, samp.getPositions(), emptyPtr, 0.0);

    fillTopology(fnMesh, iParent, pointArray, samp.getFaceIndices(),
        samp.getFaceCounts(), schema.getTopologyVariance() == Alembic::AbcGeom::kHeterogenousTopology,
        remapIndices, mayaVertexIndices, abcVertexIndices);
    
    fnMesh.setName(iNode.mMesh.getName().c_str());

    setInitialShadingGroup(fnMesh.partialPathName());

    MObject obj = fnMesh.object();

    setColorsAndUVs(iFrame, fnMesh, schema.getUVsParam(),
        iNode.mV2s, iNode.mC3s, iNode.mC4s,
        remapIndices, mayaVertexIndices, abcVertexIndices,
        true);

    // add the mFn-specific attributes to fnMesh node
    MFnNumericAttribute numAttr;
    MString attrName("SubDivisionMesh");
    MObject attrObj = numAttr.create(attrName, attrName,
        MFnNumericData::kBoolean, 1);
    numAttr.setKeyable(true);
    numAttr.setHidden(false);
    fnMesh.addAttribute(attrObj);

    if (samp.getInterpolateBoundary() > 0)
    {
        attrName = MString("interpolateBoundary");
        attrObj = numAttr.create(attrName, attrName, MFnNumericData::kBoolean,
            samp.getInterpolateBoundary());

        numAttr.setKeyable(true);
        numAttr.setHidden(false);
        fnMesh.addAttribute(attrObj);
    }

    if (samp.getFaceVaryingInterpolateBoundary() > 0)
    {
        attrName = MString("faceVaryingInterpolateBoundary");
        attrObj = numAttr.create(attrName, attrName, MFnNumericData::kBoolean,
            samp.getFaceVaryingInterpolateBoundary());

        numAttr.setKeyable(true);
        numAttr.setHidden(false);
        fnMesh.addAttribute(attrObj);
    }

    if (samp.getFaceVaryingPropagateCorners() > 0)
    {
        attrName = MString("faceVaryingPropagateCorners");
        attrObj = numAttr.create(attrName, attrName, MFnNumericData::kBoolean,
            samp.getFaceVaryingPropagateCorners());

        numAttr.setKeyable(true);
        numAttr.setHidden(false);
        fnMesh.addAttribute(attrObj);
    }

    fillCreasesCornersAndHoles(fnMesh, iNode, samp);

    return obj;
}
