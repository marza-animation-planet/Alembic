#include "AlembicScene.h"

AlembicScene::AlembicScene()
   : AlembicNode()
   , mIsFiltered(false)
{
}

AlembicScene::AlembicScene(Alembic::Abc::IArchive iArch)
   : AlembicNode(iArch.getTop())
   , mArchive(iArch)
   , mIsFiltered(false)
{
   resolveInstances(this);
}

AlembicScene::AlembicScene(const AlembicScene &rhs)
   : AlembicNode(rhs, 0)
   , mArchive(rhs.mArchive)
   , mIsFiltered(rhs.mIsFiltered)
{
   resolveInstances(this);
}

AlembicScene::AlembicScene(const AlembicScene &rhs, const AlembicSceneFilter &filter)
   : AlembicNode(rhs, filter, 0)
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

AlembicNode* AlembicScene::selfClone() const
{
   AlembicScene *cs = new AlembicScene();
   cs->mArchive = mArchive;
   return cs;
}

AlembicScene* AlembicScene::cloneSingle(const char *path) const
{
   const AlembicNode *n = find(path);
   if (!n)
   {
      return 0;
   }
   
   // clone n with its descendents
   AlembicNode *cc = n->clone();
   AlembicNode *cp = 0;
   
   // add parents up to root
   const AlembicNode *p = n->parent();
   
   while (p)
   {
      cp = p->selfClone();
      cp->addChild(cc);
      
      cc = cp;
      p = p->parent();
   }
   
   AlembicScene *cs = (AlembicScene*) selfClone();
   cs->addChild(cc);
   cs->resolveInstances(cs);
   
   return cs;
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
