/*
Copyright (C) 2015  Gaetan Guidet
This file is based on gmath library (https://github.com/gatgui/gmath)
gmath is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.
gmath is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
USA.
*/

#ifndef ABCPROC_NURBS_H_
#define ABCPROC_NURBS_H_

#include <cstdlib>
#include <cstdarg>
#include <cmath>
#include <vector>
#include <stdexcept>


template <unsigned int D>
class Vector
{
public:
   
   Vector();
   Vector(float v);
   Vector(const float *v);
   Vector(const char *fmt, ...);
   Vector(const Vector<D> &rhs);
   ~Vector();
   
   unsigned int size() const;
   void zero();
   void set(const float *v);
   float dot(const Vector<D> &rhs) const;
   float getSquaredLength() const;
   float getLength() const;
   Vector<D>& normalize();
   Vector<D>& floor(const Vector<D> &rhs);
   Vector<D>& ceil(const Vector<D> &rhs);
   void swap(unsigned int i, unsigned int j);
   
   template <unsigned int dim>
   Vector<D>& operator=(const Vector<dim> &rhs);
   Vector<D>& operator*=(float f);
   Vector<D>& operator*=(const Vector<D> &rhs);
   Vector<D>& operator/=(float f);
   Vector<D>& operator+=(const Vector<D> &rhs);
   Vector<D>& operator-=(const Vector<D> &rhs);
   float& operator[](unsigned int i);
   float operator[](unsigned int i) const;
   
public:
   
   float values[D];
};

// ---

template <unsigned int D>
class NURBS
{
public:
    
   typedef Vector<D+1> CV;
   typedef Vector<D> Pnt, Vec;
   typedef std::vector<CV> CVArray;
   typedef std::vector<Pnt> PntArray;
   typedef std::vector<float> WeightArray;
   typedef std::vector<float> KnotArray;
    
public:
    
   NURBS(long degree=3);
   NURBS(const NURBS<D> &rhs);
   virtual ~NURBS();

   NURBS<D>& operator=(const NURBS<D> &rhs);

   bool isValid() const;
   bool hasValidCVsAndKnotsCount() const;
   bool hasValidKnotSequence() const;

   long getDegree() const;
   void setDegree(long d);

   void clear();

   bool getBoundingBox(Pnt &min, Pnt &max) const;

   // cvs access
   long getNumCVs() const;
   void setNumCVs(long n);
   const CV& getCV(long idx) const;
   void setCV(long idx, const CV &cv);
   long getCVs(CVArray &cvs) const;
   void setCVs(const CVArray &cvs);
   Pnt getPoint(long idx) const;
   void setPoint(long idx, const Pnt &p);
   long getPoints(PntArray &points) const;
   void setPoints(const PntArray &points);
   float getWeight(long idx) const;
   void setWeight(long idx, float w);
   long getWeights(WeightArray &weights) const;
   void setWeights(const WeightArray &weights);
   long resizeCVs(); // to match knots count
   void adjustCVsCount(long n, float tol=0.01f);
    
   // knots access
   long getNumKnots() const;
   void setNumKnots(long n);
   float getKnot(long idx) const;
   long getKnots(std::vector<float> &knots) const;
   void setKnot(long idx, float k);
   void setKnots(const std::vector<float> &knots);

   // advanced knots operations
   bool insertKnot(float u, long n=1);
   long getKnotMultiplicity(float u, long span=-1) const;
   long getKnotMultiplicity(long k) const;
   void buildKnotsUniform(bool clamp, bool normalize=true);
   void buildKnotsChordLength();
   void buildKnotsCentripetal(float strength=0.5f);
   bool findKnotSpan(float u, long &k) const;
   long resizeKnots(); // to match cvs count
    
   // domain query
   bool getDomain(float &start, float &end) const;
   bool isInDomain(float u) const;

   // evaluation
   long computeBasis(float u, float *basis) const;
   bool derivate(NURBS<D> &d) const;
   bool eval(float u, Pnt &r) const;
   bool evalDerivative(float u, Vec &r) const;

   // length operations
   float getLength(float precision=0.001f) const;
   float getLengthTo(float to, float precision=0.001f) const;
   float getLengthFrom(float from, float precision=0.001f) const;
   float getLengthBetween(float from, float to, float precision=0.001f) const;
   float getParamAtLength(float l, float precision=0.001f);

   // vector operations
   bool getTangent(float u, Vec &t, bool normalize=true);
   bool getNormal(float u, Vec &n, bool normalize=true);

