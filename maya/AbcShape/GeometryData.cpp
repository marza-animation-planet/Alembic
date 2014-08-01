#include "GeometryData.h"

#include <iostream>

std::ostream& operator<<(std::ostream &os, MeshData::Tri &tri)
{
   os << "Tri " << tri.v0 << " - " << tri.v1 << " - " << tri.v2;
   return os;
}

// ---

MeshData::MeshData()
   : mNumPoints(0)
   , mPoints(0)
{
}

MeshData::~MeshData()
{
   clear();
}
   
void MeshData::_update(bool varyingTopology,
                       Alembic::Abc::Int32ArraySamplePtr fc,
                       Alembic::Abc::Int32ArraySamplePtr fi,
                       float t0,
                       float w0,
                       Alembic::Abc::P3fArraySamplePtr p0,
                       Alembic::Abc::V3fArraySamplePtr v0,
                       float t1,
                       float w1,
                       Alembic::Abc::P3fArraySamplePtr p1)
{  
   size_t np = p0->size();
   size_t nf = fc->size();
   size_t ni = fi->size();
   
   if (np < 1 || nf < 1 || ni < 1)
   {
      clear();
      return;
   }
   
   if (varyingTopology || (mNumPoints > 0 && mNumPoints != np))
   {
      clear();
   }
   
   // Update topology
   if (mTriangles.size() == 0)
   {
      #ifdef _DEBUG
      std::cout << "[MeshData] Rebuild faces" << std::endl;
      #endif
      
      const int32_t *counts = fc->get();
      const int32_t *indices = fi->get();
      size_t ii = 0;
      
      for (size_t i=0; i<nf; ++i)
      {
         size_t count = counts[i];
         bool skip = false;
         
         if (count >= 3)
         {
            if (ii + count > ni)
            {
               #ifdef _DEBUG
               std::cout << "[MeshData] Skip remaining faces" << std::endl;
               #endif
               break;
            }
            
            for (size_t j=ii; j<ii+count; ++j)
            {
               if (size_t(indices[j]) >= np)
               {
                  #ifdef _DEBUG
                  std::cout << "[MeshData] Skip face with invalid point index" << std::endl;
                  #endif
                  skip = true;
                  break;
               }
            }
            
            if (!skip)
            {
               // Note: Alembic store indices in clock-wise order
               mLines.push_back(Line(indices[ii], indices[ii+1]));
               
               for (size_t j=2; j<count; ++j)
               {
                  mTriangles.push_back(Tri(indices[ii], indices[ii+j], indices[ii+j-1]));
                  mLines.push_back(Line(indices[ii+j-1], indices[ii+j]));
               }
               
               mLines.push_back(Line(indices[ii+count-1], indices[ii]));
            }
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "[MeshData] Skip invalid face" << std::endl;
            #endif
         }
         
         ii += count;
      }
   }
   
   mNumPoints = np;
   
   // Update points & normals
   if (w1 > 0.0f)
   {
      if (varyingTopology)
      {
         if (v0 && v0->size() == np)
         {
            // Extrapolate using velocity
            
            float vs = w1 * (t1 - t0);
            
            mLocalPoints.resize(p0->size());
            for (size_t i=0; i<np; ++i)
            {
               mLocalPoints[i] = (*p0)[i] +  vs * (*v0)[i];
            }
            
            mPoints = &(mLocalPoints[0]);
         }
         else
         {
            // No velocities to rely on, cannot extrapolate
            mLocalPoints.clear();
            mPoints = p0->get();
         }
      }
      else
      {
         if (p1 && p1->size() == np)
         {
            // Interpolate 2 samples
            
            mLocalPoints.resize(p0->size());
            for (size_t i=0; i<np; ++i)
            {
               mLocalPoints[i] = w0 * (*p0)[i] + w1 * (*p1)[i];
            }
            
            mPoints = &(mLocalPoints[0]);
         }
         else
         {
            // Point count mismatch, cannot interpolate
            mLocalPoints.clear();
            mPoints = p0->get();
         }
      }
   }
   else
   {
      mLocalPoints.clear();
      mPoints = p0->get();
   }
   
   _computeNormals();
}

void MeshData::_computeNormals()
{
   if (!isValid())
   {
      mNormals.clear();
      return;
   }
   
   mNormals.resize(mNumPoints);
   std::fill(mNormals.begin(), mNormals.end(), Alembic::Abc::V3f(0.0f));

   for (size_t ti=0; ti<mTriangles.size(); ++ti)
   {
      const Tri &tri = mTriangles[ti];

      const Alembic::Abc::V3f &v0 = mPoints[tri.v0];
      const Alembic::Abc::V3f &v1 = mPoints[tri.v1];
      const Alembic::Abc::V3f &v2 = mPoints[tri.v2];

      Alembic::Abc::V3f e0 = v1 - v0;
      Alembic::Abc::V3f e2 = v2 - v0;
      Alembic::Abc::V3f wN = e0.cross(e2);
      
      mNormals[tri.v0] += wN;
      mNormals[tri.v1] += wN;
      mNormals[tri.v2] += wN;
   }

   // Normalize normals.
   for (size_t ni=0; ni<mNumPoints; ++ni)
   {
      mNormals[ni].normalize();
   }
}

