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
      
      const Alembic::Abc::int32_t *counts = fc->get();
      const Alembic::Abc::int32_t *indices = fi->get();
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
      //glEnable(GL_BLEND);
      //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
      //glEnable(GL_BLEND);
      //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
   const TimeSampleList<Alembic::AbcGeom::IPointsSchema> &samples = node.samples().schemaSamples;
   
   TimeSampleList<Alembic::AbcGeom::IPointsSchema>::ConstIterator samp0, samp1;
   
   if (samples.empty())
   {
      clear();
      return;
   }
   
   samp0 = samples.begin();
   
   if (!samp0->valid())
   {
      clear();
      return;
   }
   
   float t0 = float(samp0->time());
   float w0 = 1.0f;
   float t1 = 0.0f;
   float w1 = 0.0f;
   
   Alembic::Abc::P3fArraySamplePtr p0 = samp0->data().getPositions();
   Alembic::Abc::V3fArraySamplePtr v0 = samp0->data().getVelocities();
   Alembic::Abc::UInt64ArraySamplePtr id0 = samp0->data().getIds();
   
   mNumPoints = p0->size();
   
   if (mNumPoints == 0 || v0->size() != mNumPoints || id0->size() != mNumPoints)
   {
      clear();
      return;
   }
   
   //if (samp1.dataWeight > 0.0)
   if (samples.size() > 1)
   {
      samp1 = samples.end();
      --samp1;
      
      t1 = float(samp1->time());
      w1 = float(samples.lastEvaluationTime - samp0->time()) / (samp1->time() - samp0->time());
      w0 = 1.0f - w1;
      
      mLocalPoints.resize(mNumPoints);
      
      Alembic::Abc::P3fArraySamplePtr p1 = samp1->data().getPositions();
      Alembic::Abc::UInt64ArraySamplePtr id1 = samp1->data().getIds();
      
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
      //glEnable(GL_BLEND);
      //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

CurvesData::CurvesData()
   : mNumCurves(0)
   , mNumPoints(0)
   , mPoints(0)
   , mWrap(false)
   , mNumVertices(0)
{
}

CurvesData::~CurvesData()
{
   clear();
}

void CurvesData::update(const AlembicCurves &curves)
{
   const TimeSampleList<Alembic::AbcGeom::ICurvesSchema> &samples = curves.samples().schemaSamples;
   
   TimeSampleList<Alembic::AbcGeom::ICurvesSchema>::ConstIterator samp0, samp1;
   
   if (samples.empty())
   {
      clear();
      return;
   }
   
   samp0 = samples.begin();
   
   if (!samp0->valid())
   {
      clear();
      return;
   }
   
   float t0 = float(samp0->time());
   float w0 = 1.0f;
   float t1 = 0.0f;
   float w1 = 0.0f;
   
   mNumCurves = samp0->data().getNumCurves();
   
   if (mNumCurves == 0)
   {
      clear();
      return;
   }
   
   mNumPoints = samp0->data().getCurvesNumVertices()->get();
   mWrap = (samp0->data().getWrap() == Alembic::AbcGeom::kPeriodic);
   
   Alembic::Abc::P3fArraySamplePtr p0 = samp0->data().getPositions();
   
   mNumVertices = p0->size();
   
   if (samples.size() > 1)
   {
      bool interpolate = true;
      
      samp1 = samples.end();
      --samp1;
      
      t1 = float(samp1->time());
      w1 = float(samples.lastEvaluationTime - samp0->time()) / (samp1->time() - samp0->time());
      w0 = 1.0f - w1;
      
      // Check for matching number of curves and vertices per curve
      
      if (samp1->data().getNumCurves() != mNumCurves)
      {
         interpolate = false;
      }
      else
      {
         Alembic::Abc::Int32ArraySamplePtr nv1 = samp1->data().getCurvesNumVertices();
         
         for (size_t i=0; i<mNumCurves; ++i)
         {
            if (mNumPoints[i] != (*nv1)[i])
            {
               interpolate = false;
               break;
            }
         }
      }
      
      if (!interpolate)
      {
         Alembic::Abc::V3fArraySamplePtr v0 = samp0->data().getVelocities();
         
         if (v0 && v0->size() == p0->size())
         {
            float dt = w1 * (t1 - t0);
            
            mLocalPoints.resize(mNumVertices);
            
            for (size_t i=0; i<mNumVertices; ++i)
            {
               mLocalPoints[i] = (*p0)[i] + dt * (*v0)[i];
            }
            
            mPoints = &(mLocalPoints[0]);
         }
         else
         {
            mPoints = p0->get();
            mLocalPoints.clear();
         }
      }
      else
      {
         Alembic::Abc::P3fArraySamplePtr p1 = samp1->data().getPositions();
         
         if (p1 && p1->size() == p0->size())
         {
            mLocalPoints.resize(mNumVertices);
            
            for (size_t i=0; i<mNumVertices; ++i)
            {
               mLocalPoints[i] = w0 * (*p0)[i] + w1 * (*p1)[i];
            }
            
            mPoints = &(mLocalPoints[0]);
         }
         else
         {
            mPoints = p0->get();
            mLocalPoints.clear();
         }
      }
   }
   else
   {
      mPoints = p0->get();
      mLocalPoints.clear();
   }
}

void CurvesData::drawPoints(float pointWidth) const
{
   if (!isValid())
   {
      return;
   }
   
   if (pointWidth > 0.0f)
   {
      glEnable(GL_POINT_SMOOTH);
      glPointSize(pointWidth);
   }
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*) mPoints);
   glDrawArrays(GL_POINTS, 0, (GLsizei) mNumVertices);
   glDisableClientState(GL_VERTEX_ARRAY);
}

