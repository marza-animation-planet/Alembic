#ifndef ALEMBICSHAPE_ALEMBICSCENE_H_
#define ALEMBICSHAPE_ALEMBICSCENE_H_

#include <Alembic/AbcGeom/All.h>
#include <boost/regex.hpp>
#include <vector>
#include <map>
#include <set>

class AlembicScene;

class AlembicNode
{
public:
   
   friend class AlembicScene;
   
   typedef std::vector<AlembicNode*> Array;
   typedef std::map<std::string, AlembicNode*> Map;
   typedef std::set<AlembicNode*> Set;
   
   enum VisitMode
   {
      VisitDepthFirst = 0,
      VisitBreadthFirst,
      VisitFilteredFlat
   };
   
   enum VisitReturn
   {
      StopVisit = 0,
      ContinueVisit,
      DontVisitChildren
   };
   
   enum NodeType
   {
      TypeGeneric = 0,
      TypeMesh,
      TypeSubD,
      TypePoints,
      TypeNuPatch,
      TypeCurves,
      TypeXform,
      LastType
   };
   
   enum PrefixType
   {
      GlobalPrefix = 0,
      LocalPrefix
   };
   
public:
   
   static AlembicNode* Wrap(Alembic::Abc::IObject iObj, AlembicNode *iParent);
   
public:
   
   AlembicNode();
   AlembicNode(Alembic::Abc::IObject iObj, AlembicNode *parent=0);
   virtual ~AlembicNode();
   
   bool isValid() const;
   
   NodeType type() const;
   const char* typeName() const;
   
   Alembic::Abc::IObject object() const;
   
   const std::string& name() const;
   const std::string& path() const;
   std::string partialPath() const;
   
   std::string formatName(const char *prefix) const;
   std::string formatPath(const char *prefix, PrefixType type=LocalPrefix, char separator='/') const;
   std::string formatPartialPath(const char *prefix, PrefixType type=LocalPrefix, char separator='/') const;
   
   AlembicNode* parent();
   const AlembicNode* parent() const;
   
   size_t childCount() const;
   AlembicNode* child(size_t i);
   const AlembicNode* child(size_t i) const;
   AlembicNode* child(const char *name);
   const AlembicNode* child(const char *name) const;
   AlembicNode* find(const char *path);
   const AlembicNode* find(const char *path) const;
   Array::iterator beginChild();
   Array::iterator endChild();
   Array::const_iterator beginChild() const;
   Array::const_iterator endChild() const;
   bool hasAncestor(const AlembicNode *node) const;
   
   bool isInstance() const;
   void resolveInstances(AlembicScene *scene);
   size_t numInstances() const;
   size_t instanceNumber() const;
   AlembicNode* master();
   const std::string& masterPath() const;
   const AlembicNode* master() const;
   AlembicNode* instance(size_t i);
   const AlembicNode* instance(size_t i) const;
   
   bool isLocator() const;
   const Alembic::Abc::IScalarProperty& locatorProperty() const;
   void setLocatorPosition(const Alembic::Abc::V3d &p);
   void setLocatorScale(const Alembic::Abc::V3d &s);
   inline const Alembic::Abc::V3d& locatorPosition() const { return mLocatorPosition; }
   inline const Alembic::Abc::V3d& locatorScale() const { return mLocatorScale; }

   inline const Alembic::Abc::Box3d selfBounds() const { return (mMaster ? mMaster->selfBounds() : mSelfBounds); }
   inline const Alembic::Abc::M44d selfMatrix() const { return (mMaster ? mMaster->selfMatrix() : mSelfMatrix); }
   inline bool inheritsTransform() const { return (mMaster ? mMaster->inheritsTransform() : mInheritsTransform); }
   bool isVisible(bool inherited=false) const;
   
   void setSelfBounds(const Alembic::Abc::Box3d &b);
   void setSelfMatrix(const Alembic::Abc::M44d &m);
   void setInheritsTransform(bool on);
   void setVisible(bool on);
   
