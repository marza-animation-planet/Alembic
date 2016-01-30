#ifndef ABCPROC_VISITORS_H_
#define ABCPROC_VISITORS_H_

#include <AlembicNode.h>
#include <ai.h>
#include <deque>
#include "dso.h"
#include "userattr.h"
#include "globallock.h"

// ---

class GetTimeRange
{
public:
   
   GetTimeRange();
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &node, AlembicNode *instance=0);

   bool getRange(double &start, double &end) const;

private:
   
   template <class T>
   void updateTimeRange(AlembicNodeT<T> &node);

private:
   
   double mStartTime;
   double mEndTime;
};

template <class T>
void GetTimeRange::updateTimeRange(AlembicNodeT<T> &node)
{
   Alembic::AbcCoreAbstract::TimeSamplingPtr ts = node.typedObject().getSchema().getTimeSampling();
   size_t nsamples = node.typedObject().getSchema().getNumSamples();
   if (nsamples > 1)
   {
      mStartTime = std::min(ts->getSampleTime(0), mStartTime);
      mEndTime = std::max(ts->getSampleTime(nsamples-1), mEndTime);
   }
}

// ---

extern bool GetVisibility(Alembic::Abc::ICompoundProperty props, double t);

// ---

class CountShapes
{
public:
   
   CountShapes(double renderTime, bool ignoreTransforms, bool ignoreInstances, bool ignoreVisibility, bool ignoreNurbs);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &node, AlembicNode *instance=0);

   inline size_t numShapes() const { return mNumShapes; }

private:
   
   template <class T>
   bool isVisible(AlembicNodeT<T> &node);
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node);

private:
   
   double mRenderTime;
   bool mIgnoreTransforms;
   bool mIgnoreInstances;
   bool mIgnoreVisibility;
   bool mIgnoreNurbs;
   size_t mNumShapes;
};

template <class T>
AlembicNode::VisitReturn CountShapes::shapeEnter(AlembicNodeT<T> &node)
{
   if (!isVisible(node))
   {
      return AlembicNode::DontVisitChildren;
   }
   else
   {
      ++mNumShapes;
      return AlembicNode::ContinueVisit;
   }
}

template <class T>
bool CountShapes::isVisible(AlembicNodeT<T> &node)
{
   return (mIgnoreVisibility ? true : GetVisibility(node.object().getProperties(), mRenderTime));
}

// ---

class MakeProcedurals
{
public:
   
   MakeProcedurals(class Dso *dso);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicXform &node, AlembicNode *instance=0);
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   inline size_t numNodes() const { return mNodes.size(); }
   inline AtNode* node(size_t i) const { return (i < numNodes() ? mNodes[i] : 0); }
   inline const char* path(size_t i) const { return (i < numNodes() ? mPaths[i].c_str() : 0); }
   inline bool isFloat3(UserAttribute &attr) const { return (attr.arnoldType == AI_TYPE_VECTOR ||
                                                             attr.arnoldType == AI_TYPE_POINT ||
                                                             (attr.arnoldType == AI_TYPE_FLOAT && attr.dataDim == 3)); }
   
private:
   
   float adjustRadius(float radius) const;

   template <class Schema>
   bool overrideBounds(AlembicNodeT<Alembic::Abc::ISchemaObject<Schema> > &node, Alembic::Abc::Box3d &box);
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance, bool interpolateBounds, double extraPadding);

private:

   class Dso *mDso;
   std::deque<std::deque<Alembic::Abc::M44d> > mMatrixSamplesStack;
   std::deque<AtNode*> mNodes;
   std::deque<std::string> mPaths;
};

inline float MakeProcedurals::adjustRadius(float radius) const
{
   radius *= mDso->radiusScale();
   
   if (radius < mDso->radiusMin())
   {
      return mDso->radiusMin();
   }
   else if (radius > mDso->radiusMax())
   {
      return mDso->radiusMax();
   }
   else
   {
      return radius;
   }
}

template <class Schema>
bool MakeProcedurals::overrideBounds(AlembicNodeT<Alembic::Abc::ISchemaObject<Schema> > &node, Alembic::Abc::Box3d &box)
{
   bool rv = false;
   
   if (!mDso->ignoreDeformBlur())
   {
      const char *minAttrName = mDso->overrideBoundsMinName();
      if (!minAttrName)
      {
         minAttrName = "overrideBoundsMin";
      }
      
      const char *maxAttrName = mDso->overrideBoundsMaxName();
      if (!maxAttrName)
      {
         maxAttrName = "overrideBoundsMax";
      }
      
      Schema schema = node.typedObject().getSchema();
      
      Alembic::Abc::ICompoundProperty userProps = schema.getUserProperties();
      Alembic::Abc::ICompoundProperty geomParams = schema.getArbGeomParams();
      double t = mDso->renderTime();
      
      UserAttribute minAttr;
      UserAttribute maxAttr;
      
      InitUserAttribute(minAttr);
      InitUserAttribute(maxAttr);
      
      bool hasMin = ReadSingleUserAttribute(minAttrName, ObjectAttribute, t, userProps, geomParams, minAttr);
      
      if (!hasMin && mDso->isPromotedToObjectAttrib(minAttrName))
      {
         hasMin = ReadSingleUserAttribute(minAttrName, PrimitiveAttribute, t, userProps, geomParams, minAttr);
         if (!hasMin)
         {
            hasMin = ReadSingleUserAttribute(minAttrName, PointAttribute, t, userProps, geomParams, minAttr);
            if (!hasMin)
            {
               hasMin = ReadSingleUserAttribute(minAttrName, VertexAttribute, t, userProps, geomParams, minAttr);
            }
         }
         if (hasMin)
         {
            UserAttribute tmp;
            
            bool promoted = PromoteToConstantAttrib(minAttr, tmp);
            
            DestroyUserAttribute(minAttr);
            
            if (promoted)
            {
               hasMin = true;
               std::swap(tmp, minAttr);
            }
            else
            {
               hasMin = false;
            }
         }
      }
      
      bool hasMax = ReadSingleUserAttribute(maxAttrName, ObjectAttribute, t, userProps, geomParams, maxAttr);
      
      if (!hasMax && mDso->isPromotedToObjectAttrib(maxAttrName))
      {
         hasMax = ReadSingleUserAttribute(maxAttrName, PrimitiveAttribute, t, userProps, geomParams, maxAttr);
         if (!hasMax)
         {
            hasMax = ReadSingleUserAttribute(maxAttrName, PointAttribute, t, userProps, geomParams, maxAttr);
            if (!hasMax)
            {
               hasMax = ReadSingleUserAttribute(maxAttrName, VertexAttribute, t, userProps, geomParams, maxAttr);
            }
         }
         if (hasMax)
         {
            UserAttribute tmp;
            
            bool promoted = PromoteToConstantAttrib(maxAttr, tmp);
            
            DestroyUserAttribute(maxAttr);
            
            if (promoted)
            {
               hasMax = true;
               std::swap(tmp, maxAttr);
            }
            else
            {
               hasMax = false;
            }
         }
      }
      
      if (hasMin && hasMax && isFloat3(minAttr) && isFloat3(maxAttr))
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Use bounds override for \"%s\"", node.path().c_str());
         }
         
         float *bmin = (float*) minAttr.data;
         float *bmax = (float*) maxAttr.data;
         
         box.min.x = bmin[0];
         box.min.y = bmin[1];
         box.min.z = bmin[2];
         
         box.max.x = bmax[0];
         box.max.y = bmax[1];
         box.max.z = bmax[2];
         
         
         rv = true;
      }
      
      DestroyUserAttribute(minAttr);
      DestroyUserAttribute(maxAttr);
   }
   
   return rv;
}

