#ifndef SCENEHELPER_ALEMBICNODE_H_
#define SCENEHELPER_ALEMBICNODE_H_

#include "LocatorProperty.h"
#include "SampleUtils.h"
#include "AlembicSceneFilter.h"
#include <Alembic/AbcGeom/All.h>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

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
      VisitBreadthFirst
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
   
   virtual AlembicNode* clone(AlembicNode *parent=0) const;
   virtual AlembicNode* filteredClone(const AlembicSceneFilter &filter, AlembicNode *parent=0) const;
   
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
   const ILocatorProperty& locatorProperty() const;
   void setLocatorPosition(const Alembic::Abc::V3d &p);
   void setLocatorScale(const Alembic::Abc::V3d &s);
   inline const Alembic::Abc::V3d& locatorPosition() const { return (mMaster ? mMaster->locatorPosition() : mLocatorPosition); }
   inline const Alembic::Abc::V3d& locatorScale() const { return (mMaster ? mMaster->locatorScale() : mLocatorScale); }

   const Alembic::Abc::IBox3dProperty& boundsProperty() const;
   inline const Alembic::Abc::Box3d selfBounds() const { return (mMaster ? mMaster->selfBounds() : mSelfBounds); }
   void setSelfBounds(const Alembic::Abc::Box3d &b);
   inline const Alembic::Abc::Box3d childBounds() const { return mChildBounds; }
   void updateChildBounds();
   void resetChildBounds();
   
   inline const Alembic::Abc::M44d selfMatrix() const { return (mMaster ? mMaster->selfMatrix() : mSelfMatrix); }
   void setSelfMatrix(const Alembic::Abc::M44d &m);
   inline const Alembic::Abc::M44d worldMatrix() const { return mWorldMatrix; }
   void updateWorldMatrix();
   void resetWorldMatrix();
   inline bool inheritsTransform() const { return (mMaster ? mMaster->inheritsTransform() : mInheritsTransform); }
   void setInheritsTransform(bool on);
   
   
   bool isVisible(bool inherited=false) const;
   void setVisible(bool on);
   
   virtual void reset();
   
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
   
   AlembicNode(const AlembicNode &rhs, AlembicNode *parent=0);
   AlembicNode(const AlembicNode &rhs, const AlembicSceneFilter &filter, AlembicNode *parent=0);

   void setType(NodeType nt);
   
   void addInstance(AlembicNode *node);
   void removeInstance(AlembicNode *node);
   void resetMaster();
   
   void formatPath(std::string &path, const char *prefix, PrefixType type, char separator) const;

protected:
   
   Alembic::Abc::IObject mIObj;
   
   NodeType mType;
   
   AlembicNode *mParent;
   
   Array mChildren;
   Map mChildByName;
   
   std::string mMasterPath;
   AlembicNode *mMaster;
   Array mInstances;
   size_t mInstanceNumber;
   
   Alembic::Abc::IBox3dProperty mBoundsProp;
   Alembic::Abc::Box3d mSelfBounds;
   Alembic::Abc::Box3d mChildBounds;
   
   bool mInheritsTransform;
   Alembic::Abc::M44d mSelfMatrix;
   Alembic::Abc::M44d mWorldMatrix;
   
   ILocatorProperty mLocatorProp;
   Alembic::Abc::V3d mLocatorPosition;
   Alembic::Abc::V3d mLocatorScale;
   
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

template <class IObject>
class AlembicNodeT : public AlembicNode
{
public:
   
   typedef AlembicNodeT<IObject> SelfType;
   typedef SampleSet<IObject> SampleSetType;
   
public:
   
   AlembicNodeT()
      : AlembicNode()
   {
      setType((NodeType) ClassToType<SelfType>::Type);
   }
   
   AlembicNodeT(IObject iObject, AlembicNode *iParent=0)
      : AlembicNode(iObject, iParent), mITypedObj(iObject)
   {
      setType((NodeType) ClassToType<SelfType>::Type);
   }
   
   virtual ~AlembicNodeT()
   {
   }
   
   virtual AlembicNode* filteredClone(const AlembicSceneFilter &filter, AlembicNode *parent=0) const
   {
      AlembicNode *rv = 0;
      
      if (!filter.isExcluded(this))
      {
         rv = new AlembicNodeT<IObject>(*this, filter, parent);
         
         if (rv && rv->childCount() == 0 && !filter.isIncluded(rv))
         {
            delete rv;
            rv = 0;
         }
      }
      
      return rv;
   }
   
   virtual AlembicNode* clone(AlembicNode *parent=0) const
   {
      return new AlembicNodeT<IObject>(*this, parent);
   }
   
   inline IObject typedObject() const
   {
      return mITypedObj;
   }
   
   inline SampleSetType& samples()
   {
      return mSamples;
   }
   
   inline const SampleSetType& samples() const
   {
      return mSamples;
   }
   
   template <class ISampledObject>
   bool sample(ISampledObject &target,
               double t0, double t1,
               TimeSampleList<ISampledObject> &samples,
               bool mergeSamples,
               bool *resampled=0)
   {
      bool upd = samples.update(target, t0, t1, mergeSamples);
      
      if (resampled)
      {
         *resampled = upd;
      }
      
      return samples.validRange(t0, t1);
   }
   
   bool sampleBounds(double t0, double t1, bool mergeSamples, bool *resampled=0)
   {
      if (type() == AlembicNode::TypeXform)
      {
         if (isLocator())
         {
            ILocatorProperty locatorProp = locatorProperty();
            
            if (locatorProp.valid())
            {
               return sample(locatorProp, t0, t1, mSamples.locatorSamples, mergeSamples, resampled);
            }
            else
            {
               return false;
            }
         }
         else
         {
            return sampleSchema(t0, t1, mergeSamples, resampled);
         }
      }
      else
      {
         Alembic::Abc::IBox3dProperty boundsProp = boundsProperty();
         
         if (boundsProp.valid())
         {
            return sample(boundsProp, t0, t1, mSamples.boundsSamples, mergeSamples, resampled);
         }
         else
         {
            return false;
         }
      }
   }
   
   bool sampleSchema(double t0, double t1, bool mergeSamples, bool *resampled=0)
   {
      return sample(mITypedObj.getSchema(), t0, t1, mSamples.schemaSamples, mergeSamples, resampled);
   }
   
   virtual void reset()
   {
      AlembicNode::reset();
      mSamples.clear();
   }
   
protected:
   
   AlembicNodeT(const AlembicNodeT<IObject> &rhs, AlembicNode *iParent=0)
      : AlembicNode(rhs, iParent), mITypedObj(rhs.mITypedObj)
   {
   }
   
   AlembicNodeT(const AlembicNodeT<IObject> &rhs, const AlembicSceneFilter &filter, AlembicNode *iParent=0)
      : AlembicNode(rhs, filter, iParent), mITypedObj(rhs.mITypedObj)
   {
   }
   
protected:
   
   IObject mITypedObj;
   SampleSetType mSamples;
   
};

// ---

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

#endif