   inline const Alembic::Abc::Box3d childBounds() const { return mChildBounds; }
   inline const Alembic::Abc::M44d worldMatrix() const { return mWorldMatrix; }
   
   void updateWorldMatrix();
   void updateChildBounds();
   
   void resetWorldMatrix();
   void resetChildBounds();
   
   template <class Visitor>
   bool visit(VisitMode mode, AlembicScene *scene, Visitor &visitor);
   
   template <class Visitor>
   VisitReturn enter(Visitor &visitor, AlembicNode *instance=0);
   
   template <class Visitor>
   void leave(Visitor &visitor, AlembicNode *instance=0);
   
   inline AlembicNode* child(const std::string &name) { return child(name.c_str()); }
   inline const AlembicNode* child(const std::string &name) const { return child(name.c_str()); }
   inline AlembicNode* find(const std::string &path) { return find(path.c_str()); }
   inline const AlembicNode* find(const std::string &path) const { return child(path.c_str()); }
   
   template <class T>
   void getSamplesInfo(double iTime,
                       const T &iTarget,
                       Alembic::AbcCoreAbstract::index_t &oFloorIndex,
                       Alembic::AbcCoreAbstract::index_t &oCeilIndex,
                       double &oFloorTime,
                       double &oCeilTime,
                       double &oBlend) const
   {
      Alembic::AbcCoreAbstract::TimeSamplingPtr sampler = iTarget.getTimeSampling();
      
      size_t numSamps = iTarget.getNumSamples();
      
      if (numSamps <= 0)
      {
         oFloorIndex = -1;
         oCeilIndex = -1;
         oFloorTime = 0.0;
         oCeilTime = 0.0;
         oBlend = 0.0;
         return;
      }

      std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndexTime = sampler->getFloorIndex(iTime, numSamps);

      oFloorIndex = floorIndexTime.first;
      oFloorTime = floorIndexTime.second;
      oCeilIndex = oFloorIndex;
      oCeilTime = oFloorTime;

      if (fabs(iTime - floorIndexTime.second) < 0.0001)
      {
         oBlend = 0.0;
         return;
      }

      std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndexTime = sampler->getCeilIndex(iTime, numSamps);

      if (oFloorIndex == ceilIndexTime.first)
      {
         oBlend = 0.0;
         return;
      }

      oCeilIndex = ceilIndexTime.first;
      oCeilTime = ceilIndexTime.second;

      double blend = (iTime - floorIndexTime.second) / (ceilIndexTime.second - floorIndexTime.second);

      if (fabs(1.0 - blend) < 0.0001)
      {
         oFloorIndex = oCeilIndex;
         oFloorTime = oCeilTime;
         oBlend = 0.0;
      }
      else
      {
         oBlend = blend;
      }
   }
   
   template <class T>
   inline Alembic::AbcCoreAbstract::index_t getSampleIndex(double iTime, const T &iTarget) const
   {
      Alembic::AbcCoreAbstract::index_t fi, ci;
      double ft, ct, b;
      
      getSamplesInfo(iTime, iTarget, fi, ci, ft, ct, b);
      
      return fi;
   }
   
protected:

   void setType(NodeType nt);
   
   void addInstance(AlembicNode *node);
   void removeInstance(AlembicNode *node);
   void resetMaster();
   
   void formatPath(std::string &path, const char *prefix, PrefixType type, char separator) const;

protected:
   
   Alembic::Abc::IObject mIObj;
   
   AlembicNode *mParent;
   
   Array mChildren;
   Map mChildByName;
   
   std::string mMasterPath;
   AlembicNode *mMaster;
   Array mInstances;
   size_t mInstanceNumber;
   
   NodeType mType;
   
   Alembic::Abc::Box3d mSelfBounds;
   Alembic::Abc::M44d mSelfMatrix;
   
   Alembic::Abc::Box3d mChildBounds;
   Alembic::Abc::M44d mWorldMatrix;
   