void MeshData::draw(bool wireframe, float lineWidth) const
{
   if (!isValid())
   {
      return;
   }
   
   if (wireframe && lineWidth > 0.0f)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(lineWidth);
   }
   
   glEnableClientState(GL_VERTEX_ARRAY);
   
   if (!wireframe && mNormals.size() > 0)
   {
      glEnableClientState(GL_NORMAL_ARRAY);
      glNormalPointer(GL_FLOAT, 0, (const GLvoid*) &(mNormals[0]));
   }
   
   glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*) mPoints);
   
   if (wireframe)
   {
      glDrawElements(GL_LINES, (GLsizei) 2 * mLines.size(), GL_UNSIGNED_INT, (const GLvoid*) &(mLines[0].v0));
   }
   else
   {
      glDrawElements(GL_TRIANGLES, (GLsizei) 3 * mTriangles.size(), GL_UNSIGNED_INT, (const GLvoid*) &(mTriangles[0].v0));
   }
   
   if (!wireframe && mNormals.size() > 0)
   {
      glDisableClientState(GL_NORMAL_ARRAY);
   }
   
   glDisableClientState(GL_VERTEX_ARRAY);
}

void MeshData::drawPoints(float pointWidth) const
{
   if (!isValid())
   {
      return;
   }
   
   if (pointWidth > 0.0f)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_POINT_SMOOTH);
      glPointSize(pointWidth);
   }
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*) mPoints);
   glDrawArrays(GL_POINTS, 0, (GLsizei) mNumPoints);
   glDisableClientState(GL_VERTEX_ARRAY);
}

bool MeshData::isValid() const
{
   return (mTriangles.size() > 0  && mNumPoints > 0 && mPoints);
}

void MeshData::clear()
{
   mTriangles.clear();
   mLines.clear();
   mNumPoints = 0;
   mPoints = 0;
   mLocalPoints.clear();
   mNormals.clear();
}

// ---

PointsData::PointsData()
   : mNumPoints(0)
   , mPoints(0)
{
}

PointsData::~PointsData()
{
   clear();
}

void PointsData::update(const AlembicPoints &node)
{
   const typename AlembicPoints::Sample &samp0 = node.firstSample();
   const typename AlembicPoints::Sample &samp1 = node.secondSample();
   
   if (!samp0.valid(node.typedObject()))
   {
      clear();
      return;
   }
   
   float t0 = float(samp0.dataTime);
   float w0 = float(samp0.dataWeight);
   float t1 = 0.0f;
   float w1 = 0.0f;
   
   Alembic::Abc::P3fArraySamplePtr p0 = samp0.data.getPositions();
   Alembic::Abc::V3fArraySamplePtr v0 = samp0.data.getVelocities();
   Alembic::Abc::UInt64ArraySamplePtr id0 = samp0.data.getIds();
   
   mNumPoints = p0->size();
   
   if (mNumPoints == 0 || v0->size() != mNumPoints || id0->size() != mNumPoints)
   {
      clear();
      return;
   }
   
   if (samp1.dataWeight > 0.0)
   {
      mLocalPoints.resize(mNumPoints);
      
      t1 = float(samp1.dataTime);
      w1 = float(samp1.dataWeight);
      
      Alembic::Abc::P3fArraySamplePtr p1 = samp1.data.getPositions();
      Alembic::Abc::UInt64ArraySamplePtr id1 = samp1.data.getIds();
      
      float dt = w1 * (t1 - t0);
      
      std::map<size_t, size_t> idmap;
      std::map<size_t, size_t>::iterator idit;

      for (size_t j=0; j<id1->size(); ++j)
      {
         idmap[(*id1)[j]] = j;
      }
      
      for (size_t i = 0; i < mNumPoints; ++i)
      {
         Alembic::Abc::V3f p = (*p0)[i];
         Alembic::Abc::V3f v = (*v0)[i];
         
         size_t id = (*id0)[i];
         
         idit = idmap.find(id);
         
         if (idit == idmap.end())
         {
            mLocalPoints[i] = p + dt * v;
         }
         else
         {
            mLocalPoints[i] = w0 * p + w1 * (*p1)[idit->second];
         }
      }
      
      mPoints = &(mLocalPoints[0]);
   }
   else
   {
      mPoints = p0->get();
      mLocalPoints.clear();
   }
}

