//-*****************************************************************************
//
// Copyright (c) 2009-2011,
//  Sony Pictures Imageworks Inc. and
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
// Industrial Light & Magic, nor the names of their contributors may be used
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

#include <cstring>
#include <memory>
#include "ProcArgs.h"
#include "PathUtil.h"
#include "SampleUtil.h"
#include "WriteGeo.h"
#include "ArbGeomParams.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>

#include <regex.h>


namespace
{

using namespace Alembic::AbcGeom;


template <class MeshClass>
void ProcessMeshFacesets( MeshClass &mesh, ProcArgs &args, regex_t *expr, AtNode *amesh )
{
    std::vector<std::string> faceSetNames;
       
    mesh.getSchema().getFaceSetNames( faceSetNames );
    
    if ( faceSetNames.size() > 0 )
    {
        ISampleSelector frameSelector( args.frame / args.fps );
        
        std::set<int> facesToKeep;
        
        for ( size_t i=0; i<faceSetNames.size(); ++i )
        {
            IFaceSet faceSet = mesh.getSchema().getFaceSet( faceSetNames[i] );
            
            if ( !expr || regexec( expr, faceSet.getFullName().c_str(), 0, NULL, 0 ) == 0 )
            {
                IFaceSetSchema::Sample faceSetSample = faceSet.getSchema().getValue( frameSelector );
                Alembic::Abc::Int32ArraySamplePtr faces = faceSetSample.getFaces();
                
                facesToKeep.insert( faces->get(), faces->get() + faces->size() );
            }
        }
        
        if ( facesToKeep.size() > 0 )
        {
            AtArray *nsides = AiNodeGetArray( amesh, "nsides" );
            
            bool *faceVisArray = new bool[nsides->nelements];
            
            for ( unsigned int i=0; i<nsides->nelements; ++i )
            {
                faceVisArray[i] = ( facesToKeep.find( i ) != facesToKeep.end() );
            }
            
            if ( AiNodeDeclare( amesh, "face_visibility", "uniform BOOL" ) )
            {
                AiNodeSetArray( amesh, "face_visibility", ArrayConvert( nsides->nelements, 1, AI_TYPE_BOOLEAN, faceVisArray ) );
            }
            
            delete[] faceVisArray;
        }
    }
}


void WalkObject( IObject parent, const ObjectHeader &ohead, ProcArgs &args,
                 regex_t *expr,
                 MatrixSampleMap * xformSamples)
{
    //Accumulate transformation samples and pass along as an argument
    //to WalkObject
    
    IObject nextParentObject;
    
    std::auto_ptr<MatrixSampleMap> concatenatedXformSamples;
    
    if ( IXform::matches( ohead ) )
    {
        if ( args.excludeXform )
        {
            nextParentObject = IObject( parent, ohead.getName() );
        }
        else
        {
            IXform xform( parent, ohead.getName() );
            
            IXformSchema &xs = xform.getSchema();
            
            if ( xs.getNumOps() > 0 )
            { 
                TimeSamplingPtr ts = xs.getTimeSampling();
                size_t numSamples = xs.getNumSamples();
                
                SampleTimeSet sampleTimes;
                GetRelevantSampleTimes( args, ts, numSamples, sampleTimes,
                        xformSamples);
                
                MatrixSampleMap localXformSamples;
                
                MatrixSampleMap * localXformSamplesToFill = 0;
                
                concatenatedXformSamples.reset(new MatrixSampleMap);
                
                if ( !xformSamples )
                {
                    // If we don't have parent xform samples, we can fill
                    // in the map directly.
                    localXformSamplesToFill = concatenatedXformSamples.get();
                }
                else
                {
                    //otherwise we need to fill in a temporary map
                    localXformSamplesToFill = &localXformSamples;
                }
                
                
                for (SampleTimeSet::iterator I = sampleTimes.begin();
                        I != sampleTimes.end(); ++I)
                {
                    XformSample sample = xform.getSchema().getValue(
                            Abc::ISampleSelector(*I));
                    (*localXformSamplesToFill)[(*I)] = sample.getMatrix();
                }
                
                if ( xformSamples )
                {
                    ConcatenateXformSamples(args,
                            *xformSamples,
                            localXformSamples,
                            *concatenatedXformSamples.get());
                }
                
                
                xformSamples = concatenatedXformSamples.get();
                
            }
            
            nextParentObject = xform;
        }
    }
    else if ( ISubD::matches( ohead ) )
    {
        ISubD subd( parent, ohead.getName() );
        
        if ( !expr || regexec( expr, ohead.getFullName().c_str(), 0, NULL, 0 ) == 0 )
        {
            size_t nn = args.createdNodes.size();
            
            ProcessSubD( subd, args, xformSamples, "" );
            
            AtNode *amesh = ( args.createdNodes.size() == nn ? 0 : args.createdNodes.back() );
            
            if ( amesh && AiNodeIs( amesh, "polymesh" ) )
            {
                ProcessMeshFacesets( subd, args, expr, amesh );
            }
        }
    }
    else if ( IPolyMesh::matches( ohead ) )
    {
        IPolyMesh polymesh( parent, ohead.getName() );
        
        if ( !expr || regexec( expr, ohead.getFullName().c_str(), 0, NULL, 0 ) == 0 )
        {
            size_t nn = args.createdNodes.size();
            
            ProcessPolyMesh( polymesh, args, xformSamples, "" );
            
            AtNode *amesh = ( args.createdNodes.size() == nn ? 0 : args.createdNodes.back() );
            
            if ( amesh && AiNodeIs( amesh, "polymesh" ) )
            {
                ProcessMeshFacesets( polymesh, args, expr, amesh );
            }
        }
    }
    else if ( INuPatch::matches( ohead ) )
    {
        INuPatch patch( parent, ohead.getName() );
        // TODO ProcessNuPatch( patch, args );
        
        if ( !expr || regexec( expr, ohead.getFullName().c_str(), 0, NULL, 0 ) == 0 )
        {
            nextParentObject = patch;
        }
    }
    else if ( IPoints::matches( ohead ) )
    {
        IPoints points( parent, ohead.getName() );
        // TODO ProcessPoints( points, args );
        
        if ( !expr || regexec( expr, ohead.getFullName().c_str(), 0, NULL, 0 ) == 0 )
        {
            nextParentObject = points;
        }
    }
    else if ( ICurves::matches( ohead ) )
    {
        ICurves curves( parent, ohead.getName() );
        // TODO ProcessCurves( curves, args );
        
        if ( !expr || regexec( expr, ohead.getFullName().c_str(), 0, NULL, 0 ) == 0 )
        {
            nextParentObject = curves;
        }
    }
    else if ( IFaceSet::matches( ohead ) )
    {
        //don't complain about discovering a faceset upon traversal
    }
    else
    {
        std::cerr << "could not determine type of " << ohead.getName()
                  << std::endl;
        
        std::cerr << ohead.getName() << " has MetaData: "
                  << ohead.getMetaData().serialize() << std::endl;
        
        nextParentObject = parent.getChild(ohead.getName());
    }
    
    if ( nextParentObject.valid() )
    {
        //std::cerr << nextParentObject.getFullName() << std::endl;
        
        for ( size_t i=0; i<nextParentObject.getNumChildren(); ++i )
        {
            WalkObject( nextParentObject, nextParentObject.getChildHeader(i), args, expr, xformSamples );
        }
    }
}

//-*************************************************************************

int ProcInit( struct AtNode *node, void **user_ptr )
{
    ProcArgs * args = new ProcArgs( AiNodeGetStr( node, "data" ) );
    args->proceduralNode = node;
    *user_ptr = args;
    
#if (AI_VERSION_ARCH_NUM == 3 && AI_VERSION_MAJOR_NUM < 3) || AI_VERSION_ARCH_NUM < 3
    #error Arnold version 3.3+ required for AlembicArnoldProcedural
#endif
    
    if (!AiCheckAPIVersion(AI_VERSION_ARCH, AI_VERSION_MAJOR, AI_VERSION_MINOR))
    {
        std::cout << PREFIX_NAME("AlembicArnoldProcedural") << " compiled with arnold-"
                  << AI_VERSION
                  << " but is running with incompatible arnold-"
                  << AiGetVersion(NULL, NULL, NULL, NULL) << std::endl;
        return 1;
    }

    if ( args->filename.empty() )
    {
        args->usage();
        return 1;
    }

    IArchive archive( ::Alembic::AbcCoreHDF5::ReadArchive(),
                      args->filename );

    IObject root = archive.getTop();

    regex_t re;
    bool filter = (args->objectpath.length() > 0);
    
    if (filter)
    {
        if (regcomp(&re, args->objectpath.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
        {
            AiMsgWarning("[%s] Invalid object path expression: \"%s\"",
                         PREFIX_NAME("AlembicArnoldProcedural"),
                         args->objectpath.c_str());
            filter = false;
        }
    }

    try
    {
        for ( size_t i = 0; i < root.getNumChildren(); ++i )
        {
            WalkObject( root, root.getChildHeader(i), *args, (filter ? &re : 0), 0 );
        }
    }
    catch ( const std::exception &e )
    {
        std::cerr << "exception thrown during ProcInit: "
              << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "exception thrown\n";
    }
    
    if (filter)
    {
        regfree(&re);
    }
    
    return 1;
}

//-*************************************************************************

int ProcCleanup( void *user_ptr )
{
    delete reinterpret_cast<ProcArgs*>( user_ptr );
    return 1;
}

//-*************************************************************************

int ProcNumNodes( void *user_ptr )
{
    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );
    return (int) args->createdNodes.size();
}

//-*************************************************************************

struct AtNode* ProcGetNode(void *user_ptr, int i)
{
    ProcArgs * args = reinterpret_cast<ProcArgs*>( user_ptr );
    
    if ( i >= 0 && i < (int) args->createdNodes.size() )
    {
        return args->createdNodes[i];
    }
    
    return NULL;
}

} //end of anonymous namespace



proc_loader
{
    vtable->Init        = ProcInit;
    vtable->Cleanup     = ProcCleanup;
    vtable->NumNodes    = ProcNumNodes;
    vtable->GetNode     = ProcGetNode;
    strcpy(vtable->version, AI_VERSION);
    return 1;
}
