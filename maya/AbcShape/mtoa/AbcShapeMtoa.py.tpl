import mtoa.ui.ae.templates as templates

class <<NodeName>>Template(templates.ShapeTranslatorTemplate):
   def setup(self):
      self.commonShapeAttributes()
      self.addSeparator()
      
      self.renderStatsAttributes()
      self.addSeparator()
      
      self.addControl("mtoa_constant_abc_boundsPadding", label="Bounds Padding")
      self.addSeparator()
      
      self.addControl("mtoa_constant_abc_outputReference", label="Output Reference")
      self.addControl("mtoa_constant_abc_referenceFilename", label="Reference ABC File")
      self.addSeparator()
      
      self.addControl("mtoa_constant_abc_computeTangents", label="Compute Tangents")
      self.addSeparator()
      
      self.addControl("mtoa_constant_abc_radiusScale", label="Radius Scale")
      self.addControl("mtoa_constant_abc_radiusMin", label="Min. Radius")
      self.addControl("mtoa_constant_abc_radiusMax", label="Max. Radius")
      self.addControl("mtoa_constant_abc_nurbsSampleRate", label="NURBS Sample Rate")
      self.addSeparator()
      
      self.addControl("mtoa_constant_abc_velocityScale", label="Velocity Scale")
      self.addControl("mtoa_constant_abc_velocityName", label="Velocity Name")
      self.addControl("mtoa_constant_abc_accelerationName", label="Acceleration Name")
      self.addSeparator()
      
      self.addControl("mtoa_constant_useOverrideBounds", label="Use Override Bounds", annotation="Use user defined bounds embedded in ABC file")
      self.addControl("mtoa_constant_abc_overrideBoundsMinName", label="Override Bounds Min Name");
      self.addControl("mtoa_constant_abc_overrideBoundsMaxName", label="Override Bounds Max Name");
      self.addSeparator();
      
      self.addControl("mtoa_constant_abc_promoteToObjectAttribs", label="Promote To Object Attributes")
      self.addControl("mtoa_constant_abc_overrideAttribs", label="Override Attributes")
      self.addControl("mtoa_constant_abc_removeAttribPrefices", label="Remove Attribute Prefices")
      self.addControl("mtoa_constant_abc_objectAttribs", label="Output Object Attributes")
      self.addControl("mtoa_constant_abc_primitiveAttribs", label="Output Primitive Attributes")
      self.addControl("mtoa_constant_abc_pointAttribs", label="Output Point Attributes")
      self.addControl("mtoa_constant_abc_vertexAttribs", label="Output Vertex Attributes")
      self.addControl("mtoa_constant_abc_attribsFrame", label="Attributes Frame")
      #self.addControl("mtoa_attribsBlur", label="Attributes To Blur")
      self.addSeparator()
      
      self.addControl("mtoa_constant_abc_samplesExpandIterations", label="Expand Samples Iterations")
      self.addControl("mtoa_constant_abc_optimizeSamples", label="Optimize Samples")
      self.addSeparator()
      
      self.addControl("aiStepSize", label="Volume Step Size")
      self.addSeparator()
      
      self.addControl("aiUserOptions", label="User Options")
      self.addSeparator()

templates.registerTranslatorUI(<<NodeName>>Template, "<<NodeName>>", "<<NodeName>>Mtoa")

