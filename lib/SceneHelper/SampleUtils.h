#ifndef SCENEHELPER_SAMPLEUTILS_H_
#define SCENEHELPER_SAMPLEUTILS_H_

#include "LocatorProperty.h"
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcGeom/All.h>
#include <limits>
#include <list>
#include <map>
#include <set>

template <class T>
struct ArrayDeleterT
{
   typedef Alembic::AbcCoreAbstract::ArraySample ArraySample;

   void operator()(void* memory) const
   {
     ArraySample *arraySample = static_cast<ArraySample*>(memory);
     if (arraySample)
     {
         T *data = reinterpret_cast<T*>(const_cast<void*>(arraySample->getData()));
         if (data)
         {
            delete[] data;
         }
         delete arraySample;
     }
   }
};

template <class BaseSample>
class ScaledPVBSample : public BaseSample
{
public:
   // PVB: (P)ositions, (V)elocities, (B)ounds
   typedef Alembic::Abc::P3fArraySample PntSample;
   typedef Alembic::Abc::P3fArraySamplePtr PntSamplePtr;

   typedef Alembic::Abc::V3fArraySample VecSample;
   typedef Alembic::Abc::V3fArraySamplePtr VecSamplePtr;

   typedef ArrayDeleterT<Alembic::Abc::V3f> Deleter;

   typedef ScaledPVBSample this_type;

   ScaledPVBSample()
      : BaseSample()
      , m_rawScaledPositions(0)
      , m_rawScaledVelocities(0)
      , m_scale(1.0f)
   {
   }
   
   PntSamplePtr getPositions() const { return (m_rawScaledPositions ? m_scaledPositions : BaseSample::getPositions()); }
   VecSamplePtr getVelocities() const { return (m_rawScaledVelocities ? m_scaledVelocities : BaseSample::getVelocities()); }
   Alembic::Abc::Box3d getSelfBounds() const { return (m_rawScaledPositions ? m_scaledBounds : BaseSample::getSelfBounds()); }
   
   void reset()
   {
      BaseSample::reset();
      m_scaledPositions.reset();
      m_rawScaledPositions = 0;
      m_scaledVelocities.reset();
      m_rawScaledVelocities = 0;
      m_scaledBounds.makeEmpty();
   }

   float scale() const
   {
      return m_scale;
   }

   void scale(float scl)
   {
      m_scale = scl;

      if (BaseSample::valid() && fabs(1.0f - m_scale) > 0.000001f)
      {
         PntSamplePtr positions = BaseSample::getPositions();
         VecSamplePtr velocities = BaseSample::getVelocities();
         Alembic::Abc::Box3d bounds = BaseSample::getSelfBounds();
         
         size_t count = positions->size();

         if (!m_scaledPositions)
         {
            m_rawScaledPositions = new Alembic::Abc::V3f[count];
            m_scaledPositions = PntSamplePtr(new PntSample(m_rawScaledPositions, count), Deleter());
         }

         for (size_t i=0; i<count; ++i)
         {
            m_rawScaledPositions[i] = scl * (*positions)[i];
         }

         m_scaledBounds = Alembic::Abc::Box3d(double(scl) * bounds.min, double(scl) * bounds.max);

         if (!velocities)
         {
            m_scaledVelocities.reset();
            m_rawScaledVelocities = 0;
         }
         else
         {
            count = velocities->size();

            if (!m_rawScaledVelocities)
            {
               m_rawScaledVelocities = new Alembic::Abc::V3f[count];
               m_scaledVelocities = VecSamplePtr(new VecSample(m_rawScaledVelocities, count), Deleter());
            }

            for (size_t i=0; i<count; ++i)
            {
               m_rawScaledVelocities[i] = scl * (*velocities)[i];
            }
         }
         
         // Note: Keep original positions in order to be able to change scale
         //       => May want to control that to avoid memory impact
      }
      else
      {
         m_scaledPositions.reset();
         m_rawScaledPositions = 0;
         m_scaledPositions.reset();
         m_rawScaledVelocities = 0;
         m_scaledBounds.makeEmpty();
      }
   }

protected:
   PntSamplePtr m_scaledPositions;
   Alembic::Abc::V3f *m_rawScaledPositions;
   VecSamplePtr m_scaledVelocities;
   Alembic::Abc::V3f *m_rawScaledVelocities;
   Alembic::Abc::Box3d m_scaledBounds;
   float m_scale;
};

