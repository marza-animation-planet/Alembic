#include "AbcShape.h"
#include "AlembicSceneVisitors.h"
#include "AlembicSceneCache.h"

#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MFileObject.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MMaterial.h>
#include <maya/MDagPath.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MFnStringData.h>
#include <maya/MSelectionMask.h>
#include <maya/MFnCamera.h>
#include <maya/MSelectionList.h>
#include <maya/MDGModifier.h>
#include <maya/MGlobal.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#include <vector>
#include <map>

#ifdef ABCSHAPE_VRAY_SUPPORT

#include <maya/MItDependencyNodes.h>
#include <maya/MPlugArray.h>
#include <maya/MSyntax.h>
#include <maya/MArgParser.h>
#include <maya/MFnSet.h>

AbcShapeVRayDisp::DispTexMap AbcShapeVRayDisp::DispTexs;
AbcShapeVRayDisp::DispSetMap AbcShapeVRayDisp::DispSets;

void* AbcShapeVRayDisp::create()
{
   return new AbcShapeVRayDisp();
}

MSyntax AbcShapeVRayDisp::createSyntax()
{
   MSyntax syntax;
   
   syntax.addFlag("-i", "-init", MSyntax::kNoArg);
   syntax.addFlag("-r", "-reset", MSyntax::kNoArg);
   syntax.addFlag("-dl", "-displist", MSyntax::kNoArg);
   syntax.addFlag("-d", "-disp", MSyntax::kString);
   syntax.addFlag("-a", "-assigned", MSyntax::kString);
   syntax.addFlag("-f", "-float", MSyntax::kNoArg);
   syntax.addFlag("-c", "-color", MSyntax::kNoArg);
   
   syntax.setMinObjects(0);
   syntax.setMaxObjects(0);
   syntax.enableQuery(false);
   syntax.enableEdit(false);
   
   return syntax;
}

bool AbcShapeVRayDisp::getAssignedDisplacement(const MDagPath &inPath, std::string &setName, std::string &shaderName)
{
   DispSetMap::const_iterator it;
   
   MDagPath path = inPath;
     
   while (path.length() > 0)
   {
      it = DispSets.find(path.fullPathName().asChar());
      
      if (it != DispSets.end())
      {
         setName = it->second.setName;
         shaderName = it->second.shaderName;
         return true;
      }
      else
      {
         path.pop();
      }
   }
   
   return false;
}

AbcShapeVRayDisp::AbcShapeVRayDisp()
   : MPxCommand()
{
}
   
AbcShapeVRayDisp::~AbcShapeVRayDisp()
{
}

bool AbcShapeVRayDisp::hasSyntax() const
{
   return true;
}
   
bool AbcShapeVRayDisp::isUndoable() const
{
   return false;
}

MStatus AbcShapeVRayDisp::doIt(const MArgList& args)
{
   MStatus status;
   
   MArgParser argData(syntax(), args, &status);
   
   if (argData.isFlagSet("init"))
   {
      DispSets.clear();
      
      MItDependencyNodes nodeIt(MFn::kSet);
      
      while (!nodeIt.isDone())
      {
         MObject obj = nodeIt.thisNode();
         
         MFnSet dispSet(obj);
         
         if (dispSet.typeName() == "VRayDisplacement")
         {
            MPlug pDisp = dispSet.findPlug("displacement");
               
            if (!pDisp.isNull())
            {
               MPlugArray srcs;
               
               pDisp.connectedTo(srcs, true, false);
               
               std::string shaderName;
               
               if (srcs.length() > 0)
               {
                  MObject shdObj = srcs[0].node();
                  
                  shaderName = MFnDependencyNode(shdObj).name().asChar();
               }
               
               MSelectionList members;
               MDagPath memberPath;
               
               dispSet.getMembers(members, false);
               
               for (unsigned int i=0; i<members.length(); ++i)
               {
                  if (members.getDagPath(i, memberPath) == MS::kSuccess)
                  {
                     DispSet &ds = DispSets[memberPath.fullPathName().asChar()];
                     
                     ds.setName = dispSet.name().asChar();
                     ds.shaderName = shaderName;
                     
                     // Fully expand children too?
                     // beware then that deeper level assignment takes precedence
                  }
               }
            }
         }
         
         nodeIt.next();
      }
      
      return MS::kSuccess;
   }
   else if (argData.isFlagSet("reset"))
   {
      DispTexs.clear();
      
      return MS::kSuccess;
   }
   else
   {
      if (argData.isFlagSet("assigned"))
      {
         MSelectionList sl;
         MStringArray rv;
         MString val;
         MDagPath path;
         
         status = argData.getFlagArgument("assigned", 0, val);
         if (status != MS::kSuccess)
         {
            MGlobal::displayError("Invalid valud for -assigned flag (" + val + ")");
            return status;
         }
         
         if (sl.add(val) == MS::kSuccess && sl.getDagPath(0, path) == MS::kSuccess)
         {
            std::string setn, shdn;
            
            if (getAssignedDisplacement(path, setn, shdn))
            {
               rv.append(setn.c_str());
               rv.append(shdn.c_str());
            }
         }
         
         setResult(rv);
         return MS::kSuccess;
      }
      else if (argData.isFlagSet("displist"))
      {
         MStringArray names;
         
         for (DispTexMap::iterator it = DispTexs.begin(); it != DispTexs.end(); ++it)
         {
            names.append(it->first.c_str());
         }
         
         setResult(names);
         
         return MS::kSuccess;
      }
      else if (!argData.isFlagSet("disp"))
      {
         MGlobal::displayError("Either -disp or -displist flag must be set");
         return MS::kFailure;
      }
      else if (!argData.isFlagSet("color") && !argData.isFlagSet("float"))
      {
         MGlobal::displayError("Either -color or -float flag must be set");
         return MS::kFailure;
      }
      else
      {
         MString val;
         
         status = argData.getFlagArgument("disp", 0, val);
         if (status != MS::kSuccess)
         {
            MGlobal::displayError("Invalid valud for -disp flag (" + val + ")");
            return status;
         }
         
         DispTexMap::iterator it = DispTexs.find(val.asChar());
         
         MStringArray names;
         
         if (it != DispTexs.end())
         {
            DispShapes &dispShapes = it->second;
            
            if (argData.isFlagSet("color"))
            {
               if (argData.isFlagSet("float"))
               {
                  MGlobal::displayError("-float/-color cannot be used at the same time");
                  return MS::kFailure;
               }
               
               for (NameSet::iterator nit = dispShapes.asColor.begin(); nit != dispShapes.asColor.end(); ++nit)
               {
                  names.append(nit->c_str());
               }
            }
            else
            {
               for (NameSet::iterator nit = dispShapes.asFloat.begin(); nit != dispShapes.asFloat.end(); ++nit)
               {
                  names.append(nit->c_str());
               }
            }
         }
         
         setResult(names);
         
         return MS::kSuccess;
      }
   }
}

#endif

#define MCHECKERROR(STAT,MSG)                   \
    if (!STAT) {                                \
        perror(MSG);                            \
        return MS::kFailure;                    \
    }


const MTypeId AbcShape::ID(0x00082698);
MCallbackId AbcShape::CallbackID;
MObject AbcShape::aFilePath;
MObject AbcShape::aObjectExpression;
MObject AbcShape::aDisplayMode;
MObject AbcShape::aTime;
MObject AbcShape::aSpeed;
MObject AbcShape::aPreserveStartFrame;
MObject AbcShape::aStartFrame;
MObject AbcShape::aEndFrame;
MObject AbcShape::aOffset;
MObject AbcShape::aCycleType;
MObject AbcShape::aIgnoreXforms;
MObject AbcShape::aIgnoreInstances;
MObject AbcShape::aIgnoreVisibility;
MObject AbcShape::aNumShapes;
MObject AbcShape::aPointWidth;
MObject AbcShape::aLineWidth;
MObject AbcShape::aDrawTransformBounds;
MObject AbcShape::aDrawLocators;
MObject AbcShape::aOutBoxMin;
MObject AbcShape::aOutBoxMax;
MObject AbcShape::aAnimated;
#ifdef ABCSHAPE_VRAY_SUPPORT
MObject AbcShape::aOutApiType;
MObject AbcShape::aVRayGeomResult;
MObject AbcShape::aVRayGeomInfo;
MObject AbcShape::aVRayAbcVerbose;
MObject AbcShape::aVRayAbcReferenceFilename;
MObject AbcShape::aVRayAbcParticleType;
MObject AbcShape::aVRayAbcParticleAttribs;
MObject AbcShape::aVRayAbcSpriteSizeX;
MObject AbcShape::aVRayAbcSpriteSizeY;
MObject AbcShape::aVRayAbcSpriteTwist;
//MObject AbcShape::aVRayAbcSpriteOrientation;
MObject AbcShape::aVRayAbcRadius;
MObject AbcShape::aVRayAbcPointSize;
//MObject AbcShape::aVRayAbcPointRadii;
//MObject AbcShape::aVRayAbcPointWorldSize;
MObject AbcShape::aVRayAbcMultiCount;
MObject AbcShape::aVRayAbcMultiRadius;
MObject AbcShape::aVRayAbcLineWidth;
MObject AbcShape::aVRayAbcTailLength;
MObject AbcShape::aVRayAbcSortIDs;
MObject AbcShape::aVRayAbcPsizeScale;
MObject AbcShape::aVRayAbcPsizeMin;
MObject AbcShape::aVRayAbcPsizeMax;
#endif

void* AbcShape::creator()
{
   return new AbcShape();
}

