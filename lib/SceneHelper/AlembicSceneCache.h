#ifndef SCENEHELPER_ALEMBICSCENECACHE_H_
#define SCENEHELPER_ALEMBICSCENECACHE_H_

#include "AlembicScene.h"
#include <Alembic/AbcCoreFactory/All.h>
#include <map>
#include <string>

class AlembicSceneCache
{
public:
   
   static AlembicScene* Ref(const std::string &filepath);
   static bool Unref(AlembicScene *scene);
   
private:
   
   static AlembicSceneCache& Instance();
   
   AlembicSceneCache();
   ~AlembicSceneCache();
   
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
   
   std::map<std::string, CacheEntry> mScenes;
};

#endif
