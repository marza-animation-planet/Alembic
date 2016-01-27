#ifndef SCENEHELPER_SAMPLEUTILS_H_
#define SCENEHELPER_SAMPLEUTILS_H_

#include "LocatorProperty.h"
#include <Alembic/AbcCoreAbstract/All.h>
#include <Alembic/AbcGeom/All.h>
#include <limits>
#include <list>
#include <map>
#include <set>

template <class IObject>
struct SampleUtils
{
   typedef IObject AlembicType;
   typedef typename IObject::Sample SampleType;
   
   static bool IsValid(const SampleType &samp) { return samp.valid(); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
   }
   
   static void Reset(SampleType &samp) { samp.reset(); }
};

template<> struct SampleUtils<Alembic::AbcGeom::IXformSchema>
{
   typedef Alembic::AbcGeom::IXformSchema AlembicType;
   typedef Alembic::AbcGeom::IXformSchema::sample_type SampleType;
   
   static bool IsValid(const SampleType &samp) { return (samp.getNumOps() > 0); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
   }
   
   static void Reset(SampleType &samp) { samp.reset(); }
};

template <class Traits>
struct SampleUtils<Alembic::AbcGeom::ITypedScalarProperty<Traits> >
{
   typedef Alembic::AbcGeom::ITypedScalarProperty<Traits> AlembicType;
   typedef typename AlembicType::value_type SampleType;
   
   static bool IsValid(const SampleType &) { return true; }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, SampleType &out,
                   const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
   }
   
   static void Reset(SampleType &) { }
};

template <class Traits>
struct SampleUtils<Alembic::AbcGeom::ITypedArrayProperty<Traits> >
{
   typedef Alembic::AbcGeom::ITypedArrayProperty<Traits> AlembicType;
   typedef typename AlembicType::sample_ptr_type SampleType;
   
   static bool IsValid(const SampleType &samp) { return samp->valid(); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, SampleType &out,
                  const Alembic::Abc::ISampleSelector &selector = Alembic::Abc::ISampleSelector())
   {
      owner.get(out, selector);
   }
   
   static void Reset(SampleType &samp) { samp->reset(); }
};

template <class Traits>
struct SampleUtils<Alembic::AbcGeom::ITypedGeomParam<Traits> >
{
   typedef Alembic::AbcGeom::ITypedGeomParam<Traits> AlembicType;
   typedef typename AlembicType::Sample SampleType;
   
   static bool IsValid(const SampleType &samp) { return samp.valid(); }
   
   static bool IsConstant(const AlembicType &owner) { return owner.isConstant(); }
   
   static void Get(const AlembicType &owner, SampleType &out,
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
   }
   
   static void Reset(SampleType &samp) { samp.reset(); }
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
   
   bool get(const IObject &owner, Alembic::AbcCoreAbstract::index_t i, double *t=0)
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
               SampleUtils<IObject>::Get(owner, mData);
            }
            else
            {
               if (i >= 0 && size_t(i) < owner.getNumSamples())
               {
                  mIndex = i;
                  mTime = (t ? *t : owner.getTimeSampling()->getSampleTime(i));
                  SampleUtils<IObject>::Get(owner, mData, sampleSelector());
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
   
   bool get(const IObject &owner, double t, Alembic::AbcCoreAbstract::index_t *index=0)
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
               SampleUtils<IObject>::Get(owner, mData);
            }
            else
            {
               if (index)
               {
                  mIndex = *index;
                  mTime = t;
                  SampleUtils<IObject>::Get(owner, mData, sampleSelector());
               }
               else
               {
                  // index not provided, figure it out
                  std::pair<Alembic::AbcCoreAbstract::index_t, double> indexTime = owner.getTimeSampling()->getNearIndex(t, owner.getNumSamples());
                  
                  if (fabs(indexTime.second - t) > 0.0001)
                  {
                     reset();
                  }
                  else
                  {
                     mIndex = indexTime.first;
                     mTime = indexTime.second;
                     SampleUtils<IObject>::Get(owner, mData, sampleSelector());
                  }
               }
            }
         }
         return valid();
      }
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
   bool update(const IObject &owner, double t0, double t1, bool merge=false)
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
               
               indexIt->second->get(owner, sampleIndex);
               
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
            
            sampIt->get(owner, sampleIndex);
            
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
