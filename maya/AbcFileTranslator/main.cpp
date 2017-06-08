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

#include "AlembicFileTranslator.h"
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <Alembic/AbcCoreAbstract/All.h>

#ifndef ABCFILETRANSLATOR_VERSION
#  define ABCFILETRANSLATOR_VERSION "1.0"
#endif

static MStatus CheckLoadPlugin(const MString name)
{
    MString scr = "if (!`pluginInfo -query -loaded \"" + name + "\"`) loadPlugin \"" + name + "\";";
    return MGlobal::executeCommand(scr);
}

PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
    MString impname = PREFIX_NAME("AbcImport");
    MString expname = PREFIX_NAME("AbcExport");
    MString name = PREFIX_NAME("Alembic");
    MStatus status;

    MString info = PREFIX_NAME("AbcFileTranslator");
    info += " v";
    info += ABCFILETRANSLATOR_VERSION;
    info += " using ";
    info += Alembic::AbcCoreAbstract::GetLibraryVersion().c_str();
    MGlobal::displayInfo(info);

    MFnPlugin plugin(obj, name.asChar(), ABCFILETRANSLATOR_VERSION, "Any");

    status = CheckLoadPlugin(impname);

    if (!status)
    {
        status.perror(PREFIX_NAME("AbcFileTranslator"));
        return status;
    }

    status = CheckLoadPlugin(expname);

    if (!status)
    {
        status.perror(PREFIX_NAME("AbcFileTranslator"));
        return status;
    }

    status = plugin.registerFileTranslator(name,
                                           NULL,
                                           AlembicFileTranslator::creator,
                                           "alembicTranslatorOptions",
                                           "",
                                           true);
    if (!status)
    {
        status.perror("registerFileTranslator");
    }

    return status;
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);

    MString name = PREFIX_NAME("Alembic");

    MStatus status;

    status = plugin.deregisterFileTranslator(name);
    if (!status)
    {
        status.perror("deregisterFileTranslator");
    }

    return status;
}

