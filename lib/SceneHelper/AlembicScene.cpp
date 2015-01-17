#include "AlembicScene.h"

AlembicScene::AlembicScene(Alembic::Abc::IArchive iArch)
   : AlembicNode(iArch.getTop())
   , mArchive(iArch)
   , mIsFiltered(false)
{
   resolveInstances(this);
}

AlembicScene::AlembicScene(const AlembicScene &rhs)
   : AlembicNode(rhs)
   , mArchive(rhs.mArchive)
   , mIsFiltered(rhs.mIsFiltered)
{
   resolveInstances(this);
}

AlembicScene::AlembicScene(const AlembicScene &rhs, const AlembicSceneFilter &filter)
   : AlembicNode(rhs, filter)
   , mArchive(rhs.mArchive)
   , mIsFiltered(true)
{
   resolveInstances(this);
}

AlembicScene::~AlembicScene()
{
}

AlembicNode* AlembicScene::filteredClone(const AlembicSceneFilter &filter, AlembicNode *) const
{
   return new AlembicScene(*this, filter);
}

AlembicNode* AlembicScene::clone(AlembicNode *) const
{
   return new AlembicScene(*this);
}

const AlembicNode* AlembicScene::find(const char *path) const
{
   if (!path || strlen(path) == 0 || path[0] != '/')
   {
      return 0;
   }
   else
   {
      return AlembicNode::find(path + 1);
   }
}

AlembicNode* AlembicScene::find(const char *path)
{
   if (!path || strlen(path) == 0 || path[0] != '/')
   {
      return 0;
   }
   else
   {
      return AlembicNode::find(path + 1);
   }
}
