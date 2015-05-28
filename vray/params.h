#ifndef __abc_vray_params_h__
#define __abc_vray_params_h__

#include "common.h"
#include <map>


struct ParamBase : VR::VRayPluginParameter
{
   ParamBase(const tchar *paramName, bool ownName=false);
   ParamBase(const ParamBase &other);
   virtual ~ParamBase();

   virtual const tchar* getName(void);
   
protected:

   bool mOwnName;
   const tchar *mName;
};

// ---

struct ListParamBase : ParamBase
{
   ListParamBase(const tchar *paramName, bool ownName=false);
   ListParamBase(const ListParamBase &other);
   virtual ~ListParamBase();
   
   virtual VR::ListHandle openList(int i);
   virtual void closeList(VR::ListHandle list);
   
protected:
   
   int mListLevel;
};

// ---

template <typename T, int S=100>
class CopyableTable : public VR::Table<T, S>
{
public:
   
   CopyableTable(int incStep=S)
      : VR::Table<T, S>(incStep)
   {
   }
   
   CopyableTable(int size, const T &initVal, int incStep=S)
      : VR::Table<T, S>(size, initVal, incStep)
   {
   }
   
   CopyableTable(const CopyableTable &rhs)
      : VR::Table<T, S>(rhs.incStep)
   {
      operator=(rhs);
   }
   
   virtual ~CopyableTable()
   {
   }
   
   CopyableTable<T, S>& operator=(const CopyableTable<T, S> &rhs)
   {
      if (this != &rhs)
      {
         if (this->elements)
         {
            delete[] this->elements;
         }
         
         this->numElements = rhs.numElements;
         this->maxElements = this->numElements;
         this->incStep = rhs.incStep;
         
         if (this->numElements > 0)
         {
            this->elements = new T[this->numElements];
            
            for (int i=0; i<this->numElements; ++i)
            {
               this->elements[i] = rhs.elements[i];
            }
         }
         else
         {
            this->elements = 0;
         }
      }
      
      return *this;
   }
};

template <typename T>
struct ListParamT : ListParamBase, VR::VRaySettableParamInterface, VR::VRayCloneableParamInterface, VR::MyInterpolatingInterface
{
   typedef CopyableTable<T> Table;
   //typedef typename VR::Table<T> Table;
   typedef VR::PtrArray<T> List;
   typedef std::map<double, Table> Map;
   
   ListParamT(const tchar *paramName, bool ownName=false)
      : ListParamBase(paramName, ownName)
   {
   }

   ListParamT(const ListParamT<T> &other)
      : ListParamBase(other)
      , mTimedValues(other.mTimedValues)
   {
   }
   
   virtual ~ListParamT()
   {
      clear();
   }
   
   // New
   void operator+=(const T &i)
   {
      mTimedValues[0.0] += i;
   }

   T& operator[](int i)
   {
      return mTimedValues[0.0][i];
   }
   
   const T& operator[](int i) const
   {
      return mTimedValues[0.0][i];
   }
   
   void clear()
   {
      for (typename Map::iterator it = mTimedValues.begin(); it != mTimedValues.end(); ++it)
      {
         it->second.freeMem();
      }
      
      mTimedValues.clear();
   }
   
   Table& at(double time)
   {
      return mTimedValues[time];
   }
   
   void setValue(const T &value, int index, double time)
   {
      Table &values = mTimedValues[time];
      
      if (index >= 0 && index < values.count())
      {
         values[index] = value;
      }
      else
      {
         values += value;
      }
   }
   
   void setValueList(const List &lst, double time)
   {
      Table &values = mTimedValues[time];
      
      // Create an exact copy
      
      if (values.elements)
      {
         delete[] values.elements;
      }
      
      values.numElements = lst.count();
      values.maxElements = values.numElements;
      
      if (values.numElements > 0)
      {
         values.elements = new T[values.numElements];
         for (int i=0; i<values.numElements; ++i)
         {
            values.elements[i] = lst[i];
         }
      }
      else
      {
         values.elements = 0;
      }
   }
   
   T getValue(int index, double time)
   {
      typename Map::iterator it = mTimedValues.find(time);
      
      if (it != mTimedValues.end())
      {
         return it->second[index]; 
      }
      else
      {
         T tmp;
         return tmp;
      }
   }
   
