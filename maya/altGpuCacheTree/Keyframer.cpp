#include "Keyframer.h"
#include "common.h"
#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MFnTransform.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MAnimUtil.h>
#include <maya/MEulerRotation.h>
#include <maya/MVector.h>
#include <maya/MAngle.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MDGModifier.h>
#include <maya/MSelectionList.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MAnimCurveChange.h>
#include <cmath>

const char * Keyframer::msTransformAttrNames[] =
{
   "translateX",
   "translateY",
   "translateZ",
   "rotateX",
   "rotateY",
   "rotateZ",
   "scaleX",
   "scaleY",
   "scaleZ",
   "shearXY",
   "shearXZ",
   "shearYZ",
   NULL
};

MFnAnimCurve::AnimCurveType Keyframer::msTransformCurveTypes[] =
{
   MFnAnimCurve::kAnimCurveTL,
   MFnAnimCurve::kAnimCurveTL,
   MFnAnimCurve::kAnimCurveTL,
   MFnAnimCurve::kAnimCurveTA,
   MFnAnimCurve::kAnimCurveTA,
   MFnAnimCurve::kAnimCurveTA,
   MFnAnimCurve::kAnimCurveTU,
   MFnAnimCurve::kAnimCurveTU,
   MFnAnimCurve::kAnimCurveTU,
   MFnAnimCurve::kAnimCurveTU,
   MFnAnimCurve::kAnimCurveTU,
   MFnAnimCurve::kAnimCurveTU
};

// ---

Keyframer::Keyframer()
{
}

Keyframer::~Keyframer()
{
   mCurves.clear();
}

void Keyframer::setCurrentTime(double t, MTime::Unit unit)
{
   if (unit != mTime.unit())
   {
      mTime.setUnit(unit);
   }
   mTime.setValue(t);
}

Keyframer::CurveEntry& Keyframer::getOrCreateEntry(const MFnDependencyNode &dnode,
                                                   const MString &attrName,
                                                   MFnAnimCurve::AnimCurveType type)
{
   MString curveName = dnode.name() + "_" + attrName;
   
   CurveIterator it = mCurves.find(curveName);
   
   if (it == mCurves.end())
   {
      std::pair<MString, CurveEntry> entry;

      MPlug plug = dnode.findPlug(attrName, false);
      if (!plug.isKeyable())
      {
         plug.setKeyable(true);
      }

      entry.first = curveName;
      entry.second.obj = dnode.object();
      entry.second.attr = dnode.attribute(attrName);
      entry.second.type = type;
      entry.second.step = false;
      entry.second.hasinfo = false;
      entry.second.speed = 1.0;
      entry.second.offset = 0.0;
      entry.second.start = 0.0;
      entry.second.end = 0.0;
      entry.second.reverse = false;
      entry.second.preserveStart = false;

      std::pair<CurveIterator, bool> rv = mCurves.insert(entry);
      
      mNodeCurves[entry.second.obj].append(curveName);

      it = rv.first;
   }
   
   return it->second;
}

void Keyframer::clearAnyKey(const MObject &node,
                            const MString &attrName,
                            int index, double v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   MString realName = attrName;

   // if plug a compound, get indexth child
   MPlug plug = dnode.findPlug(attrName, false);

   if (plug.isCompound())
   {
      if (index < 0 || (unsigned int)index >= plug.numChildren())
      {
         return;
      }  

      realName = plug.child(index).partialName();
   }
   else if (index != 0)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, realName, MFnAnimCurve::kAnimCurveTU);

   entry.step = false;
   entry.times.clear();
   entry.values.clear();
   entry.values.append(v);
}

void Keyframer::addAnyKey(const MObject &node,
                          const MString &attrName,
                          int index, double v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   MString realName = attrName;

   // if plug a compound, get indexth child
   MPlug plug = dnode.findPlug(attrName, false);

   if (plug.isCompound())
   {
      if (index < 0 || (unsigned int)index >= plug.numChildren())
      {
         return;
      }  

      realName = plug.child(index).partialName();
   }
   else if (index != 0)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, realName, MFnAnimCurve::kAnimCurveTU);

   entry.step = false;
   if (entry.times.length() != entry.values.length())
   {
      entry.times.clear();
      entry.values.clear();
   }
   entry.times.append(mTime);
   entry.values.append(v);
}

