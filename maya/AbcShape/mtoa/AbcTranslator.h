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
   
   void ReadAlembicAttributes(double time);

private:

   bool m_motionBlur;
   std::string m_abcPath;
   std::string m_objPath;
   bool m_computeVelocityExpandedBounds;
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
   double m_renderFrame;
   std::string m_velocity;
   float m_velocityScale;
   std::string m_acceleration;
   std::string m_radius;
   float m_t0; // data 0 time
   std::vector<float> m_p0; // positions data
   std::vector<float> m_v0; // velocity data
   std::vector<float> m_a0; // acceleration data
   float m_t1; // data 1 time
   std::vector<float> m_p1; // positions data
   std::vector<float> m_v1; // velocity data
   std::vector<float> m_a1; // acceleration data
   Alembic::Abc::Box3d m_vb; // velocity expanded box
   double m_sampleFrame;
   double m_sampleTime;
   // For either points or curves
   Alembic::AbcGeom::IFloatGeomParam m_widths; // points/curves built-in width parameter
   float m_maxWidth; 
   float *m_widthAdjust;
};

#endif
