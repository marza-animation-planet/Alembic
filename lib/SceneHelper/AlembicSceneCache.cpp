#include "AlembicSceneCache.h"


AlembicScene* AlembicSceneCache::Ref(const std::string &filepath, bool persistent)
{
   return Instance().ref(filepath, persistent);
}

AlembicScene* AlembicSceneCache::Ref(const std::string &filepath, const AlembicSceneFilter &filter, bool persistent)
{
   return Instance().ref(filepath, filter, persistent);
}

AlembicScene* AlembicSceneCache::Ref(const std::string &filepath, const std::string &id, bool persistent)
{
   return Instance().ref(filepath, id, persistent);
}

AlembicScene* AlembicSceneCache::Ref(const std::string &filepath, const std::string &id, const AlembicSceneFilter &filter, bool persistent)
{
   return Instance().ref(filepath, id, filter, persistent);
}

bool AlembicSceneCache::Unref(AlembicScene *scene)
{
   return Instance().unref(scene);
}

bool AlembicSceneCache::Unref(AlembicScene *scene, const std::string &id)
{
   return Instance().unref(scene, id);
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
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache]   " << it->first << std::endl;
         #endif
         
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
      if (rv[i] == '\\')
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
   return ref(filepath, "", persistent);
}

AlembicScene* AlembicSceneCache::ref(const std::string &filepath, const std::string &id, bool persistent)
{
   std::string path = formatPath(filepath);
   std::string key = path + id;
   
   AlembicScene *rv = 0;
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(key);
   
   if (it != mScenes.end())
   {
      if (!it->second.master)
      {
         rv = NULL;
      }
      else
      {
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Return master scene [id=" << id << ", refcount=" << it->second.refcount << "]" << std::endl;
         #endif
         
         it->second.refcount++;
         
         if (it->second.type != Alembic::AbcCoreFactory::IFactory::kHDF5)
         {
            it->second.persistent = persistent;
         }
         
         rv = it->second.master;
      }
   }
   else
   {
      if (id != "")
      {
         // This will create master scene if required
         rv = ref(filepath, persistent);
         
         if (rv)
         {
            CacheEntry &me = mScenes[path];
            CacheEntry &ce = mScenes[key];
            
            ce.archive = me.archive;
            ce.type = me.type;
            ce.refcount = 1;
            ce.persistent = (ce.type != Alembic::AbcCoreFactory::IFactory::kHDF5 ? persistent : false);
            
            #ifdef _DEBUG
            std::cout << "[AlembicSceneCache] Create master scene clone" << std::endl;
            #endif
            
            ce.master = (AlembicScene*) rv->clone();
            
            rv = ce.master;
            
            // Don't call unref as we want to keep the scene with no id around
            // for probable further cloning
            //unref(rv);
         }
      }
      else
      {
         Alembic::AbcCoreFactory::IFactory factory;
         Alembic::AbcCoreFactory::IFactory::CoreType coreType;
         
         factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
         Alembic::Abc::IArchive archive = factory.getArchive(path, coreType);
         
         if (archive.valid())
         {
            #ifdef _DEBUG
            std::cout << "[AlembicSceneCache] Create master scene" << std::endl;
            #endif
            
            // note: key == path
            CacheEntry &ce = mScenes[key];
            
            ce.archive = archive;
            ce.type = coreType;
            ce.refcount = 1;
            // encountered issues keeping HDF5 achives around in arnold procedural
            ce.persistent = (ce.type != Alembic::AbcCoreFactory::IFactory::kHDF5 ? persistent : false);
            ce.master = new AlembicScene(Alembic::Abc::IArchive(ce.archive.getPtr(),
                                                                Alembic::Abc::kWrapExisting,
                                                                Alembic::Abc::ErrorHandler::kQuietNoopPolicy));
            
            rv = ce.master;
         }
      }
   }
   
   return rv;
}

AlembicScene* AlembicSceneCache::ref(const std::string &filepath, const AlembicSceneFilter &filter, bool persistent)
{
   return ref(filepath, "", filter, persistent);
}

AlembicScene* AlembicSceneCache::ref(const std::string &filepath, const std::string &id, const AlembicSceneFilter &filter, bool persistent)
{
   AlembicScene *rv = ref(filepath, id, persistent);
   
   if (rv)
   {
      if (filter.isSet())
      {
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Create filtered scene" << std::endl;
         #endif
         
         if (filter.excludeExpression().length() == 0 && rv->find(filter.includeExpression().c_str()) != 0)
         {
            // way faster for deep/huge hierarchy 
            rv = rv->cloneSingle(filter.includeExpression().c_str());
         }
         else
         {
            rv = (AlembicScene*) rv->filteredClone(filter);
         }
      }
      else
      {
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Empty filter, return master scene" << std::endl;
         #endif
      }
   }
   
   return rv;
}

bool AlembicSceneCache::unref(AlembicScene *scene)
{
   return unref(scene, "");
}

bool AlembicSceneCache::unref(AlembicScene *scene, const std::string &id)
{
   if (!scene)
   {
      return false;
   }
   
   bool rv = false;
   
   std::string path = formatPath(scene->archive().getName());
   std::string key = path + id;
   
   std::map<std::string, CacheEntry>::iterator it = mScenes.find(key);
   
   if (it != mScenes.end())
   {
      it->second.refcount--;
      
      #ifdef _DEBUG
      std::cout << "[AlembicSceneCache] Unreferencing master scene [id=" << id << ", refcount=" << it->second.refcount << "]" << std::endl;
      #endif
      
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
               it->second.master = 0;
            }
            
            mScenes.erase(it);
            
            if (id != "")
            {
               // unref empty no id scene too
               it = mScenes.find(path);
               if (it != mScenes.end())
               {
                  unref(it->second.master);
               }
            }
         }
         else
         {
            it->second.refcount = 0;
         }
      }
      
      if (!isMasterScene)
      {
         #ifdef _DEBUG
         std::cout << "[AlembicSceneCache] Destroy filtered scene" << std::endl;
         #endif
         
         delete scene;
      }
      
      
      rv = true;
   }
   
   return rv;
}