   Alembic::Abc::IScalarProperty mLocatorProp;
   Alembic::Abc::V3d mLocatorPosition;
   Alembic::Abc::V3d mLocatorScale;
   
   bool mInheritsTransform;
   
   bool mVisible;
};

// ---

template <AlembicNode::NodeType Type> 
struct TypeToClass
{
   typedef AlembicNode Class;
};

template <class Class>
struct ClassToType
{
   enum
   {
      Type = AlembicNode::TypeGeneric
   };
};

// ---

template <class ISchemaClass>
struct SchemaUtils
{
   typedef typename ISchemaClass::Sample ISampleClass;
   
   static bool IsValid(const ISampleClass &samp)
   {
      return samp.valid();
   }
   
   static Alembic::Abc::IBox3dProperty GetBoundsProperty(ISchemaClass &schema)
   {
      Alembic::Abc::IBox3dProperty prop = schema.getSelfBoundsProperty();
      if (!prop.valid())
      {
         prop = schema.getChildBoundsProperty();
      }
      return prop;
   }
};

template <>
struct SchemaUtils<Alembic::AbcGeom::IXformSchema>
{
   typedef Alembic::AbcGeom::IXformSchema::sample_type ISampleClass;
   
   static bool IsValid(const ISampleClass &samp)
   {
      return (samp.getNumOps() > 0);
   }
   
   static Alembic::Abc::IBox3dProperty GetBoundsProperty(Alembic::AbcGeom::IXformSchema &)
   {
      return Alembic::Abc::IBox3dProperty();
   }
};

// ---

template <class IClass>
struct SampleData
{
   typedef typename IClass::schema_type ISchemaClass;
   typedef typename SchemaUtils<ISchemaClass>::ISampleClass ISampleClass;
   
   ISampleClass data;
   
   void get(IClass iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      iObj.getSchema().get(data, Alembic::Abc::ISampleSelector(index));
   }
   
   void reset(IClass)
   {
      data.reset();
   }
   
   bool valid(IClass) const
   {
      return SchemaUtils<ISchemaClass>::IsValid(data);
   }
   
   bool constant(IClass iObj) const
   {
      return iObj.getSchema().isConstant();
   }
};

// Setup your include path so the right one is picked
#include <AlembicNodeSampleData.h>

// ---

template <class IClass>
class AlembicNodeT : public AlembicNode
{
public:
   
   typedef AlembicNodeT<IClass> ThisType;
   typedef typename IClass::schema_type ISchemaClass;
   
   struct Sample : public SampleData<IClass>
   {
      // putting that here, we avoid having to redeclare in every SampleData class specialization
      double boundsTime;
      double boundsWeight;
      Alembic::AbcCoreAbstract::index_t boundsIndex;
      Alembic::Abc::Box3d bounds;
      
      double dataTime;
      double dataWeight;
      Alembic::AbcCoreAbstract::index_t dataIndex;
      
      double locator[6];
   };
   
public:
   
   AlembicNodeT()
      : AlembicNode()
   {
      setType((NodeType) ClassToType<ThisType>::Type);
      initSamples();
   }
   
   AlembicNodeT(IClass iObject, AlembicNode *iParent=0)
      : AlembicNode(iObject, iParent), mITypedObj(iObject)
   {
      setType((NodeType) ClassToType<ThisType>::Type);
      initSamples();
   }
   
   virtual ~AlembicNodeT()
   {
      resetSamplesData();
   }
   
   inline IClass typedObject() const
   {
      return mITypedObj;
   }
   
   inline const Sample& firstSample() const
   {
      return mSample0;
   }
   
   inline const Sample& secondSample() const
   {
      return mSample1;
   }
   
   inline Sample& firstSample()
   {
      return mSample0;
   }
   
   inline Sample& secondSample()
   {
      return mSample1;
   }
   