   // splitting operations
   bool subdivide(float u, NURBS<D> &before, NURBS<D> &after);
   bool trimAfter(float u);
   bool trimBefore(float u);
   bool trim(float from, float to);

   // convertion operations
   void convertToBezier(bool normalize);
    
protected:
    
   float dotCV(const CV &cv0, const CV &cv1) const;
   float lenCV(const CV &cv) const;
    
protected:
    
   long mDegree;
   CVArray mCVs;
   KnotArray mKnots;
   mutable bool mDirty;
   mutable bool mValid;
   CV mDefCV;
};

// ---

template <unsigned int D>
inline Vector<D>::Vector()
{
   zero();
}

template <unsigned int D>
inline Vector<D>::Vector(float v)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] = v;
   }
}

template <unsigned int D>
inline Vector<D>::Vector(const float *v)
{
   memcpy(values, v, D*sizeof(float));
}

template <unsigned int D>
inline Vector<D>::Vector(const char *fmt, ...)
{
   zero();
   if (fmt)
   {
      unsigned int cur = 0;
      const char *cc = fmt;
      va_list al;
      va_start(al, fmt);
      while (*cc != '\0' && cur<D)
      {
         switch (*cc)
         {
         case 'g':
         case 'f':
            {
               double v = va_arg(al, double);
               values[cur] = float(v);
            }
            break;
         case 'u':
         case 'd':
         case 'l':
         case 'i':
            {
               int i = va_arg(al, int);
               values[cur] = float(i);
            }
            break;
         default:
            cur = D; // will break the outer loop
         }
         ++cc;
         ++cur;
      }
      va_end(al);
   }
}

template <unsigned int D>
inline Vector<D>::Vector(const Vector<D> &rhs)
{
   memcpy(values, rhs.values, D*sizeof(float));
}

template <unsigned int D>
inline Vector<D>::~Vector()
{
}

template <unsigned int D>
inline unsigned int Vector<D>::size() const
{
   return D;
}

template <unsigned int D>
inline void Vector<D>::zero()
{
   memset(values, 0, D*sizeof(float));
}

template <unsigned int D>
inline void Vector<D>::set(const float *v)
{
   memcpy(values, v, D*sizeof(float));
}

template <unsigned int D>
inline float Vector<D>::dot(const Vector<D> &rhs) const
{
   float r = 0.0f;
   for (unsigned int i=0; i<D; ++i)
   {
      r += values[i] * rhs[i];
   }
   return r;
}

template <unsigned int D>
inline float Vector<D>::getSquaredLength() const
{
   return dot(*this);
}

template <unsigned int D>
inline float Vector<D>::getLength() const
{
   return float(sqrt(getSquaredLength()));
}

template <unsigned int D>
inline Vector<D>& Vector<D>::normalize()
{
   float l = getSquaredLength();
   if (l > 0.000001f)
   {
      l = 1.0f / float(sqrt(l));
      for (unsigned int i=0; i<D; ++i)
      {
         values[i] *= l;
      }
   }
   return *this;
}

template <unsigned int D>
inline Vector<D>& Vector<D>::floor(const Vector<D> &rhs)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] = (values[i] > rhs.values[i]) ? rhs.values[i] : values[i];
   }
   return *this;
}

template <unsigned int D>
inline Vector<D>& Vector<D>::ceil(const Vector<D> &rhs)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] = (values[i] < rhs.values[i]) ? rhs.values[i] : values[i];
   }
   return *this;
}

template <unsigned int D>
inline void Vector<D>::swap(unsigned int i, unsigned int j)
{
   float tmp = values[i];
   values[i] = values[j];
   values[j] = tmp;
}

template <unsigned int D>
template <unsigned int dim>
inline Vector<D>& Vector<D>::operator=(const Vector<dim> &rhs)
{
   if (&(values[0]) != &(rhs.values[0]))
   {
      memcpy(values, rhs.values, (D < dim ? D : dim)*sizeof(float));
   }
   return *this;
}

template <unsigned int D>
inline Vector<D>& Vector<D>::operator*=(float f)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] *= f;
   }
   return *this;
}

template <unsigned int D>
inline Vector<D>& Vector<D>::operator*=(const Vector<D> &rhs)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] *= rhs[i];
   }
   return *this;
}

template <unsigned int D>
inline Vector<D>& Vector<D>::operator/=(float f)
{
   return operator*=(1.0f / f);
}

template <unsigned int D>
inline Vector<D>& Vector<D>::operator+=(const Vector<D> &rhs)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] += rhs.values[i];
   }
   return *this;
}

