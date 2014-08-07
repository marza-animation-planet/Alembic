#include "Keyframer.h"
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

      MPlug plug = dnode.findPlug(attrName);
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
   MPlug plug = dnode.findPlug(attrName);

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
   entry.times.append(mTime);
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
   entry.times.append(mTime);
   entry.values.append(v);
}

void  Keyframer::addInheritsTransformKey(const MObject &node, bool v)
{
   MStatus stat;

   MFnDependencyNode dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   CurveEntry &entry = getOrCreateEntry(dnode, "inheritsTransform", MFnAnimCurve::kAnimCurveTU);

   entry.step = true;
   entry.times.append(mTime);
   entry.values.append(v);
}

void Keyframer::addTransformKey(const MObject &node, const MMatrix &m)
{
   MStatus stat;

   MFnTransform dnode(node, &stat);

   if (stat != MS::kSuccess)
   {
      return;
   }

   MTransformationMatrix tm(m);
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

      entry.times.append(mTime);
      entry.values.append(v[i]);

      ++i;
   }
}

void Keyframer::cleanupCurve(CurveEntry &e, std::vector<unsigned int> &removeKeys, double threshold)
{
   unsigned int nk = e.values.length();

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

void Keyframer::setRotationCurvesInterpolation(const MString &type)
{
   MStatus stat;
   MSelectionList rotCurves;

   CurveIterator it = mCurves.begin();

   while (it != mCurves.end())
   {
      CurveEntry &entry = it->second;

      MFnDependencyNode node(entry.obj);
      MPlug plug = node.findPlug(entry.attr);

      MFnAnimCurve curve(plug, &stat);

      if (stat == MStatus::kSuccess && curve.animCurveType() == MFnAnimCurve::kAnimCurveTA)
      {
         rotCurves.add(curve.name());
      }

      ++it;
   }
   
   if (rotCurves.length() > 0)
   {
      MSelectionList oldSel;
      
      MGlobal::getActiveSelectionList(oldSel);
      MGlobal::setActiveSelectionList(rotCurves);
      MGlobal::executeCommand("rotationInterpolation -c " + type);
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
      MPlug plug = node.findPlug(entry.attr);

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
      MPlug plug = node.findPlug(entry.attr);

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
                             MFnAnimCurve::InfinityType postInf)
{
   MStatus stat;
   std::vector<unsigned int> tempIndices;
   double threshold = 0.0;
   
   CurveIterator it = mCurves.begin();
  
   while (it != mCurves.end())
   {
      CurveEntry &entry = it->second;
      
      cleanupCurve(entry, tempIndices, threshold);
    
      if (entry.type != MFnAnimCurve::kAnimCurveTA &&
          entry.type != MFnAnimCurve::kAnimCurveUA &&
          (entry.times.length() == 1 ||
           (entry.times.length() == 2 && fabs(entry.values[0] - entry.values[1]) <= threshold)))
      {
         // set constant value
         MPlug plug(entry.obj, entry.attr);
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
               if (srcs[i].node().hasFn(MFn::kAnimCurve))
               {
                  #ifdef _DEBUG
                  std::cout << "Disconnect existing curve" << std::endl;
                  #endif
                  dgmod.disconnect(srcs[i], plug);
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
               
               curve.findPlug("abcimport_speed").setDouble(entry.speed);
               curve.findPlug("abcimport_offset").setDouble(entry.offset);
               curve.findPlug("abcimport_start").setDouble(entry.start);
               curve.findPlug("abcimport_end").setDouble(entry.end);
               curve.findPlug("abcimport_reverse").setBool(entry.reverse);
               curve.findPlug("abcimport_preservestart").setBool(entry.preserveStart);
            }
         }
      }
    
      ++it;
   }
}

void Keyframer::retimeCurve(const MObject &curveObj,
                            double *speed,
                            double *offset,
                            bool *reverse,
                            bool *preserveStart,
                            MFnAnimCurve::InfinityType *preInf,
                            MFnAnimCurve::InfinityType *postInf) const
{
   MStatus stat;
   
   MFnAnimCurve curve(curveObj, &stat);
   
   if (stat != MS::kSuccess)
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Could not bind curve to re-time" << std::endl;
      #endif
      return;
   }
   
   if (!curve.isTimeInput() || curve.numKeys() == 0)
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Not a time curve or no keys" << std::endl;
      #endif
      return;
   }
   
   MPlug pSpeed = curve.findPlug("abcimport_speed");
   if (pSpeed.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Missing \"abcimport_speed\" attribute" << std::endl;
      #endif
      return;
   }
   double oldSpeed = pSpeed.asDouble();
   double newSpeed = (speed ? *speed : oldSpeed);
   if (fabs(oldSpeed) <= 0.0001 || fabs(newSpeed) <= 0.0001)
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Null speed found" << std::endl;
      #endif
      return;
   }
   double oldSpeedInv = 1.0 / oldSpeed;
   double newSpeedInv = 1.0 / newSpeed;
   
   MPlug pOffset = curve.findPlug("abcimport_offset");
   if (pOffset.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Missing \"abcimport_offset\" attribute" << std::endl;
      #endif
      return;
   }
   double oldOffset = pOffset.asDouble();
   double newOffset = (offset ? *offset : oldOffset);
   
   MPlug pStart = curve.findPlug("abcimport_start");
   if (pStart.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Missing \"abcimport_start\" attribute" << std::endl;
      #endif
      return;
   }
   double start = pStart.asDouble();
   
   MPlug pEnd = curve.findPlug("abcimport_end");
   if (pEnd.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Missing \"abcimport_end\" attribute" << std::endl;
      #endif
      return;
   }
   double end = pEnd.asDouble();
   
   MPlug pReverse = curve.findPlug("abcimport_reverse");
   if (pReverse.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Missing \"abcimport_reverse\" attribute" << std::endl;
      #endif
      return;
   }
   bool oldReverse = pReverse.asBool();
   bool newReverse = (reverse ? *reverse : oldReverse);
   
   MPlug pPreserveStart = curve.findPlug("abcimport_preservestart");
   if (pPreserveStart.isNull())
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Missing \"abcimport_preservestart\" attribute" << std::endl;
      #endif
      return;
   }
   bool oldPreserveStart = pPreserveStart.asBool();
   bool newPreserveStart = (preserveStart ? *preserveStart : oldPreserveStart);
   
   double oldTotalOffset = oldOffset;
   if (oldPreserveStart)
   {
      oldTotalOffset = start * (oldSpeed - 1.0) * oldSpeedInv;
   }
   
   double newTotalOffset = newOffset;
   if (newPreserveStart)
   {
      newTotalOffset = start * (newSpeed - 1.0) * newSpeedInv;
   }
   
   MFnAnimCurve::TangentType tt = curve.outTangentType(0);
   
   unsigned int nkeys = curve.numKeys();
   MTimeArray times(nkeys, MTime(0.0));
   MDoubleArray values(nkeys, 0.0);
   double t;
   
   // Support reverse cycle type?
   
   for (unsigned int i=0; i<nkeys; ++i)
   {
      t = curve.time(i).as(MTime::kSeconds);
      
      // invert old transform
      t = oldSpeed * (t - oldTotalOffset);
      if (oldReverse)
      {
         t = start - (t - end);
      }
      
      // apply new transform
      if (newReverse)
      {
         t = end - (t - start);
      }
      t = newTotalOffset + newSpeedInv * t;
      
      times[i] = MTime(t, MTime::kSeconds);
      values[i] = curve.value(i);
   }
   
   stat = curve.addKeys(&times, &values, tt, tt, false);
   if (stat != MS::kSuccess)
   {
      #ifdef _DEBUG
      std::cout << "[AbcShape] Failed to add keys" << std::endl;
      #endif
      return;
   }
   
   pSpeed.setDouble(newSpeed);
   pOffset.setDouble(newOffset);
   pReverse.setBool(newReverse);
   pPreserveStart.setBool(newPreserveStart);
   
   if (preInf)
   {
      curve.setPreInfinityType(*preInf);
   }
   
   if (postInf)
   {
      curve.setPostInfinityType(*postInf);
   }
}