   // returns true when sampled successfully
   // use updated optional argument to know wether or not bounds have been re-sampled
   bool sampleBounds(double t, bool *updated=0)
   {
      if (type() == AlembicNode::TypeXform)
      {
         return sampleData(t, updated);
      }
      else
      {
         ISchemaClass &schema = mITypedObj.getSchema();
         
         Alembic::Abc::IBox3dProperty boundsProp = SchemaUtils<ISchemaClass>::GetBoundsProperty(schema);
         
         if (boundsProp.valid())
         {
            bool weightsChanged = false;
            
            if (needsBoundsReSampling(boundsProp, t, weightsChanged))
            {
               #ifdef _DEBUG
               std::cout << "Sample bounds for \"" << path() << "\"" << std::endl;
               #endif
               findBoundsSamples(boundsProp, t);
               
               if (updated)
               {
                  *updated = true;
               }
               
               return sampleBoundsInternal(boundsProp);
            }
            else
            {
               if (updated)
               {
                  *updated = weightsChanged;
               }
               
               return true;
            }
         }
         else
         {
            return false;
         }
      }
   }
   
   // returns true when sampled successfully
   // use updated optional argument to know wether or not dataa has been re-sampled
   bool sampleData(double t, bool *updated=0)
   {
      if (isLocator())
      {
         return sampleData(locatorProperty(), t, updated);
      }
      else
      {
         return sampleData(mITypedObj.getSchema(), t, updated);
      }
   }
   
protected:
   
   void initSamples()
   {
      mSample0.boundsIndex = -1;
      mSample0.boundsTime = std::numeric_limits<double>::max();
      mSample0.boundsWeight = 1.0;
      
      mSample0.dataIndex = -1;
      mSample0.dataTime = std::numeric_limits<double>::max();
      mSample0.dataWeight = 1.0;
      
      mSample1.boundsIndex = -1;
      mSample1.boundsTime = std::numeric_limits<double>::max();
      mSample1.boundsWeight = 0.0;
      
      mSample1.dataIndex = -1;
      mSample1.dataTime = std::numeric_limits<double>::max();
      mSample1.dataWeight = 0.0;
      
      resetSamplesData();
      resetSamplesBounds();
   }
   
   void resetSamplesBounds()
   {
      mSample0.bounds.makeInfinite();
      mSample1.bounds.makeInfinite();
   }
   
   void resetSamplesData()
   {
      mSample0.reset(mITypedObj);
      mSample0.locator[0] = 0.0;
      mSample0.locator[1] = 0.0;
      mSample0.locator[2] = 0.0;
      mSample0.locator[3] = 1.0;
      mSample0.locator[4] = 1.0;
      mSample0.locator[5] = 1.0;
      mSample1.reset(mITypedObj);
      mSample1.locator[0] = 0.0;
      mSample1.locator[1] = 0.0;
      mSample1.locator[2] = 0.0;
      mSample1.locator[3] = 1.0;
      mSample1.locator[4] = 1.0;
      mSample1.locator[5] = 1.0;
   }
   
   bool updateBoundsSampleWeights(double t)
   {
      if (mSample0.boundsIndex >= 0 &&
          mSample1.boundsIndex >= 0 &&
          mSample0.boundsTime <= t && t <= mSample1.boundsTime)
      {
         double blend = (t - mSample0.boundsTime) / (mSample1.boundsTime - mSample0.boundsTime);
         
         if (fabs(mSample1.boundsWeight - blend) > 0.0001)
         {
            #ifdef _DEBUG
            std::cout << path() << ": Bounds blend weights have changed: " << mSample1.boundsWeight << " -> " << blend << std::endl;
            #endif
            mSample0.boundsWeight = 1.0 - blend;
            mSample1.boundsWeight = blend;
         }
         
         return true;
      }
      else
      {
         return false;
      }
   }
   
