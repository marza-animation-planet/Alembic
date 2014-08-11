#include "AbcShapeImport.h"
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
#include "AbcShape.h"
#include "SceneCache.h"
#include "AlembicSceneVisitors.h"
#include "Keyframer.h"

MSyntax AbcShapeImport::createSyntax()
{
   MSyntax syntax;
   
   syntax.addFlag("-m", "-mode", MSyntax::kString);
   syntax.addFlag("-n", "-namespace", MSyntax::kString);
   syntax.addFlag("-r", "-reparent", MSyntax::kString);
   syntax.addFlag("-ftr", "-fitTimeRange", MSyntax::kNoArg);
   syntax.addFlag("-sts", "-setToStartFrame", MSyntax::kNoArg);
   syntax.addFlag("-ci", "-createInstances", MSyntax::kNoArg);
   syntax.addFlag("-s", "-speed", MSyntax::kDouble);
   syntax.addFlag("-o", "-offset", MSyntax::kDouble);
   syntax.addFlag("-psf", "-preserveStartFrame", MSyntax::kBoolean);
   syntax.addFlag("-ct", "-cycleType", MSyntax::kString);
   syntax.addFlag("-ri", "-rotationInterpolation", MSyntax::kString);
   syntax.addFlag("-ft", "-filterObjects", MSyntax::kString);
   syntax.addFlag("-h", "-help", MSyntax::kNoArg);
   
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

template <typename T, int D, typename TT>
struct NumericData
{
   static void Set(const T* value, MPlug &plug)
   {
   }
};

template <typename T, typename TT>
struct NumericData<T, 1, TT>
{
   static void Set(const T* value, MPlug &plug)
   {
      plug.setValue(TT(value[0]));
   }
};

template <typename T, typename TT>
struct NumericData<T, 2, TT>
{
   static void Set(const T* value, MPlug &plug)
   {
      MFnNumericData data(plug.asMObject());
      data.setData(TT(value[0]), TT(value[1]));
   }
};

template <typename T, typename TT>
struct NumericData<T, 3, TT>
{
   static void Set(const T* value, MPlug &plug)
   {
      MFnNumericData data(plug.asMObject());
      data.setData(TT(value[0]), TT(value[1]), TT(value[2]));
   }
};

template <typename T, typename TT>
struct NumericData<T, 4, TT>
{
   static void Set(const T* value, MPlug &plug)
   {
      MFnNumericData data(plug.asMObject());
      data.setData(TT(value[0]), TT(value[1]), TT(value[2]), TT(value[3]));
   }
};

// ---

class CreateTree
{
public:
   
   CreateTree(const std::string &abcPath,
              AbcShape::DisplayMode mode,
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
   
   void keyTransforms(const MString &rotationInterpolation);
   
private:

   template <class T>
   AlembicNode::VisitReturn enterShape(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
   bool hasDag(const std::string &path) const;
   bool addDag(const std::string &path, const MDagPath &dag);
   bool createDag(const char *dagType, AlembicNode *node);
   
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
   void setNumericArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   template <typename T, int D, typename TT>
   void setCompoundArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   template <typename T>
   void setMatrixArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   void setStringArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug);
   
   template <typename T, int D>
   void keyAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug);
   
   template <typename T, int D, typename TT>
   void setNumericUserProp(MFnDagNode &node, const std::string &name, MFnNumericData::Type type, Alembic::Abc::IScalarProperty prop);
   template <typename T, int D>
   void setPointUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   template <typename T, int D>
   void setColorUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   template <typename T>
   void setMatrixUserProp(MFnDagNode &node, const std::string &name, MFnMatrixAttribute::Type type, Alembic::Abc::IScalarProperty prop);
   void setStringUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IScalarProperty prop);
   
   template <typename T, int D, typename TT>
   void setNumericArrayUserProp(MFnDagNode &node, const std::string &name, MFnNumericData::Type type, Alembic::Abc::IArrayProperty prop);
   template <typename T, int D>
   void setPointArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   template <typename T, int D>
   void setColorArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   template <typename T>
   void setMatrixArrayUserProp(MFnDagNode &node, const std::string &name, MFnMatrixAttribute::Type type, Alembic::Abc::IArrayProperty prop);
   void setStringArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop);
   
