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
   
const char* AttributeFrameNames[] =
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

// Common 
AtString Dso::p_filename("filename");
AtString Dso::p_objectpath("objectpath");
AtString Dso::p_frame("frame");
AtString Dso::p_samples("samples");
AtString Dso::p_relative_samples("relative_samples");
AtString Dso::p_fps("fps");
AtString Dso::p_cycle("cycle");
AtString Dso::p_start_frame("start_frame");
AtString Dso::p_end_frame("end_frame");
AtString Dso::p_speed("speed");
AtString Dso::p_offset("offset");
AtString Dso::p_preserve_start_frame("preserve_start_frame");

//AtString Dso::p_ignore_motion_blur("ignore_motion_blur");
AtString Dso::p_ignore_deform_blur("ignore_deform_blur");
AtString Dso::p_ignore_transform_blur("ignore_transform_blur");
AtString Dso::p_ignore_visibility("ignore_visibility");
AtString Dso::p_ignore_transforms("ignore_transforms");
AtString Dso::p_ignore_instances("ignore_instances");
AtString Dso::p_ignore_nurbs("ignore_nurbs");
AtString Dso::p_velocity_scale("velocity_scale");
AtString Dso::p_velocity_name("velocity_name");
AtString Dso::p_acceleration_name("acceleration_name");
AtString Dso::p_force_velocity_blur("force_velocity_blur");
AtString Dso::p_output_reference("output_reference");
AtString Dso::p_reference_source("reference_source");
AtString Dso::p_reference_position_name("reference_position_name");
AtString Dso::p_reference_normal_name("reference_normal_name");
AtString Dso::p_reference_filename("reference_filename");
AtString Dso::p_reference_frame("reference_frame");
AtString Dso::p_demote_to_object_attribute("demote_to_object_attribute");
AtString Dso::p_samples_expand_iterations("samples_expand_iterations");
AtString Dso::p_optimize_samples("optimize_samples");
AtString Dso::p_nameprefix("nameprefix");

// Multi shapes parameters
AtString Dso::p_bounds_padding("bounds_padding");
AtString Dso::p_compute_velocity_expanded_bounds("compute_velocity_expanded_bounds");
AtString Dso::p_use_override_bounds("use_override_bounds");
AtString Dso::p_override_bounds_min_name("override_bounds_min_name");
AtString Dso::p_override_bounds_max_name("override_bounds_max_name");
AtString Dso::p_pad_bounds_with_peak_radius("pad_bounds_with_peak_radius");
AtString Dso::p_peak_radius_name("peak_radius_name");
AtString Dso::p_pad_bounds_with_peak_width("pad_bounds_with_peak_width");
AtString Dso::p_peak_width_name("peak_width_name");
AtString Dso::p_override_attributes("override_attributes");

// Single shape parameters
AtString Dso::p_read_object_attributes("read_object_attributes");
AtString Dso::p_read_primitive_attributes("read_primitive_attributes");
AtString Dso::p_read_point_attributes("read_point_attributes");
AtString Dso::p_read_vertex_attributes("read_vertex_attributes");
AtString Dso::p_attributes_frame("attributes_frame");
AtString Dso::p_attribute_prefices_to_remove("attribute_prefices_to_remove");
AtString Dso::p_compute_tangents("compute_tangents");
AtString Dso::p_radius_name("radius_name");
AtString Dso::p_radius_min("radius_min");
AtString Dso::p_radius_max("radius_max");
AtString Dso::p_radius_scale("radius_scale");
AtString Dso::p_width_min("width_min");
AtString Dso::p_width_max("width_max");
AtString Dso::p_width_scale("width_scale");
AtString Dso::p_nurbs_sample_rate("nurbs_sample_rate");

// Others
AtString Dso::p_verbose("verbose");
AtString Dso::p_rootdrive("rootdrive");

// ---

