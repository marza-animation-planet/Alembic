#ifndef SCENEHELPER_ALEMBICSCENEFILTER_H_
#define SCENEHELPER_ALEMBICSCENEFILTER_H_

#include <Alembic/AbcGeom/All.h>
#include <string>
#include <map>
#include <vector>
#include <regex.h>

class AlembicNode;

class AlembicSceneFilter
{
public:
   
   AlembicSceneFilter();
   AlembicSceneFilter(const std::string &incl, const std::string &excl);
   AlembicSceneFilter(const AlembicSceneFilter &rhs);
   ~AlembicSceneFilter();
   
   AlembicSceneFilter& operator=(const AlembicSceneFilter &rhs);
   
   void set(const std::string &incl, const std::string &excl);
   void reset();
   bool isSet() const;
   
   bool isIncluded(const char *path) const;
   bool isIncluded(const AlembicNode *node) const;
   bool isExcluded(const char *path) const;
   bool isExcluded(const AlembicNode *node) const;
   
   bool keep(Alembic::Abc::IObject iObj) const;
   
   inline const std::string& includeExpression() const { return mIncludeFilterStr; }
   inline const std::string& excludeExpression() const { return mExcludeFilterStr; }

protected:
   
   std::string mIncludeFilterStr;
   std::string mExcludeFilterStr;
   regex_t *mIncludeFilters;
   size_t mIncludeFilterCount;
   regex_t *mExcludeFilters;
   size_t mExcludeFilterCount;
   mutable std::map<std::string, bool> mCache;
};


#endif