template <unsigned int D>
inline Vector<D>& Vector<D>::operator-=(const Vector<D> &rhs)
{
   for (unsigned int i=0; i<D; ++i)
   {
      values[i] -= rhs.values[i];
   }
   return *this;
}

template <unsigned int D>
inline float& Vector<D>::operator[](unsigned int i)
{
   return values[i];
}

template <unsigned int D>
inline float Vector<D>::operator[](unsigned int i) const
{
   return values[i];
}

// ---

template <unsigned int D>
inline Vector<D> operator+(const Vector<D> &v0, const Vector<D> &v1)
{
   Vector<D> result(v0);
   return result += v1;
}

template <unsigned int D>
inline Vector<D> operator-(const Vector<D> &v0, const Vector<D> &v1)
{
   Vector<D> result(v0);
   return result -= v1;
}

template <unsigned int D>
inline Vector<D> operator*(const Vector<D> &v0, const Vector<D> &v1)
{
   Vector<D> result(v0);
   return result *= v1;
}

template <unsigned int D>
inline Vector<D> operator*(const Vector<D> &v, float s)
{
   Vector<D> result(v);
   return result *= s;
}

template <unsigned int D>
inline Vector<D> operator/(const Vector<D> &v, float s)
{
   Vector<D> result(v);
   return result /= s;
}

template <unsigned int D>
inline Vector<D> operator*(float s, const Vector<D> &v)
{
   Vector<D> result(v);
   return result *= s;
}

// ---
  
template <unsigned int D>
NURBS<D>::NURBS(long degree)
   : mDegree(degree)
   , mDirty(true)
   , mValid(false)
{
   mDefCV[D] = 1.0f;
}

template <unsigned int D>
NURBS<D>::NURBS(const NURBS<D> &rhs)
   : mDegree(rhs.mDegree)
   , mCVs(rhs.mCVs)
   , mKnots(rhs.mKnots)
   , mDirty(rhs.mDirty)
   , mValid(rhs.mValid)
{
   mDefCV[D] = 1.0f;
}

template <unsigned int D>
NURBS<D>::~NURBS()
{
}

template <unsigned int D>
NURBS<D>& NURBS<D>::operator=(const NURBS<D> &rhs)
{
   if (this != &rhs)
   {
      mDegree = rhs.mDegree;
      mCVs = rhs.mCVs;
      mKnots = rhs.mKnots;
      mDirty = rhs.mDirty;
      mValid = rhs.mValid;
   }
   return *this;
}

template <unsigned int D>
bool NURBS<D>::getBoundingBox(typename NURBS<D>::Pnt &min, typename NURBS<D>::Pnt &max) const
{
   if (mCVs.size() == 0)
   {
      return false;
   }

   min.zero();
   max.zero();

   for (size_t i=1; i<mCVs.size(); ++i)
   {
      min.floor(mCVs[i]);
      max.ceil(mCVs[i]);
   }
   
   return true;
}

template <unsigned int D>
void NURBS<D>::adjustCVsCount(long, float)
{
   // TODO
   // keep end points
   // keep knots range or re-normalize
   // keep open/clamp status
   // keep degree
   throw std::runtime_error("NURBS::adjustCVsCount: Not implemented");
}

template <unsigned int D>
long NURBS<D>::getDegree() const
{
   return mDegree;
}

template <unsigned int D>
void NURBS<D>::setDegree(long d)
{
   mDegree = d;
   mDirty = true;
}

template <unsigned int D>
void NURBS<D>::clear()
{
   mCVs.clear();
   mKnots.clear();
   mDirty = true;
}

template <unsigned int D>
bool NURBS<D>::isValid() const
{
   if (mDirty)
   {
      mValid = (hasValidCVsAndKnotsCount() && hasValidKnotSequence());
      mDirty = false;
   }
   return mValid;
}

template <unsigned int D>
bool NURBS<D>::hasValidCVsAndKnotsCount() const
{
   if (getNumCVs() <= mDegree)
   {
      return false;
   }

   if (getNumKnots() != getNumCVs() + mDegree + 1)
   {
      return false;
   }

   return true;
}

template <unsigned int D>
bool NURBS<D>::hasValidKnotSequence() const
{
   for (long i=1; i<getNumKnots(); ++i)
   {
      if (mKnots[i-1] > mKnots[i])
      {
         return false;
      }
   }

   return true;
}

template <unsigned int D>
inline long NURBS<D>::getNumCVs() const
{
   return long(mCVs.size());
}