void Keyframer::clearVisibilityKey(const MObject &node, bool v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, "visibility", MFnAnimCurve::kAnimCurveTU);

   entry.step = true;
   entry.times.clear();
   entry.values.clear();
   entry.values.append(v);
}

void Keyframer::addVisibilityKey(const MObject &node, bool v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, "visibility", MFnAnimCurve::kAnimCurveTU);

   entry.step = true;
   if (entry.times.length() != entry.values.length())
   {
      entry.times.clear();
      entry.values.clear();
   }
   entry.times.append(mTime);
   entry.values.append(v);
}

void Keyframer::clearInheritsTransformKey(const MObject &node, bool v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, "inheritsTransform", MFnAnimCurve::kAnimCurveTU);

   entry.step = true;
   entry.times.clear();
   entry.values.clear();
   entry.values.append(v);
}

void Keyframer::addInheritsTransformKey(const MObject &node, bool v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, "inheritsTransform", MFnAnimCurve::kAnimCurveTU);

   entry.step = true;
   if (entry.times.length() != entry.values.length())
   {
      entry.times.clear();
      entry.values.clear();
   }
   entry.times.append(mTime);
   entry.values.append(v);
}

void Keyframer::clearTransformKeys(const MObject &node, const MTransformationMatrix &tm)
{
   MStatus stat;

   MFnTransform dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   double v[12];
   MTransformationMatrix::RotationOrder ro;

   MVector t = tm.getTranslation(MSpace::kTransform);
   v[0] = t.x;
   v[1] = t.y;
   v[2] = t.z;

   tm.getRotation(v+3, ro);
   v[3] = MAngle(v[3]).asRadians();
   v[4] = MAngle(v[4]).asRadians();
   v[5] = MAngle(v[5]).asRadians();

   tm.getScale(v+6, MSpace::kTransform);

   tm.getShear(v+9, MSpace::kTransform);

   // add curve keys
   
   size_t i = 0;

   while (msTransformAttrNames[i] != 0)
   {
      CurveEntry &entry = getOrCreateEntry(dnode, msTransformAttrNames[i], msTransformCurveTypes[i]);

      entry.times.clear();
      entry.values.clear();
      entry.values.append(v[i]);

      ++i;
   }
}

void Keyframer::clearTransformKeys(const MObject &node, const MMatrix &m)
{
   MTransformationMatrix tm(m);
   clearTransformKeys(node, tm);
}

void Keyframer::addTransformKey(const MObject &node, const MTransformationMatrix &tm)
{
   MStatus stat;

   MFnTransform dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   double v[12];
   MTransformationMatrix::RotationOrder ro;

   MVector t = tm.getTranslation(MSpace::kTransform);
   v[0] = t.x;
   v[1] = t.y;
   v[2] = t.z;

   tm.getRotation(v+3, ro);
   v[3] = MAngle(v[3]).asRadians();
   v[4] = MAngle(v[4]).asRadians();
   v[5] = MAngle(v[5]).asRadians();

   tm.getScale(v+6, MSpace::kTransform);

   tm.getShear(v+9, MSpace::kTransform);

   // add curve keys
   
   size_t i = 0;

   while (msTransformAttrNames[i] != 0)
   {
      CurveEntry &entry = getOrCreateEntry(dnode, msTransformAttrNames[i], msTransformCurveTypes[i]);

      if (entry.times.length() != entry.values.length())
      {
         entry.times.clear();
         entry.values.clear();
      }
      entry.times.append(mTime);
      entry.values.append(v[i]);

      ++i;
   }
}

void Keyframer::addTransformKey(const MObject &node, const MMatrix &m)
{
   MTransformationMatrix tm(m);
   addTransformKey(node, tm);
}