template <class T>
AlembicNode::VisitReturn MakeProcedurals::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance, bool interpolateBounds, double extraPadding)
{
   Alembic::Util::bool_t visible = (mDso->ignoreVisibility() ? true : GetVisibility(node.object().getProperties(), mDso->renderTime()));
   
   node.setVisible(visible);
   
   if (visible)
   {
      Alembic::Abc::Box3d box;
      
      if (!overrideBounds(node, box))
      {
         TimeSampleList<Alembic::Abc::IBox3dProperty> &boundsSamples = node.samples().boundsSamples;
         
         double renderTime = mDso->renderTime();
         
         const double *sampleTimes = 0;
         size_t sampleTimesCount = 0;
         
         if (mDso->ignoreDeformBlur())
         {
            sampleTimes = &renderTime;
            sampleTimesCount = 1;
         }
         else
         {
            sampleTimes = &(mDso->motionSampleTimes()[0]);
            sampleTimesCount = mDso->numMotionSamples();
         }
            
         for (size_t i=0; i<sampleTimesCount; ++i)
         {
            double t = sampleTimes[i];
            
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Sample bounds \"%s\" at t=%lf", node.path().c_str(), t);
            }
            
            node.sampleBounds(t, t, (i > 0 || instance != 0));
         }
         
         if (boundsSamples.size() == 0)
         {
            // no data... -> empty box, don't generate node
            mNodes.push_back(0);
            mPaths.push_back("");
            AiMsgWarning("[abcproc] No valid bounds for \"%s\"", node.path().c_str());
            return AlembicNode::ContinueVisit;
         }
         else if (boundsSamples.size() == 1)
         {
            box = boundsSamples.begin()->data();
         }
         else
         {
            for (size_t i=0; i<sampleTimesCount; ++i)
            {
               double t = sampleTimes[i];
               
               TimeSampleList<Alembic::Abc::IBox3dProperty>::ConstIterator samp0, samp1;
               double blend = 0.0;
               
               if (boundsSamples.getSamples(t, samp0, samp1, blend))
               {
                  if (blend > 0.0)
                  {
                     if (interpolateBounds)
                     {
                        Alembic::Abc::Box3d b0 = samp0->data();
                        Alembic::Abc::Box3d b1 = samp1->data();
                        Alembic::Abc::Box3d b2((1.0 - blend) * b0.min + blend * b1.min,
                                               (1.0 - blend) * b0.max + blend * b1.max);
                        box.extendBy(b2);
                     }
                     else
                     {
                        box.extendBy(samp0->data());
                        box.extendBy(samp1->data());
                     }
                  }
                  else
                  {
                     box.extendBy(samp0->data());
                  }
               }
            }
         }
         
         const AtUserParamEntry *pe = AiNodeLookUpUserParameter(mDso->procNode(), "disp_padding");
         if (pe && mDso->overrideAttrib("disp_padding"))
         {
            float padding = AiNodeGetFlt(mDso->procNode(), "disp_padding");
            
            if (padding > 0.0f && mDso->verbose())
            {
               AiMsgInfo("[abcproc] \"disp_padding\" attribute found on procedural, pad bounds by %f", padding);
            }
            
            extraPadding += padding;
         }
      }
      
      if (extraPadding > 0)
      {
         box.min.x -= extraPadding;
         box.min.y -= extraPadding;
         box.min.z -= extraPadding;
         
         box.max.x += extraPadding;
         box.max.y += extraPadding;
         box.max.z += extraPadding;
      }
      
      AlembicNode *targetNode = (instance ? instance : &node);
      
      // Format name 'a la maya'
      GlobalLock::Acquire();
      
      std::string targetName = targetNode->formatPartialPath(mDso->namePrefix().c_str(), AlembicNode::LocalPrefix, '|');
      std::string name = mDso->uniqueName(targetName);
      AtNode *proc = AiNode("procedural");
      AiNodeSetStr(proc, "name", name.c_str());

      GlobalLock::Release();

      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Generate new procedural node \"%s\"", name.c_str());
         AiMsgInfo("[abcproc]   Bounds: (%lf, %lf, %lf) -> (%lf, %lf, %lf)",
                   box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z);
      }
      
      AiNodeSetStr(proc, "dso", AiNodeGetStr(mDso->procNode(), "dso"));
      AiNodeSetStr(proc, "data", mDso->dataString(targetNode->path().c_str()).c_str());
      AiNodeSetBool(proc, "load_at_init", false);
      AiNodeSetPnt(proc, "min", box.min.x, box.min.y, box.min.z);
      AiNodeSetPnt(proc, "max", box.max.x, box.max.y, box.max.z);
      
      if (!mDso->ignoreTransforms() && mMatrixSamplesStack.size() > 0)
      {
         std::deque<Alembic::Abc::M44d> &matrices = mMatrixSamplesStack.back();
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] %lu xform sample(s) for \"%s\"", matrices.size(), node.path().c_str());
         }
         
         AtArray *ary = AiArrayAllocate(1, matrices.size(), AI_TYPE_MATRIX);
         
         for (size_t i=0; i<matrices.size(); ++i)
         {
            Alembic::Abc::M44d &src = matrices[i];
            
            AtMatrix dst;
            for (int r=0; r<4; ++r)
               for (int c=0; c<4; ++c)
                  dst[r][c] = float(src[r][c]);
            
            AiArraySetMtx(ary, i, dst);
         }
         
         AiNodeSetArray(proc, "matrix", ary);
      }
      
      mDso->transferUserParams(proc);
      
      // Add instance_num attribute
      if (AiNodeLookUpUserParameter(proc, "instance_num") == 0)
      {
         AiNodeDeclare(proc, "instance_num", "constant INT");
      }
      AiNodeSetInt(proc, "instance_num", int(targetNode->instanceNumber()));
      
      mNodes.push_back(proc);
      mPaths.push_back(targetNode->path());
      
      return AlembicNode::ContinueVisit;
   }
   else
   {
      return AlembicNode::DontVisitChildren;
   }
}

// ---

class CollectWorldMatrices
{
public:
   
   CollectWorldMatrices(class Dso *dso);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicXform &node, AlembicNode *instance=0);
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   inline bool getWorldMatrix(const std::string &objectPath, Alembic::Abc::M44d &W) const
   {
      std::map<std::string, Alembic::Abc::M44d>::const_iterator it = mMatrices.find(objectPath);
      if (it != mMatrices.end())
      {
         W = it->second;
         return true;
      }
      else
      {
         return false;
      }
   }
   
private:
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
private:
   
   class Dso *mDso;
   std::vector<Alembic::Abc::M44d> mMatrixStack;
   std::map<std::string, Alembic::Abc::M44d> mMatrices;
};

template <class T>
AlembicNode::VisitReturn CollectWorldMatrices::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   Alembic::Abc::M44d W;
   
   if (mMatrixStack.size() > 0)
   {
      W = mMatrixStack.back();
   }
   
   std::string path = (instance ? instance->path() : node.path());
   
   mMatrices[path] = W;
   
   return AlembicNode::ContinueVisit;
}

// ---

class MakeShape
{
public:
   
   MakeShape(class Dso *dso);
   
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
   inline AtNode* node() const { return mNode; }
   
private:
   
   typedef std::map<std::string, Alembic::AbcGeom::IV2fGeomParam> UVSets;
   
   struct MeshInfo
   {
      bool varyingTopology;
      unsigned int polygonCount;
      unsigned int pointCount;
      unsigned int vertexCount;
      unsigned int *polygonVertexCount;
      unsigned int *vertexPointIndex;
      // map alembic vertex index to arnold vertex index
      unsigned int *arnoldVertexIndex;
      UserAttributes objectAttrs;
      UserAttributes primitiveAttrs;
      UserAttributes pointAttrs;
      UserAttributes vertexAttrs;
      UVSets UVs;
      
      
      inline MeshInfo()
         : varyingTopology(false)
         , polygonCount(0)
         , pointCount(0)
         , vertexCount(0)
         , polygonVertexCount(0)
         , vertexPointIndex(0)
         , arnoldVertexIndex(0)
      {
      }
      
      inline ~MeshInfo()
      {
         if (polygonVertexCount) AiFree(polygonVertexCount);
         if (vertexPointIndex) AiFree(vertexPointIndex);
         if (arnoldVertexIndex) AiFree(arnoldVertexIndex);
         
         DestroyUserAttributes(objectAttrs);
         DestroyUserAttributes(primitiveAttrs);
         DestroyUserAttributes(pointAttrs);
         DestroyUserAttributes(vertexAttrs);
         
         UVs.clear();
      }
   };
   
   struct PointsInfo
   {
      unsigned int pointCount;
      UserAttributes objectAttrs;
      UserAttributes pointAttrs;
      