   List getValueList(double time)
   {
      typename Map::iterator it = mTimedValues.find(time);
      
      if (it != mTimedValues.end())
      {
         return List(&(it->second[0]), it->second.count());
      }
      else
      {
         return List();
      }
   }
   
   // From PluginInterface
   virtual PluginInterface* newInterface(InterfaceID id)
   {
      switch (id)
      {
      case EXT_SETTABLE_PARAM:
         return static_cast<VR::VRaySettableParamInterface*>(this);
      case EXT_CLONEABLE_PARAM:
         return static_cast<VR::VRayCloneableParamInterface*>(this);
      case EXT_INTERPOLATING:
         return static_cast<VR::MyInterpolatingInterface*>(this);
      default:
         return ListParamBase::newInterface(id);
      }
   }
   
   // From MyInterpolatingInterface
   virtual int getNumKeyFrames(void)
   {
      return int(mTimedValues.size());
   }
   
   virtual double getKeyFrameTime(int index)
   {
      typename Map::iterator it = mTimedValues.begin();
      for (int i=0; i<index; ++i, ++it);
      return (it == mTimedValues.end() ? -1.0 : it->first);
   }
   
   virtual int isIncremental(void)
   {
      return false;
   }

   // From VRaySettableParamInterface
   virtual void setCount(int count, double time=0.0)
   {
      mTimedValues[time].setCount(count, true);
   }
   
   virtual void reserve(int count, double time=0.0)
   {
      Table &values = mTimedValues[time];
      
      values.setCount(count, true);
      values.clear();
   }
   
   // From VRayPluginParameter
   virtual int getCount(double time)
   {
      if (mListLevel == 0)
      {
         typename Map::iterator it = mTimedValues.find(time);
         return (it != mTimedValues.end() ? it->second.count() : 0);
      }
      else
      {
         return -1;
      }
   }
   
protected:
   
   Map mTimedValues;
};

// ---

struct VectorListParam : ListParamT<VR::Vector>
{
   VectorListParam(const tchar *paramName, bool ownName=false);
   VectorListParam(const VectorListParam &rhs);
   virtual ~VectorListParam();
   
   // From PluginInterface
   virtual PluginBase* getPlugin(void);
   
   // From VRayPluginParameter
   virtual VR::VRayParameterType getType(int index, double time=0.0);
   virtual VR::Vector getVector(int index=0, double time=0.0);
   virtual VR::VectorList getVectorList(double time=0.0);

   // From VRaySettableParamInterface
   virtual void setVector(const VR::Vector &value, int index=0, double time=0.0);
   virtual void setVectorList(const VR::VectorRefList &value, double time=0.0);

   // From VRayCloneableParamInterface
   virtual VR::VRayPluginParameter* clone();
};

// ---

struct ColorListParam : ListParamT<VR::Color>
{
   ColorListParam(const tchar *paramName, bool ownName=false);
   ColorListParam(const ColorListParam &rhs);
   virtual ~ColorListParam();
   
   // From PluginInterface
   virtual PluginBase* getPlugin(void);
   
   // From VRayPluginParameter
   virtual VR::VRayParameterType getType(int index, double time=0.0);
   virtual VR::Color getColor(int index=0, double time=0.0);
   virtual VR::AColor getAColor(int index=0, double time=0.0);
   virtual VR::ColorList getColorList(double time=0.0);

   // From VRaySettableParamInterface
   virtual void setColor(const VR::Color &value, int index=0, double time=0.0);
   virtual void setAColor(const VR::AColor &value, int index=0, double time=0.0);
   virtual void setColorList(const VR::ColorRefList &value, double time=0.0);

   // From VRayCloneableParamInterface
   virtual VR::VRayPluginParameter* clone();
};

// ---

struct IntListParam : ListParamT<int>
{
   IntListParam(const tchar *paramName, bool ownName=false);
   IntListParam(const IntListParam &rhs);
   virtual ~IntListParam();
   
   // From PluginInterface
   virtual PluginBase* getPlugin(void);
   