void Keyframer::cleanupCurve(CurveEntry &e, std::vector<unsigned int> &removeKeys, double threshold)
{
   unsigned int nk = e.values.length();

   if (e.times.length() != nk)
   {
      return;
   }

   // clear indices
   removeKeys.clear();
   if (nk > removeKeys.capacity())
   {
      removeKeys.reserve(nk);
   }

   unsigned int prevIdx = (unsigned int)-1;
   double prevVal = 0.0;
   double val;

   for (unsigned int i=0; i<nk; ++i)
   {
      val = e.values[i];
      if (prevIdx == (unsigned int)-1 || fabs(prevVal - val) > threshold)
      {
         // To avoid unsigned int overflow change the test (i - 1) - prevIdx > 1 to i - prevIdx > 2 
         if (prevIdx != (unsigned int)-1 && (i - prevIdx > 2))
         {
            for (unsigned int j=prevIdx+1; j<i-1; ++j)
            {
               removeKeys.push_back(j);
            }
         }
         prevIdx = i;
         prevVal = val;
      }
   }

   // To avoid unsigned int overflow change the test (nk - 1) - prevIdx > 1 to nk > 2 + prevIdx 
   if (prevIdx != (unsigned int)-1 && (nk > 2 + prevIdx))
   {
      // NOTES:
      // - if nk is <= 2, we never get here => nk-2 won't overflow
      // - prevIdx is always >= 0 => --j won't overflow
      for (unsigned int j=nk-2; j>prevIdx; --j)
      {
         e.times.remove(j);
         e.values.remove(j);
      }
   }

   while (removeKeys.size() > 0)
   {
      unsigned int i = removeKeys.back();
      e.times.remove(i);
      e.values.remove(i);
      removeKeys.pop_back();
   }
}

void Keyframer::setRotationCurvesInterpolation(const MString &type, const MStringDict &nodeInterpType)
{
   MStatus stat;
   std::map<MString, MSelectionList, MStringLessThan> rotCurvesByInterpType;
   
   CurveIterator it = mCurves.begin();

   while (it != mCurves.end())
   {
      CurveEntry &entry = it->second;

      MFnDependencyNode node(entry.obj);
      MPlug plug = node.findPlug(entry.attr, false);
      
      MFnAnimCurve curve(plug, &stat);

      if (stat == MStatus::kSuccess && curve.animCurveType() == MFnAnimCurve::kAnimCurveTA)
      {
         MString interpType = type;
         
         // Check for node interpolation type override
         MString nodeName = node.name();
         int r = nodeName.rindexW(':');
         if (r != -1)
         {
            nodeName = nodeName.substringW(r + 1, nodeName.numChars() - 1);
         }
         
         MStringDict::const_iterator nit = nodeInterpType.find(nodeName);
         if (nit != nodeInterpType.end())
         {
            interpType = nit->second;
         }
         
         rotCurvesByInterpType[interpType].add(curve.name());
      }

      ++it;
   }
   
   if (rotCurvesByInterpType.size() > 0)
   {
      MSelectionList oldSel;
      
      MGlobal::getActiveSelectionList(oldSel);
      
      for (std::map<MString, MSelectionList, MStringLessThan>::iterator slit = rotCurvesByInterpType.begin();
           slit != rotCurvesByInterpType.end();
           ++slit)
      {
         MSelectionList &rotCurves = slit->second;
         
         if (rotCurves.length() > 0)
         {
            MGlobal::setActiveSelectionList(rotCurves);
            
            MString other = ((slit->first == "none" || slit->first == "euler") ? "quaternion" : "euler");
            MString cmd = "";
            
            cmd += "{ string $crv, $it = \"\";\n";
            cmd += "  for ($crv in `ls -sl`)\n";
            cmd += "  {\n";
            cmd += "    $it = `rotationInterpolation -q $crv`;\n";
            cmd += "    if ($it == \"" + slit->first + "\")\n";
            cmd += "    {\n";
            cmd += "       rotationInterpolation -c " + other + " $crv;\n";
            cmd += "    }\n";
            cmd += "    rotationInterpolation -c " + slit->first + " $crv;\n";
            cmd += "  } }";
            
            MGlobal::executeCommand(cmd);
            //MGlobal::executeCommand("rotationInterpolation -c " + slit->first);
         }
      }
      
      MGlobal::setActiveSelectionList(oldSel);
   }
}

