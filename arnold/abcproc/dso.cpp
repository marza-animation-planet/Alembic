#include <dso.h>
#include <visitors.h>
#include <globallock.h>
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

const char* AttributesEvaluationTimeNames[] =
{
   "render",
   "shutter",
   "shutter_open",
   "shutter_close",
   NULL
};

const char* ReferenceSourceNames[] =
{
   "attributes_then_file",
   "attributes",
   "file",
   "frame",
   NULL
};

// ---

void Dso::CommonParameters::reset()
{
   filePath = "";
   namePrefix = "";
   objectPath = "";

   frame = 0.0;
   startFrame = std::numeric_limits<double>::max();
   endFrame = -std::numeric_limits<double>::max();
   cycle = CT_hold;
   speed = 1.0;
   offset = 0.0;
   fps = 0.0;
   preserveStartFrame = false;

   samples = 1;
   expandSamplesIterations = 0;
   optimizeSamples = false;

   ignoreDeformBlur = false;
   ignoreTransformBlur = false;
   ignoreVisibility = false;
   ignoreTransforms = false;
   ignoreInstances = false;
   ignoreNurbs = false;

   outputReference = false;
   referenceSource = RS_attributes_then_file;
   referenceFilePath = "";
   referencePositionName = "Pref";
   referenceNormalName = "Nref";
   referenceFrame = -std::numeric_limits<float>::max();

   velocityScale = 1.0f;
   velocityName = "";
   accelerationName = "";
   forceVelocityBlur = false;

   removeAttributePrefices.clear();
   ignoreAttributes.clear();
   forceConstantAttributes.clear();
   attributesEvaluationTime = AET_render;

   verbose = false;
}

