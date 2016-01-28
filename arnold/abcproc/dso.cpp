#include "dso.h"
#include "visitors.h"
#include "globallock.h"
#include <sstream>
#include <fstream>

const char* CycleTypeNames[] =
{
   "hold",
   "loop",
   "reverse",
   "bounce",
   "clip",
   NULL
};
   
const char* AttributeFrameNames[] =
{
   "render",
   "shutter",
   "shutter_open",
   "shutter_close",
   NULL
};

// ---

void Dso::CommonParameters::reset()
{
   filePath = "";
   referenceFilePath = "";
   namePrefix = "";
   objectPath = "";
   
   frame = 0.0;
   startFrame = std::numeric_limits<double>::max();
   endFrame = -std::numeric_limits<double>::max();
   relativeSamples = false;
   samples.clear();
   cycle = CT_hold;
   speed = 1.0;
   offset = 0.0;
   fps = 0.0;
   preserveStartFrame = false;
   
   ignoreDeformBlur = false;
   ignoreTransformBlur = false;
   ignoreVisibility = false;
   ignoreTransforms = false;
   ignoreInstances = false;
   
   samplesExpandIterations = 0;
   optimizeSamples = false;
   
   verbose = false;
   
   outputReference = false;
   
   promoteToObjectAttribs.clear();
}

