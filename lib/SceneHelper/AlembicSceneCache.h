#ifndef SCENEHELPER_ALEMBICSCENECACHE_H_
#define SCENEHELPER_ALEMBICSCENECACHE_H_

#include "AlembicScene.h"
#include <Alembic/AbcCoreFactory/All.h>
#include <map>
#include <string>

class AlembicSceneCache
{
public:
   
   static AlembicScene* Ref(const std::string &filepath, bool persistent=false);
   static bool Unref(AlembicScene *scene);
   
private:
   
   static AlembicSceneCache& Instance();
   
   AlembicSceneCache();
   ~AlembicSceneCache();
   
   std::string formatPath(const std::string &filepath);
   
   AlembicScene* ref(const std::string &filepath, bool persistent=false);
   bool unref(AlembicScene *scene);
   
   
private:
   
   struct CacheEntry
   {
      Alembic::Abc::IArchive archive;
      Alembic::AbcCoreFactory::IFactory::CoreType type;
      AlembicScene *master;
      bool persistent;
      int refcount;
   };
   
   std::map<std::string, CacheEntry> mScenes;
};

#endif
