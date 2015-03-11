#include "AlembicNode.h"
#include "AlembicScene.h"

static std::string gsEmptyString("");

// ---

AlembicNode::AlembicNode()
   : mType(AlembicNode::TypeGeneric)
   , mParent(0)
   , mMasterPath("")
   , mMaster(0)
   , mInstanceNumber(0)
   , mInheritsTransform(true)
   , mVisible(true)
{
   mSelfBounds.makeEmpty();
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
}

AlembicNode::AlembicNode(const AlembicNode &rhs)
   : mIObj(rhs.mIObj)
   , mType(rhs.mType)
   , mParent(0)
   , mMasterPath(rhs.mMasterPath)
   , mMaster(0)
   , mInstanceNumber(0)
   , mBoundsProp(rhs.mBoundsProp)
   , mInheritsTransform(true)
   , mLocatorProp(rhs.mLocatorProp)
   , mVisible(true)
{
   if (rhs.isLocator())
   {
      mSelfBounds.makeInfinite();
   }
   else
   {
      mSelfBounds.makeEmpty();
   }
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
}

AlembicNode::AlembicNode(const AlembicNode &rhs, AlembicNode *parent)
   : mIObj(rhs.mIObj)
   , mType(rhs.mType)
   , mParent(parent)
   , mMasterPath(rhs.mMasterPath)
   , mMaster(0)
   , mInstanceNumber(0)
   , mBoundsProp(rhs.mBoundsProp)
   , mInheritsTransform(true)
   , mLocatorProp(rhs.mLocatorProp)
   , mVisible(true)
{
   if (rhs.isLocator())
   {
      mSelfBounds.makeInfinite();
   }
   else
   {
      mSelfBounds.makeEmpty();
   }
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
   
   size_t numChildren = rhs.childCount();
   
   mChildren.reserve(numChildren);
   
   for (size_t i=0; i<numChildren; ++i)
   {
      AlembicNode *c = rhs.child(i)->clone(this);
      if (c)
      {
         mChildren.push_back(c);
         mChildByName[c->name()] = c;
      }
   }
}

AlembicNode::AlembicNode(const AlembicNode &rhs, const AlembicSceneFilter &filter, AlembicNode *parent)
   : mIObj(rhs.mIObj)
   , mType(rhs.mType)
   , mParent(parent)
   , mMasterPath(rhs.mMasterPath)
   , mMaster(0)
   , mInstanceNumber(0)
   , mBoundsProp(rhs.mBoundsProp)
   , mInheritsTransform(true)
   , mLocatorProp(rhs.mLocatorProp)
   , mVisible(true)
{
   if (rhs.isLocator())
   {
      mSelfBounds.makeInfinite();
   }
   else
   {
      mSelfBounds.makeEmpty();
   }
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
   
   size_t numChildren = rhs.childCount();
   
   mChildren.reserve(numChildren);
   
   for (size_t i=0; i<numChildren; ++i)
   {
      if (filter.isExcluded(rhs.child(i)))
      {
         continue;
      }
      
      AlembicNode *c = FilteredWrap(rhs.child(i)->object(), filter, this);
      
      if (c)
      {
         mChildren.push_back(c);
         mChildByName[c->name()] = c;
      }
   }
}

AlembicNode::AlembicNode(Alembic::Abc::IObject iObj)
   : mIObj(iObj)
   , mType(AlembicNode::TypeGeneric)
   , mParent(0)
   , mMasterPath("")
   , mMaster(0)
   , mInstanceNumber(0)
   , mInheritsTransform(true)
   , mVisible(true)
{
   mSelfBounds.makeEmpty();
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
}

