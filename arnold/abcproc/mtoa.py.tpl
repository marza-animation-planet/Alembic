import os
import re
import arnold
from maya import OpenMaya
from maya import cmds
import scriptedTranslatorUtils as stu

Cycles = ["hold", "loop", "reverse", "bounce"]
AttribFrames = ["render", "shutter", "shutter_open", "shutter_close"]
Paddings = {}

def IsShape():
   return True

def SupportInstances():
   return False

def SupportVolumes():
   return False

def Export(renderFrame, step, sampleFrame, nodeNames, masterNodeNames):
   
   global Cycles
   global AttribFrames
   global Paddings
   
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
   
   if arnold.AiNodeIs(node, "box"):
      return []

   elif arnold.AiNodeIs(node, "ginstance"):
      return []
   
   elif not arnold.AiNodeIs(node, "procedural"):
      return []
   
   if step == 0:
      arnold.AiNodeSetStr(node, "dso", "abcproc")
      
      fname = cmds.getAttr(nodeName+".filePath")
      if fname is None:
         fname = ""
      data = "-filename %s" % fname
      
      opath = cmds.getAttr(nodeName+".objectExpression")
      if opath:
         data += " -objectpath %s" % opath
      
      data += " -fps %f -frame %f" % (stu.GetFPS(), renderFrame)
      
      val = stu.GetOverrideAttr(nodeName, "speed", None)
      if val is not None:
         data += " -speed %f" % val
      
      val = stu.GetOverrideAttr(nodeName, "offset", None)
      if val is not None:
         data += " -offset %f" % val
      
      val = stu.GetOverrideAttr(nodeName, "preserveStartFrame", None)
      if val:
         data += " -preservestartframe"
      
      val = stu.GetOverrideAttr(nodeName, "startFrame", None)
      if val is not None:
         data += " -startframe %f" % val
      
      val = stu.GetOverrideAttr(nodeName, "endFrame", None)
      if val is not None:
         data += " -endframe %f" % val
      
      val = cmds.getAttr(nodeName+".cycleType")
      if val and val >= 0 and val < len(Cycles):
         data += " -cycle %s" % Cycles[val]
      
      if not deformationBlur:
         data += " -ignoredeformblur"
      
      if not transformationBlur:
         data += " -ignoretransformblur"
      
      ignoreTransforms = cmds.getAttr(nodeName+".ignoreXforms")
      if ignoreTransforms:
         data += " -ignoretransforms"
      
      ignoreInstances = cmds.getAttr(nodeName+".ignoreInstances")
      if ignoreInstances:
         data += " -ignoreinstances"
      
      ignoreVisibility = cmds.getAttr(nodeName+".ignoreVisibility")
      if ignoreVisibility:
         data += " -ignorevisibility"
      
      val = cmds.getAttr(nodeName+".mtoa_abc_outputReference")
      if val:
         data += " -outputReference"
      
      val = cmds.getAttr(nodeName+".mtoa_abc_referenceFilename")
      if val:
         data += " -referencefilename %s" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_computeTangents", None)
      if val:
         data += " -computetangents %s" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_radiusScale", None)
      if val is not None:
         data += " -radiusscale %f" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_radiusMin", None)
      if val is not None:
         data += " -radiusmin %f" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_radiusMax", None)
      if val is not None:
         data += " -radiusmax %f" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_nurbsSampleRate", None)
      if val is not None:
         data += " -nurbssamplerate %d" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_overrideAttribs", None)
      if val:
         data += " -overrideattribs %s" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_removeAttribPrefices", None)
      if val:
         data += " -removeattribprefices %s" % val
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_objectAttribs", None)
      if val:
         data += " -objectattribs"
   
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_primitiveAttribs", None)
      if val:
         data += " -primitiveattribs"
   
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_pointAttribs", None)
      if val:
         data += " -pointattribs"
   
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_vertexAttribs", None)
      if val:
         data += " -vertexattribs"
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_attribsFrame", None)
      if val and val >= 0 and val < len(AttribFrames):
         data += " -attribsframe %s" % AttribFrames[val]
      
      val = stu.GetOverrideAttr(nodeName, "mtoa_abc_samplesExpandIterations", None)
      if val is not None:
         data += " -samplesexpanditerations %d" % val
         val = stu.GetOverrideAttr(nodeName, "mtoa_abc_optimizeSamples", None)
         if val:
            data += " -optimizesamples"
      
      if deformationBlur or transformationBlur:
         relativeSamples = stu.GetOverrideAttr(nodeName, "mtoa_abc_relativeSamples", None)
         if relativeSamples is None:
            relativeSamples = False
         
         if relativeSamples:
            data += " -relativesamples"
         
         data += " -samples %f" % (sampleFrame if not relativeSamples else (sampleFrame - renderFrame))
      
      arnold.AiNodeSetStr(node, "data", data)
      
      # This is how abcproc DSO decides wether it should expand into more procedural or the final shape
      singleShape = (cmds.getAttr(nodeName+".numShapes") == 1 and ignoreTransforms)
      
      if singleShape:
         pad = 0.0
         
         if cmds.attributeQuery("aiDispPadding", node=nodeName, exists=1):
            pad = cmds.getAttr(nodeName+".aiDispPadding")
            Paddings[nodeName] = pad
         
         elif cmds.attributeQuery("mtoa_disp_padding", node=nodeName, exists=1):
            pad = cmds.getAttr(nodeName+".mtoa_disp_padding")
            Paddings[nodeName] = pad
         
         # Note: For instanced <<NodeName>> with time related attribute overrides, 'outBoxMin', 'outBoxMax'
         #       attributes won't return the right values, maybe provide some additional attributes
         #       on the <<NodeName>> node to compute an alternate bounding box (using a different set of
         #       speed/offset/startFrame/endFrame/time/preserveStartFrame attributes) ?
         
         bmin = cmds.getAttr(nodeName+".outBoxMin")[0]
         bmax = cmds.getAttr(nodeName+".outBoxMax")[0]
         
         arnold.AiNodeSetPnt(node, "min", bmin[0]-pad, bmin[1]-pad, bmin[2]-pad)
         arnold.AiNodeSetPnt(node, "max", bmax[0]+pad, bmax[1]+pad, bmax[2]+pad)
         
         return ["dso", "data", "min", "max"]
      
      else:
         
         arnold.AiNodeSetBool(node, "load_at_init", True)
         
         return ["dso", "data", "load_at_init"]
      
   else:
      
      if not arnold.AiNodeGetBool(node, "load_at_init"):
         
         pad = Paddings.get(nodeName, 0.0)
         
         bmin = map(lambda x: x-pad, cmds.getAttr(nodeName+".outBoxMin")[0])
         bmax = map(lambda x: x+pad, cmds.getAttr(nodeName+".outBoxMax")[0])
         
         curmin = arnold.AiNodeGetPnt(node, "min")
         curmax = arnold.AiNodeGetPnt(node, "max")
         
         if bmin[0] < curmin.x:
            curmin.x = bmin[0]
         if bmin[1] < curmin.y:
            curmin.y = bmin[1]
         if bmin[2] < curmin.z:
            curmin.z = bmin[2]
         
         if bmax[0] > curmax.x:
            curmax.x = bmax[0]
         if bmax[1] > curmax.y:
            curmax.y = bmax[1]
         if bmax[2] > curmax.z:
            curmax.z = bmax[2]
         
         arnold.AiNodeSetPnt(node, "min", curmin.x, curmin.y, curmin.z)
         arnold.AiNodeSetPnt(node, "max", curmax.x, curmax.y, curmax.z)
      
      if deformationBlur or transformationBlur:
         relativeSamples = stu.GetOverrideAttr(nodeName, "mtoa_abc_relativeSamples", None)
         if relativeSamples is None:
            relativeSamples = False
         
         # Add new frame sample in data
         data = arnold.AiNodeGetStr(node, "data")
         
         data += " %f" % (sampleFrame if not relativeSamples else (sampleFrame - renderFrame))
         
         arnold.AiNodeSetStr(node, "data", data)
      
      return []


