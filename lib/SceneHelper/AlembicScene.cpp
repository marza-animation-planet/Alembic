#include "AlembicScene.h"

AlembicScene::AlembicScene(Alembic::Abc::IArchive iArch)
   : AlembicNode(iArch.getTop())
   , mArchive(iArch)
   , mIncludeFilter(0)
   , mExcludeFilter(0)
{
   resolveInstances(this);
}

AlembicScene::AlembicScene(const AlembicScene &rhs)
   : AlembicNode(rhs)
   , mArchive(rhs.mArchive)
   , mIncludeFilter(0)
   , mExcludeFilter(0)
{
   resolveInstances(this);
   setFilters(rhs.mIncludeFilterStr, rhs.mExcludeFilterStr);
}

AlembicScene::~AlembicScene()
{
   if (mIncludeFilter)
   {
      regfree(mIncludeFilter);
      mIncludeFilter = 0;
   }
   
   if (mExcludeFilter)
   {
      regfree(mExcludeFilter);
      mExcludeFilter = 0;
   }
}

AlembicNode* AlembicScene::clone(AlembicNode *) const
{
   return new AlembicScene(*this);
}

void AlembicScene::setIncludeFilter(const std::string &expr)
{
   if (mIncludeFilter)
   {
      regfree(mIncludeFilter);
      mIncludeFilter = 0;
   }
   
   mFilteredNodes.clear();
   mIncludeFilterStr = "";
   
   if (expr.length() > 0)
   {
      if (regcomp(&mIncludeFilter_, expr.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
      {
         std::cout << "Invalid expression: \"" << expr << "\"" << std::endl;
      }
      else
      {
         mIncludeFilterStr = expr;
         mIncludeFilter = &mIncludeFilter_;
         filter(this);
      }
   }
}

void AlembicScene::setExcludeFilter(const std::string &expr)
{
   if (mExcludeFilter)
   {
      regfree(mExcludeFilter);
      mExcludeFilter = 0;
   }
   
   mFilteredNodes.clear();
   mExcludeFilterStr = "";
   
   if (expr.length() > 0)
   {
      if (regcomp(&mExcludeFilter_, expr.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
      {
         std::cout << "Invalid expression: \"" << expr << "\"" << std::endl;
      }
      else
      {
         mExcludeFilterStr = expr;
         mExcludeFilter = &mExcludeFilter_;
         filter(this);
      }
   }
}

void AlembicScene::setFilters(const std::string &incl, const std::string &excl)
{
   if (mIncludeFilter)
   {
      regfree(mIncludeFilter);
      mIncludeFilter = 0;
   }
   
   if (mExcludeFilter)
   {
      regfree(mExcludeFilter);
      mExcludeFilter = 0;
   }
   
   mFilteredNodes.clear();
   
   mIncludeFilterStr = "";
   mExcludeFilterStr = "";
   
   if (incl.length() > 0)
   {
      if (regcomp(&mIncludeFilter_, incl.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
      {
         std::cout << "Invalid expression: \"" << incl << "\"" << std::endl;
      }
      else
      {
         mIncludeFilterStr = incl;
         mIncludeFilter = &mIncludeFilter_;
      }
   }
   
   if (excl.length() > 0)
   {
      if (regcomp(&mExcludeFilter_, excl.c_str(), REG_EXTENDED|REG_NOSUB) != 0)
      {
         std::cout << "Invalid expression: \"" << excl << "\"" << std::endl;
      }
      else
      {
         mExcludeFilterStr = excl;
         mExcludeFilter = &mExcludeFilter_;
      }
   }
   
   if (mIncludeFilter || mExcludeFilter)
   {
      filter(this);
   }
}

bool AlembicScene::isFiltered(AlembicNode *node) const
{
   return ((!mIncludeFilter && !mExcludeFilter) || mFilteredNodes.find(node) != mFilteredNodes.end());
}

bool AlembicScene::filter(AlembicNode *node)
{
   size_t numChildren = 0;
   
   bool excluded = (mExcludeFilter && regexec(mExcludeFilter, node->path().c_str(), 0, NULL, 0) == 0);
   
   if (excluded)
   {
      node->reset();
      return false;
   }
   
   for (Array::iterator it=node->beginChild(); it!=node->endChild(); ++it)
   {
      if (filter(*it))
      {
         ++numChildren;
         mFilteredNodes.insert(*it);
      }
   }
   
   // Even if node should not be included, if any of the children was filtered successfully, include it
   if (numChildren == 0)
   {
      bool included = (!mIncludeFilter || regexec(mIncludeFilter, node->path().c_str(), 0, NULL, 0) == 0);
      
      if (!included)
      {
         node->reset();
         return false;
      }
   }
   return true;
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