   bool updateDataSampleWeights(double t)
   {
      if (mSample0.dataIndex >= 0 &&
          mSample1.dataIndex >= 0 &&
          mSample0.dataTime <= t && t <= mSample1.dataTime)
      {
         double blend = (t - mSample0.dataTime) / (mSample1.dataTime - mSample0.dataTime);
         
         if (fabs(mSample1.dataWeight - blend) > 0.0001)
         {
            #ifdef _DEBUG
            std::cout << path() << ": Data blend weights have changed: " << mSample1.dataWeight << " -> " << blend << std::endl;
            #endif
            mSample0.dataWeight = 1.0 - blend;
            mSample1.dataWeight = blend;
         }
         
         return true;
      }
      else
      {
         return false;
      }
   }
   
   bool needsBoundsReSampling(Alembic::AbcGeom::IBox3dProperty &boundsProp, double t, bool &weightsChanged)
   {
      if (mSample0.boundsIndex < 0 || mSample0.bounds.isInfinite())
      {
         return true;
      }
      else
      {
         if (boundsProp.isConstant())
         {
            weightsChanged = false;
            return false;
         }
         else
         {
            if (fabs(mSample0.boundsTime - t) > 0.0001)
            {
               weightsChanged = updateBoundsSampleWeights(t);
               return !weightsChanged;
            }
            else if (mSample0.boundsWeight < 1.0)
            {
               // Don't need to re-sample bounds, but update blend weights if necessary
               #ifdef _DEBUG
               std::cout << path() << ": Reset bounds blend weights" << std::endl;
               #endif
               
               mSample0.boundsWeight = 1.0;
               if (mSample1.boundsIndex >= 0)
               {
                  mSample1.boundsWeight = 0.0;
               }
               
               weightsChanged = true;
               return false;
            }
            else
            {
               weightsChanged = false;
               return false;
            }
         }
      }
   }
   
   bool needsDataReSampling(const Alembic::Abc::IScalarProperty &locatorProp, double t, bool &weightsChanged)
   {
      if (mSample0.dataIndex < 0) 
      {
         return true;
      }
      else
      {
         if (locatorProp.isConstant())
         {
            weightsChanged = false;
            return false;
         }
         else
         {
            if (fabs(t - mSample0.dataTime) > 0.0001)
            {
               weightsChanged = updateDataSampleWeights(t);
               return !weightsChanged;
            }
            else if (mSample0.dataWeight < 1.0)
            {
               // Don't need to re-sample geometry, but update blend weights if necessary
               #ifdef _DEBUG
               std::cout << path() << ": Reset data blend weights" << std::endl;
               #endif
               
               if (mSample1.dataIndex >= 0)
               {
                  mSample1.dataWeight = 0.0;
               }
               mSample0.dataWeight = 1.0;
               
               weightsChanged = true;
               return false;
            }
            else
            {
               weightsChanged = false;
               return false;
            }
         }
      }
   }
   
   bool needsDataReSampling(const ISchemaClass &, double t, bool &weightsChanged)
   {
      if (mSample0.dataIndex < 0 || !mSample0.valid(mITypedObj))
      {
         return true;
      }
      else
      {
         if (mSample0.constant(mITypedObj))
         {
            weightsChanged = false;
            return false;
         }
         else
         {
            if (fabs(t - mSample0.dataTime) > 0.0001)
            {
               weightsChanged = updateDataSampleWeights(t);
               return !weightsChanged;
            }
            else if (mSample0.dataWeight < 1.0)
            {
               // Don't need to re-sample geometry, but update blend weights if necessary
               #ifdef _DEBUG
               std::cout << path() << ": Reset data blend weights" << std::endl;
               #endif
               
               if (mSample1.dataIndex >= 0)
               {
                  mSample1.dataWeight = 0.0;
               }
               mSample0.dataWeight = 1.0;
               
               weightsChanged = true;
               return false;
            }
            else
            {
               weightsChanged = false;
               return false;
            }
         }
      }
   }
   
   void findBoundsSamples(Alembic::Abc::IBox3dProperty &bounds, double t)
   {
      double blend = 0.0;
      
      getSamplesInfo(t, bounds,
                     mSample0.boundsIndex, mSample1.boundsIndex,
                     mSample0.boundsTime, mSample1.boundsTime,
                     blend);
      
      mSample0.boundsWeight = 1.0 - blend;
      mSample1.boundsWeight = blend;
   }
   
