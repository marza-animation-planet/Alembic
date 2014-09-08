#ifndef ABCSHAPE_KEYFRAMER_H_
#define ABCSHAPE_KEYFRAMER_H_

#include <maya/MFnAnimCurve.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MTime.h>
#include <maya/MTimeArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MString.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MStringArray.h>
#include <map>
#include <set>
#include <vector>
#include <cstring>

class Keyframer
{
public:
 
   Keyframer();
   ~Keyframer();

   void setCurrentTime(double t, MTime::Unit unit=MTime::kSeconds);
   void addInheritsTransformKey(const MObject &node, bool v);
   void addTransformKey(const MObject &node, const MMatrix &m);
   void addVisibilityKey(const MObject &node, bool v);
   void addAnyKey(const MObject &node, const MString &attr, int index, double val);
   void addCurvesImportInfo(const MObject &node, const MString &attr,
                            double speed, double offset,
                            double start, double end,
                            bool reverse,
                            bool preserveStart);
   void createCurves(MFnAnimCurve::InfinityType preInf = MFnAnimCurve::kConstant,
                     MFnAnimCurve::InfinityType postInf = MFnAnimCurve::kConstant);
   void setRotationCurvesInterpolation(const MString &interpType);
   void fixCurvesTangents(MFnAnimCurve::AnimCurveType type);
   void fixCurvesTangents();
   
   void beginRetime();
   void retimeCurve(MFnAnimCurve &curve,
                    double *speed,
                    double *offset,
                    bool *reverse,
                    bool *preserveStart);
   void adjustCurve(MFnAnimCurve &curve,
                    MString *interpType,
                    MFnAnimCurve::InfinityType *preInf,
                    MFnAnimCurve::InfinityType *postInf);
   void endRetime();
 
private:

   static const char * msTransformAttrNames[];
   static MFnAnimCurve::AnimCurveType msTransformCurveTypes[];
 
   struct CurveEntry
   {
      MObject obj;
      MObject attr;
      MFnAnimCurve::AnimCurveType type;
      bool step; // step type tangents
      MTimeArray times;
      MDoubleArray values;
      
      bool hasinfo;
      double speed;
      double offset;
      double start;
      double end;
      bool preserveStart;
      bool reverse;
   };
   
   struct MStringLessThan
   {
      inline bool operator()(const MString &s0, const MString &s1) const
      {
         return (strcmp(s0.asChar(), s1.asChar()) < 0);
      }
   };
   
   struct MObjectLessThan
   {
      inline bool operator()(const MObject &o0, const MObject &o1) const
      {
         return (MObjectHandle(o0).hashCode() < MObjectHandle(o1).hashCode());
      }
   };
 
private:
 
   CurveEntry& getOrCreateEntry(const MFnDependencyNode &node, const MString &attrName, MFnAnimCurve::AnimCurveType type);
   void cleanupCurve(CurveEntry &e, std::vector<unsigned int> &tempIndices, double threshold=0.000001);

private:
 
   typedef std::map<MString, CurveEntry, MStringLessThan> CurveMap;
   typedef CurveMap::iterator CurveIterator;
   
   typedef std::map<MObject, MStringArray, MObjectLessThan> NodeCurvesMap;
   typedef NodeCurvesMap::iterator NodeCurvesIterator;

   CurveMap mCurves;
   NodeCurvesMap mNodeCurves;
   MTime mTime;
   
   std::set<MString, MStringLessThan> mRetimedCurves;
   std::map<MString, MString, MStringLessThan> mRetimedCurveInterp;
};

#endif