template <unsigned int D>
void NURBS<D>::setNumCVs(long n)
{
   mDirty = (n != getNumCVs());
   mCVs.resize(n);
}

template <unsigned int D>
const typename NURBS<D>::CV& NURBS<D>::getCV(long idx) const
{
   return mCVs[idx];
}

template <unsigned int D>
void NURBS<D>::setCV(long idx, const typename NURBS<D>::CV &cv)
{
   mCVs[idx] = cv;
}

template <unsigned int D>
long NURBS<D>::getCVs(typename NURBS<D>::CVArray &cvs) const
{
   cvs = mCVs;
   return long(cvs.size());
}

template <unsigned int D>
void NURBS<D>::setCVs(const typename NURBS<D>::CVArray &cvs)
{
   mDirty = (cvs.size() != mCVs.size());
   mCVs = cvs;
}

template <unsigned int D>
typename NURBS<D>::Pnt NURBS<D>::getPoint(long idx) const
{
   return mCVs[idx];
}

template <unsigned int D>
void NURBS<D>::setPoint(long idx, const typename NURBS<D>::Pnt &point)
{
   mCVs[idx] = point;
}

template <unsigned int D>
long NURBS<D>::getPoints(typename NURBS<D>::PntArray &points) const
{
   points.resize(mCVs.size());
   for (size_t i=0; i<mCVs.size(); ++i)
   {
      points[i] = Pnt(mCVs[i]);
   }
   return long(points.size());
}

template <unsigned int D>
void NURBS<D>::setPoints(const typename NURBS<D>::PntArray &points)
{
   mDirty = (points.size() != mCVs.size());
   if (mDirty)
   {
      mCVs.resize(points.size(), mDefCV);
   }
   for (size_t i=0; i<points.size(); ++i)
   {
      mCVs[i] = points[i];
   }
}

template <unsigned int D>
float NURBS<D>::getWeight(long idx) const
{
   return mCVs[idx][D];
}

template <unsigned int D>
void NURBS<D>::setWeight(long idx, float w)
{
   mCVs[idx][D] = w;
}

template <unsigned int D>
long NURBS<D>::getWeights(typename NURBS<D>::WeightArray &weights) const
{
   weights.resize(mCVs.size());
   for (size_t i=0; i<mCVs.size(); ++i)
   {
      weights[i] = mCVs[i][D];
   }
   return long(weights.size());
}

template <unsigned int D>
void NURBS<D>::setWeights(const typename NURBS<D>::WeightArray &weights)
{
   mDirty = (weights.size() != mCVs.size());
   if (mDirty)
   {
      mCVs.resize(weights.size());
   }
   for (size_t i=0; i<weights.size(); ++i)
   {
      mCVs[i][D] = weights[i];
   }
}

template <unsigned int D>
long NURBS<D>::resizeCVs()
{
   long n = getNumKnots() - mDegree - 1;
   n = std::max<long>(0, n);
   if (n != getNumCVs())
   {
      mDirty = true;
      mCVs.resize(n, mDefCV);
   }
   return n;
}

template <unsigned int D>
inline long NURBS<D>::getNumKnots() const
{
   return long(mKnots.size());
}

template <unsigned int D>
void NURBS<D>::setNumKnots(long n)
{
   mDirty = (n != getNumKnots());
   mKnots.resize(n);
}

template <unsigned int D>
float NURBS<D>::getKnot(long idx) const
{
   return mKnots[idx];
}

template <unsigned int D>
long NURBS<D>::getKnots(typename NURBS<D>::KnotArray &knots) const
{
   knots = mKnots;
   return long(mKnots.size());
}

template <unsigned int D>
void NURBS<D>::setKnot(long idx, float k)
{
   mKnots[idx] = k;
   mDirty = true; // have to re-check knot sequence
}

template <unsigned int D>
void NURBS<D>::setKnots(const typename NURBS<D>::WeightArray &knots)
{
   mKnots = knots;
   mDirty = true; // even if size hasn't changed have to re-check knot sequence
}

template <unsigned int D>
long NURBS<D>::resizeKnots()
{
   long n = getNumCVs() + mDegree + 1;
   if (n != getNumKnots())
   {
      mDirty = true;
      mKnots.resize(n);
   }
   return n;
}

template <unsigned int D>
bool NURBS<D>::getDomain(float &start, float &end) const
{
   if (getNumKnots() == 0)
   {
      return false;
   }
   start = mKnots.front();
   end = mKnots.back();
   return true;
}