      inline PointsInfo()
         : pointCount(0)
      {
      }
      
      inline ~PointsInfo()
      {
         DestroyUserAttributes(objectAttrs);
         DestroyUserAttributes(pointAttrs);
      }
   };
   
   struct CurvesInfo
   {
      bool varyingTopology;
      unsigned int curveCount;
      unsigned int pointCount;
      int degree;
      bool periodic;
      bool nurbs;
      unsigned int cvCount;
      UserAttributes objectAttrs;
      UserAttributes primitiveAttrs;
      UserAttributes pointAttrs;
      
      CurvesInfo()
         : varyingTopology(false)
         , curveCount(0)
         , pointCount(0)
         , degree(-1)
         , periodic(false)
         , nurbs(false)
         , cvCount(0)
      {
      }
      
      ~CurvesInfo()
      {
         DestroyUserAttributes(objectAttrs);
         DestroyUserAttributes(primitiveAttrs);
         DestroyUserAttributes(pointAttrs);
      }
   };

private:
   
   typedef bool (*CollectFilter)(const Alembic::AbcCoreAbstract::PropertyHeader &header, void *args);
   
   static bool FilterReferenceAttributes(const Alembic::AbcCoreAbstract::PropertyHeader &header, void *args);
   
private:
   
   float adjustRadius(float radius) const;
   
   AtNode* createArnoldNode(const char *nodeType, AlembicNode &node, bool useProcName=false) const;
   
   template <class Info>
   bool isVaryingFloat3(Info &info, const UserAttribute &ua);
   
   bool isIndexedFloat3(MeshInfo &info, const UserAttribute &ua);
   
   void collectUserAttributes(Alembic::Abc::ICompoundProperty userProps,
                              Alembic::Abc::ICompoundProperty geomParams,
                              double t,
                              bool interpolate,
                              UserAttributes *objectLevel,
                              UserAttributes *primitiveLevel,
                              UserAttributes *pointLevel,
                              UserAttributes *vertexLevel,
                              UVSets *UVs,
                              CollectFilter filter=0, void *filterArgs=0);
   
   template <class Schema>
   AtNode* generateVolumeBox(Schema &schema);
   
   template <class MeshSchema>
   AtNode* generateBaseMesh(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node,
                            AlembicNode *instance,
                            MeshInfo &info,
                            std::vector<float*> *smoothNormals=0);
   
   template <class T>
   void outputInstanceNumber(AlembicNodeT<T> &node, AlembicNode *instance);
   
   template <class MeshSchema>
   void outputMeshUVs(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node,
                      AtNode *mesh,
                      MeshInfo &info);
   
   float* computeMeshSmoothNormals(MeshInfo &info,
                                   const float *P0,
                                   const float *P1,
                                   float blend);
   
   template <class MeshSchema>
   bool computeMeshTangentSpace(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node,
                                MeshInfo &info,
                                AtArray *uvlist,
                                AtArray *uvidxs,
                                float* &N,
                                float* &T,
                                float* &B);
   
   template <class MeshSchema>
   bool getReferenceMesh(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node, MeshInfo &info,
                         AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> >* &refMesh,
                         UserAttributes* &pointAttrs, UserAttributes* &vertexAttrs);
   
   template <class MeshSchema>
   bool fillReferencePositions(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > *refMesh,
                               MeshInfo &info,
                               UserAttributes *pointAttrs,
                               Alembic::Abc::M44d &Mref);
   
   bool checkReferenceNormals(MeshInfo &info,
                              UserAttributes *pointAttrs,
                              UserAttributes *vertexAttrs);
   
   bool readReferenceNormals(AlembicMesh *refMesh,
                             MeshInfo &info,
                             const Alembic::Abc::M44d &Mref);
   
   bool fillReferenceNormals(MeshInfo &info,
                             UserAttributes *pointAttrs,
                             UserAttributes *vertexAttrs);
   
   bool fillReferenceNormals(MeshInfo &info,
                             bool computeSmoothNormals,
                             UserAttributes *pointAttrs,
                             UserAttributes *vertexAttrs);
   
   
   bool initCurves(CurvesInfo &info,
                   const Alembic::AbcGeom::ICurvesSchema::Sample &sample,
                   std::string &arnoldBasis);
   
   bool fillCurvesPositions(CurvesInfo &info,
                            size_t motionStep,
                            size_t numMotionSteps,
                            Alembic::Abc::Int32ArraySamplePtr Nv,
                            Alembic::Abc::P3fArraySamplePtr P0,
                            Alembic::Abc::FloatArraySamplePtr W0,
                            Alembic::Abc::P3fArraySamplePtr P1,
                            Alembic::Abc::FloatArraySamplePtr W1,
                            const float *vel,
                            const float *acc,
                            float blend,
                            Alembic::Abc::FloatArraySamplePtr K,
                            AtArray* &num_points,
                            AtArray* &points);
   
   bool getReferenceCurves(AlembicCurves &node, CurvesInfo &info,
                           AlembicCurves* &refCurves,
                           UserAttributes* &pointAttrs);
   
   bool fillReferencePositions(AlembicCurves *refCurves,
                               CurvesInfo &info,
                               UserAttributes *pointAttrs);
   
   void removeConflictingAttribs(AtNode *atnode,
                                 UserAttributes *obja,
                                 UserAttributes *prma,
                                 UserAttributes *pnta,
                                 UserAttributes *vtxa,
                                 bool verbose=false);
   
private:

   class Dso *mDso;
   AtNode *mNode;
};

template <class Info>
inline bool MakeShape::isVaryingFloat3(Info &info, const UserAttribute &ua)
{
   if (ua.arnoldCategory == AI_USERDEF_VARYING &&
       ((ua.arnoldType == AI_TYPE_VECTOR && ua.dataCount == info.pointCount) ||
        (ua.arnoldType == AI_TYPE_POINT && ua.dataCount == info.pointCount) ||
        (ua.arnoldType == AI_TYPE_FLOAT && ua.dataCount == 3 * info.pointCount)))
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline bool MakeShape::isIndexedFloat3(MeshInfo &info, const UserAttribute &ua)
{
   if (ua.arnoldCategory == AI_USERDEF_INDEXED &&
       ua.indicesCount == info.vertexCount &&
       (ua.arnoldType == AI_TYPE_VECTOR || ua.arnoldType == AI_TYPE_POINT))
   {
      return true;
   }
   else
   {
      return false;
   }
}

inline float MakeShape::adjustRadius(float radius) const
{
   radius *= mDso->radiusScale();
   
   if (radius < mDso->radiusMin())
   {
      return mDso->radiusMin();
   }
   else if (radius > mDso->radiusMax())
   {
      return mDso->radiusMax();
   }
   else
   {
      return radius;
   }
}

template <class T>
void MakeShape::outputInstanceNumber(AlembicNodeT<T> &node, AlembicNode *instance)
{
   if (AiNodeLookUpUserParameter(mDso->procNode(), "instance_num") == 0)
   {
      size_t inum = (instance ? instance->instanceNumber() : node.instanceNumber());
      
      if (AiNodeLookUpUserParameter(mNode, "instance_num") == 0)
      {
         AiNodeDeclare(mNode, "instance_num", "constant INT");
      }
      AiNodeSetInt(mNode, "instance_num", int(inum));
   }
}

template <class Schema>
AtNode* MakeShape::generateVolumeBox(Schema &schema)
{
   Alembic::Abc::IBox3dProperty boundsProp = schema.getSelfBoundsProperty();
   
   if (!boundsProp.valid())
   {
      return 0;
   }
   
   AtNode *node = AiNode("box");
   
   AiNodeSetFlt(node, "step_size", mDso->volumeStepSize());
   
   TimeSampleList<Alembic::Abc::IBox3dProperty> boundsSamples;
   TimeSampleList<Alembic::Abc::IBox3dProperty>::ConstIterator b0, b1;
   
   Alembic::Abc::Box3d box;
   box.makeEmpty();
   
   for (unsigned int i=0; i<mDso->numMotionSamples(); ++i)
   {
      double t = mDso->motionSampleTime(i);
      
      if (boundsSamples.update(boundsProp, t, t, i>0))
      {
         double b = 0.0;
         
         if (boundsSamples.getSamples(t, b0, b1, b))
         {
            if (b > 0.0)
            {
               double a = 1.0 - b;
               
               Alembic::Abc::V3d bmin = a * b0->data().min + b * b1->data().min;
               Alembic::Abc::V3d bmax = a * b0->data().max + b * b1->data().max;
               
               box.extendBy(Alembic::Abc::Box3d(bmin, bmax));
            }
            else
            {
               box.extendBy(b0->data());
            }
         }
      }
   }
   
   AiNodeSetPnt(node, "min", box.min.x, box.min.y, box.min.z);
   AiNodeSetPnt(node, "max", box.max.x, box.max.y, box.max.z);
   
   AtArray *shaders = AiNodeGetArray(mDso->procNode(), "shader");
   if (shaders && shaders->nelements > 0)
   {
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Force volume shader");
      }
      AiNodeSetArray(node, "shader", AiArrayCopy(shaders));
   }
   
   return node;
}

