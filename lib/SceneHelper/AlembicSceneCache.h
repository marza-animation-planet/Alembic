#ifndef SCENEHELPER_ALEMBICSCENECACHE_H_
#define SCENEHELPER_ALEMBICSCENECACHE_H_

#include "AlembicScene.h"
#include "AlembicSceneFilter.h"
#include <Alembic/AbcCoreFactory/All.h>
#include <map>
#include <string>

class AlembicSceneCache
{
public:
   
   static void SetConcurrency(size_t n);
   static void SetUseMemoryMappedFiles(bool onoff);

   static AlembicScene* Ref(const std::string &filepath, bool persistent=false);
   static AlembicScene* Ref(const std::string &filepath, const AlembicSceneFilter &filter, bool persistent=false);
   static AlembicScene* Ref(const std::string &filepath, const std::string &id, bool persistent=false);
   static AlembicScene* Ref(const std::string &filepath, const std::string &id, const AlembicSceneFilter &filter, bool persistent=false);
   static bool Unref(AlembicScene *scene);
   static bool Unref(AlembicScene *scene, const std::string &id);
   
private:
   
   static AlembicSceneCache& Instance();
   
   AlembicSceneCache();
   ~AlembicSceneCache();

   void setConcurrency(size_t n);
   void setUseMemoryMappedFiles(bool onoff);
   
   std::string formatPath(const std::string &filepath);
   
   AlembicScene* ref(const std::string &filepath, bool persistent=false);
   AlembicScene* ref(const std::string &filepath, const AlembicSceneFilter &filter, bool persistent=false);
   AlembicScene* ref(const std::string &filepath, const std::string &id, bool persistent=false);
   AlembicScene* ref(const std::string &filepath, const std::string &id, const AlembicSceneFilter &filter, bool persistent=false);
   bool unref(AlembicScene *scene);
   bool unref(AlembicScene *scene, const std::string &id);
   
   
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

   size_t mDefaultConcurrency;
   bool mUseMemoryMappedFiles;
};

#endif