MStatus AbcShape::initialize()
{
   MStatus stat;
   MFnTypedAttribute tAttr;
   MFnEnumAttribute eAttr;
   MFnUnitAttribute uAttr;
   MFnNumericAttribute nAttr;
   
   MFnStringData filePathDefault;
   MObject filePathDefaultObject = filePathDefault.create("");
   aFilePath = tAttr.create("filePath", "fp", MFnData::kString, filePathDefaultObject, &stat);
   MCHECKERROR(stat, "Could not create 'filePath' attribute");
   tAttr.setInternal(true);
   tAttr.setUsedAsFilename(true);
   stat = addAttribute(aFilePath);
   MCHECKERROR(stat, "Could not add 'filePath' attribute");
   
   aObjectExpression = tAttr.create("objectExpression", "oe", MFnData::kString, MObject::kNullObj, &stat);
   MCHECKERROR(stat, "Could not create 'objectExpression' attribute");
   tAttr.setInternal(true);
   stat = addAttribute(aObjectExpression);
   MCHECKERROR(stat, "Could not add 'objectExpression' attribute");
   
   aDisplayMode = eAttr.create("displayMode", "dm");
   MCHECKERROR(stat, "Could not create 'displayMode' attribute");
   eAttr.addField("Box", DM_box);
   eAttr.addField("Boxes", DM_boxes);
   eAttr.addField("Points", DM_points);
   eAttr.addField("Geometry", DM_geometry);
   eAttr.setInternal(true);
   stat = addAttribute(aDisplayMode);
   MCHECKERROR(stat, "Could not add 'displayMode' attribute");
   
   aTime = uAttr.create("time", "tm", MFnUnitAttribute::kTime, 0.0);
   MCHECKERROR(stat, "Could not create 'time' attribute");
   uAttr.setStorable(true);
   uAttr.setInternal(true);
   stat = addAttribute(aTime);
   MCHECKERROR(stat, "Could not add 'time' attribute");
   
   aSpeed = nAttr.create("speed", "sp", MFnNumericData::kDouble, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'speed' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aSpeed);
   MCHECKERROR(stat, "Could not add 'speed' attribute");
   
   aPreserveStartFrame = nAttr.create("preserveStartFrame", "psf", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'preserveStartFrame' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(false);
   nAttr.setInternal(true);
   stat = addAttribute(aPreserveStartFrame);
   MCHECKERROR(stat, "Could not add 'preserveStartFrame' attribute");

   aOffset = nAttr.create("offset", "of", MFnNumericData::kDouble, 0, &stat);
   MCHECKERROR(stat, "Could not create 'offset' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aOffset);
   MCHECKERROR(stat, "Could not add 'offset' attribute");

   aCycleType = eAttr.create("cycleType", "ct", 0,  &stat);
   MCHECKERROR(stat, "Could not create 'cycleType' attribute");
   eAttr.addField("Hold", CT_hold);
   eAttr.addField("Loop", CT_loop);
   eAttr.addField("Reverse", CT_reverse);
   eAttr.addField("Bounce", CT_bounce);
   eAttr.setWritable(true);
   eAttr.setStorable(true);
   eAttr.setKeyable(true);
   eAttr.setInternal(true);
   stat = addAttribute(aCycleType);
   MCHECKERROR(stat, "Could not add 'cycleType' attribute");
   
   aStartFrame = nAttr.create("startFrame", "sf", MFnNumericData::kDouble, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'startFrame' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aStartFrame);
   MCHECKERROR(stat, "Could not add 'startFrame' attribute");

   aEndFrame = nAttr.create("endFrame", "ef", MFnNumericData::kDouble, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'endFrame' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aEndFrame);
   MCHECKERROR(stat, "Could not add 'endFrame' attribute");
   
   aIgnoreXforms = nAttr.create("ignoreXforms", "ixf", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'ignoreXforms' attribute")
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aIgnoreXforms);
   MCHECKERROR(stat, "Could not add 'ignoreXforms' attribute");
   
   aIgnoreInstances = nAttr.create("ignoreInstances", "iin", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'ignoreInstances' attribute")
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aIgnoreInstances);
   MCHECKERROR(stat, "Could not add 'ignoreInstances' attribute");
   
   aIgnoreVisibility = nAttr.create("ignoreVisibility", "ivi", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'ignoreVisibility' attribute")
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aIgnoreVisibility);
   MCHECKERROR(stat, "Could not add 'ignoreVisibility' attribute");
   
   aNumShapes = nAttr.create("numShapes", "nsh", MFnNumericData::kLong, 0, &stat);
   nAttr.setWritable(false);
   nAttr.setStorable(false);
   stat = addAttribute(aNumShapes);
   MCHECKERROR(stat, "Could not add 'numShapes' attribute");
   
   aPointWidth = nAttr.create("pointWidth", "ptw", MFnNumericData::kFloat, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'pointWidth' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aPointWidth);
   MCHECKERROR(stat, "Could not add 'pointWidth' attribute");
   
   aLineWidth = nAttr.create("lineWidth", "lnw", MFnNumericData::kFloat, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'lineWidth' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aLineWidth);
   MCHECKERROR(stat, "Could not add 'lineWidth' attribute");
   
   aDrawTransformBounds = nAttr.create("drawTransformBounds", "dtb", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'drawTransformBounds' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aDrawTransformBounds);
   MCHECKERROR(stat, "Could not add 'drawTransformBounds' attribute");
   
   aDrawLocators = nAttr.create("drawLocators", "dl", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'drawLocators' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setInternal(true);
   stat = addAttribute(aDrawLocators);
   MCHECKERROR(stat, "Could not add 'drawLocators' attribute");
   
   aOutBoxMin = nAttr.create("outBoxMin", "obmin", MFnNumericData::k3Double, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'outBoxMin' attribute");
   nAttr.setWritable(false);
   nAttr.setStorable(false);
   stat = addAttribute(aOutBoxMin);
   MCHECKERROR(stat, "Could not add 'outBoxMin' attribute");
   
   aOutBoxMax = nAttr.create("outBoxMax", "obmax", MFnNumericData::k3Double, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'outBoxMax' attribute");
   nAttr.setWritable(false);
   nAttr.setStorable(false);
   stat = addAttribute(aOutBoxMax);
   MCHECKERROR(stat, "Could not add 'outBoxMax' attribute");
   
   aAnimated = nAttr.create("animated", "anm", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'animated' attribute");
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setHidden(true);
   stat = addAttribute(aAnimated);
   MCHECKERROR(stat, "Could not add 'animated' attribute");
   
#ifdef ABCSHAPE_VRAY_SUPPORT
   MFnStringData outApiTypeDefault;
   MObject outApiTypeDefaultObject = outApiTypeDefault.create("VRayGeometry");
   aOutApiType = tAttr.create("outApiType", "oat", MFnData::kString, outApiTypeDefaultObject, &stat);
   tAttr.setKeyable(false);
   tAttr.setStorable(true);
   tAttr.setHidden(true);
   MCHECKERROR(stat, "Could not create 'outApiType' attribute");
   stat = addAttribute(aOutApiType);
   MCHECKERROR(stat, "Could not add 'outApiType' attribute");
   
   aVRayGeomInfo = nAttr.createAddr("vrayGeomInfo", "vgi", &stat);
   MCHECKERROR(stat, "Could not create 'vrayGeomInfo' attribute");;
   nAttr.setKeyable(false);
   nAttr.setStorable(false);
   nAttr.setReadable(true);
   nAttr.setWritable(true);
   nAttr.setHidden(true);
   stat = addAttribute(aVRayGeomInfo);
   MCHECKERROR(stat, "Could not add 'vrayGeomInfo' attribute");;
   
   aVRayGeomResult = nAttr.create("vrayGeomResult", "vgr", MFnNumericData::kInt, 0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayGeomResult' attribute");
   nAttr.setKeyable(false);
   nAttr.setStorable(false);
   nAttr.setReadable(true);
   nAttr.setWritable(false);
   nAttr.setHidden(true);
   stat = addAttribute(aVRayGeomResult);
   MCHECKERROR(stat, "Could not add 'vrayGeomResult' attribute");
   
   aVRayAbcVerbose = nAttr.create("vrayAbcVerbose", "vaverb", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcVerbose' attribute");
   nAttr.setKeyable(false);
   stat = addAttribute(aVRayAbcVerbose);
   MCHECKERROR(stat, "Could not add 'vrayAbcVerbose' attribute");
   
   MFnStringData refFileDef;
   MObject refFileDefObj = refFileDef.create("");
   aVRayAbcReferenceFilename = tAttr.create("vrayAbcReferenceFilename", "vareff", MFnData::kString, refFileDefObj, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcReferenceFilename' attribute");
   tAttr.setKeyable(false);
   tAttr.setUsedAsFilename(true);
   stat = addAttribute(aVRayAbcReferenceFilename);
   MCHECKERROR(stat, "Could not add 'vrayAbcReferenceFilename' attribute");
   
   aVRayAbcParticleType = eAttr.create("vrayAbcParticleType", "vapart", 6, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcParticleType' attribute");
   eAttr.addField("multipoints", 3);
   eAttr.addField("multistreaks", 4);
   eAttr.addField("points", 6);
   eAttr.addField("spheres", 7);
   eAttr.addField("sprites", 8);
   eAttr.addField("streaks", 9);
   eAttr.setKeyable(false);
   stat = addAttribute(aVRayAbcParticleType);
   MCHECKERROR(stat, "Could not add 'vrayAbcParticleType' attribute");
   
   MFnStringData partAttrDef;
   MObject partAttrDefObj = partAttrDef.create("");
   aVRayAbcParticleAttribs = tAttr.create("vrayAbcParticleAttribs", "vapara", MFnData::kString, partAttrDefObj, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcParticleAttribs' attribute");
   tAttr.setKeyable(false);
   stat = addAttribute(aVRayAbcParticleAttribs);
   MCHECKERROR(stat, "Could not add 'vrayAbcParticleAttribs' attribute");
   
   aVRayAbcRadius = nAttr.create("vrayAbcRadius", "varad", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcRadius' attribute");
   stat = addAttribute(aVRayAbcRadius);
   MCHECKERROR(stat, "Could not add 'vrayAbcRadius' attribute");
   
   aVRayAbcPointSize = nAttr.create("vrayAbcPointSize", "vaptsz", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcPointSize' attribute");
   stat = addAttribute(aVRayAbcPointSize);
   MCHECKERROR(stat, "Could not add 'vrayAbcPointSize' attribute");
   
   // aVRayAbcPointRadii = nAttr.create("vrayAbcPointRadii", "vaptra", MFnNumericData::kBoolean, 0, &stat);
   // MCHECKERROR(stat, "Could not create 'vrayAbcPointRadii' attribute");
   // stat = addAttribute(aVRayAbcPointRadii);
   // MCHECKERROR(stat, "Could not add 'vrayAbcPointRadii' attribute");
   
   // aVRayAbcPointWorldSize = nAttr.create("vrayAbcPointWorldSize", "vaptws", MFnNumericData::kBoolean, 0, &stat);
   // MCHECKERROR(stat, "Could not create 'vrayAbcPointWorldSize' attribute");
   // stat = addAttribute(aVRayAbcPointWorldSize);
   // MCHECKERROR(stat, "Could not add 'vrayAbcPointWorldSize' attribute");
   
   aVRayAbcSpriteSizeX = nAttr.create("vrayAbcSpriteSizeX", "vaspsx", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcSpriteSizeX' attribute");
   stat = addAttribute(aVRayAbcSpriteSizeX);
   MCHECKERROR(stat, "Could not add 'vrayAbcSpriteSizeX' attribute");
   
   aVRayAbcSpriteSizeY = nAttr.create("vrayAbcSpriteSizeY", "vaspsy", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcSpriteSizeY' attribute");
   stat = addAttribute(aVRayAbcSpriteSizeY);
   MCHECKERROR(stat, "Could not add 'vrayAbcSpriteSizeY' attribute");
   
   aVRayAbcSpriteTwist = nAttr.create("vrayAbcSpriteTwist", "vasptw", MFnNumericData::kFloat, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcSpriteTwist' attribute");
   stat = addAttribute(aVRayAbcSpriteTwist);
   MCHECKERROR(stat, "Could not add 'vrayAbcSpriteTwist' attribute");
   
   // aVRayAbcSpriteOrientation = eAttr.create("vrayAbcSpriteOrientation", "vaspor", 0, &stat);
   // MCHECKERROR(stat, "Could not create 'vrayAbcSpriteOrientation' attribute");
   // eAttr.addField("Towards camera center", 0);
   // eAttr.addField("Parallel to camera's image plane", 1);
   // eAttr.setKeyable(false)
   // stat = addAttribute(aVRayAbcSpriteOrientation);
   // MCHECKERROR(stat, "Could not add 'vrayAbcSpriteOrientation' attribute");
   
   aVRayAbcMultiCount = nAttr.create("vrayAbcMultiCount", "vamuco", MFnNumericData::kInt, 1, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcMultiCount' attribute");
   stat = addAttribute(aVRayAbcMultiCount);
   MCHECKERROR(stat, "Could not add 'vrayAbcMultiCount' attribute");
   
   aVRayAbcMultiRadius = nAttr.create("vrayAbcMultiRadius", "vamura", MFnNumericData::kFloat, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcMultiRadius' attribute");
   stat = addAttribute(aVRayAbcMultiRadius);
   MCHECKERROR(stat, "Could not add 'vrayAbcMultiRadius' attribute");
   
   aVRayAbcLineWidth = nAttr.create("vrayAbcLineWidth", "valnwi", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcLineWidth' attribute");
   stat = addAttribute(aVRayAbcLineWidth);
   MCHECKERROR(stat, "Could not add 'vrayAbcLineWidth' attribute");
   
   aVRayAbcTailLength = nAttr.create("vrayAbcTailLength", "vatlle", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcTailLength' attribute");
   stat = addAttribute(aVRayAbcTailLength);
   MCHECKERROR(stat, "Could not add 'vrayAbcTailLength' attribute");
   
   aVRayAbcSortIDs = nAttr.create("vrayAbcSortIDs", "vasids", MFnNumericData::kBoolean, 0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcSortIDs' attribute");
   nAttr.setKeyable(false);
   stat = addAttribute(aVRayAbcSortIDs);
   MCHECKERROR(stat, "Could not add 'vrayAbcSortIDs' attribute");
   
   aVRayAbcPsizeScale = nAttr.create("vrayAbcPsizeScale", "vapss", MFnNumericData::kFloat, 1.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcPsizeScale' attribute");
   stat = addAttribute(aVRayAbcPsizeScale);
   MCHECKERROR(stat, "Could not add 'vrayAbcPsizeScale' attribute");
   
   aVRayAbcPsizeMin = nAttr.create("vrayAbcPsizeMin", "vapmi", MFnNumericData::kFloat, 0.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcPsizeMin' attribute");
   stat = addAttribute(aVRayAbcPsizeMin);
   MCHECKERROR(stat, "Could not add 'vrayAbcPsizeMin' attribute");
   
   aVRayAbcPsizeMax = nAttr.create("vrayAbcPsizeMax", "vapma", MFnNumericData::kFloat, 1000000.0, &stat);
   MCHECKERROR(stat, "Could not create 'vrayAbcPsizeMax' attribute");
   stat = addAttribute(aVRayAbcPsizeMax);
   MCHECKERROR(stat, "Could not add 'vrayAbcPsizeMax' attribute");
   
   attributeAffects(aVRayGeomInfo, aVRayGeomResult);
   attributeAffects(aFilePath, aVRayGeomResult);
   attributeAffects(aObjectExpression, aVRayGeomResult);
   attributeAffects(aStartFrame, aVRayGeomResult);
   attributeAffects(aEndFrame, aVRayGeomResult);
   attributeAffects(aSpeed, aVRayGeomResult);
   attributeAffects(aOffset, aVRayGeomResult);
   attributeAffects(aPreserveStartFrame, aVRayGeomResult);
   attributeAffects(aCycleType, aVRayGeomResult);
   
   attributeAffects(aVRayAbcVerbose, aVRayGeomResult);
   attributeAffects(aVRayAbcReferenceFilename, aVRayGeomResult);
   attributeAffects(aVRayAbcParticleType, aVRayGeomResult);
   attributeAffects(aVRayAbcParticleAttribs, aVRayGeomResult);
   attributeAffects(aVRayAbcPointSize, aVRayGeomResult);
   attributeAffects(aVRayAbcSpriteSizeX, aVRayGeomResult);
   attributeAffects(aVRayAbcSpriteSizeY, aVRayGeomResult);
   attributeAffects(aVRayAbcSpriteTwist, aVRayGeomResult);
   attributeAffects(aVRayAbcMultiCount, aVRayGeomResult);
   attributeAffects(aVRayAbcMultiRadius, aVRayGeomResult);
   attributeAffects(aVRayAbcLineWidth, aVRayGeomResult);
   attributeAffects(aVRayAbcTailLength, aVRayGeomResult);
   attributeAffects(aVRayAbcSortIDs, aVRayGeomResult);
   attributeAffects(aVRayAbcPsizeScale, aVRayGeomResult);
   attributeAffects(aVRayAbcPsizeMin, aVRayGeomResult);
   attributeAffects(aVRayAbcPsizeMax, aVRayGeomResult);
#endif

   attributeAffects(aFilePath, aNumShapes);
   attributeAffects(aObjectExpression, aNumShapes);
   attributeAffects(aIgnoreInstances, aNumShapes);
   attributeAffects(aIgnoreVisibility, aNumShapes);
   
   attributeAffects(aFilePath, aAnimated);
   attributeAffects(aObjectExpression, aAnimated);
   attributeAffects(aIgnoreInstances, aAnimated);
   attributeAffects(aIgnoreXforms, aAnimated);
   attributeAffects(aIgnoreVisibility, aAnimated);
   
   attributeAffects(aObjectExpression, aOutBoxMin);
   attributeAffects(aDisplayMode, aOutBoxMin);
   attributeAffects(aCycleType, aOutBoxMin);
   attributeAffects(aTime, aOutBoxMin);
   attributeAffects(aSpeed, aOutBoxMin);
   attributeAffects(aOffset, aOutBoxMin);
   attributeAffects(aPreserveStartFrame, aOutBoxMin);
   attributeAffects(aStartFrame, aOutBoxMin);
   attributeAffects(aEndFrame, aOutBoxMin);
   attributeAffects(aIgnoreXforms, aOutBoxMin);
   attributeAffects(aIgnoreInstances, aOutBoxMin);
   attributeAffects(aIgnoreVisibility, aOutBoxMin);
   
   attributeAffects(aFilePath, aOutBoxMax);
   attributeAffects(aObjectExpression, aOutBoxMax);
   attributeAffects(aDisplayMode, aOutBoxMax);
   attributeAffects(aCycleType, aOutBoxMax);
   attributeAffects(aTime, aOutBoxMax);
   attributeAffects(aSpeed, aOutBoxMax);
   attributeAffects(aOffset, aOutBoxMax);
   attributeAffects(aPreserveStartFrame, aOutBoxMax);
   attributeAffects(aStartFrame, aOutBoxMax);
   attributeAffects(aEndFrame, aOutBoxMax);
   attributeAffects(aIgnoreXforms, aOutBoxMax);
   attributeAffects(aIgnoreInstances, aOutBoxMax);
   attributeAffects(aIgnoreVisibility, aOutBoxMax);
   
   return MS::kSuccess;
}

void AbcShape::AssignDefaultShader(MObject &obj)
{
   static MObject sDefaultShader = MObject::kNullObj;
   
   if (sDefaultShader.isNull())
   {
      MSelectionList sl;
      
      sl.add("initialShadingGroup");
      
      sl.getDependNode(0, sDefaultShader);
   }
   
   if (!obj.isNull() && !sDefaultShader.isNull())
   {
      MFnDependencyNode fn(sDefaultShader);
      
      MPlug dst = fn.findPlug("dagSetMembers");
      
      fn.setObject(obj);
      
      MPlug src = fn.findPlug("instObjGroups");
      
      MDGModifier dgMod;
      MIntArray indices;
      
      dst.getExistingArrayAttributeIndices(indices);
      
      unsigned int dstIdx = (indices.length() > 0 ? indices[indices.length()-1] + 1 : 0);
      
      dgMod.connect(src.elementByLogicalIndex(0), dst.elementByLogicalIndex(dstIdx));
      dgMod.doIt();
   }
}

AbcShape::AbcShape()
   : MPxSurfaceShape()
   , mOffset(0.0)
   , mSpeed(1.0)
   , mCycleType(CT_hold)
   , mStartFrame(0.0)
   , mEndFrame(0.0)
   , mSampleTime(0.0)
   , mIgnoreInstances(false)
   , mIgnoreTransforms(false)
   , mIgnoreVisibility(false)
   , mScene(0)
   , mDisplayMode(DM_box)
   , mNumShapes(0)
   , mPointWidth(0.0f)
   , mLineWidth(0.0f)
   , mPreserveStartFrame(false)
   , mDrawTransformBounds(false)
   , mDrawLocators(false)
   , mUpdateLevel(AbcShape::UL_none)
#ifdef ABCSHAPE_VRAY_SUPPORT
   , mVRFilename("filename", "")
   , mVRReferenceFilename("referenceFilename", "")
   , mVRObjectPath("objectPath", "/")
   , mVRIgnoreTransforms("ignoreTransforms", false)
   , mVRIgnoreInstances("ignoreInstances", false)
   , mVRIgnoreVisibility("ignoreVisibility", false)
   , mVRIgnoreTransformBlur("ignoreTransformBlur", false)
   , mVRIgnoreDeformBlur("ignoreDeformBlur", false)
   , mVRPreserveStartFrame("preserveStartFrame", false)
   , mVRSpeed("speed", 1.0f)
   , mVROffset("offset", 0.0f)
   , mVRStartFrame("startFrame", 0.0f)
   , mVREndFrame("endFrame", 0.0f)
   , mVRFps("fps", 24.0f)
   , mVRCycle("cycle", 0)
   , mVRVerbose("verbose", false)
   // Subdivision attributes
   , mVRSubdivEnable("subdiv_enable", false)
   , mVRSubdivUVs("subdiv_uvs", true)
   , mVRPreserveMapBorders("preserve_map_borders", -1)
   , mVRStaticSubdiv("static_subdiv", false)
   , mVRClassicCatmark("classic_catmark", false)
   // Subdivision Quality settings
   , mVRUseGlobals("use_globals", true)
   , mVRViewDep("view_dep", true)
   , mVREdgeLength("edge_length", 4.0f)
   , mVRMaxSubdivs("max_subdivs", 4)
   // OpenSubdiv attributes
   , mVROSDSubdivEnable("osd_subdiv_enable", false)
   , mVROSDSubdivLevel("osd_subdiv_level", 0)
   , mVROSDSubdivType("osd_subdiv_type", 0)
   , mVROSDSubdivUVs("osd_subdiv_uvs", true)
   , mVROSDPreserveMapBorders("osd_preserve_map_borders", 1)
   , mVROSDPreserveGeometryBorders("osd_preserve_geometry_borders", false)
   // Displacement attributes
   , mVRDisplacementType("displacement_type", 0)
   , mVRDisplacementAmount("displacement_amount", 1.0f)
   , mVRDisplacementShift("displacement_shift", 0.0f)
   , mVRKeepContinuity("keep_continuity", false)
   , mVRWaterLevel("water_level", -1e+30f)
   , mVRVectorDisplacement("vector_displacement", 0)
   , mVRMapChannel("map_channel", 0)
   , mVRUseBounds("use_bounds", false)
   , mVRMinBound("min_bound", VR::Color(0.0f, 0.0f, 0.0f))
   , mVRMaxBound("max_bound", VR::Color(1.0f, 1.0f, 1.0f))
   , mVRImageWidth("image_width", 0)
   , mVRCacheNormals("cache_normals", false)
   , mVRObjectSpaceDisplacement("object_space_displacement", false)
   , mVRStaticDisplacement("static_displacement", false)
   , mVRDisplace2d("displace_2d", false)
   , mVRResolution("resolution", 256)
   , mVRPrecision("precision", 8)
   , mVRTightBounds("tight_bounds", false)
   , mVRFilterTexture("filter_texture", false)
   , mVRFilterBlur("filter_blur", 0.001)
   // Particle attributes
   , mVRParticleType("particle_type", 6)
   , mVRParticleAttribs("particle_attribs", "")
   , mVRSpriteSizeX("sprite_size_x", 1.0f)
   , mVRSpriteSizeY("sprite_size_y", 1.0f)
   , mVRSpriteTwist("sprite_twist", 0.0f)
   , mVRSpriteOrientation("sprite_orientation", 0)
   , mVRRadius("radius", 1.0f)
   , mVRPointSize("point_size", 1.0f)
   , mVRPointRadii("point_radii", 0)
   , mVRPointWorldSize("point_world_size", 0)
   , mVRMultiCount("multi_count", 1)
   , mVRMultiRadius("multi_radius", 0.0f)
   , mVRLineWidth("line_width", 1.0f)
   , mVRTailLength("tail_length", 1.0f)
   , mVRSortIDs("sort_ids", 0)
   , mVRPsizeScale("psize_scale", 1.0f)
   , mVRPsizeMin("psize_min", 0.0f)
   , mVRPsizeMax("psize_max", 1e+30f)
#endif
{
}

AbcShape::~AbcShape()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Destructor called" << std::endl;
   #endif
   
   if (mScene && !AlembicSceneCache::Unref(mScene))
   {
      #ifdef _DEBUG
      std::cout << "[" << PREFIX_NAME("AbcShape") << "] Force delete scene" << std::endl;
      #endif
      
      delete mScene;
   }
}

void AbcShape::postConstructor()
{
   setRenderable(true);
   
   MObject self = thisMObject();
   MFnDependencyNode fn(self);
   
   aUvSet = fn.attribute("uvSet");
   aUvSetName = fn.attribute("uvSetName");
   
#ifdef ABCSHAPE_VRAY_SUPPORT
   static MString _preMel(NAME_PREFIX "AbcShapeVRayDisp -init");
   static MString _postPython("import " NAME_PREFIX "abcshape4vray; " NAME_PREFIX "abcshape4vray.CreateDispTextures()");
      
   MSelectionList sl;
   MObject drg, vs;
   
   int drgIdx = -1;
   int vsIdx = -1;
   
   if (sl.add("defaultRenderGlobals") == MS::kSuccess)
   {
      drgIdx = int(sl.length()) - 1;
   }
   
   if (sl.add("vraySettings") == MS::kSuccess)
   {
      vsIdx = int(sl.length()) - 1;
   }
   
   if (drgIdx >= 0 && sl.getDependNode(drgIdx, drg) == MS::kSuccess)
   {
      MFnDependencyNode nodeFn(drg);
      
      MPlug preMel = nodeFn.findPlug("preMel");
      
      if (!preMel.isNull())
      {
         MString s = preMel.asString();
         
         if (s.indexW(_preMel) == -1)
         {
            if (s.length() > 0)
            {
               if (s.asChar()[s.length()-1] != ';')
               {
                  s += "; " + _preMel;
               }
               else
               {
                  s += " " + _preMel + ";";
               }
            }
            else
            {
               s = _preMel;
            }
            
            preMel.setString(s);
         }
      }
   }
   
   if (vsIdx >= 0 && sl.getDependNode(vsIdx, vs) == MS::kSuccess)
   {
      MFnDependencyNode nodeFn(vs);
      
      MPlug postPython = nodeFn.findPlug("postTranslatePython");
      
      if (!postPython.isNull())
      {
         MString s = postPython.asString();
         
         if (s.indexW(_postPython) == -1)
         {
            if (s.length() > 0)
            {
               s += "\n" + _postPython;
            }
            else
            {
               s = _postPython;
            }
            
            postPython.setString(s);
         }
      }
   }
#endif
}

bool AbcShape::ignoreCulling() const
{
   MFnDependencyNode node(thisMObject());
   MPlug plug = node.findPlug("ignoreCulling");
   if (!plug.isNull())
   {
      return plug.asBool();
   }
   else
   {
      return false;
   }
}

MStatus AbcShape::compute(const MPlug &plug, MDataBlock &block)
{
   if (plug.attribute() == aOutBoxMin || plug.attribute() == aOutBoxMax)
   {
      syncInternals(block);
      
      Alembic::Abc::V3d out(0, 0, 0);
      
      if (mScene)
      {
         if (plug.attribute() == aOutBoxMin)
         {
            out = mScene->selfBounds().min;
         }
         else
         {
            out = mScene->selfBounds().max;
         }
      }
      
      MDataHandle hOut = block.outputValue(plug.attribute());
      
      hOut.set3Double(out.x, out.y, out.z);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aNumShapes)
   {
      syncInternals(block);
      
      MDataHandle hOut = block.outputValue(plug.attribute());
      
      hOut.setInt(mNumShapes);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aAnimated)
   {
      syncInternals(block);
      
      MDataHandle hOut = block.outputValue(plug.attribute());
      
      hOut.setBool(mAnimated);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
#ifdef ABCSHAPE_VRAY_SUPPORT
   else if (plug.attribute() == aVRayGeomResult)
   {
      syncInternals(block);
      
      MDataHandle hIn = block.inputValue(aVRayGeomInfo);
      MDataHandle hOut = block.outputValue(plug.attribute());
      
      void *ptr = hIn.asAddr();
      
      VR::VRayGeomInfo *geomInfo = reinterpret_cast<VR::VRayGeomInfo*>(ptr);
      
      if (geomInfo)
      {
         PluginManager *plugman = geomInfo->getPluginManager();
         
         if (plugman)
         {
            PluginDesc *plugdesc = plugman->getPluginDesc("AlembicLoader");
            
            if (plugdesc)
            {
               bool existing = false;
               
               MObject self = thisMObject();
               MFnDependencyNode thisNode(self);
               
               MDagPath thisPath;
               MDagPath::getAPathTo(self, thisPath);
               
               #ifdef _DEBUG
               std::cout << "Export to V-Ray: \"" << thisNode.name().asChar() << "\" - \"" << thisPath.fullPathName().asChar() << "\"" << std::endl;
               #endif
               
               VR::VRayPlugin *abc = geomInfo->newPlugin("AlembicLoader", existing);
               
               if (abc && !existing)
               {
                  mVRFilename.setString(mFilePath.asChar(), 0, 0.0);
                  mVRObjectPath.setString(mObjectExpression.asChar(), 0, 0.0);
                  mVRSpeed.setFloat(float(mSpeed), 0, 0.0);
                  mVROffset.setFloat(float(mOffset), 0, 0.0);
                  mVRStartFrame.setFloat(float(mStartFrame), 0, 0.0);
                  mVREndFrame.setFloat(float(mEndFrame), 0, 0.0);
                  mVRIgnoreTransforms.setBool(mIgnoreTransforms, 0, 0.0);
                  mVRIgnoreInstances.setBool(mIgnoreInstances, 0, 0.0);
                  mVRIgnoreVisibility.setBool(mIgnoreVisibility, 0, 0.0);
                  mVRPreserveStartFrame.setBool(mPreserveStartFrame, 0, 0.0);
                  mVRCycle.setInt(int(mCycleType), 0, 0.0);
                  mVRFps.setFloat(float(getFPS()), 0, 0.0);
                  
                  MPlug pMotionBlur = thisNode.findPlug("motionBlur");
                  if (!pMotionBlur.isNull())
                  {
                     bool singleShape = (mNumShapes == 1 && mIgnoreTransforms);
                     bool motionBlur = pMotionBlur.asBool();
                     
                     mVRIgnoreTransformBlur.setBool((singleShape || !motionBlur), 0, 0.0);
                     mVRIgnoreDeformBlur.setBool(!motionBlur, 0, 0.0);
                  }
                  else
                  {
                     mVRIgnoreTransformBlur.setBool(false, 0, 0.0);
                     mVRIgnoreDeformBlur.setBool(false, 0, 0.0);
                  }
                  
                  // MPlug pRefFile = thisNode.findPlug("referenceFilename");
                  // if (!pRefFile.isNull())
                  // {
                  //    mVRReferenceFilename.setString(pRefFile.asString().asChar(), 0, 0.0);
                  // }
                  // else
                  // {
                  //    mVRReferenceFilename.setString("", 0, 0.0);
                  // }
                  MDataHandle hReferenceFilename = block.inputValue(aVRayAbcReferenceFilename);
                  mVRReferenceFilename.setString(hReferenceFilename.asString().asChar(), 0, 0.0);
                  
                  // MPlug pVerbose = thisNode.findPlug("verbose");
                  // if (!pVerbose.isNull())
                  // {
                  //    mVRVerbose.setBool(pVerbose.asBool(), 0, 0.0);
                  // }
                  // else
                  // {
                  //    mVRVerbose.setBool(false, 0, 0.0);
                  // }
                  MDataHandle hVerbose = block.inputValue(aVRayAbcVerbose);
                  mVRVerbose.setBool(hVerbose.asBool(), 0, 0.0);
                  
                  // Subdivision / Displacement
                  
                  MObject subdivAttrObject = MObject::kNullObj;
                  MObject dispAttrObject = MObject::kNullObj;
                  MObject osdAttrObject = MObject::kNullObj;
                  MObject subdqAttrObject = MObject::kNullObj;
                  
                  bool displaced = true;
                  bool subdivided = false;
                  bool subdquality = false;
                  bool osd = false;
                  
                  MPlug pDispNone = thisNode.findPlug("vrayDisplacementNone");
                  if (!pDispNone.isNull())
                  {
                     if (pDispNone.asBool())
                     {
                        displaced = false;
                     }
                     else
                     {
                        dispAttrObject = thisMObject();
                     }
                  }
                  
                  MPlug pSubdivEnable = thisNode.findPlug("vraySubdivEnable");
                  if (!pSubdivEnable.isNull() && pSubdivEnable.asBool())
                  {
                     subdivAttrObject = thisMObject();
                     subdivided = true;
                  }
                  
                  MPlug pOSDEnable = thisNode.findPlug("vrayOsdSubdivEnable");
                  if (!pOSDEnable.isNull() && pOSDEnable.asBool())
                  {
                     osdAttrObject = thisMObject();
                     osd = true;
                  }
                  
                  MPlug pOverrideGlobals = thisNode.findPlug("vrayOverrideGlobalSubQual");
                  if (!pOverrideGlobals.isNull() && pOverrideGlobals.asBool())
                  {
                     subdquality = true;
                     subdqAttrObject = thisMObject();
                  }
                  
                  std::string dispSetName, dispShaderName;                  
                  bool hasDispAssigned = AbcShapeVRayDisp::getAssignedDisplacement(thisPath, dispSetName, dispShaderName);
                  
                  if (hasDispAssigned)
                  {
                     MSelectionList sl;
                     
                     if (sl.add(dispSetName.c_str()) == MS::kSuccess)
                     {
                        MObject dispSetObj;
                        
                        sl.getDependNode(0, dispSetObj);
                        
                        MFnDependencyNode dispSetFn(dispSetObj);
                        
                        if (displaced && dispShaderName.length() > 0)
                        {
                           MPlug pDispNone = dispSetFn.findPlug("vrayDisplacementNone");
                           if (!pDispNone.isNull())
                           {
                              if (pDispNone.asBool())
                              {
                                 displaced = false;
                              }
                              else
                              {
                                 if (dispAttrObject == MObject::kNullObj)
                                 {
                                    dispAttrObject = dispSetObj;
                                 }
                              }
                           }
                        }
                        
                        MPlug pSubdivEnable = dispSetFn.findPlug("vraySubdivEnable");
                        if (!pSubdivEnable.isNull())
                        {
                           if (pSubdivEnable.asBool())
                           {
                              subdivided = true;
                              if (subdivAttrObject == MObject::kNullObj)
                              {
                                 subdivAttrObject = dispSetObj;
                              }
                           }
                        }
                        
                        MPlug pOSDEnable = dispSetFn.findPlug("vrayOsdSubdivEnable");
                        if (!pOSDEnable.isNull())
                        {
                           if (pOSDEnable.asBool())
                           {
                              osd = true;
                              if (osdAttrObject == MObject::kNullObj)
                              {
                                 osdAttrObject = dispSetObj;
                              }
                           }
                        }
                        
                        MPlug pOverrideGlobals = dispSetFn.findPlug("vrayOverrideGlobalSubQual");
                        if (!pOverrideGlobals.isNull())
                        {
                           if (pOverrideGlobals.asBool())
                           {
                              subdquality = true;
                              if (subdqAttrObject == MObject::kNullObj)
                              {
                                 subdqAttrObject = dispSetObj;
                              }
                           }
                        }
                     }
                  }
                  else
                  {
                     // No displacement shader assigned
                     displaced = false;
                  }
                  
                  mVRSubdivEnable.setBool(subdivided, 0, 0.0);
                  abc->setParameter(&mVRSubdivEnable);
                  
                  if (subdivided && subdivAttrObject != MObject::kNullObj)
                  {
                     // Subdivision
                     //
                     // vraySubdivEnable: bool, 1
                     // vrayPreserveMapBorders: enum, 1, (None:0, Internal:1, All:2)
                     // vraySubdivUVs: bool, 1
                     // vrayStaticSubdiv: bool, 0
                     // vrayClassicalCatmark: bool, 0
                     
                     MFnDependencyNode nodeFn(subdivAttrObject);
                     
                     MPlug pAttr = nodeFn.findPlug("vraySubdivUVs");
                     mVRSubdivUVs.setInt(pAttr.asInt(), 0, 0.0);
                     abc->setParameter(&mVRSubdivUVs);
                     
                     if (pAttr.asBool())
                     {
                        pAttr = nodeFn.findPlug("vrayPreserveMapBorders");
                        mVRPreserveMapBorders.setInt(pAttr.asInt(), 0, 0.0);
                        abc->setParameter(&mVRPreserveMapBorders);
                     }
                     
                     pAttr = nodeFn.findPlug("vrayStaticSubdiv");
                     mVRStaticSubdiv.setInt(pAttr.asInt(), 0, 0.0);
                     abc->setParameter(&mVRStaticSubdiv);
                     
                     pAttr = nodeFn.findPlug("vrayClassicalCatmark");
                     mVRClassicCatmark.setInt(pAttr.asInt(), 0, 0.0);
                     abc->setParameter(&mVRClassicCatmark);
                  }
                  
                  mVRUseGlobals.setBool((subdquality ? false : true), 0, 0.0);
                  abc->setParameter(&mVRUseGlobals);
                  
                  if (subdquality && subdqAttrObject != MObject::kNullObj)
                  {
                     // Subdivision quality attributes
                     //
                     // vrayOverrideGlobalSubQual: bool, true
                     // vrayViewDep: bool, true
                     // vrayEdgeLength: float, 4.0
                     // vrayMaxSubdivs: int, 4
                     
                     MFnDependencyNode nodeFn(subdqAttrObject);
                                          
                     MPlug pAttr = nodeFn.findPlug("vrayViewDep");
                     mVRViewDep.setBool(pAttr.asBool(), 0, 0.0);
                     abc->setParameter(&mVRViewDep);
                     
                     pAttr = nodeFn.findPlug("vrayEdgeLength");
                     mVREdgeLength.setFloat(pAttr.asFloat(), 0, 0.0);
                     abc->setParameter(&mVREdgeLength);
                     
                     pAttr = nodeFn.findPlug("vrayMaxSubdivs");
                     mVRMaxSubdivs.setInt(pAttr.asInt(), 0, 0.0);
                     abc->setParameter(&mVRMaxSubdivs);
                  }
                  
                  mVROSDSubdivEnable.setBool(osd, 0, 0.0);
                  abc->setParameter(&mVROSDSubdivEnable);
                  
                  if (osd && osdAttrObject != MObject::kNullObj)
                  {
                     // OpenSubdiv attributes
                     //
                     // vrayOsdSubdivEnable: bool, 1
                     // vrayOsdSubdivDepth: int, 4, 0-8
                     // vrayOsdSubdivType: enum, 0, (Catmull-Clark:0, Loop:1)
                     // vrayOsdPreserveMapBorders: enum, 1, (None:0, Internal:1, All:2)
                     // vrayOsdSubdivUVs: bool, 1
                     // vrayOsdPreserveGeomBorders: bool, 0
                     
                     MFnDependencyNode nodeFn(osdAttrObject);
                     
                     MPlug pAttr = nodeFn.findPlug("vrayOsdSubdivDepth");
                     mVROSDSubdivLevel.setInt(pAttr.asInt(), 0, 0.0);
                     abc->setParameter(&mVROSDSubdivLevel);
                     
                     pAttr = nodeFn.findPlug("vrayOsdSubdivType");
                     mVROSDSubdivType.setInt(pAttr.asInt(), 0, 0.0);
                     abc->setParameter(&mVROSDSubdivType);
                     
                     pAttr = nodeFn.findPlug("vrayOsdSubdivUVs");
                     mVROSDSubdivUVs.setBool(pAttr.asBool(), 0, 0.0);
                     abc->setParameter(&mVROSDSubdivUVs);
                     
                     if (pAttr.asBool())
                     {
                        pAttr = nodeFn.findPlug("vrayOsdPreserveMapBorders");
                        mVROSDPreserveMapBorders.setInt(pAttr.asInt(), 0, 0.0);
                        abc->setParameter(&mVROSDPreserveMapBorders);
                     }
                     
                     pAttr = nodeFn.findPlug("vrayOsdPreserveGeomBorders");
                     mVROSDPreserveGeometryBorders.setBool(pAttr.asBool(), 0, 0.0);
                     abc->setParameter(&mVROSDPreserveGeometryBorders);
                  }
                  
                  mVRDisplacementType.setInt((displaced ? 1 : 0), 0, 0.0);
                  abc->setParameter(&mVRDisplacementType);
                  
                  if (displaced)
                  {
                     bool colorDisp = false;
                     
                     if (dispAttrObject != MObject::kNullObj)
                     {
                        // Displacement attributes
                        // 
                        // vrayDisplacementNone: bool, 1
                        // vrayDisplacementStatic: bool, 0
                        // vrayDisplacementType: enum, 1, (2D Displacement:0, Normal Displacement:1, Vector displacement:2, Vector displacement (absolute):3, Vector displacement (object):4)
                        // vrayDisplacementAmount: float, 1, 0-10
                        // vrayDisplacementShift: float, 0, 0-10
                        // vrayDisplacementKeepContinuity: bool, 0
                        // vrayEnableWaterLevel: bool, 0
                        // vrayWaterLevel: float, 0, 0-10
                        // vrayDisplacementCacheNormals: bool, off
                        // vray2dDisplacementResolution: int, 256, 64-1024
                        // vray2dDisplacementPrecision: int, 8, 1-32
                        // vray2dDisplacementTightBounds: bool, false
                        // vray2dDisplacementFilterTexture: bool, off
                        // vray2dDisplacementFilterBlur: float, 0.003, 0-10
                        // vrayDisplacementUseBounds: enum, 0, (Automatic:0, Explicit:1)
                        // vrayDisplacementMinValue: float3, 0,0,0 (color)
                        // vrayDisplacementMaxValue: float3, 1,1,1 (color)
                        
                        MFnDependencyNode nodeFn(dispAttrObject);
                        
                        bool displace2d = false;
                        
                        MPlug pAttr = nodeFn.findPlug("vrayDisplacementType");
                        if (pAttr.asInt() >= 2)
                        {
                           colorDisp = true;
                           
                           mVRVectorDisplacement.setInt(pAttr.asInt() - 1, 0, 0.0);
                           abc->setParameter(&mVRVectorDisplacement);
                           
                           // if (pAttr.asInt() == 4)
                           // {
                           //    mVRObjectSpaceDisplacement .setBool(true, 0, 0.0);
                           //    abc->setParameter(&mVRObjectSpaceDisplacement);
                           // }
                        }
                        else
                        {
                           displace2d = (pAttr.asInt() == 0);
                        }
                        
                        pAttr = nodeFn.findPlug("vrayDisplacementStatic");
                        mVRStaticDisplacement.setBool(pAttr.asBool(), 0, 0.0);
                        abc->setParameter(&mVRStaticDisplacement);
                        
                        pAttr = nodeFn.findPlug("vrayDisplacementAmount");
                        mVRDisplacementAmount.setFloat(pAttr.asFloat(), 0, 0.0);
                        abc->setParameter(&mVRDisplacementAmount);
                        
                        pAttr = nodeFn.findPlug("vrayDisplacementShift");
                        mVRDisplacementShift.setFloat(pAttr.asFloat(), 0, 0.0);
                        abc->setParameter(&mVRDisplacementShift);
                        
                        pAttr = nodeFn.findPlug("vrayDisplacementKeepContinuity");
                        mVRKeepContinuity.setBool(pAttr.asBool(), 0, 0.0);
                        abc->setParameter(&mVRKeepContinuity);
                        
                        mVRDisplace2d.setBool(displace2d, 0, 0.0);
                        abc->setParameter(&mVRDisplace2d);
                        
                        pAttr = nodeFn.findPlug("vrayEnableWaterLevel");
                        if (pAttr.asBool())
                        {
                           pAttr = nodeFn.findPlug("vrayWaterLevel");
                           mVRWaterLevel.setFloat(pAttr.asFloat(), 0, 0.0);
                           abc->setParameter(&mVRWaterLevel);
                        }
                        
                        pAttr = nodeFn.findPlug("vrayDisplacementCacheNormals");
                        mVRCacheNormals.setBool(pAttr.asBool(), 0, 0.0);
                        abc->setParameter(&mVRCacheNormals);
                        
                        pAttr = nodeFn.findPlug("vrayDisplacementUseBounds");
                        mVRUseBounds.setBool(pAttr.asInt() == 1, 0, 0.0);
                        abc->setParameter(&mVRUseBounds);
                        
                        if (pAttr.asInt() == 1)
                        {
                           pAttr = nodeFn.findPlug("vrayDisplacementMinValue");
                           mVRMinBound.setColor(VR::Color(pAttr.child(0).asFloat(),
                                                          pAttr.child(1).asFloat(),
                                                          pAttr.child(2).asFloat()), 0, 0.0);
                           abc->setParameter(&mVRMinBound);
                           
                           pAttr = nodeFn.findPlug("vrayDisplacementMaxValue");
                           mVRMaxBound.setColor(VR::Color(pAttr.child(0).asFloat(),
                                                          pAttr.child(1).asFloat(),
                                                          pAttr.child(2).asFloat()), 0, 0.0);
                           abc->setParameter(&mVRMaxBound);
                        }
                        
                        if (displace2d)
                        {
                           pAttr = nodeFn.findPlug("vray2dDisplacementResolution");
                           mVRResolution.setInt(pAttr.asInt(), 0, 0.0);
                           abc->setParameter(&mVRResolution);
                           
                           pAttr = nodeFn.findPlug("vray2dDisplacementPrecision");
                           mVRPrecision.setInt(pAttr.asInt(), 0, 0.0);
                           abc->setParameter(&mVRPrecision);
                           
                           pAttr = nodeFn.findPlug("vray2dDisplacementTightBounds");
                           mVRTightBounds.setBool(pAttr.asBool(), 0, 0.0);
                           abc->setParameter(&mVRTightBounds);
                           
                           pAttr = nodeFn.findPlug("vray2dDisplacementFilterTexture");
                           mVRFilterTexture.setBool(pAttr.asBool(), 0, 0.0);
                           abc->setParameter(&mVRFilterTexture);
                           
                           if (pAttr.asBool())
                           {
                              pAttr = nodeFn.findPlug("vray2dDisplacementFilterBlur");
                              mVRFilterBlur.setFloat(pAttr.asFloat(), 0, 0.0);
                              abc->setParameter(&mVRFilterBlur);
                           }
                        }
                     }
                     
                     // Track shader to be exported
                     AbcShapeVRayDisp::DispShapes &dispShapes = AbcShapeVRayDisp::DispTexs[dispShaderName];
                        
                     if (colorDisp)
                     {
                        dispShapes.asColor.insert(abc->getPluginName());
                     }
                     else
                     {
                        dispShapes.asFloat.insert(abc->getPluginName());
                     }
                  }
                  
                  // particle attributes
                  MDataHandle hParticleType = block.inputValue(aVRayAbcParticleType);
                  mVRParticleType.setInt(hParticleType.asInt(), 0, 0.0);
                  
                  MDataHandle hParticleAttribs = block.inputValue(aVRayAbcParticleAttribs);
                  mVRParticleAttribs.setString(hParticleAttribs.asString().asChar(), 0, 0.0);
                  
                  MDataHandle hRadius = block.inputValue(aVRayAbcRadius);
                  mVRRadius.setFloat(hRadius.asFloat(), 0, 0.0);
                  
                  MPlug pPointRadii = thisNode.findPlug("vrayPointSizeRadius");
                  bool point_radii = (!pPointRadii.isNull() && pPointRadii.asBool());
                  mVRPointRadii.setBool(point_radii, 0, 0.0);
                  
                  MDataHandle hPointSize = block.inputValue(aVRayAbcPointSize);
                  mVRPointSize.setFloat((point_radii ? hRadius.asFloat() : hPointSize.asFloat()), 0, 0.0);
                  
                  MPlug pPointSizePP = thisNode.findPlug("vrayPointSizePP");
                  if (!pPointSizePP.isNull() && pPointSizePP.asBool())
                  {
                     // forces 'point_radii' to true
                     mVRPointRadii.setBool(1, 0, 0.0);
                  }
                  
                  MPlug pPointSizeWorld = thisNode.findPlug("vrayPointSizeWorld");
                  mVRPointWorldSize.setBool((!pPointSizeWorld.isNull() && pPointSizeWorld.asBool()), 0, 0.0);
                  
                  MDataHandle hSpriteSizeX = block.inputValue(aVRayAbcSpriteSizeX);
                  mVRSpriteSizeX.setFloat(hSpriteSizeX.asFloat(), 0, 0.0);
                  
                  MDataHandle hSpriteSizeY = block.inputValue(aVRayAbcSpriteSizeY);
                  mVRSpriteSizeX.setFloat(hSpriteSizeY.asFloat(), 0, 0.0);
                  
                  MDataHandle hSpriteTwist = block.inputValue(aVRayAbcSpriteTwist);
                  mVRSpriteTwist.setFloat(hSpriteTwist.asFloat(), 0, 0.0);
                  
                  MPlug pSpriteOrient = thisNode.findPlug("vraySpriteOrient");
                  mVRSpriteOrientation.setInt((pSpriteOrient.isNull() ? 0 : pSpriteOrient.asInt()), 0, 0.0);
                  
                  MDataHandle hMultiCount = block.inputValue(aVRayAbcMultiCount);
                  mVRMultiCount.setInt(hMultiCount.asInt(), 0, 0.0);
                  
                  MDataHandle hMultiRadius = block.inputValue(aVRayAbcMultiRadius);
                  mVRMultiRadius.setFloat(hMultiRadius.asFloat(), 0, 0.0);
                  
                  MDataHandle hLineWidth = block.inputValue(aVRayAbcLineWidth);
                  mVRLineWidth.setFloat(hLineWidth.asFloat(), 0, 0.0);
                  
                  MDataHandle hTailLength = block.inputValue(aVRayAbcTailLength);
                  mVRTailLength.setFloat(hTailLength.asFloat(), 0, 0.0);
                  
                  MDataHandle hSortIDs = block.inputValue(aVRayAbcSortIDs);
                  mVRSortIDs.setBool(hSortIDs.asBool(), 0, 0.0);
                  
                  MDataHandle hPsizeScale = block.inputValue(aVRayAbcPsizeScale);
                  mVRPsizeScale.setFloat(hPsizeScale.asFloat(), 0, 0.0);
                  
                  MDataHandle hPsizeMin = block.inputValue(aVRayAbcPsizeMin);
                  mVRPsizeMin.setFloat(hPsizeMin.asFloat(), 0, 0.0);
                  
                  MDataHandle hPsizeMax = block.inputValue(aVRayAbcPsizeMax);
                  mVRPsizeMax.setFloat(hPsizeMax.asFloat(), 0, 0.0);
                  
                  abc->setParameter(&mVRFilename);
                  abc->setParameter(&mVRObjectPath);
                  abc->setParameter(&mVRSpeed);
                  abc->setParameter(&mVROffset);
                  abc->setParameter(&mVRStartFrame);
                  abc->setParameter(&mVREndFrame);
                  abc->setParameter(&mVRIgnoreTransforms);
                  abc->setParameter(&mVRIgnoreInstances);
                  abc->setParameter(&mVRIgnoreVisibility);
                  abc->setParameter(&mVRPreserveStartFrame);
                  abc->setParameter(&mVRCycle);
                  abc->setParameter(&mVRFps);
                  abc->setParameter(&mVRIgnoreTransformBlur);
                  abc->setParameter(&mVRIgnoreDeformBlur);
                  abc->setParameter(&mVRReferenceFilename);
                  abc->setParameter(&mVRVerbose);
                  abc->setParameter(&mVRParticleType);
                  abc->setParameter(&mVRParticleAttribs);
                  abc->setParameter(&mVRRadius);
                  abc->setParameter(&mVRPointSize);
                  abc->setParameter(&mVRPointRadii);
                  abc->setParameter(&mVRPointWorldSize);
                  abc->setParameter(&mVRSpriteSizeX);
                  abc->setParameter(&mVRSpriteSizeY);
                  abc->setParameter(&mVRSpriteTwist);
                  abc->setParameter(&mVRSpriteOrientation);
                  abc->setParameter(&mVRMultiCount);
                  abc->setParameter(&mVRMultiRadius);
                  abc->setParameter(&mVRLineWidth);
                  abc->setParameter(&mVRTailLength);
                  abc->setParameter(&mVRSortIDs);
                  abc->setParameter(&mVRPsizeScale);
                  abc->setParameter(&mVRPsizeMin);
                  abc->setParameter(&mVRPsizeMax);
                  
                  hOut.setInt(1);
               }
               else
               {
                  hOut.setInt(0);
               }
            }
            else
            {
               hOut.setInt(0);
            }
         }
         else
         {
            hOut.setInt(0);
         }
      }
      else
      {
         hOut.setInt(0);
      }
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
#endif
   else
   {
      return MS::kUnknownParameter;
   }
}

bool AbcShape::isBounded() const
{
   return true;
}

void AbcShape::syncInternals()
{
   MDataBlock block = forceCache();
   syncInternals(block);
}

void AbcShape::syncInternals(MDataBlock &block)
{
   block.inputValue(aFilePath).asString();
   block.inputValue(aObjectExpression).asString();
   block.inputValue(aTime).asTime();
   block.inputValue(aSpeed).asDouble();
   block.inputValue(aOffset).asDouble();
   block.inputValue(aStartFrame).asDouble();
   block.inputValue(aEndFrame).asDouble();
   block.inputValue(aCycleType).asShort();
   block.inputValue(aIgnoreXforms).asBool();
   block.inputValue(aIgnoreInstances).asBool();
   block.inputValue(aIgnoreVisibility).asBool();
   block.inputValue(aDisplayMode).asShort();
   block.inputValue(aPreserveStartFrame).asBool();
   
   switch (mUpdateLevel)
   {
   case UL_objects:
      updateObjects();
      break;
   case UL_range:
      if (mScene) updateRange();
      break;
   case UL_world:
      if (mScene) updateWorld();
      break;
   case UL_geometry:
      if (mScene) updateGeometry();
      break;
   default:
      break;
   }
   
   mUpdateLevel = UL_none;
}

MBoundingBox AbcShape::boundingBox() const
{
   MBoundingBox bbox;
   
   AbcShape *this2 = const_cast<AbcShape*>(this);
      
   this2->syncInternals();
   
   if (mScene)
   {
      // Use self bounds here as those are taking ignore transform/instance flag into account
      Alembic::Abc::Box3d bounds = mScene->selfBounds();
      
      if (!bounds.isEmpty() && !bounds.isInfinite())
      {
         bbox.expand(MPoint(bounds.min.x, bounds.min.y, bounds.min.z));
         bbox.expand(MPoint(bounds.max.x, bounds.max.y, bounds.max.z));
      }
   }
   
   return bbox;
}

double AbcShape::getFPS() const
{
   float fps = 24.0f;
   MTime::Unit unit = MTime::uiUnit();
   
   if (unit!=MTime::kInvalid)
   {
      MTime time(1.0, MTime::kSeconds);
      fps = static_cast<float>(time.as(unit));
   }

   if (fps <= 0.f )
   {
      fps = 24.0f;
   }

   return fps;
}

double AbcShape::computeAdjustedTime(const double inputTime, const double speed, const double timeOffset) const
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Adjust time: " << (inputTime * getFPS()) << " -> " << ((inputTime - timeOffset) * speed * getFPS()) << std::endl;
   #endif
   
   return (inputTime - timeOffset) * speed;
}

double AbcShape::computeRetime(const double inputTime,
                               const double firstTime,
                               const double lastTime,
                               AbcShape::CycleType cycleType) const
{
   const double playTime = lastTime - firstTime;
   static const double eps = 0.001;
   double retime = inputTime;

   switch (cycleType)
   {
   case CT_loop:
      if (inputTime < (firstTime - eps) || inputTime > (lastTime + eps))
      {
         const double timeOffset = inputTime - firstTime;
         const double playOffset = floor(timeOffset/playTime);
         const double fraction = fabs(timeOffset/playTime - playOffset);

         retime = firstTime + playTime * fraction;
      }
      break;
   case CT_reverse:
      if (inputTime > (firstTime + eps) && inputTime < (lastTime - eps))
      {
         const double timeOffset = inputTime - firstTime;
         const double playOffset = floor(timeOffset/playTime);
         const double fraction = fabs(timeOffset/playTime - playOffset);

         retime = lastTime - playTime * fraction;
      }
      else if (inputTime < (firstTime + eps))
      {
         retime = lastTime;
      }
      else
      {
         retime = firstTime;
      }
      break;
   case CT_bounce:
      if (inputTime < (firstTime - eps) || inputTime > (lastTime + eps))
      {
         const double timeOffset = inputTime - firstTime;
         const double playOffset = floor(timeOffset/playTime);
         const double fraction = fabs(timeOffset/playTime - playOffset);

         // forward loop
         if (int(playOffset) % 2 == 0)
         {
            retime = firstTime + playTime * fraction;
         }
         // backward loop
         else
         {
            retime = lastTime - playTime * fraction;
         }
      }
      break;
   case CT_hold:
   default:
      if (inputTime < (firstTime - eps))
      {
         retime = firstTime;
      }
      else if (inputTime > (lastTime + eps))
      {
         retime = lastTime;
      }
      break;
   }

   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Retime: " << (inputTime * getFPS()) << " -> " << (retime * getFPS()) << std::endl;
   #endif
   
   return retime;
}

double AbcShape::getSampleTime() const
{
   double invFPS = 1.0 / getFPS();
   double startOffset = 0.0f;
   if (mPreserveStartFrame && fabs(mSpeed) > 0.0001)
   {
      startOffset = (mStartFrame * (mSpeed - 1.0) / mSpeed);
   }
   double sampleTime = computeAdjustedTime(mTime.as(MTime::kSeconds), mSpeed, (startOffset + mOffset) * invFPS);
   return computeRetime(sampleTime, mStartFrame * invFPS, mEndFrame * invFPS, mCycleType);
}

void AbcShape::updateObjects()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update objects: \"" << mFilePath.asChar() << "\" | \"" << mObjectExpression.asChar() << "\"" << std::endl;
   #endif
   
   mGeometry.clear();
   mAnimated = false;
   mNumShapes = 0;
   
   mSceneFilter.set(mObjectExpression.asChar(), "");
   
   AlembicScene *scene = AlembicSceneCache::Ref(mFilePath.asChar(), mSceneFilter);
   
   if (mScene && !AlembicSceneCache::Unref(mScene))
   {
      delete mScene;
   }
   
   mScene = scene;
   
   if (mScene)
   {
      updateRange();
   }
}

void AbcShape::updateRange()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update range" << std::endl;
   #endif
   
   mAnimated = false;
   
   GetFrameRange visitor(mIgnoreTransforms, mIgnoreInstances, mIgnoreVisibility);
   mScene->visit(AlembicNode::VisitDepthFirst, visitor);
   
   double start, end;
   
   if (visitor.getFrameRange(start, end))
   {
      double fps = getFPS();
      start *= fps;
      end *= fps;
      
      mAnimated = (fabs(end - start) > 0.0001);
      
      if (fabs(mStartFrame - start) > 0.0001 ||
          fabs(mEndFrame - end) > 0.0001)
      {
         #ifdef _DEBUG
         std::cout << "[" << PREFIX_NAME("AbcShape") << "] Frame range: " << start << " - " << end << std::endl;
         #endif
         
         mStartFrame = start;
         mEndFrame = end;
         mSampleTime = getSampleTime();
         
         // This will force instant refresh of AE values
         // but won't trigger any update as mStartFrame and mEndFrame are unchanged
         MPlug plug(thisMObject(), aStartFrame);
         plug.setDouble(mStartFrame);
         
         plug.setAttribute(aEndFrame);
         plug.setDouble(mEndFrame);
      }
   }
   
   updateWorld();
}

void AbcShape::updateWorld()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update world" << std::endl;
   #endif
   
   WorldUpdate visitor(mSampleTime, mIgnoreTransforms, mIgnoreInstances, mIgnoreVisibility);
   mScene->visit(AlembicNode::VisitDepthFirst, visitor);
   
   // only get number of visible shapes
   mNumShapes = visitor.numShapes(true);
   
   MObject self = thisMObject();
   
   // Reset existing UV set names
   MPlug pUvSet(self, aUvSet);
   
   for (unsigned int i=0; i<pUvSet.numElements(); ++i)
   {
      pUvSet.elementByPhysicalIndex(i).child(aUvSetName).setString("");
   }
   
   if (visitor.numShapes(false) == 1)
   {
      mUvSetNames = visitor.uvSetNames();
      
      MPlug pUvSetName(self, aUvSetName);
      
      for (size_t i=0; i<mUvSetNames.size(); ++i)
      {
         pUvSetName.selectAncestorLogicalIndex(i, aUvSet);
         pUvSetName.setString(mUvSetNames[i].c_str());
      }
   }
   else
   {
      mUvSetNames.clear();
   }
   
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] " << mNumShapes << " shape(s) in scene" << std::endl;
   #endif
   
   mScene->updateChildBounds();
   mScene->setSelfBounds(visitor.bounds());
   
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Scene bounds: " << visitor.bounds().min << " - " << visitor.bounds().max << std::endl;
   #endif
   
   if (mDisplayMode >= DM_points)
   {
      updateGeometry();
   }
}

void AbcShape::updateGeometry()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update geometry" << std::endl;
   #endif
   
   SampleGeometry visitor(mSampleTime, &mGeometry);
   mScene->visit(AlembicNode::VisitDepthFirst, visitor);
}

void AbcShape::printInfo(bool detailed) const
{
   if (detailed)
   {
      PrintInfo pinf(mIgnoreTransforms, mIgnoreInstances, mIgnoreVisibility);
      mScene->visit(AlembicNode::VisitDepthFirst, pinf);
   }
   
   printSceneBounds();
}

void AbcShape::printSceneBounds() const
{
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Scene " << mScene->selfBounds().min << " - " << mScene->selfBounds().max << std::endl;
}

bool AbcShape::getInternalValueInContext(const MPlug &plug, MDataHandle &handle, MDGContext &ctx)
{
   if (plug == aFilePath)
   {
      handle.setString(mFilePath);
      return true;
   }
   else if (plug == aObjectExpression)
   {
      handle.setString(mObjectExpression);
      return true;
   }
   else if (plug == aTime)
   {
      handle.setMTime(mTime);
      return true;
   }
   else if (plug == aOffset)
   {
      handle.setDouble(mOffset);
      return true;
   }
   else if (plug == aSpeed)
   {
      handle.setDouble(mSpeed);
      return true;
   }
   else if (plug == aPreserveStartFrame)
   {
      handle.setBool(mPreserveStartFrame);
      return true;
   }
   else if (plug == aCycleType)
   {
      handle.setInt(mCycleType);
      return true;
   }
   else if (plug == aStartFrame)
   {
      handle.setDouble(mStartFrame);
      return true;
   }
   else if (plug == aEndFrame)
   {
      handle.setDouble(mEndFrame);
      return true;
   }
   else if (plug == aIgnoreXforms)
   {
      handle.setBool(mIgnoreTransforms);
      return true;
   }
   else if (plug == aIgnoreInstances)
   {
      handle.setBool(mIgnoreInstances);
      return true;
   }
   else if (plug == aIgnoreVisibility)
   {
      handle.setBool(mIgnoreVisibility);
      return true;
   }
   else if (plug == aDisplayMode)
   {
      handle.setInt(mDisplayMode);
      return true;
   }
   else if (plug == aLineWidth)
   {
      handle.setFloat(mLineWidth);
      return true;
   }
   else if (plug == aPointWidth)
   {
      handle.setFloat(mPointWidth);
      return true;
   }
   else if (plug == aDrawTransformBounds)
   {
      handle.setBool(mDrawTransformBounds);
      return true;
   }
   else if (plug == aDrawLocators)
   {
      handle.setBool(mDrawLocators);
      return true;
   }
   else
   {
      return MPxNode::getInternalValueInContext(plug, handle, ctx);
   }
}

bool AbcShape::setInternalValueInContext(const MPlug &plug, const MDataHandle &handle, MDGContext &ctx)
{
   bool sampleTimeUpdate = false;
   
   if (plug == aFilePath)
   {
      // Note: path seems to already be directory mapped when we reach here (consequence of setUsedAsFilename(true) when the attribute is created)
      //       don't need to use MFileObject to get the resolved path
      MString filePath = handle.asString();
      
      if (filePath != mFilePath)
      {
         mFilePath = filePath;
         mUpdateLevel = UL_objects;
      }
      
      return true;
   }
   else if (plug == aObjectExpression)
   {
      MString objectExpression = handle.asString();
      
      if (objectExpression != mObjectExpression)
      {
         mObjectExpression = objectExpression;
         mUpdateLevel = UL_objects;
      }
      
      return true;
   }
   else if (plug == aIgnoreXforms)
   {
      bool ignoreTransforms = handle.asBool();
      
      if (ignoreTransforms != mIgnoreTransforms)
      {
         mIgnoreTransforms = ignoreTransforms;
         if (mScene)
         {
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_range);
         }
      }
      
      return true;
   }
   else if (plug == aIgnoreInstances)
   {
      bool ignoreInstances = handle.asBool();
      
      if (ignoreInstances != mIgnoreInstances)
      {
         mIgnoreInstances = ignoreInstances;
         if (mScene)
         {
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_range);
         }
      }
      
      return true;
   }
   else if (plug == aIgnoreVisibility)
   {
      bool ignoreVisibility = handle.asBool();
      
      if (ignoreVisibility != mIgnoreVisibility)
      {
         mIgnoreVisibility = ignoreVisibility;
         if (mScene)
         {
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_range);
         }
      }
      
      return true;
   }
   else if (plug == aDisplayMode)
   {
      DisplayMode dm = (DisplayMode) handle.asShort();
      
      if (dm != mDisplayMode)
      {
         bool updateGeo = (mDisplayMode <= DM_boxes && dm >= DM_points);
         
         mDisplayMode = dm;
         
         if (updateGeo && mScene)
         {
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_geometry);
         }
      }
      
      return true;
   }
   else if (plug == aTime)
   {
      MTime t = handle.asTime();
      
      if (fabs(t.as(MTime::kSeconds) - mTime.as(MTime::kSeconds)) > 0.0001)
      {
         mTime = t;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aSpeed)
   {
      double speed = handle.asDouble();
      
      if (fabs(speed - mSpeed) > 0.0001)
      {
         mSpeed = speed;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aPreserveStartFrame)
   {
      bool psf = handle.asBool();
      
      if (psf != mPreserveStartFrame)
      {
         mPreserveStartFrame = psf;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aOffset)
   {
      double offset = handle.asDouble();
      
      if (fabs(offset - mOffset) > 0.0001)
      {
         mOffset = offset;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aCycleType)
   {
      CycleType c = (CycleType) handle.asShort();
      
      if (c != mCycleType)
      {
         mCycleType = c;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aStartFrame)
   {
      double sf = handle.asDouble();
      
      if (fabs(sf - mStartFrame) > 0.0001)
      {
         mStartFrame = sf;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aEndFrame)
   {
      double ef = handle.asDouble();
      
      if (fabs(ef - mEndFrame) > 0.0001)
      {
         mEndFrame = ef;
         sampleTimeUpdate = mAnimated;
      }
   }
   else if (plug == aLineWidth)
   {
      mLineWidth = handle.asFloat();
      return true;
   }
   else if (plug == aPointWidth)
   {
      mPointWidth = handle.asFloat();
      return true;
   }
   else if (plug == aDrawTransformBounds)
   {
      mDrawTransformBounds = handle.asBool();
      return true;
   }
   else if (plug == aDrawLocators)
   {
      mDrawLocators = handle.asBool();
      return true;
   }
   else
   {
      return MPxNode::setInternalValueInContext(plug, handle, ctx);
   }
   
   if (sampleTimeUpdate)
   {
      double sampleTime = getSampleTime();
      
      if (fabs(mSampleTime - sampleTime) > 0.0001)
      {
         mSampleTime = sampleTime;
         
         if (mScene)
         {
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_world);
         }
      }
   }
   
   return true;
}

void AbcShape::copyInternalData(MPxNode *source)
{
   if (source && source->typeId() == ID)
   {
      AbcShape *node = (AbcShape*)source;
      
      mFilePath = node->mFilePath;
      mObjectExpression = node->mObjectExpression;
      mTime = node->mTime;
      mOffset = node->mOffset;
      mSpeed = node->mSpeed;
      mCycleType = node->mCycleType;
      mStartFrame = node->mStartFrame;
      mEndFrame = node->mEndFrame;
      mSampleTime = node->mSampleTime;
      mIgnoreInstances = node->mIgnoreInstances;
      mIgnoreTransforms = node->mIgnoreTransforms;
      mIgnoreVisibility = node->mIgnoreVisibility;
      mLineWidth = node->mLineWidth;
      mPointWidth = node->mPointWidth;
      mDisplayMode = node->mDisplayMode;
      mPreserveStartFrame = node->mPreserveStartFrame;
      mDrawTransformBounds = node->mDrawTransformBounds;
      mDrawLocators = node->mDrawLocators;
      mAnimated = node->mAnimated;
      
      if (mScene && !AlembicSceneCache::Unref(mScene))
      {
         delete mScene;
      }
      mScene = 0;
      mSceneFilter.reset();
      mNumShapes = 0;
      mGeometry.clear();
      mUpdateLevel = UL_objects;
   }
   
}

// ---

void* AbcShapeUI::creator()
{
   return new AbcShapeUI();
}

AbcShapeUI::AbcShapeUI()
   : MPxSurfaceShapeUI()
{
}

AbcShapeUI::~AbcShapeUI()
{
}

void AbcShapeUI::getDrawRequests(const MDrawInfo &info,
                                 bool /*objectAndActiveOnly*/,
                                 MDrawRequestQueue &queue)
{
   MDrawData data;
   getDrawData(0, data);
   
   M3dView::DisplayStyle appearance = info.displayStyle();

   M3dView::DisplayStatus displayStatus = info.displayStatus();
   
   MDagPath path = info.multiPath();
   
   M3dView view = info.view();
   
   MColor color;
   
   switch (displayStatus)
   {
   case M3dView::kLive:
      color = M3dView::liveColor();
      break;
   case M3dView::kHilite:
      color = M3dView::hiliteColor();
      break;
   case M3dView::kTemplate:
      color = M3dView::templateColor();
      break;
   case M3dView::kActiveTemplate:
      color = M3dView::activeTemplateColor();
      break;
   case M3dView::kLead:
      color = M3dView::leadColor();
      break;
   case M3dView::kActiveAffected:
      color = M3dView::activeAffectedColor();
      break;
   default:
      color = MHWRender::MGeometryUtilities::wireframeColor(path);
   }
   
   switch (appearance)
   {
   case M3dView::kBoundingBox:
      {
         MDrawRequest request = info.getPrototype(*this);
         
         request.setDrawData(data);
         request.setToken(kDrawBox);
         request.setColor(color);
         
         queue.add(request);
      }
      break;
   case M3dView::kPoints:
      {
         MDrawRequest request = info.getPrototype(*this);
         
         request.setDrawData(data);
         request.setToken(kDrawPoints);
         request.setColor(color);
         
         queue.add(request);
      }
      break;
   default:
      {
         bool drawWireframe = false;
         
         if (appearance == M3dView::kWireFrame ||
             view.wireframeOnShaded() ||
             displayStatus == M3dView::kActive ||
             displayStatus == M3dView::kHilite ||
             displayStatus == M3dView::kLead)
         {
            MDrawRequest request = info.getPrototype(*this);
            
            request.setDrawData(data);
            request.setToken(appearance != M3dView::kWireFrame ? kDrawGeometryAndWireframe : kDrawGeometry);
            request.setDisplayStyle(M3dView::kWireFrame);
            request.setColor(color);
            
            queue.add(request);
            
            drawWireframe = true;
         }
         
         if (appearance != M3dView::kWireFrame)
         {
            MDrawRequest request = info.getPrototype(*this);
            
            request.setDrawData(data);
            request.setToken(drawWireframe ? kDrawGeometryAndWireframe : kDrawGeometry);
            
            // Only set material info if necessary
            AbcShape *shape = (AbcShape*) surfaceShape();
            if (shape && shape->displayMode() == AbcShape::DM_geometry)
            {
               MMaterial material = (view.usingDefaultMaterial() ? MMaterial::defaultMaterial() : MPxSurfaceShapeUI::material(path));
               material.evaluateMaterial(view, path);
               //material.evaluateTexture(data);
               //bool isTransparent = false;
               //if (material.getHasTransparency(isTransparent) && isTransparent)
               //{
               //  request.setIsTransparent(true);
               //}
               request.setMaterial(material);
            }
            else
            {
               request.setColor(color);
            }
            
            queue.add(request);
         }
      }
   }
}

void AbcShapeUI::draw(const MDrawRequest &request, M3dView &view) const
{
   AbcShape *shape = (AbcShape*) surfaceShape();
   if (!shape)
   {
      return;
   }
   
   switch (request.token())
   {
   case kDrawBox:
      drawBox(shape, request, view);
      break;
   case kDrawPoints:
      drawPoints(shape, request, view);
      break;
   case kDrawGeometry:
   case kDrawGeometryAndWireframe:
   default:
      {
         switch (shape->displayMode())
         {
         case AbcShape::DM_box:
         case AbcShape::DM_boxes:
            drawBox(shape, request, view);
            break;
         case AbcShape::DM_points:
            drawPoints(shape, request, view);
            break;
         default:
            drawGeometry(shape, request, view);
         }
      }
      break;
   }
}

bool AbcShapeUI::computeFrustum(M3dView &view, Frustum &frustum) const
{
   MMatrix projMatrix, modelViewMatrix;
   
   view.projectionMatrix(projMatrix);
   view.modelViewMatrix(modelViewMatrix);
   
   MMatrix tmp = (modelViewMatrix * projMatrix).inverse();
   
   if (tmp.isSingular())
   {
      return false;
   }
   else
   {
      M44d projViewInv;
      
      tmp.get(projViewInv.x);
      
      frustum.setup(projViewInv);
      
      return true;
   }
}

bool AbcShapeUI::computeFrustum(Frustum &frustum) const
{
   // using GL matrix
   M44d projMatrix;
   M44d modelViewMatrix;
   
   glGetDoublev(GL_PROJECTION_MATRIX, &(projMatrix.x[0][0]));
   glGetDoublev(GL_MODELVIEW_MATRIX, &(modelViewMatrix.x[0][0]));
   
   M44d projViewInv = modelViewMatrix * projMatrix;
   
   try
   {
      projViewInv.invert(true);
      frustum.setup(projViewInv);
      return true;
   }
   catch (std::exception &)
   {
      return false;
   }
}

void AbcShapeUI::getWorldMatrix(M3dView &view, Alembic::Abc::M44d &worldMatrix) const
{
   MMatrix modelViewMatrix;
   
   view.modelViewMatrix(modelViewMatrix);
   modelViewMatrix.get(worldMatrix.x);
}

bool AbcShapeUI::select(MSelectInfo &selectInfo,
                        MSelectionList &selectionList,
                        MPointArray &worldSpaceSelectPts) const
{
   MSelectionMask mask(PREFIX_NAME("AbcShape"));
   if (!selectInfo.selectable(mask))
   {
      return false;
   }
   
   AbcShape *shape = (AbcShape*) surfaceShape();
   if (!shape)
   {
      return false;
   }
   
   M3dView::DisplayStyle style = selectInfo.displayStyle();
   
   DrawToken target = (style == M3dView::kBoundingBox ? kDrawBox : (style == M3dView::kPoints ? kDrawPoints : kDrawGeometry));
   
   M3dView view = selectInfo.view();
   
   Frustum frustum;
   
   // As we use same name for all shapes, without hierarchy, don't really need a big buffer
   GLuint *buffer = new GLuint[16];
   
   view.beginSelect(buffer, 16);
   view.pushName(0);
   view.loadName(1); // Use same name for all 
   
   glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_LINE_BIT | GL_POINT_BIT);
   glDisable(GL_LIGHTING);
   
   if (shape->displayMode() == AbcShape::DM_box)
   {
      if (target == kDrawPoints)
      {
         DrawBox(shape->scene()->selfBounds(), true, shape->pointWidth());
      }
      else
      {
         DrawBox(shape->scene()->selfBounds(), false, shape->lineWidth());
      }
   }
   else
   {
      DrawScene visitor(shape->sceneGeometry(),
                        shape->ignoreTransforms(),
                        shape->ignoreInstances(),
                        shape->ignoreVisibility());
      
      // Use matrices staight from OpenGL as those will include the picking matrix so
      //   that more geometry can get culled
      if (computeFrustum(frustum))
      {
         #ifdef _DEBUG
         #if 0
         // Let's see the difference
         M44d glProjMatrix;
         M44d glModelViewMatrix;
         MMatrix mayaProjMatrix;
         MMatrix mayaModelViewMatrix;
         
         glGetDoublev(GL_PROJECTION_MATRIX, &(glProjMatrix.x[0][0]));
         glGetDoublev(GL_MODELVIEW_MATRIX, &(glModelViewMatrix.x[0][0]));
         
         M44d glProjView = glModelViewMatrix * glProjMatrix;
         
         view.projectionMatrix(mayaProjMatrix);
         view.modelViewMatrix(mayaModelViewMatrix);
   
         MMatrix tmp = (mayaModelViewMatrix * mayaProjMatrix);
         M44d mayaProjView;
         tmp.get(mayaProjView.x);
         
         std::cout << "Proj/View from GL:" << std::endl;
         std::cout << glProjView << std::endl;
         
         std::cout << "Proj/View from Maya:" << std::endl;
         std::cout << mayaProjView << std::endl;
         #endif
         #endif
         
         visitor.doCull(frustum);
      }
      visitor.setLineWidth(shape->lineWidth());
      visitor.setPointWidth(shape->pointWidth());
      
      if (target == kDrawBox)
      {
         visitor.drawBounds(true);
      }
      else if (target == kDrawPoints)
      {
         visitor.drawBounds(shape->displayMode() == AbcShape::DM_boxes);
         visitor.drawAsPoints(true);
      }
      else
      {
         if (shape->displayMode() == AbcShape::DM_boxes)
         {
            visitor.drawBounds(true);
         }
         else if (shape->displayMode() == AbcShape::DM_points)
         {
            visitor.drawAsPoints(true);
         }
         else if (style == M3dView::kWireFrame)
         {
            visitor.drawWireframe(true);
         }
      }
      
      shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
   }
   
   glPopAttrib();
   
   view.popName();
   
   int hitCount = view.endSelect();
   
   if (hitCount > 0)
   {
      unsigned int izdepth = 0xFFFFFFFF;
      GLuint *curHit = buffer;
      
      for (int i=hitCount; i>=0; --i)
      {
         if (curHit[0] && izdepth > curHit[1])
         {
            izdepth = curHit[1];
         }
         curHit += curHit[0] + 3;
      }
      
      MDagPath path = selectInfo.multiPath();
      while (path.pop() == MStatus::kSuccess)
      {
         if (path.hasFn(MFn::kTransform))
         {
            break;
         }
      }
      
      MSelectionList selectionItem;
      selectionItem.add(path);
      
      MPoint worldSpacePoint;
      // compute hit point
      {
         float zdepth = float(izdepth) / 0xFFFFFFFF;
         
         MDagPath cameraPath;
         view.getCamera(cameraPath);
         
         MFnCamera camera(cameraPath);
         
         if (!camera.isOrtho())
         {
            // z is normalized but non linear
            double nearp = camera.nearClippingPlane();
            double farp = camera.farClippingPlane();
            
            zdepth *= (nearp / (farp - zdepth * (farp - nearp)));
         }
         
         MPoint O;
         MVector D;
         
         selectInfo.getLocalRay(O, D);
         O = O * selectInfo.multiPath().inclusiveMatrix();
         
         short x, y;
         view.worldToView(O, x, y);
         
         MPoint Pn, Pf;
         view.viewToWorld(x, y, Pn, Pf);
         
         worldSpacePoint = Pn + zdepth * (Pf - Pn);
      }
      
      selectInfo.addSelection(selectionItem,
                              worldSpacePoint,
                              selectionList,
                              worldSpaceSelectPts,
                              mask,
                              false);
      
      delete[] buffer;
      
      return true;
   }
   else
   {
      return false;
   }
}

void AbcShapeUI::drawBox(AbcShape *shape, const MDrawRequest &, M3dView &view) const
{
   view.beginGL();
   
   glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_LINE_BIT);
   
   glDisable(GL_LIGHTING);
   
   if (shape->displayMode() == AbcShape::DM_box)
   {
      DrawBox(shape->scene()->selfBounds(), false, shape->lineWidth());
   }
   else
   {
      Frustum frustum;
      
      DrawScene visitor(NULL, shape->ignoreTransforms(), shape->ignoreInstances(), shape->ignoreVisibility());
      visitor.drawAsPoints(false);
      visitor.drawLocators(shape->drawLocators());
      if (!shape->ignoreCulling() && computeFrustum(view, frustum))
      {
         visitor.doCull(frustum);
      }
      if (shape->drawTransformBounds())
      {
         Alembic::Abc::M44d worldMatrix;
         getWorldMatrix(view, worldMatrix);
         visitor.drawTransformBounds(true, worldMatrix);
      }
      
      shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
   }
   
   glPopAttrib();
   
   view.endGL();
}

void AbcShapeUI::drawPoints(AbcShape *shape, const MDrawRequest &, M3dView &view) const
{
   view.beginGL();
   
   glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POINT_BIT);
   
   glDisable(GL_LIGHTING);
   
   if (shape->displayMode() == AbcShape::DM_box)
   {
      DrawBox(shape->scene()->selfBounds(), true, shape->pointWidth());
   }
   else
   {
      Frustum frustum;
   
      DrawScene visitor(shape->sceneGeometry(), shape->ignoreTransforms(), shape->ignoreInstances(), shape->ignoreVisibility());
      
      visitor.drawBounds(shape->displayMode() == AbcShape::DM_boxes);
      visitor.drawAsPoints(true);
      visitor.setLineWidth(shape->lineWidth());
      visitor.setPointWidth(shape->pointWidth());
      visitor.drawLocators(shape->drawLocators());
      if (!shape->ignoreCulling() && computeFrustum(view, frustum))
      {
         visitor.doCull(frustum);
      }
      if (shape->drawTransformBounds())
      {
         Alembic::Abc::M44d worldMatrix;
         getWorldMatrix(view, worldMatrix);
         visitor.drawTransformBounds(true, worldMatrix);
      }
      
      shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
   }
      
   glPopAttrib();
      
   view.endGL();
}

void AbcShapeUI::drawGeometry(AbcShape *shape, const MDrawRequest &request, M3dView &view) const
{
   Frustum frustum;
   
   view.beginGL();
   
   bool wireframeOnShaded = (request.token() == kDrawGeometryAndWireframe);
   bool wireframe = (request.displayStyle() == M3dView::kWireFrame);
   //bool flat = (request.displayStyle() == M3dView::kFlatShaded);
   
   glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_LINE_BIT | GL_POLYGON_BIT);
   
   if (wireframe)
   {
      glDisable(GL_LIGHTING);
      //if over shaded: glDepthMask(false)?
   }
   else
   {
      glEnable(GL_POLYGON_OFFSET_FILL);
      
      //glShadeModel(flat ? GL_FLAT : GL_SMOOTH);
      // Note: as we only store 1 smooth normal per point in mesh, flat shaded will look strange: ignore it
      glShadeModel(GL_SMOOTH);
      
      //glEnable(GL_CULL_FACE);
      //glCullFace(GL_BACK);
      
      glEnable(GL_COLOR_MATERIAL);
      glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
      
      MMaterial material = request.material();
      material.setMaterial(request.multiPath(), request.isTransparent());
      
      //bool useTextures = (material.materialIsTextured() && !view.useDefaultMaterial());
      //if (useTextures)
      //{
      //  glEnable(GL_TEXTURE_2D);
      //  material.applyTexture(view, data);
      //  // ... and set mesh UVs
      //}
      
      MColor defaultDiffuse;
      material.getDiffuse(defaultDiffuse);
      glColor3f(defaultDiffuse.r, defaultDiffuse.g, defaultDiffuse.b);
   }
   
   DrawScene visitor(shape->sceneGeometry(), shape->ignoreTransforms(), shape->ignoreInstances(), shape->ignoreVisibility());
   visitor.drawWireframe(wireframe);
   visitor.setLineWidth(shape->lineWidth());
   visitor.setPointWidth(shape->pointWidth());
   if (!shape->ignoreCulling() && computeFrustum(view, frustum))
   {
      visitor.doCull(frustum);
   }
   
   // only draw transform bounds and locators once
   if (!wireframeOnShaded || wireframe)
   {
      visitor.drawLocators(shape->drawLocators());
      if (shape->drawTransformBounds())
      {
         Alembic::Abc::M44d worldMatrix;
         getWorldMatrix(view, worldMatrix);
         visitor.drawTransformBounds(true, worldMatrix);
      }
   }
   
   shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
   
   glPopAttrib();
   
   view.endGL();
}