std::string Dso::CommonParameters::shapeKey() const
{
   std::ostringstream oss;

   oss << std::fixed << std::setprecision(6);

   if (filePath.length() > 0)
   {
      oss << " fn:" << filePath;
   }
   if (objectPath.length() > 0)
   {
      oss << " op:" << objectPath;
   }
   // Framing
   oss << " frm:" << frame;
   if (startFrame < endFrame)
   {
      oss << " sf:" << startFrame << " ef:" << endFrame;
   }
   if (cycle >= CT_hold && cycle < CT_MAX)
   {
      oss << " cyc:" << CycleTypeNames[cycle];
   }
   oss << " spd:" << speed;
   oss << " off:" << offset;
   oss << " fps:" << fps;
   oss << " psf:" << (preserveStartFrame ? "1" : "0");
   // Sampling
   oss << " smp:" << samples;
   oss << " esi:" << expandSamplesIterations;
   oss << " osm:" << (optimizeSamples ? "1" : "0");
   // Reference
   if (outputReference)
   {
      oss << " rfs:" << ReferenceSourceNames[referenceSource];
      if (referenceSource == RS_attributes || referenceSource == RS_attributes_then_file)
      {
         if (referencePositionName.length() > 0)
         {
            oss << " rfpn:" << referencePositionName;
         }
         if (referenceNormalName.length() > 0)
         {
            oss << " rfnn:" << referenceNormalName;
         }
      }
      if (referenceSource == RS_frame)
      {
         oss << " rff:" << referenceFrame;
      }
      if (referenceSource == RS_file || referenceSource == RS_attributes_then_file)
      {
         if (referenceFilePath.length() > 0)
         {
            oss << " rffn:" << referenceFilePath;
         }
      }
   }
   // Attributes
   if (removeAttributePrefices.size() > 0)
   {
      oss << " rap:";
      std::set<std::string>::const_iterator it = removeAttributePrefices.begin();
      while (it != removeAttributePrefices.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   if (forceConstantAttributes.size() > 0)
   {
      oss << " fca:";
      std::set<std::string>::const_iterator it = forceConstantAttributes.begin();
      while (it != forceConstantAttributes.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   if (ignoreAttributes.size() > 0)
   {
      oss << " ia:";
      std::set<std::string>::const_iterator it = ignoreAttributes.begin();
      while (it != ignoreAttributes.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   if (attributesEvaluationTime >= AET_render && attributesEvaluationTime < AET_MAX)
   {
      oss << " aet:" << AttributesEvaluationTimeNames[attributesEvaluationTime];
   }
   // Others
   oss << " idb:" << (ignoreDeformBlur ? "1" : "0");
   oss << " inu:" << (ignoreNurbs ? "1" : "0");
   if (!ignoreDeformBlur)
   {
      oss << " vls:" << velocityScale;
      oss << " vln:" << velocityName;
      oss << " acn:" << accelerationName;
      oss << " fvb:" << (forceVelocityBlur ? "1" : "0");
   }
   // NOTE: namePrefix doesn't change the content in terms of generated shape data

   return oss.str();
}

void Dso::SingleParameters::reset()
{
   computeTangentsForUVs.clear();

   radiusName = "";
   radiusMin = 0.0f;
   radiusMax = 1000000.0f;
   radiusScale = 1.0f;

   widthMin = 0.0f;
   widthMax = 1000000.0f;
   widthScale = 1.0f;
   nurbsSampleRate = 5;
}

std::string Dso::SingleParameters::shapeKey() const
{
   std::ostringstream oss;

   oss << std::fixed << std::setprecision(6);

   if (computeTangentsForUVs.size() > 0)
   {
      oss << " ctu:";
      std::set<std::string>::const_iterator it = computeTangentsForUVs.begin();
      while (it != computeTangentsForUVs.end())
      {
         oss << " " << *it;
         ++it;
      }
   }
   if (radiusName.length() > 0)
   {
      oss << " rdn:" << radiusName;
   }
   oss << " rdmi:" << radiusMin;
   oss << " rdma:" << radiusMax;
   oss << " rds:" << radiusScale;
   oss << " wdmi:" << widthMin;
   oss << " wdma:" << widthMax;
   oss << " wds:" << widthScale;
   oss << " nsr:" << nurbsSampleRate;

   return oss.str();
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
   , mStepSize(-1.0f)
   , mVolumePadding(0.0f)
   , mNumShapes(0)
   , mShapeKey("")
   , mInstanceNum(0)
   , mMotionStart(std::numeric_limits<double>::max())
   , mMotionEnd(-std::numeric_limits<double>::max())
   , mReverseWinding(true)
   , mGlobalFrame(0.0)
{
   mCommonParams.reset();
   mSingleParams.reset();

   readParams();

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

   // Set global frame
   if (AiNodeLookUpUserParameter(opts, "frame") != 0)
   {
      mGlobalFrame = AiNodeGetFlt(opts, "frame");
   }
   else
   {
      AiMsgWarning("[abcproc] Use abcproc node 'frame' parameter value as render frame.");
      mGlobalFrame = mCommonParams.frame;
   }

   // Compute motion start and end frames
   mMotionStart = mGlobalFrame + AiNodeGetFlt(mProcNode, Strings::motion_start);
   mMotionEnd = mGlobalFrame + AiNodeGetFlt(mProcNode, Strings::motion_end);

   // Compute shutter open and close frames
   double shutterOpenFrame = mMotionStart;
   double shutterCloseFrame = mMotionEnd;
   AtNode *cam = AiUniverseGetCamera();
   if (cam)
   {
      shutterOpenFrame = (mGlobalFrame + AiCameraGetShutterStart());
      shutterCloseFrame = (mGlobalFrame + AiCameraGetShutterEnd());
   }
   else
   {
      AiMsgWarning("[abcproc] No current camera in scene, use motion_start and motion_end time for shutter open and close time.");
   }

   // Check frame and get from options node if needed? ('frame' user attribute)
   bool useReferenceFile = mCommonParams.outputReference &&
                           (mCommonParams.referenceSource == RS_file ||
                            mCommonParams.referenceSource == RS_attributes_then_file);

   normalizeFilePath(mCommonParams.filePath);
   if (useReferenceFile && mCommonParams.referenceFilePath.length() == 0)
   {
      // look if we have a .ref file alongside the abc file path
      // this file if it exists, should contain the path to the 'reference' alembic file
      mCommonParams.referenceFilePath = getReferencePath(mCommonParams.filePath);
   }
   normalizeFilePath(mCommonParams.referenceFilePath);

   // mReverseWinding = AiNodeGetBool(opts, "CCW_points");
   mReverseWinding = true;

   // Only acquire global lock when create new alembic scenes
   //
   // Alembic libs should be build with a threadsafe version of HDF5 that will handle
   //   multi-threaded reads by itself
   // Same with Ogawa backend
   {
      AbcProcScopeLock _lock;

      char id[64];
      sprintf(id, "%p", AiThreadSelf());

      AlembicSceneFilter filter(mCommonParams.objectPath, "");

      AlembicSceneCache::SetConcurrency(size_t(AiNodeGetInt(opts, "threads")));

      mScene = AlembicSceneCache::Ref(mCommonParams.filePath, id, filter, true);

      if (useReferenceFile && mCommonParams.referenceFilePath.length() > 0)
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
                             mCommonParams.ignoreVisibility,
                             mCommonParams.ignoreNurbs);

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

         // Compute samples frame times
         std::vector<double> sampleFrames;

         mTimeSamples.clear();
         mExpandedTimeSamples.clear();

         if (ignoreMotionBlur() || (mMode == PM_single && ignoreDeformBlur()))
         {
            sampleFrames.push_back(mCommonParams.frame);
            mTimeSamples.push_back(mRenderTime);
         }
         else
         {
            if (mCommonParams.samples <= 1)
            {
               sampleFrames.push_back(mCommonParams.frame);
               mTimeSamples.push_back(mRenderTime);
            }
            else
            {
               double motionStep = (mMotionEnd - mMotionStart) / double(mCommonParams.samples - 1);
               for (unsigned int i=0; i<mCommonParams.samples; ++i)
               {
                  double sampleFrame = mMotionStart +  i * motionStep;;
                  sampleFrames.push_back(sampleFrame);
                  mTimeSamples.push_back(computeTime(sampleFrame));
               }
            }
         }

         std::vector<double> expandedSampleFrames = sampleFrames;

         // Expand samples
         if (mTimeSamples.size() > 1 && mCommonParams.expandSamplesIterations > 0)
         {
            std::vector<double> newSampleFrames;

            for (unsigned int i=0; i<mCommonParams.expandSamplesIterations; ++i)
            {
               newSampleFrames.push_back(expandedSampleFrames[0]);

               for (size_t j=1; j<expandedSampleFrames.size(); ++j)
               {
                  double mid = 0.5 * (expandedSampleFrames[j-1] + expandedSampleFrames[j]);
                  newSampleFrames.push_back(mid);
                  newSampleFrames.push_back(expandedSampleFrames[j]);
               }

               // Arnold doesn't accept more than 255 motion keys
               if (newSampleFrames.size() > 255)
               {
                  break;
               }

               std::swap(expandedSampleFrames, newSampleFrames);
               newSampleFrames.clear();
            }
         }

         // Optimize time samples
         if (mCommonParams.optimizeSamples)
         {
            // Remove samples outside of camera shutter range

            // With arnold 5, each shape node has 'motion_start' and 'motion_end'
            // attributes to specify frame relative sample ranges.
            // Samples within that range are considered uniform and shared for
            // both deformation and transformation

            std::vector<double> newSampleFrames;

            for (size_t i=0; i<expandedSampleFrames.size(); ++i)
            {
               double sampleFrame = expandedSampleFrames[i];

               if (sampleFrame < shutterOpenFrame)
               {
                  continue;
               }
               else if (sampleFrame > shutterCloseFrame)
               {
                  if (newSampleFrames.size() > 0 && newSampleFrames.back() < shutterCloseFrame)
                  {
                     newSampleFrames.push_back(sampleFrame);
                  }
               }
               else
               {
                  if (newSampleFrames.size() == 0 && i > 0 && sampleFrame > shutterOpenFrame)
                  {
                     newSampleFrames.push_back(expandedSampleFrames[i-1]);
                  }
                  newSampleFrames.push_back(sampleFrame);
               }
            }

            if (newSampleFrames.size() > 0 && newSampleFrames.size() < expandedSampleFrames.size())
            {
               std::swap(expandedSampleFrames, newSampleFrames);
               // NOTE: if transform was already sampled on the procedural, any
               //       modifications to motion_start and motion_end should influence them
               mMotionStart = expandedSampleFrames.front();
               mMotionEnd = expandedSampleFrames.back();
            }
         }

         // Convert expanded samples frames to time (in seconds)
         for (size_t i=0; i<expandedSampleFrames.size(); ++i)
         {
            mExpandedTimeSamples.push_back(computeTime(expandedSampleFrames[i]));
         }

         // Compute shape key
         mShapeKey = shapeKey();

         if (mCommonParams.verbose)
         {
            for (size_t i=0; i<mExpandedTimeSamples.size(); ++i)
            {
               AiMsgInfo("[abcproc] Add motion sample time: %lf [frame=%lf]", mExpandedTimeSamples[i], expandedSampleFrames[i]);
            }
         }

         if (mMode == PM_single)
         {
            // Read instance_num attribute
            mInstanceNum = 0;

            const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mProcNode, Strings::instance_num);
            if (pe)
            {
               int pt = AiUserParamGetType(pe);
               if (pt == AI_TYPE_INT)
               {
                  mInstanceNum = AiNodeGetInt(mProcNode, Strings::instance_num);
               }
               else if (pt == AI_TYPE_UINT)
               {
                  mInstanceNum = int(AiNodeGetUInt(mProcNode, Strings::instance_num));
               }
               else if (pt == AI_TYPE_BYTE)
               {
                  mInstanceNum = int(AiNodeGetByte(mProcNode, Strings::instance_num));
               }
            }
         }

         // Check if we should export a box for volume shading rather than a new procedural or shape
         const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mProcNode, Strings::step_size);
         if (pe)
         {
            mStepSize = AiNodeGetFlt(mProcNode, Strings::step_size);
         }

         pe = AiNodeLookUpUserParameter(mProcNode, Strings::volume_padding);
         if (pe)
         {
            mVolumePadding = AiNodeGetFlt(mProcNode, Strings::volume_padding);
         }
         else
         {
            pe = AiNodeLookUpUserParameter(mProcNode, Strings::bounds_slack);
            if (pe)
            {
               mVolumePadding = AiNodeGetFlt(mProcNode, Strings::bounds_slack);
            }
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
   AbcProcScopeLock _lock;

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
      }
      else
      {
         refPath = "";
      }
   }

   return refPath;
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
      mGeneratedNodes[idx] = node;
   }
}

double Dso::attributesTime(AttributesEvaluationTime aet) const
{
   double shutterOpenFrame = AiCameraGetShutterStart();
   double shutterCloseFrame = AiCameraGetShutterEnd();
   double shutterCenterFrame = 0.5 * (shutterOpenFrame + shutterCloseFrame);

   switch (aet)
   {
   case AET_shutter:
      return computeTime(mGlobalFrame + shutterCenterFrame);
   case AET_shutter_open:
      return computeTime(mGlobalFrame + shutterOpenFrame);
   case AET_shutter_close:
      return computeTime(mGlobalFrame + shutterCloseFrame);
   case AET_render:
   default:
      return mRenderTime;
   }
}

std::string Dso::uniqueName(const std::string &baseName) const
{
   size_t i = 0;
   std::string name = baseName;

   // should I specify the scope here (mProcNode)

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

   return t2;
}

bool Dso::cleanAttribName(std::string &name) const
{
   size_t len = name.length();
   size_t llen = 0;

   std::set<std::string>::const_iterator it = mCommonParams.removeAttributePrefices.begin();

   while (it != mCommonParams.removeAttributePrefices.end())
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

void Dso::setSingleParams(AtNode *node, const std::string &objectPath) const
{
   if (!node)
   {
      return;
   }

   size_t count;
   AtArray *array;
   std::set<std::string>::const_iterator it;

   // Common parameters
   AiNodeSetStr(node, Strings::filename, mCommonParams.filePath.c_str());
   AiNodeSetStr(node, Strings::objectpath, objectPath.c_str());
   AiNodeSetStr(node, Strings::nameprefix, mCommonParams.namePrefix.c_str());
   AiNodeSetStr(node, Strings::rootdrive, mRootDrive.c_str());
   //  Framing
   AiNodeSetFlt(node, Strings::fps, mCommonParams.fps);
   AiNodeSetFlt(node, Strings::frame, mCommonParams.frame);
   AiNodeSetFlt(node, Strings::speed, mCommonParams.speed);
   AiNodeSetFlt(node, Strings::offset, mCommonParams.offset);
   AiNodeSetFlt(node, Strings::start_frame, mCommonParams.startFrame);
   AiNodeSetFlt(node, Strings::end_frame, mCommonParams.endFrame);
   AiNodeSetBool(node, Strings::preserve_start_frame, mCommonParams.preserveStartFrame);
   AiNodeSetInt(node, Strings::cycle, (int)mCommonParams.cycle);
   //  Sampling
   AiNodeSetFlt(node, Strings::motion_start, mMotionStart - mGlobalFrame);
   AiNodeSetFlt(node, Strings::motion_end, mMotionEnd - mGlobalFrame);
   AiNodeSetUInt(node, Strings::samples, (unsigned int)mExpandedTimeSamples.size());
   AiNodeSetUInt(node, Strings::expand_samples_iterations, 0); // mCommonParams.expandSamplesIterations);
   AiNodeSetBool(node, Strings::optimize_samples, false); // mCommonParams.optimizeSamples);
   //  Ignores
   AiNodeSetBool(node, Strings::ignore_deform_blur, mCommonParams.ignoreDeformBlur);
   AiNodeSetBool(node, Strings::ignore_transform_blur, mCommonParams.ignoreTransformBlur);
   AiNodeSetBool(node, Strings::ignore_visibility, true);
   AiNodeSetBool(node, Strings::ignore_instances, true);
   AiNodeSetBool(node, Strings::ignore_transforms, true);
   AiNodeSetBool(node, Strings::ignore_nurbs, mCommonParams.ignoreNurbs);
   //  Reference data
   AiNodeSetBool(node, Strings::output_reference, mCommonParams.outputReference);
   AiNodeSetStr(node, Strings::reference_filename, mCommonParams.referenceFilePath.c_str());
   AiNodeSetInt(node, Strings::reference_source, (int)mCommonParams.referenceSource);
   AiNodeSetFlt(node, Strings::reference_frame, mCommonParams.referenceFrame);
   AiNodeSetStr(node, Strings::reference_position_name, mCommonParams.referencePositionName.c_str());
   AiNodeSetStr(node, Strings::reference_normal_name, mCommonParams.referenceNormalName.c_str());
   //  Velocity data
   AiNodeSetStr(node, Strings::velocity_name, mCommonParams.velocityName.c_str());
   AiNodeSetStr(node, Strings::acceleration_name, mCommonParams.accelerationName.c_str());
   AiNodeSetFlt(node, Strings::velocity_scale, mCommonParams.velocityScale);
   AiNodeSetBool(node, Strings::force_velocity_blur, mCommonParams.forceVelocityBlur);
   //  Others
   count = mCommonParams.removeAttributePrefices.size();
   array = AiArrayAllocate(count, 1, AI_TYPE_STRING);
   it = mCommonParams.removeAttributePrefices.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, Strings::remove_attribute_prefices, array);
   count = mCommonParams.forceConstantAttributes.size();
   array = AiArrayAllocate(count, 1, AI_TYPE_STRING);
   it = mCommonParams.forceConstantAttributes.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, Strings::force_constant_attributes, array);
   count = mCommonParams.ignoreAttributes.size();
   array = AiArrayAllocate(count, 1, AI_TYPE_STRING);
   it = mCommonParams.ignoreAttributes.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, Strings::ignore_attributes, array);
   AiNodeSetInt(node, Strings::attributes_evaluation_time, (int)mCommonParams.attributesEvaluationTime);
   AiNodeSetBool(node, Strings::verbose, mCommonParams.verbose);

   // Single shape mode parameters
   count = mSingleParams.computeTangentsForUVs.size();
   array = AiArrayAllocate(count, 1, AI_TYPE_STRING);
   it = mSingleParams.computeTangentsForUVs.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, Strings::compute_tangents_for_uvs, array);
   AiNodeSetStr(node, Strings::radius_name, mSingleParams.radiusName.c_str());
   AiNodeSetFlt(node, Strings::radius_min, mSingleParams.radiusMin);
   AiNodeSetFlt(node, Strings::radius_max, mSingleParams.radiusMax);
   AiNodeSetFlt(node, Strings::radius_scale, mSingleParams.radiusScale);
   AiNodeSetFlt(node, Strings::width_min, mSingleParams.widthMin);
   AiNodeSetFlt(node, Strings::width_max, mSingleParams.widthMax);
   AiNodeSetFlt(node, Strings::width_scale, mSingleParams.widthScale);
   AiNodeSetUInt(node, Strings::nurbs_sample_rate, mSingleParams.nurbsSampleRate);
}

