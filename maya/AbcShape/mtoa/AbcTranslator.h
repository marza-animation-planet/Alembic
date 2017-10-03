#ifndef _AbcTranslator_h_
#define _AbcTranslator_h_

#ifdef NAME_PREFIX
#   define PREFIX_NAME(s) NAME_PREFIX s
#else
#   define PREFIX_NAME(s) s
#   define NAME_PREFIX ""
#endif

#include "utils/Version.h"
#include "utils/HashUtils.h"
#include "translators/shape/ShapeTranslator.h"
#include "extension/Extension.h"
#include "AlembicSceneCache.h"
#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <set>

class CAbcTranslator : public CShapeTranslator
{
public:

   CAbcTranslator();
   virtual ~CAbcTranslator();

   virtual AtNode* CreateArnoldNodes();
   virtual void Export(AtNode* atNode);
   virtual bool RequiresMotionData();
   virtual void Init();
   virtual void ExportMotion(AtNode *atNode);
   virtual void RequestUpdate();

   static void* Create();
   static void NodeInitializer(CAbTranslator context);

private:

   void ExportAbc(AtNode *atNode, unsigned int step, bool update=false);
   void ExportProc(AtNode *atNode, unsigned int step, double renderFrame, double sampleFrame);
   void ExportVisibility(AtNode *atNode);
   void ExportSubdivAttribs(AtNode *atNode);
   void ExportMeshAttribs(AtNode *atNode);
   void ExportShader(AtNode *atNode, bool update);

   double GetSampleFrame(unsigned int step);
   void GetFrames(double inRenderFrame, double inSampleFrame,
                  double &outRenderFrame, double &outSampleFrame);
   double GetFPS();

   bool HasParameter(const AtNodeEntry *anodeEntry, const char *param, AtNode *anode=NULL, const char *decl=NULL);

   MPlug FindMayaObjectPlug(const MString &attrName, MStatus* ReturnStatus=NULL) const;

private:

   bool m_motionBlur;
   std::set<unsigned int> m_exportedSteps;
};

#endif
