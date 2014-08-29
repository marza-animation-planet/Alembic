#include "AlembicSceneCache.h"


AlembicScene* AlembicSceneCache::Ref(const std::string &filepath, bool persistent)
{
   return Instance().ref(filepath, persistent);
}

bool AlembicSceneCache::Unref(AlembicScene *scene)
{
   return Instance().unref(scene);
}

AlembicSceneCache& AlembicSceneCache::Instance()
{
   static AlembicSceneCache sTheInstance;
   return sTheInstance;
}

// ---

AlembicSceneCache::AlembicSceneCache()
{
}

AlembicSceneCache::~AlembicSceneCache()
{
   #ifdef _DEBUG
   std::cout << "[AlembicSceneCache] Clear cached scene(s) (" << mScenes.size() << " remaining)" << std::endl;
   #endif
   
   for (std::map<std::string, CacheEntry>::iterator it = mScenes.begin(); it != mScenes.end(); ++it)
   {
      if (it->second.master)
      {
         delete it->second.master;
      }
   }
   
   mScenes.clear();
}

std::string AlembicSceneCache::formatPath(const std::string &filepath)
{
   std::string rv = filepath;
   
   #ifdef _WIN32
   size_t n = rv.length();
   for (size_t i=0; i<n; ++i)
   {
      if (i == '\\')
      {
         rv[i] = '/';
      }
      else if (rv[i] >= 'A' && rv[i] <= 'Z')
      {
         rv[i] = 'a' + (rv[i] - 'A');
      }
   }
   #endif
   
   return rv;
}

AlembicScene* AlembicSceneCache::ref(const std::string &filepath, bool persistent)
{
   std::string path = formatPath(filepath);
   AlembicScene *rv = 0;
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(path);
   
   if (it != mScenes.end())
   {
      if (!it->second.master)
      {
         rv = NULL;
      }
      else
      {
         it->second.refcount++;
         it->second.persistent = it->second.persistent || persistent;
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Clone master scene" << std::endl;
         #endif
         rv = new AlembicScene(*(it->second.master));
         // Reset filter
         rv->setFilter("");
      }
   }
   else
   {
      Alembic::AbcCoreFactory::IFactory factory;
      factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
      Alembic::Abc::IArchive archive = factory.getArchive(path);
      
      if (archive.valid())
      {
         CacheEntry &ce = mScenes[path];
         
         ce.archive = archive;
         ce.refcount = 1;
         ce.persistent = persistent;
         ce.master = new AlembicScene(Alembic::Abc::IArchive(ce.archive.getPtr(),
                                                             Alembic::Abc::kWrapExisting,
                                                             Alembic::Abc::ErrorHandler::kQuietNoopPolicy));
         
         rv = ce.master;
      }
   }
   
   return rv;
}

bool AlembicSceneCache::unref(AlembicScene *scene)
{
   if (!scene)
   {
      return false;
   }
   
   bool rv = false;
   
   #ifdef _DEBUG
   std::cout << "[AlembicSceneCache] Unreferencing scene" << std::endl;
   #endif
   
   std::string path = formatPath(scene->archive().getName());
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(path);
   
   if (it != mScenes.end())
   {
      it->second.refcount--;
      
      bool isMasterScene = (scene == it->second.master);
      
      if (it->second.refcount <= 0)
      {
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Last scene referencing alembic archive \"" << it->second.archive.getName() << "\"" << std::endl;
         #endif
         
         if (!it->second.persistent)
         {
            #ifdef _DEBUG
            std::cout << "[AlembicSceneCache] Destroy master scene" << std::endl;
            #endif
            if (it->second.master)
            {
               delete it->second.master;
            }
            
            mScenes.erase(it);
         }
         else
         {
            it->second.refcount = 0;
         }
      }
      
      if (!isMasterScene)
      {
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Destroy scene" << std::endl;
         #endif
         delete scene;
      }
      
      rv = true;
   }
   
   return rv;
}
