#include "AbcShapeImport.h"
#include "AbcShape.h"
#include "Keyframer.h"
#include "AlembicSceneCache.h"
#include "AlembicSceneVisitors.h"
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MDagModifier.h>
#include <maya/MGlobal.h>
#include <maya/MNamespace.h>
#include <maya/MFnDagNode.h>
#include <maya/MAnimControl.h>
#include <maya/MPlug.h>
#include <maya/MSyntax.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MEulerRotation.h>
#include <maya/MTime.h>
#include <maya/MAngle.h>
#include <maya/MQuaternion.h>
#include <maya/MFileObject.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnStringData.h>
#include <maya/MDistance.h>

MSyntax AbcShapeImport::createSyntax()
{
   MSyntax syntax;
   
   syntax.addFlag("-m", "-mode", MSyntax::kString);
   syntax.addFlag("-n", "-namespace", MSyntax::kString);
   syntax.addFlag("-r", "-reparent", MSyntax::kString);
   syntax.addFlag("-ftr", "-fitTimeRange", MSyntax::kNoArg);
   syntax.addFlag("-sts", "-setToStartFrame", MSyntax::kNoArg);
   syntax.addFlag("-ci", "-createInstances", MSyntax::kNoArg);
   syntax.addFlag("-it", "-ignoreTransforms", MSyntax::kNoArg);
   syntax.addFlag("-s", "-speed", MSyntax::kDouble);
   syntax.addFlag("-o", "-offset", MSyntax::kDouble);
   syntax.addFlag("-psf", "-preserveStartFrame", MSyntax::kBoolean);
   syntax.addFlag("-ct", "-cycleType", MSyntax::kString);
   syntax.addFlag("-ri", "-rotationInterpolation", MSyntax::kString);
   syntax.addFlag("-nri", "-nodeRotationInterpolation", MSyntax::kString, MSyntax::kString);
   syntax.addFlag("-ft", "-filterObjects", MSyntax::kString);
   syntax.addFlag("-eft", "-excludeFilterObjects", MSyntax::kString);
   syntax.addFlag("-h", "-help", MSyntax::kNoArg);
   
   syntax.makeFlagMultiUse("-nri");
   
   syntax.addArg(MSyntax::kString);
   
   syntax.enableQuery(false);
   syntax.enableEdit(true);
   
   return syntax;
}

void* AbcShapeImport::create()
{
   return new AbcShapeImport();
}

// ---

template <typename T, int D>
struct NumericType
{
   enum
   {
      Value = MFnNumericData::kInvalid
   };
};
template <> struct NumericType<bool, 1> { enum { Value = MFnNumericData::kBoolean }; };
template <> struct NumericType<unsigned char, 1> { enum { Value = MFnNumericData::kByte }; };
template <> struct NumericType<char, 1> { enum { Value = MFnNumericData::kChar }; };
template <> struct NumericType<short, 1> { enum { Value = MFnNumericData::kShort }; };
template <> struct NumericType<short, 2> { enum { Value = MFnNumericData::k2Short }; };
template <> struct NumericType<short, 3> { enum { Value = MFnNumericData::k3Short }; };
template <> struct NumericType<int, 1> { enum { Value = MFnNumericData::kLong }; };
template <> struct NumericType<int, 2> { enum { Value = MFnNumericData::k2Long }; };
template <> struct NumericType<int, 3> { enum { Value = MFnNumericData::k3Long }; };
template <> struct NumericType<float, 1> { enum { Value = MFnNumericData::kFloat }; };
template <> struct NumericType<float, 2> { enum { Value = MFnNumericData::k2Float }; };
template <> struct NumericType<float, 3> { enum { Value = MFnNumericData::k3Float }; };
template <> struct NumericType<double, 1> { enum { Value = MFnNumericData::kDouble }; };
template <> struct NumericType<double, 2> { enum { Value = MFnNumericData::k2Double }; };
template <> struct NumericType<double, 3> { enum { Value = MFnNumericData::k3Double }; };
template <> struct NumericType<double, 4> { enum { Value = MFnNumericData::k4Double }; };

template <typename T>
struct MatrixType
{
   enum
   {
      Value = -1
   };
};
template <> struct MatrixType<float> { enum { Value = MFnMatrixAttribute::kFloat }; };
template <> struct MatrixType<double> { enum { Value = MFnMatrixAttribute::kDouble }; };

template <typename T, int D, typename TT>
struct NumericData
{
   static void Set(const T* value, MPlug &plug, bool convertDistances=false)
   {
   }
};

template <typename T, typename TT>
struct NumericData<T, 1, TT>
{
   static void Set(const T* value, MPlug &plug, bool convertDistances=false)
   {
      plug.setValue(TT(value[0]));
   }
};

template <typename TT>
struct NumericData<float, 1, TT>
{
   static void Set(const float* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      plug.setValue(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]));
   }
};

template <typename TT>
struct NumericData<double, 1, TT>
{
   static void Set(const double* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      plug.setValue(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]));
   }
};

template <typename T, typename TT>
struct NumericData<T, 2, TT>
{
   static void Set(const T* value, MPlug &plug, bool convertDistances=false)
   {
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 2>::Value;
      MObject oval = data.create(type);
      data.setData(TT(value[0]), TT(value[1]));
      plug.setValue(oval);
   }
};

template <typename TT>
struct NumericData<float, 2, TT>
{
   static void Set(const float* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 2>::Value;
      MObject oval = data.create(type);
      data.setData(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]),
                   TT(convertDistances ? MDistance(value[1], srcUnits).as(dstUnits) : value[1]));
      plug.setValue(oval);
   }
};

template <typename TT>
struct NumericData<double, 2, TT>
{
   static void Set(const double* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 2>::Value;
      MObject oval = data.create(type);
      data.setData(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]),
                   TT(convertDistances ? MDistance(value[1], srcUnits).as(dstUnits) : value[1]));
      plug.setValue(oval);
   }
};

template <typename T, typename TT>
struct NumericData<T, 3, TT>
{
   static void Set(const T* value, MPlug &plug, bool convertDistances=false)
   {
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 3>::Value;
      MObject oval = data.create(type);
      data.setData(TT(value[0]), TT(value[1]), TT(value[2]));
      plug.setValue(oval);
   }
};

template <typename TT>
struct NumericData<float, 3, TT>
{
   static void Set(const float* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 3>::Value;
      MObject oval = data.create(type);
      data.setData(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]),
                   TT(convertDistances ? MDistance(value[1], srcUnits).as(dstUnits) : value[1]),
                   TT(convertDistances ? MDistance(value[2], srcUnits).as(dstUnits) : value[2]));
      plug.setValue(oval);
   }
};

template <typename TT>
struct NumericData<double, 3, TT>
{
   static void Set(const double* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 3>::Value;
      MObject oval = data.create(type);
      data.setData(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]),
                   TT(convertDistances ? MDistance(value[1], srcUnits).as(dstUnits) : value[1]),
                   TT(convertDistances ? MDistance(value[2], srcUnits).as(dstUnits) : value[2]));
      plug.setValue(oval);
   }
};

template <typename T, typename TT>
struct NumericData<T, 4, TT>
{
   static void Set(const T* value, MPlug &plug, bool convertDistances=false)
   {
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 4>::Value;
      MObject oval = data.create(type);
      data.setData(TT(value[0]), TT(value[1]), TT(value[2]), TT(value[3]));
      plug.setValue(oval);
   }
};

template <typename TT>
struct NumericData<float, 4, TT>
{
   static void Set(const float* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 4>::Value;
      MObject oval = data.create(type);
      data.setData(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]),
                   TT(convertDistances ? MDistance(value[1], srcUnits).as(dstUnits) : value[1]),
                   TT(convertDistances ? MDistance(value[2], srcUnits).as(dstUnits) : value[2]),
                   TT(convertDistances ? MDistance(value[3], srcUnits).as(dstUnits) : value[3]));
      plug.setValue(oval);
   }
};

template <typename TT>
struct NumericData<double, 4, TT>
{
   static void Set(const double* value, MPlug &plug, bool convertDistances=false)
   {
      MDistance::Unit srcUnits = MDistance::uiUnit();
      MDistance::Unit dstUnits = MDistance::kCentimeters;
      
      MFnNumericData data;
      MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, 4>::Value;
      MObject oval = data.create(type);
      data.setData(TT(convertDistances ? MDistance(value[0], srcUnits).as(dstUnits) : value[0]),
                   TT(convertDistances ? MDistance(value[1], srcUnits).as(dstUnits) : value[1]),
                   TT(convertDistances ? MDistance(value[2], srcUnits).as(dstUnits) : value[2]),
                   TT(convertDistances ? MDistance(value[3], srcUnits).as(dstUnits) : value[3]));
      plug.setValue(oval);
   }
};

// ---

struct RetimeSample
{
   double otime;
   double ntime;
};

class RetimeSampleCompare
{
public:
   
   inline bool operator()(const RetimeSample &s0, const RetimeSample &s1) const
   {
      return (s0.otime < s1.otime);
   }
};

typedef std::set<RetimeSample, RetimeSampleCompare> RetimeSampleSet;

class CreateTree
{
public:
   
   CreateTree(const std::string &abcPath,
              AbcShape::DisplayMode mode,
              bool ignoreTransforms,
              bool createInstances,
              double speed,
              double offset,
              bool preserveStartFrame,
              AbcShape::CycleType cycleType);
   
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicNode &node, AlembicNode *instance=0);

   const MDagPath& getDag(const std::string &path) const;
   
   void keyTransforms(const MString &defaultRotationInterpolation, const MStringDict &nodeRotationInterpolation);
   void keyVisibility(Alembic::AbcGeom::IObject iobj, MFnDependencyNode &fn);
   