   // From VRayPluginParameter
   virtual VR::VRayParameterType getType(int index, double time=0.0);
   virtual int getBool(int index=0, double time=0.0);
   virtual int getInt(int index=0, double time=0.0);
   virtual float getFloat(int index=0, double time=0.0);
   virtual double getDouble(int index=0, double time=0.0);
   virtual VR::IntList getIntList(double time=0.0);

   // From VRaySettableParamInterface
   virtual void setBool(int value, int index=0, double time=0.0);
   virtual void setInt(int value, int index=0, double time=0.0);
   virtual void setFloat(float value, int index=0, double time=0.0);
   virtual void setDouble(double value, int index=0, double time=0.0);
   virtual void setIntList(const VR::IntRefList &value, double time=0.0);

   // From VRayCloneableParamInterface
   virtual VR::VRayPluginParameter* clone();
};

// ---

struct FloatListParam : ListParamT<float>
{
   FloatListParam(const tchar *paramName, bool ownName=false);
   FloatListParam(const FloatListParam &rhs);
   virtual ~FloatListParam();
   
   // From PluginInterface
   virtual PluginBase* getPlugin(void);
   
   // From VRayPluginParameter
   virtual VR::VRayParameterType getType(int index, double time=0.0);
   virtual int getBool(int index=0, double time=0.0);
   virtual int getInt(int index=0, double time=0.0);
   virtual float getFloat(int index=0, double time=0.0);
   virtual double getDouble(int index=0, double time=0.0);
   virtual VR::FloatList getFloatList(double time=0.0);

   // From VRaySettableParamInterface
   virtual void setBool(int value, int index=0, double time=0.0);
   virtual void setInt(int value, int index=0, double time=0.0);
   virtual void setFloat(float value, int index=0, double time=0.0);
   virtual void setDouble(double value, int index=0, double time=0.0);
   virtual void setFloatList(const VR::FloatRefList &value, double time=0.0);

   // From VRayCloneableParamInterface
   virtual VR::VRayPluginParameter* clone();
};

// ---

struct AlembicLoaderParams : VR::VRayParameterListDesc
{
   struct Cache
   {
      VR::CharString filename;
      VR::CharString referenceFilename;
      VR::CharString objectPath;
      float startFrame;
      float endFrame;
      float speed;
      float offset;
      float fps;
      int cycle;
      int preserveStartFrame;
      int ignoreTransforms;
      int ignoreInstances;
      int ignoreVisibility;
      int ignoreTransformBlur;
      int ignoreDeformBlur;
      int verbose;
      
      int subdivEnable;
      int subdivUVs;
      int preserveMapBorders;
      int staticSubdiv;
      int classicCatmark;
      
      int osdSubdivEnable;
      int osdSubdivLevel;
      int osdSubdivType;
      int osdSubdivUVs;
      int osdPreserveMapBorders;
      int osdPreserveGeometryBorders;
      
      int useGlobals;
      int viewDep;
      float edgeLength;
      int maxSubdivs;
      
      int displacementType;
      VR::TextureInterface *displacementTexColor;
      VR::TextureFloatInterface *displacementTexFloat;
      float displacementAmount;
      float displacementShift;
      int keepContinuity;
      float waterLevel;
      int vectorDisplacement;
      int mapChannel; // Not used
      int useBounds;
      VR::Color minBound;
      VR::Color maxBound;
      int imageWidth;
      int cacheNormals;
      int objectSpaceDisplacement; // Not used
      int staticDisplacement;
      int displace2d;
      int resolution;
      int precision;
      int tightBounds;
      int filterTexture;
      float filterBlur;
      
      int particleType;
      VR::CharString particleAttribs;
      float spriteSizeX;
      float spriteSizeY;
      float spriteTwist;
      int spriteOrientation;
      float radius;
      float pointSize;
      int pointRadii;
      int pointWorldSize;
      int multiCount;
      float multiRadius;
      float lineWidth;
      float tailLength;
      int sortIDs;
      
      float psizeScale;
      float psizeMin;
      float psizeMax;
      
      float time;
      
      Cache();
      void setupCache(VR::VRayParameterList *paramList);
   };
   
   AlembicLoaderParams();
   virtual ~AlembicLoaderParams();
};

#endif
