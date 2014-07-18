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
    MIntArray outId = fnAttrs.intArray("particleId", &status);
    MDoubleArray outCount = fnAttrs.doubleArray("count", &status);
    
    size_t pSize = inFloorPos->size();
    
    char tmpbuf[256];
    sprintf(tmpbuf, "%lu", pSize);
    MGlobal::displayInfo("Read " + MString(tmpbuf) + " particle(s)");
    
    outCount.setLength(1);
    outCount[0] = double(pSize);
    
    if (blend > 0.0)
    {
        schema.get(ceilSamp, ceilSampSel);
                
        Alembic::Abc::P3fArraySamplePtr inCeilPos = ceilSamp.getPositions();
        Alembic::Abc::UInt64ArraySamplePtr inCeilId = ceilSamp.getIds();
        
        std::map<size_t, size_t> idmap;
        std::map<size_t, size_t>::iterator idit;
        
        for (size_t j=0; j<inCeilId->size(); ++j)
        {
            idmap[(*inCeilId)[j]] = j;
        }
        
        double dt = blend * (ceilTime - floorTime);
            
        outPos.setLength(pSize);
        outVel.setLength(pSize);
        outId.setLength(pSize);
        
        for (unsigned int pId = 0; pId < pSize; pId++)
        {
            MVector vel((*inFloorVel)[pId].x,
                        (*inFloorVel)[pId].y,
                        (*inFloorVel)[pId].z);
            
            outId[pId] = (*inFloorId)[pId];
            outVel[pId] = vel;
            
            idit = idmap.find(outId[pId]);
            
            if (idit == idmap.end())
            {
                outPos[pId] = MVector((*inFloorPos)[pId].x + dt * vel.x,
                                      (*inFloorPos)[pId].y + dt * vel.y,
                                      (*inFloorPos)[pId].z + dt * vel.z);
                
                
            }
            else
            {
                MVector v0((*inFloorPos)[pId].x,
                           (*inFloorPos)[pId].y,
                           (*inFloorPos)[pId].z);
                
                MVector v1((*inCeilPos)[idit->second].x,
                           (*inCeilPos)[idit->second].y,
                           (*inCeilPos)[idit->second].z);
                
                outPos[pId] = (1.0 - blend) * v0 + blend * v1;
            }
        }
    }
    else
    {
        outPos.setLength(pSize);
        outVel.setLength(pSize);
        outId.setLength(pSize);
        
        for (unsigned int pId = 0; pId < pSize; pId++)
        {
            outPos[pId] = MVector((*inFloorPos)[pId].x,
                                  (*inFloorPos)[pId].y,
                                  (*inFloorPos)[pId].z);
            
            outVel[pId] = MVector((*inFloorVel)[pId].x,
                                  (*inFloorVel)[pId].y,
                                  (*inFloorVel)[pId].z);
            
            outId[pId] = (*inFloorId)[pId];
        }
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
    //iObject = fnParticle.create(iParent, &status);
    MDagModifier dagMod;
    // This is not enough! It won't connect to nucleus
    iObject = dagMod.createNode("nParticle", iParent, &status);
    status = dagMod.doIt();
    fnParticle.setObject(iObject);
    fnParticle.setName(iNode.getName().c_str());

    if (pSize > 0)
    {
        MPointArray pArray;
        MDoubleArray iArray;
        MVectorArray vArray;
        
        Alembic::Abc::P3fArraySamplePtr posPtr = samp.getPositions();
        Alembic::Abc::V3fArraySamplePtr velPtr = samp.getVelocities();
        Alembic::Abc::UInt64ArraySamplePtr idPtr = samp.getIds();

        for (unsigned int pId = 0; pId < pSize; pId++)
        {
            pArray.append((*posPtr)[pId].x,
                          (*posPtr)[pId].y,
                          (*posPtr)[pId].z);
            
            vArray.append(MVector((*velPtr)[pId].x,
                                  (*velPtr)[pId].y,
                                  (*velPtr)[pId].z));
            
            iArray.append((*idPtr)[pId]);
        }

        // velocities? ids? other attributes?
        status = fnParticle.emit(pArray, vArray);
        fnParticle.setPerParticleAttribute("particleId", iArray, &status);
    }
    
    status = fnParticle.saveInitialState();
    
    MPlug isDyn = fnParticle.findPlug("isDynamic");
    isDyn.setBool(false);

    return status;
}
