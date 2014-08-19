#ifndef ABCSHAPE_SCENECACHE_H_
#define ABCSHAPE_SCENECACHE_H_

#include "common.h"

#include <Alembic/AbcCoreFactory/All.h>

//#include <maya/MMutexLock.h>

#include <map>
#include <string>

class SceneCache
{
public:
   
   static AlembicScene* Ref(const std::string &filepath);
   static bool Unref(AlembicScene *scene);
   
private:
   
   static SceneCache& Instance();
   
   SceneCache();
   ~SceneCache();
   
   std::string formatPath(const std::string &filepath);
   
   AlembicScene* ref(const std::string &filepath);
   bool unref(AlembicScene *scene);
   
   
   
private:
   
   struct CacheEntry
   {
      Alembic::Abc::IArchive archive;
      AlembicScene *master;
      int refcount;
   };
   
   //MMutexLock mMutex;
   std::map<std::string, CacheEntry> mScenes;
};


#endif
