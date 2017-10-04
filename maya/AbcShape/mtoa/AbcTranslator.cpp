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
#include <maya/MTypes.h>

#if MAYA_API_VERSION >= 2018000
#  include <maya/MDGContextGuard.h>
#endif

void* CAbcTranslator::Create()
{
   return new CAbcTranslator();
}

void CAbcTranslator::NodeInitializer(CAbTranslator context)
{
   CExtensionAttrHelper helper(context.maya, "abcproc");

   CShapeTranslator::MakeCommonAttributes(helper);

   CAttrData data;

   // velocity

   data.stringDefault = "";
   data.name = "aiVelocityName";
   data.shortName = "ai_velocity_name";
   helper.MakeInputString(data);

   data.stringDefault = "";
   data.name = "aiAccelerationName";
   data.shortName = "ai_acceleration_name";
   helper.MakeInputString(data);

   data.defaultValue.FLT() = 1.0f;
   data.name = "aiVelocityScale";
   data.shortName = "ai_velocity_scale";
   data.hasMin = false;
   data.hasSoftMin = true;
   data.softMin.FLT() = 0.0f;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.FLT() = 10.0f;
   helper.MakeInputFloat(data);

   data.defaultValue.BOOL() = false;
   data.name = "aiForceVelocityBlur";
   data.shortName = "ai_force_velocity_blur";
   helper.MakeInputBoolean(data);

   // reference

   data.defaultValue.BOOL() = false;
   data.name = "aiOutputReference";
   data.shortName = "ai_output_reference";
   helper.MakeInputBoolean(data);

   data.defaultValue.INT() = 0;
   data.enums.clear();
   data.enums.append("attributes_then_file");
   data.enums.append("attributes");
   data.enums.append("file");
   data.enums.append("frame");
   data.name = "aiReferenceSource";
   data.shortName = "ai_reference_source";
   helper.MakeInputEnum(data);

   data.stringDefault = "";
   data.name = "aiReferencePositionName";
   data.shortName = "ai_reference_position_name";
   helper.MakeInputString(data);

   data.stringDefault = "";
   data.name = "aiReferenceNormalName";
   data.shortName = "ai_reference_normal_name";
   helper.MakeInputString(data);

   data.stringDefault = "";
   data.name = "aiReferenceFilename";
   data.shortName = "ai_reference_filename";
   helper.MakeInputString(data);

   data.defaultValue.FLT() = 0.0f;
   data.name = "aiReferenceFrame";
   data.shortName = "ai_reference_frame";
   data.hasMin = false;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   // attributes

   data.stringDefault = "";
   data.name = "aiRemoveAttributePrefices";
   data.shortName = "ai_remove_attribute_prefices";
   helper.MakeInputString(data);

   data.stringDefault = "";
   data.name = "aiForceConstantAttributes";
   data.shortName = "ai_force_constant_attributes";
   helper.MakeInputString(data);

   data.stringDefault = "";
   data.name = "aiIgnoreAttributes";
   data.shortName = "ai_ignore_attributes";
   helper.MakeInputString(data);

   data.defaultValue.INT() = 0;
   data.enums.clear();
   data.enums.append("render");
   data.enums.append("shutter");
   data.enums.append("shutter_open");
   data.enums.append("shutter_close");
   data.name = "aiAttributesEvaluationTime";
   data.shortName = "ai_attributes_evaluation_time";
   helper.MakeInputEnum(data);

   // mesh

   data.stringDefault = "";
   data.name = "aiComputeTangentsForUVs";
   data.shortName = "ai_compute_tangents_for_uvs";
   helper.MakeInputString(data);

   // particles

   data.stringDefault = "";
   data.name = "aiRadiusName";
   data.shortName = "ai_radius_name";
   helper.MakeInputString(data);

   data.defaultValue.FLT() = 1.0;
   data.name = "aiRadiusScale";
   data.shortName = "ai_radius_scale";
   data.hasMin = true;
   data.min.FLT() = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   data.defaultValue.FLT() = 0.0;
   data.name = "aiRadiusMin";
   data.shortName = "ai_radius_min";
   data.hasMin = true;
   data.min.FLT() = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   data.defaultValue.FLT() = 1000000.0;
   data.name = "aiRadiusMax";
   data.shortName = "ai_radius_max";
   data.hasMin = true;
   data.min.FLT() = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   // curves

   data.defaultValue.BOOL() = true;
   data.name = "aiIgnoreNurbs";
   data.shortName = "ai_ignore_nurbs";
   helper.MakeInputBoolean(data);

   data.defaultValue.INT() = 5;
   data.name = "aiNurbsSampleRate";
   data.shortName = "ai_nurbs_sample_rate";
   data.hasMin = true;
   data.min.INT() = 1;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.INT() = 10;
   helper.MakeInputInt(data);

   data.defaultValue.FLT() = 1.0;
   data.name = "aiWidthScale";
   data.shortName = "ai_width_scale";
   data.hasMin = true;
   data.min.FLT() = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   data.defaultValue.FLT() = 0.0;
   data.name = "aiWidthMin";
   data.shortName = "ai_width_min";
   data.hasMin = true;
   data.min.FLT() = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   data.defaultValue.FLT() = 1000000.0;
   data.name = "aiWidthMax";
   data.shortName = "ai_width_max";
   data.hasMin = true;
   data.min.FLT() = 0.0;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = false;
   helper.MakeInputFloat(data);

   // samples expansion

   data.defaultValue.INT() = 0;
   data.name = "aiExpandSamplesIterations";
   data.shortName = "ai_expand_samples_iterations";
   data.hasMin = true;
   data.min.INT() = 0;
   data.hasSoftMin = false;
   data.hasMax = true;
   data.max.INT() = 10;
   data.hasSoftMax = false;
   helper.MakeInputInt(data);

   data.defaultValue.BOOL() = false;
   data.name = "aiOptimizeSamples";
   data.shortName = "ai_optimize_samples";
   helper.MakeInputBoolean(data);

   // volume
   data.defaultValue.FLT() = 0.0f;
   data.name = "aiStepSize";
   data.shortName = "ai_step_size";
   data.hasMin = true;
   data.min.FLT() = 0.f;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.FLT() = 1.f;
   helper.MakeInputFloat(data);

   data.defaultValue.FLT() = 0.0f;
   data.name = "aiVolumePadding";
   data.shortName = "ai_volume_padding";
   data.hasMin = true;
   data.min.FLT() = 0.f;
   data.hasSoftMin = false;
   data.hasMax = false;
   data.hasSoftMax = true;
   data.softMax.FLT() = 1.f;
   helper.MakeInputFloat(data);

   // others

   data.stringDefault = "";
   data.name = "aiNameprefix";
   data.shortName = "ai_nameprefix";
   helper.MakeInputString(data);

   data.defaultValue.BOOL() = false;
   data.name = "aiVerbose";
   data.shortName = "ai_verbose";
   helper.MakeInputBoolean(data);
}

