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

#include "AlembicFileTranslator.h"
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MAnimControl.h>
#include <cstring>

static MString EscapeString(const MString &s)
{
    std::string tmp = s.asChar();
    
    size_t p = 0;
    
    while (p < tmp.length())
    {
       if (tmp[p] == '\\' || tmp[p] == '\"')
       {
          tmp.insert(p, "\\");
          p += 2;
       }
       else
       {
          ++p;
       }
    }
    
    return tmp.c_str();
}

MStatus AlembicFileTranslator::reader(const MFileObject& file,
                                      const MString& optionsString,
                                      MPxFileTranslator::FileAccessMode mode)
{
    MString script = PREFIX_NAME("AbcImport");
    MString options;
    MString impmode;
    MStringArray optlist;
    bool reparent = false;
    bool addnodes = false;
    bool remnodes = false;
    MString submode = "Add";

    MFileObject::MFileResolveMethod resmeth = MFileObject::kInputFile;
    
    if (mode == MPxFileTranslator::kOpenAccessMode)
    {
        impmode = " -m open";
    }
    
    else if (mode == MPxFileTranslator::kImportAccessMode ||
             mode == MPxFileTranslator::kReferenceAccessMode)
    {
        impmode = " -m import";

        if (mode == MPxFileTranslator::kReferenceAccessMode)
        {
            resmeth = MFileObject::kInputReference;
        }
    }
    else
    {
        return MStatus::kFailure;
    }
    
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
                options += " -ftr";
            }
        }
        else if (key == "rcs")
        {
            if (val == "1")
            {
                options += " -rcs";
            }
        }
        else if (key == "rus")
        {
            if (val == "1")
            {
                options += " -rus";
            }
        }
        else if (key == "d")
        {
            if (val == "1")
            {
                options += " -d";
            }
        }
        else if (key == "ft")
        {
            if (val.length() > 0)
            {
                options += " -ft \"" + val + "\"";
            }
        }
        else if (key == "eft")
        {
            if (val.length() > 0)
            {
                options += " -eft \"" + val + "\"";
            }
        }
        else if (key == "sts")
        {
            if (val == "1")
            {
                options += " -sts";
            }
        }
        else if (key == "ci")
        {
            if (val == "1")
            {
                options += " -ci";
            }
        }
        else if (key == "crm")
        {
            if (val == "1")
            {
                options += " -crm";
            }
        }
        else if (mode == MPxFileTranslator::kImportAccessMode)
        {
            if (key == "mode")
            {
                if (val == "Add" || val == "Update" || val == "Replace")
                {
                    submode = val;
                    if (val == "Replace")
                    {
                        impmode = " -m replace";
                    }
                }
            }
            else if (key == "rpr")
            {
                reparent = (val == "1");
            }
            else if (key == "crt")
            {
                addnodes = (val == "1");
            }
            else if (key == "rm")
            {
                remnodes = (val == "1");
            }
        }
    }
    
    if (submode == "Add")
    {
        if (reparent)
        {
            MSelectionList sel;
            MDagPath path;
            
            MGlobal::getActiveSelectionList(sel);
            
            for (unsigned int i=0; i<sel.length(); ++i)
            {
                if (sel.getDagPath(i, path) == MStatus::kSuccess)
                {
                    options += " -rpr \"" + path.fullPathName() + "\"";
                    break;
                }
            }
        }
    }
    else if (submode == "Update")
    {
        MSelectionList sel;
        MDagPath path;
        MString roots;
        
        MGlobal::getActiveSelectionList(sel);
        
        for (unsigned int i=0; i<sel.length(); ++i)
        {
            if (sel.getDagPath(i, path) == MStatus::kSuccess)
            {
                if (roots.length() > 0)
                {
                    roots += " ";
                }
                roots += path.partialPathName();
            }
        }
        
        options += " -connect \"" + roots + "\"";
        if (addnodes)
        {
            options += " -crt";
        }
        if (remnodes)
        {
            options += " -rm";
        }
    }
    
    MFileObject fobj(file);

    fobj.setResolveMethod(resmeth);

    script += impmode + options + " \"" + fobj.resolvedFullName() + "\";";

    MStatus status = MGlobal::executeCommand(script, true);

    return status;
}