private:

   template <class T>
   AlembicNode::VisitReturn enterShape(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
   bool hasDag(const std::string &path) const;
   bool addDag(const std::string &path, const MDagPath &dag);
   bool checkExistingDag(const char *dagType, AlembicNode *node);
   bool createDag(const char *dagType, AlembicNode *node, bool force=false);
   
   void getTransformSamples(AlembicXform *node, RetimeSampleSet &samples);
   void getDefaultTransform(AlembicXform *node, bool worldSpace, MMatrix &outM);
   void getTransformAtTime(AlembicXform *node, double t, bool worldSpace, MMatrix &outM);
   
   template <class T>
   void addUserProps(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
   bool createNumericAttribute(MFnDagNode &node, const std::string &name, MFnNumericData::Type type, bool array, MPlug &plug);
   bool createPointAttribute(MFnDagNode &node, const std::string &name, bool array, MPlug &plug);
   bool createColorAttribute(MFnDagNode &node, const std::string &name, bool alpha, bool array, MPlug &plug);
   bool createMatrixAttribute(MFnDagNode &node, const std::string &name, MFnMatrixAttribute::Type type, bool array, MPlug &plug);
   bool createStringAttribute(MFnDagNode &node, const std::string &name, bool array, MPlug &plug);
   
   bool checkNumericAttribute(const MPlug &plug, MFnNumericData::Type type, bool array);
   bool checkPointAttribute(const MPlug &plug, bool array);
   bool checkColorAttribute(const MPlug &plug, bool alpha, bool array);
   bool checkMatrixAttribute(const MPlug &plug, bool array);
   bool checkStringAttribute(const MPlug &plug, bool array);
   
   template <typename T, int D, typename TT>
   void setNumericAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug);
   template <typename T, int D, typename TT>
   void setCompoundAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug);
   template <typename T>
   void setMatrixAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug);
   void setStringAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug);
   
   template <typename T, int D, typename TT>
   void setNumericAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   template <typename T, int D, typename TT>
   void setCompoundAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   template <typename T>
   void setMatrixAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   void setStringAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   
   template <typename T, int D, typename TT>
   void setNumericArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   template <typename T, int D, typename TT>
   void setCompoundArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   template <typename T>
   void setMatrixArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   void setStringArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   
   template <typename T, int D>
   void attributeSample(Alembic::Abc::IScalarProperty prop, Alembic::AbcCoreAbstract::index_t sampIdx, T *outVal);
   template <typename T, int D>
   void attributeSample(Alembic::Abc::IArrayProperty prop, Alembic::AbcCoreAbstract::index_t sampIdx, T *outVal);
   template <typename T, int D, class Property>
   void keyAttribute(Property prop, MPlug &plug, bool convertDistances=false);
   
   template <typename T, int D, typename TT>
   void setNumericUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   template <typename T, int D>
   void setPointUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   template <typename T, int D>
   void setColorUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   template <typename T>
   void setMatrixUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   void setStringUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   
   template <typename T, int D, typename TT>
   void setNumericArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   template <typename T, int D>
   void setPointArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   template <typename T, int D>
   void setColorArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   template <typename T>
   void setMatrixArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   void setStringArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   
private:
   
   std::string mAbcPath;
   AbcShape::DisplayMode mMode;
   bool mIgnoreTransforms;
   bool mCreateInstances;
   double mSpeed;
   double mOffset;
   bool mPreserveStartFrame;
   AbcShape::CycleType mCycleType;
   std::map<std::string, MDagPath> mDags;
   Keyframer mKeyframer;
   MPlug mTimeSource;
};

CreateTree::CreateTree(const std::string &abcPath,
                       AbcShape::DisplayMode mode,
                       bool ignoreTransforms,
                       bool createInstances,
                       double speed,
                       double offset,
                       bool preserveStartFrame,
                       AbcShape::CycleType cycleType)
   : mAbcPath(abcPath)
   , mMode(mode)
   , mIgnoreTransforms(ignoreTransforms)
   , mCreateInstances(createInstances)
   , mSpeed(speed)
   , mOffset(offset)
   , mPreserveStartFrame(preserveStartFrame)
   , mCycleType(cycleType)
{
   MSelectionList sl;
   MObject timeObj;
   
   sl.add("time1");
   sl.getDependNode(0, timeObj);
   
   MFnDependencyNode timeNode(timeObj);
   mTimeSource = timeNode.findPlug("outTime");
}

bool CreateTree::hasDag(const std::string &path) const
{
   return (mDags.find(path) != mDags.end());
}

const MDagPath& CreateTree::getDag(const std::string &path) const
{
   static MDagPath sInvalidDag;
   std::map<std::string, MDagPath>::const_iterator it = mDags.find(path);
   return (it != mDags.end() ? it->second : sInvalidDag);
}

bool CreateTree::addDag(const std::string &path, const MDagPath &dag)
{
   if (hasDag(path))
   {
      return false;
   }
   
   mDags[path] = dag;
   
   return true;
}

bool CreateTree::checkExistingDag(const char *dagType, AlembicNode *node)
{
   if (!dagType || !node)
   {
      return false;
   }
   else if (!hasDag(node->path()))
   {
      MString curNs = MNamespace::currentNamespace();
      MSelectionList sl;
      MDagPath dagPath;
      
      std::string tmp = node->path();
      
      size_t p0 = 0;
      size_t p1 = tmp.find('/');
      while (p1 != std::string::npos)
      {
         tmp[p1] = '|';
         p0 = p1 + 1;
         p1 = tmp.find('/', p0);
      }
      
      if (curNs != "" && curNs != ":")
      {
         std::string ns = curNs.asChar();
         
         if (ns[ns.length()-1] != ':')
         {
            ns += ":";
         }
         
         p0 = 0;
         p1 = tmp.find('|');
         while (p1 != std::string::npos)
         {
            tmp.insert(p1+1, ns);
            p0 = p1 + 1 + ns.length();
            p1 = tmp.find('|', p0);
         }
      }
      
      if (sl.add(tmp.c_str()) == MS::kSuccess && sl.getDagPath(0, dagPath) == MS::kSuccess)
      {
         MFnDagNode dagNode(dagPath);
         
         // Check type
         if (dagNode.typeName () == dagType)
         {
            MGlobal::displayInfo("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Re-use existing DAG \"" + dagPath.fullPathName() + "\"");
            
            mDags[node->path()] = dagPath;
            return true;
         }
      }
      
      return false;
   }
   else
   {
      return true;
   }
}