template <class MeshSchema>
AtNode* MakeShape::generateBaseMesh(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node,
                                    AlembicNode *instance,
                                    MeshInfo &info,
                                    std::vector<float*> *smoothNormals)
{
   info.pointCount = 0;
   info.polygonCount = 0;
   info.vertexCount = 0;
   info.polygonVertexCount = 0;
   info.vertexPointIndex = 0;
   info.arnoldVertexIndex = 0;
   
   AtArray *nsides = 0;
   AtArray *vidxs = 0;
   AtArray *vlist = 0;
   
   MeshSchema schema = node.typedObject().getSchema();
   
   TimeSampleList<MeshSchema> &meshSamples = node.samples().schemaSamples;
   typename TimeSampleList<MeshSchema>::ConstIterator samp0, samp1;
   
   if (info.varyingTopology)
   {
      node.sampleSchema(mDso->renderTime(), mDso->renderTime(), false);
   }
   else
   {
      for (size_t i=0; i<mDso->numMotionSamples(); ++i)
      {
         double t = mDso->motionSampleTime(i);
      
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample mesh \"%s\" at t=%lf", node.path().c_str(), t);
         }
      
         node.sampleSchema(t, t, i > 0);
      }
   }
   
   if (mDso->verbose())
   {
      AiMsgInfo("[abcproc] Read %lu mesh samples", meshSamples.size());
   }
   
   if (meshSamples.size() == 0)
   {
      return 0;
   }
   
   AtNode *mesh = createArnoldNode("polymesh", (instance ? *instance : node), true);
   
   AtPoint pnt;
   
   if (meshSamples.size() == 1 && !info.varyingTopology)
   {
      samp0 = meshSamples.begin();
      
      Alembic::Abc::P3fArraySamplePtr P = samp0->data().getPositions();
      Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
      Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
      
      info.pointCount = P->size();
      info.vertexCount = FI->size();
      info.polygonCount = FC->size();
      info.polygonVertexCount = (unsigned int*) AiMalloc(sizeof(unsigned int) * FC->size());
      info.vertexPointIndex = (unsigned int*) AiMalloc(sizeof(unsigned int) * FI->size());
      info.arnoldVertexIndex = (unsigned int*) AiMalloc(sizeof(unsigned int) * FI->size());
      
      vlist = AiArrayAllocate(P->size(), 1, AI_TYPE_POINT);
      nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
      vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
      
      for (size_t i=0, j=0; i<FC->size(); ++i)
      {
         unsigned int nv = FC->get()[i];
         
         info.polygonVertexCount[i] = nv;
         
         if (nv == 0)
         {
            continue;
         }
         
         // Alembic winding is CW not CCW
         if (mDso->reverseWinding())
         {
            // Keep first vertex unchanged
            info.arnoldVertexIndex[j] = j;
            info.vertexPointIndex[j] = FI->get()[j];
            
            for (size_t k=1; k<nv; ++k)
            {
               info.arnoldVertexIndex[j+k] = j + nv - k;
               info.vertexPointIndex[j+nv-k] = FI->get()[j+k];
            }
            
            j += nv;
         }
         else
         {
            for (size_t k=0; k<nv; ++k, ++j)
            {
               info.arnoldVertexIndex[j] = j;
               info.vertexPointIndex[j] = FI->get()[j];
            }
         }
      }
      
      for (size_t i=0; i<P->size(); ++i)
      {
         Alembic::Abc::V3f p = (*P)[i];
         
         pnt.x = p.x;
         pnt.y = p.y;
         pnt.z = p.z;
         
         AiArraySetPnt(vlist, i, pnt);
      }
      
      if (smoothNormals)
      {
         smoothNormals->push_back(computeMeshSmoothNormals(info, (const float*) P->getData(), 0, 0.0f));
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Read %lu faces", FC->size());
         AiMsgInfo("[abcproc] Read %lu points", P->size());
         AiMsgInfo("[abcproc] Read %lu vertices", FI->size());
      }
   }
   else
   {
      if (info.varyingTopology)
      {
         double b = 0.0;
         
         meshSamples.getSamples(mDso->renderTime(), samp0, samp1, b);
         
         Alembic::Abc::P3fArraySamplePtr P = samp0->data().getPositions();
         Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
         Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
         
         info.polygonCount = FC->size();
         info.pointCount = P->size();
         info.vertexCount = FI->size();
         info.polygonVertexCount = (unsigned int*) AiMalloc(sizeof(unsigned int) * FC->size());
         info.vertexPointIndex = (unsigned int*) AiMalloc(sizeof(unsigned int) * FI->size());
         info.arnoldVertexIndex = (unsigned int*) AiMalloc(sizeof(unsigned int) * FI->size());
         
         nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
         vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
         
         // Get velocity
         const float *vel = 0;
         const char *velName = mDso->velocityName();
         UserAttributes::iterator it = info.pointAttrs.end();
         
         if (velName)
         {
            if (strcmp(velName, "<builtin>") != 0)
            {
               it = info.pointAttrs.find(velName);
               if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.end();
               }
            }
         }
         else
         {
            it = info.pointAttrs.find("velocity");
            
            if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
            {
               it = info.pointAttrs.find("vel");
               if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.find("v");
                  if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.end();
                  }
               }
            }
         }
         
         if (it != info.pointAttrs.end())
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Using user defined attribute \"%s\" for point velocities", it->first.c_str());
            }
            vel = (const float*) it->second.data;
         }
         else if (samp0->data().getVelocities())
         {
            Alembic::Abc::V3fArraySamplePtr V = samp0->data().getVelocities();
            if (V->size() == P->size())
            {
               vel = (const float*) V->getData();
            }
            else
            {
               AiMsgWarning("[abcproc] Velocities count doesn't match points' one (%lu for %lu). Ignoring it.", V->size(), P->size());
            }
         }
         
         // Get acceleration
         const float *acc = 0;
         if (vel)
         {
            const char *accName = mDso->accelerationName();
            
            if (accName)
            {
               it = info.pointAttrs.find(accName);
               if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.end();
               }
            }
            else
            {
               it = info.pointAttrs.find("acceleration");
               
               if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
               {
                  it = info.pointAttrs.find("accel");
                  if (it == info.pointAttrs.end() || !isVaryingFloat3(info, it->second))
                  {
                     it = info.pointAttrs.find("a");
                     if (it != info.pointAttrs.end() && !isVaryingFloat3(info, it->second))
                     {
                        it = info.pointAttrs.end();
                     }
                  }
               }
            }
            
            if (it != info.pointAttrs.end())
            {
               acc = (const float*) it->second.data;
            }
         }
         
         for (size_t i=0, j=0; i<FC->size(); ++i)
         {
            unsigned int nv = FC->get()[i];
            
            info.polygonVertexCount[i] = nv;
            
            if (nv == 0)
            {
               continue;
            }
            
            if (mDso->reverseWinding())
            {
               info.arnoldVertexIndex[j] = j;
               info.vertexPointIndex[j] = FI->get()[j];
               
               for (size_t k=1; k<nv; ++k)
               {
                  info.arnoldVertexIndex[j+k] = j + nv - k;
                  info.vertexPointIndex[j+nv-k] = FI->get()[j+k];
               }
               
               j += nv;
            }
            else
            {
               for (size_t k=0; k<nv; ++k, ++j)
               {
                  info.arnoldVertexIndex[j] = j;
                  info.vertexPointIndex[j] = FI->get()[j];
               }
            }
         }
         
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Read %lu faces", FC->size());
            AiMsgInfo("[abcproc] Read %lu points", P->size());
            AiMsgInfo("[abcproc] Read %lu vertices", FI->size());
            
            if (vel)
            {
               AiMsgInfo("[abcproc] Use velocity to compute sample positions");
               if (acc)
               {
                  AiMsgInfo("[abcproc] Use velocity & acceleration to compute sample positions");
               }
            }
         }
         
         if (!vel)
         {
            vlist = AiArrayAllocate(P->size(), 1, AI_TYPE_POINT);
            
            for (size_t i=0; i<P->size(); ++i)
            {
               Alembic::Abc::V3f p = P->get()[i];
               
               pnt.x = p.x;
               pnt.y = p.y;
               pnt.z = p.z;
               
               AiArraySetPnt(vlist, i, pnt);
            }
            
            if (smoothNormals)
            {
               smoothNormals->push_back(computeMeshSmoothNormals(info, (const float*) P->getData(), 0, 0.0f));
            }
         }
         else
         {
            vlist = AiArrayAllocate(P->size(), mDso->numMotionSamples(), AI_TYPE_POINT);
            
            // extrapolated positions to use for smooth normal computation
            float *eP = 0;
               
            if (smoothNormals)
            {
               eP = (float*) AiMalloc(3 * P->size() * sizeof(float));
            }
            
            for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
            {
               double dt = mDso->motionSampleTime(i) - mDso->renderTime();
               dt *= mDso->velocityScale();
               
               if (acc)
               {
                  const float *vvel = vel;
                  const float *vacc = acc;
                  
                  for (size_t k=0, l=0; k<P->size(); ++k, vvel+=3, vacc+=3, l+=3)
                  {
                     Alembic::Abc::V3f p = P->get()[k];
                     
                     pnt.x = p.x + dt * (vvel[0] + 0.5 * dt * vacc[0]);
                     pnt.y = p.y + dt * (vvel[1] + 0.5 * dt * vacc[1]);
                     pnt.z = p.z + dt * (vvel[2] + 0.5 * dt * vacc[2]);
                     
                     AiArraySetPnt(vlist, j+k, pnt);
                     
                     if (eP)
                     {
                        eP[l+0] = pnt.x;
                        eP[l+1] = pnt.y;
                        eP[l+2] = pnt.z;
                     }
                  }
               }
               else
               {
                  const float *vvel = vel;
                  
                  for (size_t k=0, l=0; k<P->size(); ++k, vvel+=3, l+=3)
                  {
                     Alembic::Abc::V3f p = P->get()[k];
                     
                     pnt.x = p.x + dt * vvel[0];
                     pnt.y = p.y + dt * vvel[1];
                     pnt.z = p.z + dt * vvel[2];
                     
                     AiArraySetPnt(vlist, j+k, pnt);
                     
                     if (eP)
                     {
                        eP[l+0] = pnt.x;
                        eP[l+1] = pnt.y;
                        eP[l+2] = pnt.z;
                     }
                  }
               }
               
               if (smoothNormals)
               {
                  smoothNormals->push_back(computeMeshSmoothNormals(info, eP, 0, 0.0f));
               }
               
               j += info.pointCount;
            }
            
            if (smoothNormals)
            {
               AiFree(eP);
            }
         }
      }
      else
      {
         AtPoint pnt;
         
         for (size_t i=0, j=0; i<mDso->numMotionSamples(); ++i)
         {
            double b = 0.0;
            double t = mDso->motionSampleTime(i);
            
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] Read position samples [t=%lf]", t);
            }
            
            meshSamples.getSamples(t, samp0, samp1, b);
            
            Alembic::Abc::P3fArraySamplePtr P0 = samp0->data().getPositions();
            
            if (info.pointCount > 0 && P0->size() != info.pointCount)
            {
               AiMsgWarning("[abcproc] Changing points count amongst sample");
               if (nsides) AiArrayDestroy(nsides);
               if (vidxs) AiArrayDestroy(vidxs);
               if (vlist) AiArrayDestroy(vlist);
               nsides = vidxs = vlist = 0;
               break;
            }
            
            info.pointCount = P0->size();
            
            if (!nsides)
            {
               Alembic::Abc::Int32ArraySamplePtr FC = samp0->data().getFaceCounts();
               Alembic::Abc::Int32ArraySamplePtr FI = samp0->data().getFaceIndices();
               
               info.polygonCount = FC->size();
               info.vertexCount = FI->size();
               info.polygonVertexCount = (unsigned int*) AiMalloc(sizeof(unsigned int) * FC->size());
               info.vertexPointIndex = (unsigned int*) AiMalloc(sizeof(unsigned int) * FI->size());
               info.arnoldVertexIndex = (unsigned int*) AiMalloc(sizeof(unsigned int) * FI->size());
               
               nsides = AiArrayAllocate(FC->size(), 1, AI_TYPE_UINT);
               vidxs = AiArrayAllocate(FI->size(), 1, AI_TYPE_UINT);
               
               for (size_t k=0, l=0; k<FC->size(); ++k)
               {
                  unsigned int nv = FC->get()[k];
                  
                  info.polygonVertexCount[k] = nv;
                  
                  if (nv == 0)
                  {
                     continue;
                  }
                  
                  if (mDso->reverseWinding())
                  {
                     info.arnoldVertexIndex[l] = l;
                     info.vertexPointIndex[l] = FI->get()[l];
                     
                     for (size_t m=1; m<nv; ++m)
                     {
                        info.arnoldVertexIndex[l+m] = l + nv - m;
                        info.vertexPointIndex[l+nv-m] = FI->get()[l+m];
                     }
                     
                     l += nv;
                  }
                  else
                  {
                     for (size_t m=0; m<nv; ++m, ++l)
                     {
                        info.arnoldVertexIndex[l] = l;
                        info.vertexPointIndex[l] = FI->get()[l];
                     }
                  }
               }
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu faces", FC->size());
                  AiMsgInfo("[abcproc] Read %lu vertices", FI->size());
               }
            }
            
            if (!vlist)
            {
               vlist = AiArrayAllocate(P0->size(), mDso->numMotionSamples(), AI_TYPE_POINT);
            }
            
            if (b > 0.0)
            {
               Alembic::Abc::P3fArraySamplePtr P1 = samp1->data().getPositions();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu points / %lu points [interpolate]", P0->size(), P1->size());
               }
               
               double a = 1.0 - b;
               
               for (size_t k=0; k<P0->size(); ++k)
               {
                  Alembic::Abc::V3f p0 = P0->get()[k];
                  Alembic::Abc::V3f p1 = P1->get()[k];
                  
                  pnt.x = a * p0.x + b * p1.x;
                  pnt.y = a * p0.y + b * p1.y;
                  pnt.z = a * p0.z + b * p1.z;
                  
                  AiArraySetPnt(vlist, j+k, pnt);
               }
               
               if (smoothNormals)
               {
                  smoothNormals->push_back(computeMeshSmoothNormals(info, (const float*) P0->getData(), (const float*) P1->getData(), b));
               }
            }
            else
            {
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu points", P0->size());
               }
               
               for (size_t k=0; k<P0->size(); ++k)
               {
                  Alembic::Abc::V3f p = P0->get()[k];
                  
                  pnt.x = p.x;
                  pnt.y = p.y;
                  pnt.z = p.z;
                  
                  AiArraySetPnt(vlist, j+k, pnt);
               }
               
               if (smoothNormals)
               {
                  smoothNormals->push_back(computeMeshSmoothNormals(info, (const float*) P0->getData(), 0, 0.0f));
               }
            }
            
            j += P0->size();
         }
      }
   }
   
   if (!nsides || !vidxs || !vlist || !info.polygonVertexCount || !info.vertexPointIndex)
   {
      AiMsgWarning("[abcproc] Failed to generate base mesh data");
      
      if (smoothNormals)
      {
         for (std::vector<float*>::iterator it=smoothNormals->begin(); it!=smoothNormals->end(); ++it)
         {
            AiFree(*it);
         }
         smoothNormals->clear();
      }
      
      AiNodeDestroy(mesh);
      
      if (nsides) AiArrayDestroy(nsides);
      if (vidxs) AiArrayDestroy(vidxs);
      if (vlist) AiArrayDestroy(vlist);
      
      if (info.polygonVertexCount) AiFree(info.polygonVertexCount);
      if (info.vertexPointIndex) AiFree(info.vertexPointIndex);
      if (info.arnoldVertexIndex) AiFree(info.arnoldVertexIndex);
      
      info.polygonVertexCount = 0;
      info.vertexPointIndex = 0;
      info.arnoldVertexIndex = 0;
      info.polygonCount = 0;
      info.pointCount = 0;
      
      return 0;
   }
   
   AiArraySetKey(nsides, 0, info.polygonVertexCount);
   AiArraySetKey(vidxs, 0, info.vertexPointIndex);
   
   AiNodeSetArray(mesh, "nsides", nsides);
   AiNodeSetArray(mesh, "vidxs", vidxs);
   AiNodeSetArray(mesh, "vlist", vlist);
   
   return mesh;
}

