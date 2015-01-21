#ifndef SCENEHELPER_ALEMBICSCENE_H_
#define SCENEHELPER_ALEMBICSCENE_H_

#include "AlembicNode.h"
#include "AlembicSceneFilter.h"
#include <Alembic/AbcGeom/All.h>
#include <regex.h>

class AlembicScene : public AlembicNode
{
public:
   
   friend class AlembicSceneFilter;
   
   AlembicScene(Alembic::Abc::IArchive iArch);
   AlembicScene(Alembic::Abc::IArchive iArch, const AlembicSceneFilter &filter);
   AlembicScene(const AlembicScene &rhs);
   virtual ~AlembicScene();
   
   virtual AlembicNode* filteredClone(const AlembicSceneFilter &filter, AlembicNode *parent=0) const;
   virtual AlembicNode* clone(AlembicNode *parent=0) const;
   virtual AlembicNode* selfClone() const;
   
   AlembicScene* cloneSingle(const char *path) const;
   
   template <class Visitor>
   bool visit(VisitMode mode, Visitor &visitor);
   
   AlembicNode* find(const char *path);
   const AlembicNode* find(const char *path) const;
   
   inline AlembicNode* find(const std::string &path) { return find(path.c_str()); }
   inline const AlembicNode* find(const std::string &path) const { return find(path.c_str()); }
   
   inline Alembic::Abc::IArchive archive() { return mArchive; }
   
   inline bool isFiltered() const { return mIsFiltered; }
   
private:
   
   AlembicScene();
   AlembicScene(const AlembicScene &rhs, const AlembicSceneFilter &filter);

private:
   
   Alembic::Abc::IArchive mArchive;
   bool mIsFiltered;
};

// ---

template <class Visitor>
bool AlembicNode::visit(VisitMode mode, AlembicScene *scene, Visitor &visitor)
{
   VisitReturn rv;
   
   if (mode == VisitBreadthFirst)
   {
      // Don't need to call enter/leave or check filtered status on current node (called by parent's visit)
      
      std::set<AlembicNode*> skipNodes;
      
      for (Array::iterator it=beginChild(); it!=endChild(); ++it)
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
   
   return true;
}

template <class Visitor>
bool AlembicScene::visit(VisitMode mode, Visitor &visitor)
{
   if (mode == VisitBreadthFirst)
   {
      return AlembicNode::visit(VisitBreadthFirst, this, visitor);
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
   
   return true;
}

#endif