private:
   
   std::string mAbcPath;
   AbcShape::DisplayMode mMode;
   bool mCreateInstances;
   double mSpeed;
   double mOffset;
   bool mPreserveStartFrame;
   AbcShape::CycleType mCycleType;
   std::map<std::string, MDagPath> mCreatedDags;
   Keyframer mKeyframer;
};

CreateTree::CreateTree(const std::string &abcPath,
                       AbcShape::DisplayMode mode,
                       bool createInstances,
                       double speed,
                       double offset,
                       bool preserveStartFrame,
                       AbcShape::CycleType cycleType)
   : mAbcPath(abcPath)
   , mMode(mode)
   , mCreateInstances(createInstances)
   , mSpeed(speed)
   , mOffset(offset)
   , mPreserveStartFrame(preserveStartFrame)
   , mCycleType(cycleType)
{
}

bool CreateTree::hasDag(const std::string &path) const
{
   return (mCreatedDags.find(path) != mCreatedDags.end());
}

const MDagPath& CreateTree::getDag(const std::string &path) const
{
   static MDagPath sInvalidDag;
   std::map<std::string, MDagPath>::const_iterator it = mCreatedDags.find(path);
   return (it != mCreatedDags.end() ? it->second : sInvalidDag);
}

bool CreateTree::addDag(const std::string &path, const MDagPath &dag)
{
   if (hasDag(path))
   {
      return false;
   }
   
   mCreatedDags[path] = dag;
   
   return true;
}