void Dso::readParams()
{
   AtArray *array = 0;
   std::string str;

   // Drive for directory mapping
   str = AiNodeGetStr(mProcNode, Strings::rootdrive).c_str();
   if (str.length() == 1 &&
       ((str[0] >= 'a' && str[0] <= 'z') ||
        (str[0] >= 'A' && str[0] <= 'Z')))
   {
      mRootDrive = str + ":";
   }
   else if (str.length() > 0)
   {
      AiMsgWarning("[abcproc] 'rootdrive' parameter value should be a single letter");
   }

   // Common parameters
   mCommonParams.filePath = AiNodeGetStr(mProcNode, Strings::filename).c_str();
   mCommonParams.objectPath = AiNodeGetStr(mProcNode, Strings::objectpath).c_str();
   mCommonParams.namePrefix = AiNodeGetStr(mProcNode, Strings::nameprefix).c_str();
   //  Framing
   mCommonParams.fps = AiNodeGetFlt(mProcNode, Strings::fps);
   mCommonParams.frame = AiNodeGetFlt(mProcNode, Strings::frame);
   mCommonParams.speed = AiNodeGetFlt(mProcNode, Strings::speed);
   mCommonParams.offset = AiNodeGetFlt(mProcNode, Strings::offset);
   mCommonParams.startFrame = AiNodeGetFlt(mProcNode, Strings::start_frame);
   mCommonParams.endFrame = AiNodeGetFlt(mProcNode, Strings::end_frame);
   mCommonParams.preserveStartFrame = AiNodeGetBool(mProcNode, Strings::preserve_start_frame);
   mCommonParams.cycle = (CycleType) AiNodeGetInt(mProcNode, Strings::cycle);
   //  Sampling
   mCommonParams.samples = AiNodeGetUInt(mProcNode, Strings::samples);
   mCommonParams.expandSamplesIterations = AiNodeGetUInt(mProcNode, Strings::expand_samples_iterations);
   mCommonParams.optimizeSamples = AiNodeGetBool(mProcNode, Strings::optimize_samples);
   //  Ignores
   mCommonParams.ignoreDeformBlur = (AiNodeGetBool(mProcNode, Strings::ignore_deform_blur) || mCommonParams.ignoreDeformBlur);
   mCommonParams.ignoreTransformBlur = (AiNodeGetBool(mProcNode, Strings::ignore_transform_blur) || mCommonParams.ignoreTransformBlur);
   mCommonParams.ignoreVisibility = AiNodeGetBool(mProcNode, Strings::ignore_visibility);
   mCommonParams.ignoreInstances = AiNodeGetBool(mProcNode, Strings::ignore_instances);
   mCommonParams.ignoreTransforms = AiNodeGetBool(mProcNode, Strings::ignore_transforms);
   mCommonParams.ignoreNurbs = AiNodeGetBool(mProcNode, Strings::ignore_nurbs);
   //  Reference data
   mCommonParams.outputReference = AiNodeGetBool(mProcNode, Strings::output_reference);
   mCommonParams.referenceFilePath = AiNodeGetStr(mProcNode, Strings::reference_filename).c_str();
   mCommonParams.referenceSource = (ReferenceSource) AiNodeGetInt(mProcNode, Strings::reference_source);
   mCommonParams.referenceFrame = AiNodeGetFlt(mProcNode, Strings::reference_frame);
   mCommonParams.referencePositionName = AiNodeGetStr(mProcNode, Strings::reference_position_name).c_str();
   if (mCommonParams.referencePositionName.length() == 0)
   {
      mCommonParams.referencePositionName = "Pref";
   }
   mCommonParams.referenceNormalName = AiNodeGetStr(mProcNode, Strings::reference_normal_name).c_str();
   if (mCommonParams.referenceNormalName.length() == 0)
   {
      mCommonParams.referenceNormalName = "Nref";
   }
   //  Velocity data
   mCommonParams.velocityName = AiNodeGetStr(mProcNode, Strings::velocity_name).c_str();
   mCommonParams.accelerationName = AiNodeGetStr(mProcNode, Strings::acceleration_name).c_str();
   mCommonParams.velocityScale = AiNodeGetFlt(mProcNode, Strings::velocity_scale);
   mCommonParams.forceVelocityBlur = AiNodeGetBool(mProcNode, Strings::force_velocity_blur);
   // Attributes
   mCommonParams.attributesEvaluationTime = (AttributesEvaluationTime) AiNodeGetInt(mProcNode, Strings::attributes_evaluation_time);
   mCommonParams.removeAttributePrefices.clear();
   array = AiNodeGetArray(mProcNode, Strings::remove_attribute_prefices);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mCommonParams.removeAttributePrefices.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   mCommonParams.ignoreAttributes.clear();
   array = AiNodeGetArray(mProcNode, Strings::ignore_attributes);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mCommonParams.ignoreAttributes.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   mCommonParams.forceConstantAttributes.clear();
   array = AiNodeGetArray(mProcNode, Strings::force_constant_attributes);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mCommonParams.forceConstantAttributes.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   //  Others
   mCommonParams.verbose = AiNodeGetBool(mProcNode, Strings::verbose);

   // Single shape mode parameters
   mSingleParams.computeTangentsForUVs.clear();
   array = AiNodeGetArray(mProcNode, Strings::compute_tangents_for_uvs);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mSingleParams.computeTangentsForUVs.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   mSingleParams.radiusName = AiNodeGetStr(mProcNode, Strings::radius_name).c_str();
   mSingleParams.radiusMin = AiNodeGetFlt(mProcNode, Strings::radius_min);
   mSingleParams.radiusMax = AiNodeGetFlt(mProcNode, Strings::radius_max);
   mSingleParams.radiusScale = AiNodeGetFlt(mProcNode, Strings::radius_scale);
   mSingleParams.widthMin = AiNodeGetFlt(mProcNode, Strings::width_min);
   mSingleParams.widthMax = AiNodeGetFlt(mProcNode, Strings::width_max);
   mSingleParams.widthScale = AiNodeGetFlt(mProcNode, Strings::width_scale);
   mSingleParams.nurbsSampleRate = AiNodeGetUInt(mProcNode, Strings::nurbs_sample_rate);
}