   template <class T>
   void findDataSamples(const T &target, double t)
   {
      double blend = 0.0;
      
      getSamplesInfo(t, target,
                     mSample0.dataIndex, mSample1.dataIndex,
                     mSample0.dataTime, mSample1.dataTime,
                     blend);
      
      mSample0.dataWeight = 1.0 - blend;
      mSample1.dataWeight = blend;
   }
   
   bool sampleBoundsInternal(Alembic::AbcGeom::IBox3dProperty &boundsProp)
   {
      resetSamplesBounds();
      
      boundsProp.get(mSample0.bounds, Alembic::Abc::ISampleSelector(mSample0.boundsIndex));
      
      if (mSample1.boundsWeight > 0.0)
      {
         boundsProp.get(mSample1.bounds, Alembic::Abc::ISampleSelector(mSample1.boundsIndex));
      }
      
      return true;
   }
   
   template <class T>
   bool sampleData(const T &target, double t, bool *updated=0)
   {
      bool weightsChanged = false;
      
      if (needsDataReSampling(target, t, weightsChanged))
      {
         #ifdef _DEBUG
         std::cout << "Sample " << (isLocator() ? "locator" : typeName()) << " data for \"" << path() << "\"" << std::endl;
         #endif
         findDataSamples(target, t);
         
         if (updated)
         {
            *updated = true;
         }
         
         return sampleDataInternal(target);
      }
      else
      {
         // Note: weights may have changed
         if (updated)
         {
            *updated = weightsChanged;
         }
         
         return true;
      }
   }
   
   bool sampleDataInternal(const Alembic::Abc::IScalarProperty &locatorProp)
   {
      resetSamplesData();
      
      locatorProp.get(mSample0.locator, mSample0.dataIndex);
      
      if (mSample1.dataWeight > 0.0)
      {
         locatorProp.get(mSample1.locator, mSample1.dataIndex);
      }
      
      return true;
   }
   
   bool sampleDataInternal(const ISchemaClass &)
   {
      resetSamplesData();
      
      mSample0.get(mITypedObj, mSample0.dataIndex);
      
      if (!mSample0.valid(mITypedObj))
      {
         return false;
      }
      
      if (mSample1.dataWeight > 0.0)
      {
         mSample1.get(mITypedObj, mSample1.dataIndex);
         return mSample1.valid(mITypedObj);
      }
      else
      {
         return true;
      }
   }
   
protected:
   
   IClass mITypedObj;
   Sample mSample0;
   Sample mSample1;
};

typedef AlembicNodeT<Alembic::AbcGeom::IPolyMesh> AlembicMesh;
typedef AlembicNodeT<Alembic::AbcGeom::ISubD> AlembicSubD;
typedef AlembicNodeT<Alembic::AbcGeom::IPoints> AlembicPoints;
typedef AlembicNodeT<Alembic::AbcGeom::INuPatch> AlembicNuPatch;
typedef AlembicNodeT<Alembic::AbcGeom::ICurves> AlembicCurves;
typedef AlembicNodeT<Alembic::AbcGeom::IXform> AlembicXform;

template <> struct TypeToClass<AlembicNode::TypeMesh> { typedef AlembicMesh Class; };
template <> struct TypeToClass<AlembicNode::TypeSubD> { typedef AlembicSubD Class; };
template <> struct TypeToClass<AlembicNode::TypePoints> { typedef AlembicPoints Class; };
template <> struct TypeToClass<AlembicNode::TypeNuPatch> { typedef AlembicNuPatch Class; };
template <> struct TypeToClass<AlembicNode::TypeCurves> { typedef AlembicCurves Class; };
template <> struct TypeToClass<AlembicNode::TypeXform> { typedef AlembicXform Class; };