void Dso::CommonParameters::reset()
{
   filePath = "";
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

   referenceSource = RS_attributes_then_file;
   referenceFilePath = "";
   referencePositionName = "Pref";
   referenceNormalName = "Nref";
   referenceFrame = -std::numeric_limits<float>::max();
   
   velocityScale = 1.0f;
   velocityName = "";
   accelerationName = "";
   
   forceVelocityBlur = false;

   promoteToObjectAttribs.clear();
   
   ignoreNurbs = false;
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
      oss << " -outputreference -referencesource " << ReferenceSourceNames[referenceSource];
      if (referenceSource == RS_attributes || referenceSource == RS_attributes_then_file)
      {
         if (referencePositionName.length() > 0)
         {
            oss << " -referencepositionname " << referencePositionName;
         }
         if (referenceNormalName.length() > 0)
         {
            oss << " -referencenormalname " << referenceNormalName;
         }
      }
      if (referenceSource == RS_frame)
      {
         oss << " -referenceframe " << referenceFrame;
      }
      if (referenceSource == RS_file || referenceSource == RS_attributes_then_file)
      {
         if (referenceFilePath.length() > 0)
         {
            oss << " -referencefilename " << referenceFilePath;
         }
      }
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
   else
   {
      oss << " -velocityscale " << velocityScale;
      if (velocityName.length() > 0)
      {
         oss << " -velocityname " << velocityName;
      }
      if (accelerationName.length() > 0)
      {
         oss << " -accelerationname " << accelerationName;
      }
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
   if (ignoreNurbs)
   {
      oss << " -ignorenurbs";
   }
   if (forceVelocityBlur)
   {
      oss << " -forcevelocityblur";
   }
   
   return oss.str();
}

void Dso::MultiParameters::reset()
{
   overrideAttribs.clear();
   
   boundsPadding = 0.0f;
   
   computeVelocityExpandedBounds = false;
   
   useOverrideBounds = false;
   overrideBoundsMinName = "overrideBoundsMin";
   overrideBoundsMaxName = "overrideBoundsMax";
   
   padBoundsWithPeakRadius = false;
   peakRadiusName = "peakRadius";
   
   padBoundsWithPeakWidth = false;
   peakWidthName = "peakWidth";
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
   if (radiusName.length() > 0)
   {
      oss << " -radiusname " << radiusName;
   }
   oss << " -radiusmin " << radiusMin;
   oss << " -radiusmax " << radiusMax;
   oss << " -radiusscale " << radiusScale;
   oss << " -nurbssamplerate " << nurbsSampleRate;
   oss << " -widthmin " << widthMin;
   oss << " -widthmax " << widthMax;
   oss << " -widthscale " << widthScale;
   
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
   , mSetMotionRange(false)
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
   mMultiParams.reset();
   
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
            if (mCommonParams.samples.size() == 0)
            {
               AiMsgWarning("[abcproc] No samples set, use render frame");
               sampleFrames.push_back(mCommonParams.frame);
               mTimeSamples.push_back(mRenderTime);
            }
            else
            {
               if (mCommonParams.relativeSamples)
               {
                  for (size_t i=0; i<mCommonParams.samples.size(); ++i)
                  {
                     double sampleFrame = mCommonParams.frame + mCommonParams.samples[i];
                     sampleFrames.push_back(sampleFrame);
                     mTimeSamples.push_back(computeTime(sampleFrame));
                  }
               }
               else
               {
                  for (size_t i=0; i<mCommonParams.samples.size(); ++i)
                  {
                     double sampleFrame = mCommonParams.samples[i];
                     sampleFrames.push_back(sampleFrame);
                     mTimeSamples.push_back(computeTime(sampleFrame));
                  }
               }
            }
         }

         std::vector<double> expandedSampleFrames = sampleFrames;

         // Expand samples
         if (mTimeSamples.size() > 1 && mCommonParams.samplesExpandIterations > 0)
         {
            std::vector<double> newSampleFrames;

            for (int i=0; i<mCommonParams.samplesExpandIterations; ++i)
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
         if ((expandedSampleFrames.size() > sampleFrames.size()) && mCommonParams.optimizeSamples)
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
               mSetMotionRange = true;
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
               AiMsgInfo("[abcproc] Add motion sample time: %lf", mExpandedTimeSamples[i]);
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
      if (mSetMotionRange)
      {
         AiNodeSetFlt(node, Strings::motion_start, mMotionStart);
         AiNodeSetFlt(node, Strings::motion_end, mMotionEnd);
      }
      
      mGeneratedNodes[idx] = node;
   }
}

