#include "SceneCache.h"


AlembicScene* SceneCache::Ref(const std::string &filepath)
{
   return Instance().ref(filepath);
}

bool SceneCache::Unref(AlembicScene *scene)
{
   return Instance().unref(scene);
}

SceneCache& SceneCache::Instance()
{
   static SceneCache sTheInstance;
   return sTheInstance;
}

// ---

SceneCache::SceneCache()
{
}

SceneCache::~SceneCache()
{
   #ifdef _DEBUG
   std::cout << "[AbcShape] Clear cached scene(s) (" << mScenes.size() << " remaining)" << std::endl;
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

std::string SceneCache::formatPath(const std::string &filepath)
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

AlembicScene* SceneCache::ref(const std::string &filepath)
{
   std::string path = formatPath(filepath);
   AlembicScene *rv = 0;
   
   //mMutex.lock();
   
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
         #ifdef _DEBUG
         std::cout << "[AbcShape] Clone master scene" << std::endl;
         #endif
         rv = new AlembicScene(*(it->second.master));
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
         ce.master = new AlembicScene(Alembic::Abc::IArchive(ce.archive.getPtr(),
                                                             Alembic::Abc::kWrapExisting,
                                                             Alembic::Abc::ErrorHandler::kQuietNoopPolicy));
         
         return ce.master;
      }
   }
   
   //mMutex.unlock();
   
   return rv;
}

bool SceneCache::unref(AlembicScene *scene)
{
   if (!scene)
   {
      return false;
   }
   
   bool rv = false;
   
   #ifdef _DEBUG
   std::cout << "[AbcShape] Unreferencing scene" << std::endl;
   #endif
   
   //mMutex.lock();
   
   std::string path = formatPath(scene->archive().getName());
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(path);
   
   if (it != mScenes.end())
   {
      it->second.refcount--;
      
      bool isMasterScene = (scene == it->second.master);
      
      if (it->second.refcount == 0)
      {
         #ifdef _DEBUG
         std::cout << "[AbcShape] Last scene referencing alembic archive \"" << it->second.archive.getName() << "\"" << std::endl;
         #endif
         
         #ifdef _DEBUG
         std::cout << "[AbcShape] Destroy master scene" << std::endl;
         #endif
         if (it->second.master)
         {
            delete it->second.master;
         }
         
         mScenes.erase(it);
      }
      
      if (!isMasterScene)
      {
         #ifdef _DEBUG
         std::cout << "[AbcShape] Destroy scene" << std::endl;
         #endif
         delete scene;
      }
      
      rv = true;
   }
   
   //mMutex.unlock();
   
   return rv;
}