bool CreateTree::createDag(const char *dagType, AlembicNode *node)
{
   if (!dagType || !node)
   {
      return false;
   }
   else if (!hasDag(node->path()))
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
         MDagPath dag;
         
         MDagPath::getAPathTo(obj, dag);
         
         dagmod.renameNode(obj, node->name().c_str());
         dagmod.doIt();
         
         return addDag(node->path(), dag);
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
   
   if (!createDag("AbcShape", target))
   {
      return AlembicNode::StopVisit;
   }
   
   // Set AbcShape (the order in which attributes are set is though as to have the lowest possible update cost)
   MPlug plug;
   MFnDagNode dagNode(getDag(target->path()));
   
   plug = dagNode.findPlug("ignoreXforms");
   plug.setBool(true);
   
   plug = dagNode.findPlug("ignoreInstances");
   plug.setBool(true);
   
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
   plug.setString(node.path().c_str());
   
   plug = dagNode.findPlug("filePath");
   plug.setString(mAbcPath.c_str());
   
   plug = dagNode.findPlug("displayMode");
   plug.setShort(short(mMode));
   
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

AlembicNode::VisitReturn CreateTree::enter(AlembicXform &node, AlembicNode *instance)
{
   AlembicNode *target = (instance ? instance : &node);
   
   if (node.isLocator())
   {
      if (!createDag("locator", target))
      {
         return AlembicNode::StopVisit;
      }
      
      Alembic::Abc::IScalarProperty loc = node.locatorProperty();
      double posscl[6] = {0, 0, 0, 1, 1, 1};
      
      loc.get(posscl);
      
      MFnDagNode locNode(getDag(target->path()));
      MObject locObj = locNode.object();
      
      MPlug pPx = locNode.findPlug("localPositionX");
      MPlug pPy = locNode.findPlug("localPositionY");
      MPlug pPz = locNode.findPlug("localPositionZ");
      MPlug pSx = locNode.findPlug("localScaleX");
      MPlug pSy = locNode.findPlug("localScaleY");
      MPlug pSz = locNode.findPlug("localScaleZ");
      
      pPx.setDouble(posscl[0]);
      pPy.setDouble(posscl[1]);
      pPz.setDouble(posscl[2]);
      pSx.setDouble(posscl[3]);
      pSy.setDouble(posscl[4]);
      pSz.setDouble(posscl[5]);
      
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
            
            mKeyframer.addAnyKey(locObj, "localPositionX", 0, posscl[0]);
            mKeyframer.addAnyKey(locObj, "localPositionY", 0, posscl[1]);
            mKeyframer.addAnyKey(locObj, "localPositionZ", 0, posscl[2]);
            mKeyframer.addAnyKey(locObj, "localScaleX", 0, posscl[3]);
            mKeyframer.addAnyKey(locObj, "localScaleY", 0, posscl[4]);
            mKeyframer.addAnyKey(locObj, "localScaleZ", 0, posscl[5]);
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
      if (!createDag("transform", target))
      {
         return AlembicNode::StopVisit;
      }
      
      Alembic::AbcGeom::IXformSchema schema = node.typedObject().getSchema();
      
      // key xform attributes, visibilty and inheritsTransform
      
      MFnTransform xform(getDag(target->path()));
      MObject xformObj = xform.object();
      
      MPlug pIT = xform.findPlug("inheritsTransform");
      
      // Get default sample and set node state
      Alembic::AbcGeom::XformSample sample = schema.getValue();
      
      MTransformationMatrix mmat(MMatrix(sample.getMatrix().x));
      xform.set(mmat);
      
      bool inheritsXforms = sample.getInheritsXforms();
      pIT.setBool(inheritsXforms);
      
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
            
            mKeyframer.setCurrentTime(t);
            mKeyframer.addTransformKey(xformObj, mmat);
            mKeyframer.addInheritsTransformKey(xformObj, sample.getInheritsXforms());
         }
         
         mKeyframer.addCurvesImportInfo(xformObj, "", mSpeed, secOffset, secStart, secEnd,
                                        (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
      }
      
      Alembic::Abc::ICompoundProperty props = node.object().getProperties();
   
      if (props.valid())
      {
         const Alembic::Abc::PropertyHeader *header = props.getPropertyHeader("visible");
         
         if (header)
         {
            if (Alembic::Abc::ICharProperty::matches(*header))
            {
               MPlug pVis = xform.findPlug("visibility");
               
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
                     mKeyframer.addVisibilityKey(xformObj, v != 0);
                  }
                  
                  mKeyframer.addCurvesImportInfo(xformObj, "visibility", mSpeed, secOffset, secStart, secEnd,
                                                 (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
               }
            }
            else if (Alembic::Abc::IBoolProperty::matches(*header))
            {
               MPlug pVis = xform.findPlug("visibility");
               
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
                     mKeyframer.addVisibilityKey(xformObj, v);
                  }
                  
                  mKeyframer.addCurvesImportInfo(xformObj, "visibility", mSpeed, secOffset, secStart, secEnd,
                                                 (mCycleType == AbcShape::CT_reverse), mPreserveStartFrame);
               }
            }
         }
      }
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
      
      MObject aattr = nAttr.create(aname+"A", aname+"A", MFnNumericData::kFloat, 0.0);
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
void CreateTree::keyAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug)
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
         
         prop.get(val, Alembic::Abc::ISampleSelector(idx));
         
         mKeyframer.setCurrentTime(t);
         for (int d=0; d<D; ++d)
         {
            mKeyframer.addAnyKey(nodeObj, plugName, d, val[d]);
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
   T val[D];
   
   prop.get(val);
   
   NumericData<T, D, TT>::Set(val, plug);
   
   keyAttribute<T, D>(prop, plug);
}

template <typename T, int D, typename TT>
void CreateTree::setNumericArrayAttribute(Alembic::Abc::IArrayProperty prop, MPlug &plug)
{
   Alembic::AbcCoreAbstract::ArraySamplePtr samp;
   prop.get(samp);
   
   const T *val = (const T*) samp->getData();
   
   for (size_t i=0; i<samp->size(); ++i, val+=D)
   {
      MPlug elem = plug.elementByLogicalIndex(i);
      NumericData<T, D, TT>::Set(val, elem);
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
   
   MFnMatrixData data(plug.asMObject());
   data.set(m);
   
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
         for (int c=0; c<4; ++c, ++i)
         {
            m[r][c] = double(val[j]);
         }
      }
      
      MFnMatrixData data(plug.elementByLogicalIndex(i).asMObject());
      data.set(m);
   }
}