void Keyframer::fixCurvesTangents()
{
   MStatus stat;

   CurveIterator it = mCurves.begin();

   while (it != mCurves.end())
   {
      CurveEntry &entry = it->second;

      MFnDependencyNode node(entry.obj);
      MPlug plug = node.findPlug(entry.attr, false);

      MFnAnimCurve curve(plug, &stat);

      if (stat == MStatus::kSuccess)
      {
         MFnAnimCurve::TangentType tt = (entry.step ? MFnAnimCurve::kTangentStep : MFnAnimCurve::kTangentLinear);

         unsigned int nk = curve.numKeys();
         for (unsigned int i=0; i<nk; ++i)
         {
            curve.setInTangentType(i, tt);
            curve.setOutTangentType(i, tt);
         }
      }

      ++it;
   }
}

void Keyframer::fixCurvesTangents(MFnAnimCurve::AnimCurveType type)
{
   MStatus stat;

   CurveIterator it = mCurves.begin();

   while (it != mCurves.end())
   {
      CurveEntry &entry = it->second;

      MFnDependencyNode node(entry.obj);
      MPlug plug = node.findPlug(entry.attr, false);

      MFnAnimCurve curve(plug, &stat);

      if (stat == MStatus::kSuccess && curve.animCurveType() == type)
      {
         MFnAnimCurve::TangentType tt = (entry.step ? MFnAnimCurve::kTangentStep : MFnAnimCurve::kTangentLinear);

         unsigned int nk = curve.numKeys();
         for (unsigned int i=0; i<nk; ++i)
         {
            curve.setInTangentType(i, tt);
            curve.setOutTangentType(i, tt);
         }
      }

      ++it;
   }
}

void Keyframer::addCurvesImportInfo(const MObject &node,
                                    const MString &attr,
                                    double speed,
                                    double offset,
                                    double start,
                                    double end,
                                    bool reverse,
                                    bool preserveStart)
{
   MStatus stat;
   
   if (attr.length() == 0)
   {
      NodeCurvesIterator nit = mNodeCurves.find(node);
      CurveIterator cit;
      
      if (nit != mNodeCurves.end())
      {
         for (unsigned int i=0; i<nit->second.length(); ++i)
         {
            cit = mCurves.find(nit->second[i]);
            
            if (cit == mCurves.end())
            {
               continue;
            }
            
            CurveEntry &entry = cit->second;
            
            entry.hasinfo = true;
            entry.speed = speed;
            entry.offset = offset;
            entry.start = start;
            entry.end = end;
            entry.reverse = reverse;
            entry.preserveStart = preserveStart;
         }
      }
   }
   else
   {
      MFnDependencyNode dnode(node);
      
      MString curveName = dnode.name() + "_" + attr;
      
      CurveIterator it = mCurves.find(curveName);
      
      if (it != mCurves.end())
      {
         it->second.hasinfo = true;
         it->second.speed = speed;
         it->second.offset = offset;
         it->second.start = start;
         it->second.end = end;
         it->second.reverse = reverse;
         it->second.preserveStart = preserveStart;
      }
   }
}