template <class MeshSchema>
bool MakeShape::computeMeshTangentSpace(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node,
                                        MeshInfo &info,
                                        AtArray *uvlist,
                                        AtArray *uvidxs,
                                        float* &N, float* &T, float* &B)
{
   TimeSampleList<MeshSchema> &meshSamples = node.samples().schemaSamples;
   typename TimeSampleList<MeshSchema>::ConstIterator samp0, samp1;
   double blend = 0.0;
   
   if (!node.sampleSchema(mDso->renderTime(), mDso->renderTime(), true))
   {
      return false;
   }
   
   if (!meshSamples.getSamples(mDso->renderTime(), samp0, samp1, blend))
   {
      return false;
   }
   
   Alembic::Abc::P3fArraySamplePtr P0 = samp0->data().getPositions();
   Alembic::Abc::P3fArraySamplePtr P1;
   Alembic::Abc::V3f p0, p1, p2;
   
   float a = 1.0f;
   float b = 0.0f;
   
   if (blend > 0.0 && !info.varyingTopology)
   {
      P1 = samp1->data().getPositions();
      b = float(blend);
      a = 1.0f - b;
   }
   
   size_t bytesize = 3 * info.pointCount * sizeof(float);
   
   if (!N)
   {
      N = computeMeshSmoothNormals(info, (const float*) P0->getData(), (const float*) P1->getData(), b);
      if (!N)
      {
         return false;
      }
   }
   
   // Build tangents and bitangent
   
   T = (float*) AiMalloc(bytesize);
   memset(T, 0, bytesize);
   
   B = (float*) AiMalloc(bytesize);
   memset(B, 0, bytesize);
   
   for (unsigned int f=0, v=0; f<info.polygonCount; ++f)
   {
      unsigned int nfv = info.polygonVertexCount[f];
      
      for (unsigned int fv=2; fv<nfv; ++fv)
      {
         unsigned int pi[3] = {info.vertexPointIndex[v         ],
                               info.vertexPointIndex[v + fv - 1],
                               info.vertexPointIndex[v + fv    ]};
         
         if (P1)
         {
            p0 = a * P0->get()[pi[0]] + b * P1->get()[pi[0]];
            p1 = a * P0->get()[pi[1]] + b * P1->get()[pi[1]];
            p2 = a * P0->get()[pi[2]] + b * P1->get()[pi[2]];
         }
         else
         {
            p0 = P0->get()[pi[0]];
            p1 = P0->get()[pi[1]];
            p2 = P0->get()[pi[2]];
         }
         
         AtPoint2 uv0 = AiArrayGetPnt2(uvlist, AiArrayGetUInt(uvidxs, v));
         AtPoint2 uv1 = AiArrayGetPnt2(uvlist, AiArrayGetUInt(uvidxs, v+fv-1));
         AtPoint2 uv2 = AiArrayGetPnt2(uvlist, AiArrayGetUInt(uvidxs, v+fv));
         
         // For any point Q(u,v) in triangle:
         // (0) Q - p0 = T * (u - u0) + B * (v - v0)
         // 
         // Replace Q by p1 and p2 in (0)
         // (1) p1 - p0 = T * (u1 - u0) + B * (v1 - v0)
         // (2) p2 - p0 = T * (u2 - u0) + B * (v2 - v0)
         
         float s1 = uv1.x - uv0.x;
         float t1 = uv1.y - uv0.y;
         
         float x1 = p1.x - p0.x;
         float y1 = p1.y - p0.y;
         float z1 = p1.z - p0.z;
         
         float s2 = uv2.x - uv0.x;
         float t2 = uv2.y - uv0.y;
         
         float x2 = p2.x - p0.x;
         float y2 = p2.y - p0.y;
         float z2 = p2.z - p0.z;
         
         float r = (s1 * t2 - s2 * t1);
         
         if (fabs(r) > 0.000001f)
         {
            r = 1.0f / r;
            
            Alembic::Abc::V3f t(r * (t2 * x1 - t1 * x2),
                                r * (t2 * y1 - t1 * y2),
                                r * (t2 * z1 - t1 * z2));
            t.normalize();
            
            Alembic::Abc::V3f b(r * (s1 * x2 - s2 * x1),
                                r * (s1 * y2 - s2 * y1),
                                r * (s1 * z2 - s2 * z1));
            b.normalize();
            
            for (int i=0; i<3; ++i)
            {
               unsigned int off = 3 * pi[i];
               for (int j=0; j<3; ++j)
               {
                  T[off+j] += t[j];
                  B[off+j] += b[j];
               }
            }
         }
      }
      
      v += nfv;
   }
   
   // Ortho-normalization step
   
   for (unsigned int p=0, off=0; p<info.pointCount; ++p, off+=3)
   {
      Alembic::Abc::V3f n(N[off], N[off+1], N[off+2]);
      Alembic::Abc::V3f t(T[off], T[off+1], T[off+2]);
      Alembic::Abc::V3f b(B[off], B[off+1], B[off+2]);
      
      t = (t - t.dot(n) * n).normalized();
      
      b = (b - b.dot(n) * n - b.dot(t) * t).normalized();
      
      T[off  ] = t.x;
      T[off+1] = t.y;
      T[off+2] = t.z;
      
      B[off  ] = b.x;
      B[off+1] = b.y;
      B[off+2] = b.z;
   }
   
   return true;
}