bool CreateTree::createDag(const char *dagType, AlembicNode *node, bool force)
{
   if (!dagType || !node)
   {
      return false;
   }
   else if (force || !hasDag(node->path()))
   {
      MDagModifier dagmod;
      MStatus status;
      MObject parentObj = MObject::kNullObj;
      AlembicNode *parent = node->parent();
      
      if (parent)
      {
         MDagPath parentDag = getDag(parent->path());
         if (parentDag.isValid())
         {
            parentObj = parentDag.node();
         }
      }
      
      MObject obj = dagmod.createNode(dagType, parentObj, &status);
      
      if (status == MS::kSuccess && dagmod.doIt() == MS::kSuccess)
      {
         MDagPath dagPath;
         
         MFnDagNode dagNode(obj);
         
         dagNode.setName(node->name().c_str());
         dagNode.getPath(dagPath);
         
         mDags[node->path()] = dagPath;
         
         if (dagType == PREFIX_NAME("AbcShape"))
         {
            AbcShape::AssignDefaultShader(obj);
         }
         
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return true;
   }
}

template <class T>
AlembicNode::VisitReturn CreateTree::enterShape(AlembicNodeT<T> &node, AlembicNode *instance)
{
   AlembicNode *target = (instance ? instance : &node);
   
   if (checkExistingDag(PREFIX_NAME("AbcShape"), target))
   {
      return AlembicNode::ContinueVisit;
   }
   
   if (!createDag(PREFIX_NAME("AbcShape"), target, true))
   {
      return AlembicNode::StopVisit;
   }
   
   // Set AbcShape (the order in which attributes are set is though as to have the lowest possible update cost)
   MPlug plug;
   MFnDagNode dagNode(getDag(target->path()));
   
   plug = dagNode.findPlug("visibleInReflections");
   plug.setBool(true);
   
   plug = dagNode.findPlug("visibleInRefractions");
   plug.setBool(true);
   
   plug = dagNode.findPlug("ignoreXforms");
   // plug.setBool(true);
   plug.setBool(!mIgnoreTransforms);
   // Should I disable 'inheritsTransform' on parent too?
   
   plug = dagNode.findPlug("ignoreInstances");
   // plug.setBool(true);
   plug.setBool(mCreateInstances);
   
   plug = dagNode.findPlug("ignoreVisibility");
   plug.setBool(true);
   
   plug = dagNode.findPlug("cycleType");
   plug.setShort(short(mCycleType));
   
   plug = dagNode.findPlug("speed");
   plug.setDouble(mSpeed);
   
   plug = dagNode.findPlug("offset");
   plug.setDouble(mOffset);
   
   plug = dagNode.findPlug("preserveStartFrame");
   plug.setBool(mPreserveStartFrame);
   
   plug = dagNode.findPlug("objectExpression");
   plug.setString(target->path().c_str());
   
   plug = dagNode.findPlug("displayMode");
   plug.setShort(short(mMode));
   
   plug = dagNode.findPlug("filePath");
   plug.setString(mAbcPath.c_str());
   
   // Only connect time if animated
   // Note: On first evaluation, plug.asXXX doesn't seem to return valid values
   //       though everything is correct within the compute method
   //       Trigger a dummy evaluation before querying the value we're interested in
   //       Internally, AbcShape will handle that nicely so that no un-necessary computations happen
   plug = dagNode.findPlug("numShapes");
   plug.asInt();
   
   plug = dagNode.findPlug("animated");
   if (plug.asBool())
   {
      MDGModifier dgmod;
      
      plug = dagNode.findPlug("time");
      dgmod.connect(mTimeSource, plug);
      dgmod.doIt();
   }
   
   // also key shape level visibilty
   keyVisibility(node.object(), dagNode);
   
   addUserProps(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CreateTree::enter(AlembicMesh &node, AlembicNode *instance)
{
   return enterShape(node, instance);
}

AlembicNode::VisitReturn CreateTree::enter(AlembicSubD &node, AlembicNode *instance)
{
   return enterShape(node, instance);
}

AlembicNode::VisitReturn CreateTree::enter(AlembicPoints &node, AlembicNode *instance)
{
   return enterShape(node, instance);
}

AlembicNode::VisitReturn CreateTree::enter(AlembicCurves &node, AlembicNode *instance)
{
   return enterShape(node, instance);
}

AlembicNode::VisitReturn CreateTree::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   return enterShape(node, instance);
}

void CreateTree::getTransformSamples(AlembicXform *node, RetimeSampleSet &samples)
{
   Alembic::AbcGeom::IXformSchema schema = node->typedObject().getSchema();
   
   size_t numSamples = schema.getNumSamples();
   
   Alembic::AbcCoreAbstract::TimeSamplingPtr ts = schema.getTimeSampling();
   
   double invSpeed = 1.0 / mSpeed;
   double secStart = ts->getSampleTime(0);
   double secEnd = ts->getSampleTime(numSamples-1);
   double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
   double offset = secOffset;
   
   if (mPreserveStartFrame)
   {
      offset += secStart * (mSpeed - 1.0) * invSpeed;
   }
   
   for (size_t i=0; i<numSamples; ++i)
   {
      Alembic::AbcCoreAbstract::index_t idx = i;
      
      RetimeSample rs;
      
      rs.otime = ts->getSampleTime(idx);
      rs.ntime = rs.otime;
      
      if (mCycleType == AbcShape::CT_reverse)
      {
         rs.ntime = secEnd - (rs.ntime - secStart);
      }
      rs.ntime = offset + invSpeed * rs.ntime;
      
      samples.insert(rs);
   }
   
   if (node->parent() && node->parent()->type() == AlembicNode::TypeXform)
   {
      getTransformSamples((AlembicXform*) node->parent(), samples);
   }
}

void CreateTree::getDefaultTransform(AlembicXform *node, bool worldSpace, MMatrix &outM)
{
   Alembic::AbcGeom::IXformSchema schema = node->typedObject().getSchema();
   
   Alembic::AbcGeom::XformSample sample = schema.getValue();
   
   MMatrix M = MMatrix(sample.getMatrix().x);
   
   MDistance::Unit srcUnits = MDistance::uiUnit();
   MDistance::Unit dstUnits = MDistance::kCentimeters;
   
   if (srcUnits != dstUnits)
   {
      M[3][0] = MDistance(M[3][0], srcUnits).as(dstUnits);
      M[3][1] = MDistance(M[3][1], srcUnits).as(dstUnits);
      M[3][2] = MDistance(M[3][2], srcUnits).as(dstUnits);
   }
   
   if (!worldSpace)
   {
      outM = M;
   }
   else
   {
      AlembicNode *parent = node->parent();
      
      if (parent && parent->type() == AlembicNode::TypeXform)
      {
         MMatrix pM;
         
         getDefaultTransform((AlembicXform*) parent, true, pM);
         
         outM = M * pM;
      }
      else
      {
         outM = M;
      }
   }
}

void CreateTree::getTransformAtTime(AlembicXform *node, double t, bool worldSpace, MMatrix &outM)
{
   Alembic::AbcGeom::IXformSchema schema = node->typedObject().getSchema();
   
   MMatrix M;
   
   if (schema.isConstant() || fabs(mSpeed) <= 0.0001)
   {
      Alembic::AbcGeom::XformSample sample = schema.getValue();
      
      M = MMatrix(sample.getMatrix().x);
   }
   else
   {
      // Note: sample doesn't necessarily exists... should then interpolate ourselves
      Alembic::Abc::ISampleSelector selector(t);
      Alembic::AbcGeom::XformSample sample = schema.getValue(selector);
      
      M = MMatrix(sample.getMatrix().x);
   }
   
   MDistance::Unit srcUnits = MDistance::uiUnit();
   MDistance::Unit dstUnits = MDistance::kCentimeters;
   
   if (srcUnits != dstUnits)
   {
      M[3][0] = MDistance(M[3][0], srcUnits).as(dstUnits);
      M[3][1] = MDistance(M[3][1], srcUnits).as(dstUnits);
      M[3][2] = MDistance(M[3][2], srcUnits).as(dstUnits);
   }
   
   if (!worldSpace)
   {
      outM = M;
   }
   else
   {
      AlembicNode *parent = node->parent();
      
      if (parent && parent->type() == AlembicNode::TypeXform)
      {
         MMatrix pM;
         
         getTransformAtTime((AlembicXform*) parent, t, true, pM);
         
         outM = M * pM;
      }
      else
      {
         outM = M;
      }
   }
}

void CreateTree::keyVisibility(Alembic::AbcGeom::IObject iobj, MFnDependencyNode &fn)
{
   Alembic::Abc::ICompoundProperty props = iobj.getProperties();

   if (props.valid())
   {
      const Alembic::Abc::PropertyHeader *header = props.getPropertyHeader("visible");
      
      if (header)
      {
         MObject mobj = fn.object();
         
         if (Alembic::Abc::ICharProperty::matches(*header))
         {
            MPlug pVis = fn.findPlug("visibility");
            
            Alembic::Abc::ICharProperty prop(props, "visible");
            Alembic::Util::int8_t v = 1;
            
            prop.get(v);
            pVis.setBool(v != 0);
               
            if (!prop.isConstant() && fabs(mSpeed) > 0.0001)
            {
               Alembic::AbcCoreAbstract::TimeSamplingPtr ts = prop.getTimeSampling();
               
               size_t numSamples = prop.getNumSamples();
               
               double invSpeed = 1.0 / mSpeed;
               double secStart = ts->getSampleTime(0);
               double secEnd = ts->getSampleTime(numSamples-1);
               double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
               
               double offset = secOffset;
               if (mPreserveStartFrame)
               {
                  offset += secStart * (mSpeed - 1.0) * invSpeed;
               }
               
               for (size_t i=0; i<numSamples; ++i)
               {
                  Alembic::AbcCoreAbstract::index_t idx = i;
                  
                  double t = ts->getSampleTime(idx);
                  if (mCycleType == AbcShape::CT_reverse)
                  {
                     t = secEnd - (t - secStart);
                  }
                  t = offset + invSpeed * t;
                  
                  prop.get(v, Alembic::Abc::ISampleSelector(idx));
                  
                  mKeyframer.setCurrentTime(t);
                  mKeyframer.addVisibilityKey(mobj, v != 0);
               }
               
               mKeyframer.addCurvesImportInfo(mobj, "visibility", mSpeed, secOffset, secStart, secEnd,
                                              (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
            }
         }
         else if (Alembic::Abc::IBoolProperty::matches(*header))
         {
            MPlug pVis = fn.findPlug("visibility");
            
            Alembic::Abc::IBoolProperty prop(props, "visible");
            Alembic::Util::bool_t v = true;
            
            prop.get(v);
            pVis.setBool(v);
               
            if (!prop.isConstant() && fabs(mSpeed) > 0.0001)
            {
               Alembic::AbcCoreAbstract::TimeSamplingPtr ts = prop.getTimeSampling();
               
               size_t numSamples = prop.getNumSamples();
               
               double invSpeed = 1.0 / mSpeed;
               double secStart = ts->getSampleTime(0);
               double secEnd = ts->getSampleTime(numSamples-1);
               double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
               
               double offset = secOffset;
               if (mPreserveStartFrame)
               {
                  offset += secStart * (mSpeed - 1.0) * invSpeed;
               }
               
               for (size_t i=0; i<numSamples; ++i)
               {
                  Alembic::AbcCoreAbstract::index_t idx = i;
                  
                  double t = ts->getSampleTime(idx);
                  if (mCycleType == AbcShape::CT_reverse)
                  {
                     t = secEnd - (t - secStart);
                  }
                  t = offset + invSpeed * t;
                  
                  prop.get(v, Alembic::Abc::ISampleSelector(idx));
                  
                  mKeyframer.setCurrentTime(t);
                  mKeyframer.addVisibilityKey(mobj, v);
               }
               
               mKeyframer.addCurvesImportInfo(mobj, "visibility", mSpeed, secOffset, secStart, secEnd,
                                              (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
            }
         }
      }
   }
}

AlembicNode::VisitReturn CreateTree::enter(AlembicXform &node, AlembicNode *instance)
{
   AlembicNode *target = (instance ? instance : &node);
   
   if (checkExistingDag(node.isLocator() ? "locator" : "transform", target))
   {
      return AlembicNode::ContinueVisit;
   }
   
   MDistance::Unit srcUnits = MDistance::uiUnit();
   MDistance::Unit dstUnits = MDistance::kCentimeters;
   bool convertDistances = (srcUnits != dstUnits);
   
   if (node.isLocator())
   {
      if (!createDag("locator", target, true))
      {
         return AlembicNode::StopVisit;
      }
      
      Alembic::Abc::IScalarProperty loc = node.locatorProperty();
      double posscl[6] = {0, 0, 0, 1, 1, 1};
      
      loc.get(posscl);
      
      MFnDagNode locNode(getDag(target->path()));
      MObject locObj = locNode.object();
      
      if (mIgnoreTransforms)
      {
         // Key direct parent transform with world transformation
         
         AlembicNode *parent = target->parent();
         
         if (parent && parent->type() == AlembicNode::TypeXform)
         {
            MMatrix mmat;
            RetimeSampleSet samples;
            
            AlembicXform *pnode = (AlembicXform*) parent;
            
            MFnTransform xform(getDag(parent->path()));
            MObject xformObj = xform.object();
            
            getTransformSamples(pnode, samples);
            
            if (samples.size() >= 2)
            {
               // samples are already remapped
               RetimeSampleSet::iterator it = samples.begin();
               
               double secStart = it->otime;
               double secEnd = 0.0;
               
               for (; it!=samples.end(); ++it)
               {
                  getTransformAtTime(pnode, it->otime, true, mmat);
                  mKeyframer.setCurrentTime(it->ntime);
                  mKeyframer.addTransformKey(xformObj, mmat);
                  
                  secEnd = it->otime;
               }
               
               double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
               
               mKeyframer.addCurvesImportInfo(xformObj, "", mSpeed, secOffset, secStart, secEnd,
                                              (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
            }
            else if (samples.size() == 1)
            {
               getTransformAtTime(pnode, samples.begin()->otime, true, mmat);
               MTransformationMatrix tm(mmat);
               xform.set(tm);
            }
            else
            {
               getDefaultTransform(pnode, true, mmat);
               MTransformationMatrix tm(mmat);
               xform.set(tm);
            }
         }
      }
      
      MPlug pPx = locNode.findPlug("localPositionX");
      MPlug pPy = locNode.findPlug("localPositionY");
      MPlug pPz = locNode.findPlug("localPositionZ");
      MPlug pSx = locNode.findPlug("localScaleX");
      MPlug pSy = locNode.findPlug("localScaleY");
      MPlug pSz = locNode.findPlug("localScaleZ");
      
      pPx.setDouble(convertDistances ? MDistance(posscl[0], srcUnits).as(dstUnits) : posscl[0]);
      pPy.setDouble(convertDistances ? MDistance(posscl[1], srcUnits).as(dstUnits) : posscl[1]);
      pPz.setDouble(convertDistances ? MDistance(posscl[2], srcUnits).as(dstUnits) : posscl[2]);
      pSx.setDouble(convertDistances ? MDistance(posscl[3], srcUnits).as(dstUnits) : posscl[3]);
      pSy.setDouble(convertDistances ? MDistance(posscl[4], srcUnits).as(dstUnits) : posscl[4]);
      pSz.setDouble(convertDistances ? MDistance(posscl[5], srcUnits).as(dstUnits) : posscl[5]);
      
      if (!loc.isConstant() && fabs(mSpeed) > 0.0001)
      {
         size_t numSamples = loc.getNumSamples();
         
         Alembic::AbcCoreAbstract::TimeSamplingPtr ts = loc.getTimeSampling();
         
         double invSpeed = 1.0 / mSpeed;
         double secStart = ts->getSampleTime(0);
         double secEnd = ts->getSampleTime(numSamples-1);
         double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
         
         double offset = secOffset;
         if (mPreserveStartFrame)
         {
            offset += secOffset * (mSpeed - 1.0) * invSpeed;
         }
         
         for (size_t i=0; i<numSamples; ++i)
         {
            Alembic::AbcCoreAbstract::index_t idx = i;
            
            double t = ts->getSampleTime(idx);
            if (mCycleType == AbcShape::CT_reverse)
            {
               t = secEnd - (t - secStart);
            }
            t = offset + invSpeed * t;
            
            loc.get(posscl, Alembic::Abc::ISampleSelector(idx));
            
            mKeyframer.setCurrentTime(t);
            
            mKeyframer.addAnyKey(locObj, "localPositionX", 0, (convertDistances ? MDistance(posscl[0], srcUnits).as(dstUnits) : posscl[0]));
            mKeyframer.addAnyKey(locObj, "localPositionY", 0, (convertDistances ? MDistance(posscl[1], srcUnits).as(dstUnits) : posscl[1]));
            mKeyframer.addAnyKey(locObj, "localPositionZ", 0, (convertDistances ? MDistance(posscl[2], srcUnits).as(dstUnits) : posscl[2]));
            mKeyframer.addAnyKey(locObj, "localScaleX", 0, (convertDistances ? MDistance(posscl[3], srcUnits).as(dstUnits) : posscl[3]));
            mKeyframer.addAnyKey(locObj, "localScaleY", 0, (convertDistances ? MDistance(posscl[4], srcUnits).as(dstUnits) : posscl[4]));
            mKeyframer.addAnyKey(locObj, "localScaleZ", 0, (convertDistances ? MDistance(posscl[5], srcUnits).as(dstUnits) : posscl[5]));
         }
         
         bool rev = (mCycleType == AbcShape::CT_reverse);
         
         mKeyframer.addCurvesImportInfo(locObj, "localPositionX", mSpeed, secOffset, secStart, secEnd, rev, mPreserveStartFrame);
         mKeyframer.addCurvesImportInfo(locObj, "localPositionY", mSpeed, secOffset, secStart, secEnd, rev, mPreserveStartFrame);
         mKeyframer.addCurvesImportInfo(locObj, "localPositionZ", mSpeed, secOffset, secStart, secEnd, rev, mPreserveStartFrame);
         mKeyframer.addCurvesImportInfo(locObj, "localScaleX", mSpeed, secOffset, secStart, secEnd, rev, mPreserveStartFrame);
         mKeyframer.addCurvesImportInfo(locObj, "localScaleY", mSpeed, secOffset, secStart, secEnd, rev, mPreserveStartFrame);
         mKeyframer.addCurvesImportInfo(locObj, "localScaleZ", mSpeed, secOffset, secStart, secEnd, rev, mPreserveStartFrame);
      }
   }
   else
   {
      if (!createDag("transform", target, true))
      {
         return AlembicNode::StopVisit;
      }
      
      Alembic::AbcGeom::IXformSchema schema = node.typedObject().getSchema();
      
      // key xform attributes, visibilty and inheritsTransform
      
      MFnTransform xform(getDag(target->path()));
      MObject xformObj = xform.object();
      
      if (!mIgnoreTransforms)
      {
         // Note: even if mIgnoreTransforms is set, set maya's 'inheritsTransform' attribute?
         
         // Get default sample and set node state
         Alembic::AbcGeom::XformSample sample = schema.getValue();
         
         // Inherit flag
         MPlug pIT = xform.findPlug("inheritsTransform");
         
         bool inheritsXforms = sample.getInheritsXforms();
         pIT.setBool(inheritsXforms);
         
         // Transformation matrix
         MMatrix mtx(sample.getMatrix().x);
         
         if (convertDistances)
         {
            mtx[3][0] = MDistance(mtx[3][0], srcUnits).as(dstUnits);
            mtx[3][1] = MDistance(mtx[3][1], srcUnits).as(dstUnits);
            mtx[3][2] = MDistance(mtx[3][2], srcUnits).as(dstUnits);
         }
         
         MTransformationMatrix mmat(mtx);
         xform.set(mmat);
         
         if (!schema.isConstant() && fabs(mSpeed) > 0.0001)
         {
            size_t numSamples = schema.getNumSamples();
            
            Alembic::AbcCoreAbstract::TimeSamplingPtr ts = schema.getTimeSampling();
            
            double invSpeed = 1.0 / mSpeed;
            double secStart = ts->getSampleTime(0);
            double secEnd = ts->getSampleTime(numSamples-1);
            double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
            
            double offset = secOffset;
            if (mPreserveStartFrame)
            {
               offset += secStart * (mSpeed - 1.0) * invSpeed;
            }
            
            for (size_t i=0; i<numSamples; ++i)
            {
               Alembic::AbcCoreAbstract::index_t idx = i;
               
               double t = ts->getSampleTime(idx);
               if (mCycleType == AbcShape::CT_reverse)
               {
                  t = secEnd - (t - secStart);
               }
               t = offset + invSpeed * t;
               
               Alembic::AbcGeom::XformSample sample = schema.getValue(Alembic::Abc::ISampleSelector(idx));
               
               MMatrix mmat(sample.getMatrix().x);
               
               if (convertDistances)
               {
                  mmat[3][0] = MDistance(mmat[3][0], srcUnits).as(dstUnits);
                  mmat[3][1] = MDistance(mmat[3][1], srcUnits).as(dstUnits);
                  mmat[3][2] = MDistance(mmat[3][2], srcUnits).as(dstUnits);
               }
               
               mKeyframer.setCurrentTime(t);
               mKeyframer.addTransformKey(xformObj, mmat);
               mKeyframer.addInheritsTransformKey(xformObj, sample.getInheritsXforms());
            }
            
            mKeyframer.addCurvesImportInfo(xformObj, "", mSpeed, secOffset, secStart, secEnd,
                                           (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
         }
      }
      else
      {
         // Leave transform to identity (AbcShape will handle it)
      }
      
      keyVisibility(node.object(), xform);
   }
   
   addUserProps(node, instance);
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn CreateTree::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      if (mCreateInstances)
      {
         // get master
         // need to be sure master is created befoer
         AlembicNode::VisitReturn rv = node.master()->enter(*this);
         if (rv != AlembicNode::ContinueVisit)
         {
            return rv;
         }
         
         MDagPath masterDag = getDag(node.masterPath());
         
         if (!masterDag.isValid())
         {
            return AlembicNode::StopVisit;
         }
         
         if (!node.parent())
         {
            return AlembicNode::StopVisit;
         }
         
         MDagPath parentDag = getDag(node.parent()->path());
         
         if (!parentDag.isValid())
         {
            return AlembicNode::StopVisit;
         }
         
         MFnDagNode parentNode(parentDag);
         MObject masterObj = masterDag.node();
         
         if (parentNode.addChild(masterObj, MFnDagNode::kNextPos, true) != MS::kSuccess)
         {
            return AlembicNode::StopVisit;
         }
         
         return AlembicNode::DontVisitChildren;
      }
      else
      {
         return node.master()->enter(*this, &node);
      }
   }
   
   return AlembicNode::ContinueVisit;
}

bool CreateTree::createPointAttribute(MFnDagNode &node, const std::string &name, bool array, MPlug &plug)
{
   MFnNumericAttribute nAttr;
   MString aname(name.c_str());
   
   MObject attr = nAttr.createPoint(aname, aname);
   nAttr.setReadable(true);
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setArray(array);
   
   if (node.addAttribute(attr) == MS::kSuccess)
   {
      plug = node.findPlug(aname);
      return (!plug.isNull());
   }
   else
   {
      return false;
   }
}

bool CreateTree::createColorAttribute(MFnDagNode &node, const std::string &name, bool alpha, bool array, MPlug &plug)
{
   MFnNumericAttribute nAttr;
   MString aname(name.c_str());
   
   MObject attr;
   
   if (!alpha)
   {
      attr = nAttr.createColor(aname, aname);
      nAttr.setReadable(true);
      nAttr.setWritable(true);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      nAttr.setArray(array);
   }
   else
   {
      MFnCompoundAttribute cAttr;
      
      attr = cAttr.create(aname, aname);
      cAttr.setReadable(true);
      cAttr.setWritable(true);
      cAttr.setStorable(true);
      cAttr.setKeyable(true);
      cAttr.setArray(array);
      
      MObject rattr = nAttr.create(aname+"R", aname+"R", MFnNumericData::kFloat, 0.0);
      nAttr.setReadable(true);
      nAttr.setWritable(true);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      cAttr.addChild(rattr);
      
      MObject gattr = nAttr.create(aname+"G", aname+"G", MFnNumericData::kFloat, 0.0);
      nAttr.setReadable(true);
      nAttr.setWritable(true);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      cAttr.addChild(gattr);
      
      MObject battr = nAttr.create(aname+"B", aname+"B", MFnNumericData::kFloat, 0.0);
      nAttr.setReadable(true);
      nAttr.setWritable(true);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      cAttr.addChild(battr);
      
      MObject aattr = nAttr.create(aname+"A", aname+"A", MFnNumericData::kFloat, 1.0);
      nAttr.setReadable(true);
      nAttr.setWritable(true);
      nAttr.setStorable(true);
      nAttr.setKeyable(true);
      cAttr.addChild(aattr);
   }
   
   if (node.addAttribute(attr) == MS::kSuccess)
   {
      plug = node.findPlug(aname);
      return (!plug.isNull());
   }
   else
   {
      return false;
   }
}

bool CreateTree::createNumericAttribute(MFnDagNode &node, const std::string &name, MFnNumericData::Type type, bool array, MPlug &plug)
{
   MFnNumericAttribute nAttr;
   MString aname(name.c_str());
   
   MObject attr = nAttr.create(aname, aname, type, 0.0);
   nAttr.setReadable(true);
   nAttr.setWritable(true);
   nAttr.setStorable(true);
   nAttr.setKeyable(true);
   nAttr.setArray(array);
   
   if (node.addAttribute(attr) == MS::kSuccess)
   {
      plug = node.findPlug(aname);
      return (!plug.isNull());
   }
   else
   {
      return false;
   }
}

bool CreateTree::createMatrixAttribute(MFnDagNode &node, const std::string &name, MFnMatrixAttribute::Type type, bool array, MPlug &plug)
{
   MFnMatrixAttribute mAttr;
   MString aname(name.c_str());
   
   MObject attr = mAttr.create(aname, aname, type);
   mAttr.setReadable(true);
   mAttr.setWritable(true);
   mAttr.setStorable(true);
   mAttr.setKeyable(true);
   mAttr.setArray(array);
   
   if (node.addAttribute(attr) == MS::kSuccess)
   {
      plug = node.findPlug(aname);
      return (!plug.isNull());
   }
   else
   {
      return false;
   }
}

bool CreateTree::createStringAttribute(MFnDagNode &node, const std::string &name, bool array, MPlug &plug)
{
   MFnTypedAttribute tAttr;
   MString aname(name.c_str());
   
   MFnStringData defaultData;
   MObject defaultObj = defaultData.create("");
   
   MObject attr = tAttr.create(aname, aname, MFnData::kString, defaultObj);
   tAttr.setReadable(true);
   tAttr.setWritable(true);
   tAttr.setStorable(true);
   tAttr.setKeyable(true);
   tAttr.setArray(array);
   
   if (node.addAttribute(attr) == MS::kSuccess)
   {
      plug = node.findPlug(aname);
      return (!plug.isNull());
   }
   else
   {
      return false;
   }
}

bool CreateTree::checkPointAttribute(const MPlug &plug, bool array)
{
   if (plug.isArray() != array)
   {
      // Note: MFnTypedAttribute kPointArray, kVectorArray
      return false;
   }
   
   MStatus stat;
   
   MFnNumericAttribute nAttr(plug.attribute(), &stat);
   
   if (stat != MS::kSuccess)
   {
      return false;
   }
   
   return (nAttr.unitType() == MFnNumericData::k3Float);
}

bool CreateTree::checkColorAttribute(const MPlug &plug, bool alpha, bool array)
{
   if (plug.isArray() != array)
   {
      return false;
   }
   
   MStatus stat;
   
   if (alpha)
   {
      MFnCompoundAttribute cAttr(plug.attribute(), &stat);
      
      if (stat != MS::kSuccess || cAttr.numChildren() != 4)
      {
         return false;
      }
      
      for (int i=0; i<4; ++i)
      {
         MFnNumericAttribute nAttr(cAttr.child(i), &stat);
         
         if (stat != MS::kSuccess || nAttr.unitType() != MFnNumericData::kFloat)
         {
            return false;
         }
      }
      
      return true;
   }
   else
   {
      MFnNumericAttribute nAttr(plug.attribute(), &stat);
   
      if (stat != MS::kSuccess)
      {
         return false;
      }
      
      return (nAttr.unitType() == MFnNumericData::k3Float);
   }
}

bool CreateTree::checkNumericAttribute(const MPlug &plug, MFnNumericData::Type type, bool array)
{
   if (plug.isArray() != array)
   {
      return false;
   }
   
   MStatus stat;
   
   MFnNumericAttribute nAttr(plug.attribute(), &stat);
   
   if (stat != MS::kSuccess)
   {
      return false;
   }
   
   return (nAttr.unitType() == type);
}

bool CreateTree::checkMatrixAttribute(const MPlug &plug, bool array)
{
   if (plug.isArray() != array)
   {
      return false;
   }
   
   MStatus stat;
   
   MFnMatrixAttribute mAttr(plug.attribute(), &stat);
   
   if (stat != MS::kSuccess)
   {
      //MFnTypedAttribute tAttr(plug.attribute(), &stat);
      //return (stat == MS::kSuccess && tAttr.attrType() == MFnData::kMatrix);
      return false;
   }
   
   return true;
}

bool CreateTree::checkStringAttribute(const MPlug &plug, bool array)
{
   if (plug.isArray() != array)
   {
      return false;
   }
   
   MStatus stat;
   
   MFnTypedAttribute tAttr(plug.attribute(), &stat);
   
   if (stat != MS::kSuccess)
   {
      return false;
   }
   
   return (tAttr.attrType() == MFnData::kString);
}

template <typename T, int D>
void CreateTree::attributeSample(Alembic::Abc::IScalarProperty prop, Alembic::AbcCoreAbstract::index_t sampIdx, T *outVal)
{
   prop.get(outVal, Alembic::Abc::ISampleSelector(sampIdx));
}

template <typename T, int D>
void CreateTree::attributeSample(Alembic::Abc::IArrayProperty prop, Alembic::AbcCoreAbstract::index_t sampIdx, T *outVal)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp, Alembic::Abc::ISampleSelector(sampIdx));
   memcpy(outVal, samp->getData(), D * sizeof(T));
}

template <typename T, int D, class Property>
void CreateTree::keyAttribute(Property prop, MPlug &plug, bool convertDistances)
{
   if (!prop.isConstant() && fabs(mSpeed) > 0.0001)
   {
      MObject nodeObj = plug.node();
      MString plugName = plug.partialName();
      T val[D];
      
      size_t numSamples = prop.getNumSamples();
         
      Alembic::AbcCoreAbstract::TimeSamplingPtr ts = prop.getTimeSampling();
      
      double invSpeed = 1.0 / mSpeed;
      double secStart = ts->getSampleTime(0);
      double secEnd = ts->getSampleTime(numSamples-1);
      double secOffset = MTime(mOffset, MTime::uiUnit()).as(MTime::kSeconds);
      bool reverse = (mCycleType == AbcShape::CT_reverse);
      
      double offset = secOffset;
      if (mPreserveStartFrame)
      {
         offset += secOffset * (mSpeed - 1.0) * invSpeed;
      }
      
      for (size_t i=0; i<numSamples; ++i)
      {
         Alembic::AbcCoreAbstract::index_t idx = i;
         
         double t = ts->getSampleTime(idx);
         if (reverse)
         {
            t = secEnd - (t - secStart);
         }
         t = offset + invSpeed * t;
         
         attributeSample<T, D>(prop, idx, val);
         
         mKeyframer.setCurrentTime(t);
         
         if (convertDistances)
         {
            MDistance::Unit srcUnits = MDistance::uiUnit();
            MDistance::Unit dstUnits = MDistance::kCentimeters;
            
            for (int d=0; d<D; ++d)
            {
               mKeyframer.addAnyKey(nodeObj, plugName, d, MDistance(val[d], srcUnits).as(dstUnits));
            }
         }
         else
         {
            for (int d=0; d<D; ++d)
            {
               mKeyframer.addAnyKey(nodeObj, plugName, d, val[d]);
            }
         }
      }
      
      if (D > 1)
      {
         // compound case
         for (int d=0; d<D; ++d)
         {
            mKeyframer.addCurvesImportInfo(nodeObj, plug.child(d).partialName(), mSpeed, secOffset, secStart, secEnd, reverse, mPreserveStartFrame);
         }
      }
      else
      {
         mKeyframer.addCurvesImportInfo(nodeObj, plugName, mSpeed, secOffset, secStart, secEnd, reverse, mPreserveStartFrame);
      }
   }
}

template <typename T, int D, typename TT>
void CreateTree::setNumericAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug)
{
   bool convertDistances = (MDistance::uiUnit() != MDistance::kCentimeters && plug.attribute().hasFn(MFn::kUnitAttribute));
   T val[D];
   
   prop.get(val);
   
   NumericData<T, D, TT>::Set(val, plug, convertDistances);
   
   keyAttribute<T, D>(prop, plug, convertDistances);
}

template <typename T, int D, typename TT>
void CreateTree::setNumericAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   bool convertDistances = (MDistance::uiUnit() != MDistance::kCentimeters && plug.attribute().hasFn(MFn::kUnitAttribute));
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   NumericData<T, D, TT>::Set(val, plug, convertDistances);
   
   keyAttribute<T, D>(prop, plug, convertDistances);
}

template <typename T, int D, typename TT>
void CreateTree::setNumericArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   bool convertDistances = (MDistance::uiUnit() != MDistance::kCentimeters && plug.attribute().hasFn(MFn::kUnitAttribute));
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   for (size_t i=0; i<samp->size(); ++i, val+=D)
   {
      MPlug elem = plug.elementByLogicalIndex(i);
      NumericData<T, D, TT>::Set(val, elem, convertDistances);
   }
}

template <typename T, int D, typename TT>
void CreateTree::setCompoundAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug)
{
   T val[D];
   
   prop.get(val);
   
   for (int d=0; d<D; ++d)
   {
      plug.child(d).setValue(TT(val[d]));
   }
   
   keyAttribute<T, D>(prop, plug);
}

template <typename T, int D, typename TT>
void CreateTree::setCompoundAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   for (int d=0; d<D; ++d)
   {
      plug.child(d).setValue(TT(val[d]));
   }
   
   keyAttribute<T, D>(prop, plug);
}

template <typename T, int D, typename TT>
void CreateTree::setCompoundArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   for (size_t i=0; i<samp->size(); ++i, val+=D)
   {
      MPlug elem = plug.elementByLogicalIndex(i);
      for (int d=0; d<D; ++d)
      {
         elem.child(d).setValue(TT(val[d]));
      }
   }
}

template <typename T>
void CreateTree::setMatrixAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug)
{
   T val[16];
   prop.get(val);
   
   MMatrix m;
   for (int r=0, i=0; r<4; ++r)
   {
      for (int c=0; c<4; ++c, ++i)
      {
         m[r][c] = double(val[i]);
      }
   }
   
   MDistance::Unit srcUnits = MDistance::uiUnit();
   MDistance::Unit dstUnits = MDistance::kCentimeters;
   if (srcUnits != dstUnits)
   {
      m[3][0] = MDistance(m[3][0], srcUnits).as(dstUnits);
      m[3][1] = MDistance(m[3][1], srcUnits).as(dstUnits);
      m[3][2] = MDistance(m[3][2], srcUnits).as(dstUnits);
   }
   
   MFnMatrixData data;
   MObject oval = data.create(m);
   plug.setValue(oval);
   
   // key frames not supported
}

