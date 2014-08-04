#include "AlembicScene.h"
#include <Alembic/AbcCoreFactory/All.h>
#include <iostream>
#include <ctime>

static void Indent(int level)
{
   for (int i=0; i<level; ++i)
   {
      std::cout << "  ";
   }
}

// ---

class PrintVisitor
{
public:
   
   PrintVisitor()
      : mCurDepth(0)
   {
   }
   
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0)
   {
      Indent(mCurDepth);
      std::cout << node.formatPartialPath("cubes:", AlembicNode::LocalPrefix, '|'); // << std::endl;
      if (node.isInstance())
      {
         std::cout << " [" << node.instanceNumber() << "] [instance: " << node.masterPath() << "]";
      }
      else
      {
         std::cout << " [" << node.typeName() << "]";
      }
      std::cout << std::endl;
      ++mCurDepth;
      return AlembicNode::ContinueVisit;
   }
   
   void leave(AlembicNode &, AlembicNode *instance=0)
   {
      --mCurDepth;
   }

private:
   
   int mCurDepth;
};

class SampleBoundsVisitor
{
public:
   
   SampleBoundsVisitor(double t)
      : mTime(t)
   {
   }
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0)
   {
      node.sampleBounds(mTime);
      
      AlembicXform::Sample &samp0 = node.firstSample();
      AlembicXform::Sample &samp1 = node.secondSample();
      
      if (samp1.dataWeight > 0.0)
      {
         if (samp0.data.getInheritsXforms() != samp1.data.getInheritsXforms())
         {
            std::cout << node.path() << ": Animated inherits transform property found, use first sample value" << std::endl;
            node.setSelfMatrix(samp0.data.getMatrix());
         }
         else
         {
            Alembic::Abc::M44d m0 = samp0.data.getMatrix();
            Alembic::Abc::M44d m1 = samp1.data.getMatrix();
            
            node.setSelfMatrix(samp0.dataWeight * m0 + samp1.dataWeight * m1);
         }
      }
      else
      {
         node.setSelfMatrix(samp0.data.getMatrix());
      }
      
      node.setInheritsTransform(samp0.data.getInheritsXforms());
      
      node.updateWorldMatrix();
      
      return AlembicNode::ContinueVisit;
   }
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0)
   {
      node.sampleBounds(mTime);
      
      typename AlembicNodeT<T>::Sample &samp0 = node.firstSample();
      typename AlembicNodeT<T>::Sample &samp1 = node.secondSample();
      
      if (samp1.boundsWeight > 0.0)
      {
         
         Alembic::Abc::Box3d b0 = samp0.bounds;
         Alembic::Abc::Box3d b1 = samp1.bounds;
         
         node.setSelfBounds(Alembic::Abc::Box3d(samp0.boundsWeight * b0.min + samp1.boundsWeight * b1.min,
                                                samp0.boundsWeight * b0.max + samp1.boundsWeight * b1.max));
      }
      else
      {
         node.setSelfBounds(samp0.bounds);
      }
      
      node.updateWorldMatrix();
      
      return AlembicNode::ContinueVisit;
   }
   
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0)
   {
      return shapeEnter(node, instance);
   }
   
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0)
   {
      return shapeEnter(node, instance);
   }
   
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0)
   {
      return shapeEnter(node, instance);
   }
   
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0)
   {
      return shapeEnter(node, instance);
   }
   
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0)
   {
      return shapeEnter(node, instance);
   }
   
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0)
   {
      if (node.isInstance())
      {
         AlembicNode *m = node.master();
         m->enter(*this, &node);
      }
      
      node.updateWorldMatrix();
      
      return AlembicNode::ContinueVisit;
   }
      
   void leave(AlembicNode &node, AlembicNode *instance=0)
   {
      node.updateChildBounds();
      
      std::cout << "\"" << node.path() << "\" updated" << std::endl;
      std::cout << "  World matrix: " << node.worldMatrix() << std::endl;
      std::cout << "  Child bounds: " << node.childBounds().min << " - " << node.childBounds().max << std::endl;
      std::cout << std::endl;
   }
   
private:
   
   double mTime;
};

// ---

