#include "AlembicScene.h"

AlembicScene::AlembicScene(Alembic::Abc::IArchive iArch)
   : AlembicNode(iArch.getTop())
   , mArchive(iArch)
   , mFiltered(false)
{
   resolveInstances(this);
}

AlembicScene::AlembicScene(const AlembicScene &rhs)
   : AlembicNode(rhs)
   , mArchive(rhs.mArchive)
   , mFiltered(false)
{
   resolveInstances(this);
   setFilter(rhs.mFilterStr);
}

AlembicScene::~AlembicScene()
{
}

AlembicNode* AlembicScene::clone(AlembicNode *) const
{
   return new AlembicScene(*this);
}

void AlembicScene::setFilter(const std::string &expr)
{
   if (mFiltered)
   {
      regfree(&mFilter);
   }
   
   mFilteredNodes.clear();
   mFilterStr = "";
   mFiltered = (expr.length() > 0);
   
   if (mFiltered)
   {
      if (regcomp(&mFilter, expr.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
      {
         std::cout << "Invalid expression: \"" << expr << "\"" << std::endl;
         mFiltered = false;
      }
      else
      {
         mFilterStr = expr;
         filter(this);
      }
   }
}

bool AlembicScene::isFiltered(AlembicNode *node) const
{
   return (!mFiltered || mFilteredNodes.find(node) != mFilteredNodes.end());
}

bool AlembicScene::filter(AlembicNode *node)
{
   size_t numChildren = 0;
   
   bool matched = (regexec(&mFilter, node->path().c_str(), 0, NULL, 0) == 0);
   
   for (Array::iterator it=node->beginChild(); it!=node->endChild(); ++it)
   {
      if (filter(*it))
      {
         ++numChildren;
         mFilteredNodes.insert(*it);
      }
   }
   
   // Even if result is 0, if any of the children was filtered successfully, keep this node
   if (numChildren > 0 || matched)
   {
      return true;
   }
   else
   {
      node->reset();
      return false;
   }
}

AlembicNode::Set::iterator AlembicScene::beginFiltered()
{
   return mFilteredNodes.begin();
}

AlembicNode::Set::iterator AlembicScene::endFiltered()
{
   return mFilteredNodes.end();
}

AlembicNode::Set::const_iterator AlembicScene::beginFiltered() const
{
   return mFilteredNodes.begin();
}

AlembicNode::Set::const_iterator AlembicScene::endFiltered() const
{
   return mFilteredNodes.end();
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
