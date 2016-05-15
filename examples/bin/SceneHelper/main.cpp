#include "AlembicSceneCache.h"
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
   
   PrintVisitor();
   
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
   void leave(AlembicNode &, AlembicNode *instance=0);

private:
   
   int mCurDepth;
};

PrintVisitor::PrintVisitor()
   : mCurDepth(0)
{
}

AlembicNode::VisitReturn PrintVisitor::enter(AlembicNode &node, AlembicNode *)
{
   Indent(mCurDepth);
   std::cout << node.formatPartialPath("", AlembicNode::LocalPrefix, '|'); // << std::endl;
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

void PrintVisitor::leave(AlembicNode &, AlembicNode *)
{
   --mCurDepth;
}

// ---

class SampleBoundsVisitor
{
public:
   
   SampleBoundsVisitor(double t, bool keepSamples);
   
   template <class T>
   AlembicNode::VisitReturn shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance=0);
   
   AlembicNode::VisitReturn enter(AlembicXform &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicMesh &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicSubD &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicPoints &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicCurves &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNuPatch &node, AlembicNode *instance=0);
   AlembicNode::VisitReturn enter(AlembicNode &node, AlembicNode *instance=0);
      
   void leave(AlembicNode &node, AlembicNode *instance=0);
   
private:
   
   double mTime;
   bool mKeepSamples;
};

SampleBoundsVisitor::SampleBoundsVisitor(double t, bool keepSamples)
   : mTime(t)
   , mKeepSamples(keepSamples)
{
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicXform &node, AlembicNode *instance)
{
   bool noData = true;
   bool updated = true;
   
   if (node.sampleBounds(mTime, mTime, mKeepSamples, &updated))
   {
      double blend = 0.0;
      TimeSampleList<Alembic::AbcGeom::IXformSchema>::ConstIterator it0, it1;
      
      AlembicXform::SampleSetType &samples = node.samples();
      
      if (samples.schemaSamples.size() == 1)
      {
         if (updated)
         {
            it0 = samples.schemaSamples.begin();
            node.setSelfMatrix(it0->data().getMatrix());
            
            #ifdef _DEBUG
            std::cout << "[SampleBoundsVisitor::enter(xform)] " << node.path() << ": Update constant transform [" << mTime << ", " << mTime << "]" << std::endl;
            #endif
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "[SampleBoundsVisitor::enter(xform)] " << node.path() << ": Constant transform up to date [" << mTime << ", " << mTime << "]" << std::endl;
            #endif
         }
         
         noData = false;
      }
      else
      {
         if (updated || fabs(samples.schemaSamples.lastEvaluationTime - mTime) > 0.0001)
         {
            if (samples.schemaSamples.getSamples(mTime, it0, it1, blend))
            {
               // should check sample validity here
               Alembic::Abc::M44d m0 = it0->data().getMatrix();
               
               if (blend > 0.0)
               {
                  if (it0->data().getInheritsXforms() != it1->data().getInheritsXforms())
                  {
                     std::cout << "[SampleBoundsVisitor::enter(xform)] " << node.path() << ": Samples inherits transform do not match. Use first sample value" << std::endl;
                     
                     node.setSelfMatrix(m0);
                  }
                  else
                  {
                     Alembic::Abc::M44d m1 = it1->data().getMatrix();
                     
                     node.setSelfMatrix((1.0 - blend) * m0 + blend * m1);
                  }
               }
               else
               {
                  node.setSelfMatrix(m0);
               }
               
               node.setInheritsTransform(it0->data().getInheritsXforms());
               
               samples.schemaSamples.lastEvaluationTime = mTime;
               
               #ifdef _DEBUG
               std::cout << "[SampleBoundsVisitor::enter(xform)] " << node.path() << ": Update animated transform [" << mTime << ", " << mTime << "]" << std::endl;
               #endif
               
               noData = false;
            }
         }
         else
         {
            // identital sample
            #ifdef _DEBUG
            std::cout << "[SampleBoundsVisitor::enter(xform)] " << node.path() << ": Animated transform up to date [" << mTime << ", " << mTime << "]" << std::endl;
            #endif
            
            noData = false;
         }
      }
   }
   
   if (noData)
   {
      #ifdef _DEBUG
      std::cout << "[SampleBoundsVisitor::enter(xform)] " << node.path() << ": No valid data [" << mTime << ", " << mTime << "]" << std::endl;
      #endif
      // no valid sample data found
      node.setSelfMatrix(Alembic::Abc::M44d());
      node.setInheritsTransform(true);
   }
   
   if (instance == 0)
   {
      node.updateWorldMatrix();
   }
   
   return AlembicNode::ContinueVisit;
}