template <class MeshSchema>
void MakeShape::outputMeshUVs(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node,
                              AtNode *mesh,
                              MeshInfo &info)
{
   float *smoothNormals = 0;
   
   for (UVSets::iterator uvit=info.UVs.begin(); uvit!=info.UVs.end(); ++uvit)
   {
      TimeSampleList<Alembic::AbcGeom::IV2fGeomParam> sampler;
      TimeSampleList<Alembic::AbcGeom::IV2fGeomParam>::ConstIterator uv0, uv1;
      
      if (uvit->first == "uv")
      {
         AiMsgWarning("[abcproc] \"uv\" is a reserved UV set name");
         continue;
      }
      
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Sample \"%s\" uvs", uvit->first.length() > 0 ? uvit->first.c_str() : "uv");
      }
      
      if (sampler.update(uvit->second, mDso->renderTime(), mDso->renderTime(), true))
      {
         double blend = 0.0;
         AtPoint2 pnt2;
         
         AtArray *uvlist = 0;
         AtArray *uvidxs = 0;
         
         if (sampler.getSamples(mDso->renderTime(), uv0, uv1, blend))
         {
            if (blend > 0.0 && !info.varyingTopology)
            {
               double a = 1.0 - blend;
               
               Alembic::Abc::V2fArraySamplePtr vals0 = uv0->data().getVals();
               Alembic::Abc::V2fArraySamplePtr vals1 = uv1->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs0 = uv0->data().getIndices();
               Alembic::Abc::UInt32ArraySamplePtr idxs1 = uv1->data().getIndices();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu uvs / %lu uvs [interpolate]", vals0->size(), vals1->size());
               }
               
               if (idxs0)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read %lu uv indices", idxs0->size());
                  }
                  
                  if (idxs1)
                  {
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu uv indices", idxs1->size());
                     }
                     
                     if (idxs0->size() != idxs1->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<idxs0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[idxs0->get()[i]];
                           Alembic::Abc::V2f val1 = vals1->get()[idxs1->get()[i]];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, info.arnoldVertexIndex[i], i);
                        }
                     }
                  }
                  else
                  {
                     // I don't think this should happend
                     if (vals1->size() != idxs0->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(idxs0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<idxs0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[idxs0->get()[i]];
                           Alembic::Abc::V2f val1 = vals1->get()[i];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, info.arnoldVertexIndex[i], i);
                        }
                     }
                  }
               }
               else
               {
                  if (idxs1)
                  {
                     Alembic::Abc::UInt32ArraySamplePtr idxs1 = uv1->data().getIndices();
                     
                     if (mDso->verbose())
                     {
                        AiMsgInfo("[abcproc] Read %lu uv indices", idxs1->size());
                     }
                     
                     if (idxs1->size() != vals0->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(vals0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(vals0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<vals0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[i];
                           Alembic::Abc::V2f val1 = vals1->get()[idxs1->get()[i]];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, info.arnoldVertexIndex[i], i);
                        }
                     }
                  }
                  else
                  {
                     if (vals0->size() != vals1->size())
                     {
                        AiMsgWarning("[abcproc] Ignore UVs: samples topology don't match");
                     }
                     else
                     {
                        uvlist = AiArrayAllocate(vals0->size(), 1, AI_TYPE_POINT2);
                        uvidxs = AiArrayAllocate(vals0->size(), 1, AI_TYPE_UINT);
                        
                        for (size_t i=0; i<vals0->size(); ++i)
                        {
                           Alembic::Abc::V2f val0 = vals0->get()[i];
                           Alembic::Abc::V2f val1 = vals1->get()[i];
                           
                           pnt2.x = a * val0.x + blend * val1.x;
                           pnt2.y = a * val0.y + blend * val1.y;
                           
                           AiArraySetPnt2(uvlist, i, pnt2);
                           AiArraySetUInt(uvidxs, info.arnoldVertexIndex[i], i);
                        }
                     }
                  }
               }
            }
            else
            {
               Alembic::Abc::V2fArraySamplePtr vals = uv0->data().getVals();
               Alembic::Abc::UInt32ArraySamplePtr idxs = uv0->data().getIndices();
               
               if (mDso->verbose())
               {
                  AiMsgInfo("[abcproc] Read %lu uvs", vals->size());
               }
               
               if (idxs)
               {
                  if (mDso->verbose())
                  {
                     AiMsgInfo("[abcproc] Read %lu uv indices", idxs->size());
                  }
                  
                  uvlist = AiArrayAllocate(vals->size(), 1, AI_TYPE_POINT2);
                  uvidxs = AiArrayAllocate(idxs->size(), 1, AI_TYPE_UINT);
                  
                  for (size_t i=0; i<vals->size(); ++i)
                  {
                     Alembic::Abc::V2f val = vals->get()[i];
                     
                     pnt2.x = val.x;
                     pnt2.y = val.y;
                     
                     AiArraySetPnt2(uvlist, i, pnt2);
                  }
                  
                  for (size_t i=0; i<idxs->size(); ++i)
                  {
                     AiArraySetUInt(uvidxs, info.arnoldVertexIndex[i], idxs->get()[i]);
                  }
               }
               else
               {
                  uvlist = AiArrayAllocate(vals->size(), 1, AI_TYPE_POINT2);
                  uvidxs = AiArrayAllocate(vals->size(), 1, AI_TYPE_UINT);
                  
                  for (size_t i=0; i<vals->size(); ++i)
                  {
                     Alembic::Abc::V2f val = vals->get()[i];
                     
                     pnt2.x = val.x;
                     pnt2.y = val.y;
                     
                     AiArraySetPnt2(uvlist, i, pnt2);
                     AiArraySetUInt(uvidxs, info.arnoldVertexIndex[i], i);
                  }
               }
            }
         }
         
         if (uvlist && uvidxs)
         {
            bool computeTangents = false;
            
            if (uvit->first.length() > 0)
            {
               std::string iname = uvit->first + "idxs";
               
               AiNodeDeclare(mesh, uvit->first.c_str(), "indexed POINT2");
               AiNodeSetArray(mesh, uvit->first.c_str(), uvlist);
               AiNodeSetArray(mesh, iname.c_str(), uvidxs);
               
               computeTangents = mDso->computeTangents(uvit->first);
            }
            else
            {
               AiNodeSetArray(mesh, "uvlist", uvlist);
               AiNodeSetArray(mesh, "uvidxs", uvidxs);
               
               computeTangents = mDso->computeTangents("uv");
            }
            
            if (computeTangents)
            {
               float *T = 0;
               float *B = 0;
               
               std::string Tname = "T" + uvit->first;
               std::string Bname = "B" + uvit->first;
               
               bool hasT = (info.pointAttrs.find(Tname) != info.pointAttrs.end());
               bool hasB = (info.pointAttrs.find(Bname) != info.pointAttrs.end());
               
               if (hasT && hasB)
               {
                  AiMsgWarning("[abcproc] Skip tangents generation for uv set \"%s\": attributes exists", uvit->first.c_str());
                  continue;
               }
               
               if (!computeMeshTangentSpace(node, info, uvlist, uvidxs, smoothNormals, T, B))
               {
                  AiMsgWarning("[abcproc] Skip tangents generation for uv set \"%s\": Failed to compute tangents", uvit->first.c_str());
                  continue;
               }
               
               if (hasT)
               {
                  AiMsgWarning("[abcproc] Point user attribute \"%s\" already exists", Tname.c_str());
                  if (T) AiFree(T);
               }
               else
               {
                  // copy normals in for test
                  UserAttribute &Tattr = info.pointAttrs[Tname];
                  
                  InitUserAttribute(Tattr);
                  
                  Tattr.arnoldCategory = AI_USERDEF_VARYING;
                  Tattr.arnoldType = AI_TYPE_VECTOR;
                  Tattr.arnoldTypeStr = "VECTOR";
                  Tattr.dataDim = 3;
                  Tattr.isArray = true;
                  Tattr.dataCount = info.pointCount;
                  Tattr.data = T;
               }
               
               if (hasB)
               {
                  AiMsgWarning("[abcproc] Point user attribute \"%s\" already exists", Bname.c_str());
                  if (B) AiFree(B);
               }
               else
               {
                  UserAttribute &Battr = info.pointAttrs[Bname];
                  
                  InitUserAttribute(Battr);
                  
                  Battr.arnoldCategory = AI_USERDEF_VARYING;
                  Battr.arnoldType = AI_TYPE_VECTOR;
                  Battr.arnoldTypeStr = "VECTOR";
                  Battr.dataDim = 3;
                  Battr.isArray = true;
                  Battr.dataCount = info.pointCount;
                  Battr.data = B;
               }
            }
         }
      }
      else
      {
         AiMsgWarning("[abcproc] Failed to output \"%s\" uvs", uvit->first.length() > 0 ? uvit->first.c_str() : "uv");
      }
   }
   
   if (smoothNormals)
   {
      #ifdef _DEBUG
      UserAttribute &ua = info.pointAttrs["Nsmooth"];
      
      InitUserAttribute(ua);
      
      ua.arnoldCategory = AI_USERDEF_VARYING;
      ua.arnoldType = AI_TYPE_VECTOR;
      ua.arnoldTypeStr = "VECTOR";
      ua.isArray = true;
      ua.dataDim = 3;
      ua.dataCount = info.pointCount;
      ua.data = smoothNormals;
      
      #else
      
      AiFree(smoothNormals);
      
      #endif
   }
}