template <> struct ClassToType<AlembicMesh> { enum { Type = AlembicNode::TypeMesh }; };
template <> struct ClassToType<AlembicSubD> { enum { Type = AlembicNode::TypeSubD }; };
template <> struct ClassToType<AlembicPoints> { enum { Type = AlembicNode::TypePoints }; };
template <> struct ClassToType<AlembicNuPatch> { enum { Type = AlembicNode::TypeNuPatch }; };
template <> struct ClassToType<AlembicCurves> { enum { Type = AlembicNode::TypeCurves }; };
template <> struct ClassToType<AlembicXform> { enum { Type = AlembicNode::TypeXform }; };

// ---

class AlembicScene : public AlembicNode
{
public:
   
   AlembicScene(Alembic::Abc::IArchive iArch);
   virtual ~AlembicScene();
   
   bool isFiltered(AlembicNode *node) const;
   void setFilter(const std::string &filter);
   Set::iterator beginFiltered();
   Set::iterator endFiltered();
   Set::const_iterator beginFiltered() const;
   Set::const_iterator endFiltered() const;
   
   template <class Visitor>
   bool visit(VisitMode mode, Visitor &visitor);
   
   AlembicNode* find(const char *path);
   const AlembicNode* find(const char *path) const;
   
   inline AlembicNode* find(const std::string &path) { return find(path.c_str()); }
   inline const AlembicNode* find(const std::string &path) const { return find(path.c_str()); }
   
   inline Alembic::Abc::IArchive archive() { return mArchive; }
   
private:
   
   bool filter(AlembicNode *node);

private:
   
   Alembic::Abc::IArchive mArchive;
   bool mFiltered;
   //MString mFilter;
   boost::regex mFilter;
   std::set<AlembicNode*> mFilteredNodes;
};

// ---

template <class Visitor>
AlembicNode::VisitReturn AlembicNode::enter(Visitor &visitor, AlembicNode *instance)
{
   switch (mType)
   {
   case TypeGeneric:
      return visitor.enter(*((typename TypeToClass<TypeGeneric>::Class*)this), instance);
   case TypeMesh:
      return visitor.enter(*((typename TypeToClass<TypeMesh>::Class*)this), instance);
   case TypeSubD:
      return visitor.enter(*((typename TypeToClass<TypeSubD>::Class*)this), instance);
   case TypePoints:
      return visitor.enter(*((typename TypeToClass<TypePoints>::Class*)this), instance);
   case TypeCurves:
      return visitor.enter(*((typename TypeToClass<TypeCurves>::Class*)this), instance);
   case TypeNuPatch:
      return visitor.enter(*((typename TypeToClass<TypeNuPatch>::Class*)this), instance);
   case TypeXform:
      return visitor.enter(*((typename TypeToClass<TypeXform>::Class*)this), instance);
   default:
      std::cout << "AlembicNode::enter: Ignore node \"" << path() << "\" (Unhandled type)" << std::endl;
      return ContinueVisit;
   }
}

template <class Visitor>
void AlembicNode::leave(Visitor &visitor, AlembicNode *instance)
{
   switch (mType)
   {
   case TypeGeneric:
      visitor.leave(*((typename TypeToClass<TypeGeneric>::Class*)this), instance);
      break;
   case TypeMesh:
      visitor.leave(*((typename TypeToClass<TypeMesh>::Class*)this), instance);
      break;
   case TypeSubD:
      visitor.leave(*((typename TypeToClass<TypeSubD>::Class*)this), instance);
      break;
   case TypePoints:
      visitor.leave(*((typename TypeToClass<TypePoints>::Class*)this), instance);
      break;
   case TypeCurves:
      visitor.leave(*((typename TypeToClass<TypeCurves>::Class*)this), instance);
      break;
   case TypeNuPatch:
      visitor.leave(*((typename TypeToClass<TypeNuPatch>::Class*)this), instance);
      break;
   case TypeXform:
      visitor.leave(*((typename TypeToClass<TypeXform>::Class*)this), instance);
      break;
   default:
      std::cout << "AlembicNode::leave: Ignore node \"" << path() << "\" (Unhandled type)" << std::endl;
   }
}

