#include "dso.h"
#include "visitors.h"
#include "globallock.h"
#include <sstream>

const char* CycleTypeNames[] =
{
   "hold",
   "loop",
   "reverse",
   "bounce",
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
   fps = 24.0;
   preserveStartFrame = false;
   
   ignoreMotionBlur = false;
   ignoreVisibility = false;
   ignoreTransforms = false;
   ignoreInstances = false;
   
   samplesExpandIterations = 0;
   optimizeSamples = false;
   
   verbose = false;
}

std::string Dso::CommonParameters::dataString() const
{
   std::ostringstream oss;
   
   if (filePath.length() > 0)
   {
      oss << " -filename " << filePath;
   }
   if (referenceFilePath.length() > 0)
   {
      oss << " -referencefilename " << referenceFilePath;
   }
   if (namePrefix.length() > 0)
   {
      oss << " -nameprefix " << namePrefix;
   }
   if (objectPath.length() > 0)
   {
      oss << " -objectpath " + objectPath;
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
   if (ignoreMotionBlur)
   {
      oss << " -ignoremotionblur";
   }
   if (ignoreTransforms)
   {
      oss << " -ignoretransforms";
   }
   if (ignoreInstances)
   {
      oss << " -ignoreinstances";
   }
   if (ignoreVisibility)
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
   if (verbose)
   {
      oss << " -verbose";
   }
   
   return oss.str();
}

void Dso::MultiParameters::reset()
{
   overrideAttribs.clear();
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
   
   return oss.str();
}

void Dso::SingleParameters::reset()
{
   readObjectAttribs = false;
   readPrimitiveAttribs = false;
   readPointAttribs = false;
   readVertexAttribs = false;
   attribsFrame = AF_render;
   attribsToBlur.clear();
   attribPreficesToRemove.clear();
   
   computeTangents = false;
   
   radiusMin = 0.0f;
   radiusMax = 1000000.0f;
   radiusScale = 1.0f;
}

std::string Dso::SingleParameters::dataString() const
{
   std::ostringstream oss;
   
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
   if (attribsToBlur.size() > 0)
   {
      oss << " -blurattribs";
      std::set<std::string>::const_iterator it = attribsToBlur.begin();
      while (it != attribsToBlur.end())
      {
         oss << " " << *it;
         ++it;
      }
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
   if (computeTangents)
   {
      oss << " -computetangents";
   }
   oss << " -radiusmin " << radiusMin;
   oss << " -radiusmax " << radiusMax;
   oss << " -radiuscale " << radiusScale;
   
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
   , mSetTimeSamples(false)
   , mStepSize(-1.0f)
   , mShapeKey("")
   , mInstanceNum(0)
   , mShutterOpen(0.0)
   , mShutterClose(0.0)
   , mShutterStep(0)
{
   mCommonParams.reset();
   mSingleParams.reset();
   mMultiParams.reset();
   
   AiMsgDebug("[abcproc] Read \"data\" string");
   readFromDataParam();
   
   AiMsgDebug("[abcproc] Read \"abc_\" user attributes");
   // Override values from "data" string
   readFromUserParams();
   
   normalizeFilePath(mCommonParams.filePath);
   normalizeFilePath(mCommonParams.referenceFilePath);
   
   AiMsgDebug("[abcproc] Final parameters: \"%s\"", dataString().c_str());
   
   // Only acquire global lock when create new alembic scenes
   //
   // Alembic libs should be build with a threadsafe version of HDF5 that will handle
   //   multi-threaded reads by itself
   // Same with Ogawa backend
   AiMsgDebug("[abcproc] Creating alembic scene(s)");
   {
      ScopeLock _lock;
      
      mScene = AlembicSceneCache::Ref(mCommonParams.filePath);
   
      if (mCommonParams.referenceFilePath.length() > 0)
      {
         mRefScene = AlembicSceneCache::Ref(mCommonParams.referenceFilePath);
      }
   }
   
   if (mScene)
   {
      mScene->setFilter(mCommonParams.objectPath);
      
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
      }
      
      mRenderTime = computeTime(mCommonParams.frame);
      
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
      
      if (visitor.numShapes() == 1 &&
          mCommonParams.ignoreTransforms &&
          mCommonParams.ignoreVisibility && 
          mCommonParams.ignoreInstances)
      {
         // output a single shape in object space
         if (mCommonParams.verbose)
         {
            AiMsgInfo("[abcproc] Single shape mode");
         }
         mMode = PM_single;
      }
      
      // build sample list
   }
   else
   {
      AiMsgWarning("[abcproc] Invalid alembic file \"%s\"", mCommonParams.filePath.c_str());
   }
}

Dso::~Dso()
{
   ScopeLock _lock;
   
   if (mScene)
   {
      AlembicSceneCache::Unref(mScene);
   }
   if (mRefScene)
   {
      AlembicSceneCache::Unref(mRefScene);
   }
}

std::string Dso::dataString() const
{
   std::string rv = mCommonParams.dataString();
   
   if (mMode == PM_multi)
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

double Dso::computeTime(double frame) const
{
   // Apply speed / offset
   
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
   case CT_hold:
   default:
      if (t < (startTime - eps))
      {
         t = startTime;
      }
      else if (t > (endTime + eps))
      {
         t = endTime;
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
         
         if (!processFlag(args, i))
         {
            AiMsgWarning("[abcproc] Failed processing flag \"%s\"", args[j].c_str());
         }
         else
         {
            AiMsgDebug("[abcproc] Processed flag \"%s\"", args[j].c_str());
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
      if (i >= args.size())
      {
         return false;
      }
      mCommonParams.filePath = args[i];
   }
   else if (args[i] == "-nameprefix")
   {
      ++i;
      if (i >= args.size())
      {
         return false;
      }
      mCommonParams.namePrefix = args[i];
   }
   else if (args[i] == "-objectpath")
   {
      ++i;
      if (i >= args.size())
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
   else if (args[i] == "-referencefilename")
   {
      ++i;
      if (i >= args.size())
      {
         return false;
      }
      mCommonParams.referenceFilePath = args[i];
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
      mCommonParams.ignoreMotionBlur = true;
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
      toLower(cts);
      
      for (size_t j=0; j<=CT_MAX; ++j)
      {
         if (cts == CycleTypeNames[j])
         {
            mCommonParams.cycle = (CycleType)j;
            break;
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
   else if (args[i] == "-blurattribs")
   {
      ++i;
      while (i < args.size() && !isFlag(args[i]))
      {
         mSingleParams.attribsToBlur.insert(args[i]);
         ++i;
      }
      --i;
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
      mSingleParams.computeTangents = true;
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
            mCommonParams.ignoreMotionBlur = AiNodeGetBool(mProcNode, pname);
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
      else if (param == "blurattribs")
      {
         mSingleParams.attribsToBlur.clear();
         
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
                  mSingleParams.attribsToBlur.insert(name);
               }
               
               p0 = p1 + 1;
               p1 = names.find(' ', p0);
            }
            
            name = names.substr(p0);
            strip(name);
            
            if (name.length() > 0)
            {
               mSingleParams.attribsToBlur.insert(name);
            }
         }
         else if (AiUserParamGetType(p) == AI_TYPE_ARRAY && AiUserParamGetArrayType(p) == AI_TYPE_STRING)
         {
            AtArray *names = AiNodeGetArray(mProcNode, pname);
            
            if (names)
            {
               for (unsigned int i=0; i<names->nelements; ++i)
               {
                  mSingleParams.attribsToBlur.insert(AiArrayGetStr(names, i));
               }
            }
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected an array of string values", pname);
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
         if (AiUserParamGetType(p) == AI_TYPE_BOOLEAN)
         {
            mSingleParams.computeTangents = AiNodeGetBool(mProcNode, pname);
         }
         else
         {
            AiMsgWarning("[abcproc] Ignore parameter \"%s\": Expected a boolean value", pname);
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
   }
   
   AiUserParamIteratorDestroy(pit);
   
   // Only override relative samples if samples were read from user attribute
   if (samplesSet)
   {
      mCommonParams.relativeSamples = relativeSamples;
   }
}

void Dso::transferUserParams(AtNode *node)
{
   // TODO
}
