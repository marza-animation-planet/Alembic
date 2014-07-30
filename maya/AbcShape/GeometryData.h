#ifndef ABCSHAPE_GEOMETRYDATA_H_
#define ABCSHAPE_GEOMETRYDATA_H_

#include "AlembicScene.h"

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

#include <Alembic/Abc/Foundation.h>
#include <deque>

class MeshData
{
public:
   
   struct Tri
   {
      GLuint v0;
      GLuint v1;
      GLuint v2;
      
      inline Tri()
         : v0(0), v1(0), v2(0)
      {
      }  
      
      inline Tri(int32_t _v0, int32_t _v1, int32_t _v2)
         : v0(_v0), v1(_v1), v2(_v2)
      {
      }
      
      inline Tri(const Tri &rhs)
         : v0(rhs.v0), v1(rhs.v1), v2(rhs.v2)
      {
      }
      
      inline Tri& operator=(const Tri &rhs)
      {
         v0 = rhs.v0;
         v1 = rhs.v1;
         v2 = rhs.v2;
         return *this;
      }
   };
   
   typedef std::vector<Tri> TriArray;
   
public:
   
   MeshData();
   ~MeshData();
   
   inline void update(const AlembicMesh &mesh) { _update(mesh); }
   inline void update(const AlembicSubD &subd) { _update(subd); }
   inline const AlembicNode* node() const { return mNode; }
   
   void draw(bool wireframe, float lineWidth=0.0) const;
   void drawPoints(float pointWidth=0.0) const;
   
   bool isValid() const;
   void clear();

private:
   
   template <class IClass>
   void _update(const AlembicNodeT<IClass> &node)
   {
      const typename AlembicNodeT<IClass>::Sample &samp0 = node.firstSample();
      const typename AlembicNodeT<IClass>::Sample &samp1 = node.secondSample();
      
      IClass iObj = node.typedObject();
      
      if (!samp0.valid(iObj))
      {
         clear();
         return;
      }
      
      bool varyingTopology = (iObj.getSchema().getTopologyVariance() == Alembic::AbcGeom::kHeterogeneousTopology);
      
      Alembic::Abc::Int32ArraySamplePtr fc = samp0.data.getFaceCounts();
      Alembic::Abc::Int32ArraySamplePtr fi = samp0.data.getFaceIndices();
      Alembic::Abc::P3fArraySamplePtr p0 = samp0.data.getPositions();
      Alembic::Abc::V3fArraySamplePtr v0;
      Alembic::Abc::P3fArraySamplePtr p1;
      
      float t0 = float(samp0.dataTime);
      float w0 = float(samp0.dataWeight);
      float t1 = 0.0f;
      float w1 = 0.0f;
      
      if (samp1.dataWeight > 0.0)
      {
         t1 = float(samp1.dataTime);
         w1 = float(samp1.dataWeight);
         
         if (varyingTopology)
         {
            v0 = samp0.data.getVelocities();
         }
         else
         {
            if (samp1.valid(iObj))
            {
               p1 = samp1.data.getPositions();
            }
         }
      }
      
      mNode = &node;
      
      _update(varyingTopology, fc, fi, t0, w0, p0, v0, t1, w1, p1);
   }
   
   void _update(bool varyingTopology,
                Alembic::Abc::Int32ArraySamplePtr faceCounts,
                Alembic::Abc::Int32ArraySamplePtr faceIndices,
                float t0,
                float w0,
                Alembic::Abc::P3fArraySamplePtr p0,
                Alembic::Abc::V3fArraySamplePtr v0,
                float t1,
                float w1,
                Alembic::Abc::P3fArraySamplePtr p1);
   
   void _computeNormals();
   
private:
    
   size_t mNumPoints;
   const Alembic::Abc::V3f *mPoints;
   TriArray mTriangles;
   
   std::vector<Alembic::Abc::V3f> mLocalPoints;
   std::vector<Alembic::Abc::V3f> mNormals;
   
   const AlembicNode *mNode;
};

// PointsData
// CurvesData
// NuPatchData

// ---

class SceneGeometryData
{
public:
   
   SceneGeometryData();
   ~SceneGeometryData();
   
   inline MeshData* mesh(const AlembicNode &node)
   {
      return data<MeshData>(node, mMeshData, mMeshIndices, mFreeMeshIndices);
   }
   
   inline const MeshData* mesh(const AlembicNode &node) const
   {
      return data<MeshData>(node, mMeshData, mMeshIndices);
   }
   
   //PointsData& points(const AlembicNode &node);
   //const PointsData& points(const AlembicNode &node) const;
   
   //CurvesData& curves(const AlembicNode &node);
   //const CurvesData& curves(const AlembicNode &node) const;
   
   //NuPatchData& nupatch(const AlembicNode &node);
   //const NuPatchData& nupatch(const AlembicNode &node) const;
   
   void remove(const AlembicNode&);
   void clear();
   
private:
   
   typedef std::map<std::string, size_t> NodeIndexMap;
   
   template <class T>
   T* data(const AlembicNode &node, std::deque<T> &allData, NodeIndexMap &indices, std::set<size_t> &freeIndices)
   {
      T *data = 0;
   
      NodeIndexMap::iterator it = indices.find(node.path());
      
      if (it == indices.end())
      {
         size_t idx = 0;
         
         if (freeIndices.size() > 0)
         {
            std::set<size_t>::iterator iit = freeIndices.begin();
            idx = *iit;
            freeIndices.erase(iit);
         }
         else
         {
            idx = allData.size();
            allData.push_back(T());
         }
         
         indices[node.path()] = idx;
         
         data = &(allData[idx]);
      }
      else
      {
         data = &(allData[it->second]);
      }
      
      return data;
   }
   
   template <class T>
   const T* data(const AlembicNode &node, const std::deque<T> &allData, const NodeIndexMap &indices) const
   {
      const T *data = 0;
   
      NodeIndexMap::const_iterator it = indices.find(node.path());
      
      if (it != indices.end())
      {
         data = &(allData[it->second]);
      }
      
      return data;
   }
   
private:
   
   std::deque<MeshData> mMeshData;
   //std::deque<PointsData> mPointsData;
   //std::deque<CurvesData> mCurvesData;
   //std::deque<NuPatchData> mNuPatchData;
   
   NodeIndexMap mMeshIndices;
   NodeIndexMap mPointsIndices;
   NodeIndexMap mCurvesIndices;
   NodeIndexMap mNuPatchIndices;
   
   std::set<size_t> mFreeMeshIndices;
   std::set<size_t> mFreePointsIndices;
   std::set<size_t> mFreeCurvesIndices;
   std::set<size_t> mFreeNuPatchIndices;
};


#endif
