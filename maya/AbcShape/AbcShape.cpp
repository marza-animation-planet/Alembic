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

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#include <vector>
#include <map>


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

void* AbcShape::creator()
{
   return new AbcShape();
}

void AbcShape::createdCallback(MObject& node, void*)
{
   MFnDependencyNode dn(node);
   MPlug plug;
   
   plug = dn.findPlug("visibleInReflections");
   if (!plug.isNull())
   {
      plug.setBool(true);
   }
   
   plug = dn.findPlug("visibleInRefractions");
   if (!plug.isNull())
   {
      plug.setBool(true);
   }
   
   plug = dn.findPlug("time");
   if (!plug.isNull())
   {
      MSelectionList sl;
      MObject obj;
      
      sl.add("time1");
      
      if (sl.getDependNode(0, obj) == MStatus::kSuccess)
      {
         MFnDependencyNode timeNode(obj);
         MPlug srcPlug = timeNode.findPlug("outTime");
         
         if (!srcPlug.isNull())
         {
            MDGModifier mod;
            
            mod.connect(srcPlug, plug);
            mod.doIt();
         }
      }
   }
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
   
   attributeAffects(aFilePath, aNumShapes);
   attributeAffects(aObjectExpression, aNumShapes);
   attributeAffects(aIgnoreInstances, aNumShapes);
   attributeAffects(aIgnoreVisibility, aNumShapes);
   
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
   
   AbcShape::CallbackID = MDGMessage::addNodeAddedCallback(AbcShape::createdCallback, PREFIX_NAME("AbcShape"));
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
   case UL_scene:
      updateScene();
      break;
   case UL_objects:
      if (mScene) updateObjects();
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

void AbcShape::updateScene()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update scene: " << mFilePath.asChar() << std::endl;
   #endif
   if (mScene && !AlembicSceneCache::Unref(mScene))
   {
      delete mScene;
   }
   
   mGeometry.clear();
   
   mScene = AlembicSceneCache::Ref(mFilePath.asChar());
   
   if (mScene)
   {
      GetFrameRange visitor;
      mScene->visit(AlembicNode::VisitDepthFirst, visitor);
      
      double start, end;
      
      if (visitor.getFrameRange(start, end))
      {
         double fps = getFPS();
         start *= fps;
         end *= fps;
         
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
      
      updateObjects();
   }
   else
   {
      mNumShapes = 0;
   }
}

void AbcShape::updateObjects()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Filter objects: \"" << mObjectExpression.asChar() << "\"" << std::endl;
   #endif
   
   mGeometry.clear();
   
   mSceneFilter.set(mObjectExpression.asChar(), "");
   
   AlembicScene *scene = AlembicSceneCache::Ref(mFilePath.asChar(), mSceneFilter);
   
   if (mScene && !AlembicSceneCache::Unref(mScene))
   {
      delete mScene;
   }
   
   mScene = scene;
   
   if (mScene)
   {
      updateWorld();
   }
   else
   {
      #ifdef _DEBUG
      std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update objects: failed to create filtered scene" << std::endl;
      #endif
   }
}

void AbcShape::updateWorld()
{
   #ifdef _DEBUG
   std::cout << "[" << PREFIX_NAME("AbcShape") << "] Update world" << std::endl;
   #endif
   
   WorldUpdate visitor(mSampleTime, mIgnoreTransforms, mIgnoreInstances, mIgnoreVisibility);
   mScene->visit(AlembicNode::VisitDepthFirst, visitor);
   
   mNumShapes = visitor.numShapes();
   
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
      MFileObject file;
      file.setRawFullName(handle.asString());
      
      MString filePath = file.resolvedFullName();
      
      if (filePath != mFilePath)
      {
         mFilePath = filePath;
         mUpdateLevel = UL_scene;
      }
      
      return true;
   }
   else if (plug == aObjectExpression)
   {
      MString objectExpression = handle.asString();
      
      if (objectExpression != mObjectExpression)
      {
         mObjectExpression = objectExpression;
         if (mScene)
         {
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_objects);
         }
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
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_world);
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
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_world);
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
            mUpdateLevel = std::max<int>(mUpdateLevel, UL_world);
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
         sampleTimeUpdate = true;
      }
   }
   else if (plug == aSpeed)
   {
      double speed = handle.asDouble();
      
      if (fabs(speed - mSpeed) > 0.0001)
      {
         mSpeed = speed;
         sampleTimeUpdate = true;
      }
   }
   else if (plug == aPreserveStartFrame)
   {
      bool psf = handle.asBool();
      
      if (psf != mPreserveStartFrame)
      {
         mPreserveStartFrame = psf;
         sampleTimeUpdate = true;
      }
   }
   else if (plug == aOffset)
   {
      double offset = handle.asDouble();
      
      if (fabs(offset - mOffset) > 0.0001)
      {
         mOffset = offset;
         sampleTimeUpdate = true;
      }
   }
   else if (plug == aCycleType)
   {
      CycleType c = (CycleType) handle.asShort();
      
      if (c != mCycleType)
      {
         mCycleType = c;
         sampleTimeUpdate = true;
      }
   }
   else if (plug == aStartFrame)
   {
      double sf = handle.asDouble();
      
      if (fabs(sf - mStartFrame) > 0.0001)
      {
         mStartFrame = sf;
         sampleTimeUpdate = true;
      }
   }
   else if (plug == aEndFrame)
   {
      double ef = handle.asDouble();
      
      if (fabs(ef - mEndFrame) > 0.0001)
      {
         mEndFrame = ef;
         sampleTimeUpdate = true;
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
      
      if (mScene && !AlembicSceneCache::Unref(mScene))
      {
         delete mScene;
      }
      mScene = 0;
      mSceneFilter.reset();
      mNumShapes = 0;
      mGeometry.clear();
      mUpdateLevel = UL_scene;
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
