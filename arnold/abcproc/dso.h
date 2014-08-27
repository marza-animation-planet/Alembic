#ifndef ABCARNOLD_PROCARGS_H_
#define ABCARNOLD_PROCARGS_H_

#include <ai.h>
#include <AlembicSceneCache.h>

#ifdef NAME_PREFIX
#   define PREFIX_NAME(s) NAME_PREFIX s
#else
#   define PREFIX_NAME(s) s
#   define NAME_PREFIX ""
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

struct ShapeInfo
{
   std::string name;
   int index;
};

struct Dso
{
public:
   
   Dso(AtNode *node);
   ~Dso();
   
   void readFromDataParam();
   void readFromUserParams();
   void transferUserParams(AtNode *node);
   
   
   /*
   void setRenderFrame(double rf, bool updateMotionSamples=true)
   {
      rf /= mCommonParams.fps;
      double df = rf - mBaseTime;
      if (updateMotionSamples)
      {
         for (size_t i=0; i<mTimeSamples.size(); ++i)
         {
            mTimeSamples[i] += d;
         }
      }
   }

   
   inline setShapesInfo(const std::vector<ShapeInfo> infos)
   {
      mShapesInfo = infos;
   }

   inline size_t numShapes() const
   {
      return mShapesInfo.size();
   }
   
   inline const ShapeInfo& shapeInfo(size_t i) const
   {
      return mShapesInfo[i];
   }
   */
   
   /*
   bool isInstance() const;
   void setMasterNodeName(const std::string &name);
   const std::string& getMasterNodeName() const;
   */
   
   /*
   size_t numGeneratedNodes() const;
   void setGeneratedNodesCount(size_t n);
   AtNode* getGeneratedNode(size_t idx) const;
   void setGeneratedNode(size_t idx, AtNode *n);
   */

private:
   
   std::string dataString() const;
   
   bool cleanAttribName(std::string &name) const;
   
   void strip(std::string &s) const;
   void toLower(std::string &s) const;
   bool isFlag(std::string &s) const;
   
   bool processFlag(std::vector<std::string> &args, size_t &i);
   
   void normalizeFilePath(std::string &path) const;
   
   double computeTime(double frame) const;
   
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
      
      bool ignoreMotionBlur;
      bool ignoreVisibility;
      bool ignoreTransforms;
      bool ignoreInstances;
      
      int samplesExpandIterations;
      bool optimizeSamples;
      
      bool verbose;
      
      
      void reset();
      std::string dataString() const;
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
      std::set<std::string> attribsToBlur;
      std::set<std::string> attribPreficesToRemove;
            
      // mesh
      bool computeTangents;
      
      // particles/curves
      float radiusMin;
      float radiusMax;
      float radiusScale;
      
      
      void reset();
      std::string dataString() const;
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
   
   std::string mShapeKey;
   int mInstanceNum;
   
   // fallback to official procedural way of defining motion sample range
   double mShutterOpen;
   double mShutterClose;
   int mShutterStep;
   
   std::vector<AtNode*> mGeneratedNodes;
   std::vector<ShapeInfo> mShapeInfos;
   
   static std::map<std::string, std::string> msMasterNodes;
};


#endif