template <typename T>
void CreateTree::setMatrixAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   MMatrix m;
   for (int r=0, i=0; r<4; ++r)
   {
      for (int c=0; c<4; ++c, ++i)
      {
         m[r][c] = double(val[i]);
      }
   }
   
   MDistance::Unit srcUnits = MDistance::uiUnit();
   MDistance::Unit dstUnits = MDistance::kCentimeters;
   if (srcUnits != dstUnits)
   {
      m[3][0] = MDistance(m[3][0], srcUnits).as(dstUnits);
      m[3][1] = MDistance(m[3][1], srcUnits).as(dstUnits);
      m[3][2] = MDistance(m[3][2], srcUnits).as(dstUnits);
   }
   
   MFnMatrixData data;
   MObject oval = data.create(m);
   plug.setValue(oval);
   
   // key frames not supported
}

template <typename T>
void CreateTree::setMatrixArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   MMatrix m;
   
   for (size_t i=0; i<samp->size(); ++i, val+=16)
   {
      for (int r=0, j=0; r<4; ++r)
      {
         for (int c=0; c<4; ++c, ++j)
         {
            m[r][c] = double(val[j]);
         }
      }
      
      MFnMatrixData data;
      MObject oval = data.create(m);
      plug.elementByLogicalIndex(i).setValue(oval);
   }
}