template <unsigned int D>
bool NURBS<D>::isInDomain(float u) const
{
   float start = 0.0f, end = 0.0f;
   if (!getDomain(start, end))
   {
      return false;
   }
   return (start <= u && u <= end);
}

template <unsigned int D>
bool NURBS<D>::findKnotSpan(float u, long &k) const
{
   // if u == last knot value and we have a clamped curve
   // we'd like k to point at the first value
   if (!isInDomain(u))
   {
      return false;
   }

   long low = 0;
   long high = long(mKnots.size()) - 1;
   long mid = low + (high - low) / 2;

   while (mid != low)
   {
      float val = mKnots[mid];
      if (u < val)
      {
         high = mid;
      }
      //else
      else if (u > val)
      {
         low = mid;
      }
      else
      {
         break;
      }
      mid = low + (high - low) / 2;
   }

   k = mid;

   while ((k + 1) < getNumKnots() && u >= mKnots[k+1])
   {
      ++k;
   }

   return true;
}

template <unsigned int D>
long NURBS<D>::getKnotMultiplicity(float u, long span) const
{
   if (span < 0)
   {
      if (!findKnotSpan(u, span))
      {
         return 0;
      }
   }

   long m = 0;
   long l = span;
   
   while (l >= 0)
   {
      if (u - mKnots[l] < 0.000001f)
      {
         m += 1;
         l -= 1;
      }
      else
      {
         break;
      }
   }
   
   return m;
}

template <unsigned int D>
long NURBS<D>::getKnotMultiplicity(long k) const
{
   if (k >= mKnots.size())
   {
      return 0;
   }
 
   float u = mKnots[k];
   long m = 1;

   long l = k + 1;
 
   while (l < getNumKnots())
   {
      if ((mKnots[l] - u) < 0.000001f)
      {
         m += 1;
         l += 1;
      }
      else
      {
         break;
      }
   }

   l = k - 1;

   while (l >= 0)
   {
      if ((u - mKnots[l]) < 0.000001)
      {
         m += 1;
         l -= 1;
      }
      else
      {
         break;
      }
   }

   return m;
}

template <unsigned int D>
void NURBS<D>::buildKnotsUniform(bool clamp, bool normalize)
{
   if (!hasValidCVsAndKnotsCount())
   {
      resizeKnots();
      
      if (!hasValidCVsAndKnotsCount())
      {
         return;
      }
   }
 
   long nk = long(mKnots.size());
 
   if (clamp)
   {
      if (normalize)
      {
         for (long i=0; i<=mDegree; ++i)
         {
            mKnots[i] = 0.0f;
            mKnots[nk - 1 - i] = 1.0f;
         }
         
         float step = 1.0f / (nk - 2 * (mDegree + 1) + 1);
         float v = step;
         
         for (long i=mDegree+1; i<nk-mDegree-1; ++i)
         {
            mKnots[i] = v;
            v += step;
         }
      }
      else
      {
         for (long i=0; i<=mDegree; ++i)
         {
            mKnots[i] = 0.0f;
         }
         
         float v = 1.0f;
         
         for (long i=mDegree+1; i<nk-mDegree-1; ++i)
         {
            mKnots[i] = v;
            v += 1.0f;
         }
         
         for (long i=nk-mDegree-1; i<nk; ++i)
         {
            mKnots[i] = v;
         }
      }
   }
   else
   {
      float v = 0.0f;
      float step = (normalize ? 1.0f / (nk - 1) : 1.0f);
      
      for (long i=0; i<nk; ++i)
      {
         mKnots[i] = v;
         v += step;
      }
   }
}

template <unsigned int D>
void NURBS<D>::buildKnotsChordLength()
{
   buildKnotsCentripetal(1.0f);
}

template <unsigned int D>
float NURBS<D>::dotCV(const typename NURBS<D>::CV &cv0, const typename NURBS<D>::CV &cv1) const
{
   float rv = 0.0f;
   for (unsigned int i=0; i<D; ++i)
   {
      rv += cv0[i] * cv1[i];
   }
   return rv;
}

template <unsigned int D>
float NURBS<D>::lenCV(const typename NURBS<D>::CV &cv) const
{
   return Sqrt(dotCV(cv, cv));
}