enum Action
{
   Print = 0,
   Bounds,
   NumActions
};

static const char* ActionNames[] =
{
   "print",
   "bounds"
};

void usage()
{
   std::cout << "AlembicShape [-e expression] [-t frame] [-f framerate] [-m mode] [-a ";
   std::cout << ActionNames[0];
   for (int i=1; i<NumActions; ++i)
   {
      std::cout << "|" << ActionNames[i];
   }
   std::cout << "] [-h] filepath" << std::endl;
}



int main(int argc, char **argv)
{
   std::string path = "";
   std::string filter = "";
   AlembicNode::VisitMode mode = AlembicNode::VisitDepthFirst;
   double frame = 1.0;
   double fps = 24.0;
   Action action = Print;
   
   int i = 1;
   while (i < argc)
   {
      if (!strcmp(argv[i], "-f"))
      {
         ++i;
         if (i >= argc) exit(1);
         double rv = fps;
         if (sscanf(argv[i], "%lf", &rv) == 1)
         {
            fps = rv;
         }
      }
      if (!strcmp(argv[i], "-t"))
      {
         ++i;
         if (i >= argc) exit(1);
         double rv = frame;
         if (sscanf(argv[i], "%lf", &rv) == 1)
         {
            frame = rv;
         }
      }
      else if (!strcmp(argv[i], "-m"))
      {
         ++i;
         if (i >= argc) exit(1);
         int rv = -1;
         if (sscanf(argv[i], "%d", &rv) == 1)
         {
            if (rv < AlembicNode::VisitDepthFirst)
            {
               mode = AlembicNode::VisitDepthFirst;
            }
            else if (rv > AlembicNode::VisitFilteredFlat)
            {
               mode = AlembicNode::VisitFilteredFlat;
            }
            else
            {
               mode = (AlembicNode::VisitMode) rv;
            }
         }
      }
      else if (!strcmp(argv[i], "-e"))
      {
         ++i;
         if (i >= argc) exit(1);
         filter = argv[i];
      }
      else if (!strcmp(argv[i], "-a"))
      {
         ++i;
         if (i >= argc) exit(1);
         for (int j=0; j<NumActions; ++j)
         {
            if (!strcmp(argv[i], ActionNames[j]))
            {
               action = (Action)j;
               break;
            }
         }
      }
      else if (!strcmp(argv[i], "-h"))
      {
         usage();
      }
      else
      {
         path = argv[i];
      }
      ++i;
   }
   
   if (path.length() > 0)
   {
      clock_t st, et;
      float ts = 1.0f / CLOCKS_PER_SEC;
      
      st = clock();
      
      Alembic::Abc::IArchive archive;
      
      Alembic::AbcCoreFactory::IFactory factory;
      factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
      archive = factory.getArchive(path.c_str());
      
      if (!archive.valid())
      {
         std::cout << "Not a valid ABC file: \"" << path << "\"" << std::endl;
         return 1;
      }
      
      et = clock();
      std::cout << "Read archive in " << (ts * (et - st)) << " second(s)" << std::endl;
      
      // ---
      
      st = clock();
      
      AlembicScene scene(archive);
      
      et = clock();
      std::cout << "Build graph in " << (ts * (et - st)) << " second(s)" << std::endl;
      
      if (filter.length() > 0)
      {
         st = clock();
         
         scene.setFilter(filter);
         
         et = clock();
         std::cout << "Filter graph in " << (ts * (et - st)) << " second(s)" << std::endl;
      }
      
      // ---
      
      double t = (frame / fps);
      
      st = clock();
      
      switch (action)
      {
      case Print:
         {
            PrintVisitor print;
            scene.visit(mode, print);   
         }
         break;
      case Bounds:
         {
            SampleBoundsVisitor sampleBounds(t);
            scene.visit(mode, sampleBounds);
         }
         break;
      default:
         break;
      }
      
      et = clock();
      std::cout << "[" << ActionNames[action] << " (t=" << t << ")] Visited graph in " << (ts * (et - st)) << " second(s)" << std::endl;
      
      // ---
      
      std::cout << std::endl << "PRESS ENTER KEY TO FINISH" << std::endl;
      getchar();
   }
   
   return 0;
}