void Dso::transferInstanceParams(AtNode *dst)
{
   AiNodeSetInt(dst, Strings::id, AiNodeGetInt(mProcNode, Strings::id));
   AiNodeSetBool(dst, Strings::receive_shadows, AiNodeGetBool(mProcNode, Strings::receive_shadows));
   AiNodeSetBool(dst, Strings::self_shadows, AiNodeGetBool(mProcNode, Strings::self_shadows));
   AiNodeSetBool(dst, Strings::invert_normals, AiNodeGetBool(mProcNode, Strings::invert_normals));
   AiNodeSetBool(dst, Strings::opaque, AiNodeGetBool(mProcNode, Strings::opaque));
   AiNodeSetBool(dst, Strings::matte, AiNodeGetBool(mProcNode, Strings::matte));
   AiNodeSetBool(dst, Strings::use_light_group, AiNodeGetBool(mProcNode, Strings::use_light_group));
   AiNodeSetBool(dst, Strings::use_shadow_group, AiNodeGetBool(mProcNode, Strings::use_shadow_group));
   AiNodeSetFlt(dst, Strings::ray_bias, AiNodeGetFlt(mProcNode, Strings::ray_bias));
   AiNodeSetByte(dst, Strings::visibility, AiNodeGetByte(mProcNode, Strings::visibility));
   AiNodeSetByte(dst, Strings::sidedness, AiNodeGetByte(mProcNode, Strings::sidedness));
   AiNodeSetFlt(dst, Strings::motion_start, AiNodeGetFlt(mProcNode, Strings::motion_start));
   AiNodeSetFlt(dst, Strings::motion_end, AiNodeGetFlt(mProcNode, Strings::motion_end));
   AiNodeSetInt(dst, Strings::transform_type, AiNodeGetInt(mProcNode, Strings::transform_type));

   AtArray *ary;

   ary = AiNodeGetArray(mProcNode, Strings::shader);
   if (ary)
   {
      AiNodeSetArray(dst, Strings::shader, AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, Strings::light_group);
   if (ary)
   {
      AiNodeSetArray(dst, Strings::light_group, AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, Strings::shadow_group);
   if (ary)
   {
      AiNodeSetArray(dst, Strings::shadow_group, AiArrayCopy(ary));
   }

   ary = AiNodeGetArray(mProcNode, Strings::trace_sets);
   if (ary)
   {
      AiNodeSetArray(dst, Strings::trace_sets, AiArrayCopy(ary));
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
      // Shouldn't 'cleanAttrName' be called here?
      // => No, those are only for attributes contained in alembic the alembic file
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
      else
      {
         if (ignoreAttribute(pname))
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
         // Unless it is forced constant?
         // => No, only for geoparams read from the alembic file
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
            case AI_TYPE_VECTOR2:
               if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " VECTOR2").c_str());
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
               switch (AiArrayGetType(val))
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
               case AI_TYPE_VECTOR2:
                  if (doDeclare) AiNodeDeclare(dst, pname, (std::string(sDeclBase[pcat]) + " ARRAY VECTOR2").c_str());
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
            if (// int -> enum, string -> enum
                (mptype == AI_TYPE_ENUM && (ptype == AI_TYPE_INT || ptype == AI_TYPE_STRING)) ||
                // string -> node
                (mptype == AI_TYPE_NODE && ptype == AI_TYPE_STRING) ||
                // int -> byte
                (mptype == AI_TYPE_BYTE && ptype == AI_TYPE_INT) ||
                // byte -> int
                (mptype == AI_TYPE_INT && ptype == AI_TYPE_BYTE))
            {
               // Noop
            }
            else
            {
               doCopy = false;
               if (mptype == AI_TYPE_ARRAY && ptype == AI_TYPE_STRING)
               {
                  AtArray *a = AiNodeGetArray(dst, pname);
                  if (a && AiArrayGetType(a) == AI_TYPE_NODE)
                  {
                     // string -> node[]
                     doCopy = true;
                  }
               }
               if (!doCopy)
               {
                  AiMsgWarning("[abcproc]     Parameter type mismatch \"%s\": %s on destination, %s on source", pname, AiParamGetTypeName(AiParamGetType(pe)), AiParamGetTypeName(ptype));
               }
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
                  if (AiNodeIs(dst, (AtString)"polymesh"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "nsides");
                     if (AiArrayGetNumElements(val) != AiArrayGetNumElements(a))
                     {
                        AiMsgWarning("[abcproc]     Polygon count mismatch for uniform attribute");
                        continue;
                     }
                  }
                  else if (AiNodeIs(dst, (AtString)"curves"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "num_points");
                     if (AiArrayGetNumElements(val) != AiArrayGetNumElements(a))
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
                  if (AiNodeIs(dst, (AtString)"polymesh"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "vlist");
                     if (AiArrayGetNumElements(val) != AiArrayGetNumElements(a))
                     {
                        AiMsgWarning("[gto DSO]     Point count mismatch for varying attribute");
                        continue;
                     }
                  }
                  else if (AiNodeIs(dst, (AtString)"curves"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "points");
                     if (AiArrayGetNumElements(val) != AiArrayGetNumElements(a))
                     {
                        AiMsgWarning("[gto DSO]     Point count mismatch for varying attribute");
                        continue;
                     }
                  }
                  else if (AiNodeIs(dst, (AtString)"points"))
                  {
                     AtArray *a = AiNodeGetArray(dst, "points");
                     if (AiArrayGetNumElements(val) != AiArrayGetNumElements(a))
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
                  if (AiNodeIs(dst, (AtString)"polymesh"))
                  {
                     // get the idxs attribute count
                     AtArray *a = AiNodeGetArray(dst, "vidxs");
                     size_t plen = strlen(pname);
                     if (plen > 4 && !strncmp(pname + plen - 4, "idxs", 4))
                     {
                        // direcly compare size
                        if (AiArrayGetNumElements(val) != AiArrayGetNumElements(a))
                        {
                           AiMsgWarning("[gto DSO]     Count mismatch for indexed attribute");
                           continue;
                        }
                     }
                     else
                     {
                        // get idxs attribute and compare size [if exists]
                        AtArray *idxs = AiNodeGetArray(mProcNode, indexAttr.c_str());
                        if (!idxs || AiArrayGetNumElements(idxs) != AiArrayGetNumElements(a))
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
               case AI_TYPE_VECTOR2: {
                  AtVector2 val = AiNodeGetVec2(mProcNode, pname);
                  AiNodeSetVec2(dst, pname, val.x, val.y);
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
                     const char *nn = AiNodeGetStr(mProcNode, pname).c_str();
                     AtNode *n = AiNodeLookUpByName(nn);
                     AiNodeSetPtr(dst, pname, (void*)n);
                  }
                  else
                  {
                     AiNodeSetPtr(dst, pname, AiNodeGetPtr(mProcNode, pname));
                  }
                  break;
               case AI_TYPE_MATRIX: {
                  AtMatrix val = AiNodeGetMatrix(mProcNode, pname);
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
                  if (ptype == AI_TYPE_STRING)
                  {
                     // The only special case for mptype == AI_TYPE_ARRAY
                     const char *nn = AiNodeGetStr(mProcNode, pname);
                     AtNode *n = AiNodeLookUpByName(nn);
                     AtArray *nodes = AiArrayAllocate(1, 1, AI_TYPE_NODE);
                     AiArraySetPtr(nodes, 0, n);
                     AiNodeSetArray(dst, pname, nodes);
                  }
                  else
                  {
                     // just pass the array on?
                     AtArray *val = AiNodeGetArray(mProcNode, pname);
                     AtArray *valCopy = AiArrayCopy(val);
                     AiNodeSetArray(dst, pname, valCopy);
                  }
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