template <class T>
AlembicNode::VisitReturn SampleBoundsVisitor::shapeEnter(AlembicNodeT<T> &node, AlembicNode *instance)
{
   bool noData = true;
   bool updated = true;
   
   if (node.sampleBounds(mTime, mTime, mKeepSamples, &updated))
   {
      double blend = 0.0;
      TimeSampleList<Alembic::Abc::IBox3dProperty>::ConstIterator it0, it1;
      
      typename AlembicNodeT<T>::SampleSetType &samples = node.samples();
      
      if (samples.boundsSamples.size() == 1)
      {
         if (updated)
         {
            it0 = samples.boundsSamples.begin();
            node.setSelfBounds(it0->data());
            
            #ifdef _DEBUG
            std::cout << "[SampleBoundsVisitor::shapeEnter] " << node.path() << ": Update constant bounds [" << mTime << ", " << mTime << "]" << std::endl;
            #endif
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "[SampleBoundsVisitor::shapeEnter] " << node.path() << ": Constant bounds up to date [" << mTime << ", " << mTime << "]" << std::endl;
            #endif
         }
         
         noData = false;
      }
      else
      {
         if (updated || fabs(samples.boundsSamples.lastEvaluationTime - mTime) > 0.0001)
         {
            if (samples.boundsSamples.getSamples(mTime, it0, it1, blend))
            {
               if (blend > 0.0)
               {
                  Alembic::Abc::Box3d b0 = it0->data();
                  Alembic::Abc::Box3d b1 = it1->data();
                  
                  node.setSelfBounds(Alembic::Abc::Box3d((1.0 - blend) * b0.min + blend * b1.min,
                                                         (1.0 - blend) * b0.max + blend * b1.max));
               }
               else
               {
                  node.setSelfBounds(it0->data());
               }
               
               samples.boundsSamples.lastEvaluationTime = mTime;
               
               #ifdef _DEBUG
               std::cout << "[SampleBoundsVisitor::shapeEnter] " << node.path() << ": Update animated bounds [" << mTime << ", " << mTime << "]" << std::endl;
               #endif
               
               noData = false;
            }
         }
         else
         {
            #ifdef _DEBUG
            std::cout << "[SampleBoundsVisitor::shapeEnter] " << node.path() << ": Animated bounds up to date [" << mTime << ", " << mTime << "]" << std::endl;
            #endif
            
            noData = false;
         }
      }
   }
   
   if (noData)
   {
      #ifdef _DEBUG
      std::cout << "[SampleBoundsVisitor::shapeEnter] " << node.path() << ": No valid data [" << mTime << ", " << mTime << "]" << std::endl;
      #endif
      
      Alembic::Abc::Box3d box;
      box.makeEmpty();
      
      node.setSelfBounds(box);
   }
   
   if (instance == 0)
   {
      node.updateWorldMatrix();
   }
   
   return AlembicNode::ContinueVisit;
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicMesh &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicSubD &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicPoints &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicCurves &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicNuPatch &node, AlembicNode *instance)
{
   return shapeEnter(node, instance);
}

