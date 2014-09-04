#ifndef ABCARNOLD_PROCARGS_H_
#define ABCARNOLD_PROCARGS_H_

#include <ai.h>
#include <AlembicSceneCache.h>

#if AI_VERSION_ARCH_NUM > 4 || (AI_VERSION_ARCH_NUM == 4 && AI_VERSION_MAJOR_NUM >= 1)
#  define ARNOLD_4_1_OR_ABOVE
#endif

enum CycleType
{
   CT_hold = 0,
   CT_loop,
   CT_reverse,
   CT_bounce,
   CT_MAX
};

enum AttributeFrame
{
   AF_render = 0,
   AF_shutter,
   AF_shutter_open,
   AF_shutter_close,
   AF_MAX
};

enum ProceduralMode
{
   PM_multi = 0,
   PM_single,
   PM_undefined,
   PM_MAX
};

extern const char* CycleTypeNames[];
extern const char* AttributeFrameNames[];

struct Dso
{
public:
   
   Dso(AtNode *node);
   ~Dso();
   
   void readFromDataParam();
   void readFromUserParams();
   
   void transferUserParams(AtNode *dst);
   void transferInstanceParams(AtNode *dst);
   
   
   inline AtNode* procNode() const
   {
      return mProcNode;
   }
   
   inline AlembicScene* scene() const
   {
      return mScene;
   }
   
   inline AlembicScene* referenceScene() const
   {
      return mRefScene;
   }
   
   // Common setup
   
   inline bool verbose() const
   {
      return mCommonParams.verbose;
   }
   
   inline ProceduralMode mode() const
   {
      return mMode;
   }
   
   inline const std::string& filePath() const
   {
      return mCommonParams.filePath;
   }
   
   inline bool hasReference() const
   {
      return (mCommonParams.referenceFilePath.length() > 0);
   }
   
   inline const std::string& referenceFilePath() const
   {
      return mCommonParams.referenceFilePath;
   }
   
   inline const std::string& objectPath() const
   {
      return mCommonParams.objectPath;
   }
   
   inline const std::string& namePrefix() const
   {
      return mCommonParams.namePrefix;
   }
   
   inline bool ignoreVisibility() const
   {
      return mCommonParams.ignoreVisibility;
   }
   
   inline bool ignoreInstances() const
   {
      return mCommonParams.ignoreInstances;
   }
   
   inline bool ignoreMotionBlur() const
   {
      return (ignoreDeformBlur() && ignoreTransformBlur());
   }
   
   inline bool ignoreDeformBlur() const
   {
      return mCommonParams.ignoreDeformBlur;
   }
   
   inline bool ignoreTransformBlur() const
   {
      return mCommonParams.ignoreTransformBlur;
   }
   
   inline bool ignoreTransforms() const
   {
      return mCommonParams.ignoreTransforms;
   }
   
   inline double renderTime() const
   {
      return mRenderTime;
   }
   
   double attribsTime(AttributeFrame af) const;
   
   inline double fps() const
   {
      return mCommonParams.fps;
   }
   
   inline size_t numMotionSamples() const
   {
      return mExpandedTimeSamples.size();
   }
   
   inline double motionSampleTime(size_t i) const
   {
      return mExpandedTimeSamples[i];
   }
   
   inline const std::vector<double>& motionSampleTimes() const
   {
      return mExpandedTimeSamples;
   }
   
   double computeTime(double frame) const;
   
   // Attributes
   
   inline AttributeFrame attribsFrame() const
   {
      return mSingleParams.attribsFrame;
   }
   
   inline bool readObjectAttribs() const
   {
      return mSingleParams.readObjectAttribs;
   }
   
   inline bool readPrimitiveAttribs() const
   {
      return mSingleParams.readPrimitiveAttribs;
   }
   
   inline bool readPointAttribs() const
   {
      return mSingleParams.readPointAttribs;
   }
   
   inline bool readVertexAttribs() const
   {
      return mSingleParams.readVertexAttribs;
   }
   
   inline const std::set<std::string>& overrideAttribs() const
   {
      return mMultiParams.overrideAttribs;
   }
   
   inline bool overrideAttrib(const std::string &name) const
   {
      return (mMode == PM_single || mMultiParams.overrideAttribs.find(name) != mMultiParams.overrideAttribs.end());
   }
   
   bool cleanAttribName(std::string &name) const;
   
   // Shapes
   
   inline size_t numShapes() const
   {
      return mNumShapes;
   }
   
   std::string uniqueName(const std::string &name) const;
   
   // Mesh
   
   inline bool reverseWinding() const
   {
      return mReverseWinding;
   }
   