void CreateTree::setStringAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug)
{
   std::string val;
   
   prop.get(&val);
   
   plug.setString(val.c_str());
   
   // key frames not supported
}

void CreateTree::setStringAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const std::string *val = (const std::string*) samp->getData();
   
   plug.setString(val[0].c_str());
   
   // key frames not supported
}

void CreateTree::setStringArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const std::string *val = (const std::string*) samp->getData();
   
   for (size_t i=0; i<samp->size(); ++i)
   {
      plug.elementByLogicalIndex(i).setString(val[i].c_str());
   }
}

template <typename T, int D, typename TT>
void CreateTree::setNumericUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop)
{
   MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, D>::Value;
   
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createNumericAttribute(node, name, type, false, plug)
                                 : checkNumericAttribute(plug, type, false));
   
   if (process)
   {
      setNumericAttribute<T, D, TT>(prop, plug);
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Numeric attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T, int D, typename TT>
void CreateTree::setNumericArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MFnNumericData::Type type = (MFnNumericData::Type) NumericType<TT, D>::Value;
   
   MPlug plug = node.findPlug(name.c_str());
   
   bool array = !prop.isScalarLike();
   
   bool process = (plug.isNull() ? createNumericAttribute(node, name, type, array, plug)
                                 : checkNumericAttribute(plug, type, array));
   
   if (process)
   {
      if (array)
      {
         setNumericArrayAttribute<T, D, TT>(prop, plug);
      }
      else
      {
         setNumericAttribute<T, D, TT>(prop, plug);
      }
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Numeric array attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T, int D>
void CreateTree::setPointUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createPointAttribute(node, name, false, plug)
                                 : checkPointAttribute(plug, false));
   
   if (process)
   {
      setNumericAttribute<T, D, float>(prop, plug);
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Point attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T, int D>
void CreateTree::setPointArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool array = !prop.isScalarLike();
   
   bool process = (plug.isNull() ? createPointAttribute(node, name, array, plug)
                                 : checkPointAttribute(plug, array));
   
   if (process)
   {
      if (array)
      {
         setNumericArrayAttribute<T, D, float>(prop, plug);
      }
      else
      {
         setNumericAttribute<T, D, float>(prop, plug);
      }
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Point array attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T, int D>
void CreateTree::setColorUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool alpha = (D == 4);
   
   bool process = (plug.isNull() ? createColorAttribute(node, name, alpha, false, plug)
                                 : checkColorAttribute(plug, alpha, false));
   
   if (process)
   {
      if (!alpha)
      {
         setNumericAttribute<T, D, float>(prop, plug);
      }
      else
      {
         setCompoundAttribute<T, D, float>(prop, plug);
      }
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Color attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T, int D>
void CreateTree::setColorArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool array = !prop.isScalarLike();
   
   bool alpha = (D == 4);
   
   bool process = (plug.isNull() ? createColorAttribute(node, name, alpha, array, plug)
                                 : checkColorAttribute(plug, alpha, array));
   
   if (process)
   {
      if (array)
      {
         if (!alpha)
         {
            setNumericArrayAttribute<T, D, float>(prop, plug);
         }
         else
         {
            setCompoundArrayAttribute<T, D, float>(prop, plug);
         }
      }
      else
      {
         if (!alpha)
         {
            setNumericAttribute<T, D, float>(prop, plug);
         }
         else
         {
            setCompoundAttribute<T, D, float>(prop, plug);
         }
      }
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Color array attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T>
void CreateTree::setMatrixUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop)
{
   MFnMatrixAttribute::Type type = (MFnMatrixAttribute::Type) MatrixType<T>::Value;
   
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createMatrixAttribute(node, name, type, false, plug)
                                 : checkMatrixAttribute(plug, false));
   
   if (process)
   {
      setMatrixAttribute<T>(prop, plug);
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Matrix attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <typename T>
void CreateTree::setMatrixArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MFnMatrixAttribute::Type type = (MFnMatrixAttribute::Type) MatrixType<T>::Value;
   
   MPlug plug = node.findPlug(name.c_str());
   
   bool array = !prop.isScalarLike();
   
   bool process = (plug.isNull() ? createMatrixAttribute(node, name, type, array, plug)
                                 : checkMatrixAttribute(plug, array));
   
   if (process)
   {
      if (array)
      {
         setMatrixArrayAttribute<T>(prop, plug);
      }
      else
      {
         setMatrixAttribute<T>(prop, plug);
      }
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] Matrix array attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

void CreateTree::setStringUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createStringAttribute(node, name, false, plug)
                                 : checkStringAttribute(plug, false));
   
   if (process)
   {
      setStringAttribute(prop, plug);
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] String attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

void CreateTree::setStringArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool array = !prop.isScalarLike();
   
   bool process = (plug.isNull() ? createStringAttribute(node, name, array, plug)
                                 : checkStringAttribute(plug, array));
   
   if (process)
   {
      if (array)
      {
         setStringArrayAttribute(prop, plug);
      }
      else
      {
         setStringAttribute(prop, plug);
      }
   }
   else
   {
      MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + "] String array attribute \"" + MString(name.c_str()) + "\" could not be created or doesn't match alembic file's description");
   }
}

template <class T>
void CreateTree::addUserProps(AlembicNodeT<T> &node, AlembicNode *instance)
{
   Alembic::Abc::ICompoundProperty props = node.typedObject().getSchema().getUserProperties();
   
   if (!props.valid())
   {
      return;
   }
   
   MDagPath dag = getDag(instance ? instance->path() : node.path());
   
   if (!dag.isValid())
   {
      return;
   }
   
   MFnDagNode target(dag);
   
   size_t numProps = props.getNumProperties();
   
   for (size_t i=0; i<numProps; ++i)
   {
      const Alembic::Abc::PropertyHeader &header = props.getPropertyHeader(i);
      const std::string &propName = header.getName();
      
      if (propName.empty())
      {
         continue;
      }
      
      // filter?
      
      const Alembic::AbcCoreAbstract::DataType &dataType = header.getDataType();
      const Alembic::AbcCoreAbstract::MetaData &metaData = header.getMetaData();
      std::string interpretation = metaData.get("interpretation");
      
      switch (dataType.getPod())
      {
      case Alembic::Util::kBooleanPOD:
         if (dataType.getExtent() == 1)
         {
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::bool_t, 1, bool>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::bool_t, 1, bool>(target, propName, prop);
            }
         }
         else
         {
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kUint8POD:
         if (dataType.getExtent() == 1)
         {
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint8_t, 1, unsigned char>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint8_t, 1, unsigned char>(target, propName, prop);
            }
         }
         else
         {
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kInt8POD:
         if (dataType.getExtent() == 1)
         {
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int8_t, 1, char>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int8_t, 1, char>(target, propName, prop);
            }
         }
         else
         {
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kUint16POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint16_t, 1, short>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint16_t, 1, short>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint16_t, 2, short>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint16_t, 2, short>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint16_t, 3, short>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint16_t, 3, short>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kInt16POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int16_t, 1, short>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int16_t, 1, short>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int16_t, 2, short>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int16_t, 2, short>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int16_t, 3, short>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int16_t, 3, short>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kUint32POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint32_t, 1, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint32_t, 1, int>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint32_t, 2, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint32_t, 2, int>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint32_t, 3, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint32_t, 3, int>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kInt32POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int32_t, 1, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int32_t, 1, int>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int32_t, 2, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int32_t, 2, int>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int32_t, 3, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int32_t, 3, int>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kUint64POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint64_t, 1, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint64_t, 1, int>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint64_t, 2, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint64_t, 2, int>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::uint64_t, 3, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::uint64_t, 3, int>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kInt64POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int64_t, 1, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int64_t, 1, int>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int64_t, 2, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int64_t, 2, int>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::int64_t, 3, int>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::int64_t, 3, int>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kFloat16POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::float16_t, 1, float>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::float16_t, 1, float>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<Alembic::Util::float16_t, 2, float>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<Alembic::Util::float16_t, 2, float>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               if (interpretation == "vector" || interpretation == "normal" || interpretation == "normal")
               {
                  setPointUserProp<Alembic::Util::float16_t, 3>(target, propName, prop);
               }
               else if (interpretation == "rgb")
               {
                  setColorUserProp<Alembic::Util::float16_t, 3>(target, propName, prop);
               }
               else
               {
                  setNumericUserProp<Alembic::Util::float16_t, 3, float>(target, propName, prop);
               }
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               if (interpretation == "vector" || interpretation == "normal" || interpretation == "normal")
               {
                  setPointArrayUserProp<Alembic::Util::float16_t, 3>(target, propName, prop);
               }
               else if (interpretation == "rgb")
               {
                  setColorArrayUserProp<Alembic::Util::float16_t, 3>(target, propName, prop);
               }
               else
               {
                  setNumericArrayUserProp<Alembic::Util::float16_t, 3, float>(target, propName, prop);
               }
            }
            break;
         case 4:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               if (interpretation == "rgba")
               {
                  setColorUserProp<Alembic::Util::float16_t, 4>(target, propName, prop);
               }
               else
               {
                  setNumericUserProp<Alembic::Util::float16_t, 4, double>(target, propName, prop);
               }
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               if (interpretation == "rgba")
               {
                  setColorArrayUserProp<Alembic::Util::float16_t, 4>(target, propName, prop);
               }
               else
               {
                  setNumericArrayUserProp<Alembic::Util::float16_t, 4, double>(target, propName, prop);
               }
            }
            break;
         case 16:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setMatrixUserProp<Alembic::Util::float16_t>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setMatrixArrayUserProp<Alembic::Util::float16_t>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kFloat32POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<float, 1, float>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<float, 1, float>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<float, 2, float>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<float, 2, float>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               if (interpretation == "vector" || interpretation == "normal" || interpretation == "normal")
               {
                  setPointUserProp<float, 3>(target, propName, prop);
               }
               else if (interpretation == "rgb")
               {
                  setColorUserProp<float, 3>(target, propName, prop);
               }
               else
               {
                  setNumericUserProp<float, 3, float>(target, propName, prop);
               }
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               if (interpretation == "vector" || interpretation == "normal" || interpretation == "normal")
               {
                  setPointArrayUserProp<float, 3>(target, propName, prop);
               }
               else if (interpretation == "rgb")
               {
                  setColorArrayUserProp<float, 3>(target, propName, prop);
               }
               else
               {
                  setNumericArrayUserProp<float, 3, float>(target, propName, prop);
               }
            }
            break;
         case 4:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               if (interpretation == "rgba")
               {
                  setColorUserProp<float, 4>(target, propName, prop);
               }
               else
               {
                  setNumericUserProp<float, 4, double>(target, propName, prop);
               }
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               if (interpretation == "rgba")
               {
                  setColorArrayUserProp<float, 4>(target, propName, prop);
               }
               else
               {
                  setNumericArrayUserProp<float, 4, double>(target, propName, prop);
               }
            }
            break;
         case 16:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setMatrixUserProp<float>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setMatrixArrayUserProp<float>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kFloat64POD:
         switch (dataType.getExtent())
         {
         case 1:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<double, 1, double>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<double, 1, double>(target, propName, prop);
            }
            break;
         case 2:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setNumericUserProp<double, 2, double>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setNumericArrayUserProp<double, 2, double>(target, propName, prop);
            }
            break;
         case 3:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               if (interpretation == "vector" || interpretation == "normal" || interpretation == "normal")
               {
                  setPointUserProp<double, 3>(target, propName, prop);
               }
               else if (interpretation == "rgb")
               {
                  setColorUserProp<double, 3>(target, propName, prop);
               }
               else
               {
                  setNumericUserProp<double, 3, double>(target, propName, prop);
               }
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               if (interpretation == "vector" || interpretation == "normal" || interpretation == "normal")
               {
                  setPointArrayUserProp<double, 3>(target, propName, prop);
               }
               else if (interpretation == "rgb")
               {
                  setColorArrayUserProp<double, 3>(target, propName, prop);
               }
               else
               {
                  setNumericArrayUserProp<double, 3, double>(target, propName, prop);
               }
            }
            break;
         case 4:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               if (interpretation == "rgba")
               {
                  setColorUserProp<double, 4>(target, propName, prop);
               }
               else
               {
                  setNumericUserProp<double, 4, double>(target, propName, prop);
               }
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               if (interpretation == "rgba")
               {
                  setColorArrayUserProp<double, 4>(target, propName, prop);
               }
               else
               {
                  setNumericArrayUserProp<double, 4, double>(target, propName, prop);
               }
            }
            break;
         case 16:
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setMatrixUserProp<double>(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setMatrixArrayUserProp<double>(target, propName, prop);
            }
            break;
         default:
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      case Alembic::Util::kStringPOD:
         if (dataType.getExtent() == 1)
         {
            if (header.isScalar())
            {
               Alembic::Abc::IScalarProperty prop(props, propName);
               setStringUserProp(target, propName, prop);
            }
            else
            {
               Alembic::Abc::IArrayProperty prop(props, propName);
               setStringArrayUserProp(target, propName, prop);
            }
         }
         else
         {
            MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported extent size for user property of type ") +
                                    Alembic::Util::PODName(dataType.getPod()));
         }
         break;
      default:
         MGlobal::displayWarning("[" + MString(PREFIX_NAME("AbcShapeImport")) + MString("] Unsupported user property type ") +
                                 Alembic::Util::PODName(dataType.getPod()));
      }
   }
}