AlembicNode::AlembicNode(Alembic::Abc::IObject iObj, AlembicNode *parent)
   : mIObj(iObj)
   , mType(AlembicNode::TypeGeneric)
   , mParent(parent)
   , mMasterPath("")
   , mMaster(0)
   , mInstanceNumber(0)
   , mInheritsTransform(true)
   , mVisible(true)
{
   mSelfBounds.makeEmpty();
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
   
   if (mIObj.valid())
   {
      if (mIObj.isInstanceRoot())
      {
         mMasterPath = mIObj.instanceSourcePath();
      }
      else if (mIObj.isInstanceDescendant())
      {
         mMasterPath = mIObj.getHeader().getFullName();
      }
      
      size_t numChildren = (mIObj.valid() ? mIObj.getNumChildren() : 0);
      
      mChildren.reserve(numChildren);
      
      for (size_t i=0; i<numChildren; ++i)
      {
         AlembicNode *c = Wrap(mIObj.getChild(i), this);
         if (c)
         {
            mChildren.push_back(c);
            mChildByName[c->name()] = c;
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "[AlembicNode] Failed to clone iObj child[" << i << "] (" << mIObj.getChild(i).getName() << ")" << std::endl;
            #endif
         }
      }
   }
}

AlembicNode::AlembicNode(Alembic::Abc::IObject iObj, const AlembicSceneFilter &filter, AlembicNode *parent)
   : mIObj(iObj)
   , mType(AlembicNode::TypeGeneric)
   , mParent(parent)
   , mMasterPath("")
   , mMaster(0)
   , mInstanceNumber(0)
   , mInheritsTransform(true)
   , mVisible(true)
{
   mSelfBounds.makeEmpty();
   mChildBounds.makeEmpty();
   mSelfMatrix.makeIdentity();
   mWorldMatrix.makeIdentity();
   mLocatorPosition.setValue(0, 0, 0);
   mLocatorScale.setValue(1, 1, 1);
   
   if (mIObj.valid())
   {
      if (mIObj.isInstanceRoot())
      {
         mMasterPath = mIObj.instanceSourcePath();
      }
      else if (mIObj.isInstanceDescendant())
      {
         mMasterPath = mIObj.getHeader().getFullName();
      }
      
      size_t numChildren = (mIObj.valid() ? mIObj.getNumChildren() : 0);
      
      mChildren.reserve(numChildren);
      
      for (size_t i=0; i<numChildren; ++i)
      {
         //if (filter.isExcluded(mIObj.getFullName().c_str()))
         if (filter.isExcluded(mIObj.getChild(i).getFullName().c_str()))
         {
            continue;
         }
         
         AlembicNode *c = FilteredWrap(mIObj.getChild(i), filter, this);
         
         if (c)
         {
            mChildren.push_back(c);
            mChildByName[c->name()] = c;
         }
      }
   }
}

AlembicNode::~AlembicNode()
{
   while (mInstances.size() > 0)
   {
      AlembicNode *i = mInstances.back();
      mInstances.pop_back();
      i->resetMaster();
   }
   
   if (mMaster)
   {
      mMaster->removeInstance(this);
      mMaster = 0;
   }
   
   mChildByName.clear();
   
   while (mChildren.size() > 0)
   {
      AlembicNode *c = mChildren.back();
      mChildren.pop_back();
      delete c;
   }
   
   mParent = 0;
}

AlembicNode* AlembicNode::clone(AlembicNode *parent) const
{
   return new AlembicNode(*this, parent);
}

AlembicNode* AlembicNode::selfClone() const
{
   return new AlembicNode(*this);
}

AlembicNode* AlembicNode::filteredClone(const AlembicSceneFilter &filter, AlembicNode *parent) const
{
   AlembicNode *rv = 0;
   
   if (!filter.isExcluded(this))
   {
      rv = new AlembicNode(*this, filter, parent);
      
      bool isLeaf = (type() >= TypeMesh && type() <= TypeCurves);
      
      //if (rv && rv->childCount() == 0 && !filter.isIncluded(rv))
      if (rv && rv->childCount() == 0 && !filter.isIncluded(rv, !isLeaf))
      {
         delete rv;
         rv = 0;
      }
   }
   
   return rv;
}

void AlembicNode::addChild(AlembicNode *n)
{
   if (n)
   {
      mChildren.push_back(n);
      mChildByName[n->name()] = n;
      
      n->mParent = this;
   }
}

AlembicNode::NodeType AlembicNode::type() const
{
   return mType;
}

const char* AlembicNode::typeName() const
{
   static const char* sTypeNames[] = 
   {
      "generic",
      "mesh",
      "subd",
      "points",
      "nupatch",
      "curves",
      "xform"
   };
   
   if (mType < 0 || mType >= LastType)
   {
      return "";
   }
   else
   {
      return sTypeNames[mType];
   }
}

