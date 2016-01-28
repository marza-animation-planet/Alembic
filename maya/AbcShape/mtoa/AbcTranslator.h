#ifndef _AbcTranslator_h_
#define _AbcTranslator_h_

#ifdef NAME_PREFIX
#   define PREFIX_NAME(s) NAME_PREFIX s
#else
#   define PREFIX_NAME(s) s
#   define NAME_PREFIX ""
#endif

#include "translators/shape/ShapeTranslator.h"
#include "extension/Extension.h"
#include "AlembicSceneCache.h"
#include <maya/MDagPath.h>
#include <maya/MPlug.h>

#include <string>
#include <map>

class CAbcTranslator : public CShapeTranslator
{
public:

   CAbcTranslator();
   virtual ~CAbcTranslator();

   virtual AtNode* CreateArnoldNodes();
   virtual AtNode* Init(CArnoldSession* session, MDagPath &dagPath, MString outputAttr="");
   virtual AtNode* Init(CArnoldSession* session, MObject &object, MString outputAttr="");
   virtual void Export(AtNode* atNode);
   virtual void ExportMotion(AtNode* atNode, unsigned int step);
   virtual void Update(AtNode *atNode);
   virtual void UpdateMotion(AtNode* atNode, unsigned int step);
   virtual bool RequiresMotionData();
   virtual void Delete();

   static void* Create();
   static void NodeInitializer(CAbTranslator context);

private:

   void ExportAbc(AtNode *atNode, unsigned int step, bool update=false);
   void ExportProc(AtNode *atNode, unsigned int step, double renderFrame, double sampleFrame);
   void ExportVisibility(AtNode *atNode);
   void ExportSubdivAttribs(AtNode *atNode);
   void ExportMeshAttribs(AtNode *atNode);
   void ExportBounds(AtNode *atNode, unsigned int step);
   void ExportShader(AtNode *atNode, bool update);
   
   bool IsSingleShape() const;

   float GetSampleFrame(unsigned int step);
   void GetFrames(double inRenderFrame, double inSampleFrame,
                  double &outRenderFrame, double &outSampleFrame);
   double GetFPS();
   MString ToString(double val);
   MString ToString(int val);

   bool HasParameter(const AtNodeEntry *anodeEntry, const char *param, AtNode *anode=NULL, const char *decl=NULL);
   bool ReadFloat3Attribute(Alembic::Abc::ICompoundProperty props, const std::string &name, bool geoParam, AtPoint &out);

private:

   bool m_motionBlur;
   std::string m_abcPath;
   std::string m_objPath;
   bool m_overrideBounds;
   bool m_boundsOverridden;
   double m_renderTime;
   std::string m_overrideBoundsMin;
   std::string m_overrideBoundsMax;
   AlembicScene *m_scene;
};

#endif