void CreateTree::setStringAttribute(Alembic::Abc::IScalarProperty prop, MPlug &plug)
{
   std::string val;
   
   prop.get(&val);
   
   plug.setString(val.c_str());
   
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
void CreateTree::setNumericUserProp(MFnDagNode &node, const std::string &name, MFnNumericData::Type type, Alembic::Abc::IScalarProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createNumericAttribute(node, name, type, false, plug)
                                 : checkNumericAttribute(plug, type, false));
   
   if (process)
   {
      setNumericAttribute<T, D, TT>(prop, plug);
   }
}

template <typename T, int D, typename TT>
void CreateTree::setNumericArrayUserProp(MFnDagNode &node, const std::string &name, MFnNumericData::Type type, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createNumericAttribute(node, name, type, true, plug)
                                 : checkNumericAttribute(plug, type, true));
   
   if (process)
   {
      setNumericArrayAttribute<T, D, TT>(prop, plug);
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
}

template <typename T, int D>
void CreateTree::setPointArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createPointAttribute(node, name, true, plug)
                                 : checkPointAttribute(plug, true));
   
   if (process)
   {
      setNumericArrayAttribute<T, D, float>(prop, plug);
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
      setNumericAttribute<T, D, float>(prop, plug);
   }
}

template <typename T, int D>
void CreateTree::setColorArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool alpha = (D == 4);
   
   bool process = (plug.isNull() ? createColorAttribute(node, name, alpha, true, plug)
                                 : checkColorAttribute(plug, alpha, true));
   
   if (process)
   {
      setNumericArrayAttribute<T, D, float>(prop, plug);
   }
}

template <typename T>
void CreateTree::setMatrixUserProp(MFnDagNode &node, const std::string &name, MFnMatrixAttribute::Type type, Alembic::Abc::IScalarProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createMatrixAttribute(node, name, type, false, plug)
                                 : checkMatrixAttribute(plug, false));
   
   if (process)
   {
      setMatrixAttribute<T>(prop, plug);
   }
}

template <typename T>
void CreateTree::setMatrixArrayUserProp(MFnDagNode &node, const std::string &name, MFnMatrixAttribute::Type type, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createMatrixAttribute(node, name, type, true, plug)
                                 : checkMatrixAttribute(plug, true));
   
   if (process)
   {
      setMatrixArrayAttribute<T>(prop, plug);
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
}