template <class MeshSchema>
bool MakeShape::getReferenceMesh(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > &node, MeshInfo &info,
                                 AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> >* &refMesh,
                                 UserAttributes* &pointAttrs, UserAttributes* &vertexAttrs)
{
   if (!pointAttrs || !vertexAttrs)
   {
      // pointAttrs and vertexAttrs must be set
      refMesh = 0;
      return false;
   }
   
   if (pointAttrs == &(info.pointAttrs) || vertexAttrs == &(info.vertexAttrs))
   {
      // pointAttrs and vertexAttrs should point to attribute storage other than current scene's one
      refMesh = 0;
      return false;
   }
   
   // Priority
   //   1/ Pref attribute in current alembic scene
   //   2/ Pref attribute in reference alembic scene
   //   3/ Positions from reference alembic scene (considered to be static) x Mref
   //   4/ Positions from current alembic scene's referenceFrame frame (or closest) x Mref
   
   UserAttributes::iterator uait;
   bool hasPref = false;
   
   uait = info.pointAttrs.find(mDso->referencePositionName());
   
   if (uait != info.pointAttrs.end())
   {
      if (isVaryingFloat3(info, uait->second))
      {
         hasPref = true;
      }
      else
      {
         // Pref exists but doesn't match requirements, get rid of it
         DestroyUserAttribute(uait->second);
         info.pointAttrs.erase(uait);
      }
   }
   
   if (hasPref)
   {
      refMesh = &node;
      pointAttrs = &(info.pointAttrs);
      vertexAttrs = &(info.vertexAttrs);
   }
   else
   {
      AlembicScene *refScene = mDso->referenceScene();
      
      if (!refScene)
      {
         if (mDso->verbose())
         {
            if (mDso->hasReferenceFrame())
            {
               AiMsgInfo("[abcproc] Using current alembic scene frame %f as reference", mDso->referenceFrame());
            }
            else
            {
               AiMsgInfo("[abcproc] Using current alembic scene first sample as reference.");
            }
         }
         
         refMesh = &node;
         pointAttrs = &(info.pointAttrs);
         vertexAttrs = &(info.vertexAttrs);
      }
      else
      {
         AlembicNode *refNode = refScene->find(node.path());
         
         if (refNode)
         {
            refMesh = dynamic_cast<AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> >*>(refNode);
            
            if (!refMesh)
            {
               if (mDso->hasReferenceFrame())
               {
                  AiMsgWarning("[abcproc] Node in reference alembic doesn't have the same type. Using current alembic scene frame %f as reference", mDso->referenceFrame());
               }
               else
               {
                  AiMsgWarning("[abcproc] Node in reference alembic doesn't have the same type. Using current alembic scene first sample as reference");
               }
               
               refMesh = &node;
               pointAttrs = &(info.pointAttrs);
               vertexAttrs = &(info.vertexAttrs);
            }
            else
            {
               MeshSchema meshSchema = refMesh->typedObject().getSchema();
               
               double reftime = meshSchema.getTimeSampling()->getSampleTime(0);
               
               collectUserAttributes(meshSchema.getUserProperties(), meshSchema.getArbGeomParams(),
                                     reftime, false,
                                     0, 0, pointAttrs, vertexAttrs, 0,
                                     FilterReferenceAttributes, &mDso);
               
               uait = pointAttrs->find(mDso->referencePositionName());
               
               if (uait != pointAttrs->end())
               {
                  if (isVaryingFloat3(info, uait->second))
                  {
                     hasPref = true;
                  }
                  else
                  {
                     // Pref exists but doesn't match requirements, get rid of it
                     DestroyUserAttribute(uait->second);
                     pointAttrs->erase(uait);
                  }
               }
               else
               {
                  // using positions from refMesh
               }
            }
         }
         else
         {
            if (mDso->hasReferenceFrame())
            {
               AiMsgWarning("[abcproc] Couldn't find node in reference alembic scene. Using current alembic scene first frame %f as reference", mDso->referenceFrame());
            }
            else
            {
               AiMsgWarning("[abcproc] Couldn't find node in reference alembic scene. Using current alembic scene first sample as reference");
            }
            
            refMesh = &node;
            pointAttrs = &(info.pointAttrs);
            vertexAttrs = &(info.vertexAttrs);
         }
      }
   }
   
   return hasPref;
}