void Keyframer::createCurves(MFnAnimCurve::InfinityType preInf,
                             MFnAnimCurve::InfinityType postInf,
                             bool deleteExistingCurves,
                             bool simplifyCurves,
                             double simplifyCurveThreshold)
{
   MStatus stat;
   std::vector<unsigned int> tempIndices;
   double threshold = 0.000001;
   
   CurveIterator it = mCurves.begin();
  
   while (it != mCurves.end())
   {
      CurveEntry &entry = it->second;
      
      if (simplifyCurves)
      {
         cleanupCurve(entry, tempIndices, simplifyCurveThreshold);
      }
    
      // Also check for curves to delete
      // In any other case, times.length() == values.length()
      if ((entry.times.length() == 0 && entry.values.length() == 1) ||
          // Why checking for angle curves here?
          (entry.type != MFnAnimCurve::kAnimCurveTA &&
           entry.type != MFnAnimCurve::kAnimCurveUA &&
           (entry.times.length() == 1 ||
            (entry.times.length() == 2 && fabs(entry.values[0] - entry.values[1]) <= threshold))))
      {
         // set constant value
         // (if there's a curve connected -> disconnect or delete)
         
         MPlug plug(entry.obj, entry.attr);
         
         MDGModifier dgmod;
         MPlugArray srcs;
         plug.connectedTo(srcs, true, false);
         
         for (unsigned int i=0; i<srcs.length(); ++i)
         {
            MObject srcNode = srcs[i].node();
            if (srcNode.hasFn(MFn::kAnimCurve))
            {
               #ifdef _DEBUG
               std::cout << "Disconnect existing curve" << std::endl;
               #endif
               dgmod.disconnect(srcs[i], plug);
               
               if (deleteExistingCurves)
               {
                  dgmod.deleteNode(srcNode);
               }
               
               dgmod.doIt();
            }
         }
         
         plug.setValue(entry.values[0]);
      }
      else
      {
         MFnAnimCurve curve;
         
         curve.create(entry.obj, entry.attr, entry.type, NULL, &stat);
         if (stat != MS::kSuccess)
         {
            // curve might already exists or attribute is non-keyable
            // what policy to observe? keep existing curve
            // -> diconnect existing curve of any and try again
            
            MDGModifier dgmod;
            bool retry = false;
            MPlug plug(entry.obj, entry.attr);
            MPlugArray srcs;
            plug.connectedTo(srcs, true, false);
            
            for (unsigned int i=0; i<srcs.length(); ++i)
            {
               MObject srcNode = srcs[i].node();
               if (srcNode.hasFn(MFn::kAnimCurve))
               {
                  #ifdef _DEBUG
                  std::cout << "Disconnect existing curve" << std::endl;
                  #endif
                  dgmod.disconnect(srcs[i], plug);
                  
                  if (deleteExistingCurves)
                  {
                     dgmod.deleteNode(srcNode);
                  }
                  
                  retry = true;
               }
            }
           
            if (retry)
            {
               dgmod.doIt();
               curve.create(entry.obj, entry.attr, entry.type, NULL, &stat);
            }
         }
         
         if (stat == MS::kSuccess)
         {
            MFnAnimCurve::TangentType tt = (entry.step ? MFnAnimCurve::kTangentStep : MFnAnimCurve::kTangentLinear);
            
            curve.setPreInfinityType(preInf);
            curve.setPostInfinityType(postInf);
            curve.addKeys(&entry.times, &entry.values, tt, tt);
            
            if (entry.hasinfo)
            {
               MFnNumericAttribute nAttr;
                  
               MObject speedAttr = nAttr.create("abcimport_speed", "abcsp", MFnNumericData::kDouble, 0.0);
               nAttr.setStorable(true);
               nAttr.setWritable(true);
               nAttr.setReadable(true);
               nAttr.setKeyable(false);
               nAttr.setHidden(true);
               curve.addAttribute(speedAttr);
               
               MObject offsetAttr = nAttr.create("abcimport_offset", "abcof", MFnNumericData::kDouble, 0.0);
               nAttr.setStorable(true);
               nAttr.setWritable(true);
               nAttr.setReadable(true);
               nAttr.setKeyable(false);
               nAttr.setHidden(true);
               curve.addAttribute(offsetAttr);
               
               MObject startAttr = nAttr.create("abcimport_start", "abcst", MFnNumericData::kDouble, 0.0);
               nAttr.setStorable(true);
               nAttr.setWritable(true);
               nAttr.setReadable(true);
               nAttr.setKeyable(false);
               nAttr.setHidden(true);
               curve.addAttribute(startAttr);
               
               MObject endAttr = nAttr.create("abcimport_end", "abcen", MFnNumericData::kDouble, 0.0);
               nAttr.setStorable(true);
               nAttr.setWritable(true);
               nAttr.setReadable(true);
               nAttr.setKeyable(false);
               nAttr.setHidden(true);
               curve.addAttribute(endAttr);
               
               MObject reverseAttr = nAttr.create("abcimport_reverse", "abcre", MFnNumericData::kBoolean, 0.0);
               nAttr.setStorable(true);
               nAttr.setWritable(true);
               nAttr.setReadable(true);
               nAttr.setKeyable(false);
               nAttr.setHidden(true);
               curve.addAttribute(reverseAttr);
               
               MObject preserveStartAttr = nAttr.create("abcimport_preservestart", "abcps", MFnNumericData::kBoolean, 0.0);
               nAttr.setStorable(true);
               nAttr.setWritable(true);
               nAttr.setReadable(true);
               nAttr.setKeyable(false);
               nAttr.setHidden(true);
               curve.addAttribute(preserveStartAttr);
               
               curve.findPlug("abcimport_speed", false).setDouble(entry.speed);
               curve.findPlug("abcimport_offset", false).setDouble(entry.offset);
               curve.findPlug("abcimport_start", false).setDouble(entry.start);
               curve.findPlug("abcimport_end", false).setDouble(entry.end);
               curve.findPlug("abcimport_reverse", false).setBool(entry.reverse);
               curve.findPlug("abcimport_preservestart", false).setBool(entry.preserveStart);
            }
         }
      }
    
      ++it;
   }
}

