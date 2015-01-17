#ifndef SCENEHELPER_ALEMBICSCENEFILTER_H_
#define SCENEHELPER_ALEMBICSCENEFILTER_H_

#include <string>
#include <set>
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
   
   bool isIncluded(const AlembicNode *node) const;
   bool isExcluded(const AlembicNode *node) const;

protected:
   
   std::string mIncludeFilterStr;
   std::string mExcludeFilterStr;
   regex_t *mIncludeFilter;
   regex_t *mExcludeFilter;
   regex_t mIncludeFilter_;
   regex_t mExcludeFilter_;
};


#endif