bool AlembicNode::isValid() const
{
   return mIObj.valid();
}

const std::string& AlembicNode::name() const
{
   return (mIObj.valid() ? mIObj.getName() : gsEmptyString);
}

const std::string& AlembicNode::path() const
{
   return (mIObj.valid() ? mIObj.getFullName() : gsEmptyString);
}

std::string AlembicNode::partialPath() const
{
   std::string pp = name();
   
   if (this->isInstance() || this->numInstances() > 0)
   {
      if (mParent)
      {
         pp = mParent->partialPath() + "/" + pp;
      }
   }
   
   return pp;
}

std::string AlembicNode::formatName(const char *prefix) const
{
   if (!prefix)
   {
      return name();
   }
   else
   {
      return (prefix + name());
   }
}

void AlembicNode::formatPath(std::string &path, const char *prefix, PrefixType type, char sep) const
{
   if (prefix)
   {
      if (type == GlobalPrefix)
      {
         path = prefix + path;
      }
      else
      {
         std::string p = path;
         std::string ssep(1, sep);
         
         path = "";
         
         size_t p0 = 0;
         size_t p1 = p.find('/', p0);
         
         while (p1 != std::string::npos)
         {
            path += prefix + p.substr(p0, p1-p0) + ssep;
            p0 = p1 + 1;
            p1 = p.find('/', p0);
         }
         
         if (p0 < p.length())
         {
            path += prefix + p.substr(p0);
         }
         
         // set sep to '/' to avoid replace loop
         sep = '/';
      }
   }
   
   if (sep != '/')
   {
      size_t p = path.find('/');
      while (p != std::string::npos)
      {
         path[p] = sep;
         p = path.find('/', p);
      }
   }
}

std::string AlembicNode::formatPath(const char *prefix, PrefixType type, char sep) const
{
   std::string pp = path();
   formatPath(pp, prefix, type, sep);
   return pp;
}

std::string AlembicNode::formatPartialPath(const char *prefix, PrefixType type, char sep) const
{
   std::string pp = partialPath();
   formatPath(pp, prefix, type, sep);
   return pp;
}

AlembicNode* AlembicNode::parent()
{
   return mParent;
}

const AlembicNode* AlembicNode::parent() const
{
   return mParent;
}

bool AlembicNode::hasAncestor(const AlembicNode *node) const
{
   return (this == node || (mParent && mParent->hasAncestor(node)));
}

size_t AlembicNode::childCount() const
{
   return mChildren.size();
}

AlembicNode* AlembicNode::child(size_t i)
{
   return (i < mChildren.size() ? mChildren[i] : 0);
}

const AlembicNode* AlembicNode::child(size_t i) const
{
   return (i < mChildren.size() ? mChildren[i] : 0);
}

AlembicNode* AlembicNode::child(const char *name)
{
   Map::iterator it = mChildByName.find(name);
   return (it != mChildByName.end() ? it->second : 0);
}

const AlembicNode* AlembicNode::child(const char *name) const
{
   Map::const_iterator it = mChildByName.find(name);
   return (it != mChildByName.end() ? it->second : 0);
}

AlembicNode::Array::iterator AlembicNode::beginChild()
{
   return mChildren.begin();
}

AlembicNode::Array::iterator AlembicNode::endChild()
{
   return mChildren.end();
}

AlembicNode::Array::const_iterator AlembicNode::beginChild() const
{
   return mChildren.begin();
}

AlembicNode::Array::const_iterator AlembicNode::endChild() const
{
   return mChildren.end();
}

void AlembicNode::setSelfBounds(const Alembic::Abc::Box3d &b)
{
   if (!isInstance())
   {
      mSelfBounds = b;
   }
}

void AlembicNode::setSelfMatrix(const Alembic::Abc::M44d &m)
{
   if (!isInstance())
   {
      mSelfMatrix = m;
   }
}

void AlembicNode::setInheritsTransform(bool on)
{
   if (!isInstance())
   {
      mInheritsTransform = on;
   }
}

void AlembicNode::setVisible(bool on)
{
   if (!isInstance())
   {
      mVisible = on;
   }
}