MStatus AlembicFileTranslator::writer(const MFileObject& file,
                                      const MString& optionsString,
                                      MPxFileTranslator::FileAccessMode mode)
{
    MString script = PREFIX_NAME("AbcExport");
    MString options;
    MString jobopts;
    MStringArray optlist;
    
    if (mode == MPxFileTranslator::kExportActiveAccessMode)
    {
        MSelectionList sel;
        MDagPath path;

        MGlobal::getActiveSelectionList(sel);

        for (unsigned int i=0; i<sel.length(); ++i)
        {
            if (sel.getDagPath(i, path) == MStatus::kSuccess)
            {
                jobopts += " -root " + path.partialPathName();
            }
        }
    }
    else if (mode != MPxFileTranslator::kExportAccessMode)
    {
        return MStatus::kFailure;
    }
    
    optionsString.split(';', optlist);
    double startFrame = MAnimControl::currentTime().value();
    double endFrame = startFrame;
    double preRollFrames = 0.0;
    double frameStep = 1.0;
    MDoubleArray frameRelativeSamples;
    
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
        
        
        if (key == "ff")
        {
            if (val.isFloat())
            {
                startFrame = double(val.asFloat());
            }
            else if (val.isDouble())
            {
                startFrame = val.asDouble();
            }
        }
        else if (key == "lf")
        {
            if (val.isFloat())
            {
                endFrame = double(val.asFloat());
            }
            else if (val.isDouble())
            {
                endFrame = val.asDouble();
            }
        }
        else if (key == "s")
        {
            if (val.isFloat())
            {
                frameStep = double(val.asFloat());
            }
            else if (val.isDouble())
            {
                frameStep = val.asDouble();
            }
        }
        else if (key == "frs")
        {
            MStringArray samplesStr;
            val.split(' ', samplesStr);
            for (unsigned int j=0; j<samplesStr.length(); ++j)
            {
                if (samplesStr[j].isFloat())
                {
                    frameRelativeSamples.append(double(samplesStr[j].asFloat()));
                }
                else if (samplesStr[j].isDouble())
                {
                    frameRelativeSamples.append(samplesStr[j].asDouble());
                }
            }
        }
        else if (key == "pr")
        {
            if (val.isFloat())
            {
                preRollFrames = double(val.asFloat());
            }
            else if (val.isDouble())
            {
                preRollFrames = val.asDouble();
            }
        }
        else if (key == "df")
        {
            jobopts += " -df " + val;
        }
        else if (key == "v")
        {
            if (val == "1")
            {
                options += " -v";
            }
        }
        else if (key == "sn")
        {
            if (val == "1")
            {
                jobopts += " -sn";
            }
        }
        else if (key == "wfg")
        {
            if (val == "1")
            {
                jobopts += " -wfg";
            }
        }
        else if (key == "ro")
        {
            if (val == "1")
            {
                jobopts += " -ro";
            }
        }
        else if (key == "ws")
        {
            if (val == "1")
            {
                jobopts += " -ws";
            }
        }
        else if (key == "wv")
        {
            if (val == "1")
            {
                jobopts += " -wv";
            }
        }
        else if (key == "wi")
        {
            if (val == "1")
            {
                jobopts += " -wi";
            }
        }
        else if (key == "wrm")
        {
            if (val == "1")
            {
                jobopts += " -wrm";
            }
        }
        else if (key == "ef")
        {
            if (val == "1")
            {
                jobopts += " -ef";
            }
        }
        else if (key == "nn")
        {
            if (val == "1")
            {
                jobopts += " -nn";
            }
        }
        else if (key == "uv")
        {
            if (val == "1")
            {
                jobopts += " -uv";
            }
        }
        else if (key == "wuvs")
        {
            if (val == "1")
            {
                jobopts += " -wuvs";
            }
        }
        else if (key == "uvo")
        {
            if (val == "1")
            {
                jobopts += " -uvo";
            }
        }
        else if (key == "as")
        {
            if (val == "1")
            {
                jobopts += " -as";
            }
        }
        else if (key == "wcs")
        {
            if (val == "1")
            {
                jobopts += " -wcs";
            }
        }
        else if (key == "wfs")
        {
            if (val == "1")
            {
                jobopts += " -wfs";
            }
        }
        else if (key == "a")
        {
            MStringArray attrs;
            val.split(' ', attrs);
            for (unsigned int j=0; j<attrs.length(); ++j)
            {
                if (attrs[j].length() > 0)
                {
                    jobopts += " -a " + attrs[j];
                }
            }
        }
        else if (key == "u")
        {
            MStringArray attrs;
            val.split(' ', attrs);
            for (unsigned int j=0; j<attrs.length(); ++j)
            {
                if (attrs[j].length() > 0)
                {
                    jobopts += " -u " + attrs[j];
                }
            }
        }
        else if (key == "atp")
        {
            MStringArray prefs;
            val.split(' ', prefs);
            for (unsigned int j=0; j<prefs.length(); ++j)
            {
                if (prefs[j].length() > 0)
                {
                    jobopts += " -atp " + prefs[j];
                }
            }
        }
        else if (key == "uatp")
        {
            MStringArray prefs;
            val.split(' ', prefs);
            for (unsigned int j=0; j<prefs.length(); ++j)
            {
                if (prefs[j].length() > 0)
                {
                    jobopts += " -uatp " + prefs[j];
                }
            }
        }
        else if (key == "fc" || key == "pc")
        {
            if (!strncmp(val.asChar(), "<py>", 4))
            {
                while (val.length() > 5 && strncmp(val.asChar() + val.length() - 5, "</py>", 5))
                {
                    ++i;
                    if (i >= optlist.length())
                    {
                        break;
                    }
                    val += ";" + optlist[i];
                }
                val = val.substring(4, val.length() - 6);
                jobopts += MString(key == "fc" ? " -pfc \"" : " -ppc \"") + EscapeString(val) + "\"";
            }
            else if (!strncmp(val.asChar(), "<mel>", 5))
            {
                while (val.length() > 6 && strncmp(val.asChar() + val.length() - 6, "</mel>", 6))
                {
                    ++i;
                    if (i >= optlist.length())
                    {
                        break;
                    }
                    val += ";" + optlist[i];
                }
                val = val.substring(5, val.length() - 7);
                jobopts += MString(key == "fc" ? " -mfc \"" : " -mpc \"") + EscapeString(val) + "\"";
            }
        }
    }
    
    char tmpbuf[1024];
    sprintf(tmpbuf, "-fr %f %f -s %f", startFrame, endFrame, frameStep);
    MString sampopts = tmpbuf;
    
    for (unsigned int i=0; i<frameRelativeSamples.length(); ++i)
    {
        sprintf(tmpbuf, " -frs %f", frameRelativeSamples[i]);
        sampopts += tmpbuf;
    }
    
    if (preRollFrames > 0.0)
    {
        sprintf(tmpbuf, " -prs %f", startFrame-preRollFrames);
        sampopts += tmpbuf;
    }
    
    jobopts += " -file " + file.resolvedFullName();
    
    jobopts = EscapeString(sampopts + jobopts);
    
    script += options + " -j \"" + jobopts + "\"";
    
    MStatus status = MGlobal::executeCommand(script, true);
    
    return status;
}

MPxFileTranslator::MFileKind
AlembicFileTranslator::identifyFile(const MFileObject& file,
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

