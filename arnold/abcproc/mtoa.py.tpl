import os
import re
import arnold
from maya import OpenMaya
from maya import cmds
import scriptedTranslatorUtils as stu

Cycles = ["hold", "loop", "reverse", "bounce"]
AttributesEvaluationTime = ["render", "shutter", "shutter_open", "shutter_close"]

def ArnoldType():
   return "abcproc"

def IsShape():
   return True

def SupportInstances():
   return False

def SupportVolumes():
   return False

def Export(renderFrame, step, sampleFrame, nodeNames, masterNodeNames):
   global Cycles
   global AttributesEvaluationTime

   isInstance = (masterNodeNames is not None)

   nodeName, arnoldNodeName = nodeNames
   masterNodeName, arnoldMasterNodeName = masterNodeNames if isInstance else ("", "")

   deformationBlur = stu.GetDeformationBlur(nodeName)
   transformationBlur = stu.GetTransformationBlur(nodeName)

   # EvaluateFrames returns adequatly modified render and sample frames
   renderFrame, sampleFrame = stu.EvaluateFrames(nodeName, "time", renderFrame)

   # Support for volumes (box or ginstance nodes)
   # => just leave everything up to sciptedTranslator extension
   node = arnold.AiNodeLookUpByName(arnoldNodeName)
   if not node:
      return []

   setAttrs = []

   if arnold.AiNodeIs(node, "box"):
      return []

   elif arnold.AiNodeIs(node, "ginstance"):
      return []

   elif not arnold.AiNodeIs(node, "abcproc"):
      return []

   if step == 0:
      fname = cmds.getAttr(nodeName+".filePath")
      if fname is None:
         fname = ""
      arnold.AiNodeSetStr(node, "filename", fname)
      setAttrs.append("filename")

      opath = cmds.getAttr(nodeName+".objectExpression")
      if opath:
         arnold.AiNodeSetStr(node, "objectpath", opath)
         setAttrs.append("objectpath")

      arnold.AiNodeSetFlt(node, "fps", stu.GetFPS())
      setAttrs.append("fps")

      arnold.AiNodeSetFlt(node, "frame", renderFrame)
      setAttrs.append("frame")

      val = stu.GetOverrideAttr(nodeName, "speed", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "speed", val)
         setAttrs.append("speed")

      val = stu.GetOverrideAttr(nodeName, "offset", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "offset", val)
         setAttrs.append("offset")

      val = stu.GetOverrideAttr(nodeName, "preserveStartFrame", None)
      if val is not None:
         arnold.AiNodeSetBool(node, "preserve_start_frame", val)
         setAttrs.append("preserve_start_frame")

      val = stu.GetOverrideAttr(nodeName, "startFrame", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "start_frame", val)
         setAttrs.append("start_frame")

      val = stu.GetOverrideAttr(nodeName, "endFrame", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "end_frame", val)
         setAttrs.append("end_frame")

      val = cmds.getAttr(nodeName+".cycleType")
      if val and val >= 0 and val < len(Cycles):
         arnold.AiNodeSetInt(node, "cycle", val)
         setAttrs.append("cycle")

      arnold.AiNodeSetBool(node, "ignore_deform_blur", (True if not deformationBlur else False))
      setAttrs.append("ignore_deform_blur")

      arnold.AiNodeSetBool(node, "ignore_transform_blur", (True if not transformationBlur else False))
      setAttrs.append("ignore_transform_blur")

      ignoreTransforms = cmds.getAttr(nodeName+".ignoreXforms")
      arnold.AiNodeSetBool(node, "ignore_transforms", (True if ignoreTransforms else False))
      setAttrs.append("ignore_transforms")

      ignoreInstances = cmds.getAttr(nodeName+".ignoreInstances")
      arnold.AiNodeSetBool(node, "ignore_instances", (True if ignoreInstances else False))
      setAttrs.append("ignore_instances")

      ignoreVisibility = cmds.getAttr(nodeName+".ignoreVisibility")
      arnold.AiNodeSetBool(node, "ignore_visibility", (True if ignoreVisibility else False))
      setAttrs.append("ignore_visibility")

      val = stu.GetOverrideAttr(nodeName, "aiVelocityName", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "velocity_name", val)
         setAttrs.append("velocity_name")

      val = stu.GetOverrideAttr(nodeName, "aiAccelerationName", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "acceleration_name", val)
         setAttrs.append("acceleration_name")

      val = stu.GetOverrideAttr(nodeName, "aiVelocityScale", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "velocity_scale", val)
         setAttrs.append("velocity_scale")

      val = stu.GetOverrideAttr(nodeName, "aiForceVelocityBlur", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "force_velocity_blur", val)
         setAttrs.append("force_velocity_blur")

      val = stu.GetOverrideAttr(nodeName, "aiIgnoreReference", None)
      if val is not None:
         arnold.AiNodeSetBool(node, "ignore_reference", (True if val else False))
         setAttrs.append("ignore_reference")

      val = stu.GetOverrideAttr(nodeName, "aiReferenceSource", None)
      if val is not None:
         arnold.AiNodeSetInt(node, "reference_source", val)
         setAttrs.append("reference_source")

      val = stu.GetOverrideAttr(nodeName, "aiReferencePositionName", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "reference_position_name", val)
         setAttrs.append("reference_position_name")

      val = stu.GetOverrideAttr(nodeName, "aiReferenceNormalName", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "reference_normal_name", val)
         setAttrs.append("reference_normal_name")

      val = stu.GetOverrideAttr(nodeName, "aiReferenceFrame", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "reference_frame", val)
         setAttrs.append("reference_frame")

      val = stu.GetOverrideAttr(nodeName, "aiComputeTangentsForUVs", None)
      if val is not None:
         lst = filter(lambda y: len(y) > 0, map(lambda x: x.strip(), val.split(" ")))
         ary = arnold.AiArrayAllocate(len(lst), 1, arnold.AI_TYPE_STRING)
         for i in xrange(len(lst)):
            arnold.AiArraySetStr(ary, i, lst[i])
         arnold.AiNodeSetArray(node, "compute_tangents_for_uvs", ary)
         setAttrs.append("compute_tangents_for_uvs")

      val = stu.GetOverrideAttr(nodeName, "aiRadiusName", None)
      if val is not None:
         arnold.AiNodeSetStr(node, "radius_name", val)
         setAttrs.append("radius_name")

      val = stu.GetOverrideAttr(nodeName, "aiRadiusScale", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "radius_scale", val)
         setAttrs.append("radius_scale")

      val = stu.GetOverrideAttr(nodeName, "aiRadiusMin", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "radius_min", val)
         setAttrs.append("radius_min")

      val = stu.GetOverrideAttr(nodeName, "aiRadiusMax", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "radius_max", val)
         setAttrs.append("radius_max")

      val = stu.GetOverrideAttr(nodeName, "aiIgnoreNurbs", None)
      if val is not None:
         arnold.AiNodeSetBool(node, "ignore_nurbs", val)
         setAttrs.append("ignore_nurbs")

      val = stu.GetOverrideAttr(nodeName, "aiNurbsSampleRate", None)
      if val is not None:
         arnold.AiNodeSetUInt(node, "nurbs_sample_rate", val)
         setAttrs.append("nurbs_sample_rate")

      val = stu.GetOverrideAttr(nodeName, "aiWidthScale", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "width_scale", val)
         setAttrs.append("width_scale")

      val = stu.GetOverrideAttr(nodeName, "aiWidthMin", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "width_min", val)
         setAttrs.append("width_min")

      val = stu.GetOverrideAttr(nodeName, "aiWidthMax", None)
      if val is not None:
         arnold.AiNodeSetFlt(node, "width_max", val)
         setAttrs.append("width_max")

      val = stu.GetOverrideAttr(nodeName, "aiRemoveAttributePrefices", None)
      if val is not None:
         lst = filter(lambda y: len(y) > 0, map(lambda x: x.strip(), val.split(" ")))
         ary = arnold.AiArrayAllocate(len(lst), 1, arnold.AI_TYPE_STRING)
         for i in xrange(len(lst)):
            arnold.AiArraySetStr(ary, i, lst[i])
         arnold.AiNodeSetArray(node, "remove_attribute_prefices", ary)
         setAttrs.append("remove_attribute_prefices")

      val = stu.GetOverrideAttr(nodeName, "aiForceConstantAttributes", None)
      if val is not None:
         lst = filter(lambda y: len(y) > 0, map(lambda x: x.strip(), val.split(" ")))
         ary = arnold.AiArrayAllocate(len(lst), 1, arnold.AI_TYPE_STRING)
         for i in xrange(len(lst)):
            arnold.AiArraySetStr(ary, i, lst[i])
         arnold.AiNodeSetArray(node, "force_constant_attributes", ary)
         setAttrs.append("force_constant_attributes")

      val = stu.GetOverrideAttr(nodeName, "aiIgnoreAttributes", None)
      if val is not None:
         lst = filter(lambda y: len(y) > 0, map(lambda x: x.strip(), val.split(" ")))
         ary = arnold.AiArrayAllocate(len(lst), 1, arnold.AI_TYPE_STRING)
         for i in xrange(len(lst)):
            arnold.AiArraySetStr(ary, i, lst[i])
         arnold.AiNodeSetArray(node, "ignore_attributes", ary)
         setAttrs.append("ignore_attributes")

      val = stu.GetOverrideAttr(nodeName, "aiAttributesEvaluationTime", None)
      if val and val >= 0 and val < len(AttributesEvaluationTime):
         arnold.AiNodeSetInt(node, "attributes_evaluation_time", val)
         setAttrs.append("attributes_evaluation_time")

      arnold.AiNodeSetUInt(node, "samples", 1)
      setAttrs.append("samples")

      val = stu.GetOverrideAttr(nodeName, "aiExpandSamplesIterations", None)
      if val is not None:
         arnold.AiNodeSetUInt(node, "expand_samples_iterations", val)
         setAttrs.append("expand_samples_iterations")

      val = stu.GetOverrideAttr(nodeName, "aiOptimizeSamples", None)
      if val is not None:
         arnold.AiNodeSetBool(node, "optimize_samples", val)
         setAttrs.append("optimize_samples")

   else:
      samples = arnold.AiNodeGetUInt(node, "samples")
      if samples < step + 1:
         arnold.AiNodeSetUInt(node, "samples", step+1)
      setAttrs.append("samples")

   return setAttrs