AlembicNode::VisitReturn SampleBoundsVisitor::enter(AlembicNode &node, AlembicNode *)
{
   if (node.isInstance())
   {
      AlembicNode *m = node.master();
      m->enter(*this, &node);
   }
   
   node.updateWorldMatrix();
   
   return AlembicNode::ContinueVisit;
}
   
void SampleBoundsVisitor::leave(AlembicNode &node, AlembicNode *)
{
   node.updateChildBounds();
   
   std::cout << "\"" << node.path() << "\" updated" << std::endl;
   std::cout << "  World matrix: " << node.worldMatrix() << std::endl;
   std::cout << "  Child bounds: " << node.childBounds().min << " - " << node.childBounds().max << std::endl;
   std::cout << std::endl;
}

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
   std::cout << "SceneHelper [-e expression] [-t frames] [-f framerate] [-m mode] [-a ";
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
   std::vector<double> frames;
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
         
         std::string ranges = argv[i];
         
         double f0, f1, step;
         
         size_t p0 = 0;
         size_t p1 = ranges.find(',', p0);
         
         while (p0 != std::string::npos)
         {
            std::string range = ranges.substr(p0, (p1 == std::string::npos ? ranges.length() : p1) - p0);
            
            #ifdef _DEBUG
            std::cout << "Process frame range: \"" << range << "\"" << std::endl;
            #endif
            
            if (sscanf(range.c_str(), "%lf-%lf:%lf", &f0, &f1, &step) == 3)
            {
               #ifdef _DEBUG
               std::cout << "  " << f0 << " to " << f1 << " by " << step << std::endl;
               #endif
               for (double f=f0; f<=f1; f+=step)
               {
                  frames.push_back(f);
               }
            }
            else if (sscanf(range.c_str(), "%lf-%lf", &f0, &f1) == 2)
            {
               #ifdef _DEBUG
               std::cout << "  " << f0 << " to " << f1 << std::endl;
               #endif
               for (double f=f0; f<=f1; f+=1)
               {
                  frames.push_back(f);
               }
            }
            else if (sscanf(range.c_str(), "%lf", &f0) == 1)
            {
               #ifdef _DEBUG
               std::cout << "  " << f0 << std::endl;
               #endif
               frames.push_back(f0);
            }
            
            p0 = (p1 == std::string::npos ? std::string::npos : p1 + 1);
            p1 = (p0 == std::string::npos ? std::string::npos : ranges.find(',', p0));
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
            // else if (rv > AlembicNode::VisitFilteredFlat)
            // {
            //    mode = AlembicNode::VisitFilteredFlat;
            // }
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
      
      AlembicScene *scene = AlembicSceneCache::Ref(path, AlembicSceneFilter(filter, ""));
      
      if (!scene)
      {
         std::cout << "Not a valid ABC file: \"" << path << "\"" << std::endl;
         return 1;
      }
      
      et = clock();
      std::cout << "Read scene in " << (ts * (et - st)) << " second(s)" << std::endl;
      
      if (frames.size() == 0)
      {
         frames.push_back(1.0);
      }
      
      for (size_t i=0; i<frames.size(); ++i)
      {
         double t = (frames[i] / fps);
         #ifdef _DEBUG
         std::cout << "Process frame " << frames[i] << " (t=" << t << ")" << std::endl;
         #endif
      
         st = clock();
         
         switch (action)
         {
         case Print:
            {
               PrintVisitor print;
               scene->visit(mode, print);   
            }
            break;
         case Bounds:
            {
               SampleBoundsVisitor sampleBounds(t, frames.size() > 1);
               scene->visit(mode, sampleBounds);
            }
            break;
         default:
            break;
         }
         
         et = clock();
         std::cout << "[" << ActionNames[action] << " (t=" << t << ")] Visited graph in " << (ts * (et - st)) << " second(s)" << std::endl;
      }
      
      std::cout << std::endl << "PRESS ENTER KEY TO FINISH" << std::endl;
      getchar();
      
      AlembicSceneCache::Unref(scene);
   }
   
   return 0;
}