double Dso::attribsTime(AttributeFrame af) const
{
   double shutterOpenFrame = AiCameraGetShutterStart();
   double shutterCloseFrame = AiCameraGetShutterEnd();
   double shutterCenterFrame = 0.5 * (shutterOpenFrame + shutterCloseFrame);
   
   switch (af)
   {
   case AF_shutter:
      return computeTime(mMotionStart + shutterCenterFrame * (mMotionEnd - mMotionStart));
   case AF_shutter_open:
      return computeTime(mMotionStart + shutterOpenFrame * (mMotionEnd - mMotionStart));
   case AF_shutter_close:
      return computeTime(mMotionStart + shutterCloseFrame * (mMotionEnd - mMotionStart));
   case AF_render:
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
   AiNodeSetStr(node, p_filename, mCommonParams.filePath.c_str());
   AiNodeSetStr(node, p_objectpath, objectPath.c_str());
   AiNodeSetStr(node, p_nameprefix, mCommonParams.namePrefix.c_str());
   AiNodeSetStr(node, p_rootdrive, mRootDrive.c_str());
   //  Framing
   AiNodeSetFlt(node, p_fps, mCommonParams.fps);
   AiNodeSetFlt(node, p_frame, mCommonParams.frame);
   AiNodeSetFlt(node, p_speed, mCommonParams.speed);
   AiNodeSetFlt(node, p_offset, mCommonParams.offset);
   AiNodeSetFlt(node, p_start_frame, mCommonParams.startFrame);
   AiNodeSetFlt(node, p_end_frame, mCommonParams.endFrame);
   AiNodeSetBool(node, p_preserve_start_frame, mCommonParams.preserveStartFrame);
   AiNodeSetInt(node, p_cycle, (int)mCommonParams.cycle);
   //  Sampling
   AiNodeSetBool(node, p_relative_samples, mCommonParams.relativeSamples);
   count = mCommonParams.samples.size();
   array = AiArray(count, 1, AI_TYPE_FLOAT);
   for (size_t i=0; i<count; ++i)
   {
      AiArraySetFlt(array, i, mCommonParams.samples[i]);
   }
   AiNodeSetArray(node, p_samples, array);
   AiNodeSetInt(node, p_samples_expand_iterations, mCommonParams.samplesExpandIterations);
   AiNodeSetBool(node, p_optimize_samples, mCommonParams.optimizeSamples);
   //  Ignores
   AiNodeSetBool(node, p_ignore_deform_blur, mCommonParams.ignoreDeformBlur);
   AiNodeSetBool(node, p_ignore_transform_blur, mCommonParams.ignoreTransformBlur);
   AiNodeSetBool(node, p_ignore_visibility, true);
   AiNodeSetBool(node, p_ignore_instances, true);
   AiNodeSetBool(node, p_ignore_transforms, true);
   AiNodeSetBool(node, p_ignore_nurbs, mCommonParams.ignoreNurbs);
   //  Reference data
   AiNodeSetBool(node, p_output_reference, mCommonParams.outputReference);
   AiNodeSetStr(node, p_reference_filename, mCommonParams.referenceFilePath.c_str());
   AiNodeSetInt(node, p_reference_source, (int)mCommonParams.referenceSource);
   AiNodeSetFlt(node, p_reference_frame, mCommonParams.referenceFrame);
   AiNodeSetStr(node, p_reference_position_name, mCommonParams.referencePositionName.c_str());
   AiNodeSetStr(node, p_reference_normal_name, mCommonParams.referenceNormalName.c_str());
   //  Velocity data
   AiNodeSetStr(node, p_velocity_name, mCommonParams.velocityName.c_str());
   AiNodeSetStr(node, p_acceleration_name, mCommonParams.accelerationName.c_str());
   AiNodeSetFlt(node, p_velocity_scale, mCommonParams.velocityScale);
   AiNodeSetBool(node, p_force_velocity_blur, mCommonParams.forceVelocityBlur);
   //  Others
   count = mCommonParams.promoteToObjectAttribs.size();
   array = AiArray(count, 1, AI_TYPE_STRING);
   it = mCommonParams.promoteToObjectAttribs.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, p_demote_to_object_attribute, array);
   AiNodeSetBool(node, p_verbose, mCommonParams.verbose);

   // Single shape mode parameters
   AiNodeSetBool(node, p_read_object_attributes, mSingleParams.readObjectAttribs);
   AiNodeSetBool(node, p_read_primitive_attributes, mSingleParams.readPrimitiveAttribs);
   AiNodeSetBool(node, p_read_point_attributes, mSingleParams.readPointAttribs);
   AiNodeSetBool(node, p_read_vertex_attributes, mSingleParams.readVertexAttribs);
   AiNodeSetInt(node, p_attributes_frame, (int)mSingleParams.attribsFrame);
   count = mSingleParams.attribPreficesToRemove.size();
   array = AiArray(count, 1, AI_TYPE_STRING);
   it = mSingleParams.attribPreficesToRemove.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, p_attribute_prefices_to_remove, array);
   count = mSingleParams.computeTangents.size();
   array = AiArray(count, 1, AI_TYPE_STRING);
   it = mSingleParams.computeTangents.begin();
   for (size_t i=0; i<count; ++i, ++it)
   {
      AiArraySetStr(array, i, it->c_str());
   }
   AiNodeSetArray(node, p_compute_tangents, array);
   AiNodeSetStr(node, p_radius_name, mSingleParams.radiusName.c_str());
   AiNodeSetFlt(node, p_radius_min, mSingleParams.radiusMin);
   AiNodeSetFlt(node, p_radius_max, mSingleParams.radiusMax);
   AiNodeSetFlt(node, p_radius_scale, mSingleParams.radiusScale);
   AiNodeSetFlt(node, p_width_min, mSingleParams.widthMin);
   AiNodeSetFlt(node, p_width_max, mSingleParams.widthMax);
   AiNodeSetFlt(node, p_width_scale, mSingleParams.widthScale);
   AiNodeSetInt(node, p_nurbs_sample_rate, mSingleParams.nurbsSampleRate);

   // Those two?
   // AiNodeSetFlt(node, Strings::motion_start, mMotionStart);
   // AiNodeSetFlt(node, Strings::motion_end, mMotionEnd);
}

