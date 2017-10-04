#ifndef ABCPROC_DSO_H_
#define ABCPROC_DSO_H_

#include <ai.h>
#include <AlembicSceneCache.h>
#include <strings.h>

enum CycleType
{
   CT_hold = 0,
   CT_loop,
   CT_reverse,
   CT_bounce,
   CT_clip,
   CT_MAX
};

enum AttributesEvaluationTime
{
   AET_render = 0,
   AET_shutter,
   AET_shutter_open,
   AET_shutter_close,
   AET_MAX
};

enum ReferenceSource
{
   RS_attributes = 0,
   RS_frame,
   RS_MAX
};

enum ProceduralMode
{
   PM_multi = 0,
   PM_single,
   PM_undefined,
   PM_MAX
};

extern const char* CycleTypeNames[];
extern const char* AttributesEvaluationTimeNames[];
extern const char* ReferenceSourceNames[];


class Dso
{
public:

   Dso(AtNode *node);
   ~Dso();

   void readParams();
   void setSingleParams(AtNode *node, const std::string &objectPath) const;

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

   inline bool ignoreReference() const
   {
      return mCommonParams.ignoreReference;
   }

   inline ReferenceSource referenceSource() const
   {
      return mCommonParams.referenceSource;
   }

   inline float referenceFrame() const
   {
      return mCommonParams.referenceFrame;
   }

   inline const std::string& referencePositionName() const
   {
      return mCommonParams.referencePositionName;
   }

   inline const std::string& referenceNormalName() const
   {
      return mCommonParams.referenceNormalName;
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

   double attributesTime(AttributesEvaluationTime aet) const;

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

   inline float velocityScale() const
   {
      return mCommonParams.velocityScale;
   }

   inline const char* velocityName() const
   {
      return (mCommonParams.velocityName.length() > 0 ? mCommonParams.velocityName.c_str() : 0);
   }

   inline const char* accelerationName() const
   {
      return (mCommonParams.accelerationName.length() > 0 ? mCommonParams.accelerationName.c_str() : 0);
   }

   inline bool forceVelocityBlur() const
   {
      return mCommonParams.forceVelocityBlur;
   }

   double computeTime(double frame, bool *exclude=0) const;

   // Attributes

   inline AttributesEvaluationTime attributesEvaluationTime() const
   {
      return mCommonParams.attributesEvaluationTime;
   }

   inline bool forceConstantAttribute(const std::string &name) const
   {
      return (mCommonParams.forceConstantAttributes.find(name) != mCommonParams.forceConstantAttributes.end());
   }

   inline bool ignoreAttribute(const std::string &name) const
   {
      return (mCommonParams.ignoreAttributes.find(name) != mCommonParams.ignoreAttributes.end());
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

   inline bool computeTangentsForUVs(const std::string &uvname) const
   {
      return (mSingleParams.computeTangentsForUVs.find(uvname) != mSingleParams.computeTangentsForUVs.end());
   }

   // Points

   inline const char* radiusName() const
   {
      return (mSingleParams.radiusName.length() > 0 ? mSingleParams.radiusName.c_str() : 0);
   }

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

   // Curves

   inline bool ignoreNurbs() const
   {
      return mCommonParams.ignoreNurbs;
   }

   inline unsigned int nurbsSampleRate() const
   {
      return mSingleParams.nurbsSampleRate;
   }

   inline double widthMin() const
   {
      return mSingleParams.widthMin;
   }

   inline double widthMax() const
   {
      return mSingleParams.widthMax;
   }

   inline double widthScale() const
   {
      return mSingleParams.widthScale;
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

   inline float volumePadding() const
   {
      return mVolumePadding;
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

private:

   void strip(std::string &s) const;
   void toLower(std::string &s) const;
   void normalizeFilePath(std::string &path) const;

   void setGeneratedNodesCount(size_t n);

   std::string shapeKey() const;

private:

   struct CommonParameters
   {
      std::string filePath;
      std::string namePrefix;
      std::string objectPath;

      double frame;
      double startFrame;
      double endFrame;
      CycleType cycle;
      double speed;
      bool preserveStartFrame;
      double offset;
      double fps;

      unsigned int samples;
      unsigned int expandSamplesIterations;
      bool optimizeSamples;

      bool ignoreDeformBlur;
      bool ignoreTransformBlur;
      bool ignoreVisibility;
      bool ignoreTransforms;
      bool ignoreInstances;
      bool ignoreNurbs;

      bool verbose;

      bool ignoreReference;
      ReferenceSource referenceSource;
      std::string referencePositionName;
      std::string referenceNormalName;
      float referenceFrame;

      float velocityScale;
      std::string velocityName;
      std::string accelerationName;
      bool forceVelocityBlur;

      std::set<std::string> removeAttributePrefices;
      std::set<std::string> forceConstantAttributes;
      std::set<std::string> ignoreAttributes;
      AttributesEvaluationTime attributesEvaluationTime;

      void reset();
      std::string shapeKey() const;
   };

   struct SingleParameters
   {
      // mesh
      std::set<std::string> computeTangentsForUVs;

      // particles
      std::string radiusName;
      float radiusMin;
      float radiusMax;
      float radiusScale;

      // curves
      float widthMin;
      float widthMax;
      float widthScale;
      unsigned int nurbsSampleRate;

      void reset();
      std::string shapeKey() const;
   };

   // ---

   AtNode *mProcNode;

   ProceduralMode mMode;

   CommonParameters mCommonParams;
   SingleParameters mSingleParams;

   AlembicScene *mScene;

   // For simple unix/window directory mapping
   std::string mRootDrive;

   double mRenderTime;
   std::vector<double> mTimeSamples;
   std::vector<double> mExpandedTimeSamples;

   float mStepSize;
   float mVolumePadding;

   size_t mNumShapes;

   std::string mShapeKey;
   int mInstanceNum;

   // in frames
   double mMotionStart;
   double mMotionEnd;

   std::vector<AtNode*> mGeneratedNodes;

   bool mReverseWinding;

   double mGlobalFrame;

   static std::map<std::string, std::string> msMasterNodes;
};


#endif