template <class MeshSchema>
bool MakeShape::fillReferencePositions(AlembicNodeT<Alembic::Abc::ISchemaObject<MeshSchema> > *refMesh,
                                       MeshInfo &info,
                                       UserAttributes* pointAttrs,
                                       Alembic::Abc::M44d &Mref)
{
   if (!refMesh || !pointAttrs)
   {
      return false;
   }
   
   MeshSchema meshSchema = refMesh->typedObject().getSchema();
   
   TimeSample<MeshSchema> meshSampler;
   typename MeshSchema::Sample meshSample;
   Alembic::Abc::P3fArraySamplePtr Pref;
   
   std::string PrefName = mDso->referencePositionName();
      
   bool hasPref = (pointAttrs->find(PrefName) != pointAttrs->end());
   
   // only sample if Pref attrib not found
   if (!hasPref)
   {
      // if using positions from current alembic scenes, try to sample at referenceFrame
      if (pointAttrs == &(info.pointAttrs) &&
          mDso->hasReferenceFrame() &&
          meshSampler.get(meshSchema, mDso->computeTime(mDso->referenceFrame())))
      {
         if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] Sample mesh Pref at t=%f (frame=%f)", mDso->computeTime(mDso->referenceFrame()), mDso->referenceFrame());
         }
         meshSample = meshSampler.data();
      }
      else
      {
         meshSample = meshSchema.getValue();
      }
      
      Pref = meshSample.getPositions();
   }
   
   if (!hasPref && Pref->size() == info.pointCount)
   {
      float *vals = (float*) AiMalloc(3 * info.pointCount * sizeof(float));
      
      // recompute matrix
      if (mDso->verbose())
      {
         AiMsgInfo("[abcproc] Compute reference world matrix");
      }
      
      AlembicXform *refParent = dynamic_cast<AlembicXform*>(refMesh->parent());
      
      while (refParent)
      {
         Alembic::AbcGeom::IXformSchema xformSchema = refParent->typedObject().getSchema();
         
         Alembic::AbcGeom::XformSample xformSample = xformSchema.getValue();
         
         Mref = Mref * xformSample.getMatrix();
         
         if (xformSchema.getInheritsXforms())
         {
            refParent = dynamic_cast<AlembicXform*>(refParent->parent());
         }
         else
         {
            refParent = 0;
         }
      }
      
      for (unsigned int p=0, off=0; p<info.pointCount; ++p, off+=3)
      {
         Alembic::Abc::V3f P = Pref->get()[p] * Mref;
         
         vals[off] = P.x;
         vals[off+1] = P.y;
         vals[off+2] = P.z;
      }
      
      UserAttribute &ua = info.pointAttrs[PrefName];
      
      InitUserAttribute(ua);
      
      ua.arnoldCategory = AI_USERDEF_VARYING;
      ua.arnoldType = AI_TYPE_POINT;
      ua.arnoldTypeStr = "POINT";
      ua.isArray = true;
      ua.dataDim = 3;
      ua.dataCount = info.pointCount;
      ua.data = vals;
      
      hasPref = true;
   }
   else
   {
      if (hasPref)
      {
         if (pointAttrs != &(info.pointAttrs))
         {
            // Transfer from reference shape attributes to current shape attribtues
            UserAttribute &src = (*pointAttrs)[PrefName];
            UserAttribute &dst = info.pointAttrs[PrefName];
            
            dst = src;
            
            pointAttrs->erase(pointAttrs->find(PrefName));
         }
         
         // Little type aliasing
         UserAttribute &ua = info.pointAttrs[PrefName];
         
         if (ua.dataDim == 1 && ua.dataCount == (3 * info.pointCount))
         {
            if (mDso->verbose())
            {
               AiMsgInfo("[abcproc] \"%s\" exported with wrong base type: float instead if float[3]", PrefName.c_str());
            }
            
            ua.dataDim = 3;
            ua.dataCount = info.pointCount;
            ua.arnoldType = AI_TYPE_POINT;
            ua.arnoldTypeStr = "POINT";
         }
         
         if (mDso->referenceScene())
         {
            AiMsgWarning("[abcproc] \"%s\" read from user attribute, ignore values from reference alembic", PrefName.c_str());
         }
         else if (mDso->verbose())
         {
            AiMsgInfo("[abcproc] \"%s\" read from user attribute", PrefName.c_str());
         }
      }
      else
      {
         AiMsgWarning("[abcproc] Could not generate \"%s\" for mesh \"%s\"", PrefName.c_str(), refMesh->path().c_str());
      }
   }
   
   return hasPref;
}

#endif
