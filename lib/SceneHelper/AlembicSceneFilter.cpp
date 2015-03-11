#include "AlembicSceneFilter.h"
#include "AlembicScene.h"
#include "AlembicNode.h"

AlembicSceneFilter::AlembicSceneFilter()
   : mIncludeFilter(0)
   , mExcludeFilter(0)
{
}

AlembicSceneFilter::AlembicSceneFilter(const std::string &incl, const std::string &excl)
   : mIncludeFilter(0)
   , mExcludeFilter(0)
{
   set(incl, excl);
}

AlembicSceneFilter::AlembicSceneFilter(const AlembicSceneFilter &rhs)
   : mIncludeFilter(0)
   , mExcludeFilter(0)
{
   set(rhs.mIncludeFilterStr, rhs.mExcludeFilterStr);
}

AlembicSceneFilter::~AlembicSceneFilter()
{
   reset();
}

AlembicSceneFilter& AlembicSceneFilter::operator=(const AlembicSceneFilter &rhs)
{
   if (this != &rhs)
   {
      set(rhs.mIncludeFilterStr, rhs.mExcludeFilterStr);
   }
   return *this;
}

void AlembicSceneFilter::set(const std::string &incl, const std::string &excl)
{
   bool changed = false;
   
   if (incl != mIncludeFilterStr)
   {
      if (mIncludeFilter)
      {
         regfree(mIncludeFilter);
         mIncludeFilter = 0;
         changed = true;
      }
      
      mIncludeFilterStr = "";
      
      if (incl.length() > 0)
      {
         if (regcomp(&mIncludeFilter_, incl.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
         {
            std::cout << "[AlembicSceneFilter] Invalid expression: \"" << incl << "\"" << std::endl;
         }
         else
         {
            mIncludeFilterStr = incl;
            mIncludeFilter = &mIncludeFilter_;
            changed = true;
         }
      }
   }
   
   if (excl != mExcludeFilterStr)
   {
      if (mExcludeFilter)
      {
         regfree(mExcludeFilter);
         mExcludeFilter = 0;
         changed = true;
      }
      
      mExcludeFilterStr = "";
      
      if (excl.length() > 0)
      {
         if (regcomp(&mExcludeFilter_, excl.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
         {
            std::cout << "[AlembicSceneFilter] Invalid expression: \"" << excl << "\"" << std::endl;
         }
         else
         {
            mExcludeFilterStr = excl;
            mExcludeFilter = &mExcludeFilter_;
            changed = true;
         }
      }
   }
   
   if (changed)
   {
      mCache.clear();
   }
}

void AlembicSceneFilter::reset()
{
   if (mIncludeFilter)
   {
      regfree(mIncludeFilter);
      mIncludeFilter = 0;
      mIncludeFilterStr = "";
   }
   
   if (mExcludeFilter)
   {
      regfree(mExcludeFilter);
      mExcludeFilter = 0;
      mExcludeFilterStr = "";
   }
   
   mCache.clear();
}

bool AlembicSceneFilter::isSet() const
{
   return (mIncludeFilter || mExcludeFilter);
}

bool AlembicSceneFilter::keep(Alembic::Abc::IObject iObj) const
{
   if (!iObj.valid())
   {
      return false;
   }
   
   const std::string &path = iObj.getFullName();
   
   std::map<std::string, bool>::iterator it = mCache.find(path);
   
   if (it != mCache.end())
   {
      return it->second;
   }
   else
   {
      bool rv = false;
      
      if (!isExcluded(path.c_str()))
      {
         size_t n = iObj.getNumChildren();
         size_t c = 0;
         size_t fsc = 0;
         
         for (size_t i=0; i<n; ++i)
         {
            Alembic::Abc::IObject iChild = iObj.getChild(i);
            
            if (Alembic::AbcGeom::IFaceSet::matches(iChild.getMetaData()))
            {
               ++fsc;
            }
            
            if (keep(iChild))
            {
               ++c;
            }
         }
         
         bool isLeaf = (n == 0 || n == fsc);
         
         //rv = (c > 0 || isIncluded(path.c_str()));
         rv = (c > 0 || isIncluded(path.c_str(), !isLeaf));
      }
      
      mCache[path] = rv;
      
      return rv;
   }
}

bool AlembicSceneFilter::isIncluded(const char *path, bool strict) const
{
   if (!strict)
   {
      return (path && (!mIncludeFilter || regexec(mIncludeFilter, path, 0, NULL, 0) == 0));
   }
   else
   {
      return (path && mIncludeFilter && regexec(mIncludeFilter, path, 0, NULL, 0) == 0);
   }
}

bool AlembicSceneFilter::isExcluded(const char *path) const
{
   return (!path || (mExcludeFilter && regexec(mExcludeFilter, path, 0, NULL, 0) == 0));
}


bool AlembicSceneFilter::isIncluded(const AlembicNode *node, bool strict) const
{
   if (!strict)
   {
      return (node && (!mIncludeFilter || regexec(mIncludeFilter, node->path().c_str(), 0, NULL, 0) == 0));
   }
   else
   {
      return (node && mIncludeFilter && regexec(mIncludeFilter, node->path().c_str(), 0, NULL, 0) == 0);
   }
}

bool AlembicSceneFilter::isExcluded(const AlembicNode *node) const
{
   return (!node || (mExcludeFilter && regexec(mExcludeFilter, node->path().c_str(), 0, NULL, 0) == 0));
}