void CreateTree::setStringArrayUserProp(MFnDagNode &node, const std::string &name, Alembic::Abc::IArrayProperty prop)
{
   MPlug plug = node.findPlug(name.c_str());
   
   bool process = (plug.isNull() ? createStringAttribute(node, name, true, plug)
                                 : checkStringAttribute(plug, true));
   
   if (process)
   {
      setStringArrayAttribute(prop, plug);
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
      
      if (header.isScalar())
      {
         Alembic::Abc::IScalarProperty prop(props, propName);
         
         if (Alembic::Abc::IBoolProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::bool_t, 1, bool>(target, propName, MFnNumericData::kBoolean, prop);
         }
         else if (Alembic::Abc::IUcharProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::uint8_t, 1, char>(target, propName, MFnNumericData::kChar, prop);
         }
         else if (Alembic::Abc::ICharProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int8_t, 1, char>(target, propName, MFnNumericData::kChar, prop);  
         }
         else if (Alembic::Abc::IUInt16Property::matches(header))
         {
            setNumericUserProp<Alembic::Util::uint16_t, 1, short>(target, propName, MFnNumericData::kShort, prop);
         }
         else if (Alembic::Abc::IInt16Property::matches(header))
         {
            setNumericUserProp<Alembic::Util::int16_t, 1, short>(target, propName, MFnNumericData::kShort, prop);  
         }
         else if (Alembic::Abc::IUInt32Property::matches(header))
         {
            setNumericUserProp<Alembic::Util::uint32_t, 1, int>(target, propName, MFnNumericData::kLong, prop);
         }
         else if (Alembic::Abc::IInt32Property::matches(header))
         {
            setNumericUserProp<Alembic::Util::int32_t, 1, int>(target, propName, MFnNumericData::kLong, prop);  
         }
         else if (Alembic::Abc::IUInt64Property::matches(header))
         {
            setNumericUserProp<Alembic::Util::uint64_t, 1, int>(target, propName, MFnNumericData::kLong, prop);
         }
         else if (Alembic::Abc::IInt64Property::matches(header))
         {
            setNumericUserProp<Alembic::Util::int64_t, 1, int>(target, propName, MFnNumericData::kLong, prop);  
         }
         else if (Alembic::Abc::IHalfProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::float16_t, 1, float>(target, propName, MFnNumericData::kFloat, prop);  
         }
         else if (Alembic::Abc::IFloatProperty::matches(header))
         {
            setNumericUserProp<float, 1, float>(target, propName, MFnNumericData::kFloat, prop);  
         }
         else if (Alembic::Abc::IDoubleProperty::matches(header))
         {
            setNumericUserProp<double, 1, double>(target, propName, MFnNumericData::kDouble, prop);  
         }
         else if (Alembic::Abc::IV2sProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int16_t, 2, short>(target, propName, MFnNumericData::k2Short, prop);
         }
         else if (Alembic::Abc::IV2iProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int32_t, 2, int>(target, propName, MFnNumericData::k2Long, prop);
         }
         else if (Alembic::Abc::IV2fProperty::matches(header))
         {
            setNumericUserProp<float, 2, float>(target, propName, MFnNumericData::k2Float, prop);
         }
         else if (Alembic::Abc::IV2dProperty::matches(header))
         {
            setNumericUserProp<double, 2, double>(target, propName, MFnNumericData::k2Double, prop);
         }
         else if (Alembic::Abc::IV3sProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int16_t, 3, short>(target, propName, MFnNumericData::k3Short, prop);
         }
         else if (Alembic::Abc::IV3iProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int32_t, 3, int>(target, propName, MFnNumericData::k3Long, prop);
         }
         else if (Alembic::Abc::IV3fProperty::matches(header))
         {
            setPointUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IV3dProperty::matches(header))
         {
            setPointUserProp<double, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IP2sProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int16_t, 2, short>(target, propName, MFnNumericData::k2Short, prop);
         }
         else if (Alembic::Abc::IP2iProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int32_t, 2, short>(target, propName, MFnNumericData::k2Long, prop);
         }
         else if (Alembic::Abc::IP2fProperty::matches(header))
         {
            setNumericUserProp<float, 2, float>(target, propName, MFnNumericData::k2Float, prop);
         }
         else if (Alembic::Abc::IP2dProperty::matches(header))
         {
            setNumericUserProp<double, 2, double>(target, propName, MFnNumericData::k2Double, prop);
         }
         else if (Alembic::Abc::IP3sProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int16_t, 3, short>(target, propName, MFnNumericData::k3Short, prop);
         }
         else if (Alembic::Abc::IP3iProperty::matches(header))
         {
            setNumericUserProp<Alembic::Util::int32_t, 3, int>(target, propName, MFnNumericData::k3Long, prop);
         }
         else if (Alembic::Abc::IP3fProperty::matches(header))
         {
            setPointUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IP3dProperty::matches(header))
         {
            setPointUserProp<double, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IN2fProperty::matches(header))
         {
            setNumericUserProp<float, 2, float>(target, propName, MFnNumericData::k2Float, prop);
         }
         else if (Alembic::Abc::IN2dProperty::matches(header))
         {
            setNumericUserProp<double, 2, double>(target, propName, MFnNumericData::k2Double, prop);
         }
         else if (Alembic::Abc::IN3fProperty::matches(header))
         {
            setPointUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IN3dProperty::matches(header))
         {
            setPointUserProp<double, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IC3hProperty::matches(header))
         {
            setColorUserProp<Alembic::Util::float16_t, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IC3fProperty::matches(header))
         {
            setColorUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IC4hProperty::matches(header))
         {
            setColorUserProp<Alembic::Util::float16_t, 4>(target, propName, prop);
         }
         else if (Alembic::Abc::IC4fProperty::matches(header))
         {
            setColorUserProp<float, 4>(target, propName, prop);
         }
         else if (Alembic::Abc::IQuatfProperty::matches(header))
         {
            setNumericUserProp<float, 4, double>(target, propName, MFnNumericData::k4Double, prop);
         }
         else if (Alembic::Abc::IQuatdProperty::matches(header))
         {
            setNumericUserProp<double, 4, double>(target, propName, MFnNumericData::k4Double, prop);
         }
         else if (Alembic::Abc::IM44fProperty::matches(header))
         {
            setMatrixUserProp<float>(target, propName, MFnMatrixAttribute::kFloat, prop);
         }
         else if (Alembic::Abc::IM44dProperty::matches(header))
         {
            setMatrixUserProp<double>(target, propName, MFnMatrixAttribute::kDouble, prop);
         }
         else if (Alembic::Abc::IStringProperty::matches(header))
         {
            setStringUserProp(target, propName, prop);
         }
         // Ignored types:
         //   Box2*
         //   Box3*
         //   M33*
         //   C3c
         //   C4c
         //   Wstring
      }
      else if (header.isArray())
      {
         Alembic::Abc::IArrayProperty prop(props, propName);
         
         if (Alembic::Abc::IBoolArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::bool_t, 1, bool>(target, propName, MFnNumericData::kBoolean, prop);
         }
         else if (Alembic::Abc::IUcharArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::uint8_t, 1, char>(target, propName, MFnNumericData::kChar, prop);
         }
         else if (Alembic::Abc::ICharArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int8_t, 1, char>(target, propName, MFnNumericData::kChar, prop);  
         }
         else if (Alembic::Abc::IUInt16ArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::uint16_t, 1, short>(target, propName, MFnNumericData::kShort, prop);
         }
         else if (Alembic::Abc::IInt16ArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int16_t, 1, short>(target, propName, MFnNumericData::kShort, prop);  
         }
         else if (Alembic::Abc::IUInt32ArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::uint32_t, 1, int>(target, propName, MFnNumericData::kLong, prop);
         }
         else if (Alembic::Abc::IInt32ArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int32_t, 1, int>(target, propName, MFnNumericData::kLong, prop);  
         }
         else if (Alembic::Abc::IUInt64ArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::uint64_t, 1, int>(target, propName, MFnNumericData::kLong, prop);
         }
         else if (Alembic::Abc::IInt64ArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int64_t, 1, int>(target, propName, MFnNumericData::kLong, prop);  
         }
         else if (Alembic::Abc::IHalfArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::float16_t, 1, float>(target, propName, MFnNumericData::kFloat, prop);  
         }
         else if (Alembic::Abc::IFloatArrayProperty::matches(header))
         {
            setNumericArrayUserProp<float, 1, float>(target, propName, MFnNumericData::kFloat, prop);  
         }
         else if (Alembic::Abc::IDoubleArrayProperty::matches(header))
         {
            setNumericArrayUserProp<double, 1, double>(target, propName, MFnNumericData::kDouble, prop);  
         }
         else if (Alembic::Abc::IV2sArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int16_t, 2, short>(target, propName, MFnNumericData::k2Short, prop);
         }
         else if (Alembic::Abc::IV2iArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int32_t, 2, int>(target, propName, MFnNumericData::k2Long, prop);
         }
         else if (Alembic::Abc::IV2fArrayProperty::matches(header))
         {
            setNumericArrayUserProp<float, 2, float>(target, propName, MFnNumericData::k2Float, prop);
         }
         else if (Alembic::Abc::IV2dArrayProperty::matches(header))
         {
            setNumericArrayUserProp<double, 2, double>(target, propName, MFnNumericData::k2Double, prop);
         }
         else if (Alembic::Abc::IV3sArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int16_t, 3, short>(target, propName, MFnNumericData::k3Short, prop);
         }
         else if (Alembic::Abc::IV3iArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int32_t, 3, int>(target, propName, MFnNumericData::k3Long, prop);
         }
         else if (Alembic::Abc::IV3fArrayProperty::matches(header))
         {
            setPointArrayUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IV3dArrayProperty::matches(header))
         {
            setPointArrayUserProp<double, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IP2sArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int16_t, 2, short>(target, propName, MFnNumericData::k2Short, prop);
         }
         else if (Alembic::Abc::IP2iArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int32_t, 2, short>(target, propName, MFnNumericData::k2Long, prop);
         }
         else if (Alembic::Abc::IP2fArrayProperty::matches(header))
         {
            setNumericArrayUserProp<float, 2, float>(target, propName, MFnNumericData::k2Float, prop);
         }
         else if (Alembic::Abc::IP2dArrayProperty::matches(header))
         {
            setNumericArrayUserProp<double, 2, double>(target, propName, MFnNumericData::k2Double, prop);
         }
         else if (Alembic::Abc::IP3sArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int16_t, 3, short>(target, propName, MFnNumericData::k3Short, prop);
         }
         else if (Alembic::Abc::IP3iArrayProperty::matches(header))
         {
            setNumericArrayUserProp<Alembic::Util::int32_t, 3, int>(target, propName, MFnNumericData::k3Long, prop);
         }
         else if (Alembic::Abc::IP3fArrayProperty::matches(header))
         {
            setPointArrayUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IP3dArrayProperty::matches(header))
         {
            setPointArrayUserProp<double, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IN2fArrayProperty::matches(header))
         {
            setNumericArrayUserProp<float, 2, float>(target, propName, MFnNumericData::k2Float, prop);
         }
         else if (Alembic::Abc::IN2dArrayProperty::matches(header))
         {
            setNumericArrayUserProp<double, 2, double>(target, propName, MFnNumericData::k2Double, prop);
         }
         else if (Alembic::Abc::IN3fArrayProperty::matches(header))
         {
            setPointArrayUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IN3dArrayProperty::matches(header))
         {
            setPointArrayUserProp<double, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IC3hArrayProperty::matches(header))
         {
            setColorArrayUserProp<Alembic::Util::float16_t, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IC3fArrayProperty::matches(header))
         {
            setColorArrayUserProp<float, 3>(target, propName, prop);
         }
         else if (Alembic::Abc::IC4hArrayProperty::matches(header))
         {
            setColorArrayUserProp<Alembic::Util::float16_t, 4>(target, propName, prop);
         }
         else if (Alembic::Abc::IC4fArrayProperty::matches(header))
         {
            setColorArrayUserProp<float, 4>(target, propName, prop);
         }
         else if (Alembic::Abc::IQuatfArrayProperty::matches(header))
         {
            setNumericArrayUserProp<float, 4, double>(target, propName, MFnNumericData::k4Double, prop);
         }
         else if (Alembic::Abc::IQuatdArrayProperty::matches(header))
         {
            setNumericArrayUserProp<double, 4, double>(target, propName, MFnNumericData::k4Double, prop);
         }
         else if (Alembic::Abc::IM44fArrayProperty::matches(header))
         {
            setMatrixArrayUserProp<float>(target, propName, MFnMatrixAttribute::kFloat, prop);
         }
         else if (Alembic::Abc::IM44dArrayProperty::matches(header))
         {
            setMatrixArrayUserProp<double>(target, propName, MFnMatrixAttribute::kDouble, prop);
         }
         else if (Alembic::Abc::IStringArrayProperty::matches(header))
         {
            setStringArrayUserProp(target, propName, prop);
         }
      }
   }
}

