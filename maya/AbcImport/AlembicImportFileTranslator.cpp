//-*****************************************************************************
//
// Copyright (c) 2009-2012,
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

#include "AlembicImportFileTranslator.h"

#include <maya/MGlobal.h>

MStatus AlembicImportFileTranslator::reader(
                                        const MFileObject& file,
                                        const MString& optionsString,
                                        MPxFileTranslator::FileAccessMode mode)
{
    MString script = "AbcImport";
    MString impmode;
    
    if (mode == MPxFileTranslator::kOpenAccessMode)
    {
        impmode = "-m open";
    }
    else if (mode == MPxFileTranslator::kImportAccessMode)
    {
        impmode = "-m import";
    }
    else
    {
        return MStatus::kFailure;
    }
    
    MStringArray optlist;
    optionsString.split(';', optlist);
    
    for (unsigned int i=0; i<optlist.length(); ++i)
    {
        MStringArray keyval;
        
        optlist[i].split('=', keyval);
        
        if (keyval.length() < 2)
        {
            continue;
        }
        
        MString key = keyval[0];
        MString val = keyval[1];
        for (unsigned int j=2; j<keyval.length(); ++j)
        {
            val += "=";
            val += keyval[j];
        }
        
        if (key == "ftr")
        {
            if (val == "1")
            {
                script += " -ftr";
            }
        }
        else if (key == "rcs")
        {
            if (val == "1")
            {
                script += " -rcs";
            }
        }
        else if (key == "rus")
        {
            if (val == "1")
            {
                script += " -rus";
            }
        }
        else if (key == "d")
        {
            if (val == "1")
            {
                script += " -d";
            }
        }
        else if (key == "ft")
        {
            if (val.length() > 0)
            {
                script += " -ft \"" + val + "\"";
            }
        }
        else if (key == "eft")
        {
            if (val.length() > 0)
            {
                script += " -eft \"" + val + "\"";
            }
        }
    }
    
    //script.format ("AbcImport ^1s ^2s \"^3s\";", impmode, optionsString, file.resolvedFullName());
    
    script += " \"" + file.resolvedFullName() + "\";";

    MStatus status = MGlobal::executeCommand (script);

    return status;
}

MPxFileTranslator::MFileKind
AlembicImportFileTranslator::identifyFile(const MFileObject& file,
                                          const char* buffer,
                                          short size) const
{
    MString name = file.resolvedName();
    unsigned int len = name.numChars();
    if (len > 4 && name.substringW(len - 4, len - 1).toLowerCase() == ".abc")
    {
        return kIsMyFileType;
    }

    return kNotMyFileType;
}