template <unsigned int D>
void NURBS<D>::buildKnotsCentripetal(float strength)
{
   // resulting curve is clamped and knots vector normalized
   if (!hasValidCVsAndKnotsCount())
   {
      resizeKnots();
      
      if (!hasValidCVsAndKnotsCount())
      {
         return;
      }
   }

   // first build parameter vector
   long n = getNumCVs();
   float L = 0.0f;
   float l = 0.0f;
   float v = 0.0f;
   float invDegree = 1.0f / float(mDegree);
   KnotArray params(n, 0.0f);

   for (long i=1; i<n; ++i)
   {
      l = powf(lenCV(mCVs[i] - mCVs[i-1]), strength);
      params[i] = params[i-1] + l;
      L += l;
   }
 
   L = (L > 0.0f ? 1.0f/L : 1.0f);

   for (size_t i=0; i<params.size(); ++i)
   {
      params[i] *= L;
   }
 
   long endOff = getNumKnots() - mDegree - 1;

   for (long i=0; i<=mDegree; ++i)
   {
      mKnots[i] = 0.0f;
      mKnots[endOff+i] = 1.0f;
   }
 
   for (long i=mDegree+1; i<endOff; ++i)
   {
      v = 0.0f;
      for (long j=i; j<i+mDegree; ++j)
      {
         v += params[j-mDegree];
      }
      mKnots[i] = v * invDegree;
   }
}

template <unsigned int D>
bool NURBS<D>::insertKnot(float u, long n)
{
   // will this work properly if u not in domain
   //                         for a open knots vector
   long k, m, iter = 1;

   float ai;

   CVArray newCVs(mDegree);

   while (n > 0)
   {
      if (!findKnotSpan(u, k))
      {
         return false;
      }

      m = long(getKnotMultiplicity(u, k));

      if (m >= mDegree)
      {
         return true;
      }

      long off = k - mDegree + 1;

      for (long i=off, j=0; i<=k; ++i, ++j)
      {
         ai = mKnots[i+mDegree] - mKnots[i];
         ai = (ai < 0.000001f ? 0.0f : (u - mKnots[i]) / ai);
         newCVs[j] = (1.0f - ai) * mCVs[i-1] + ai * mCVs[i];
      }

      mKnots.insert(mKnots.begin()+k+1, u);
      mCVs.erase(mCVs.begin()+off, mCVs.begin()+k);
      mCVs.insert(mCVs.begin()+off, newCVs.begin(), newCVs.end());

      --n;
      ++iter;
   }

   return true;
}

template <unsigned int D>
long NURBS<D>::computeBasis(float u, float *basis) const
{
   if (!isValid())
   {
      return -1;
   }

   // degree + 1 basis coeffs
   // applies to CVs starting at returned index
   // return negative index for errors
   long k = 0;

   // this takes care of cases where first and last knots are not
   // of multiplicity degree+1 (open knot vector)
   if (u < mKnots[mDegree])
   {
      u = mKnots[mDegree];
   } 
   else if (u > mKnots[mKnots.size()-mDegree-1])
   {
      u = mKnots[mKnots.size()-mDegree-1];
   }

   if (!findKnotSpan(u, k))
   {
      return -1;
   }
 
   // Handles u == last knot value case (findKnotSpan will return index of last knot)
   if (k >= long(mCVs.size()))
   {
      // problem arise if u equals the last knot value...
      k = long(mCVs.size()) - 1;
   }


   for (long i=0; i<=mDegree; ++i)
   {
      basis[i] = 0.0f;
   }
   basis[mDegree] = 1.0f;

   long p = k - mDegree;

   float left, right;

   // Ni,n(u) = ((u - k[i]) / (k[i+n] - k[i]))         * Ni,n-1 + 
   //           ((k[i+n+1] - u) / (k[i+n+1] - k[i+1])) * Ni+1,n-1 

   // at this point, Nk,0 = 1.0 and any other Ni,0 = 0.0

   for (long d=1; d<=mDegree; ++d)
   {
      // only right term with i = k-d, n = d
      right = mKnots[k+1] - mKnots[k-d+1];
      right = (right < 0.000001f ? 0.0f  : (mKnots[k+1] - u) / right);
      basis[mDegree-d] = right * basis[mDegree-d+1];

      // left & right term with i = j, n = d
      for (long j=k-d+1, l=mDegree-d+1; j<k; ++j, ++l)
      {
         left = mKnots[j+d] - mKnots[j];
         left = (left < 0.000001f ? 0.0f : (u - mKnots[j]) / left);

         right = mKnots[j+d+1] - mKnots[j+1];
         right = (right < 0.000001f ? 0.0f  : (mKnots[j+d+1] - u) / right);

         basis[l] = left * basis[l] + right * basis[l+1];
      }

      // only left term with i = k and n = d
      left = mKnots[k+d] - mKnots[k];
      left = (left < 0.000001f ? 0.0f : (u - mKnots[k]) / left);
      basis[mDegree] = left * basis[mDegree];
   }

   return p;
}