void CurvesData::draw(float lineWidth) const
{
   if (!isValid())
   {
      return;
   }
   
   if (lineWidth > 0.0f)
   {
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(lineWidth);
   }
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, (const GLvoid*) mPoints);
   
   GLint p = 0;
   for (size_t c=0; c<mNumCurves; ++c)
   {
      Alembic::Abc::int32_t nv = mNumPoints[c];
      glDrawArrays((mWrap ? GL_LINE_LOOP : GL_LINE_STRIP), p, (GLsizei) nv);
      p += nv;
   }
   
   glDisableClientState(GL_VERTEX_ARRAY);
}

bool CurvesData::isValid() const
{
   return (mNumCurves > 0 && mNumPoints && mPoints);
}

void CurvesData::clear()
{
   mNumCurves = 0;
   mNumPoints = 0;
   mPoints = 0;
   mLocalPoints.clear();
   mWrap = false;
   mNumVertices = 0;
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
      //glEnable(GL_BLEND);
      //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

void DrawLocator(const Alembic::Abc::V3d &p, const Alembic::Abc::V3d &s, float width)
{
   if (s.length2() < 0.01)
   {
      return;
   }
   
   if (width > 0.0f)
   {
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(width);
   }
   
   GLdouble curColor[4];
   glGetDoublev(GL_CURRENT_COLOR, curColor);
   
   bool lightingOn = glIsEnabled(GL_LIGHTING);
   
   if (lightingOn)
   {
      glDisable(GL_LIGHTING);
   }
   
   glBegin(GL_LINES);
   
   glColor3d(0.7, 0, 0);
   glVertex3d(p.x - s.x, p.y, p.z);
   glVertex3d(p.x + s.x, p.y, p.z);
   
   glColor3d(0, 0.7, 0);
   glVertex3d(p.x, p.y - s.y, p.z);
   glVertex3d(p.x, p.y + s.y, p.z);
   
   glColor3d(0, 0, 0.7);
   glVertex3d(p.x, p.y, p.z - s.z);
   glVertex3d(p.x, p.y, p.z + s.z);
   
   if (lightingOn)
   {
      glEnable(GL_LIGHTING);
   }
   
   glColor4dv(curColor);
   
   glEnd();
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
   
   it = mCurvesIndices.find(node.path());
   if (it != mCurvesIndices.end())
   {
      mCurvesData[it->second].clear();
      mFreeCurvesIndices.insert(it->second);
      mCurvesIndices.erase(it);
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
   mCurvesData.clear();
}