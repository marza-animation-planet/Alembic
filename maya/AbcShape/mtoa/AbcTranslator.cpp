#include "AbcTranslator.h"

#include <ai_msg.h>
#include <ai_nodes.h>
#include <ai_ray.h>

#include <maya/MDagPath.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MGlobal.h>
#include <maya/MBoundingBox.h>
#include <maya/MDGContext.h>

void* CAbcTranslator::Create()
{
   return new CAbcTranslator();
}

void CAbcTranslator::NodeInitializer(CAbTranslator context)
{
   CExtensionAttrHelper helper(context.maya, "polymesh");
   
   CShapeTranslator::MakeCommonAttributes(helper);

   CAttrData data;
   
   // reference
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_outputReference";
   data.shortName = "outref";
   helper.MakeInputBoolean(data);
   
   data.defaultValue.INT = 0;
   data.enums.clear();
   data.enums.append("attributes_then_file");
   data.enums.append("attributes");
   data.enums.append("file");
   data.enums.append("frame");
   data.name = "mtoa_constant_abc_referenceSource";
   data.shortName = "refsrc";
   helper.MakeInputEnum(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_referencePositionName";
   data.shortName = "refpna";
   helper.MakeInputString(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_referenceNormalName";
   data.shortName = "refnna";
   helper.MakeInputString(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_referenceFilename";
   data.shortName = "reffp";
   helper.MakeInputString(data);
   
   data.defaultValue.FLT = 0.0f;
   data.name = "mtoa_constant_abc_referenceFrame";
   data.shortName = "reffrm";
   data.hasMin = false;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   // velocity
   
   data.defaultValue.FLT = 1.0f;
   data.name = "mtoa_constant_abc_velocityScale";
   data.shortName = "velscl";
   data.hasMin = false;
   data.hasSoftMin = true;
   data.softMin.FLT = 0.0f;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.FLT = 10.0f;
   helper.MakeInputFloat(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_velocityName";
   data.shortName = "velan";
   helper.MakeInputString(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_accelerationName";
   data.shortName = "accan";
   helper.MakeInputString(data);
   
   // bounding box
   
   data.defaultValue.FLT = 0.0f;
   data.name = "mtoa_constant_abc_boundsPadding";
   data.shortName = "bndpad";
   data.hasMin = false;
   data.hasSoftMin = true;
   data.softMin.FLT = 0.0f;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.FLT = 100.0f;
   helper.MakeInputFloat(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_useOverrideBounds";
   data.shortName = "uovbnd";
   helper.MakeInputBoolean(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_overrideBoundsMinName";
   data.shortName = "ovbdmi";
   helper.MakeInputString(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_overrideBoundsMaxName";
   data.shortName = "ovbdma";
   helper.MakeInputString(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_padBoundsWithPeakRadius";
   data.shortName = "pkrpad";
   helper.MakeInputBoolean(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_peakRadiusName";
   data.shortName = "pkrdn";
   helper.MakeInputString(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_padBoundsWithPeakWidth";
   data.shortName = "pkwpad";
   helper.MakeInputBoolean(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_peakWidthName";
   data.shortName = "pkwdt";
   helper.MakeInputString(data);
   
   // mesh
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_computeTangents";
   data.shortName = "cmptan";
   helper.MakeInputString(data);
   
   // particles
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_radiusName";
   data.shortName = "radnam";
   helper.MakeInputString(data);
   
   data.defaultValue.FLT = 1.0;
   data.name = "mtoa_constant_abc_radiusScale";
   data.shortName = "radscl";
   data.hasMin = true;
   data.min.FLT = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   data.defaultValue.FLT = 0.0;
   data.name = "mtoa_constant_abc_radiusMin";
   data.shortName = "radmin";
   data.hasMin = true;
   data.min.FLT = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   data.defaultValue.FLT = 1000000.0;
   data.name = "mtoa_constant_abc_radiusMax";
   data.shortName = "radmax";
   data.hasMin = true;
   data.min.FLT = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   // curves
   
   data.defaultValue.BOOL = true;
   data.name = "mtoa_constant_abc_ignoreNurbs";
   data.shortName = "ignnrb";
   helper.MakeInputBoolean(data);
   
   data.defaultValue.INT = 5;
   data.name = "mtoa_constant_abc_nurbsSampleRate";
   data.shortName = "nurbssr";
   data.hasMin = true;
   data.min.INT = 1;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.INT = 10;
   helper.MakeInputInt(data);
   
   data.defaultValue.FLT = 1.0;
   data.name = "mtoa_constant_abc_widthScale";
   data.shortName = "wdtscl";
   data.hasMin = true;
   data.min.FLT = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   data.defaultValue.FLT = 0.0;
   data.name = "mtoa_constant_abc_widthMin";
   data.shortName = "wdtmin";
   data.hasMin = true;
   data.min.FLT = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   data.defaultValue.FLT = 1000000.0;
   data.name = "mtoa_constant_abc_widthMax";
   data.shortName = "wdtmax";
   data.hasMin = true;
   data.min.FLT = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);
   
   // attributes
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_objectAttribs";
   data.shortName = "objatrs";
   helper.MakeInputBoolean(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_primitiveAttribs";
   data.shortName = "pratrs";
   helper.MakeInputBoolean(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_pointAttribs";
   data.shortName = "ptatrs";
   helper.MakeInputBoolean(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_vertexAttribs";
   data.shortName = "vtxatrs";
   helper.MakeInputBoolean(data);
   
   data.defaultValue.INT = 0;
   data.enums.clear();
   data.enums.append("render");
   data.enums.append("shutter");
   data.enums.append("shutter_open");
   data.enums.append("shutter_close");
   data.name = "mtoa_constant_abc_attribsFrame";
   data.shortName = "atrsfrm";
   helper.MakeInputEnum(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_promoteToObjectAttribs";
   data.shortName = "probja";
   helper.MakeInputString(data);
   
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_removeAttribPrefices";
   data.shortName = "rematp";
   helper.MakeInputString(data);
   
   // for multi mode only
   data.stringDefault = "";
   data.name = "mtoa_constant_abc_overrideAttribs";
   data.shortName = "ovatrs";
   helper.MakeInputString(data);
   
   // samples expansion
   
   data.defaultValue.INT = 0;
   data.name = "mtoa_constant_abc_samplesExpandIterations";
   data.shortName = "sampexpiter";
   data.hasMin = true;
   data.min.INT = 0;
   data.hasSoftMin = false;
   data.hasMax = true;
   data.max.INT = 10;
   data.hasSoftMax = false;
   helper.MakeInputInt(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_optimizeSamples";
   data.shortName = "optsamp";
   helper.MakeInputBoolean(data);
   
   // others
   
   // volume
   data.defaultValue.FLT = 0.0f;
   data.name = "aiStepSize";
   data.shortName = "ai_step_size";
   data.hasMin = true;
   data.min.FLT = 0.f;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.FLT = 1.f;
   helper.MakeInputFloat(data);
   
   data.defaultValue.BOOL = false;
   data.name = "mtoa_constant_abc_verbose";
   data.shortName = "vrbexp";
   helper.MakeInputBoolean(data);
}

CAbcTranslator::CAbcTranslator()
   : CShapeTranslator()
   , m_motionBlur(false)
   , m_overrideBounds(false)
   , m_boundsOverridden(false)
   , m_renderTime(0.0)
   , m_scene(0)
   , m_padBoundsWithPeakRadius(false)
   , m_padBoundsWithPeakWidth(false)
   , m_peakPadding(0.0f)
{
   m_radiusSclMinMax[0] = 1.0f;
   m_radiusSclMinMax[1] = 0.0f;
   m_radiusSclMinMax[2] = 1000000.0f;
   
   m_widthSclMinMax[0] = 1.0f;
   m_widthSclMinMax[1] = 0.0f;
   m_widthSclMinMax[2] = 1000000.0f;
}

CAbcTranslator::~CAbcTranslator()
{
   if (m_scene)
   {
      AlembicSceneCache::Unref(m_scene);
   }
}

AtNode* CAbcTranslator::Init(CArnoldSession *session, MDagPath& dagPath, MString outputAttr)
{
   AtNode *rv = CShapeTranslator::Init(session, dagPath, outputAttr);
   m_motionBlur = (IsMotionBlurEnabled(MTOA_MBLUR_DEFORM|MTOA_MBLUR_OBJECT) && IsLocalMotionBlurEnabled());
   return rv;
}

AtNode* CAbcTranslator::Init(CArnoldSession* session, MObject& object, MString outputAttr)
{
   AtNode *rv = CDagTranslator::Init(session, object, outputAttr);
   m_motionBlur = (IsMotionBlurEnabled(MTOA_MBLUR_DEFORM|MTOA_MBLUR_OBJECT) && IsLocalMotionBlurEnabled());
   return rv;
}

AtNode* CAbcTranslator::CreateArnoldNodes()
{
   return AddArnoldNode("procedural");
}

void CAbcTranslator::Export(AtNode *atNode)
{
   ExportAbc(atNode, 0);
}

void CAbcTranslator::ExportMotion(AtNode *atNode, unsigned int step)
{
   ExportAbc(atNode, step);
}

void CAbcTranslator::Update(AtNode *atNode)
{
   ExportAbc(atNode, 0, true);
}

void CAbcTranslator::UpdateMotion(AtNode *atNode, unsigned int step)
{
   ExportAbc(atNode, step, true);
}

bool CAbcTranslator::RequiresMotionData()
{
   return m_motionBlur;
}

bool CAbcTranslator::HasParameter(const AtNodeEntry *anodeEntry, const char *param, AtNode *anode, const char *decl)
{
   if (AiNodeEntryLookUpParameter(anodeEntry, param) != NULL)
   {
      return true;
   }
   else if (anode)
   {
      if (AiNodeLookUpUserParameter(anode, param) != NULL)
      {
         return true;
      }
      else if (decl != NULL)
      {
         return AiNodeDeclare(anode, param, decl);
      }
      else
      {
         return false;
      }
   }
   else
   {
      return false;
   }
}

MString CAbcTranslator::ToString(double val)
{
   MString rv;
   rv += val;
   return rv;
}

MString CAbcTranslator::ToString(int val)
{
   MString rv;
   rv += val;
   return rv;
}

void CAbcTranslator::GetFrames(double inRenderFrame, double inSampleFrame,
                               double &outRenderFrame, double &outSampleFrame)
{
   MPlug pTime = FindMayaObjectPlug("time");
   
   MDGContext ctx0(MTime(inRenderFrame, MTime::uiUnit()));
   MDGContext ctx1(MTime(inSampleFrame, MTime::uiUnit()));
   
   MStatus st;
   MTime t;
   
   t = pTime.asMTime(ctx0, &st);
   outRenderFrame = (st == MS::kSuccess ? t.asUnits(MTime::uiUnit()) : inRenderFrame);
   
   t = pTime.asMTime(ctx1, &st);
   outSampleFrame = (st == MS::kSuccess ? t.asUnits(MTime::uiUnit()) : inSampleFrame);
}

double CAbcTranslator::GetFPS()
{
   MTime::Unit units = MTime::uiUnit();

   switch (units)
   {
   case MTime::kHours:
      return (1.0 / 3600.0);
   case MTime::kMinutes:
      return (1.0 / 60.0);
   case MTime::kSeconds:
      return 1.0;
   case MTime::kMilliseconds:
      return 1000.0;
   case MTime::kGames:
      return 15.0;
   case MTime::kFilm:
      return 24.0;
   case MTime::kPALFrame:
      return 25.0;
   case MTime::kNTSCFrame:
      return 30.0;
   case MTime::kShowScan:
      return 48.0;
   case MTime::kPALField:
      return 50.0;
   case MTime::kNTSCField:
      return 60.0;
   case MTime::k2FPS:
      return 2.0;
   case MTime::k3FPS:
      return 3.0;
   case MTime::k4FPS:
      return 4.0;
   case MTime::k5FPS:
      return 5.0;
   case MTime::k6FPS:
      return 6.0;
   case MTime::k8FPS:
      return 8.0;
   case MTime::k10FPS:
      return 10.0;
   case MTime::k12FPS:
      return 12.0;
   case MTime::k16FPS:
      return 16.0;
   case MTime::k20FPS:
      return 20.0;
   case MTime::k40FPS:
      return 40.0;
   case MTime::k75FPS:
      return 75.0;
   case MTime::k80FPS:
      return 80.0;
   case MTime::k100FPS:
      return 100.0;
   case MTime::k120FPS:
      return 120.0;
   case MTime::k125FPS:
      return 125.0;
   case MTime::k150FPS:
      return 150.0;
   case MTime::k200FPS:
      return 200.0;
   case MTime::k240FPS:
      return 240.0;
   case MTime::k250FPS:
      return 250.0;
   case MTime::k300FPS:
      return 300.0;
   case MTime::k375FPS:
      return 375.0;
   case MTime::k400FPS:
      return 400.0;
   case MTime::k500FPS:
      return 500.0;
   case MTime::k600FPS:
      return 600.0;
   case MTime::k750FPS:
      return 750.0;
   case MTime::k1200FPS:
      return 1200.0;
   case MTime::k1500FPS:
      return 1500.0;
   case MTime::k2000FPS:
      return 2000.0;
   case MTime::k3000FPS:
      return 3000.0;
   case MTime::k6000FPS:
      return 6000.0;
   case MTime::kUserDef:
   case MTime::kInvalid:
   default:
      return 0.0;
   }
}

bool CAbcTranslator::IsSingleShape() const
{
   bool isGpuCache = !strcmp(m_dagPath.node().apiTypeStr(), "gpuCache");
   if (isGpuCache)
   {
      return false;
   }
   else
   {
      bool ignoreXforms = FindMayaObjectPlug("ignoreXforms").asBool();
      int numShapes = FindMayaObjectPlug("numShapes").asInt();
      
      return (ignoreXforms && numShapes == 1);
   }
}

void CAbcTranslator::ExportSubdivAttribs(AtNode *proc)
{
   static bool sInit = true;
   static const char *sAdaptiveErrorName = "subdiv_pixel_error";
   
   if (sInit)
   {
      const AtNodeEntry *polymesh = AiNodeEntryLookUp("polymesh");
      if (polymesh)
      {
         if (AiNodeEntryLookUpParameter(polymesh, "subdiv_adaptive_error") != NULL)
         {
            sAdaptiveErrorName = "subdiv_adaptive_error";
         }
      }
      sInit = true;
   }
   
   const AtNodeEntry *nodeEntry = AiNodeGetNodeEntry(proc);
   MPlug plug;

   plug = FindMayaPlug("aiSubdivType");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_type", proc, "constant INT"))
   {
      AiNodeSetInt(proc, "subdiv_type", plug.asInt());
   }

   plug = FindMayaPlug("aiSubdivIterations");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_iterations", proc, "constant BYTE"))
   {
      AiNodeSetByte(proc, "subdiv_iterations", plug.asInt() & 0xFF);
   }

   plug = FindMayaPlug("aiSubdivAdaptiveMetric");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_adaptive_metric", proc, "constant INT"))
   {
      AiNodeSetInt(proc, "subdiv_adaptive_metric", plug.asInt());
   }

   // starting arnold 4.2.8.0
   plug = FindMayaPlug("aiSubdivAdaptiveSpace");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_adaptive_space", proc, "constant INT"))
   {
      AiNodeSetInt(proc, "subdiv_adaptive_space", plug.asInt());
   }

   // The long name hasn't changed as is still "aiSubdivPixelError"
   // Only shot name was changed from "ai_subdiv_pixel_error" to "ai_subdiv_adaptive_error"
   // It may be though in a future MtoA version where compatibility can be broken
   //   so lookup first for what the name should really be
   plug = FindMayaPlug("aiSubdivAdaptiveError");
   if (plug.isNull())
   {
      plug = FindMayaPlug("aiSubdivPixelError");
   }
   if (!plug.isNull() && HasParameter(nodeEntry, sAdaptiveErrorName, proc, "constant FLOAT"))
   {
      AiNodeSetFlt(proc, sAdaptiveErrorName, plug.asFloat());
   }

   plug = FindMayaPlug("aiSubdivDicingCamera");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_dicing_camera", proc, "constant NODE"))
   {
      MString cameraName = plug.asString();

      AtNode *cameraNode = NULL;

      if (cameraName != "" && cameraName != "Default")
      {
         cameraNode = AiNodeLookUpByName(cameraName.asChar());
      }

      AiNodeSetPtr(proc, "subdiv_dicing_camera", cameraNode);
   }
   
   plug = FindMayaPlug("aiSubdivUvSmoothing");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_uv_smoothing", proc, "constant INT"))
   {
      AiNodeSetInt(proc, "subdiv_uv_smoothing", plug.asInt());
   }

   plug = FindMayaPlug("aiSubdivSmoothDerivs");
   if (!plug.isNull() && HasParameter(nodeEntry, "subdiv_smooth_derivs", proc, "constant BOOL"))
   {
      AiNodeSetBool(proc, "subdiv_smooth_derivs", plug.asBool());
   }
}

void CAbcTranslator::ExportMeshAttribs(AtNode *proc)
{
   const AtNodeEntry *nodeEntry = AiNodeGetNodeEntry(proc);
   MPlug plug;
   
   plug = FindMayaPlug("smoothShading"); // maya shape built-in attribute
   if (!plug.isNull() && HasParameter(nodeEntry, "smoothing", proc, "constant BOOL"))
   {
      AiNodeSetBool(proc, "smoothing", plug.asBool());
   }
   
   plug = FindMayaPlug("doubleSided"); // maya shape built-in attribute
   if (!plug.isNull() && HasParameter(nodeEntry, "sidedness", proc, "constant BYTE"))
   {
      AiNodeSetByte(proc, "sidedness", plug.asBool() ? AI_RAY_ALL : 0);

      // only set invert_normals if doubleSided attribute could be found
      if (!plug.asBool())
      {
         plug = FindMayaPlug("opposite"); // maya shape built-in attribute
         if (!plug.isNull() && HasParameter(nodeEntry, "invert_normals", proc, "constant BOOL"))
         {
            AiNodeSetBool(proc, "invert_normals", plug.asBool());
         }
      }
   }
}

void CAbcTranslator::ExportVisibility(AtNode *proc)
{
   MPlug plug;

   int visibility = AI_RAY_ALL;

   plug = FindMayaPlug("castsShadows"); // maya shape built-in attribute
   if (!plug.isNull() && !plug.asBool())
   {
      visibility &= ~AI_RAY_SHADOW;
   }

   plug = FindMayaPlug("primaryVisibility"); // maya shape built-in attribute
   if (!plug.isNull() && !plug.asBool())
   {
      visibility &= ~AI_RAY_CAMERA;
   }

   plug = FindMayaPlug("visibleInReflections"); // maya shape built-in attribute
   if (!plug.isNull() && !plug.asBool())
   {
      visibility &= ~AI_RAY_REFLECTED;
   }

   plug = FindMayaPlug("visibleInRefractions"); // maya shape built-in attribute
   if (!plug.isNull() && !plug.asBool())
   {
      visibility &= ~AI_RAY_REFRACTED;
   }

   plug = FindMayaPlug("aiVisibleInDiffuse");
   if (!plug.isNull() && !plug.asBool())
   {
      visibility &= ~AI_RAY_DIFFUSE;
   }

   plug = FindMayaPlug("aiVisibleInGlossy");
   if (!plug.isNull() && !plug.asBool())
   {
      visibility &= ~AI_RAY_GLOSSY;
   }

   AiNodeSetByte(proc, "visibility", visibility);
   
   plug = FindMayaPlug("aiSelfShadows");
   if (!plug.isNull()) AiNodeSetBool(proc, "self_shadows", plug.asBool());

   plug = FindMayaPlug("aiOpaque");
   if (!plug.isNull()) AiNodeSetBool(proc, "opaque", plug.asBool());

   plug = FindMayaPlug("receiveShadows");
   if (!plug.isNull()) AiNodeSetBool(proc, "receive_shadows", plug.asBool());

   // Sub-Surface Scattering
   //plug = FindMayaPlug("aiSssSampleDistribution");
   //if (!plug.isNull()) AiNodeSetInt(proc, "sss_sample_distribution", plug.asInt());

   //plug = FindMayaPlug("aiSssSampleSpacing");
   //if (!plug.isNull()) AiNodeSetFlt(proc, "sss_sample_spacing", plug.asFloat());
   
   const AtNodeEntry *nodeEntry = AiNodeGetNodeEntry(proc);
   
   plug = FindMayaPlug("aiSssSetname");
   //if (!plug.isNull() && HasParameter(AiNodeGetNodeEntry(proc), "sss_setname", proc, "constant STRING")) AiNodeSetStr(proc, "sss_setname", plug.asString().asChar());
   if (!plug.isNull() && plug.asString().length() > 0)
   {
      if (HasParameter(nodeEntry, "sss_setname", proc, "constant STRING"))
      {
         AiNodeSetStr(proc, "sss_setname", plug.asString().asChar());
      }
   }
   
   plug = FindMayaPlug("aiMatte");
   if (!plug.isNull() && HasParameter(nodeEntry, "matte", proc, "constant BOOL"))
   {
      AiNodeSetBool(proc, "matte", plug.asBool());
   }
}

void CAbcTranslator::ExportShader(AtNode *proc, bool update)
{
   const AtNodeEntry *nodeEntry = AiNodeGetNodeEntry(proc);
   MPlug plug;
   MFnDependencyNode shadingEngine, masterShadingEngine;
   
   MPlug shadingGroupPlug = GetNodeShadingGroup(m_dagPath.node(), (m_dagPath.isInstanced() ? m_dagPath.instanceNumber() : 0));
   
   if (!shadingGroupPlug.isNull())
   {
      shadingEngine.setObject(shadingGroupPlug.node());
      
      // Surface shader
      AtNode *shader = ExportNode(shadingGroupPlug);
      
      if (shader != NULL)
      {
         AiNodeSetPtr(proc, "shader", shader);
         
         if (AiNodeLookUpUserParameter(proc, "mtoa_shading_groups") == 0)
         {
            AiNodeDeclare(proc, "mtoa_shading_groups", "constant ARRAY NODE");
            AiNodeSetArray(proc, "mtoa_shading_groups", AiArrayConvert(1, 1, AI_TYPE_NODE, &shader));
         }
      }
   }

   MDagPath masterDag;
   if (DoIsMasterInstance(m_dagPath, masterDag))
   {
      masterDag = m_dagPath;
   }
   
   shadingGroupPlug = GetNodeShadingGroup(masterDag.node(), (masterDag.isInstanced() ? masterDag.instanceNumber() : 0));
   if (!shadingGroupPlug.isNull())
   {
      masterShadingEngine.setObject(shadingGroupPlug.node());
   }
   
   float dispPadding = 0.0f;
   float dispZeroValue = 0.0f;
   float dispHeight = 1.0f;
   bool dispAutobump = false;
   bool outputDispPadding = false;
   bool outputDispHeight = false;
   bool outputDispAutobump = false;
   bool outputDispZeroValue = false;

   plug = FindMayaPlug("aiDispHeight");
   if (!plug.isNull())
   {
      outputDispHeight = true;
      dispHeight = plug.asFloat();
   }

   plug = FindMayaPlug("aiDispPadding");
   if (!plug.isNull())
   {
      outputDispPadding = true;
      dispPadding = plug.asFloat();
   }

   plug = FindMayaPlug("aiDispZeroValue");
   if (!plug.isNull())
   {
      outputDispZeroValue = true;
      dispZeroValue = plug.asFloat();
   }

   plug = FindMayaPlug("aiDispAutobump");
   if (!plug.isNull())
   {
      outputDispAutobump = true;
      dispAutobump = plug.asBool();
   }

   if (masterShadingEngine.object() != MObject::kNullObj)
   {
      MPlugArray shaderConns;

      MPlug shaderPlug = masterShadingEngine.findPlug("displacementShader");

      shaderPlug.connectedTo(shaderConns, true, false);

      if (shaderConns.length() > 0)
      {
         MFnDependencyNode dispNode(shaderConns[0].node());

         plug = dispNode.findPlug("aiDisplacementPadding");
         if (!plug.isNull())
         {
            outputDispPadding = true;
            dispPadding = MAX(dispPadding, plug.asFloat());
         }
         
         plug = dispNode.findPlug("aiDisplacementAutoBump");
         if (!plug.isNull())
         {
            outputDispAutobump = true;
            dispAutobump = dispAutobump || plug.asBool();
         }

         if (HasParameter(nodeEntry, "disp_map", proc, "constant ARRAY NODE"))
         {
            AtNode *dispImage = ExportNode(shaderConns[0]);
            AiNodeSetArray(proc, "disp_map", AiArrayConvert(1, 1, AI_TYPE_NODE, &dispImage));
         }
      }
   }

   if (outputDispHeight && HasParameter(nodeEntry, "disp_height", proc, "constant FLOAT"))
   {
      AiNodeSetFlt(proc, "disp_height", dispHeight);
   }
   if (outputDispZeroValue && HasParameter(nodeEntry, "disp_zero_value", proc, "constant FLOAT"))
   {
      AiNodeSetFlt(proc, "disp_zero_value", dispZeroValue);
   }
   if (outputDispPadding && HasParameter(nodeEntry, "disp_padding", proc, "constant FLOAT"))
   {
      AiNodeSetFlt(proc, "disp_padding", dispPadding);
   }
   if (outputDispAutobump && HasParameter(nodeEntry, "disp_autobump", proc, "constant BOOL"))
   {
      AiNodeSetBool(proc, "disp_autobump", dispAutobump);
   }
}

bool CAbcTranslator::ReadFloat3Attribute(Alembic::Abc::ICompoundProperty userProps,
                                         Alembic::Abc::ICompoundProperty geomParams,
                                         const std::string &name,
                                         Alembic::AbcGeom::GeometryScope geoScope,
                                         AtPoint &out)
{
   bool rv = false;
   
   if (geomParams.valid())
   {
      const Alembic::AbcCoreAbstract::PropertyHeader *header = geomParams.getPropertyHeader(name);
   
      if (header && Alembic::AbcGeom::GetGeometryScope(header->getMetaData()) == geoScope)
      {
         if (Alembic::AbcGeom::IV3dGeomParam::matches(*header))
         {
            TimeSample<Alembic::AbcGeom::IV3dGeomParam> sampler;
            Alembic::AbcGeom::IV3dGeomParam prop(geomParams, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::AbcGeom::IV3dGeomParam::Sample::samp_ptr_type vl = sampler.data().getVals();
            
            if (vl && vl->size() >= 1)
            {
               Alembic::Abc::V3d v = (*vl)[0];
               
               out.x = float(v.x);
               out.y = float(v.y);
               out.z = float(v.z);
               
               rv = true;
            }
         }
         else if (Alembic::AbcGeom::IV3fGeomParam::matches(*header))
         {
            TimeSample<Alembic::AbcGeom::IV3fGeomParam> sampler;
            Alembic::AbcGeom::IV3fGeomParam prop(geomParams, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::AbcGeom::IV3fGeomParam::Sample::samp_ptr_type vl = sampler.data().getVals();
            
            if (vl && vl->size() >= 1)
            {
               Alembic::Abc::V3f v = (*vl)[0];
               
               out.x = v.x;
               out.y = v.y;
               out.z = v.z;
               
               rv = true;
            }
         }
         else if (Alembic::AbcGeom::IP3dGeomParam::matches(*header))
         {
            TimeSample<Alembic::AbcGeom::IP3dGeomParam> sampler;
            Alembic::AbcGeom::IP3dGeomParam prop(geomParams, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::AbcGeom::IP3dGeomParam::Sample::samp_ptr_type vl = sampler.data().getVals();
            
            if (vl && vl->size() >= 1)
            {
               Alembic::Abc::V3d v = (*vl)[0];
               
               out.x = float(v.x);
               out.y = float(v.y);
               out.z = float(v.z);
               
               rv = true;
            }
         }
         else if (Alembic::AbcGeom::IP3fGeomParam::matches(*header))
         {
            TimeSample<Alembic::AbcGeom::IP3fGeomParam> sampler;
            Alembic::AbcGeom::IP3fGeomParam prop(geomParams, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::AbcGeom::IP3fGeomParam::Sample::samp_ptr_type vl = sampler.data().getVals();
            
            if (vl && vl->size() >= 1)
            {
               Alembic::Abc::V3f v = (*vl)[0];
               
               out.x = v.x;
               out.y = v.y;
               out.z = v.z;
               
               rv = true;
            }
         }
      }
   }
   
   if (!rv && userProps.valid())
   {
      const Alembic::AbcCoreAbstract::PropertyHeader *header = userProps.getPropertyHeader(name);
      
      if (header)
      {
         if (Alembic::Abc::IV3dProperty::matches(*header))
         {
            TimeSample<Alembic::Abc::IV3dProperty> sampler;
            Alembic::Abc::IV3dProperty prop(userProps, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::Abc::V3d v = sampler.data();
            
            out.x = float(v.x);
            out.y = float(v.y);
            out.z = float(v.z);
            
            rv = true;
         }
         else if (Alembic::Abc::IV3fProperty::matches(*header))
         {
            TimeSample<Alembic::Abc::IV3fProperty> sampler;
            Alembic::Abc::IV3fProperty prop(userProps, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::Abc::V3f v = sampler.data();
            
            out.x = v.x;
            out.y = v.y;
            out.z = v.z;
            
            rv = true;
         }
         else if (Alembic::Abc::IP3dProperty::matches(*header))
         {
            TimeSample<Alembic::Abc::IP3dProperty> sampler;
            Alembic::Abc::IP3dProperty prop(userProps, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::Abc::V3d v = sampler.data();
            
            out.x = float(v.x);
            out.y = float(v.y);
            out.z = float(v.z);
            
            rv = true;
         }
         else if (Alembic::Abc::IP3fProperty::matches(*header))
         {
            TimeSample<Alembic::Abc::IP3fProperty> sampler;
            Alembic::Abc::IP3fProperty prop(userProps, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::Abc::V3f v = sampler.data();
            
            out.x = v.x;
            out.y = v.y;
            out.z = v.z;
            
            rv = true;
         }
      }
   }
   
   return rv;
}

bool CAbcTranslator::ReadFloatAttribute(Alembic::Abc::ICompoundProperty userProps,
                                        Alembic::Abc::ICompoundProperty geomParams,
                                        const std::string &name,
                                        Alembic::AbcGeom::GeometryScope geoScope,
                                        float &out)
{
   bool rv = false;
   
   if (geomParams.valid())
   {
      const Alembic::AbcCoreAbstract::PropertyHeader *header = geomParams.getPropertyHeader(name);
      
      if (header && Alembic::AbcGeom::GetGeometryScope(header->getMetaData()) == geoScope)
      {
         if (Alembic::AbcGeom::IFloatGeomParam::matches(*header))
         {
            TimeSample<Alembic::AbcGeom::IFloatGeomParam> sampler;
            Alembic::AbcGeom::IFloatGeomParam prop(geomParams, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::AbcGeom::IFloatGeomParam::Sample::samp_ptr_type vl = sampler.data().getVals();
            
            if (vl && vl->size() >= 1)
            {
               out = (*vl)[0];
               
               rv = true;
            }
         }
         else if (Alembic::AbcGeom::IDoubleGeomParam::matches(*header))
         {
            TimeSample<Alembic::AbcGeom::IDoubleGeomParam> sampler;
            Alembic::AbcGeom::IDoubleGeomParam prop(geomParams, name);
            
            sampler.get(prop, m_renderTime);
            
            Alembic::AbcGeom::IDoubleGeomParam::Sample::samp_ptr_type vl = sampler.data().getVals();
            
            if (vl && vl->size() >= 1)
            {
               out = (float) (*vl)[0];
               
               rv = true;
            }
         }
      }
   }
   
   if (!rv && userProps.valid())
   {
      const Alembic::AbcCoreAbstract::PropertyHeader *header = userProps.getPropertyHeader(name);
      
      if (header)
      {
         if (Alembic::Abc::IFloatProperty::matches(*header))
         {
            TimeSample<Alembic::Abc::IFloatProperty> sampler;
            Alembic::Abc::IFloatProperty prop(userProps, name);
            
            sampler.get(prop, m_renderTime);
            
            out = sampler.data();
            
            rv = true;
         }
         else if (Alembic::Abc::IDoubleProperty::matches(*header))
         {
            TimeSample<Alembic::Abc::IDoubleProperty> sampler;
            Alembic::Abc::IDoubleProperty prop(userProps, name);
            
            sampler.get(prop, m_renderTime);
            
            out = (float) sampler.data();
            
            rv = true;
         }
      }
   }
   
   return rv;
}

void CAbcTranslator::ReadAlembicAttributes()
{
   bool promoteMin = false;
   bool promoteMax = false;
   bool promotePeakRadius = false;
   bool promotePeakWidth = false;
   
   if (m_overrideBounds)
   {
      if (m_overrideBoundsMin.length() == 0)
      {
         m_overrideBoundsMin = "overrideBoundsMin";
      }
      
      if (m_overrideBoundsMax.length() == 0)
      {
         m_overrideBoundsMax = "overrideBoundsMax";
      }
   }
   
   if (m_padBoundsWithPeakRadius && m_peakRadius.length() == 0)
   {
      m_peakRadius = "peakRadius";
   }
   
   if (m_padBoundsWithPeakWidth && m_peakWidth.length() == 0)
   {
      m_peakWidth = "peakWidth";
   }
   
   size_t i = m_promoteToObjAttr.find_first_not_of(" ");
   size_t j = 0;
   
   while (i != std::string::npos)
   {
      j = m_promoteToObjAttr.find(' ', i);
      
      std::string name = (j != std::string::npos ? m_promoteToObjAttr.substr(i, j - i) : m_promoteToObjAttr.substr(i));
      
      if (m_overrideBounds && name == m_overrideBoundsMin)
      {
         promoteMin = true;
      }
      else if (m_overrideBounds && name == m_overrideBoundsMax)
      {
         promoteMax = true;
      }
      else if (m_padBoundsWithPeakRadius && name == m_peakRadius)
      {
         promotePeakRadius = true;
      }
      else if (m_padBoundsWithPeakWidth && name == m_peakWidth)
      {
         promotePeakWidth = true;
      }
      
      if (j != std::string::npos)
      {
         i = m_promoteToObjAttr.find_first_not_of(" ", j + 1);
      }
      else
      {
         i = j;
      }
   }
   
   if (m_scene)
   {
      AlembicSceneCache::Unref(m_scene);
      m_scene = 0;
   }
   
   AlembicSceneFilter filter(m_objPath, "");
   
   m_scene = AlembicSceneCache::Ref(m_abcPath, filter);
   
   if (m_scene)
   {
      AlembicNode *node = m_scene->find(m_objPath);
      
      if (node)
      {
         Alembic::Abc::IObject iobj = node->object();
         Alembic::AbcGeom::IGeomBaseObject tobj(iobj, Alembic::Abc::kWrapExisting);
         Alembic::Abc::ICompoundProperty emptyProp;
         Alembic::Abc::ICompoundProperty userProps = tobj.getSchema().getUserProperties();
         Alembic::Abc::ICompoundProperty geomParams = tobj.getSchema().getArbGeomParams();
         
         if (m_overrideBounds)
         {
            AtPoint min, max;
            
            bool hasMin = ReadFloat3Attribute(userProps, geomParams, m_overrideBoundsMin, Alembic::AbcGeom::kConstantScope, min);
            if (!hasMin && promoteMin)
            {
               MGlobal::displayInfo(MString("[mzAbcShapeMtoa] Check for promoted attribute \"") + m_overrideBoundsMin.c_str() + MString("\". [") + m_dagPath.partialPathName() + "]");
               hasMin = ReadFloat3Attribute(emptyProp, geomParams, m_overrideBoundsMin, Alembic::AbcGeom::kUniformScope, min);
            }
            if (hasMin)
            {
               AtPoint max;
               
               bool hasMax = ReadFloat3Attribute(userProps, geomParams, m_overrideBoundsMax, Alembic::AbcGeom::kConstantScope, max);
               if (!hasMax && promoteMax)
               {
                  MGlobal::displayInfo(MString("[mzAbcShapeMtoa] Check for promoted attribute \"") + m_overrideBoundsMax.c_str() + MString("\". [") + m_dagPath.partialPathName() + "]");
                  hasMax = ReadFloat3Attribute(emptyProp, geomParams, m_overrideBoundsMax, Alembic::AbcGeom::kUniformScope, max);
               }
               if (hasMax)
               {
                  MGlobal::displayInfo("[mzAbcShapeMtoa] Use bounds overrides contained in alembic file. [" + m_dagPath.partialPathName() + "]");
                  m_min = min;
                  m_max = max;
                  m_boundsOverridden = true;
               }
               else
               {
                  MGlobal::displayWarning(MString("Ignore bounds override: No object (or promoted) property named \"") + m_overrideBoundsMax.c_str() + MString("\" found in alembic file. [" + m_dagPath.partialPathName() + "]"));
               }
            }
            else
            {
               MGlobal::displayWarning(MString("Ignore bounds override: No object (or promoted) property named \"") + m_overrideBoundsMin.c_str() + MString("\" found in alembic file. [" + m_dagPath.partialPathName() + "]"));
            }
         }
         
         float peakRadius = 0.0f;
         if (m_padBoundsWithPeakRadius)
         {
            bool hasPeakRadius = ReadFloatAttribute(userProps, geomParams, m_peakRadius, Alembic::AbcGeom::kConstantScope, peakRadius);
            if (!hasPeakRadius && promotePeakRadius)
            {
               MGlobal::displayInfo(MString("[mzAbcShapeMtoa] Check for promoted attribute \"") + m_peakRadius.c_str() + MString("\". [") + m_dagPath.partialPathName() + "]");
               hasPeakRadius = ReadFloatAttribute(emptyProp, geomParams, m_peakRadius, Alembic::AbcGeom::kUniformScope, peakRadius);
            }
            if (hasPeakRadius)
            {
               MGlobal::displayInfo("[mzAbcShapeMtoa] Pad bounds with '" + MString(m_peakRadius.c_str()) + "' alembic attribute value. [" + m_dagPath.partialPathName() + "]");
               peakRadius *= m_radiusSclMinMax[0];
               if (peakRadius < m_radiusSclMinMax[1]) peakRadius = m_radiusSclMinMax[1];
               if (peakRadius > m_radiusSclMinMax[2]) peakRadius = m_radiusSclMinMax[2];
               m_peakPadding += peakRadius;
            }
         }
         
         float peakWidth = 0.0f;
         if (m_padBoundsWithPeakWidth)
         {
            bool hasPeakWidth = ReadFloatAttribute(userProps, geomParams, m_peakWidth, Alembic::AbcGeom::kConstantScope, peakWidth);
            if (!hasPeakWidth && promotePeakWidth)
            {
               MGlobal::displayInfo(MString("[mzAbcShapeMtoa] Check for promoted attribute \"") + m_peakWidth.c_str() + MString("\". [") + m_dagPath.partialPathName() + "]");
               hasPeakWidth = ReadFloatAttribute(emptyProp, geomParams, m_peakWidth, Alembic::AbcGeom::kUniformScope, peakWidth);
            }
            if (hasPeakWidth)
            {
               MGlobal::displayInfo("[mzAbcShapeMtoa] Pad bounds with '" + MString(m_peakWidth.c_str()) + "' alembic attribute value. [" + m_dagPath.partialPathName() + "]");
               peakWidth *= m_widthSclMinMax[0];
               if (peakWidth < m_widthSclMinMax[1]) peakWidth = m_widthSclMinMax[1];
               if (peakWidth > m_widthSclMinMax[2]) peakWidth = m_widthSclMinMax[2];
               m_peakPadding += 0.5f * peakWidth;
            }
         }
      }
      else
      {
         MGlobal::displayWarning(MString("Ignore bounds override: Could not find object in alembic file. [" + m_dagPath.partialPathName() + "]"));
      }
   }
   else
   {
      MGlobal::displayWarning(MString("Ignore bounds override: Could not read alembic file. [" + m_dagPath.partialPathName() + "]"));
   }
}

void CAbcTranslator::ExportBounds(AtNode *proc, unsigned int step)
{
   MFnDagNode node(m_dagPath.node());
   
   bool singleShape = IsSingleShape();
   bool transformBlur = IsMotionBlurEnabled(MTOA_MBLUR_OBJECT) && IsLocalMotionBlurEnabled();
   bool deformBlur = IsMotionBlurEnabled(MTOA_MBLUR_DEFORM) && IsLocalMotionBlurEnabled();

   MPoint bmin;
   MPoint bmax;
   
   MPlug pbmin = node.findPlug("outBoxMin");
   MPlug pbmax = node.findPlug("outBoxMax");
   
   MFnNumericData dbmin(pbmin.asMObject());
   MFnNumericData dbmax(pbmax.asMObject());
   
   dbmin.getData(bmin.x, bmin.y, bmin.z);
   dbmax.getData(bmax.x, bmax.y, bmax.z);
   
   if (step == 0)
   {
      m_peakPadding = 0.0f;
      m_overrideBounds = false;
      m_padBoundsWithPeakWidth = false;
      m_padBoundsWithPeakRadius = false;
      m_boundsOverridden = false;
      
      if (IsSingleShape())
      {
         bool supportBoundsOverrides = false;
         
         MPlug plug = FindMayaObjectPlug("inCustomFrame");
         if (!plug.isNull())
         {
            plug.setDouble(m_renderFrame);
            plug = FindMayaObjectPlug("outCustomTime");
            m_renderTime = plug.asDouble();
            supportBoundsOverrides = true;
         }
         else
         {
            plug = FindMayaObjectPlug("outSampleTime");
            if (!plug.isNull())
            {
               m_renderTime = plug.asDouble();
               supportBoundsOverrides = true;
            }
         }
         
         if (supportBoundsOverrides)
         {
            m_overrideBounds = FindMayaPlug("mtoa_constant_abc_useOverrideBounds").asBool();
            m_padBoundsWithPeakRadius = FindMayaPlug("mtoa_constant_abc_padBoundsWithPeakRadius").asBool();
            m_padBoundsWithPeakWidth = FindMayaPlug("mtoa_constant_abc_padBoundsWithPeakWidth").asBool();
            
            if (m_overrideBounds || m_padBoundsWithPeakWidth || m_padBoundsWithPeakRadius)
            {
               char msg[1024];
               sprintf(msg, "[AbcShapeMtoa] Sample bounds override attributes at frame %f (time = %f)", m_renderFrame, m_renderTime);
               MGlobal::displayInfo(msg);
               
               ReadAlembicAttributes();
            }
         }
         
         AiNodeSetBool(proc, "load_at_init", false);
         AiNodeSetPnt(proc, "min", static_cast<float>(bmin.x), static_cast<float>(bmin.y), static_cast<float>(bmin.z));
         AiNodeSetPnt(proc, "max", static_cast<float>(bmax.x), static_cast<float>(bmax.y), static_cast<float>(bmax.z));
      }
      else
      {
         AiNodeSetBool(proc, "load_at_init", true);
         AiNodeSetPnt(proc, "min", 0.0f, 0.0f, 0.0f);
         AiNodeSetPnt(proc, "max", 0.0f, 0.0f, 0.0f);
      }
   }
   else if (singleShape && (transformBlur || deformBlur))
   {
      if (m_boundsOverridden)
      {
         AiNodeSetPnt(proc, "min", m_min.x, m_min.y, m_min.z);
         AiNodeSetPnt(proc, "max", m_max.x, m_max.y, m_max.z);
         
         return;
      }
      
      AtPoint cmin = AiNodeGetPnt(proc, "min");
      AtPoint cmax = AiNodeGetPnt(proc, "max");
      
      if (bmin.x < cmin.x)
         cmin.x = static_cast<float>(bmin.x);
      if (bmin.y < cmin.y)
         cmin.y = static_cast<float>(bmin.y);
      if (bmin.z < cmin.z)
         cmin.z = static_cast<float>(bmin.z);
      if (bmax.x > cmax.x)
         cmax.x = static_cast<float>(bmax.x);
      if (bmax.y > cmax.y)
         cmax.y = static_cast<float>(bmax.y);
      if (bmax.z > cmax.z)
         cmax.z = static_cast<float>(bmax.z);
      
      AiNodeSetPnt(proc, "min", cmin.x, cmin.y, cmin.z);
      AiNodeSetPnt(proc, "max", cmax.x, cmax.y, cmax.z);
   }
}

void CAbcTranslator::ExportProc(AtNode *proc, unsigned int step, double renderFrame, double sampleFrame)
{
   bool transformBlur = IsMotionBlurEnabled(MTOA_MBLUR_OBJECT) && IsLocalMotionBlurEnabled();
   bool deformBlur = IsMotionBlurEnabled(MTOA_MBLUR_DEFORM) && IsLocalMotionBlurEnabled();
   
   bool ignoreXforms = FindMayaObjectPlug("ignoreXforms").asBool();
   bool ignoreInstances = FindMayaObjectPlug("ignoreInstances").asBool();
   bool ignoreVisibility = FindMayaObjectPlug("ignoreVisibility").asBool();
   
   MPlug plug = FindMayaPlug("mtoa_constant_abc_relativeSamples");
   
   bool relativeSamples = (plug.isNull() ? true : plug.asBool());
   
   bool isGpuCache = !strcmp(m_dagPath.node().apiTypeStr(), "gpuCache");
   
   MString abcfile = FindMayaObjectPlug(isGpuCache ? "cacheFileName" : "filePath").asString();
   MString objpath = FindMayaObjectPlug(isGpuCache ? "cacheGeomPath" : "objectExpression").asString();
   
   if (isGpuCache)
   {
      // replace "|" by "/" in objpath
      std::string tmp = objpath.asChar();
      size_t p0 = 0;
      size_t p1 = tmp.find('|', p0);
      while (p1 != std::string::npos)
      {
         tmp[p1] = '/';
         p0 = p1;
         p1 = tmp.find('|', p0);
      }
      objpath = tmp.c_str();
   }
   
   m_abcPath = abcfile.asChar();
   m_objPath = objpath.asChar();
   m_renderFrame = renderFrame;
   
   // ---

   if (step == 0)
   {
      AiNodeSetStr(proc, "dso", "abcproc");

      MString data, tmp;
      
      data += "-filename " + abcfile;
      if (objpath.length() > 0)
      {
         data += " -objectpath " + objpath;
      }
      data += " -fps " + ToString(GetFPS());
      data += " -frame " + ToString(renderFrame);
      
      if (!isGpuCache)
      {
         plug = FindMayaObjectPlug("speed");
         if (!plug.isNull())
         {
            data += " -speed " + ToString(plug.asFloat());
         }
         
         plug = FindMayaObjectPlug("offset");
         if (!plug.isNull())
         {
            data += " -offset " + ToString(plug.asFloat());
         }
         
         plug = FindMayaObjectPlug("preserveStartFrame");
         if (!plug.isNull() && plug.asBool())
         {
            data += " -preservestartframe";
         }
         
         plug = FindMayaObjectPlug("startFrame");
         if (!plug.isNull())
         {
            data += " -startframe " + ToString(plug.asFloat());
         }
         
         plug = FindMayaObjectPlug("endFrame");
         if (!plug.isNull())
         {
            data += " -endframe " + ToString(plug.asFloat());
         }
         
         plug = FindMayaObjectPlug("cycleType");
         if (!plug.isNull())
         {
            data += " -cycle " + ToString(plug.asInt());
         }
         
         if (!deformBlur)
         {
            data += " -ignoredeformblur";
         }
         
         if (!transformBlur)
         {
            data += " -ignoretransformblur";
         }
         
         if (ignoreXforms)
         {
            data += " -ignoretransforms";
         }
         
         if (ignoreInstances)
         {
            data += " -ignoreinstances";
         }
         
         if (ignoreVisibility)
         {
            data += " -ignorevisibility";
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_outputReference");
      if (!plug.isNull() && plug.asBool())
      {
         data += " -outputreference";
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_referenceSource");
      if (!plug.isNull())
      {
         switch (plug.asInt())
         {
         case 0:
            data += " -referencesource attributes_then_file"; break;
         case 1:
            data += " -referencesource attributes"; break;
         case 2:
            data += " -referencesource file"; break;
         case 3:
            data += " -referencesource frame"; break;
         default:
            break;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_referenceFilename");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -referencefilename " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_referencePositionName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -referencepositionname " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_referenceNormalName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -referencenormalname " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_referenceFrame");
      if (!plug.isNull())
      {
         data += " -referenceframe " + ToString(plug.asFloat());
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_computeTangents");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -computetangents " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_radiusName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -radiusname " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_radiusScale");
      if (!plug.isNull())
      {
         m_radiusSclMinMax[0] = plug.asFloat();
         data += " -radiusscale " + ToString(m_radiusSclMinMax[0]);
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_radiusMin");
      if (!plug.isNull())
      {
         m_radiusSclMinMax[1] = plug.asFloat();
         data += " -radiusmin " + ToString(m_radiusSclMinMax[1]);
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_radiusMax");
      if (!plug.isNull())
      {
         m_radiusSclMinMax[2] = plug.asFloat();
         data += " -radiusmax " + ToString(m_radiusSclMinMax[2]);
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_peakRadiusName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -peakradiusname " + tmp;
            m_peakRadius = tmp.asChar();
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_ignoreNurbs");
      if (!plug.isNull() && plug.asBool())
      {
         data += " -ignorenurbs";
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_nurbsSampleRate");
      if (!plug.isNull())
      {
         data += " -nurbssamplerate " + ToString(plug.asInt());
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_widthScale");
      if (!plug.isNull())
      {
         m_widthSclMinMax[0] = plug.asFloat();
         data += " -widthscale " + ToString(m_widthSclMinMax[0]);
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_widthMin");
      if (!plug.isNull())
      {
         m_widthSclMinMax[1] = plug.asFloat();
         data += " -widthmin " + ToString(m_widthSclMinMax[1]);
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_widthMax");
      if (!plug.isNull())
      {
         m_widthSclMinMax[2] = plug.asFloat();
         data += " -widthmax " + ToString(m_widthSclMinMax[2]);
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_peakWidthName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -peakwidthname " + tmp;
            m_peakWidth = tmp.asChar();
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_overrideBoundsMinName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -overrideboundsminname " + tmp;
            m_overrideBoundsMin = tmp.asChar();
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_overrideBoundsMaxName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -overrideboundsmaxname " + tmp;
            m_overrideBoundsMax = tmp.asChar();
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_promoteToObjectAttribs");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -promotetoobjectattribs " + tmp;
            m_promoteToObjAttr = tmp.asChar();
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_overrideAttribs");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -overrideattribs " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_removeAttribPrefices");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -removeattribprefices " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_objectAttribs");
      if (!plug.isNull() && plug.asBool())
      {
         data += " -objectattribs";
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_primitiveAttribs");
      if (!plug.isNull() && plug.asBool())
      {
         data += " -primitiveattribs";
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_pointAttribs");
      if (!plug.isNull() && plug.asBool())
      {
         data += " -pointAttribs";
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_vertexAttribs");
      if (!plug.isNull() && plug.asBool())
      {
         data += " -vertexattribs";
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_attribsFrame");
      if (!plug.isNull())
      {
         switch (plug.asInt())
         {
         case 0:
            data += " -attribsframe render"; break;
         case 1:
            data += " -attribsframe shutter"; break;
         case 2:
            data += " -attribsframe shutter_open"; break;
         case 3:
            data += " -attribsframe shutter_close"; break;
         default:
            break;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_samplesExpandIterations");
      if (!plug.isNull())
      {
         int expIter = plug.asInt();
         data += " -samplesexpanditerations " + ToString(expIter);
         
         if (expIter > 0)
         {
            plug = FindMayaPlug("mtoa_constant_abc_optimizeSamples");
            if (!plug.isNull() && plug.asBool())
            {
               data += " -optimizesamples";
            }
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_velocityScale");
      if (!plug.isNull())
      {
         data += " -velocityscale " + ToString(plug.asFloat());
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_velocityName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -velocityname " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_accelerationName");
      if (!plug.isNull())
      {
         tmp = plug.asString();
         if (tmp.numChars() > 0)
         {
            data += " -accelerationname " + tmp;
         }
      }
      
      plug = FindMayaPlug("mtoa_constant_abc_verbose");
      if (!plug.isNull())
      {
         data += " -verbose";
      }
      
      if (deformBlur || transformBlur)
      {
         if (relativeSamples)
         {
            data += " -relativesamples";
         }
         data += " -samples " + ToString(relativeSamples ? sampleFrame - renderFrame : sampleFrame);
      }

      AiNodeSetStr(proc, "data", data.asChar());
   }
   else
   {
      if (deformBlur || transformBlur)
      {
         MString data = AiNodeGetStr(proc, "data");
         data += " " + ToString(relativeSamples ? sampleFrame - renderFrame : sampleFrame);
         AiNodeSetStr(proc, "data", data.asChar());
      }
   }
}

float CAbcTranslator::GetSampleFrame(unsigned int step)
{
   MFnDependencyNode opts(m_session->GetArnoldRenderOptions());
   
   int steps = opts.findPlug("motion_steps").asInt();
   
   if (!IsMotionBlurEnabled() || !IsLocalMotionBlurEnabled() || steps <= 1)
   {
      return m_session->GetExportFrame();
   }
   else
   {
      int mbtype = opts.findPlug("range_type").asInt();
      float start = m_session->GetExportFrame();
      float end = start;
      
      if (mbtype == MTOA_MBLUR_TYPE_CUSTOM)
      {
         start += opts.findPlug("motion_start").asFloat();
         end += opts.findPlug("motion_end").asFloat();
      }
      else
      {
         float frames = opts.findPlug("motion_frames").asFloat();
         
         if (mbtype == MTOA_MBLUR_TYPE_START)
         {
            end += frames;
         }
         else if (mbtype == MTOA_MBLUR_TYPE_END)
         {
            start -= frames;
         }
         else
         {
            float half_frames = 0.5f * frames;
            start -= half_frames;
            end += half_frames;
         }
      }
      
      float incr = (end - start) / (steps - 1);
      
      return start + step * incr;
   }
}

void CAbcTranslator::ExportAbc(AtNode *proc, unsigned int step, bool update)
{
   double renderFrame = GetExportFrame();
   double sampleFrame = GetSampleFrame(step);
   
   GetFrames(renderFrame, sampleFrame, renderFrame, sampleFrame);

   MDagPath masterDag;
   bool isInstance = !DoIsMasterInstance(m_dagPath, masterDag);
   if (!isInstance)
   {
      // IsMasterInstance doesn't set masterDag when it returns true
      masterDag = m_dagPath;
   }

   ExportProc(proc, step, renderFrame, sampleFrame);
   ExportBounds(proc, step);
   ExportMatrix(proc, step);

   if (step == 0)
   {
      MPlug plug;
      const AtNodeEntry *entry = AiNodeGetNodeEntry(proc);
      
      plug = FindMayaPlug("aiStepSize");
      if (!plug.isNull() && HasParameter(entry, "step_size", proc, "constant FLOAT"))
      {
         AiNodeSetFlt(proc, "step_size", plug.asFloat());
      }
      
      ExportShader(proc, update);
      ExportVisibility(proc);
      ExportMeshAttribs(proc);
      ExportSubdivAttribs(proc);
      
      // For curves/particles
      
      plug = FindMayaPlug("aiMinPixelWidth");
      if (!plug.isNull() && HasParameter(entry, "min_pixel_width", proc, "constant FLOAT"))
      {
         AiNodeSetFlt(proc, "min_pixel_width", plug.asFloat());
      }
      
      plug = FindMayaPlug("aiMode");
      if (!plug.isNull() && HasParameter(entry, "mode", proc, "constant INT"))
      {
         AiNodeSetInt(proc, "mode", plug.asInt());
      }
      
      // For curves (overrides)
      plug = FindMayaPlug("aiRenderCurve");
      if (!plug.isNull() && HasParameter(entry, "abc_ignoreNurbs", proc, "constant BOOL"))
      {
         MGlobal::displayWarning("[mzAbcShape] Override 'abc_ignoreNurbs' from MtoA specific parameter 'aiRenderCurve'");
         AiNodeSetBool(proc, "abc_ignoreNurbs", !plug.asBool());
      }
      
      plug = FindMayaPlug("aiSampleRate");
      if (!plug.isNull() && HasParameter(entry, "abc_nurbsSampleRate", proc, "constant INT"))
      {
         MGlobal::displayWarning("[mzAbcShape] Override 'abc_nurbsSampleRate' from MtoA specific parameter 'aiSampleRate'");
         AiNodeSetInt(proc, "abc_nurbsSampleRate", plug.asInt());
      }
      
      plug = FindMayaPlug("aiCurveWidth");
      if (!plug.isNull() && HasParameter(entry, "abc_widthMin", proc, "constant FLOAT") &&
                            HasParameter(entry, "abc_widthMax", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[mzAbcShape] Override 'abc_widthMin' and 'abc_widthMax' from MtoA specific parameter 'aiCurveWidth'");
         float width = plug.asFloat();
         AiNodeSetFlt(proc, "abc_widthMin", width);
         AiNodeSetFlt(proc, "abc_widthMax", width);
      }
      
      // For particles (overrides)
      plug = FindMayaPlug("aiRadiusMultiplier");
      if (!plug.isNull() && HasParameter(entry, "abc_radiusScale", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[mzAbcShape] Override 'abc_radiusScale' from MtoA specific parameter 'aiRadiusMultiplier'");
         AiNodeSetFlt(proc, "abc_radiusScale", plug.asFloat());
      }
      
      plug = FindMayaPlug("aiMinParticleRadius");
      if (!plug.isNull() && HasParameter(entry, "abc_radiusMin", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[mzAbcShape] Override 'abc_radiusMin' from MtoA specific parameter 'aiMinParticleRadius'");
         AiNodeSetFlt(proc, "abc_radiusMin", plug.asFloat());
      }
      
      plug = FindMayaPlug("aiMaxParticleRadius");
      if (!plug.isNull() && HasParameter(entry, "abc_radiusMax", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[mzAbcShape] Override 'abc_radiusMax' from MtoA specific parameter 'aiMaxParticleRadius'");
         AiNodeSetFlt(proc, "abc_radiusMax", plug.asFloat());
      }
      
      // Handled in DoExport of base Translator class
      //ExportUserAttribute(proc);
      
      ExportLightLinking(proc);
      
      plug = FindMayaPlug("aiTraceSets");
      if (!plug.isNull())
      {
         ExportTraceSets(proc, plug);
      }
   }

   if (!IsMotionBlurEnabled() || !IsLocalMotionBlurEnabled() || int(step) >= (int(GetNumMotionSteps()) - 1))
   {
      // Last motion sample exported      
      const AtNodeEntry *nodeEntry = AiNodeGetNodeEntry(proc);

      // Add padding to bounding box
      float padding = m_peakPadding;
      
      if (HasParameter(nodeEntry, "disp_padding", proc))
      {
         padding += AiNodeGetFlt(proc, "disp_padding");
      }
      
      MPlug plug = FindMayaPlug("mtoa_constant_abc_boundsPadding");
      if (!plug.isNull())
      {
         padding += plug.asFloat();
      }
      
      if (padding != 0.0f)
      {
         AtPoint cmin = AiNodeGetPnt(proc, "min");
         AtPoint cmax = AiNodeGetPnt(proc, "max");

         cmin.x -= padding;
         cmin.y -= padding;
         cmin.z -= padding;
         cmax.x += padding;
         cmax.y += padding;
         cmax.z += padding;

         AiNodeSetPnt(proc, "min", cmin.x, cmin.y, cmin.z);
         AiNodeSetPnt(proc, "max", cmax.x, cmax.y, cmax.z);
      }
   }
}

void CAbcTranslator::Delete()
{
}