def Cleanup(nodeNames, masterNodeNames):
   pass


def SetupAttrs():
   attrs = []

   try:
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="velocity_name"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="acceleration_name"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="velocity_scale"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="force_velocity_blur"))

      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="ignore_reference"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="reference_source"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="reference_position_name"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="reference_normal_name"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="reference_frame"))

      attrs.append(stu.AttrData(name="aiRemoveAttributePrefices", shortName="ai_remove_attribute_prefices", type=arnold.AI_TYPE_STRING, defaultValue=""))
      attrs.append(stu.AttrData(name="aiForceConstantAttributes", shortName="ai_force_constant_attributes", type=arnold.AI_TYPE_STRING, defaultValue=""))
      attrs.append(stu.AttrData(name="aiIgnoreAttributes", shortName="ai_ignore_attributes", type=arnold.AI_TYPE_STRING, defaultValue=""))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="attributes_evaluation_time"))

      attrs.append(stu.AttrData(name="aiComputeTangentsForUVs", shortName="ai_compute_tangents_for_uvs", type=arnold.AI_TYPE_STRING, defaultValue=""))

      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="width_scale"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="width_min"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="width_max"))

      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="radius_name"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="radius_scale"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="radius_min"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="radius_max"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="ignore_nurbs"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="nurbs_sample_rate"))

      attrs.append(stu.AttrData(arnoldNode="box", arnoldAttr="step_size"))
      attrs.append(stu.AttrData(arnoldNode="box", arnoldAttr="volume_padding"))

      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="expand_samples_iterations"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="optimize_samples"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="nameprefix"))
      attrs.append(stu.AttrData(arnoldNode="abcproc", arnoldAttr="verbose"))

   except Exception, e:
      print("mtoa_<<NodeName>>.SetupAttrs: Failed\n%s" % e)
      attrs = []

   return map(str, attrs)