CAbcTranslator::CAbcTranslator()
   : CShapeTranslator()
   , m_motionBlur(false)
{
}

CAbcTranslator::~CAbcTranslator()
{
}

void CAbcTranslator::Init()
{
   CShapeTranslator::Init();
   m_motionBlur = (IsMotionBlurEnabled(MTOA_MBLUR_DEFORM|MTOA_MBLUR_OBJECT) && IsLocalMotionBlurEnabled());
}

void CAbcTranslator::Export(AtNode *atNode)
{
   ExportAbc(atNode, GetMotionStep(), IsExported());
}

void CAbcTranslator::ExportMotion(AtNode *atNode)
{
   ExportAbc(atNode, GetMotionStep(), IsExported());
}

void CAbcTranslator::RequestUpdate()
{
   SetUpdateMode(AI_RECREATE_NODE);
   CShapeTranslator::RequestUpdate();
}

AtNode* CAbcTranslator::CreateArnoldNodes()
{
   return AddArnoldNode("abcproc");
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

MPlug CAbcTranslator::FindMayaObjectPlug(const MString &attrName, MStatus* ReturnStatus) const
{
   MObject obj = GetMayaObject();
   MFnDependencyNode node(obj);
   return node.findPlug(attrName, ReturnStatus);
}

void CAbcTranslator::GetFrames(double inRenderFrame, double inSampleFrame,
                               double &outRenderFrame, double &outSampleFrame)
{
   MPlug pTime = FindMayaObjectPlug("time");

   MDGContext ctx0(MTime(inRenderFrame, MTime::uiUnit()));
   MDGContext ctx1(MTime(inSampleFrame, MTime::uiUnit()));

   MStatus st;
   MTime t;

   #if MAYA_API_VERSION >= 2018000
      {
         MDGContextGuard guard(ctx0);
         t = pTime.asMTime(&st);
         outRenderFrame = (st == MS::kSuccess ? t.asUnits(MTime::uiUnit()) : inRenderFrame);
      }
      {
         MDGContextGuard guard(ctx1);
         t = pTime.asMTime(&st);
         outSampleFrame = (st == MS::kSuccess ? t.asUnits(MTime::uiUnit()) : inSampleFrame);
      }
   #else
      t = pTime.asMTime(ctx0, &st);
      outRenderFrame = (st == MS::kSuccess ? t.asUnits(MTime::uiUnit()) : inRenderFrame);

      t = pTime.asMTime(ctx1, &st);
      outSampleFrame = (st == MS::kSuccess ? t.asUnits(MTime::uiUnit()) : inSampleFrame);
   #endif
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

   int visibility = AI_RAY_UNDEFINED;

   if (IsRenderable())
   {
      visibility = AI_RAY_ALL;

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

      plug = FindMayaPlug("aiVisibleInDiffuseReflection");
      if (!plug.isNull() && !plug.asBool())
      {
         visibility &= ~(AI_RAY_DIFFUSE_REFLECT);
      }

      plug = FindMayaPlug("aiVisibleInSpecularReflection");
      if (!plug.isNull() && !plug.asBool())
      {
         visibility &= ~(AI_RAY_SPECULAR_REFLECT);
      }

      plug = FindMayaPlug("aiVisibleInDiffuseTransmission");
      if (!plug.isNull() && !plug.asBool())
      {
         visibility &= ~(AI_RAY_DIFFUSE_TRANSMIT);
      }

      plug = FindMayaPlug("aiVisibleInSpecularTransmission");
      if (!plug.isNull() && !plug.asBool())
      {
         visibility &= ~(AI_RAY_SPECULAR_TRANSMIT);
      }
   }

   AiNodeSetByte(proc, "visibility", visibility);

   plug = FindMayaPlug("aiSelfShadows");
   if (!plug.isNull())
   {
      AiNodeSetBool(proc, "self_shadows", plug.asBool());
   }

   plug = FindMayaPlug("aiOpaque");
   if (!plug.isNull())
   {
      AiNodeSetBool(proc, "opaque", plug.asBool());
   }

   plug = FindMayaPlug("receiveShadows");
   if (!plug.isNull())
   {
      AiNodeSetBool(proc, "receive_shadows", plug.asBool());
   }

   const AtNodeEntry *nodeEntry = AiNodeGetNodeEntry(proc);

   plug = FindMayaPlug("aiSssSetname");
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

   plug = FindMayaPlug("aiTraceSets");
   if (!plug.isNull())
   {
      ExportTraceSets(proc, plug);
   }

   AiNodeSetUInt(proc, "id",  getHash(proc));
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
      AtNode *shader = ExportConnectedNode(shadingGroupPlug);

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
   masterDag = (IsMasterInstance() ? m_dagPath : GetMasterInstance());

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
            dispPadding = AiMax(dispPadding, plug.asFloat());
         }

         plug = dispNode.findPlug("aiDisplacementAutoBump");
         if (!plug.isNull())
         {
            outputDispAutobump = true;
            dispAutobump = dispAutobump || plug.asBool();
         }

         if (HasParameter(nodeEntry, "disp_map", proc, "constant ARRAY NODE"))
         {
            AtNode *dispImage = ExportConnectedNode(shaderConns[0]);
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

static AtArray* ToArray(const std::string &s)
{
   size_t p0, p1;
   std::string name;
   std::vector<std::string> names;

   p0 = 0;
   p1 = s.find_first_of(" \t\n");
   while (p0 != std::string::npos)
   {
      //  0   1   2   3   4   5   6   7
      // ' ' 'a' 'b' ' ' ' ' 'd' 'd' 'd'
      if (p1 == std::string::npos)
      {
         name = s.substr(p0);
         p0 = p1;
      }
      else if (p1 > p0)
      {
         name = s.substr(p0, p1 - p0);
         p0 = p1 + 1;
      }
      else
      {
         name = "";
         p0 = p1 + 1;
      }
      if (name.length() > 0)
      {
         names.push_back(name);
      }
      p1 = s.find_first_of(" \t\n", p0);
   }

   AtArray *out = AiArrayAllocate(names.size(), 1, AI_TYPE_STRING);
   for (size_t i=0; i<names.size(); ++i)
   {
      AiArraySetStr(out, i, names[i].c_str());
   }
   return out;
}

void CAbcTranslator::ExportProc(AtNode *proc, unsigned int step, double renderFrame, double sampleFrame)
{
   bool transformBlur = IsMotionBlurEnabled(MTOA_MBLUR_OBJECT) && IsLocalMotionBlurEnabled();
   bool deformBlur = IsMotionBlurEnabled(MTOA_MBLUR_DEFORM) && IsLocalMotionBlurEnabled();

   bool ignoreXforms = FindMayaObjectPlug("ignoreXforms").asBool();
   bool ignoreInstances = FindMayaObjectPlug("ignoreInstances").asBool();
   bool ignoreVisibility = FindMayaObjectPlug("ignoreVisibility").asBool();

   MPlug plug;

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

   if (!IsExportingMotion())
   {
      std::string tmp;

      AiNodeSetStr(proc, "filename", abcfile.asChar());
      AiNodeSetStr(proc, "objectpath", objpath.asChar());
      AiNodeSetFlt(proc, "fps", GetFPS());
      AiNodeSetFlt(proc, "frame", renderFrame);

      if (!isGpuCache)
      {
         AiNodeSetFlt(proc, "speed", FindMayaObjectPlug("speed").asFloat());
         AiNodeSetFlt(proc, "offset", FindMayaObjectPlug("offset").asFloat());
         AiNodeSetBool(proc, "preserve_start_frame", FindMayaObjectPlug("preserveStartFrame").asBool());
         AiNodeSetFlt(proc, "start_frame", FindMayaObjectPlug("startFrame").asFloat());
         AiNodeSetFlt(proc, "end_frame", FindMayaObjectPlug("endFrame").asFloat());
         AiNodeSetInt(proc, "cycle", FindMayaObjectPlug("cycleType").asInt());
         AiNodeSetBool(proc, "ignore_deform_blur", !deformBlur);
         AiNodeSetBool(proc, "ignore_transform_blur", !transformBlur);
         AiNodeSetBool(proc, "ignore_transforms", ignoreXforms);
         AiNodeSetBool(proc, "ignore_instances", ignoreInstances);
         AiNodeSetBool(proc, "ignore_visibility", ignoreVisibility);
      }

      plug = FindMayaPlug("aiOutputReference");
      if (!plug.isNull())
      {
         AiNodeSetBool(proc, "output_reference", plug.asBool());
      }

      plug = FindMayaPlug("aiReferenceSource");
      if (!plug.isNull())
      {
         AiNodeSetInt(proc, "reference_source", plug.asInt());
      }

      plug = FindMayaPlug("aiReferenceFilename");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "reference_filename", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiReferencePositionName");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "reference_position_name", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiReferenceNormalName");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "reference_normal_name", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiReferenceFrame");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "reference_frame", plug.asFloat());
      }

      plug = FindMayaPlug("aiComputeTangentsForUVs");
      if (!plug.isNull())
      {
         tmp = plug.asString().asChar();
         AiNodeSetArray(proc, "compute_tangents_for_uvs", ToArray(tmp));
      }

      plug = FindMayaPlug("aiRadiusName");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "radius_name", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiRadiusScale");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "radius_scale", plug.asFloat());
      }

      plug = FindMayaPlug("aiRadiusMin");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "radius_min", plug.asFloat());
      }

      plug = FindMayaPlug("aiRadiusMax");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "radius_max", plug.asFloat());
      }

      plug = FindMayaPlug("aiIgnoreNurbs");
      if (!plug.isNull())
      {
         AiNodeSetBool(proc, "ignore_nurbs", plug.asBool());
      }

      plug = FindMayaPlug("aiNurbsSampleRate");
      if (!plug.isNull())
      {
         AiNodeSetInt(proc, "nurbs_sample_rate", plug.asInt());
      }

      plug = FindMayaPlug("aiWidthScale");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "width_scale", plug.asFloat());
      }

      plug = FindMayaPlug("aiWidthMin");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "width_min", plug.asFloat());
      }

      plug = FindMayaPlug("aiWidthMax");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "width_max", plug.asFloat());
      }

      plug = FindMayaPlug("aiRemoveAttributePrefices");
      if (!plug.isNull())
      {
         tmp = plug.asString().asChar();
         AiNodeSetArray(proc, "remove_attribute_prefices", ToArray(tmp));
      }

      plug = FindMayaPlug("aiForceConstantAttributes");
      if (!plug.isNull())
      {
         tmp = plug.asString().asChar();
         AiNodeSetArray(proc, "force_constant_attributes", ToArray(tmp));
      }

      plug = FindMayaPlug("aiIgnoreAttributes");
      if (!plug.isNull())
      {
         tmp = plug.asString().asChar();
         AiNodeSetArray(proc, "ignore_attributes", ToArray(tmp));
      }

      plug = FindMayaPlug("aiAttributesEvaluationTime");
      if (!plug.isNull())
      {
         AiNodeSetInt(proc, "attributes_evaluation_time", plug.asInt());
      }

      plug = FindMayaPlug("aiExpandSamplesIterations");
      if (!plug.isNull())
      {
         AiNodeSetUInt(proc, "expand_samples_iterations", plug.asInt());
      }

      plug = FindMayaPlug("aiOptimizeSamples");
      if (!plug.isNull())
      {
         AiNodeSetBool(proc, "optimize_samples", plug.asBool());
      }

      plug = FindMayaPlug("aiVelocityName");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "velocity_name", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiAccelerationName");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "acceleration_name", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiVelocityScale");
      if (!plug.isNull())
      {
         AiNodeSetFlt(proc, "velocity_scale", plug.asFloat());
      }

      plug = FindMayaPlug("aiForceVelocityBlur");
      if (!plug.isNull())
      {
         AiNodeSetBool(proc, "force_velocity_blur", plug.asBool());
      }

      plug = FindMayaPlug("aiNameprefix");
      if (!plug.isNull())
      {
         AiNodeSetStr(proc, "nameprefix", plug.asString().asChar());
      }

      plug = FindMayaPlug("aiVerbose");
      if (!plug.isNull())
      {
         AiNodeSetBool(proc, "verbose", plug.asBool());
      }
   }
}