template <class Visitor>
bool AlembicNode::visit(VisitMode mode, AlembicScene *scene, Visitor &visitor)
{
   VisitReturn rv;
   
   if (mode == VisitFilteredFlat)
   {
      // Should never reach here from AlembicScene::visit
      rv = enter(visitor);
      
      leave(visitor);
      
      return (rv != StopVisit);
   }
   else if (mode == VisitBreadthFirst)
   {
      // Don't need to call enter/leave or check filtered status on current node (called by parent's visit)
      
      std::set<AlembicNode*> skipNodes;
      
      for (Array::iterator it=beginChild(); it!=endChild(); ++it)
      {
         if (!scene->isFiltered(*it))
         {
            skipNodes.insert(*it);
         }
         else
         {
            rv = (*it)->enter(visitor);
            (*it)->leave(visitor);
            
            if (rv == StopVisit)
            {
               return false;
            }
            else if (rv == DontVisitChildren)
            {
               skipNodes.insert(*it);
            }
         }
      }
      
      for (Array::iterator it=mChildren.begin(); it!=mChildren.end(); ++it)
      {
         if (skipNodes.find(*it) == skipNodes.end())
         {
            if (!(*it)->visit(mode, scene, visitor))
            {
               return false;
            }
         }
      }
   }
   else
   {
      if (scene->isFiltered(this))
      {
         rv = enter(visitor);
         
         if (rv == StopVisit)
         {
            leave(visitor);
            return false;
         }
         else if (rv != DontVisitChildren)
         {
            for (Array::iterator it=beginChild(); it!=endChild(); ++it)
            {
               if (!(*it)->visit(mode, scene, visitor))
               {
                  leave(visitor);
                  return false;
               }
            }
         }
         
         leave(visitor);
      }
   }
   
   return true;
}

template <class Visitor>
bool AlembicScene::visit(VisitMode mode, Visitor &visitor)
{
   VisitReturn rv;
   
   if (mode == VisitFilteredFlat)
   {
      std::set<AlembicNode*> skipChildrenOf;
      
      for (Set::iterator it=beginFiltered(); it!=endFiltered(); ++it)
      {
         bool skip = false;
         
         for (std::set<AlembicNode*>::iterator pit=skipChildrenOf.begin(); pit!=skipChildrenOf.end(); ++pit)
         {
            if ((*it)->hasAncestor(*pit))
            {
               skip = true;
               break;
            }
         }
         
         if (!skip)
         {
            rv = (*it)->enter(visitor);
            (*it)->leave(visitor);
            
            if (rv == StopVisit)
            {
               return false;
            }
            else if (rv == DontVisitChildren)
            {
               skipChildrenOf.insert(*it);
            }
         }
      }
   }
   else
   {
      if (mode == VisitBreadthFirst)
      {
         return AlembicNode::visit(VisitBreadthFirst, this, visitor);
         
         /*
         std::set<AlembicNode*> skipNodes;
         
         for (Array::iterator it=beginChild(); it!=endChild(); ++it)
         {
            if (!isFiltered(*it))
            {
               skipNodes.insert(*it);
            }
            else
            {
               rv = (*it)->enter(visitor);
               (*it)->leave(visitor);
               
               if (!rv == StopVisit)
               {
                  return false;
               }
               else if (rv == DontVisitChildren)
               {
                  skipNodes.insert(*it);
               }
            }
         }
         
         for (Array::iterator it=beginChild(); it!=endChild(); ++it)
         {
            if (skipNodes.find(*it) == skipNodes.end())
            {
               if (!(*it)->visit(mode, this, visitor))
               {
                  return false;
               }
            }
         }
         */
      }
      else
      {
         for (Array::iterator it=beginChild(); it!=endChild(); ++it)
         {
            if (!(*it)->visit(mode, this, visitor))
            {
               return false;
            }
         }
      }
   }
   
   return true;
}

#endif