template <unsigned int D>
bool NURBS<D>::eval(float u, typename NURBS<D>::Pnt &r) const
{
   float *coeffs = (float*) alloca((mDegree+1) * sizeof(float));

   long idx = computeBasis(u, coeffs);

   if (idx < 0)
   {
      return false;
   }

   CV rv, tmp;

   rv.zero();

   for (long i=0; i<=mDegree; ++i)
   {
      tmp = mCVs[idx+i];
      for (unsigned int j=0; j<D; ++j)
      {
         tmp[j] *= tmp[D];
      }
      rv += coeffs[i] * tmp;
   }

   if (fabs(rv[D]) > 0.000001f)
   {
      r = rv;
      r /= rv[D];
   }
   else
   {
      r.zero();
   }

   return true;
}

template <unsigned int D>
bool NURBS<D>::derivate(NURBS<D> &d) const
{
   // doesn't match maya for open knot vector curve...
   if (!isValid() || mDegree == 0)
   {
      return false;
   }

   d = NURBS(mDegree - 1);
   d.setNumCVs(mCVs.size() - 1);
   d.resizeKnots();

   long n = getNumCVs() - 1;

   float tmp;
   CV q;

   for (long i=0; i<n; ++i)
   {
      tmp = mKnots[i+mDegree+1] - mKnots[i+1];
      tmp = (tmp < 0.000001f ? 0.0f : float(mDegree) / tmp);

      q = tmp * (mCVs[i+1] - mCVs[i]);
      // if weights are both one resulting weight is zero...
      // averaging weights seems to be the best (seeing derivative results)
      q[D] = 0.5f * (mCVs[i+1][D] + mCVs[i][D]);

      d.setCV(i, q);
   }

   for (long i=0, j=1; j<(getNumKnots() - 1); ++i, ++j)
   {
      d.setKnot(i, mKnots[j]);
   }

   return true;
}

template <unsigned int D>
bool NURBS<D>::evalDerivative(float u, typename NURBS<D>::Vec &r) const
{
   NURBS<D> d;
   if (derivate(d))
   {
      return d.eval(u, r);
   }
   return false;
}

template <unsigned int D>
float NURBS<D>::getLength(float precision) const
{
   float from = 0.0f, to = 0.0f;
   if (!getDomain(from, to))
   {
      return -1.0f;
   }
   return getLengthBetween(from, to, precision);
}

template <unsigned int D>
float NURBS<D>::getLengthTo(float to, float precision) const
{
   float from = 0.0f, dummy = 0.0f;
   if (!getDomain(from, dummy))
   {
      return -1.0f;
   }
   return getLengthBetween(from, to, precision);
}

template <unsigned int D>
float NURBS<D>::getLengthFrom(float from, float precision) const
{
   float dummy = 0.0f, to = 0.0f;
   if (!getDomain(dummy, to))
   {
      return -1.0f;
   }
   return getLengthBetween(from, to, precision);
}

template <unsigned int D>
float NURBS<D>::getLengthBetween(float from, float to, float precision) const
{
   if (mKnots.size() == 0)
   {
      return -1.0f;
   }

   if (from < mKnots.front())
   {
      from = mKnots.front();
   }

   if (to > mKnots.back())
   {
      to = mKnots.back();
   }

   if (from >= to)
   {
      return 0.0f;
   }

   long nseg = 1;

   Pnt Pfrom, Pto;

   if (!eval(from, Pfrom) || !eval(to, Pto))
   {
      throw std::runtime_error("Could not eval end points");
      return 0.0f;
   }

   float L = (Pto - Pfrom).getLength();
   float lastL = L + 10.0f * precision;
   float u, du;

   while (fabs(L - lastL) > precision)
   {
      lastL = L;
      L = 0.0f;
      nseg *= 2;
      du = (to - from) / nseg;
      u = from;
      
      for (long j=0; j<nseg; ++j)
      {
         if (!eval(u, Pfrom) || !eval(u+du, Pto))
         {
            throw std::runtime_error("Could not eval");
            return 0.0f;
         }
         
         L += (Pto - Pfrom).getLength();
         u += du;
      }
   }

   return L;
}