double CAbcTranslator::GetSampleFrame(unsigned int step)
{
   unsigned int count = 0;
   const double *frames = GetMotionFrames(count);
   if (step >= count)
   {
      return GetExportFrame();
   }
   else
   {
      return frames[step];
   }
}

void CAbcTranslator::ExportAbc(AtNode *proc, unsigned int step, bool update)
{
   double renderFrame = GetExportFrame();
   double sampleFrame = GetSampleFrame(step);

   GetFrames(renderFrame, sampleFrame, renderFrame, sampleFrame);

   MDagPath masterDag;
   masterDag = (IsMasterInstance() ? m_dagPath : GetMasterInstance());

   ExportProc(proc, step, renderFrame, sampleFrame);
   ExportMatrix(proc);

   if (!IsExportingMotion())
   {
      MPlug plug;
      const AtNodeEntry *entry = AiNodeGetNodeEntry(proc);

      m_exportedSteps.clear();

      AiNodeSetUInt(proc, "samples", GetNumMotionSteps());

      if (RequiresShaderExport())
      {
         ExportShader(proc, update);
      }
      // ExportVisibility(proc);
      ProcessRenderFlags(proc);

      // For meshes

      ExportMeshAttribs(proc);
      ExportSubdivAttribs(proc);

      // For volumes

      plug = FindMayaPlug("aiStepSize");
      if (!plug.isNull() && HasParameter(entry, "step_size", proc, "constant FLOAT"))
      {
         AiNodeSetFlt(proc, "step_size", plug.asFloat());
      }

      plug = FindMayaPlug("aiVolumePadding");
      if (!plug.isNull() && HasParameter(entry, "volume_padding", proc, "constant FLOAT"))
      {
         AiNodeSetFlt(proc, "volume_padding", plug.asFloat());
      }

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
      if (!plug.isNull() && HasParameter(entry, "ignore_nurbs", proc, "constant BOOL"))
      {
         MGlobal::displayWarning("[AbcShapeMtoa] Override 'ignore_nurbs' from MtoA specific parameter 'aiRenderCurve'");
         AiNodeSetBool(proc, "ignore_nurbs", !plug.asBool());
      }

      plug = FindMayaPlug("aiSampleRate");
      if (!plug.isNull() && HasParameter(entry, "nurbs_sample_rate", proc, "constant INT"))
      {
         MGlobal::displayWarning("[AbcShapeMtoa] Override 'nurbs_sample_rate' from MtoA specific parameter 'aiSampleRate'");
         AiNodeSetInt(proc, "nurbs_sample_rate", plug.asInt());
      }

      plug = FindMayaPlug("aiCurveWidth");
      if (!plug.isNull() && HasParameter(entry, "width_min", proc, "constant FLOAT") &&
                            HasParameter(entry, "width_max", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[AbcShapeMtoa] Override 'width_min' and 'width_max' from MtoA specific parameter 'aiCurveWidth'");
         float width = plug.asFloat();
         AiNodeSetFlt(proc, "width_min", width);
         AiNodeSetFlt(proc, "width_max", width);
      }

      // For particles (overrides)

      plug = FindMayaPlug("aiRadiusMultiplier");
      if (!plug.isNull() && HasParameter(entry, "radius_scale", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[AbcShapeMtoa] Override 'radius_scale' from MtoA specific parameter 'aiRadiusMultiplier'");
         AiNodeSetFlt(proc, "radius_scale", plug.asFloat());
      }

      plug = FindMayaPlug("aiMinParticleRadius");
      if (!plug.isNull() && HasParameter(entry, "radius_min", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[AbcShapeMtoa] Override 'radius_min' from MtoA specific parameter 'aiMinParticleRadius'");
         AiNodeSetFlt(proc, "radius_min", plug.asFloat());
      }

      plug = FindMayaPlug("aiMaxParticleRadius");
      if (!plug.isNull() && HasParameter(entry, "radius_max", proc, "constant FLOAT"))
      {
         MGlobal::displayWarning("[AbcShapeMtoa] Override 'radius_max' from MtoA specific parameter 'aiMaxParticleRadius'");
         AiNodeSetFlt(proc, "radius_max", plug.asFloat());
      }

      // Handled in DoExport of base Translator class
      //ExportUserAttribute(proc);

      ExportLightLinking(proc);
   }

   // Call cleanup command on last export step
   if (m_exportedSteps.find(step) != m_exportedSteps.end())
   {
      char numstr[16];
      sprintf(numstr, "%u", step);
      MGlobal::displayWarning(MString("[AbcShapeMtoa] Motion step already processed: ") + numstr);
   }
   m_exportedSteps.insert(step);

   if (!m_motionBlur || m_exportedSteps.size() == GetNumMotionSteps())
   {
      // NOOP
   }
}