void CreateTree::leave(AlembicNode &, AlembicNode *)
{
}

void CreateTree::keyTransforms(const MString &defaultRotationInterpolation, const MStringDict &nodeRotationInterpolation)
{
   MFnAnimCurve::InfinityType inf = MFnAnimCurve::kConstant;
   
   // Note: CT_reverse is like CT_hold as far as infinity is concerned
   
   switch (mCycleType)
   {
   case AbcShape::CT_loop:
      inf = MFnAnimCurve::kCycle;
      break;
   case AbcShape::CT_bounce:
      inf = MFnAnimCurve::kOscillate;
   default:
      break;
   }
   
   mKeyframer.createCurves(inf, inf);
   mKeyframer.setRotationCurvesInterpolation(defaultRotationInterpolation, nodeRotationInterpolation);
}

// ---

AbcShapeImport::AbcShapeImport()
   : MPxCommand()
{
}

AbcShapeImport::~AbcShapeImport()
{
}

bool AbcShapeImport::hasSyntax() const
{
   return true;
}

bool AbcShapeImport::isUndoable() const
{
   return false;
}

MStatus AbcShapeImport::doIt(const MArgList& args)
{
   MStatus status;
   
   MArgParser argData(syntax(), args, &status);
   
   // default interpoation type
   MString rotInterp("quaternion");
   MStringDict nodeRotInterp;
   AbcShape::DisplayMode dm = AbcShape::DM_box;
   AbcShape::CycleType ct = AbcShape::CT_hold;
   double speed = 1.0;
   double offset = 0.0;
   bool preserveStartFrame = false;
   
   bool setSpeed = false;
   bool setOffset = false;
   bool setMode = false;
   bool setCycle = false;
   bool setPreserveStart = false;
   bool setInterp = false;
   
   argData.isFlagSet("preserveStartFrame");
   
   if (argData.isFlagSet("help"))
   {
      MGlobal::displayInfo(MString(PREFIX_NAME("AbcShapeImport")) + " [options] abc_file_path");
      MGlobal::displayInfo("Options:");
      MGlobal::displayInfo("-r / -reparent                   : dagpath");
      MGlobal::displayInfo("                                   Reparent the whole hierarchy under a node in the current Maya scene.");
      MGlobal::displayInfo("-n / -namespace                  : string");
      MGlobal::displayInfo("                                   Namespace to add nodes to (default to current namespace).");
      MGlobal::displayInfo("-m / -mode                       : string (box|boxes|points|geometry)");
      MGlobal::displayInfo("                                   " + MString(PREFIX_NAME("AbcShape")) + " nodes display mode (default to box).");
      MGlobal::displayInfo("-s / -speed                      : double");
      MGlobal::displayInfo("                                   " + MString(PREFIX_NAME("AbcShape")) + " nodes speed (default to 1.0).");
      MGlobal::displayInfo("-o / -offset                     : double");
      MGlobal::displayInfo("                                   " + MString(PREFIX_NAME("AbcShape")) + " nodes offset in frames (default to 0.0).");
      MGlobal::displayInfo("-psf / -preserveStartFrame       : bool");
      MGlobal::displayInfo("                                   Preserve range start frame when using speed.");
      MGlobal::displayInfo("-ct / -cycleType                 : string (hold|loop|reverse|bounce)");
      MGlobal::displayInfo("                                   " + MString(PREFIX_NAME("AbcShape")) + " nodes cycle type (default to hold).");
      MGlobal::displayInfo("-ci / -createInstances           :");
      MGlobal::displayInfo("                                   Create maya instances.");
      MGlobal::displayInfo("-it / -ignoreTransforms          :");
      MGlobal::displayInfo("                                   Do not key transform nodes (but for locators direct parent).");
      MGlobal::displayInfo("-ri / -rotationInterpolation     : string (none|euler|quaternion|quaternionSlerp|quaternionSquad)");
      MGlobal::displayInfo("                                   Set created rotation curves interpolation type (default to quaternion).");
      MGlobal::displayInfo("-ri / -nodeRotationInterpolation : string string (none|euler|quaternion|quaternionSlerp|quaternionSquad)");
      MGlobal::displayInfo("                                   Override rotation curves interpolation type (second value) for a specific node (first value).");
      MGlobal::displayInfo("                                   Node name is the alembic node name.");
      MGlobal::displayInfo("-ftr / -fitTimeRange             :");
      MGlobal::displayInfo("                                   Change Maya time slider to fit the range of input file.");
      MGlobal::displayInfo("-sts / -setToStartFrame          :");
      MGlobal::displayInfo("                                   Set the current time to the start of the frame range.");
      MGlobal::displayInfo("-ft / -filterObjects             : string");
      MGlobal::displayInfo("                                   Selective import cache objects whose name matches expression.");
      MGlobal::displayInfo("-eft / -excludeFilterObjects     : string");
      MGlobal::displayInfo("                                   Selective exclude cache objects whose name matches expression.");
      MGlobal::displayInfo("-h / -help                       :");
      MGlobal::displayInfo("                                   Display this help.");
      MGlobal::displayInfo("");
      MGlobal::displayInfo("Command also work in edit mode:");
      MGlobal::displayInfo("  -mode, -speed, -offset, -preserveStartFrame, -cycleType, -rotationInterpolation flags are supported.");
      MGlobal::displayInfo("  Acts on all " + MString(PREFIX_NAME("AbcShape")) + " in selected node trees.");
   }
   
   if (argData.isFlagSet("preserveStartFrame"))
   {
      status = argData.getFlagArgument("preserveStartFrame", 0, preserveStartFrame);
      
      if (status != MS::kSuccess)
      {
         MGlobal::displayWarning("Invalid preserveStartFrame flag argument");
         preserveStartFrame = false;
      }
      else
      {
         setPreserveStart = true;
      }
   }
   
   if (argData.isFlagSet("speed"))
   {
      status = argData.getFlagArgument("speed", 0, speed);
      
      if (status != MS::kSuccess)
      {
         MGlobal::displayWarning("Invalid speed flag argument");
         speed = 1.0;
      }
      else
      {
         setSpeed = true;
      }
   }
   
   if (argData.isFlagSet("offset"))
   {
      status = argData.getFlagArgument("offset", 0, offset);
      
      if (status != MS::kSuccess)
      {
         MGlobal::displayWarning("Invalid offset flag argument");
         offset = 0.0;
      }
      else
      {
         setOffset = true;
      }
   }
   
   if (argData.isFlagSet("mode"))
   {
      MString val;
      status = argData.getFlagArgument("mode", 0, val);
      
      if (status == MS::kSuccess)
      {
         if (val == "box")
         {
            dm = AbcShape::DM_box;
            setMode = true;
         }
         else if (val == "boxes")
         {
            dm = AbcShape::DM_boxes;
            setMode = true;
         }
         else if (val == "points")
         {
            dm = AbcShape::DM_points;
            setMode = true;
         }
         else if (val == "geometry")
         {
            dm = AbcShape::DM_geometry;
            setMode = true;
         }
         else
         {
            MGlobal::displayWarning(val + " is not a valid mode");
         }
      }
      else
      {
         MGlobal::displayWarning("Invalid mode flag argument");
      }
   }
   
   if (argData.isFlagSet("cycleType"))
   {
      MString val;
      status = argData.getFlagArgument("cycleType", 0, val);
      
      if (status == MS::kSuccess)
      {
         if (val == "hold")
         {
            ct = AbcShape::CT_hold;
            setCycle = true;
         }
         else if (val == "loop")
         {
            ct = AbcShape::CT_loop;
            setCycle = true;
         }
         else if (val == "reverse")
         {
            ct = AbcShape::CT_reverse;
            setCycle = true;
         }
         else if (val == "bounce")
         {
            ct = AbcShape::CT_bounce;
            setCycle = true;
         }
         else
         {
            MGlobal::displayWarning(val + " is not a valid cycleType value");
         }
      }
      else
      {
         MGlobal::displayWarning("Invalid cycleTyle flag argument");
      }
   }
   
   if (argData.isFlagSet("rotationInterpolation"))
   {
      MString val;
      status = argData.getFlagArgument("rotationInterpolation", 0, val);
      
      if (status == MS::kSuccess)
      {
         if (val == "none" ||
             val == "euler" ||
             val == "quaternion" ||
             val == "quaternionSlerp" ||
             val == "quaternionSquad")
         {
            rotInterp = val;
            setInterp = true;
         }
         else
         {
            MGlobal::displayWarning("Unsupported interpolation type \"" + val + "\" for rotationInterpolation flag");
         }
      }
      else
      {
         MGlobal::displayWarning("Invalid rotationInterpolation flag argument");
      }
   }
   
   unsigned int count = argData.numberOfFlagUses("nodeRotationInterpolation");
   for (unsigned int i=0; i<count; ++i)
   {
      MArgList argList;
      
      argData.getFlagArgumentList("nodeRotationInterpolation", i, argList);
      
      if (argList.length() != 2)
      {
         MGlobal::displayWarning("Invalid arguments for nodeRotationInterpolation flag");
         continue;
      }
      
      MString nodeName = argList.asString(0, &status);
      if (status != MS::kSuccess)
      {
         MGlobal::displayWarning("Invalid arguments for nodeRotationInterpolation flag");
         continue;
      }
      
      MString interpType = argList.asString(1, &status);
      if (status != MS::kSuccess)
      {
         MGlobal::displayWarning("Invalid arguments for nodeRotationInterpolation flag");
         continue;
      }
      else if (interpType != "none" &&
               interpType != "euler" &&
               interpType != "quaternion" &&
               interpType != "quaternionSlerp" &&
               interpType != "quaternionSquad")
      {
         MGlobal::displayWarning("Unsupported interpolation type \"" + interpType + "\" for nodeRotationInterpolation flag");
         continue;
      }
      
      nodeRotInterp[nodeName] = interpType;
   }
   
   if (argData.isEdit())
   {
      MSelectionList oldSel;
      MSelectionList newSel;
      MSelectionList tmpSel;
      
      MGlobal::getActiveSelectionList(oldSel);
      
      MGlobal::executeCommand("select -hi");
      MGlobal::getActiveSelectionList(newSel);
      
      MDagPath path;
      MFnDagNode node;
      
      // Keep a list of processed curves
      bool processCurves = (setSpeed || setOffset || setCycle || setPreserveStart || setInterp || nodeRotInterp.size() > 0);
      
      Keyframer keyframer;
      MStringArray connectedCurves;
      
      double secOffset = MTime(offset, MTime::uiUnit()).as(MTime::kSeconds);
      
      keyframer.beginRetime();
      
      for (unsigned int i=0; i<newSel.length(); ++i)
      {
         if (newSel.getDagPath(i, path) != MS::kSuccess)
         {
            continue;
         }
         
         if (node.setObject(path) != MS::kSuccess)
         {
            continue;
         }
         
         if (node.typeName() == PREFIX_NAME("AbcShape"))
         {
            if (setSpeed)
            {
               node.findPlug("speed").setDouble(speed);
            }
            if (setOffset)
            {
               node.findPlug("offset").setDouble(offset);
            }
            if (setMode)
            {
               node.findPlug("displayMode").setShort(short(dm));
            }
            if (setCycle)
            {
               node.findPlug("cycleType").setShort(short(ct));
            }
            if (setPreserveStart)
            {
               node.findPlug("preserveStartFrame").setBool(preserveStartFrame);
            }
         }
         
         if (processCurves)
         {
            MObject curveObj;
            
            connectedCurves.clear();
            MGlobal::executeCommand("listConnections -type animCurve -s 1 -d 0 \"" + path.fullPathName() + "\"", connectedCurves);
            
            MFnAnimCurve::InfinityType inf = MFnAnimCurve::kConstant;
            bool reverse = false;
            
            if (setCycle)
            {
               switch (ct)
               {
               case AbcShape::CT_loop:
                  inf = MFnAnimCurve::kCycle;
                  break;
               case AbcShape::CT_bounce:
                  inf = MFnAnimCurve::kOscillate;
                  break;
               case AbcShape::CT_reverse:
                  reverse = true;
               default:
                  break;
               }
            }
            
            bool setNodeInterp = setInterp;
            MString nodeInterp = rotInterp;
            
            // Get target node name without parent or namespaces
            MString nodeName = path.partialPathName();
            int r = nodeName.rindexW(':');
            if (r == -1)
            {
               r = nodeName.rindexW('|');
            }
            if (r != -1)
            {
               nodeName = nodeName.substringW(r + 1, nodeName.numChars() - 1);
            }
            
            // Check for node specific interpolation override
            MStringDict::const_iterator it = nodeRotInterp.find(nodeName);
            if (it != nodeRotInterp.end())
            {
               nodeInterp = it->second;
               setNodeInterp = true;
            }
            
            for (unsigned int j=0; j<connectedCurves.length(); ++j)
            {
               tmpSel.clear();
               if (tmpSel.add(connectedCurves[j]) != MS::kSuccess)
               {
                  continue;
               }
               
               if (tmpSel.getDependNode(0, curveObj) != MS::kSuccess)
               {
                  continue;
               }
               
               MFnAnimCurve curve(curveObj, &status);
               if (status != MS::kSuccess)
               {
                  continue;
               }
               
               keyframer.retimeCurve(curve,
                                     (setSpeed ? &speed : 0),
                                     (setOffset ? &secOffset : 0),
                                     (setCycle ? &reverse : 0),
                                     (setPreserveStart ? &preserveStartFrame : 0));
               
               keyframer.adjustCurve(curve,
                                     (setNodeInterp ? &nodeInterp : 0),
                                     (setCycle ? &inf : 0),
                                     (setCycle ? &inf : 0));
            }
         }
      }
      
      keyframer.endRetime();
      
      MGlobal::setActiveSelectionList(oldSel);
      
      status = MS::kSuccess;
   }
   else
   {
      MString filename("");
      MString ns("");
      MString includeFilter("");
      MString excludeFilter("");
      MString curNs = MNamespace::currentNamespace();
      MDagPath parentDag;
      
      bool fitTimeRange = argData.isFlagSet("fitTimeRange");
      bool setToStartFrame = argData.isFlagSet("setToStartFrame");
      bool createInstances = argData.isFlagSet("createInstances");
      bool ignoreTransforms = argData.isFlagSet("ignoreTransforms");
      
      if (ignoreTransforms && createInstances)
      {
         MGlobal::displayWarning("'createInstances' and 'ignoreTransforms' options are incompatible");
      }
      
      if (argData.isFlagSet("reparent"))
      {
         MSelectionList sl;
         
         MString val;
         MDagPath dag;
         status = argData.getFlagArgument("reparent", 0, val);
         
         if (status == MS::kSuccess)
         {
            if (sl.add(val) == MS::kSuccess && sl.getDagPath(0, dag) == MS::kSuccess)
            {
               parentDag = dag;
            }
            else
            {
               MGlobal::displayWarning(val + " is not a valid dag path");
            }
         }
         else
         {
            MGlobal::displayWarning("Invalid reparent flag argument");
         }
      }
      
      if (argData.isFlagSet("namespace"))
      {
         MString val;
         status = argData.getFlagArgument("namespace", 0, val);
         
         if (status == MS::kSuccess)
         {
            if ((!MNamespace::namespaceExists(val) &&
                  MNamespace::addNamespace(val) != MS::kSuccess) ||
                MNamespace::setCurrentNamespace(val) != MS::kSuccess)
            {
               MGlobal::displayWarning(val + " is not a valid namespace");
            }
            else
            {
               ns = val;
            }
         }
         else
         {
            MGlobal::displayWarning("Invalid namespace flag argument");
         }   
      }
      
      if (argData.isFlagSet("filterObjects"))
      {
         MString val;
         status = argData.getFlagArgument("filterObjects", 0, val);
         
         if (status == MS::kSuccess)
         {
            includeFilter = val;
         }
         else
         {
            MGlobal::displayWarning("Invalid filterObjects flag argument");
         }
      }
      
      if (argData.isFlagSet("excludeFilterObjects"))
      {
         MString val;
         status = argData.getFlagArgument("excludeFilterObjects", 0, val);
         
         if (status == MS::kSuccess)
         {
            excludeFilter = val;
         }
         else
         {
            MGlobal::displayWarning("Invalid excludeFilterObjects flag argument");
         }
      }
      
      status = argData.getCommandArgument(0, filename);
      MStringArray result;
      
      if (status == MS::kSuccess)
      {
         MFileObject file;
         file.setRawFullName(filename);
         
         if (!file.exists())
         {
            MGlobal::displayError("Invalid file path: " + filename);
            status = MS::kFailure;
         }
         else
         {
            std::string abcPath = file.resolvedFullName().asChar();
            
            AlembicSceneFilter filter(includeFilter.asChar(), excludeFilter.asChar());
            
            AlembicScene *scene = AlembicSceneCache::Ref(abcPath, filter);
            if (scene)
            {
               // scene->setFilters(includeFilter.asChar(), excludeFilter.asChar());
               
               CreateTree visitor(abcPath, dm, ignoreTransforms, createInstances, speed, offset, preserveStartFrame, ct);
               scene->visit(AlembicNode::VisitDepthFirst, visitor);
               
               visitor.keyTransforms(rotInterp, nodeRotInterp);
               
               if (parentDag.isValid())
               {
                  MFnDagNode parentNode(parentDag);
                  
                  for (size_t i=0; i<scene->childCount(); ++i)
                  {
                     MDagPath rootDag = visitor.getDag(scene->child(i)->path());
                     if (rootDag.isValid())
                     {
                        MObject rootObj = rootDag.node();
                        parentNode.addChild(rootObj);
                        
                        result.append(rootDag.fullPathName());
                     }
                  }
               }
               
               if (fitTimeRange || setToStartFrame)
               {
                  double start, end;
                  
                  GetFrameRange visitor(false, false, false);
                  scene->visit(AlembicNode::VisitDepthFirst, visitor);
                  
                  if (visitor.getFrameRange(start, end))
                  {
                     MTime startFrame(start, MTime::kSeconds);
                     MTime endFrame(end, MTime::kSeconds);
                     
                     if (fitTimeRange)
                     {
                        MAnimControl::setAnimationStartEndTime(startFrame, endFrame);
                        MAnimControl::setMinMaxTime(startFrame, endFrame);
                     }
                     if (setToStartFrame)
                     {
                        MAnimControl::setCurrentTime(startFrame);
                     }
                  }
               }
               
               if (result.length() == 0)
               {
                  for (size_t i=0; i<scene->childCount(); ++i)
                  {
                     MDagPath rootDag = visitor.getDag(scene->child(i)->path());
                     if (rootDag.isValid())
                     {
                        result.append(rootDag.fullPathName());
                     }
                  }
               }
               
               if (scene && !AlembicSceneCache::Unref(scene))
               {
                  delete scene;
               }
            }
            else
            {
               MGlobal::displayError("Could not read scene from file: " + filename);
               status = MS::kFailure;
            }
         }
      }
      
      if (ns.length() > 0)
      {
         MNamespace::setCurrentNamespace(curNs);
      }
      
      setResult(result);
   }
   
   return status;
}