template <unsigned int D>
float NURBS<D>::getParamAtLength(float l, float precision)
{
   if (mKnots.size() == 0)
   {
      return -1.0f;
   }

   if (l <= 0.0f)
   {
      return mKnots.front();
   }

   float from = mKnots.front();
   float u = mKnots.back();
   float maxU = u;

   float L = getLengthBetween(from, u, precision);

   if (l >= L)
   {
      return u;
   }

   while (fabs(l-L) > precision && u > from)
   {
      if (l < L)
      {
         maxU = u;
         u = from + 0.5f * (u - from);
      }
      else
      {
         u = from + 0.5f * (u + maxU);
      }
      L = getLengthBetween(from, u, precision);
   }

   return u;
}

template <unsigned int D>
bool NURBS<D>::getTangent(float u, typename NURBS<D>::Vec &t, bool normalize)
{
   if (!evalDerivative(u, t))
   {
      return false;
   }
   if (normalize)
   {
      t.normalize();
   }
   return true;
}

template <unsigned int D>
bool NURBS<D>::getNormal(float u, typename NURBS<D>::Vec &n, bool normalize)
{
   NURBS<D> d;
   
   if (derivate(d))
   {
      Pnt t;

      if (!d.eval(u, t))
      {
         return false;
      }

      t.normalize();

      if (!d.evalDerivative(u, n))
      {
         return false;
      }

      n -= n.dot(t) * t;

      if (normalize)
      {
         n.normalize();
      }

      return true;
   }
   
   return false;
}

template <unsigned int D>
bool NURBS<D>::subdivide(float u, NURBS<D> &before, NURBS<D> &after)
{
   // NOTE: if weights are not all 1
   //       the shape is not preserved...
   CVArray oldCVs = mCVs;
   KnotArray oldKnots = mKnots;

   if (!insertKnot(u, mDegree))
   {
      std::swap(mCVs, oldCVs);
      std::swap(mKnots, oldKnots);
      return false;
   }

   long k = -1, ncv, nk;

   if (!findKnotSpan(u, k))
   {
      std::swap(mCVs, oldCVs);
      std::swap(mKnots, oldKnots);
      return false;
   }

   // k can be last knot index, beware here (this has changed compare to py version)

   if (k <= mDegree || k >= (getNumKnots() - mDegree - 1))
   {
      std::swap(mCVs, oldCVs);
      std::swap(mKnots, oldKnots);
      return false;
   }

   // split
   // cvs   : [0, k-mDegree], [k-mDegree, numCVs)
   // knots : [0, k], [k, numKnots)

   before = NURBS<D>(mDegree);
   ncv = k - mDegree + 1;
   before.setNumCVs(ncv);
   nk = before.resizeKnots();
   
   for (long i=0; i<=k-mDegree; ++i)
   {
      before.setCV(i, mCVs[i]);
   }
   
   for (long i=0; i<=k; ++i)
   {
      before.setKnot(i, mKnots[i]);
   }
   
   for (long i=k+1; i<nk; ++i)
   {
      before.setKnot(i, before.getKnot(i-1));
   }

   after = NURBS<D>(mDegree);
   ncv = getNumCVs() - (k - mDegree);
   after.setNumCVs(ncv);
   nk = after.resizeKnots();
   
   for (long i=k-mDegree, j=0; i<getNumCVs(); ++i, ++j)
   {
      after.setCV(j, mCVs[i]);
   }
   
   long j = after.getNumKnots() - 1;
   
   for (long i=getNumKnots()-1; i>=k; --i, --j)
   {
      after.setKnot(j, mKnots[i]);
   }
   
   while (j >= 0)
   {
      after.setKnot(j, after.getKnot(j+1));
      --j;
   }

   std::swap(mCVs, oldCVs);
   std::swap(mKnots, oldKnots);

   return true;
}

template <unsigned int D>
bool NURBS<D>::trimAfter(float u)
{
   NURBS<D> before, after;
   if (subdivide(u, before, after))
   {
      *this = before;
      return true;
   }
   return false;
}

template <unsigned int D>
bool NURBS<D>::trimBefore(float u)
{
   NURBS<D> before, after;
   if (subdivide(u, before, after))
   {
      *this = after;
      return true;
   }
   return false;
}

template <unsigned int D>
bool NURBS<D>::trim(float from, float to)
{
   NURBS<D> b0, b1, a0, a1;
   if (subdivide(from, b0, a0))
   {
      if (a0.subdivide(to, b1, a1))
      {
         *this = b1;
         return true;
      }
   }
   return false;
}

template <unsigned int D>
void NURBS<D>::convertToBezier(bool normalize)
{
   mDegree = long(mCVs.size()) - 1;
   buildKnotsUniform(true, normalize);
}

#endif