std::string Dso::CommonParameters::dataString(const char *targetShape) const
{
   std::ostringstream oss;
   
   oss << std::fixed << std::setprecision(6);
   
   if (filePath.length() > 0)
   {
      oss << " -filename " << filePath;
   }
   if (outputReference)
   {
      oss << " -outputreference";
   }
   if (referenceFilePath.length() > 0)
   {
      oss << " -referencefilename " << referenceFilePath;
   }
   if (namePrefix.length() > 0)
   {
      oss << " -nameprefix " << namePrefix;
   }
   if (targetShape)
   {
      oss << " -objectpath " << targetShape; 
   }
   else if (objectPath.length() > 0)
   {
      oss << " -objectpath " << objectPath;
   }
   oss << " -frame " << frame;
   if (startFrame < endFrame)
   {
      oss << " -startframe " << startFrame << " -endframe " << endFrame;
   }
   if (relativeSamples)
   {
      oss << " -relativesamples";
   }
   if (samples.size() > 0)
   {
      oss << " -samples";
      for (size_t i=0; i<samples.size(); ++i)
      {
         oss << " " << samples[i];
      }
   }
   if (cycle >= CT_hold && cycle < CT_MAX)
   {
      oss << " -cycle " << CycleTypeNames[cycle];
   }
   oss << " -speed " << speed;
   oss << " -offset " << offset;
   oss << " -fps " << fps;
   if (preserveStartFrame)
   {
      oss << " -preservestartframe";
   }
   if (ignoreTransformBlur)
   {
      oss << " -ignoretransformblur";
   }
   if (ignoreDeformBlur)
   {
      oss << " -ignoredeformblur";
   }
   if (targetShape || ignoreTransforms)
   {
      oss << " -ignoretransforms";
   }
   if (targetShape || ignoreInstances)
   {
      oss << " -ignoreinstances";
   }
   if (targetShape || ignoreVisibility)
   {
      oss << " -ignorevisibility";
   }
   if (samplesExpandIterations > 0)
   {
      oss << " -samplesexpanditerations " << samplesExpandIterations;
      if (optimizeSamples)
      {
         oss << " -optimizesamples";
      }
   }
   else
   {
      oss << " -samplesexpanditerations 0";
   }
   if (promoteToObjectAttribs.size() > 0)
   {
      oss << " -promotetoobjectattribs";
      std::set<std::string>::const_iterator it = promoteToObjectAttribs.begin();
      while (it != promoteToObjectAttribs.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   if (verbose)
   {
      oss << " -verbose";
   }
   
   return oss.str();
}

std::string Dso::CommonParameters::shapeKey() const
{
   std::ostringstream oss;
   
   oss << std::fixed << std::setprecision(6);
   
   if (filePath.length() > 0)
   {
      oss << " -filename " << filePath;
   }
   if (outputReference)
   {
      oss << " -outputreference";
   }
   if (referenceFilePath.length() > 0)
   {
      oss << " -referencefilename " << referenceFilePath;
   }
   if (objectPath.length() > 0)
   {
      oss << " -objectpath " << objectPath;
   }
   oss << " -frame " << frame;
   if (startFrame < endFrame)
   {
      oss << " -startframe " << startFrame << " -endframe " << endFrame;
   }
   if (relativeSamples)
   {
      oss << " -relativesamples";
   }
   if (samples.size() > 0)
   {
      oss << " -samples";
      for (size_t i=0; i<samples.size(); ++i)
      {
         oss << " " << samples[i];
      }
   }
   if (cycle >= CT_hold && cycle < CT_MAX)
   {
      oss << " -cycle " << CycleTypeNames[cycle];
   }
   oss << " -speed " << speed;
   oss << " -offset " << offset;
   oss << " -fps " << fps;
   if (preserveStartFrame)
   {
      oss << " -preservestartframe";
   }
   if (ignoreDeformBlur)
   {
      oss << " -ignoredeformblur";
   }
   if (samplesExpandIterations > 0)
   {
      oss << " -samplesexpanditerations " << samplesExpandIterations;
      if (optimizeSamples)
      {
         oss << " -optimizesamples";
      }
   }
   else
   {
      oss << " -samplesexpanditerations 0";
   }
   
   return oss.str();
}

void Dso::MultiParameters::reset()
{
   overrideAttribs.clear();
   overrideBoundsMinName = "";
   overrideBoundsMaxName = "";
}

std::string Dso::MultiParameters::dataString() const
{
   std::ostringstream oss;
   
   if (overrideAttribs.size() > 0)
   {
      oss << " -overrideattribs";
      std::set<std::string>::const_iterator it = overrideAttribs.begin();
      while (it != overrideAttribs.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   
   if (overrideBoundsMinName.length() > 0)
   {
      oss << " -overrideBoundsMinName " << overrideBoundsMinName;
   }
   
   if (overrideBoundsMaxName.length() > 0)
   {
      oss << " -overrideBoundsMaxName " << overrideBoundsMaxName;
   }
   
   return oss.str();
}

void Dso::SingleParameters::reset()
{
   readObjectAttribs = false;
   readPrimitiveAttribs = false;
   readPointAttribs = false;
   readVertexAttribs = false;
   attribsFrame = AF_render;
   attribPreficesToRemove.clear();
   
   computeTangents.clear();
   
   radiusMin = 0.0f;
   radiusMax = 1000000.0f;
   radiusScale = 1.0f;
   
   nurbsSampleRate = 5;
}

std::string Dso::SingleParameters::dataString() const
{
   std::ostringstream oss;
   
   oss << std::fixed << std::setprecision(6);
   
   if (readObjectAttribs)
   {
      oss << " -objectattribs";
   }
   if (readPrimitiveAttribs)
   {
      oss << " -primitiveattribs";
   }
   if (readPointAttribs)
   {
      oss << " -pointattribs";
   }
   if (readVertexAttribs)
   {
      oss << " -vertexattribs";
   }
   if (attribsFrame >= AF_render && attribsFrame < AF_MAX)
   {
      oss << " -attribsframe " << AttributeFrameNames[attribsFrame];
   }
   if (attribPreficesToRemove.size() > 0)
   {
      oss << " -removeattribprefices";
      std::set<std::string>::const_iterator it = attribPreficesToRemove.begin();
      while (it != attribPreficesToRemove.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   if (computeTangents.size() > 0)
   {
      oss << " -computetangents";
      std::set<std::string>::const_iterator it = computeTangents.begin();
      while (it != computeTangents.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   oss << " -radiusmin " << radiusMin;
   oss << " -radiusmax " << radiusMax;
   oss << " -radiusscale " << radiusScale;
   oss << " -nurbssamplerate " << nurbsSampleRate;
   
   return oss.str();
}

std::string Dso::SingleParameters::shapeKey() const
{
   return dataString();
}
   
// ---

std::map<std::string, std::string> Dso::msMasterNodes;


Dso::Dso(AtNode *node)
   : mProcNode(node)
   , mMode(PM_undefined)
   , mScene(0)
   , mRefScene(0)
   , mRootDrive("")
   , mRenderTime(0.0)
   , mSetTimeSamples(false)
   , mStepSize(-1.0f)
   , mNumShapes(0)
   , mShapeKey("")
   , mInstanceNum(0)
   , mShutterOpen(std::numeric_limits<double>::max())
   , mShutterClose(-std::numeric_limits<double>::max())
   , mShutterStep(0)
   , mMotionStart(std::numeric_limits<double>::max())
   , mMotionEnd(-std::numeric_limits<double>::max())
   , mReverseWinding(true)
{
   mCommonParams.reset();
   mSingleParams.reset();
   mMultiParams.reset();
   
   readFromDataParam();
   
   // Override values from "data" string
   readFromUserParams();
   
   AtNode *opts = AiUniverseGetOptions();

   // Set framerate if needed
   if (mCommonParams.fps <= 0.0)
   {
      if (AiNodeLookUpUserParameter(opts, "fps") != 0)
      {
         double fps = AiNodeGetFlt(opts, "fps");
         if (fps > 0.0)
         {
            mCommonParams.fps = fps;
         }
      }
      
      if (mCommonParams.fps <= 0.0)
      {
         AiMsgWarning("[abcproc] Defaulting 'fps' to 24");
         mCommonParams.fps = 24.0;
      }
   }
   
   // Check frame and get from options node if needed? ('frame' user attribute)
   
   normalizeFilePath(mCommonParams.filePath);
   if (mCommonParams.referenceFilePath.length() == 0)
   {
      // look if we have a .ref file alongside the abc file path
      // this file if it exists, should contain the path to the 'reference' alembic file
      mCommonParams.referenceFilePath = getReferencePath(mCommonParams.filePath);
   }
   normalizeFilePath(mCommonParams.referenceFilePath);
   
   mReverseWinding = AiNodeGetBool(opts, "CCW_points");
   
   // Only acquire global lock when create new alembic scenes
   //
   // Alembic libs should be build with a threadsafe version of HDF5 that will handle
   //   multi-threaded reads by itself
   // Same with Ogawa backend
   {
      ScopeLock _lock;
      
      char id[64];
      sprintf(id, "%p", AiThreadSelf());
      
      AlembicSceneFilter filter(mCommonParams.objectPath, "");

      AlembicSceneCache::SetConcurrency(size_t(AiNodeGetInt(opts, "threads")));
      
      mScene = AlembicSceneCache::Ref(mCommonParams.filePath, id, filter, true);
   
      if (mCommonParams.referenceFilePath.length() > 0)
      {
         mRefScene = AlembicSceneCache::Ref(mCommonParams.referenceFilePath, id, filter, true);
      }
   }
   
   if (mScene)
   {
      // mScene->setFilter(mCommonParams.objectPath);
      
      if (mCommonParams.startFrame > mCommonParams.endFrame)
      {
         if (mCommonParams.verbose)
         {
            AiMsgInfo("[abcproc] Get frame range from ABC file");
         }
         
         GetTimeRange visitor;
         mScene->visit(AlembicNode::VisitDepthFirst, visitor);
         if (visitor.getRange(mCommonParams.startFrame, mCommonParams.endFrame))
         {
            mCommonParams.startFrame *= mCommonParams.fps;
            mCommonParams.endFrame *= mCommonParams.fps;
         }
         else
         {
            mCommonParams.startFrame = mCommonParams.frame;
            mCommonParams.endFrame = mCommonParams.frame;
         }
         
         if (mCommonParams.verbose)
         {
            AiMsgInfo("[abcproc]   %f - %f", mCommonParams.startFrame, mCommonParams.endFrame);
         }
      }
      
      bool exclude = false;
      
      mRenderTime = computeTime(mCommonParams.frame, &exclude);
      
      if (!exclude)
      {
         if (mCommonParams.verbose)
         {
            AiMsgInfo("[abcproc] Render at t = %f [%f, %f]",
                      mRenderTime,
                      mCommonParams.startFrame / mCommonParams.fps,
                      mCommonParams.endFrame / mCommonParams.fps);
         }
         
         CountShapes visitor(mRenderTime,
                             mCommonParams.ignoreTransforms,
                             mCommonParams.ignoreInstances,
                             mCommonParams.ignoreVisibility);
         
         mScene->visit(AlembicNode::VisitDepthFirst, visitor);
         
         if (mCommonParams.verbose)
         {
            AiMsgInfo("[abcproc] %lu shape(s) in scene", visitor.numShapes());
         }
         
         mMode = PM_multi;
         mNumShapes = visitor.numShapes();
         setGeneratedNodesCount(mNumShapes);
         
         if (mNumShapes == 1 && mCommonParams.ignoreTransforms)
         {
            // output a single shape in object space
            if (mCommonParams.verbose)
            {
               AiMsgInfo("[abcproc] Single shape mode");
            }
            mMode = PM_single;
         }
         
         
         mTimeSamples.clear();
         
         if (ignoreMotionBlur() || (mMode == PM_single && ignoreDeformBlur()))
         {
            mTimeSamples.push_back(mRenderTime);
         }
         else
         {
            if (mCommonParams.samples.size() == 0)
            {
               //AiMsgWarning("[abcproc] No samples set");
               if (mShutterOpen <= mShutterClose)
               {
                  if (mShutterOpen < mShutterClose)
                  {
                     double step = mShutterClose - mShutterOpen;
                     
                     if (mShutterStep > 1)
                     {
                        step /= (mShutterStep - 1);
                     }
                     
                     if (mShutterStep > 0)
                     {
                        
                     }
                     
                     for (double relSample=mShutterOpen; relSample<=mShutterClose; relSample+=step)
                     {
                        mTimeSamples.push_back(computeTime(mCommonParams.frame + relSample));
                     }
                  }
                  else
                  {
                     mTimeSamples.push_back(computeTime(mCommonParams.frame + mShutterOpen));
                  }
               }
               else
               {
                  AiMsgWarning("[abcproc] No samples set, use render frame");
                  mTimeSamples.push_back(mRenderTime);
               }
            }
            else
            {
               if (mCommonParams.relativeSamples)
               {
                  for (size_t i=0; i<mCommonParams.samples.size(); ++i)
                  {
                     mTimeSamples.push_back(computeTime(mCommonParams.frame + mCommonParams.samples[i]));
                  }
               }
               else
               {
                  for (size_t i=0; i<mCommonParams.samples.size(); ++i)
                  {
                     mTimeSamples.push_back(computeTime(mCommonParams.samples[i]));
                  }
               }
            }
         }
         
         mExpandedTimeSamples = mTimeSamples;
         
         // Expand time samples
         if (mTimeSamples.size() > 1 && mCommonParams.samplesExpandIterations > 0)
         {
            std::vector<double> samples;
            
            for (int i=0; i<mCommonParams.samplesExpandIterations; ++i)
            {
               samples.push_back(mExpandedTimeSamples[0]);
               
               for (size_t j=1; j<mExpandedTimeSamples.size(); ++j)
               {
                  double mid = 0.5 * (mExpandedTimeSamples[j-1] + mExpandedTimeSamples[j]);
                  samples.push_back(mid);
                  samples.push_back(mExpandedTimeSamples[j]);
               }
               
               // Arnold doesn't accept more than 255 motion keys
               if (samples.size() > 255)
               {
                  break;
               }
               
               std::swap(mExpandedTimeSamples, samples);
               samples.clear();
            }
         }
         
         // Optimize time samples
         if ((mExpandedTimeSamples.size() > mTimeSamples.size()) && mCommonParams.optimizeSamples)
         {
            // Remove samples outside of camera shutter range
            AtNode *cam = AiUniverseGetCamera();
            
            if (cam)
            {
               AtNode *opts = AiUniverseGetOptions();
               
               if (AiNodeLookUpUserParameter(opts, "motion_start_frame") &&
                   AiNodeLookUpUserParameter(opts, "motion_end_frame"))
               {
                  std::vector<double> samples;
                  
                  mMotionStart = AiNodeGetFlt(opts, "motion_start_frame");
                  mMotionEnd = AiNodeGetFlt(opts, "motion_end_frame");
                  
                  if (AiNodeLookUpUserParameter(opts, "frame") &&
                      AiNodeLookUpUserParameter(opts, "relative_motion_frame") &&
                      AiNodeGetBool(opts, "relative_motion_frame"))
                  {
                     float frame = AiNodeGetFlt(opts, "frame");
                     mMotionStart += frame;
                     mMotionEnd += frame;
                  }
                  
                  mMotionStart /= mCommonParams.fps;
                  mMotionEnd /= mCommonParams.fps;
                  
                  double motionLength = mMotionEnd - mMotionStart;
                  
                  double shutterOpen = (mMotionStart + AiCameraGetShutterStart() * motionLength);
                  double shutterClose = (mMotionStart + AiCameraGetShutterEnd() * motionLength);
                  
                  for (size_t i=0; i<mExpandedTimeSamples.size(); ++i)
                  {
                     double sample = mExpandedTimeSamples[i];
                     
                     if (sample < shutterOpen)
                     {
                        continue;
                     }
                     else if (sample > shutterClose)
                     {
                        if (samples.size() > 0 && samples.back() < shutterClose)
                        {
                           samples.push_back(sample);
                        }
                     }
                     else
                     {
                        if (samples.size() == 0 && i > 0 && sample > shutterOpen)
                        {
                           samples.push_back(mExpandedTimeSamples[i-1]);
                        }
                        samples.push_back(sample);
                     }
                  }
                  
                  if (samples.size() > 0 && samples.size() < mExpandedTimeSamples.size())
                  {
                     std::swap(samples, mExpandedTimeSamples);
                     mSetTimeSamples = true;
                  }
               }
               else
               {
                 AiMsgWarning("[abcproc] Cannot optimize motion blur samples (missing motion sample range info)");
               }
            }
            else
            {
               AiMsgWarning("[abcproc] Cannot optimize motion blur samples (no camera set)");
            }
         }
         
         // Compute shape key
         mShapeKey = shapeKey();
         
         if (mCommonParams.verbose)
         {
            AiMsgInfo("[abcproc] Procedural parameters: \"%s\"", dataString(0).c_str());
            
            for (size_t i=0; i<mExpandedTimeSamples.size(); ++i)
            {
               AiMsgInfo("[abcproc] Add motion sample time: %lf", mExpandedTimeSamples[i]);
            }
         }
         
         if (mMode == PM_single)
         {
            // Read instance_num attribute
            mInstanceNum = 0;
            
            const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mProcNode, "instance_num");
            if (pe)
            {
               int pt = AiUserParamGetType(pe);
               if (pt == AI_TYPE_INT)
               {
                  mInstanceNum = AiNodeGetInt(mProcNode, "instance_num");
               }
               else if (pt == AI_TYPE_UINT)
               {
                  mInstanceNum = int(AiNodeGetUInt(mProcNode, "instance_num"));
               }
               else if (pt == AI_TYPE_BYTE)
               {
                  mInstanceNum = int(AiNodeGetByte(mProcNode, "instance_num"));
               }
            }
         }
         
         // Check if we should export a box for volume shading rather than a new procedural or shape
         const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mProcNode, "step_size");
         if (pe)
         {
            mStepSize = AiNodeGetFlt(mProcNode, "step_size");
         }
      }
      else
      {
         AiMsgInfo("[abcproc] Out of frame range");
      }
   }
   else
   {
      AiMsgWarning("[abcproc] Invalid alembic file \"%s\"", mCommonParams.filePath.c_str());
   }
}

Dso::~Dso()
{
   ScopeLock _lock;
   
   char id[64];
   sprintf(id, "%p", AiThreadSelf());
   
   if (mScene)
   {
      AlembicSceneCache::Unref(mScene, id);
   }
   if (mRefScene)
   {
      AlembicSceneCache::Unref(mRefScene, id);
   }
}

std::string Dso::getReferencePath(const std::string &basePath) const
{
   std::string dotRefPath;
   std::string refPath;
  
   size_t p0 = basePath.find_last_of("\\/");
  
   size_t p1 = basePath.rfind('.');
   if (p1 == std::string::npos || (p0 != std::string::npos && p0 > p1))
   {
      dotRefPath = basePath + ".ref";
   }
   else
   {
      dotRefPath = basePath.substr(0, p1) + ".ref";
   }
   
   std::ifstream dotRefFile(dotRefPath.c_str());
   
   if (dotRefFile.is_open())
   {
      std::getline(dotRefFile, refPath);
      
      if (!dotRefFile.fail())
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc] Got reference path from .ref file: %s", refPath.c_str());
         }
         return refPath;
      }
   }
   
   return "";
}

std::string Dso::dataString(const char *targetShape) const
{
   if (targetShape && strlen(targetShape) == 0)
   {
      targetShape = 0;
   }
   
   std::string rv = mCommonParams.dataString(targetShape);
   
   if (!targetShape)
   {
      rv += mMultiParams.dataString();
   }
   
   rv += mSingleParams.dataString();
   
   if (mRootDrive.length() > 0)
   {
      rv += " -rootdrive " + mRootDrive.substr(0, 1);
   }
   
   if (rv.length() > 0 && rv[0] == ' ')
   {
      return rv.substr(1);
   }
   else
   {
      return rv;
   }
}

std::string Dso::shapeKey() const
{
   return mCommonParams.shapeKey() + ":" + mSingleParams.shapeKey();
}

void Dso::setGeneratedNodesCount(size_t n)
{
   mGeneratedNodes.resize(n, 0);
}

void Dso::setGeneratedNode(size_t idx, AtNode *node)
{
   if (idx < mGeneratedNodes.size())
   {
      if (mSetTimeSamples)
      {
         float invMotionLength = 1.0 / (mMotionEnd - mMotionStart);
        
         AtArray *dtimes = AiArrayAllocate(mExpandedTimeSamples.size(), 1, AI_TYPE_FLOAT);
         AtArray *xtimes = AiArrayAllocate(mExpandedTimeSamples.size(), 1, AI_TYPE_FLOAT);
        
         for (size_t i=0; i<mExpandedTimeSamples.size(); ++i)
         {
            float t = (mExpandedTimeSamples[i] - mMotionStart) * invMotionLength;
          
            AiArraySetFlt(dtimes, i, t);
            AiArraySetFlt(xtimes, i, t);
         }
         
         AiNodeSetArray(node, "transform_time_samples", xtimes);
         AiNodeSetArray(node, "deform_time_samples", dtimes);
      }
      
      // Add '_proc_generator' user attribute
      if (AiNodeLookUpUserParameter(node, "_proc_generator") == NULL)
      {
         AiNodeDeclare(node, "_proc_generator", "constant STRING");
      }
      AiNodeSetStr(node, "_proc_generator", AiNodeGetName(mProcNode));
      
      mGeneratedNodes[idx] = node;
   }
}

double Dso::attribsTime(AttributeFrame af) const
{
   double shutterOpen = AiCameraGetShutterStart();
   double shutterClose = AiCameraGetShutterEnd();
   double shutterCenter = 0.5 * (shutterOpen + shutterClose);
   
   double motionStart = mRenderTime;
   double motionEnd = mRenderTime;
   
   if (mExpandedTimeSamples.size() > 0)
   {
      motionStart = mExpandedTimeSamples[0];
      motionEnd = mExpandedTimeSamples.back();
   }
   
   switch (af)
   {
   case AF_shutter:
      return motionStart + shutterCenter * (motionEnd - motionStart);
   case AF_shutter_open:
      return motionStart + shutterOpen * (motionEnd - motionStart);
   case AF_shutter_close:
      return motionStart + shutterClose * (motionEnd - motionStart);
   case AF_render:
   default:
      return mRenderTime;
   }
}

std::string Dso::uniqueName(const std::string &baseName) const
{
   size_t i = 0;
   std::string name = baseName;

   while (AiNodeLookUpByName(name.c_str()) != 0)
   {
      ++i;
      
      std::ostringstream oss;
      oss << baseName << i;
      
      name = oss.str();
   }

   return name;
}

double Dso::computeTime(double frame, bool *exclude) const
{
   // Apply speed / offset
   if (exclude)
   {
      *exclude = false;
   }
   
   double extraOffset = 0.0;
   
   if (mCommonParams.preserveStartFrame && fabs(mCommonParams.speed) > 0.0001)
   {
      extraOffset = (mCommonParams.startFrame * (mCommonParams.speed - 1.0) / mCommonParams.speed);
   }
   
   double invFps = 1.0 / mCommonParams.fps;
   
   double t = mCommonParams.speed * invFps * (frame - (mCommonParams.offset + extraOffset));
   
   // Apply cycle type
   
   double startTime = invFps * mCommonParams.startFrame;
   double endTime = invFps * mCommonParams.endFrame;
   
   double playTime = endTime - startTime;
   double eps = 0.001;
   double t2 = t;
   
   switch (mCommonParams.cycle)
   {
   case CT_loop:
      if (t < (startTime - eps) || t > (endTime + eps))
      {
         double timeOffset = t - startTime;
         double playOffset = floor(timeOffset / playTime);
         double fraction = fabs(timeOffset / playTime - playOffset);
         
         t2 = startTime + fraction * playTime;
      }
      break;
   case CT_reverse:
      if (t > (startTime + eps) && t < (endTime - eps))
      {
         double timeOffset = t - startTime;
         double playOffset = floor(timeOffset / playTime);
         double fraction = fabs(timeOffset / playTime - playOffset);
         
         t2 = endTime - fraction * playTime;
      }
      else if (t < (startTime + eps))
      {
         t2 = endTime;
      }
      else
      {
         t2 = startTime;
      }
      break;
   case CT_bounce:
      if (t < (startTime - eps) || t > (endTime + eps))
      {
         double timeOffset = t - startTime;
         double playOffset = floor(timeOffset / playTime);
         double fraction = fabs(timeOffset / playTime - playOffset);
         
         if (int(playOffset) % 2 == 0)
         {
            t2 = startTime + fraction * playTime;
         }
         else
         {
            t2 = endTime - fraction * playTime;
         }
      }
      break;
   case CT_clip:
      if (t < (startTime - eps))
      {
         t2 = startTime;
         if (exclude)
         {
            *exclude = true;
         }
      }
      else if (t > (endTime + eps))
      {
         t2 = endTime;
         if (exclude)
         {
            *exclude = true;
         }
      }
      break;
   case CT_hold:
   default:
      if (t < (startTime - eps))
      {
         t2 = startTime;
      }
      else if (t > (endTime + eps))
      {
         t2 = endTime;
      }
      break;
   }
   
   if (mCommonParams.verbose)
   {
      AiMsgInfo("[abcproc] frame %lf -> time %lf (speed, offset) -> %lf (cycle)", frame, t, t2);
   }
   
   return t2;
}

bool Dso::cleanAttribName(std::string &name) const
{
   size_t len = name.length();
   size_t llen = 0;
   
   std::set<std::string>::const_iterator it = mSingleParams.attribPreficesToRemove.begin();
   
   while (it != mSingleParams.attribPreficesToRemove.end())
   {
      size_t plen = it->length();
      
      if (len > plen && plen > llen && !strncmp(name.c_str(), it->c_str(), plen))
      {
         llen = plen;
      }
      
      ++it;
   }
   
   if (llen > 0)
   {
      name = name.substr(llen);
      return true;
   }
   
   return false;
}

void Dso::readFromDataParam()
{
   std::string data = AiNodeGetStr(mProcNode, "data");
   
   // split data string
   
   std::string arg;
   std::vector<std::string> args;
   
   size_t p0 = 0;
   size_t p1 = data.find(' ', p0);
   
   while (p1 != std::string::npos)
   {
      arg = data.substr(p0, p1-p0);
      
      strip(arg);
      
      if (arg.length() > 0)
      {
         args.push_back(arg);
      }
      
      p0 = p1 + 1;
      p1 = data.find(' ', p0);
   }
   
   arg = data.substr(p0);
   strip(arg);
   
   if (arg.length() > 0)
   {
      args.push_back(arg);
   }
   
   // process args
   
   for (size_t i=0; i<args.size(); ++i)
   {
      if (isFlag(args[i]))
      {
         size_t j = i;
         
         if (!processFlag(args, j))
         {
            AiMsgWarning("[abcproc] Failed processing flag \"%s\"", args[i].c_str());
            ++i;
         }
         else
         {
            i = j;
         }
      }
      else
      {
         AiMsgWarning("[abcproc] Ignore arguments: \"%s\"", args[i].c_str());
      }
   }
}

void Dso::normalizeFilePath(std::string &path) const
{
   // Convert \ to /
   size_t p0 = 0;
   size_t p1 = path.find('\\', p0);

   while (p1 != std::string::npos)
   {
      path[p1] = '/';
      p0 = p1 + 1;
      p1 = path.find('\\', p0);
   }
   
   #ifdef _WIN32
   
   // Convert to lower calse
   toLower(path);
   
   // Add driver letter
   if (path.length() >= 1 && path[0] == '/')
   {
      if (path.length() >= 2 && path[1] != '/')
      {
         if (mRootDrive.length() > 0)
         {
            path = mRootDrive + path;
         }
         else
         {
            path = "Z:" + path;
         }
      }
      else
      {
         // ignore path starting with //
         // -> i.e: //10.10.121.1/ifs/mz_onyx/... should not be mapped
      }
   }
   
   #else
   
   // Remove driver letter
   if (path.length() >= 2 && path[1] == ':' &&
       ((path[0] >= 'a' && path[0] <= 'z') ||
        (path[0] >= 'A' && path[0] <= 'Z')))
   {
      path = path.substr(2);
   }
   
   #endif
}

void Dso::strip(std::string &s) const
{
   size_t p0 = s.find_first_not_of(" \t\n\v");
   size_t p1 = s.find_last_not_of(" \t\n\v");
   
   if (p0 == std::string::npos || p1 == std::string::npos)
   {
      s = "";
      return;
   }
   
   s = s.substr(p0, p1-p0+1);
}

void Dso::toLower(std::string &s) const
{
   for (size_t i=0; i<s.length(); ++i)
   {
      if (s[i] >= 'A' && s[i] <= 'Z')
      {
         s[i] = 'a' + (s[i] - 'A');
      }
   }
}

bool Dso::isFlag(std::string &s) const
{
   return (s.length() > 0 && s[0] == '-');
}

bool Dso::processFlag(std::vector<std::string> &args, size_t &i)
{
   toLower(args[i]);
   
   if (args[i] == "-rootdrive")
   {
      ++i;
      if (i >= args.size())
      {
         return false;
      }
      
      std::string rootDrive = args[i];
      if (rootDrive.length() == 1 && 
          ((rootDrive[0] >= 'a' && rootDrive[0] <= 'z') ||
           (rootDrive[0] >= 'A' && rootDrive[0] <= 'Z')))
      {
         mRootDrive = rootDrive + ":";
      }
   }
   // First start with flags supported in official procedural
   else if (args[i] == "-frame")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &(mCommonParams.frame)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-fps")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &(mCommonParams.fps)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-filename")
   {
      ++i;
      if (i >= args.size() || isFlag(args[i]))
      {
         return false;
      }
      mCommonParams.filePath = args[i];
   }
   else if (args[i] == "-nameprefix")
   {
      ++i;
      if (i >= args.size() || isFlag(args[i]))
      {
         return false;
      }
      mCommonParams.namePrefix = args[i];
   }
   else if (args[i] == "-objectpath")
   {
      ++i;
      if (i >= args.size() || isFlag(args[i]))
      {
         return false;
      }
      mCommonParams.objectPath = args[i];
   }
   else if (args[i] == "-excludexform")
   {
      mCommonParams.ignoreTransforms = true;
   }
   else if (args[i] == "-subditerations")
   {
      ++i;
      if (i >= args.size())
      {
         return false;
      }
      // Don't use
   }
   else if (args[i] == "-makeinstance")
   {
      // Don't use
   }
   else if (args[i] == "-shutteropen")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &mShutterOpen) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-shutterclose")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &mShutterClose) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-shutterstep")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%d", &mShutterStep) != 1)
      {
         return false;
      }
   }
   // Process common parameters
   else if (args[i] == "-outputreference")
   {
      mCommonParams.outputReference = true;
   }
   else if (args[i] == "-referencefilename")
   {
      ++i;
      if (i < args.size() && !isFlag(args[i]))
      {
         mCommonParams.referenceFilePath = args[i];
      }
   }
   else if (args[i] == "-offset")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &(mCommonParams.offset)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-speed")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &(mCommonParams.speed)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-startframe")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &(mCommonParams.startFrame)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-endframe")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%lf", &(mCommonParams.endFrame)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-preservestartframe")
   {
      mCommonParams.preserveStartFrame = true;
   }
   else if (args[i] == "-ignoremotionblur")
   {
      mCommonParams.ignoreTransformBlur = true;
      mCommonParams.ignoreDeformBlur = true;
   }
   else if (args[i] == "-ignoredeformblur")
   {
      mCommonParams.ignoreDeformBlur = true;
   }
   else if (args[i] == "-ignoretransformblur")
   {
      mCommonParams.ignoreTransformBlur = true;
   }
   else if (args[i] == "-ignoretransforms")
   {
      mCommonParams.ignoreTransforms = true;
   }
   else if (args[i] == "-ignorevisibility")
   {
      mCommonParams.ignoreVisibility = true;
   }
   else if (args[i] == "-ignoreinstances")
   {
      mCommonParams.ignoreInstances = true;
   }
   else if (args[i] == "-cycle")
   {
      ++i;
      
      if (i >= args.size())
      {
         return false;
      }
      
      std::string cts = args[i];
      
      int ival = -1;
      
      if (sscanf(cts.c_str(), "%d", &ival) == 1)
      {
         if (ival >= 0 && ival < CT_MAX)
         {
            mCommonParams.cycle = (CycleType) ival;
         }
      }
      else
      {
         toLower(cts);
         
         for (size_t j=0; j<CT_MAX; ++j)
         {
            if (cts == CycleTypeNames[j])
            {
               mCommonParams.cycle = (CycleType)j;
               break;
            }
         }
      }
   }
   else if (args[i] == "-relativesamples")
   {
      mCommonParams.relativeSamples = true;
   }
   else if (args[i] == "-samples")
   {
      ++i;
      
      double sample;
      
      while (i < args.size() && sscanf(args[i].c_str(), "%lf", &sample) == 1)
      {
         mCommonParams.samples.push_back(sample);
         ++i;
      }
      --i;
   }
   else if (args[i] == "-samplesexpanditerations")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%d", &(mCommonParams.samplesExpandIterations)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-optimizesamples")
   {
      mCommonParams.optimizeSamples = true;
   }
   else if (args[i] == "-verbose")
   {
      mCommonParams.verbose = true;
   }
   // Process multi params
   else if (args[i] == "-overrideattribs")
   {
      ++i;
      while (i < args.size() && !isFlag(args[i]))
      {
         mMultiParams.overrideAttribs.insert(args[i]);
         ++i;
      }
      --i;
   }
   else if (args[i] == "-promotetoobjectattribs")
   {
      ++i;
      while (i < args.size() && !isFlag(args[i]))
      {
         mCommonParams.promoteToObjectAttribs.insert(args[i]);
         ++i;
      }
      --i;
   }
   else if (args[i] == "-overrideboundsminname")
   {
      ++i;
      if (i < args.size() && !isFlag(args[i]))
      {
         mMultiParams.overrideBoundsMinName = args[i];
      }
   }
   else if (args[i] == "-overrideboundsmaxname")
   {
      ++i;
      if (i < args.size() && !isFlag(args[i]))
      {
         mMultiParams.overrideBoundsMaxName = args[i];
      }
   }
   // Process single params
   else if (args[i] == "-objectattribs")
   {
      mSingleParams.readObjectAttribs = true;
   }
   else if (args[i] == "-primitiveattribs")
   {
      mSingleParams.readPrimitiveAttribs = true;
   }
   else if (args[i] == "-pointattribs")
   {
      mSingleParams.readPointAttribs = true;
   }
   else if (args[i] == "-vertexattribs")
   {
      mSingleParams.readVertexAttribs = true;
   }
   else if (args[i] == "-attribsframe")
   {
      ++i;
      if (i >= args.size())
      {
         return false;
      }
      
      std::string afs = args[i];
      
      int ival = -1;
      
      if (sscanf(afs.c_str(), "%d", &ival) == 1)
      {
         if (ival >= 0 && ival < AF_MAX)
         {
            mSingleParams.attribsFrame = (AttributeFrame) ival;
         }
      }
      else
      {
         toLower(afs);
         
         for (size_t j=0; j<AF_MAX; ++j)
         {
            if (afs == AttributeFrameNames[j])
            {
               mSingleParams.attribsFrame = (AttributeFrame)j;
               break;
            }
         }
      }
   }
   else if (args[i] == "-removeattribprefices")
   {
      ++i;
      while (i < args.size() && !isFlag(args[i]))
      {
         mSingleParams.attribPreficesToRemove.insert(args[i]);
         ++i;
      }
      --i;
   }
   else if (args[i] == "-computetangents")
   {
      ++i;
      while (i < args.size() && !isFlag(args[i]))
      {
         mSingleParams.computeTangents.insert(args[i]);
         ++i;
      }
      --i;
   }
   else if (args[i] == "-radiusmin")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%f", &(mSingleParams.radiusMin)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-radiusmax")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%f", &(mSingleParams.radiusMax)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-radiusscale")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%f", &(mSingleParams.radiusScale)) != 1)
      {
         return false;
      }
   }
   else if (args[i] == "-nurbssamplerate")
   {
      ++i;
      if (i >= args.size() ||
          sscanf(args[i].c_str(), "%d", &(mSingleParams.nurbsSampleRate)) != 1)
      {
         return false;
      }
   }
   else
   {
      AiMsgWarning("[abcproc] Unknown flag \"%s\"", args[i].c_str());
      return false;
   }
   
   return true;
}