bool AlembicNode::isVisible(bool inherited) const
{
   if (mMaster)
   {
      return mMaster->isVisible();
   }
   else if (mVisible)
   {
      return ((inherited && mParent) ? mParent->isVisible() : true);
   }
   else
   {
      return false;
   }
}

void AlembicNode::reset()
{
   Alembic::Abc::Box3d emptyBox;
   Alembic::Abc::M44d identityMatrix;
   Alembic::Abc::V3d zero(0, 0, 0);
   Alembic::Abc::V3d one(1, 1, 1);
   
   setSelfBounds(emptyBox);
   setSelfMatrix(identityMatrix);
   setInheritsTransform(true);
   setVisible(false);
   setLocatorPosition(zero);
   setLocatorScale(one);
   
   resetWorldMatrix();
   resetChildBounds();
}

void AlembicNode::resetWorldMatrix()
{
   mWorldMatrix.makeIdentity();
}

void AlembicNode::resetChildBounds()
{
   mChildBounds.makeEmpty();
}

void AlembicNode::updateWorldMatrix()
{
   if (mParent && mInheritsTransform)
   {
      mWorldMatrix = selfMatrix() * mParent->worldMatrix();
   }
   else
   {
      mWorldMatrix = selfMatrix();
   }
}

void AlembicNode::updateChildBounds()
{
   mChildBounds.makeEmpty();
   
   if (childCount() > 0)
   {
      Alembic::Abc::Box3d bb;
      
      mSelfBounds.makeEmpty();
      
      bool allInfinite = true;
      
      for (Array::iterator it = beginChild(); it != endChild(); ++it)
      {
         bb = (*it)->childBounds();
         if (!bb.isInfinite())
         {
            mChildBounds.extendBy(bb);
            allInfinite = false;
         }
         
         bb = (*it)->selfBounds();
         if (!bb.isInfinite())
         {
            mSelfBounds.extendBy(bb);
         }
      }
      
      // Only make bounds infinite if all children have inifnite bounds
      if (allInfinite)
      {
         mSelfBounds.makeInfinite();
         mChildBounds.makeInfinite();
      }
   }
   else
   {
      Alembic::Abc::Box3d bb = selfBounds();
      
      if (bb.isInfinite())
      {
         mChildBounds.makeInfinite();
      }
      else if (!bb.isEmpty())
      {
         Alembic::Abc::V3d bbmin = bb.min;
         Alembic::Abc::V3d bbmax = bb.max;
         Alembic::Abc::M44d wm = worldMatrix(); // already take inheritsTransfor into account
         
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmin.x, bbmin.y, bbmin.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmin.x, bbmin.y, bbmax.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmin.x, bbmax.y, bbmin.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmin.x, bbmax.y, bbmax.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmax.x, bbmin.y, bbmin.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmax.x, bbmin.y, bbmax.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmax.x, bbmax.y, bbmin.z) * wm);
         mChildBounds.extendBy(Alembic::Abc::V3d(bbmax.x, bbmax.y, bbmax.z) * wm);
      }
   }
}

bool AlembicNode::isInstance() const
{
   return (mMasterPath.length() > 0);
}

size_t AlembicNode::instanceNumber() const
{
   return mInstanceNumber;
}

void AlembicNode::addInstance(AlembicNode *node)
{
   Array::iterator it = std::find(mInstances.begin(), mInstances.end(), node);
   if (it == mInstances.end())
   {
      mInstances.push_back(node);
      node->mInstanceNumber = mInstances.size();
   }
}

void AlembicNode::removeInstance(AlembicNode *node)
{
   Array::iterator it = std::find(mInstances.begin(), mInstances.end(), node);
   if (it != mInstances.end())
   {
      Array::iterator rit = it;
      ++rit;
      
      for (; rit != mInstances.end(); ++rit)
      {
         (*rit)->mInstanceNumber -= 1;
      }
      
      mInstances.erase(it);
   }
}

void AlembicNode::resetMaster()
{
   mMaster = 0;
}

AlembicNode* AlembicNode::find(const char *path)
{
   if (!path)
   {
      return 0;
   }
   
   const char *p = strchr(path, '/');
   
   if (p)
   {
      std::string n(path, p - path);
      AlembicNode *c = child(n);
      return (c ? c->find(p + 1) : 0);
   }
   else
   {
      return child(path);
   }
}