template <class Traits>
class ScaledScalarSample
{
public:
   typedef typename Traits::value_type ValueType;

   ScaledScalarSample() {}

   float scale() const { return 1.0f; }

   void scale(float scl) {}

   ValueType& rawValue() { return m_value; }

   operator const ValueType& () const { return m_value; }

protected:
   ValueType m_value;
};

template <> class ScaledScalarSample<LocatorTraits>
{
public:
   typedef LocatorTraits::value_type ValueType;

   ScaledScalarSample() : m_scale(1.0f) {}

   float scale() const { return m_scale; }
   void scale(float scl)
   {
      m_scale = scl;
   }

   ValueType& rawValue() { return m_value; }

   operator ValueType () const
   {
      ValueType v = m_value;
      v.T[0] *= m_scale; v.T[1] *= m_scale; v.T[2] *= m_scale;
      v.S[0] *= m_scale; v.S[1] *= m_scale; v.S[2] *= m_scale;
      return v;
   }

protected:
   ValueType m_value;
   float m_scale;
};

template <> class ScaledScalarSample<Alembic::Abc::Box3dTPTraits>
{
public:
   typedef Alembic::Abc::Box3dTPTraits::value_type ValueType;

   ScaledScalarSample() : m_scale(1.0f) {}

   float scale() const { return m_scale; }
   void scale(float scl)
   {
      m_scale = scl;
   }

   ValueType& rawValue() { return m_value; }

   operator ValueType () const
   {
      return ValueType(double(m_scale) * m_value.min, double(m_scale) * m_value.max);
   }

protected:
   ValueType m_value;
   float m_scale;
};

// ---

// Adds scaling ability to base sample
template <class IObject>
struct SampleOverride
{
   typedef typename IObject::Sample BaseSample;

   class Sample : public BaseSample
   {
   public:
      Sample() : BaseSample() {}
      float scale() const { return 1.0f; }
      void scale(float) {}
   };

