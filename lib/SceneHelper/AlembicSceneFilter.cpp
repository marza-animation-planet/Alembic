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
   if (incl != mIncludeFilterStr)
   {
      if (mIncludeFilter)
      {
         regfree(mIncludeFilter);
         mIncludeFilter = 0;
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
         }
      }
   }
   
   if (excl != mExcludeFilterStr)
   {
      if (mExcludeFilter)
      {
         regfree(mExcludeFilter);
         mExcludeFilter = 0;
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
         }
      }
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
}

bool AlembicSceneFilter::isSet() const
{
   return (mIncludeFilter || mExcludeFilter);
}

bool AlembicSceneFilter::isIncluded(const char *path) const
{
   return (path && (!mIncludeFilter || regexec(mIncludeFilter, path, 0, NULL, 0) == 0));
}

bool AlembicSceneFilter::isExcluded(const char *path) const
{
   return (!path || (mExcludeFilter && regexec(mExcludeFilter, path, 0, NULL, 0) == 0));
}


bool AlembicSceneFilter::isIncluded(const AlembicNode *node) const
{
   return (node && (!mIncludeFilter || regexec(mIncludeFilter, node->path().c_str(), 0, NULL, 0) == 0));
}

bool AlembicSceneFilter::isExcluded(const AlembicNode *node) const
{
   return (!node || (mExcludeFilter && regexec(mExcludeFilter, node->path().c_str(), 0, NULL, 0) == 0));
}