void Dso::readParams()
{
   bool relativeSamples = false;
   bool samplesSet = false;
   AtArray *array = 0;
   std::string str;

   // Drive for directory mapping
   str = AiNodeGetStr(mProcNode, p_rootdrive).c_str();
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
   mCommonParams.filePath = AiNodeGetStr(mProcNode, p_filename).c_str();
   mCommonParams.objectPath = AiNodeGetStr(mProcNode, p_objectpath).c_str();
   mCommonParams.namePrefix = AiNodeGetStr(mProcNode, p_nameprefix).c_str();
   //  Framing
   mCommonParams.fps = AiNodeGetFlt(mProcNode, p_fps);
   mCommonParams.frame = AiNodeGetFlt(mProcNode, p_frame);
   mCommonParams.speed = AiNodeGetFlt(mProcNode, p_speed);
   mCommonParams.offset = AiNodeGetFlt(mProcNode, p_offset);
   mCommonParams.startFrame = AiNodeGetFlt(mProcNode, p_start_frame);
   mCommonParams.endFrame = AiNodeGetFlt(mProcNode, p_end_frame);
   mCommonParams.preserveStartFrame = AiNodeGetBool(mProcNode, p_preserve_start_frame);
   mCommonParams.cycle = (CycleType) AiNodeGetInt(mProcNode, p_cycle);
   //  Sampling
   relativeSamples = AiNodeGetBool(mProcNode, p_relative_samples);
   array = AiNodeGetArray(mProcNode, p_samples);
   if (array)
   {
      unsigned int ne = AiArrayGetNumElements(array);
      unsigned int nk = AiArrayGetNumKeys(array);
      if (ne * nk > 0)
      {
         mCommonParams.samples.clear();
         // Either number of keys or number of elements
         if (nk > 1)
         {
            ne = 1;
         }
         for (unsigned int i=0, j=0; i<AiArrayGetNumKeys(array); ++i, j+=AiArrayGetNumElements(array))
         {
            for (unsigned int k=0; k<ne; ++k)
            {
               mCommonParams.samples.push_back(AiArrayGetFlt(array, j+k));
            }
         }
         samplesSet = true;
      }
   }
   mCommonParams.samplesExpandIterations = AiNodeGetInt(mProcNode, p_samples_expand_iterations);
   mCommonParams.optimizeSamples = AiNodeGetBool(mProcNode, p_optimize_samples);
   //  Ignores
   mCommonParams.ignoreDeformBlur = (AiNodeGetBool(mProcNode, p_ignore_deform_blur) || mCommonParams.ignoreDeformBlur);
   mCommonParams.ignoreTransformBlur = (AiNodeGetBool(mProcNode, p_ignore_transform_blur) || mCommonParams.ignoreTransformBlur);
   mCommonParams.ignoreVisibility = AiNodeGetBool(mProcNode, p_ignore_visibility);
   mCommonParams.ignoreInstances = AiNodeGetBool(mProcNode, p_ignore_instances);
   mCommonParams.ignoreTransforms = AiNodeGetBool(mProcNode, p_ignore_transforms);
   mCommonParams.ignoreNurbs = AiNodeGetBool(mProcNode, p_ignore_nurbs);
   //  Reference data
   mCommonParams.outputReference = AiNodeGetBool(mProcNode, p_output_reference);
   mCommonParams.referenceFilePath = AiNodeGetStr(mProcNode, p_reference_filename).c_str();
   mCommonParams.referenceSource = (ReferenceSource) AiNodeGetInt(mProcNode, p_reference_source);
   mCommonParams.referenceFrame = AiNodeGetFlt(mProcNode, p_reference_frame);
   mCommonParams.referencePositionName = AiNodeGetStr(mProcNode, p_reference_position_name).c_str();
   if (mCommonParams.referencePositionName.length() == 0)
   {
      mCommonParams.referencePositionName = "Pref";
   }
   mCommonParams.referenceNormalName = AiNodeGetStr(mProcNode, p_reference_normal_name).c_str();
   if (mCommonParams.referenceNormalName.length() == 0)
   {
      mCommonParams.referenceNormalName = "Nref";
   }
   //  Velocity data
   mCommonParams.velocityName = AiNodeGetStr(mProcNode, p_velocity_name).c_str();
   mCommonParams.accelerationName = AiNodeGetStr(mProcNode, p_acceleration_name).c_str();
   mCommonParams.velocityScale = AiNodeGetFlt(mProcNode, p_velocity_scale);
   mCommonParams.forceVelocityBlur = AiNodeGetBool(mProcNode, p_force_velocity_blur);
   //  Others
   mCommonParams.promoteToObjectAttribs.clear();
   array = AiNodeGetArray(mProcNode, p_demote_to_object_attribute);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mCommonParams.promoteToObjectAttribs.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   mCommonParams.verbose = AiNodeGetBool(mProcNode, p_verbose);

   // Multi mode specific parameters
   mMultiParams.boundsPadding = AiNodeGetFlt(mProcNode, p_bounds_padding);
   mMultiParams.computeVelocityExpandedBounds = AiNodeGetBool(mProcNode, p_compute_velocity_expanded_bounds);
   mMultiParams.useOverrideBounds = AiNodeGetBool(mProcNode, p_use_override_bounds);
   mMultiParams.overrideBoundsMinName = AiNodeGetStr(mProcNode, p_override_bounds_min_name).c_str();
   mMultiParams.overrideBoundsMaxName = AiNodeGetStr(mProcNode, p_override_bounds_max_name).c_str();
   mMultiParams.padBoundsWithPeakRadius = AiNodeGetBool(mProcNode, p_pad_bounds_with_peak_width);
   mMultiParams.peakRadiusName = AiNodeGetStr(mProcNode, p_peak_radius_name).c_str();
   mMultiParams.padBoundsWithPeakWidth = AiNodeGetBool(mProcNode, p_pad_bounds_with_peak_width);
   mMultiParams.peakWidthName = AiNodeGetStr(mProcNode, p_peak_width_name).c_str();
   mMultiParams.overrideAttribs.clear();
   array = AiNodeGetArray(mProcNode, p_override_attributes);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mMultiParams.overrideAttribs.insert(AiArrayGetStr(array, i).c_str());
      }
   }

   // Single shape mode parameters
   mSingleParams.readObjectAttribs = AiNodeGetBool(mProcNode, p_read_object_attributes);
   mSingleParams.readPrimitiveAttribs = AiNodeGetBool(mProcNode, p_read_primitive_attributes);
   mSingleParams.readPointAttribs = AiNodeGetBool(mProcNode, p_read_point_attributes);
   mSingleParams.readVertexAttribs = AiNodeGetBool(mProcNode, p_read_vertex_attributes);
   mSingleParams.attribsFrame = (AttributeFrame) AiNodeGetInt(mProcNode, p_attributes_frame);
   mSingleParams.attribPreficesToRemove.clear();
   array = AiNodeGetArray(mProcNode, p_attribute_prefices_to_remove);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mSingleParams.attribPreficesToRemove.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   mSingleParams.computeTangents.clear();
   array = AiNodeGetArray(mProcNode, p_compute_tangents);
   if (array)
   {
      for (unsigned int i=0; i<AiArrayGetNumElements(array); ++i)
      {
         mSingleParams.computeTangents.insert(AiArrayGetStr(array, i).c_str());
      }
   }
   mSingleParams.radiusName = AiNodeGetStr(mProcNode, p_radius_name).c_str();
   mSingleParams.radiusMin = AiNodeGetFlt(mProcNode, p_radius_min);
   mSingleParams.radiusMax = AiNodeGetFlt(mProcNode, p_radius_max);
   mSingleParams.radiusScale = AiNodeGetFlt(mProcNode, p_radius_scale);
   mSingleParams.widthMin = AiNodeGetFlt(mProcNode, p_width_min);
   mSingleParams.widthMax = AiNodeGetFlt(mProcNode, p_width_max);
   mSingleParams.widthScale = AiNodeGetFlt(mProcNode, p_width_scale);
   mSingleParams.nurbsSampleRate = AiNodeGetInt(mProcNode, p_nurbs_sample_rate);

   // Only override relative samples if samples were read from user attribute
   if (samplesSet)
   {
      mCommonParams.relativeSamples = relativeSamples;
   }
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
