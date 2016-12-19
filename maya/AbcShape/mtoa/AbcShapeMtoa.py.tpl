import mtoa.ui.ae.templates as templates
import maya.cmds as cmds

class <<NodeName>>Template(templates.ShapeTranslatorTemplate):
   def setup(self):
      self.commonShapeAttributes()
      self.addSeparator()
      
      self.renderStatsAttributes()
      
      self.addSeparator()
      
      self.addControl("aiSssSetname", label="SSS Set Name")
      self.addControl("aiStepSize", label="Volume Step Size")
      self.addControl("aiUserOptions", label="User Options")
      
      self.beginLayout("Bounds", collapse=True)
      self.addControl("mtoa_constant_abc_useOverrideBounds", label="Use Override Bounds", annotation="Use user defined bounds embedded in ABC file")
      self.addControl("mtoa_constant_abc_overrideBoundsMinName", label="Override Bounds Min Name")
      self.addControl("mtoa_constant_abc_overrideBoundsMaxName", label="Override Bounds Max Name")
      self.addControl("mtoa_constant_abc_computeVelocityExpandedBounds", label="Compute Velocity Expanded Bounds")
      self.addControl("mtoa_constant_abc_padBoundsWithPeakRadius", label="Pad With Peak Radius")
      self.addControl("mtoa_constant_abc_peakRadiusName", label="Peak Radius Name")
      self.addControl("mtoa_constant_abc_padBoundsWithPeakWidth", label="Pad With Peak Width")
      self.addControl("mtoa_constant_abc_peakWidthName", label="Peak Width Name")
      self.addControl("mtoa_constant_abc_boundsPadding", label="Bounds Padding")
      self.endLayout()
      
      self.beginLayout("Attributes", collapse=True)
      self.addControl("mtoa_constant_abc_objectAttribs", label="Output Object Attributes")
      self.addControl("mtoa_constant_abc_primitiveAttribs", label="Output Primitive Attributes")
      self.addControl("mtoa_constant_abc_pointAttribs", label="Output Point Attributes")
      self.addControl("mtoa_constant_abc_vertexAttribs", label="Output Vertex Attributes")
      self.addControl("mtoa_constant_abc_attribsFrame", label="Attributes Frame")
      self.addControl("mtoa_constant_abc_promoteToObjectAttribs", label="Promote To Object Attributes")
      self.addControl("mtoa_constant_abc_removeAttribPrefices", label="Remove Attribute Prefices")
      self.addSeparator()
      self.addControl("mtoa_constant_abc_overrideAttribs", label="Override Attributes")
      self.endLayout()
      
      self.beginLayout("Reference Object", collapse=True)
      self.addControl("mtoa_constant_abc_outputReference", label="Output Reference")
      self.addControl("mtoa_constant_abc_referenceSource", label="Reference Source")
      self.addControl("mtoa_constant_abc_referencePositionName", label="Reference Position Name")
      self.addControl("mtoa_constant_abc_referenceNormalName", label="Reference Normal Name")
      self.addControl("mtoa_constant_abc_referenceFilename", label="Reference ABC File")
      self.addControl("mtoa_constant_abc_referenceFrame", label="Reference Frame")
      self.endLayout()
      
      self.beginLayout("Velocity", collapse=True)
      self.addControl("mtoa_constant_abc_velocityScale", label="Velocity Scale")
      self.addControl("mtoa_constant_abc_velocityName", label="Velocity Name")
      self.addControl("mtoa_constant_abc_accelerationName", label="Acceleration Name")
      self.addControl("mtoa_constant_abc_forceVelocityBlur", label="Force Velocity Blur")
      self.endLayout()
      
      self.beginLayout("Mesh", collapse=True)
      self.addControl("mtoa_constant_abc_computeTangents", label="Compute Tangents")
      self.endLayout()
      
      self.beginLayout("Particles", collapse=True)
      self.addControl("mtoa_constant_abc_radiusName", label="Radius Name")
      self.addControl("mtoa_constant_abc_radiusScale", label="Radius Scale")
      self.addControl("mtoa_constant_abc_radiusMin", label="Min. Radius")
      self.addControl("mtoa_constant_abc_radiusMax", label="Max. Radius")
      self.endLayout()
      
      self.beginLayout("Curves", collapse=True)
      self.addControl("mtoa_constant_abc_ignoreNurbs", label="Ignore NURBS")
      self.addControl("mtoa_constant_abc_nurbsSampleRate", label="NURBS Sample Rate")
      self.addControl("mtoa_constant_abc_widthScale", label="Width Scale")
      self.addControl("mtoa_constant_abc_widthMin", label="Min. Width")
      self.addControl("mtoa_constant_abc_widthMax", label="Max. Width")
      self.endLayout()
      
      self.beginLayout("Samples Expansion", collapse=True)
      self.addControl("mtoa_constant_abc_samplesExpandIterations", label="Expand Samples Iterations")
      self.addControl("mtoa_constant_abc_optimizeSamples", label="Optimize Samples")
      self.endLayout()
      
      self.beginLayout("Others", collapse=True)
      self.addControl("mtoa_constant_abc_verbose", label="Verbose")
      self.endLayout()

templates.registerTranslatorUI(<<NodeName>>Template, "<<NodeName>>", "<<NodeName>>Mtoa")

