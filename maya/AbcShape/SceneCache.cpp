#include "SceneCache.h"


AlembicScene* SceneCache::Ref(const std::string &filepath)
{
   return Instance().ref(filepath);
}

void SceneCache::Unref(AlembicScene *scene)
{
   Instance().unref(scene);
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
   mMutex.lock();
   
   #ifdef _DEBUG
   std::cout << "[AbcShape] Clear cached scene(s) (" << mScenes.size() << " remaining)" << std::endl;
   #endif
   
   mScenes.clear();
   
   mMutex.unlock();
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
   
   mMutex.lock();
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(path);
   
   if (it != mScenes.end())
   {
      it->second.refcount++;
      rv = new AlembicScene(Alembic::Abc::IArchive(it->second.archive.getPtr(),
                                                   Alembic::Abc::kWrapExisting,
                                                   Alembic::Abc::ErrorHandler::kQuietNoopPolicy));
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
         
         rv = new AlembicScene(Alembic::Abc::IArchive(ce.archive.getPtr(),
                                                      Alembic::Abc::kWrapExisting,
                                                      Alembic::Abc::ErrorHandler::kQuietNoopPolicy));
      }
   }
   
   mMutex.unlock();
   
   return rv;
}

void SceneCache::unref(AlembicScene *scene)
{
   if (!scene)
   {
      return;
   }
   
   #ifdef _DEBUG
   std::cout << "[AbcShape] Unreferencing scene" << std::endl;
   #endif
   
   mMutex.lock();
   
   std::string path = formatPath(scene->archive().getName());
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(path);
   
   if (it != mScenes.end())
   {
      it->second.refcount--;
      if (it->second.refcount == 0)
      {
         #ifdef _DEBUG
         std::cout << "[AbcShape] Last scene referencing alembic archive \"" << it->second.archive.getName() << "\"" << std::endl;
         #endif
         
         mScenes.erase(it);
      }
      
      delete scene;
   }
   
   mMutex.unlock();
}