   inline bool computeTangents(const std::string &uvname) const
   {
      return (mSingleParams.computeTangents.find(uvname) != mSingleParams.computeTangents.end());
   }
   
   // Points/Curves
   
   inline double radiusMin() const
   {
      return mSingleParams.radiusMin;
   }
   
   inline double radiusMax() const
   {
      return mSingleParams.radiusMax;
   }
   
   inline double radiusScale() const
   {
      return mSingleParams.radiusScale;
   }
   
   // Volume
   
   inline bool isVolume() const
   {
      return (mStepSize > 0.0f);
   }
   
   inline float volumeStepSize() const
   {
      return mStepSize;
   }
   
   // Instance tacking
   
   inline void setMasterNodeName(const std::string &name)
   {
      msMasterNodes[mShapeKey] = name;
   }
   
   inline const std::string& masterNodeName() const
   {
      return msMasterNodes[mShapeKey];
   }
   
   inline bool isInstance(std::string *masterNodeName=0) const
   {
      std::map<std::string, std::string>::const_iterator it = msMasterNodes.find(mShapeKey);
      
      if (it != msMasterNodes.end())
      {
         if (masterNodeName)
         {
            *masterNodeName = it->second;
         }
         return true;
      }
      else
      {
         return false;
      }
   }
   
   inline int instanceNumber() const
   {
      return mInstanceNum;
   }
   
   // Node tracking
   
   void setGeneratedNode(size_t idx, AtNode *node);
   
   inline size_t numGeneratedNodes() const
   {
      return mGeneratedNodes.size();
   }
   
   inline AtNode* generatedNode(size_t idx) const
   {
      return (idx < mGeneratedNodes.size() ? mGeneratedNodes[idx] : 0);
   }
   
   std::string dataString(const char *targetShape) const;

private:
   
   void strip(std::string &s) const;
   void toLower(std::string &s) const;
   bool isFlag(std::string &s) const;
   
   bool processFlag(std::vector<std::string> &args, size_t &i);
   
   void normalizeFilePath(std::string &path) const;
   
   void setGeneratedNodesCount(size_t n);
   
   std::string shapeKey() const;
   
private:
   
   struct CommonParameters
   {
      std::string filePath;
      std::string referenceFilePath;
      std::string namePrefix;
      std::string objectPath;
      
      double frame;
      double startFrame;
      double endFrame;
      bool relativeSamples;
      std::vector<double> samples;
      CycleType cycle;
      double speed;
      bool preserveStartFrame;
      double offset;
      double fps;
      
      bool ignoreDeformBlur;
      bool ignoreTransformBlur;
      bool ignoreVisibility;
      bool ignoreTransforms;
      bool ignoreInstances;
      
      int samplesExpandIterations;
      bool optimizeSamples;
      
      bool verbose;
      
      
      void reset();
      std::string dataString(const char *targetShape) const;
      std::string shapeKey() const;
   };
   
   struct MultiParameters
   {
      std::set<std::string> overrideAttribs;
      
      
      void reset();
      std::string dataString() const;
   };
   
   struct SingleParameters
   {
      // attributes
      bool readObjectAttribs;
      bool readPrimitiveAttribs;
      bool readPointAttribs;
      bool readVertexAttribs;
      AttributeFrame attribsFrame;
      std::set<std::string> attribPreficesToRemove;
      
      // mesh
      std::set<std::string> computeTangents;
      
      // particles/curves
      float radiusMin;
      float radiusMax;
      float radiusScale;
      
      
      void reset();
      std::string dataString() const;
      std::string shapeKey() const;
   };
   
   // ---
   
   AtNode *mProcNode;
   
   ProceduralMode mMode;
   
   CommonParameters mCommonParams;
   MultiParameters mMultiParams;
   SingleParameters mSingleParams;
   
   AlembicScene *mScene;
   AlembicScene *mRefScene;
   
   // For simple unix/window directory mapping
   std::string mRootDrive;
   
   double mRenderTime;
   std::vector<double> mTimeSamples;
   std::vector<double> mExpandedTimeSamples;
   bool mSetTimeSamples;
   
   float mStepSize;
   
   size_t mNumShapes;
   
   std::string mShapeKey;
   int mInstanceNum;
   
   // fallback to official procedural way of defining motion sample range
   double mShutterOpen;
   double mShutterClose;
   int mShutterStep;
   
   // on seconds
   double mMotionStart;
   double mMotionEnd;
   
   std::vector<AtNode*> mGeneratedNodes;
   
   bool mReverseWinding;
   
   static std::map<std::string, std::string> msMasterNodes;
};


#endif