const AlembicNode* AlembicNode::find(const char *path) const
{
   if (!path)
   {
      return 0;
   }
   
   const char *p = strchr(path, '/');
   
   if (p)
   {
      std::string n(path, p - path);
      const AlembicNode *c = child(n);
      return (c ? c->find(p + 1) : 0);
   }
   else
   {
      return child(path);
   }
}

void AlembicNode::resolveInstances(AlembicScene *scene)
{
   if (isInstance() && !mMaster)
   {
      mMaster = scene->find(mMasterPath.c_str());
      
      if (mMaster)
      {
         mMaster->addInstance(this);
      }
      else
      {
         // Un-instance node
         #ifdef _DEBUG
         std::cout << "[AlembicNode] resolveInstances: reset master path" << std::endl;
         #endif
         mMasterPath = "";
      }
   }
   
   for (Array::iterator it=mChildren.begin(); it!=mChildren.end(); ++it)
   {
      (*it)->resolveInstances(scene);
   }
}

size_t AlembicNode::numInstances() const
{
   return mInstances.size();
}

const std::string& AlembicNode::masterPath() const
{
   return mMasterPath;
}

AlembicNode* AlembicNode::master()
{
   return mMaster;
}

const AlembicNode* AlembicNode::master() const
{
   return mMaster;
}

AlembicNode* AlembicNode::instance(size_t i)
{
   return (i < mInstances.size() ? mInstances[i] : 0);
}

const AlembicNode* AlembicNode::instance(size_t i) const
{
   return (i < mInstances.size() ? mInstances[i] : 0);
}
   
Alembic::Abc::IObject AlembicNode::object() const
{
   return mIObj;
}

void AlembicNode::setType(AlembicNode::NodeType nt)
{
   mLocatorProp.reset();
   mBoundsProp.reset();
   
   if (mIObj.valid())
   {
      const Alembic::AbcCoreAbstract::PropertyHeader *header;
      
      Alembic::Abc::ICompoundProperty props = mIObj.getProperties();
         
      if (props.valid())
      {
         if (nt == TypeXform)
         {
            header = props.getPropertyHeader("locator");

            if (header && ILocatorProperty::matches(*header))
            {
               mLocatorProp = ILocatorProperty(props, "locator");
               
               mSelfBounds.makeInfinite();
            }
         }
         else if (props.getPropertyHeader(".geom") != 0)
         {
            Alembic::Abc::ICompoundProperty geom(props, ".geom");
            
            if (geom.valid())
            {
               header = geom.getPropertyHeader(".selfBnds");
               
               if (header && Alembic::Abc::IBox3dProperty::matches(*header))
               {
                  mBoundsProp = Alembic::Abc::IBox3dProperty(geom, ".selfBnds");
               }
               else
               {
                  header = geom.getPropertyHeader(".childBnds");
                  
                  if (header && Alembic::Abc::IBox3dProperty::matches(*header))
                  {
                     mBoundsProp = Alembic::Abc::IBox3dProperty(geom, ".childBnds");
                  }
               }
            }
         }
      }
   }
   
   mType = nt;
}

bool AlembicNode::isLocator() const
{
   if (mMaster)
   {
      return mMaster->isLocator();
   }
   else
   {
      return (mType == TypeXform && mLocatorProp.valid());
   }
}

const ILocatorProperty& AlembicNode::locatorProperty() const
{
   return (mMaster ? mMaster->locatorProperty() : mLocatorProp);
}

void AlembicNode::setLocatorPosition(const Alembic::Abc::V3d &p)
{
   if (!isInstance() && isLocator())
   {
      mLocatorPosition = p;
   }
}

void AlembicNode::setLocatorScale(const Alembic::Abc::V3d &s)
{
   if (!isInstance() && isLocator())
   {
      mLocatorScale = s;
   }
}

const Alembic::Abc::IBox3dProperty& AlembicNode::boundsProperty() const
{
   return (mMaster ? mMaster->boundsProperty() : mBoundsProp);
}

// ---

