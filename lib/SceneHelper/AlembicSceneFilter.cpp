#include "AlembicSceneFilter.h"
#include "AlembicScene.h"
#include "AlembicNode.h"

AlembicSceneFilter::AlembicSceneFilter()
   : mIncludeFilters(0)
   , mIncludeFilterCount(0)
   , mExcludeFilters(0)
   , mExcludeFilterCount(0)
{
}

AlembicSceneFilter::AlembicSceneFilter(const std::string &incl, const std::string &excl)
   : mIncludeFilters(0)
   , mIncludeFilterCount(0)
   , mExcludeFilters(0)
   , mExcludeFilterCount(0)
{
   set(incl, excl);
}

AlembicSceneFilter::AlembicSceneFilter(const AlembicSceneFilter &rhs)
   : mIncludeFilters(0)
   , mIncludeFilterCount(0)
   , mExcludeFilters(0)
   , mExcludeFilterCount(0)
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
      if (mIncludeFilters)
      {
         for (size_t i=0; i<mIncludeFilterCount; ++i)
         {
            regfree(&mIncludeFilters[i]);
         }
         delete[] mIncludeFilters;
         mIncludeFilters = 0;
         mIncludeFilterCount = 0;
         changed = true;
      }
      
      mIncludeFilterStr = "";
      
      if (incl.length() > 0)
      {
         // Split string by spaces
         std::vector<std::string> ilist;
         size_t p0 = 0;
         size_t p1 = incl.find_first_of(" \t", p0);
         while (p0 != std::string::npos)
         {
            std::string ss = (p1 == std::string::npos ? incl.substr(p0) : incl.substr(p0, p1 - p0));
            if (ss.length() > 0)
            {
               ilist.push_back(ss);
            }
            if (p1 != std::string::npos)
            {
               p0 = incl.find_first_not_of(" \t", p1);
               p1 = (p0 != std::string::npos ? incl.find_first_of(" \t", p0) : std::string::npos);
            }
            else
            {
               p0 = p1;
            }
         }

         if (ilist.size() > 0)
         {
            mIncludeFilters = new regex_t[ilist.size()];

            for (std::vector<std::string>::iterator it=ilist.begin(); it!=ilist.end(); ++it)
            {
               if (regcomp(&mIncludeFilters[mIncludeFilterCount], it->c_str(), REG_EXTENDED|REG_NOSUB) != 0)
               {
                  std::cout << "[AlembicSceneFilter] Invalid expression: \"" << *it << "\"" << std::endl;
               }
               else
               {
                  ++mIncludeFilterCount;
                  if (mIncludeFilterStr.length() > 0)
                  {
                     mIncludeFilterStr += " ";
                  }
                  mIncludeFilterStr += *it;
                  changed = true;
               }
            }

            if (mIncludeFilterCount == 0)
            {
               delete[] mIncludeFilters;
               mIncludeFilters = 0;
            }
         }
      }
   }
   
   if (excl != mExcludeFilterStr)
   {
      if (mExcludeFilters)
      {
         for (size_t i=0; i<mExcludeFilterCount; ++i)
         {
            regfree(&mExcludeFilters[i]);
         }
         delete[] mExcludeFilters;
         mExcludeFilters = 0;
         mExcludeFilterCount = 0;
         changed = true;
      }
      
      mExcludeFilterStr = "";
      
      if (excl.length() > 0)
      {
         // Split string by spaces
         std::vector<std::string> elist;
         size_t p0 = 0;
         size_t p1 = excl.find_first_of(" \t", p0);
         while (p0 != std::string::npos)
         {
            std::string ss = (p1 == std::string::npos ? excl.substr(p0) : excl.substr(p0, p1 - p0));
            if (ss.length() > 0)
            {
               elist.push_back(ss);
            }
            if (p1 != std::string::npos)
            {
               p0 = excl.find_first_not_of(" \t", p1);
               p1 = (p0 != std::string::npos ? excl.find_first_of(" \t", p0) : std::string::npos);
            }
            else
            {
               p0 = p1;
            }
         }

         if (elist.size() > 0)
         {
            mExcludeFilters = new regex_t[elist.size()];

            for (std::vector<std::string>::iterator it=elist.begin(); it!=elist.end(); ++it)
            {
               if (regcomp(&mExcludeFilters[mExcludeFilterCount], it->c_str(), REG_EXTENDED|REG_NOSUB) != 0)
               {
                  std::cout << "[AlembicSceneFilter] Invalid expression: \"" << *it << "\"" << std::endl;
               }
               else
               {
                  ++mExcludeFilterCount;
                  if (mExcludeFilterStr.length() > 0)
                  {
                     mExcludeFilterStr += " ";
                  }
                  mExcludeFilterStr += *it;
                  changed = true;
               }
            }

            if (mExcludeFilterCount == 0)
            {
               delete[] mExcludeFilters;
               mExcludeFilters = 0;
            }
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
   if (mIncludeFilters)
   {
      for (size_t i=0; i<mIncludeFilterCount; ++i)
      {
         regfree(&mIncludeFilters[i]);
      }
      delete[] mIncludeFilters;
      mIncludeFilters = 0;
      mIncludeFilterCount = 0;
      mIncludeFilterStr = "";
   }
   
   if (mExcludeFilters)
   {
      for (size_t i=0; i<mExcludeFilterCount; ++i)
      {
         regfree(&mExcludeFilters[i]);
      }
      delete[] mExcludeFilters;
      mExcludeFilters = 0;
      mExcludeFilterCount = 0;
      mExcludeFilterStr = "";
   }
   
   mCache.clear();
}

bool AlembicSceneFilter::isSet() const
{
   return (mIncludeFilterCount > 0 || mExcludeFilterCount > 0);
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
         
         for (size_t i=0; i<n; ++i)
         {
            if (keep(iObj.getChild(i)))
            {
               ++c;
            }
         }
         
         rv = (c > 0 || isIncluded(path.c_str()));
      }
      
      mCache[path] = rv;
      
      return rv;
   }
}

bool AlembicSceneFilter::isIncluded(const char *path) const
{
   if (path)
   {
      if (mIncludeFilterCount > 0)
      {
         for (size_t i=0; i<mIncludeFilterCount; ++i)
         {
            if (regexec(&mIncludeFilters[i], path, 0, NULL, 0) == 0)
            {
               return true;
            }
         }
         return false;
      }
      else
      {
         return true;
      }
   }
   else
   {
      return false;
   }
}

bool AlembicSceneFilter::isExcluded(const char *path) const
{
   if (path)
   {
      if (mExcludeFilterCount > 0)
      {
         for (size_t i=0; i<mExcludeFilterCount; ++i)
         {
            if (regexec(&mExcludeFilters[i], path, 0, NULL, 0) == 0)
            {
               return true;
            }
         }
         return false;
      }
      else
      {
         return false;
      }
   }
   else
   {
      return true;
   }
}


bool AlembicSceneFilter::isIncluded(const AlembicNode *node) const
{
   return isIncluded(node ? node->path().c_str() : NULL);
}

bool AlembicSceneFilter::isExcluded(const AlembicNode *node) const
{
   return isExcluded(node ? node->path().c_str() : NULL);
}
