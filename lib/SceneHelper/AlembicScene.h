#ifndef SCENEHELPER_ALEMBICSCENE_H_
#define SCENEHELPER_ALEMBICSCENE_H_

#include "AlembicNode.h"
#include <Alembic/AbcGeom/All.h>
#include <regex.h>

class AlembicScene : public AlembicNode
{
public:
   
   AlembicScene(Alembic::Abc::IArchive iArch);
   AlembicScene(const AlembicScene &rhs);
   virtual ~AlembicScene();
   
   virtual AlembicNode* clone(AlembicNode *parent=0) const;
   
   bool isFiltered(AlembicNode *node) const;
   inline void setFilter(const std::string &filter) { setFilters(filter, ""); }
   void setIncludeFilter(const std::string &includeFilter);
   void setExcludeFilter(const std::string &excludeFilter);
   void setFilters(const std::string &incl, const std::string &excl);
   Set::iterator beginFiltered();
   Set::iterator endFiltered();
   Set::const_iterator beginFiltered() const;
   Set::const_iterator endFiltered() const;
   
   template <class Visitor>
   bool visit(VisitMode mode, Visitor &visitor);
   
   AlembicNode* find(const char *path);
   const AlembicNode* find(const char *path) const;
   
   inline AlembicNode* find(const std::string &path) { return find(path.c_str()); }
   inline const AlembicNode* find(const std::string &path) const { return find(path.c_str()); }
   
   inline Alembic::Abc::IArchive archive() { return mArchive; }
   
private:
   
   bool filter(AlembicNode *node);

private:
   
   Alembic::Abc::IArchive mArchive;
   std::string mIncludeFilterStr;
   std::string mExcludeFilterStr;
   regex_t *mIncludeFilter;
   regex_t mIncludeFilter_;
   regex_t *mExcludeFilter;
   regex_t mExcludeFilter_;
   std::set<AlembicNode*> mFilteredNodes;
};

// ---

template <class Visitor>
bool AlembicNode::visit(VisitMode mode, AlembicScene *scene, Visitor &visitor)
{
   VisitReturn rv;
   
   if (mode == VisitFilteredFlat)
   {
      // Should never reach here from AlembicScene::visit
      rv = enter(visitor);
      
      leave(visitor);
      
      return (rv != StopVisit);
   }
   else if (mode == VisitBreadthFirst)
   {
      // Don't need to call enter/leave or check filtered status on current node (called by parent's visit)
      
      std::set<AlembicNode*> skipNodes;
      
      for (Array::iterator it=beginChild(); it!=endChild(); ++it)
      {
         if (!scene->isFiltered(*it))
         {
            skipNodes.insert(*it);
         }
         else
         {
            rv = (*it)->enter(visitor);
            (*it)->leave(visitor);
            
            if (rv == StopVisit)
            {
               return false;
            }
            else if (rv == DontVisitChildren)
            {
               skipNodes.insert(*it);
            }
         }
      }
      
      for (Array::iterator it=mChildren.begin(); it!=mChildren.end(); ++it)
      {
         if (skipNodes.find(*it) == skipNodes.end())
         {
            if (!(*it)->visit(mode, scene, visitor))
            {
               return false;
            }
         }
      }
   }
   else
   {
      if (scene->isFiltered(this))
      {
         rv = enter(visitor);
         
         if (rv == StopVisit)
         {
            leave(visitor);
            return false;
         }
         else if (rv != DontVisitChildren)
         {
            for (Array::iterator it=beginChild(); it!=endChild(); ++it)
            {
               if (!(*it)->visit(mode, scene, visitor))
               {
                  leave(visitor);
                  return false;
               }
            }
         }
         
         leave(visitor);
      }
   }
   
   return true;
}

template <class Visitor>
bool AlembicScene::visit(VisitMode mode, Visitor &visitor)
{
   VisitReturn rv;
   
   if (mode == VisitFilteredFlat)
   {
      std::set<AlembicNode*> skipChildrenOf;
      
      for (Set::iterator it=beginFiltered(); it!=endFiltered(); ++it)
      {
         bool skip = false;
         
         for (std::set<AlembicNode*>::iterator pit=skipChildrenOf.begin(); pit!=skipChildrenOf.end(); ++pit)
         {
            if ((*it)->hasAncestor(*pit))
            {
               skip = true;
               break;
            }
         }
         
         if (!skip)
         {
            rv = (*it)->enter(visitor);
            (*it)->leave(visitor);
            
            if (rv == StopVisit)
            {
               return false;
            }
            else if (rv == DontVisitChildren)
            {
               skipChildrenOf.insert(*it);
            }
         }
      }
   }
   else
   {
      if (mode == VisitBreadthFirst)
      {
         return AlembicNode::visit(VisitBreadthFirst, this, visitor);
         
         /*
         std::set<AlembicNode*> skipNodes;
         
         for (Array::iterator it=beginChild(); it!=endChild(); ++it)
         {
            if (!isFiltered(*it))
            {
               skipNodes.insert(*it);
            }
            else
            {
               rv = (*it)->enter(visitor);
               (*it)->leave(visitor);
               
               if (!rv == StopVisit)
               {
                  return false;
               }
               else if (rv == DontVisitChildren)
               {
                  skipNodes.insert(*it);
               }
            }
         }
         
         for (Array::iterator it=beginChild(); it!=endChild(); ++it)
         {
            if (skipNodes.find(*it) == skipNodes.end())
            {
               if (!(*it)->visit(mode, this, visitor))
               {
                  return false;
               }
            }
         }
         */
      }
      else
      {
         for (Array::iterator it=beginChild(); it!=endChild(); ++it)
         {
            if (!(*it)->visit(mode, this, visitor))
            {
               return false;
            }
         }
      }
   }
   
   return true;
}

#endif