void PointsData::draw(float pointWidth) const
{
   if (!isValid())
   {
      return;
   }
   
   if (pointWidth > 0.0f)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_POINT_SMOOTH);
      glPointSize(pointWidth);
   }
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*) mPoints);
   glDrawArrays(GL_POINTS, 0, (GLsizei) mNumPoints);
   glDisableClientState(GL_VERTEX_ARRAY);
}

bool PointsData::isValid() const
{
   return (mNumPoints > 0 && mPoints);
}

void PointsData::clear()
{
   mLocalPoints.clear();
   mPoints = 0;
   mNumPoints = 0;
}

// ---

void DrawBox(const Alembic::Abc::Box3d &bounds, bool asPoints, float width)
{
   if (bounds.isEmpty() || bounds.isInfinite())
   {
      return;
   }
   
   if (width > 0.0f)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      if (asPoints)
      {
         glEnable(GL_POINT_SMOOTH);
         glPointSize(width);
      }
      else
      {
         glEnable(GL_LINE_SMOOTH);
         glLineWidth(width);
      }
   }
   
   float min_x = bounds.min[0];
   float min_y = bounds.min[1];
   float min_z = bounds.min[2];
   float max_x = bounds.max[0];
   float max_y = bounds.max[1];
   float max_z = bounds.max[2];
   
   float w = max_x - min_x;
   float h = max_y - min_y;
   float d = max_z - min_z;
   
   if (asPoints)
   {
      glBegin(GL_POINTS);
      
      glVertex3f(min_x,   min_y,   min_z  );
      glVertex3f(min_x,   min_y,   min_z+d);
      glVertex3f(min_x,   min_y+h, min_z  );
      glVertex3f(min_x,   min_y+h, min_z+d);
      glVertex3f(min_x+w, min_y,   min_z  );
      glVertex3f(min_x+w, min_y,   min_z+d);
      glVertex3f(min_x+w, min_y+h, min_z  );
      glVertex3f(min_x+w, min_y+h, min_z+d);
      
      glEnd();
   }
   else
   {
      glBegin(GL_LINES);

      glVertex3f(min_x, min_y, min_z);
      glVertex3f(min_x+w, min_y, min_z);
      glVertex3f(min_x, min_y, min_z);
      glVertex3f(min_x, min_y+h, min_z);
      glVertex3f(min_x, min_y, min_z);
      glVertex3f(min_x, min_y, min_z+d);
      glVertex3f(min_x+w, min_y, min_z);
      glVertex3f(min_x+w, min_y+h, min_z);
      glVertex3f(min_x+w, min_y+h, min_z);
      glVertex3f(min_x, min_y+h, min_z);
      glVertex3f(min_x, min_y, min_z+d);
      glVertex3f(min_x+w, min_y, min_z+d);
      glVertex3f(min_x+w, min_y, min_z+d);
      glVertex3f(min_x+w, min_y, min_z);
      glVertex3f(min_x, min_y, min_z+d);
      glVertex3f(min_x, min_y+h, min_z+d);
      glVertex3f(min_x, min_y+h, min_z+d);
      glVertex3f(min_x, min_y+h, min_z);
      glVertex3f(min_x+w, min_y+h, min_z);
      glVertex3f(min_x+w, min_y+h, min_z+d);
      glVertex3f(min_x+w, min_y, min_z+d);
      glVertex3f(min_x+w, min_y+h, min_z+d);
      glVertex3f(min_x, min_y+h, min_z+d);
      glVertex3f(min_x+w, min_y+h, min_z+d);

      glEnd();
   }
}

// ---

SceneGeometryData::SceneGeometryData()
{
}

SceneGeometryData::~SceneGeometryData()
{
   clear();
}

void SceneGeometryData::remove(const AlembicNode &node)
{
   NodeIndexMap::iterator it;
   
   it = mMeshIndices.find(node.path());
   if (it != mMeshIndices.end())
   {
      mMeshData[it->second].clear();
      mFreeMeshIndices.insert(it->second);
      mMeshIndices.erase(it);
   }
   
   it = mPointsIndices.find(node.path());
   if (it != mPointsIndices.end())
   {
      mPointsData[it->second].clear();
      mFreePointsIndices.insert(it->second);
      mPointsIndices.erase(it);
   }
}

void SceneGeometryData::clear()
{
   mMeshIndices.clear();
   mPointsIndices.clear();
   mCurvesIndices.clear();
   mNuPatchIndices.clear();
   
   mFreeMeshIndices.clear();
   mFreePointsIndices.clear();
   mFreeCurvesIndices.clear();
   mFreeNuPatchIndices.clear();
   
   mMeshData.clear();
   mPointsData.clear();
}