void Keyframer::beginRetime()
{
   mRetimedCurves.clear();
   mRetimedCurveInterp.clear();
}

void Keyframer::retimeCurve(MFnAnimCurve &curve,
                            double *speed,
                            double *offset,
                            bool *reverse,
                            bool *preserveStart)
{
   MStatus stat;
   
   if (!speed && !offset && !reverse && !preserveStart)
   {
      // Nothing to do
      return;
   }
   
   if (!curve.isTimeInput() || curve.numKeys() == 0)
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Not a time curve or no keys" << std::endl;
      #endif
      return;
   }
   
   if (mRetimedCurves.find(curve.name()) != mRetimedCurves.end())
   {
      // Nothing to do
      return;
   }
   
   // Retrieve import state
   MPlug pStart = curve.findPlug("abcimport_start", false);
   if (pStart.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Missing \"abcimport_start\" attribute" << std::endl;
      #endif
      return;
   }
   double start = pStart.asDouble();
   
   MPlug pEnd = curve.findPlug("abcimport_end", false);
   if (pEnd.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Missing \"abcimport_end\" attribute" << std::endl;
      #endif
      return;
   }
   double end = pEnd.asDouble();
   
   MPlug pSpeed = curve.findPlug("abcimport_speed", false);
   if (pSpeed.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Missing \"abcimport_speed\" attribute" << std::endl;
      #endif
      return;
   }
   double oldSpeed = pSpeed.asDouble();
   double newSpeed = (speed ? *speed : oldSpeed);
   
   if (fabs(oldSpeed) <= 0.0001 || fabs(newSpeed) <= 0.0001)
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Null speed found" << std::endl;
      #endif
      return;
   }
   double oldSpeedInv = 1.0 / oldSpeed;
   double newSpeedInv = 1.0 / newSpeed;
   
   MPlug pOffset = curve.findPlug("abcimport_offset", false);
   if (pOffset.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Missing \"abcimport_offset\" attribute" << std::endl;
      #endif
      return;
   }
   double oldOffset = pOffset.asDouble();
   double newOffset = (offset ? *offset : oldOffset);
   
   MPlug pReverse = curve.findPlug("abcimport_reverse", false);
   if (pReverse.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Missing \"abcimport_reverse\" attribute" << std::endl;
      #endif
      return;
   }
   bool oldReverse = pReverse.asBool();
   bool newReverse = (reverse ? *reverse : oldReverse);
   
   MPlug pPreserveStart = curve.findPlug("abcimport_preservestart", false);
   if (pPreserveStart.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[altGpuCacheTree] Missing \"abcimport_preservestart\" attribute" << std::endl;
      #endif
      return;
   }
   bool oldPreserveStart = pPreserveStart.asBool();
   bool newPreserveStart = (preserveStart ? *preserveStart : oldPreserveStart);
   
   double oldTotalOffset = oldOffset;
   if (oldPreserveStart)
   {
      oldTotalOffset += start * (oldSpeed - 1.0) * oldSpeedInv;
   }
   
   double newTotalOffset = newOffset;
   if (newPreserveStart)
   {
      newTotalOffset += start * (newSpeed - 1.0) * newSpeedInv;
   }
   
   // Only update curve keys if really needed
   if (fabs(newSpeed - oldSpeed) > 0.0001 ||
       fabs(newTotalOffset - oldTotalOffset) > 0.0001 ||
       newReverse != oldReverse)
   {
      MFnAnimCurve::TangentType tt = curve.outTangentType(0);
      unsigned int nkeys = curve.numKeys();
      MTimeArray times(nkeys, MTime(0.0));
      MDoubleArray values(nkeys, 0.0);
      double t;
      
      // Reset rotation curve interpolation type to "none"
      if (curve.animCurveType() == MFnAnimCurve::kAnimCurveTA)
      {
         MString interp;
         MGlobal::executeCommand("rotationInterpolation -q " + curve.name(), interp);
         if (interp != "none")
         {
            mRetimedCurveInterp[curve.name()] = interp;
            MGlobal::executeCommand("rotationInterpolation -c none " + curve.name());
         }
      }
      
      for (unsigned int i=0; i<nkeys; ++i)
      {
         t = curve.time(i).as(MTime::kSeconds);
         
         // Invert old transform
         t = oldSpeed * (t - oldTotalOffset);
         if (oldReverse)
         {
            t = start - (t - end);
         }
         
         // Apply new transform
         if (newReverse)
         {
            t = end - (t - start);
         }
         t = newTotalOffset + newSpeedInv * t;
         
         times[i] = MTime(t, MTime::kSeconds);
         values[i] = curve.value(i);
      }
      
      for (unsigned int i=0; i<nkeys; ++i)
      {
         curve.remove(nkeys-i-1);
      }
      
      stat = curve.addKeys(&times, &values, tt, tt);
      if (stat != MS::kSuccess)
      {
         #ifdef _DEBUG
         std::cout << "[altGpuCacheTree] Failed to add keys" << std::endl;
         #endif
         return;
      }
      
      mRetimedCurves.insert(curve.name());
   }
   
   pSpeed.setDouble(newSpeed);
   pOffset.setDouble(newOffset);
   pReverse.setBool(newReverse);
   pPreserveStart.setBool(newPreserveStart);
}

void Keyframer::adjustCurve(MFnAnimCurve &curve,
                            MString *interpType,
                            MFnAnimCurve::InfinityType *preInf,
                            MFnAnimCurve::InfinityType *postInf)
{
   if (interpType)
   {
      if (curve.animCurveType() == MFnAnimCurve::kAnimCurveTA)
      {
         mRetimedCurveInterp[curve.name()] = *interpType;
      }
   }
   
   if (preInf)
   {
      curve.setPreInfinityType(*preInf);
   }
   
   if (postInf)
   {
      curve.setPostInfinityType(*postInf);
   }
}

void Keyframer::endRetime()
{
   // Restore rotation curves interpolation type
   MStringDict::iterator it = mRetimedCurveInterp.begin();
   MString cmd;
   
   while (it != mRetimedCurveInterp.end())
   {
      cmd  = "if (`rotationInterpolation -q " + it->first + "` != \"" + it->second + "\") {";
      cmd += " rotationInterpolation -c " + it->second + " " + it->first + "; ";
      cmd += "}";
      MGlobal::executeCommand(cmd);
      ++it;
   }
}