def Cleanup(nodeNames, masterNodeNames):
   global Paddings
   
   nodeName, _ = nodeNames
   if nodeName in Paddings:
      del(Paddings[nodeName])


def SetupAttrs():
   attrs = []
   
   try:
      attrs.append(stu.AttrData(arnoldNode="box", arnoldAttr="step_size"))
      
      attrs.append(stu.AttrData(name="mtoa_abc_outputReference", shortName="outref", type=arnold.AI_TYPE_BOOLEAN, defaultValue=False))
      attrs.append(stu.AttrData(name="mtoa_abc_referenceFilename", shortName="reffp", type=arnold.AI_TYPE_STRING, defaultValue=""))
      
      attrs.append(stu.AttrData(name="mtoa_abc_computeTangents", shortName="cmptan", type=arnold.AI_TYPE_STRING, defaultValue=""))
      
      attrs.append(stu.AttrData(name="mtoa_abc_radiusScale", shortName="radscl", type=arnold.AI_TYPE_FLOAT, defaultValue=1.0, min=0))
      attrs.append(stu.AttrData(name="mtoa_abc_radiusMin", shortName="radmin", type=arnold.AI_TYPE_FLOAT, defaultValue=0.0, min=0))
      attrs.append(stu.AttrData(name="mtoa_abc_radiusMax", shortName="radmax", type=arnold.AI_TYPE_FLOAT, defaultValue=1000000.0, min=0))
      attrs.append(stu.AttrData(name="mtoa_abc_nurbsSampleRate", shortName="nurbssr", type=arnold.AI_TYPE_INT, defaultValue=2, min=1))
      
      attrs.append(stu.AttrData(name="mtoa_abc_overrideAttribs", shortname="ovatrs", type=arnold.AI_TYPE_STRING, defaultValue=""))
      attrs.append(stu.AttrData(name="mtoa_abc_removeAttribPrefices", shortName="rematp", type=arnold.AI_TYPE_STRING, defaultValue=""))
      attrs.append(stu.AttrData(name="mtoa_abc_objectAttribs", shortName="objatrs", type=arnold.AI_TYPE_BOOLEAN, defaultValue=False))
      attrs.append(stu.AttrData(name="mtoa_abc_primitiveAttribs", shortName="pratrs", type=arnold.AI_TYPE_BOOLEAN, defaultValue=False))
      attrs.append(stu.AttrData(name="mtoa_abc_pointAttribs", shortName="ptatrs", type=arnold.AI_TYPE_BOOLEAN, defaultValue=False))
      attrs.append(stu.AttrData(name="mtoa_abc_vertexAttribs", shortName="vtxatrs", type=arnold.AI_TYPE_BOOLEAN, defaultValue=False))
      attrs.append(stu.AttrData(name="mtoa_abc_attribsFrame", shortName="atrsfrm", type=arnold.AI_TYPE_ENUM, enums=["render", "shutter", "shutter_open", "shutter_close"], defaultValue=0))
      
      attrs.append(stu.AttrData(name="mtoa_abc_samplesExpandIterations", shortName="sampexpiter", type=arnold.AI_TYPE_INT, defaultValue=0, min=0, max=10))
      attrs.append(stu.AttrData(name="mtoa_abc_optimizeSamples", shortName="optsamp", type=arnold.AI_TYPE_BOOLEAN, defaultValue=False))
      
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
            
            self.addControl("mtoa_abc_referenceFilename", label="Reference ABC File")
            self.addSeparator()
            
            self.addControl("mtoa_abc_computeTangents", label="Compute Tangents")
            self.addSeparator()
            
            self.addControl("mtoa_abc_radiusScale", label="Radius Scale")
            self.addControl("mtoa_abc_radiusMin", label="Min. Radius")
            self.addControl("mtoa_abc_radiusMax", label="Max. Radius")
            self.addControl("mtoa_abc_nurbsSampleRate", label="NURBS Sample Rate")
            self.addSeparator()
            
            self.addControl("mtoa_abc_overrideAttribs", label="Override Attributes")
            self.addControl("mtoa_abc_removeAttribPrefices", label="Remove Attribute Prefices")
            self.addControl("mtoa_abc_objectAttribs", label="Output Object Attributes")
            self.addControl("mtoa_abc_primitiveAttribs", label="Output Primitive Attributes")
            self.addControl("mtoa_abc_pointAttribs", label="Output Point Attributes")
            self.addControl("mtoa_abc_vertexAttribs", label="Output Vertex Attributes")
            self.addControl("mtoa_abc_attribsFrame", label="Attributes Frame")
            self.addSeparator()
            
            self.addControl("mtoa_abc_samplesExpandIterations", label="Expand Samples Iterations")
            self.addControl("mtoa_abc_optimizeSamples", label="Optimize Samples")
            self.addSeparator()
            
            self.addControl("aiStepSize", label="Volume Step Size")
            self.addSeparator()
            
            self.addControl("aiUserOptions", label="User Options")
            self.addSeparator()
     
      templates.registerTranslatorUI(<<NodeName>>Template, "<<NodeName>>", translator)
      
   except Exception, e:
      print("mtoa_<<NodeName>>.SetupAE: Failed\n%s" % e)