AlembicNode* AlembicNode::Wrap(Alembic::Abc::IObject iObj, AlembicNode *iParent)
{
   if (!iObj.valid())
   {
      return 0;
   }
   
   if (iObj.isInstanceDescendant())
   {
      return new AlembicNode(iObj, iParent);
   }
   else if (Alembic::AbcGeom::IXform::matches(iObj.getHeader()))
   {
      return new AlembicXform(iObj, iParent);
   }
   else if (Alembic::AbcGeom::IPoints::matches(iObj.getHeader()))
   {
      return new AlembicPoints(iObj, iParent);
   }
   else if (Alembic::AbcGeom::IPolyMesh::matches(iObj.getHeader()))
   {
      return new AlembicMesh(iObj, iParent);
   }
   else if (Alembic::AbcGeom::ISubD::matches(iObj.getHeader()))
   {
      return new AlembicSubD(iObj, iParent);
   }
   else if (Alembic::AbcGeom::INuPatch::matches(iObj.getHeader()))
   {
      return new AlembicNuPatch(iObj, iParent);
   }
   else if (Alembic::AbcGeom::ICurves::matches(iObj.getHeader()))
   {
      return new AlembicCurves(iObj, iParent);
   }
   else
   {
      return 0;
   }
}

AlembicNode* AlembicNode::WrapSingle(Alembic::Abc::IObject iObj)
{
   if (!iObj.valid())
   {
      return 0;
   }
   else if (Alembic::AbcGeom::IXform::matches(iObj.getHeader()))
   {
      return new AlembicXform(iObj);
   }
   else if (Alembic::AbcGeom::IPoints::matches(iObj.getHeader()))
   {
      return new AlembicPoints(iObj);
   }
   else if (Alembic::AbcGeom::IPolyMesh::matches(iObj.getHeader()))
   {
      return new AlembicMesh(iObj);
   }
   else if (Alembic::AbcGeom::ISubD::matches(iObj.getHeader()))
   {
      return new AlembicSubD(iObj);
   }
   else if (Alembic::AbcGeom::INuPatch::matches(iObj.getHeader()))
   {
      return new AlembicNuPatch(iObj);
   }
   else if (Alembic::AbcGeom::ICurves::matches(iObj.getHeader()))
   {
      return new AlembicCurves(iObj);
   }
   else
   {
      return 0;
   }
}

AlembicNode* AlembicNode::FilteredWrap(Alembic::Abc::IObject iObj, const AlembicSceneFilter &filter, AlembicNode *iParent)
{
   if (!iObj.valid())
   {
      return 0;
   }
   
   AlembicNode *rv = 0;
   
   if (iObj.isInstanceDescendant())
   {
      Alembic::Abc::IObject p = iObj;
      
      while (p.valid() && !p.isInstanceRoot())
      {
         p = p.getParent();
      }
      
      if (p.valid())
      {
         Alembic::Abc::IObject iRoot(p.getPtr(), Alembic::Abc::kWrapExisting);
         
         if (filter.keep(iRoot))
         {
            // Note: instance root parent will be included if any of its children is incuded
            return new AlembicNode(iObj, filter, iParent);
         }
      }
      
      // seems not to get the right obj then...
   }
   
   bool isLeaf = true;
   
   if (Alembic::AbcGeom::IXform::matches(iObj.getHeader()))
   {
      isLeaf = false;
      rv = new AlembicXform(iObj, filter, iParent);
   }
   else if (Alembic::AbcGeom::IPoints::matches(iObj.getHeader()))
   {
      rv = new AlembicPoints(iObj, filter, iParent);
   }
   else if (Alembic::AbcGeom::IPolyMesh::matches(iObj.getHeader()))
   {
      rv = new AlembicMesh(iObj, filter, iParent);
   }
   else if (Alembic::AbcGeom::ISubD::matches(iObj.getHeader()))
   {
      rv = new AlembicSubD(iObj, filter, iParent);
   }
   else if (Alembic::AbcGeom::INuPatch::matches(iObj.getHeader()))
   {
      rv = new AlembicNuPatch(iObj, filter, iParent);
   }
   else if (Alembic::AbcGeom::ICurves::matches(iObj.getHeader()))
   {
      rv = new AlembicCurves(iObj, filter, iParent);
   }
   
   //if (rv && rv->childCount() == 0 && !filter.isIncluded(rv))
   if (rv && rv->childCount() == 0 && !filter.isIncluded(rv, !isLeaf))
   {
      delete rv;
      rv = 0;
   }
   
   return rv;
}