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
   
   void ReadAlembicAttributes();
   bool ReadFloat3Attribute(Alembic::Abc::ICompoundProperty userProps,
                            Alembic::Abc::ICompoundProperty geomParams,
                            const std::string &name,
                            Alembic::AbcGeom::GeometryScope geoScope,
                            AtPoint &out);
   bool ReadFloatAttribute(Alembic::Abc::ICompoundProperty userProps,
                           Alembic::Abc::ICompoundProperty geomParams,
                           const std::string &name,
                           Alembic::AbcGeom::GeometryScope geoScope,
                           float &out);

private:

   bool m_motionBlur;
   std::string m_abcPath;
   std::string m_objPath;
   bool m_overrideBounds;
   bool m_boundsOverridden;
   double m_renderTime;
   std::string m_overrideBoundsMin;
   std::string m_overrideBoundsMax;
   AtPoint m_min;
   AtPoint m_max;
   AlembicScene *m_scene;
   bool m_padBoundsWithPeakRadius;
   std::string m_peakRadius;
   float m_radiusSclMinMax[3];
   bool m_padBoundsWithPeakWidth;
   std::string m_peakWidth;
   float m_widthSclMinMax[3];
   float m_peakPadding;
   std::string m_promoteToObjAttr;
};

#endif