   typedef typename Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <> struct SampleOverride<Alembic::AbcGeom::IPolyMeshSchema>
{
   typedef Alembic::AbcGeom::IPolyMeshSchema::Sample BaseSample;
   typedef ScaledPVBSample<BaseSample> Sample;
   typedef Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <> struct SampleOverride<Alembic::AbcGeom::ISubDSchema>
{
   typedef Alembic::AbcGeom::ISubDSchema::Sample BaseSample;
   typedef ScaledPVBSample<BaseSample> Sample;
   typedef Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <> struct SampleOverride<Alembic::AbcGeom::IPointsSchema>
{
   typedef Alembic::AbcGeom::IPointsSchema::Sample BaseSample;
   typedef ScaledPVBSample<BaseSample> Sample;
   typedef Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <> struct SampleOverride<Alembic::AbcGeom::ICurvesSchema>
{
   typedef Alembic::AbcGeom::ICurvesSchema::Sample BaseSample;
   typedef ScaledPVBSample<BaseSample> Sample;
   typedef Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <> struct SampleOverride<Alembic::AbcGeom::INuPatchSchema>
{
   typedef Alembic::AbcGeom::INuPatchSchema::Sample BaseSample;
   typedef ScaledPVBSample<BaseSample> Sample;
   typedef Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <> struct SampleOverride<Alembic::AbcGeom::IXformSchema>
{
   typedef Alembic::AbcGeom::XformSample BaseSample;

   class Sample : public BaseSample
   {
   public:
      Sample()
         : XformSample()
         , m_scale(1.0)
      {
      }

      float scale() const
      {
         return float(m_scale);
      }

      void scale(float scl)
      {
         m_scale = double(scl);
      }

      // --- Only overrides getters for now

      Alembic::AbcGeom::XformOp getOp(const size_t idx) const
      {
         Alembic::AbcGeom::XformOp op = XformSample::getOp(idx);
         if (op.isTranslateOp())
         {
            op.setTranslate(m_scale * op.getTranslate());
         }
         return op;
      }

      Alembic::Abc::V3d getTranslation() const
      {
         return m_scale * XformSample::getTranslation();
      }

      Alembic::Abc::M44d getMatrix() const
      {
         Alembic::Abc::M44d m = XformSample::getMatrix();
         m.x[3][0] *= m_scale;
         m.x[3][1] *= m_scale;
         m.x[3][2] *= m_scale;
         return m;
      }

   protected:
      double m_scale;
      double m_invScale;
   };

   typedef Alembic::Util::shared_ptr<Sample> SamplePtr;
};

template <class Traits>
struct SampleOverride<Alembic::AbcGeom::ITypedScalarProperty<Traits> >
{
   typedef ScaledScalarSample<Traits> Sample;
   typedef typename Alembic::Util::shared_ptr<Sample> SamplePtr;
};

// ITypedArrayProperty?
// ITypedGeomParam?

// ---

template <class IObject>
struct SampleUtils
{
   typedef IObject AlembicType;
   typedef typename SampleOverride<IObject>::Sample SampleType;
   
   static bool IsValid(const SampleType &samp) { return samp.valid(); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, float scale, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
      out.scale(scale);
   }

   static void Reset(SampleType &samp) { samp.reset(); }

   static float GetScale(const SampleType &samp) { return samp.scale(); }

   static void SetScale(SampleType &samp, float scl) { samp.scale(scl); }
};

template<> struct SampleUtils<Alembic::AbcGeom::IXformSchema>
{
   typedef Alembic::AbcGeom::IXformSchema AlembicType;
   typedef SampleOverride<Alembic::AbcGeom::IXformSchema>::Sample SampleType;
   
   static bool IsValid(const SampleType &samp) { return (samp.getNumOps() > 0); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, float scale, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
      out.scale(scale);
   }
   
   static void Reset(SampleType &samp) { samp.reset(); }

   static float GetScale(const SampleType &samp) { return samp.scale(); }

   static void SetScale(SampleType &samp, float scl) { samp.scale(scl); }
};

template <class Traits>
struct SampleUtils<Alembic::AbcGeom::ITypedScalarProperty<Traits> >
{
   typedef Alembic::AbcGeom::ITypedScalarProperty<Traits> AlembicType;
   typedef typename SampleOverride<AlembicType>::Sample SampleType;
   
   static bool IsValid(const SampleType &) { return true; }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, float scale, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out.rawValue(), selector);
      out.scale(scale);
   }
   
   static void Reset(SampleType &) { }

   static float GetScale(const SampleType &samp) { return samp.scale(); } //return 1.0f; }

   static void SetScale(SampleType &samp, float s) { samp.scale(s); }
};

template <class Traits>
struct SampleUtils<Alembic::AbcGeom::ITypedArrayProperty<Traits> >
{
   typedef Alembic::AbcGeom::ITypedArrayProperty<Traits> AlembicType;
   typedef typename AlembicType::sample_ptr_type SampleType;
   
   static bool IsValid(const SampleType &samp) { return samp->valid(); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, float scale, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
   }
   
   static void Reset(SampleType &samp) { samp->reset(); }

   static float GetScale(const SampleType &samp) { return 1.0f; }

   static void SetScale(SampleType &, float) { }
};

template <class Traits>
struct SampleUtils<Alembic::AbcGeom::ITypedGeomParam<Traits> >
{
   typedef Alembic::AbcGeom::ITypedGeomParam<Traits> AlembicType;
   typedef typename SampleOverride<AlembicType>::Sample SampleType;
   
   static bool IsValid(const SampleType &samp) { return samp.valid(); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, float scale, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      if (owner.getScope() == Alembic::AbcGeom::kFacevaryingScope)
      {
         owner.getIndexed(out, selector);
      }
      else
      {
         owner.getExpanded(out, selector);
      }
      out.scale(scale);
   }
   
   static void Reset(SampleType &samp) { samp.reset(); }

   static float GetScale(const SampleType &samp) { return samp.scale(); }

   static void SetScale(SampleType &samp, float scl) { samp.scale(scl); }
};

// ---

template <class IObject>
class TimeSample
{
public:
   
   typedef typename SampleUtils<IObject>::SampleType SampleType;
   
public:
   
   TimeSample()
      : mTime(std::numeric_limits<double>::max())
      , mIndex(-1)
   {
   }
   
   ~TimeSample()
   {
      reset();
   }
   
   bool valid() const
   {
      return (mIndex >= 0 && SampleUtils<IObject>::IsValid(mData));
   }
   
   void reset()
   {
      SampleUtils<IObject>::Reset(mData);
      mIndex = -1;
      mTime = std::numeric_limits<double>::max();
   }
   
   bool get(const IObject &owner, Alembic::AbcCoreAbstract::index_t i, float scale, double *t=0)
   {
      bool isConstant = SampleUtils<IObject>::IsConstant(owner);
      
      if (valid() && (isConstant || mIndex == i))
      {
         return true;
      }
      else
      {
         if (owner.getNumSamples() == 0)
         {
            reset();
         }
         else
         {
            if (isConstant)
            {
               mIndex = 0;
               mTime = 0.0;
               SampleUtils<IObject>::Get(owner, scale, mData);
            }
            else
            {
               if (i >= 0 && size_t(i) < owner.getNumSamples())
               {
                  mIndex = i;
                  mTime = (t ? *t : owner.getTimeSampling()->getSampleTime(i));
                  SampleUtils<IObject>::Get(owner, scale, mData, sampleSelector());
               }
               else
               {
                  reset();
               }
            }
         }
         return valid();
      }
   }
   
   bool get(const IObject &owner, double t, float scale, Alembic::AbcCoreAbstract::index_t *index=0)
   {
      bool isConstant = SampleUtils<IObject>::IsConstant(owner);
      
      if (valid() && (isConstant || fabs(mTime - t) <= 0.0001))
      {
         return true;
      }
      else
      {
         if (owner.getNumSamples() == 0)
         {
            reset();
            return false;
         }
         else
         {
            if (isConstant)
            {
               mIndex = 0;
               mTime = 0.0;
               SampleUtils<IObject>::Get(owner, scale, mData);
            }
            else
            {
               if (index)
               {
                  mIndex = *index;
                  mTime = t;
                  SampleUtils<IObject>::Get(owner, scale, mData, sampleSelector());
               }
               else
               {
                  // index not provided, figure it out
                  std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime = owner.getTimeSampling().getNearIndex(t);
                  
                  if (fabs(indexTime.second - t) > 0.0001)
                  {
                     reset();
                  }
                  else
                  {
                     mIndex = indexTime.first;
                     mTime = indexTime.second;
                     SampleUtils<IObject>::Get(owner, scale, mData, sampleSelector());
                  }
               }
            }
         }
         return valid();
      }
   }

   float scale() const
   {
      return SampleUtils<IObject>::GetScale(mData);
   }

   void scale(float scale)
   {
      return SampleUtils<IObject>::SetScale(mData, scale);
   }
   
   inline double time() const
   {
      return mTime;
   }
   
   inline Alembic::AbcCoreAbstract::index_t index() const
   {
      return mIndex;
   }
   
   inline const SampleType& data() const
   {
      return mData;
   }
   
private:
   
   inline Alembic::Abc::ISampleSelector sampleSelector() const
   {
      return Alembic::Abc::ISampleSelector(mIndex);
   }
   
private:
   
   double mTime;
   Alembic::AbcCoreAbstract::index_t mIndex;
   SampleType mData;
};

template <typename IObject>
class TimeSampleList
{
public:
   
   typedef TimeSample<IObject> TimeSampleType;
   typedef typename TimeSampleType::SampleType SampleType;
   
   typedef typename std::list<TimeSampleType> List;
   typedef typename List::const_iterator ListConstIt;
   typedef typename List::iterator ListIt;
   
   typedef typename std::map<Alembic::AbcCoreAbstract::index_t, ListIt> Map;
   typedef typename Map::const_iterator MapConstIt;
   typedef typename Map::iterator MapIt;
   
   class Iterator
   {
   public:
      
      inline Iterator(MapIt it=MapIt())
         : _it(it)
      {
      }
      
      inline Iterator(const Iterator &rhs)
         : _it(rhs._it)
      {
      }
      
      inline Iterator& operator=(const MapIt &rhs)
      {
         _it = rhs._it;
         return *this;
      }
      
      inline Iterator& operator=(const Iterator &rhs)
      {
         _it = rhs._it;
         return *this;
      }
      
      inline bool operator==(const Iterator &rhs) const
      {
         return (_it == rhs._it);
      }
      
      inline bool operator!=(const Iterator &rhs) const
      {
         return (_it != rhs._it);
      }
      
      inline Iterator& operator++()
      {
         ++_it;
         return *this;
      }
      
      inline Iterator operator++(int)
      {
         Iterator tmp(*this);
         ++_it;
         return tmp;
      }
      
      inline Iterator& operator--()
      {
         --_it;
         return *this;
      }
      
      inline Iterator operator--(int)
      {
         Iterator tmp(*this);
         --_it;
         return tmp;
      }
      
      inline TimeSampleType& operator*()
      {
         return _it->second.operator*();
      }
      
      inline TimeSampleType* operator->()
      {
         return _it->second.operator->();
      }
      
      inline operator MapConstIt () const
      {
         return MapConstIt(_it);
      }
      
   private:
      
      MapIt _it;
   };
   
   // basically a MapConstIt with * and -> overloaded
   class ConstIterator
   {
   public:
      inline ConstIterator(MapConstIt it=MapConstIt())
         : _it(it)
      {
      }
      
      inline ConstIterator(const ConstIterator &rhs)
         : _it(rhs._it)
      {
      }
      
      inline ConstIterator& operator=(const MapConstIt &it)
      {
         _it = it;
         return *this;
      }
      
      inline ConstIterator& operator=(const ConstIterator &rhs)
      {
         _it = rhs._it;
         return *this;
      }
      
      inline bool operator==(const ConstIterator &rhs) const
      {
         return (_it == rhs._it);
      }
      
      inline bool operator==(const MapConstIt &it) const
      {
         return (_it == it);
      }
      
      inline bool operator!=(const ConstIterator &rhs) const
      {
         return (_it != rhs._it);
      }
      
      inline bool operator!=(const MapConstIt &it) const
      {
         return (_it != it);
      }
      
      inline ConstIterator& operator++()
      {
         ++_it;
         return *this;
      }
      
      inline ConstIterator operator++(int)
      {
         ConstIterator tmp(*this);
         ++_it;
         return tmp;
      }
      
      inline ConstIterator& operator--()
      {
         --_it;
         return *this;
      }
      
      inline ConstIterator operator--(int)
      {
         ConstIterator tmp(*this);
         --_it;
         return tmp;
      }
      
      inline const TimeSampleType& operator*()
      {
         return _it->second.operator*();
      }
      
      inline const TimeSampleType* operator->()
      {
         return _it->second.operator->();
      }
      
   private:
      
      MapConstIt _it;
   };
      
public:
   
   // Here for convenience
   double lastEvaluationTime;
   
   TimeSampleList()
      : lastEvaluationTime(std::numeric_limits<double>::max())
   {
   }
   
   TimeSampleList(const TimeSampleList<IObject> &rhs)
      : lastEvaluationTime(std::numeric_limits<double>::max())
      , mSamples(rhs.mSamples)
      , mIndices(rhs.mIndices)
   {
   }
   
   TimeSampleList<IObject>& operator=(const TimeSampleList<IObject> &rhs)
   {
      if (this != &rhs)
      {
         lastEvaluationTime = std::numeric_limits<double>::max();
         mSamples = rhs.mSamples;
         mIndices = rhs.mIndices;
      }
      return *this;
   }
   
   ~TimeSampleList()
   {
      clear();
   }
   
   void clear()
   {
      for (ListIt it = mSamples.begin(); it != mSamples.end(); ++it)
      {
         it->reset();
      }
      
      mSamples.clear();
      mIndices.clear();
   }
   
   inline bool empty() const
   {
      return (mSamples.size() == 0);
   }
   
   inline size_t size() const
   {
      return mSamples.size();
   }
   
   inline Iterator begin()
   {
      return Iterator(mIndices.begin());
   }
   
   inline Iterator end()
   {
      return Iterator(mIndices.end());
   }
   
   // which = -2 -> prev sample
   // which = -1 -> exact or prev sample
   // which =  0 -> exact sample
   // which =  1 -> exact or next sample
   // which =  2 -> next sample
   inline Iterator find(double t, int which=0)
   {
      return find(t, which, mIndices.begin(), mIndices.end());
   }
   
   inline ConstIterator begin() const
   {
      return ConstIterator(mIndices.begin());
   }
   
   inline ConstIterator end() const
   {
      return ConstIterator(mIndices.end());
   }
   
   inline ConstIterator find(double t, int which=0) const
   {
      return find(t, which, mIndices.begin(), mIndices.end());
   }
   
   bool getTimeRange(double &tmin, double &tmax) const
   {
      if (!empty())
      {
         MapConstIt it;
         
         it = mIndices.begin();
         tmin = it->second->time();
         
         it = mIndices.end();
         --it;
         tmax = it->second->time();
         
         return true;
      }
      else
      {
         return false;
      }
   }
   
   bool validRange(double t0, double t1) const
   {
      ConstIterator firstIt = find(t0, -1);
      ConstIterator endIt = find(t1, 1);
      
      if (firstIt == end())
      {
         if (endIt == end())
         {
            return false;
         }
         else
         {
            firstIt = endIt;
         }
      }
      else if (endIt == end())
      {
         endIt = firstIt;
      }
      
      ++endIt;
      
      for (ConstIterator it=firstIt; it!=endIt; ++it)
      {
         if (!it->valid())
         {
            return false;
         }
      }
      
      return true;
   }
   
   bool getSamples(double t, ConstIterator &it0, ConstIterator &it1, double &blend)
   {
      return getSamples(t, it0, it1, blend, mIndices.begin(), mIndices.end());
   }
   
   bool getSamples(double t, ConstIterator &it0, ConstIterator &it1, double &blend) const
   {
      return getSamples(t, it0, it1, blend, mIndices.begin(), mIndices.end());
   }
   
   // Returns true if samples array was modified with respect to given time range
   // Note: samples may have been removed outside of [t0,t1] range if merge is false
   //       still only if samples in [t0,t1] range have changed will true be returned
   bool update(const IObject &owner, double t0, double t1, float scale, bool merge=false)
   {
      bool modified = false;
      
      Alembic::AbcCoreAbstract::TimeSamplingPtr timeSampling = owner.getTimeSampling();
      size_t maxSamples = owner.getNumSamples();
      
      if (maxSamples == 0)
      {
         if (!merge)
         {
            clear();
            modified = true;
         }
         
         return modified;
      }
      
      std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime0, indexTime1;
      Alembic::AbcCoreAbstract::index_t sampleIndex;
      
      if (SampleUtils<IObject>::IsConstant(owner))
      {
         indexTime0.first = 0;
         indexTime0.second = timeSampling->getSampleTime(0);
         indexTime1 = indexTime1;
         // Force merge false for constant samples
         merge = false;
      }
      else
      {
         indexTime0 = timeSampling->getFloorIndex(t0, maxSamples);
         indexTime1 = timeSampling->getCeilIndex(t1, maxSamples);
      }
      
      std::set<Alembic::AbcCoreAbstract::index_t> usedSamples;
      
      for (sampleIndex = indexTime0.first; sampleIndex <= indexTime1.first; ++sampleIndex)
      {
         MapIt indexIt = mIndices.find(sampleIndex);
         
         if (indexIt != mIndices.end())
         {
            // Check sample validity and fetch it if needed
            if (!indexIt->second->valid())
            {
               #ifdef _DEBUG
               std::cout << "[TimeSampleList] Read sample " << sampleIndex << std::endl;
               #endif
               
               indexIt->second->get(owner, sampleIndex, scale);
               
               modified = true;
            }
            else if (fabs(scale - indexIt->second->scale()) > 0.000001f)
            {
               #ifdef _DEBUG
               std::cout << "[TimeSampleList] Re-scale sample " << sampleIndex << std::endl;
               #endif

               indexIt->second->scale(scale);

               modified = true;
            }

            if (!merge)
            {
               usedSamples.insert(indexIt->first);
            }
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "[TimeSampleList] Read sample " << sampleIndex << std::endl;
            #endif
            
            // Create a new sample
            ListIt sampIt = mSamples.insert(mSamples.end(), TimeSampleType());
            
            mIndices[sampleIndex] = sampIt;
            
            sampIt->get(owner, sampleIndex, scale);
            
            if (!merge)
            {
               usedSamples.insert(sampleIndex);
            }
            
            modified = true;
         }
      }
      
      if (!merge)
      {
         MapIt indexIt = mIndices.begin();
         MapIt prevIt = mIndices.end();
         
         while (indexIt != mIndices.end())
         {
            if (usedSamples.find(indexIt->first) == usedSamples.end())
            {
               // Remove sample
               mSamples.erase(indexIt->second);
               
               // Remove sample index
               mIndices.erase(indexIt);
               
               // Reset indexIt to continue iteration
               if (prevIt == mIndices.end())
               {
                  indexIt = mIndices.begin();
               }
               else
               {
                  indexIt = prevIt;
                  ++indexIt;
               }
            }
            else
            {
               prevIt = indexIt;
               ++indexIt;
            }
         }
      }
      
      return modified;
   }

private:
   
   template <class IteratorType>
   bool getSamples(double t, ConstIterator &it0, ConstIterator &it1, double &blend,
                   const IteratorType &beg, const IteratorType &end) const
   {
      double tmin, tmax;
      
      if (getTimeRange(tmin, tmax))
      {
         blend = 0.0;
         
         if (t < tmin)
         {
            it0 = ConstIterator(beg);
            it1 = ConstIterator(end);
            
            return true;
         }
         else if (t > tmax)
         {
            IteratorType indexIt = end;
            --indexIt;
            
            it0 = ConstIterator(indexIt);
            it1 = ConstIterator(end);
            
            return true;
         }
         else
         {
            IteratorType indexIt = find(t, -1, beg, end);
            
            if (indexIt != end)
            {
               it0 = ConstIterator(indexIt);
               it1 = ConstIterator(end);
               
               if (t - indexIt->second->time() > 0.0001)
               {
                  IteratorType nextIt = indexIt;
                  ++nextIt;
                  
                  if (nextIt != end)
                  {
                     it1 = ConstIterator(nextIt);
                     
                     blend = (t - indexIt->second->time()) / (nextIt->second->time() - indexIt->second->time());
                  }
               }
               
               return true;
            }
            else
            {
               return false;
            }
         }
      }
      else
      {
         it0 = ConstIterator(end);
         it1 = ConstIterator(end);
         
         return false;
      }
   }
   
   template <class IteratorType>
   IteratorType find(double t, int which,
                     const IteratorType &beg, const IteratorType &end) const
   {
      IteratorType indexIt;
      
      for (indexIt = beg; indexIt != end; ++indexIt)
      {
         if (fabs(t - indexIt->second->time()) <= 0.0001)
         {
            // Found matching sample
            if (which == -2)
            {
               // We want the previous sample
               return (indexIt == beg ? end : --indexIt);
            }
            else if (which == 2)
            {
               // We want the next sample
               return ++indexIt;
            }
            else
            {
               return indexIt;
            }
         }
         else if (indexIt->second->time() > t)
         {
            // indexIt->second->time() > t
            // => if we have a previous sample we know its time is not t
            
            if (which >= 1)
            {
               return indexIt;
            }
            else if (which <= -1)
            {
               return (indexIt == beg ? end : --indexIt);
            }
            else
            {
               // we wanted the 'exact' sample
               return end;
            }
         }
         else
         {
            // indexIt->second->time() < t
            // just continue to next sample
         }
      }
      
      // return last sample if we have one and the query includes previous sample
      
      if (indexIt == end && which <= -1 && !empty())
      {
         --indexIt;
      }
      
      return indexIt;
   }
   
private:
   
   List mSamples;
   Map mIndices;
};


template <class IObject>
struct SampleSetBase
{
   typedef typename IObject::schema_type Schema;
   
   TimeSampleList<Schema> schemaSamples;
   TimeSampleList<Alembic::AbcGeom::IBox3dProperty> boundsSamples;
   TimeSampleList<ILocatorProperty> locatorSamples;
   
   void clear()
   {
      schemaSamples.clear();
      boundsSamples.clear();
      locatorSamples.clear();
   }
};

// Setup your include path so the right one is picked
// This header should contain the declaration of a template class called SampleData
//   whose template argument will be on of the Alembic::AbcGeom::I* class

// Default declaration 
//
// class <class IObject>
// struct SampleSet : public SampleSetBase<IObject>
// {
// }

// Custom declaration example
//
// class <class IObject>
// struct SampleSet : public SampleSetBase<IObject>
// {
//    TimeSampleList<Alembic::Abc::IV2fGeomParam> uvSamples;
//    TimeSampleList<Alembic::Abc::IN3fGeomParam> nSamples;
// }

#include <AlembicNodeSampleData.h>

#endif