void CreateTree::leave(AlembicNode &, AlembicNode *)
{
}

void CreateTree::keyTransforms(const MString &rotationInterpolation)
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
   mKeyframer.setRotationCurvesInterpolation(rotationInterpolation);
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
   
   MString rotInterp("quaternion");
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
      MGlobal::displayInfo("AbcShapeImport [options] abc_file_path");
      MGlobal::displayInfo("Options:");
      MGlobal::displayInfo("-r / -reparent               : dagpath");
      MGlobal::displayInfo("                               Reparent the whole hierarchy under a node in the current Maya scene.");
      MGlobal::displayInfo("-n / -namespace              : string");
      MGlobal::displayInfo("                               Namespace to add nodes to (default to current namespace).");
      MGlobal::displayInfo("-m / -mode                   : string (box|boxes|points|geometry)");
      MGlobal::displayInfo("                               AbcShape nodes display mode (default to box).");
      MGlobal::displayInfo("-s / -speed                  : double");
      MGlobal::displayInfo("                               AbcShape nodes speed (default to 1.0).");
      MGlobal::displayInfo("-o / -offset                 : double");
      MGlobal::displayInfo("                               AbcShape nodes offset in frames (default to 0.0).");
      MGlobal::displayInfo("-psf / -preserveStartFrame   : bool");
      MGlobal::displayInfo("                               Preserve range start frame when using speed.");
      MGlobal::displayInfo("-ct / -cycleType             : string (hold|loop|reverse|bounce)");
      MGlobal::displayInfo("                               AbcShape nodes cycle type (default to hold).");
      MGlobal::displayInfo("-ci / -createInstances       :");
      MGlobal::displayInfo("                               Create maya instances.");
      MGlobal::displayInfo("-ri / -rotationInterpolation : string (none|euler|quaternion|quaternionSlerp|quaternionSquad)");
      MGlobal::displayInfo("                               Set created rotation curves interpolation type (default to quaternion).");
      MGlobal::displayInfo("-ftr / -fitTimeRange         :");
      MGlobal::displayInfo("                               Change Maya time slider to fit the range of input file.");
      MGlobal::displayInfo("-sts / -setToStartFrame      :");
      MGlobal::displayInfo("                               Set the current time to the start of the frame range.");
      MGlobal::displayInfo("-ft / -filterObjects         : string");
      MGlobal::displayInfo("                               Selective import cache objects whose name matches expression.");
      MGlobal::displayInfo("-h / -help                   :");
      MGlobal::displayInfo("                               Display this help.");
      MGlobal::displayInfo("");
      MGlobal::displayInfo("Command also work in edit mode:");
      MGlobal::displayInfo("  -mode, -speed, -offset, -preserveStartFrame, -cycleType, -rotationInterpolation flags are supported.");
      MGlobal::displayInfo("  Acts on all AbcShape in selected node trees.");
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
            MGlobal::displayWarning(val + " is not a valid rotationInterpolation value");
         }
      }
      else
      {
         MGlobal::displayWarning("Invalid rotationInterpolation flag argument");
      }
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
      bool processCurves = (setSpeed || setOffset || setCycle || setPreserveStart || setInterp);
      
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
         
         if (node.typeName() == "AbcShape")
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
                                     (setInterp ? &rotInterp : 0),
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
      MString filter("");
      MString curNs = MNamespace::currentNamespace();
      MDagPath parentDag;
      
      bool fitTimeRange = argData.isFlagSet("fitTimeRange");
      bool setToStartFrame = argData.isFlagSet("setToStartFrame");
      bool createInstances = argData.isFlagSet("createInstances");
      
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
            filter = val;
         }
         else
         {
            MGlobal::displayWarning("Invalid filterObjects flag argument");
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
            
            AlembicScene *scene = SceneCache::Ref(abcPath);
            if (scene)
            {
               scene->setFilter(filter.asChar());
               
               CreateTree visitor(abcPath, dm, createInstances, speed, offset, preserveStartFrame, ct);
               scene->visit(AlembicNode::VisitDepthFirst, visitor);
               
               visitor.keyTransforms(rotInterp);
               
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
                  
                  GetFrameRange visitor;
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
               
               SceneCache::Unref(scene);
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