void Dso::readFromUserParams()
{
   bool relativeSamples = false;
   bool samplesSet = false;

   AtUserParamIterator *pit = AiNodeGetUserParamIterator(mProcNode);
   
   while (!AiUserParamIteratorFinished(pit))
   {
      const AtUserParamEntry *p = AiUserParamIteratorGetNext(pit);
      if (!p)
      {
         continue;
      }

      const char *pname = AiUserParamGetName(p);
      if (!pname)
      {
         continue;
      }

      if (strncmp(pname, "abc_", 4) != 0)
      {
         continue;
      }
      
      if (AiUserParamGetCategory(p) != AI_USERDEF_CONSTANT)
      {
         continue;
      }
      
      std::string param = pname + 4;
      toLower(param);
      
      if (param == "rootdrive")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string rootDrive = AiNodeGetStr(mProcNode, pname);
            
            if (rootDrive.length() == 1 && 
                ((rootDrive[0] >= 'a' && rootDrive[0] <= 'z') ||
                 (rootDrive[0] >= 'A' && rootDrive[0] <= 'Z')))
            {
               mRootDrive = rootDrive + ":";
            }
            else
            {
               AiMsgWarning("[abcproc] Ignore parameter \"%s\": Value should be a single letter", pname);
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "filename")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            mCommonParams.filePath = AiNodeGetStr(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "outputreference")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.outputReference = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "referencefilename")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            mCommonParams.referenceFilePath = AiNodeGetStr(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "objectpath")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            mCommonParams.objectPath = AiNodeGetStr(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "nameprefix")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            mCommonParams.namePrefix = AiNodeGetStr(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "fps")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mCommonParams.fps = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "frame")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mCommonParams.frame = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "speed")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mCommonParams.speed = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "offset")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mCommonParams.offset = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "startframe")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mCommonParams.startFrame = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "endframe")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mCommonParams.endFrame = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "preservestartframe")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.preserveStartFrame = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "cycle")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string val = AiNodeGetStr(mProcNode, pname);
            toLower(val);
            
            int i = 0;
            
            for (; i<CT_MAX; ++i)
            {
               if (val == CycleTypeNames[i])
               {
                  mCommonParams.cycle = (CycleType) i;
                  break;
               }
            }
            
            if (i >= CT_MAX)
            {
               AiMsgWarning("[abcproc] Ignore parameter \"%s\": Invalid value", pname);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_INT)
         {
            int val = AiNodeGetInt(mProcNode, pname);
            if (val >= 0 && val < CT_MAX)
            {
               mCommonParams.cycle = (CycleType) val;
            }
            else
            {
               AiMsgWarning("[abcproc] Ignore parameter \"%s\": Invalid value", pname);
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an int or string value", pname);
         }
      }
      else if (param == "relativesamples")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            relativeSamples = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "samples")
      {
         if (AiUserParamGetType(p) == AI_TYPE_ARRAY && AiUserParamGetArrayType(p) == AI_TYPE_FLOAT)
         {
            AtArray *samples = AiNodeGetArray(mProcNode, pname);
            
            if (samples)
            {
               unsigned int ne = samples->nelements;
               unsigned int nk = samples->nkeys;
               
               if (ne * nk > 0)
               {
                  mCommonParams.samples.clear();
                  
                  // Either number of keys or number of elements
                  if (nk > 1)
                  {
                     ne = 1;
                  }
                  
                  for (unsigned int i=0, j=0; i<samples->nkeys; ++i, j+=samples->nelements)
                  {
                     for (unsigned int k=0; k<ne; ++k)
                     {
                        mCommonParams.samples.push_back(AiArrayGetFlt(samples, j+k));
                     }
                  }
                  
                  samplesSet = true;
               }
               else
               {
                  AiMsgWarning("[abcproc] Ignore parameter \"%s\": Empty value", pname);
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an array of float values", pname);
         }
      }
      else if (param == "samplesexpanditerations")
      {
         if (AiUserParamGetType(p) == AI_TYPE_INT)
         {
            mCommonParams.samplesExpandIterations = AiNodeGetInt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an integer value", pname);
         }
      }
      else if (param == "optimizesamples")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.optimizeSamples = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "ignoremotionblur")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.ignoreDeformBlur = (AiNodeGetBool(mProcNode, pname) || mCommonParams.ignoreDeformBlur);
            mCommonParams.ignoreTransformBlur = (AiNodeGetBool(mProcNode, pname) || mCommonParams.ignoreTransformBlur);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "ignoredeformblur")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.ignoreDeformBlur = (AiNodeGetBool(mProcNode, pname) || mCommonParams.ignoreDeformBlur);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "ignoretransformblur")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.ignoreTransformBlur = (AiNodeGetBool(mProcNode, pname) || mCommonParams.ignoreTransformBlur);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "ignorevisibility")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.ignoreVisibility = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "ignoreinstances")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.ignoreInstances = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "ignoretransforms")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.ignoreTransforms = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "verbose")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mCommonParams.verbose = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "overrideattribs")
      {
         mMultiParams.overrideAttribs.clear();
         
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string names = AiNodeGetStr(mProcNode, pname);
            std::string name;
            
            size_t p0 = 0;
            size_t p1 = names.find(' ', p0);
            
            while (p1 != std::string::npos)
            {
               name = names.substr(p0, p1-p0);
               strip(name);
               
               if (name.length() > 0)
               {
                  mMultiParams.overrideAttribs.insert(name);
               }
               
               p0 = p1 + 1;
               p1 = names.find(' ', p0);
            }
            
            name = names.substr(p0);
            strip(name);
            
            if (name.length() > 0)
            {
               mMultiParams.overrideAttribs.insert(name);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_ARRAY && AiUserParamGetArrayType(p) == AI_TYPE_STRING)
         {
            AtArray *names = AiNodeGetArray(mProcNode, pname);
            
            if (names)
            {
               for (unsigned int i=0; i<names->nelements; ++i)
               {
                  mMultiParams.overrideAttribs.insert(AiArrayGetStr(names, i));
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an array of string values", pname);
         }
      }
      else if (param == "promotetoobjectattribs")
      {
         mCommonParams.promoteToObjectAttribs.clear();
         
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string names = AiNodeGetStr(mProcNode, pname);
            std::string name;
            
            size_t p0 = 0;
            size_t p1 = names.find(' ', p0);
            
            while (p1 != std::string::npos)
            {
               name = names.substr(p0, p1-p0);
               strip(name);
               
               if (name.length() > 0)
               {
                  mCommonParams.promoteToObjectAttribs.insert(name);
               }
               
               p0 = p1 + 1;
               p1 = names.find(' ', p0);
            }
            
            name = names.substr(p0);
            strip(name);
            
            if (name.length() > 0)
            {
               mCommonParams.promoteToObjectAttribs.insert(name);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_ARRAY && AiUserParamGetArrayType(p) == AI_TYPE_STRING)
         {
            AtArray *names = AiNodeGetArray(mProcNode, pname);
            
            if (names)
            {
               for (unsigned int i=0; i<names->nelements; ++i)
               {
                  mCommonParams.promoteToObjectAttribs.insert(AiArrayGetStr(names, i));
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an array of string values", pname);
         }
      }
      else if (param == "overrideboundsminname")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            mMultiParams.overrideBoundsMinName = AiNodeGetStr(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "overrideboundsmaxname")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            mMultiParams.overrideBoundsMaxName = AiNodeGetStr(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a string value", pname);
         }
      }
      else if (param == "objectattribs")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mSingleParams.readObjectAttribs = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "primitiveattribs")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mSingleParams.readPrimitiveAttribs = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "pointattribs")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mSingleParams.readPointAttribs = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "vertexattribs")
      {
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mSingleParams.readVertexAttribs = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
         }
      }
      else if (param == "attribsframe")
      {
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string val = AiNodeGetStr(mProcNode, pname);
            toLower(val);
            
            int i = 0;
            
            for (; i<AF_MAX; ++i)
            {
               if (val == AttributeFrameNames[i])
               {
                  mSingleParams.attribsFrame = (AttributeFrame) i;
                  break;
               }
            }
            
            if (i >= AF_MAX)
            {
               AiMsgWarning("[abcproc] Ignore parameter \"%s\": Invalid value", pname);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_INT)
         {
            int val = AiNodeGetInt(mProcNode, pname);
            if (val >= 0 && val < AF_MAX)
            {
               mSingleParams.attribsFrame = (AttributeFrame) val;
            }
            else
            {
               AiMsgWarning("[abcproc] Ignore parameter \"%s\": Invalid value", pname);
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an int or string value", pname);
         }
      }
      else if (param == "removeattribprefices")
      {
         mSingleParams.attribPreficesToRemove.clear();
         
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string prefices = AiNodeGetStr(mProcNode, pname);
            std::string prefix;
            
            size_t p0 = 0;
            size_t p1 = prefices.find(' ', p0);
            
            while (p1 != std::string::npos)
            {
               prefix = prefices.substr(p0, p1-p0);
               strip(prefix);
               
               if (prefix.length() > 0)
               {
                  mSingleParams.attribPreficesToRemove.insert(prefix);
               }
               
               p0 = p1 + 1;
               p1 = prefices.find(' ', p0);
            }
            
            prefix = prefices.substr(p0);
            strip(prefix);
            
            if (prefix.length() > 0)
            {
               mSingleParams.attribPreficesToRemove.insert(prefix);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_ARRAY && AiUserParamGetArrayType(p) == AI_TYPE_STRING)
         {
            AtArray *prefices = AiNodeGetArray(mProcNode, pname);
            
            if (prefices)
            {
               for (unsigned int i=0; i<prefices->nelements; ++i)
               {
                  mSingleParams.attribPreficesToRemove.insert(AiArrayGetStr(prefices, i));
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an array of string values", pname);
         }
      }
      else if (param == "computetangents")
      {
         mSingleParams.computeTangents.clear();
         
         if (AiUserParamGetType(p) == AI_TYPE_STRING)
         {
            std::string uvnames = AiNodeGetStr(mProcNode, pname);
            std::string uvname;
            
            size_t p0 = 0;
            size_t p1 = uvnames.find(' ', p0);
            
            while (p1 != std::string::npos)
            {
               uvname = uvnames.substr(p0, p1-p0);
               strip(uvname);
               
               if (uvname.length() > 0)
               {
                  mSingleParams.computeTangents.insert(uvname);
               }
               
               p0 = p1 + 1;
               p1 = uvnames.find(' ', p0);
            }
            
            uvname = uvnames.substr(p0);
            strip(uvname);
            
            if (uvname.length() > 0)
            {
               mSingleParams.computeTangents.insert(uvname);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_ARRAY && AiUserParamGetArrayType(p) == AI_TYPE_STRING)
         {
            AtArray *uvnames = AiNodeGetArray(mProcNode, pname);
            
            if (uvnames)
            {
               for (unsigned int i=0; i<uvnames->nelements; ++i)
               {
                  mSingleParams.computeTangents.insert(AiArrayGetStr(uvnames, i));
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an array of string values", pname);
         }
      }
      else if (param == "radiusmin")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mSingleParams.radiusMin = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "radiusmax")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mSingleParams.radiusMax = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "radiusscale")
      {
         if (AiUserParamGetType(p) == AI_TYPE_FLOAT)
         {
            mSingleParams.radiusScale = AiNodeGetFlt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a float value", pname);
         }
      }
      else if (param == "nurbssamplerate")
      {
         if (AiUserParamGetType(p) == AI_TYPE_INT)
         {
            mSingleParams.nurbsSampleRate = AiNodeGetInt(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an integer value", pname);
         }
      }
   }
   
   AiUserParamIteratorDestroy(pit);
   
   // Only override relative samples if samples were read from user attribute
   if (samplesSet)
   {
      mCommonParams.relativeSamples = relativeSamples;
   }
}

void Dso::transferInstanceParams(AtNode *dst)
{
   // As of arnold 4.1, type has changed to Byte
   AiNodeSetInt(dst, "id", AiNodeGetInt(mProcNode, "id"));
   AiNodeSetBool(dst, "receive_shadows", AiNodeGetBool(mProcNode, "receive_shadows"));
   AiNodeSetBool(dst, "self_shadows", AiNodeGetBool(mProcNode, "self_shadows"));
   AiNodeSetBool(dst, "invert_normals", AiNodeGetBool(mProcNode, "invert_normals"));
   AiNodeSetBool(dst, "opaque", AiNodeGetBool(mProcNode, "opaque"));
   AiNodeSetBool(dst, "use_light_group", AiNodeGetBool(mProcNode, "use_light_group"));
   AiNodeSetBool(dst, "use_shadow_group", AiNodeGetBool(mProcNode, "use_shadow_group"));
   AiNodeSetFlt(dst, "ray_bias", AiNodeGetFlt(mProcNode, "ray_bias"));
   #ifdef ARNOLD_4_1_OR_ABOVE
   AiNodeSetByte(dst, "visibility", AiNodeGetByte(mProcNode, "visibility"));
   AiNodeSetByte(dst, "sidedness", AiNodeGetByte(mProcNode, "sidedness"));
   #else
   AiNodeSetInt(dst, "visibility", AiNodeGetInt(mProcNode, "visibility"));
   AiNodeSetInt(dst, "sidedness", AiNodeGetInt(mProcNode, "sidedness"));
   AiNodeSetInt(dst, "sss_sample_distribution", AiNodeGetInt(mProcNode, "sss_sample_distribution"));
   AiNodeSetFlt(dst, "sss_sample_spacing", AiNodeGetFlt(mProcNode, "sss_sample_spacing"));
   #endif
     
   AtArray *ary;
  
   ary = AiNodeGetArray(mProcNode, "matrix");
   if (ary)
   {
      AiNodeSetArray(dst, "matrix", AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, "shader");
   if (ary)
   {
      AiNodeSetArray(dst, "shader", AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, "light_group");
   if (ary)
   {
      AiNodeSetArray(dst, "light_group", AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, "shadow_group");
   if (ary)
   {
      AiNodeSetArray(dst, "shadow_group", AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, "trace_sets");
   if (ary)
   {
      AiNodeSetArray(dst, "trace_sets", AiArrayCopy(ary));
   }
}

void Dso::transferUserParams(AtNode *dst)
{
   static const char* sDeclBase[] = {
      "constant", // AI_USERDEF_UNDEFINED (treat as constant)
      "constant", // AI_USERDEF_CONSTANT
      "uniform",  // AI_USERDEF_UNIFORM
      "varying",  // AI_USERDEF_VARYING
      "indexed"   // AI_USERDEF_INDEXED
   };

   if (verbose())
   {
      AiMsgInfo("[abcproc] Copy user attributes...");
   }
   
   std::set<std::string> fviattrs; // face varying attributes indices
   std::set<std::string> ncattrs; // non constant attributes

   AtUserParamIterator *pit = AiNodeGetUserParamIterator(mProcNode);
  
   while (!AiUserParamIteratorFinished(pit))
   {
      const AtUserParamEntry *upe = AiUserParamIteratorGetNext(pit);
      
      bool doDeclare = true;
      bool doCopy = true;

      const char *pname = AiUserParamGetName(upe);
      int ptype = AiUserParamGetType(upe);
      int pcat = AiUserParamGetCategory(upe);

      if (strlen(pname) == 0)
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]   Skip unnamed parameter");
         }
         continue;
      }
      else if (!strncmp(pname, "abc_", 4))
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]   Skip DSO parmeter \"%s\"", pname+4);
         }
         continue;
      }
      else if (!strcmp(pname, "_proc_generator"))
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]   Skip DSO parmeter \"%s\"", pname);
         }
         continue;
      }
      else
      {
         if (!overrideAttrib(pname))
         {
            if (verbose())
            {
               AiMsgInfo("[abcproc]   Skip parmeter \"%s\"", pname);
            }
            continue;
         }
         if (verbose())
         {
            AiMsgInfo("[abcproc]   Process parameter \"%s\"", pname);
         }
      }
      
      if (fviattrs.find(pname) != fviattrs.end())
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Face varying attribute indices attribute \"%s\", do not declare", pname);
         }
         
         doDeclare = false;
         
         // Check that value attribute was properly
         std::string vname(pname);
         // Strip 'idxs' suffix
         vname = vname.substr(0, vname.length()-4);
         
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Check if \"%s\" or \"%slist\" were exported", vname.c_str(), vname.c_str());
         }
         
         if (ncattrs.find(vname) == ncattrs.end() && ncattrs.find(vname+"list") == ncattrs.end())
         {
            if (verbose())
            {
               AiMsgInfo("[abcproc]     Face varying attribute indices attribute \"%s\", do not copy", pname);
            }
            continue;
         }
      }
      else if (pcat != AI_USERDEF_CONSTANT)
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Add non constant attribute name \"%s\"", pname);
         }
         ncattrs.insert(pname);
      }

      const AtParamEntry *pe = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(dst), pname);
      
      int mptype = -1;
      std::string indexAttr;
    
      if (pcat == AI_USERDEF_INDEXED)
      {
         // Note: what is the category of the index array for an indexed attribute?
         //       might want to check for trailing idxs here to avoid adding aaaidxsidxs attribute
         indexAttr = pname;
         if (indexAttr.length() > 4 && !strcmp(indexAttr.c_str() + indexAttr.length() - 4, "list"))
         {
           indexAttr = indexAttr.substr(0, indexAttr.length()-4);
         }
         
         indexAttr += "idxs";
         
         fviattrs.insert(indexAttr);
         
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Add index attribute name \"%s\"", indexAttr.c_str());
         }
      }
      
      if (pe == NULL)
      {
         // No such built-in attribute on newly created node
         mptype = ptype;
      
         if (AiNodeLookUpUserParameter(dst, pname) != 0)
         {
            // Already declared during expansion (most probably read from gto file)
            doCopy = false;
         }
         else
         {
            switch (ptype)
            {
            case AI_TYPE_BYTE:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " BYTE").c_str());
               break;
            case AI_TYPE_INT:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " INT").c_str());
               break;
            case AI_TYPE_UINT:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " UINT").c_str());
               break;
            case AI_TYPE_BOOLEAN:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " BOOL").c_str());
               break;
            case AI_TYPE_FLOAT:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " FLOAT").c_str());
               break;
            case AI_TYPE_RGB:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " RGB").c_str());
               break;
            case AI_TYPE_RGBA:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " RGBA").c_str());
               break;
            case AI_TYPE_VECTOR:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " VECTOR").c_str());
               break;
            case AI_TYPE_POINT:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " POINT").c_str());
               break;
            case AI_TYPE_POINT2:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " POINT2").c_str());
               break;
            case AI_TYPE_STRING:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " STRING").c_str());
               break;
            case AI_TYPE_NODE:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " NODE").c_str());
               break;
            case AI_TYPE_MATRIX:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " MATRIX").c_str());
               break;
            case AI_TYPE_ARRAY: {
               AtArray *val = AiNodeGetArray(mProcNode, pname);
               switch (val->type)
               {
               case AI_TYPE_BYTE:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY BYTE").c_str());
                  break;
               case AI_TYPE_INT:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY INT").c_str());
                  break;
               case AI_TYPE_UINT:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY UINT").c_str());
                  break;
               case AI_TYPE_BOOLEAN:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY BOOL").c_str());
                  break;
               case AI_TYPE_FLOAT:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY FLOAT").c_str());
                  break;
               case AI_TYPE_RGB:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY RGB").c_str());
                  break;
               case AI_TYPE_RGBA:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY RGBA").c_str());
                  break;
               case AI_TYPE_VECTOR:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY VECTOR").c_str());
                  break;
               case AI_TYPE_POINT:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY POINT").c_str());
                  break;
               case AI_TYPE_POINT2:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY POINT2").c_str());
                  break;
               case AI_TYPE_STRING:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY STRING").c_str());
                  break;
               case AI_TYPE_NODE:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY NODE").c_str());
                  break;
               case AI_TYPE_MATRIX:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY MATRIX").c_str());
                  break;
               case AI_TYPE_ENUM:
               default:
                  if (verbose())
                  {
                     AiMsgInfo("[abcproc]     Un-supported user array parameter type \"%s\" : %s", pname, AiParamGetTypeName(ptype));
                  }
                  doCopy = false;
                  break;
               }
               break;
            }
            case AI_TYPE_ENUM:
            default:
               if (verbose())
               {
                  AiMsgInfo("[abcproc]     Un-supported user parameter type \"%s\" : %s", pname, AiParamGetTypeName(ptype));
               }
               doCopy = false;
               break;
            }
         }
      }
      else
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Parameter already exists on target node (%s)", AiNodeEntryGetName(AiNodeGetNodeEntry(dst)));
         }
         
         mptype = AiParamGetType(pe);
         if (mptype != ptype)
         {
            // Do some simple type aliasing
            if ((mptype == AI_TYPE_ENUM && (ptype == AI_TYPE_INT || ptype == AI_TYPE_STRING)) ||
                (mptype == AI_TYPE_NODE && ptype == AI_TYPE_STRING) ||
                (mptype == AI_TYPE_BYTE && ptype == AI_TYPE_INT) ||
                (mptype == AI_TYPE_INT && ptype == AI_TYPE_BYTE))
            {
               // Noop
            }
            else
            {
               AiMsgWarning("[abcproc]     Parameter type mismatch \"%s\": %s on destination, %s on source", pname, AiParamGetTypeName(AiParamGetType(pe)), AiParamGetTypeName(ptype));
               doCopy = false;
            }
         }
      }
    
      if (doCopy)
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Copy %s", pname);
         }
         
         AtNode *link = AiNodeGetLink(mProcNode, pname);
         if (link != 0)
         {
            AiNodeLink(link, pname, dst);
         }
         else
         {
            if (pcat != AI_USERDEF_CONSTANT)
            {
               AtArray *val = AiNodeGetArray(mProcNode, pname);

               // just pass the array on?
               // => May want to check size validity depending on target type
               switch (pcat)
               {
               case AI_USERDEF_UNIFORM:
                  if (AiNodeIs(dst, "polymesh"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "nsides");
                     if (val->nelements != a->nelements)
                     {
                        AiMsgWarning("[abcproc]     Polygon count mismatch for uniform attribute");
                        continue;
                     }
                  }
                  else if (AiNodeIs(dst, "curves"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "num_points");
                     if (val->nelements != a->nelements)
                     {
                        AiMsgWarning("[abcproc]     Curve count mismatch for uniform attribute");
                        continue;
                     }
                  }
                  else
                  {
                     if (verbose())
                     {
                        AiMsgInfo("[abcproc]     Cannot apply uniform attribute");
                     }
                     continue;
                  }
                  break;
               case AI_USERDEF_VARYING: // varying
                  if (AiNodeIs(dst, "polymesh"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "vlist");
                     if (val->nelements != a->nelements)
                     {
                        AiMsgWarning("[gto DSO]     Point count mismatch for varying attribute");
                        continue;
                     }
                  }
                  else if (AiNodeIs(dst, "curves"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "points");
                     if (val->nelements != a->nelements)
                     {
                        AiMsgWarning("[gto DSO]     Point count mismatch for varying attribute");
                        continue;
                     }
                  }
                  else if (AiNodeIs(dst, "points"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "points");
                     if (val->nelements != a->nelements)
                     {
                        AiMsgWarning("[gto DSO]     Point count mismatch for varying attribute");
                        continue;
                     }
                  }
                  else
                  {
                     if (verbose())
                     {
                        AiMsgInfo("[gto DSO]     Cannot apply varying attribute");
                     }
                     continue;
                  }
                  break;
               case AI_USERDEF_INDEXED:
                  if (AiNodeIs(dst, "polymesh"))
                  {
                     // get the idxs attribute count
                     AtArray *a = AiNodeGetArray(dst, "vidxs");
                     size_t plen = strlen(pname);
                     if (plen > 4 && !strncmp(pname + plen - 4, "idxs", 4))
                     {
                        // direcly compare size
                        if (val->nelements != a->nelements)
                        {
                           AiMsgWarning("[gto DSO]     Count mismatch for indexed attribute");
                           continue;
                        }
                     }
                     else
                     {
                        // get idxs attribute and compare size [if exists]
                        AtArray *idxs = AiNodeGetArray(mProcNode, indexAttr.c_str());
                        if (!idxs || idxs->nelements != a->nelements)
                        {
                           AiMsgWarning("[gto DSO]     Count mismatch for indexed attribute");
                           continue;
                        }
                     }
                  }
                  else
                  {
                     if (verbose())
                     {
                        AiMsgInfo("[gto DSO]     Cannot apply indexed attribute");
                     }
                  }
                  break;
               default:
                  continue;
               }
            
               AtArray *valCopy = AiArrayCopy(val);
               AiNodeSetArray(dst, pname, valCopy);
            }
            else
            {
               switch (mptype)
               {
               case AI_TYPE_BYTE:
                  if (ptype == AI_TYPE_INT)
                  {
                     AiNodeSetByte(dst, pname, AiNodeGetInt(mProcNode, pname));
                  }
                  else
                  {
                     AiNodeSetByte(dst, pname, AiNodeGetByte(mProcNode, pname));
                  }
                  break;
               case AI_TYPE_INT:
                  if (ptype == AI_TYPE_BYTE)
                  {
                     AiNodeSetInt(dst, pname, AiNodeGetByte(mProcNode, pname));
                  }
                  else
                  {
                     AiNodeSetInt(dst, pname, AiNodeGetInt(mProcNode, pname));
                  }
                  break;
               case AI_TYPE_UINT:
                  AiNodeSetUInt(dst, pname, AiNodeGetUInt(mProcNode, pname));
                  break;
               case AI_TYPE_BOOLEAN:
                  AiNodeSetBool(dst, pname, AiNodeGetBool(mProcNode, pname));
                  break;
               case AI_TYPE_FLOAT:
                  AiNodeSetFlt(dst, pname, AiNodeGetFlt(mProcNode, pname));
                  break;
               case AI_TYPE_RGB: {
                  AtRGB val = AiNodeGetRGB(mProcNode, pname);
                  AiNodeSetRGB(dst, pname, val.r, val.g, val.b);
                  break;
               }
               case AI_TYPE_RGBA: {
                  AtRGBA val = AiNodeGetRGBA(mProcNode, pname);
                  AiNodeSetRGBA(dst, pname, val.r, val.g, val.b, val.a);
                  break;
               }
               case AI_TYPE_VECTOR: {
                  AtVector val = AiNodeGetVec(mProcNode, pname);
                  AiNodeSetVec(dst, pname, val.x, val.y, val.z);
                  break;
               }
               case AI_TYPE_POINT: {
                  AtPoint val = AiNodeGetPnt(mProcNode, pname);
                  AiNodeSetPnt(dst, pname, val.x, val.y, val.z);
                  break;
               }
               case AI_TYPE_POINT2: {
                  AtPoint2 val = AiNodeGetPnt2(mProcNode, pname);
                  AiNodeSetPnt2(dst, pname, val.x, val.y);
                  break;
               }
               case AI_TYPE_STRING:
                  if (ptype == AI_TYPE_NODE)
                  {
                     AtNode *n = (AtNode*) AiNodeGetPtr(mProcNode, pname);
                     AiNodeSetStr(dst, pname, (n ? AiNodeGetName(n) : ""));
                  }
                  else
                  {
                     AiNodeSetStr(dst, pname, AiNodeGetStr(mProcNode, pname));
                  }
                  break;
               case AI_TYPE_NODE:
                  if (ptype == AI_TYPE_STRING)
                  {
                     const char *nn = AiNodeGetStr(mProcNode, pname);
                     AtNode *n = AiNodeLookUpByName(nn);
                     AiNodeSetPtr(dst, pname, (void*)n);
                  }
                  else
                  {
                     AiNodeSetPtr(dst, pname, AiNodeGetPtr(mProcNode, pname));
                  }
                  break;
               case AI_TYPE_MATRIX: {
                  AtMatrix val;
                  AiNodeGetMatrix(mProcNode, pname, val);
                  AiNodeSetMatrix(dst, pname, val);
                  break;
                }
               case AI_TYPE_ENUM:
                  if (ptype == AI_TYPE_STRING)
                  {
                     AiNodeSetStr(dst, pname, AiNodeGetStr(mProcNode, pname));
                  }
                  else
                  {
                    // this works whether ptype is AI_TYPE_ENUM or AI_TYPE_INT
                     AiNodeSetInt(dst, pname, AiNodeGetInt(mProcNode, pname));
                  }
                  break;
               case AI_TYPE_ARRAY: {
                  // just pass the array on?
                  AtArray *val = AiNodeGetArray(mProcNode, pname);
                  AtArray *valCopy = AiArrayCopy(val);
                  AiNodeSetArray(dst, pname, valCopy);
                  break;
               }
               default:
                  if (verbose())
                  {
                     AiMsgInfo("[abcproc] Ignored parameter \"%s\"", pname);
                  }
                  break;
               }
            }
         }
      }
      else
      {
         if (verbose())
         {
            AiMsgInfo("[abcproc]     Skip");
         }
      }
   }
   
   AiUserParamIteratorDestroy(pit);
}