def SetupAE(translator):
   import pymel.core as pm
   if not pm.pluginInfo("<<NodeName>>", query=1, loaded=1):
      return

   try:
      import mtoa.ui.ae.templates as templates

      class <<NodeName>>Template(templates.ShapeTranslatorTemplate):
         def setup(self):
            self.commonShapeAttributes()
            self.addSeparator()

            self.renderStatsAttributes()
            self.addSeparator()

            self.addControl("aiSssSetname", label="SSS Set Name")
            self.addControl("aiStepSize", label="Volume Step Size")
            self.addControl("aiVolumePadding", label="Volume Padding")
            self.addControl("aiUserOptions", label="User Options")
            self.addSeparator()

            self.addControl("aiVelocityName", label="Velocity Name")
            self.addControl("aiAccelerationName", label="Acceleration Name")
            self.addControl("aiVelocityScale", label="Velocity Scale")
            self.addControl("aiForceVelocityBlur", label="Force Velocity Blur")
            self.addSeparator()

            self.addControl("aiIgnoreReference", label="Ignore Reference")
            self.addControl("aiReferenceSource", label="Reference Source")
            self.addControl("aiReferencePositionName", label="Reference Position Name")
            self.addControl("aiReferenceNormalName", label="Reference Normal Name")
            self.addControl("aiReferenceFrame", label="Reference Frame")
            self.addSeparator()

            self.addControl("aiRemoveAttributePrefices", label="Remove Attribute Prefices")
            self.addControl("aiForceConstantAttributes", label="Force Constant Attributes")
            self.addControl("aiIgnoreAttributes", label="Ignore Attributes")
            self.addControl("aiAttributesEvaluationTime", label="Attributes Evaluation TIme")
            self.addSeparator()

            self.addControl("aiComputeTangentsForUVs", label="Compute Tangents For UVs")
            self.addSeparator()

            self.addControl("aiRadiusName", label="Radius Attribute Name")
            self.addControl("aiRadiusScale", label="Radius Scale")
            self.addControl("aiRadiusMin", label="Min. Radius")
            self.addControl("aiRadiusMax", label="Max. Radius")
            self.addControl("aiIgnoreNurbs", label="Ignore NURBS")
            self.addControl("aiNurbsSampleRate", label="NURBS Sample Rate")
            self.addSeparator()

            self.addControl("aiWidthScale", label="Width Scale")
            self.addControl("aiWidthMin", label="Min. Width")
            self.addControl("aiWidthMax", label="Max. Width")
            self.addSeparator()

            self.addControl("aiExpandSamplesIterations", label="Expand Samples Iterations")
            self.addControl("aiOptimizeSamples", label="Optimize Samples")
            self.addSeparator()

            self.addControl("aiNameprefix", label="Name Prefix")
            self.addControl("aiVerbose", label="Verbose")
            self.addSeparator()

      templates.registerTranslatorUI(<<NodeName>>Template, "<<NodeName>>", translator)

   except Exception, e:
      print("mtoa_<<NodeName>>.SetupAE: Failed\n%s" % e)
