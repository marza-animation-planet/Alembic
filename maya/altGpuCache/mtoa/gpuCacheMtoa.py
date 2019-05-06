import mtoa.ui.ae.templates as templates
import maya.cmds as cmds

class gpuCacheTemplate(templates.ShapeTranslatorTemplate):
   def setup(self):
      self.commonShapeAttributes()
      self.addSeparator()

      self.renderStatsAttributes()
      self.addSeparator()

      self.addControl("aiSssSetname", label="SSS Set Name")
      # trace sets added in commonShapeAttributes
      self.addControl("aiStepSize", label="Volume Step Size")
      self.addControl("aiVolumePadding", label="Volume Padding")
      self.addControl("aiUserOptions", label="User Options")

      self.beginLayout("Velocity", collapse=True)
      self.addControl("aiVelocityName", label="Velocity Name")
      self.addControl("aiAccelerationName", label="Acceleration Name")
      self.addControl("aiVelocityScale", label="Velocity Scale")
      self.addControl("aiForceVelocityBlur", label="Force Velocity Blur")
      self.endLayout()

      self.beginLayout("Reference Object", collapse=True)
      self.addControl("aiIgnoreReference", label="Ignore Reference")
      self.addControl("aiReferenceSource", label="Reference Source")
      self.addControl("aiReferencePositionName", label="Reference Position Name")
      self.addControl("aiReferenceNormalName", label="Reference Normal Name")
      self.addControl("aiReferenceFrame", label="Reference Frame")
      self.endLayout()

      self.beginLayout("Attributes", collapse=True)
      self.addControl("aiRemoveAttributePrefices", label="Remove Attribute Prefices")
      self.addControl("aiForceConstantAttributes", label="Force Constant Attributes")
      self.addControl("aiIgnoreAttributes", label="Ignore Attributes")
      self.addControl("aiAttributesEvaluationTime", label="Attributes Evaluation Time")
      self.endLayout()

      self.beginLayout("Mesh", collapse=True)
      self.addControl("aiComputeTangentsForUVs", label="Compute Tangents For UVs")
      self.endLayout()

      self.beginLayout("Particles", collapse=True)
      self.addControl("aiRadiusName", label="Radius Name")
      self.addControl("aiRadiusScale", label="Radius Scale")
      self.addControl("aiRadiusMin", label="Min. Radius")
      self.addControl("aiRadiusMax", label="Max. Radius")
      self.endLayout()

      self.beginLayout("Curves", collapse=True)
      self.addControl("aiIgnoreNurbs", label="Ignore NURBS")
      self.addControl("aiNurbsSampleRate", label="NURBS Sample Rate")
      self.addControl("aiWidthScale", label="Width Scale")
      self.addControl("aiWidthMin", label="Min. Width")
      self.addControl("aiWidthMax", label="Max. Width")
      self.endLayout()

      self.beginLayout("Sampling", collapse=True)
      self.addControl("aiExpandSamplesIterations", label="Expand Samples Iterations")
      self.addControl("aiOptimizeSamples", label="Optimize Samples")
      self.endLayout()

      self.beginLayout("Others", collapse=True)
      self.addControl("aiNameprefix", label="Arnold Names Prefix")
      self.addControl("aiVerbose", label="Verbose")
      self.endLayout()

templates.registerTranslatorUI(gpuCacheTemplate, "altGpuCache", "gpuCacheMtoa")

