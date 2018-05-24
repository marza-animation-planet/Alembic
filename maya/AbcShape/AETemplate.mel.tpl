
global proc AE<<NodeName>>SelectFile(string $attr)
{
   string $result[] = `fileDialog2 -returnFilter 1
                                   -fileFilter "Alembic File (*.abc)"
                                   -dialogStyle 2
                                   -caption "Select An Alembic File..."
                                   -fileMode 1`;
   
   if (size($result) == 0 || size($result[0]) == 0)
   {
      return;
   }
   else
   {
      setAttr $attr -type "string" $result[0];
   }
}

global proc AE<<NodeName>>FileNew(string $attr)
{
   setUITemplate -pushTemplate attributeEditorTemplate;
   
   columnLayout -adj true;
      rowLayout -nc 3;
         text -label "File Path";
         textField AE<<NodeName>>_FilePathField;
         symbolButton -image "navButtonBrowse.png" AE<<NodeName>>_FilePathBtn;
         setParent ..;
      setParent ..;
   setParent ..;
   
   setUITemplate -popTemplate;
   
   AE<<NodeName>>FileReplace($attr);
}

global proc AE<<NodeName>>FileReplace(string $attr)
{
   connectControl -fileName AE<<NodeName>>_FilePathField $attr;
   
   button -e -c ("AE<<NodeName>>SelectFile " + $attr) AE<<NodeName>>_FilePathBtn;
}

global proc AE<<NodeName>>Template(string $nodeName)
{
   editorTemplate -suppress "doubleSided";
   editorTemplate -suppress "opposite";
   editorTemplate -suppress "smoothShading";
   editorTemplate -suppress "controlPoints";
   editorTemplate -suppress "weights";
   editorTemplate -suppress "tweak";
   editorTemplate -suppress "relativeTweak";
   editorTemplate -suppress "currentUVSet";
   editorTemplate -suppress "currentColorSet";
   editorTemplate -suppress "uvSet";
   editorTemplate -suppress "displayColors";
   editorTemplate -suppress "displayColorChannel";
   editorTemplate -suppress "displayImmediate";
   editorTemplate -suppress "colorSet";
   editorTemplate -suppress "compInstObjGroups";
   editorTemplate -suppress "ignoreSelfShadowing";
   editorTemplate -suppress "featureDisplacement";
   editorTemplate -suppress "initialSampleRate";
   editorTemplate -suppress "extraSampleRate";
   editorTemplate -suppress "textureThreshold";
   editorTemplate -suppress "normalThreshold";
   editorTemplate -suppress "boundingBoxScale";
   editorTemplate -suppress "collisionOffsetVelocityIncrement";
   editorTemplate -suppress "collisionDepthVelocityIncrement";
   editorTemplate -suppress "geometryAntialiasingOverride";
   editorTemplate -suppress "antialiasingLevel";
   editorTemplate -suppress "shadingSamplesOverride";
   editorTemplate -suppress "shadingSamples";
   editorTemplate -suppress "maxShadingSamples";
   editorTemplate -suppress "maxVisibilitySamplesOverride";
   editorTemplate -suppress "maxVisibilitySamples";
   editorTemplate -suppress "volumeSamplesOverride";
   editorTemplate -suppress "volumeSamples";
   editorTemplate -suppress "depthJitter";
   editorTemplate -suppress "collisionEnable";
   editorTemplate -suppress "collisionOffset";
   editorTemplate -suppress "collisionDepth";
   editorTemplate -suppress "collisionPriority";
   editorTemplate -suppress "depthMapEnable";
   editorTemplate -suppress "depthMapWeight";
   editorTemplate -suppress "displayCollision";
   editorTemplate -suppress "collisionOffsetVelocityMultiplier";
   editorTemplate -suppress "collisionDepthVelocityMultiplier";
   
   // ---
   
   editorTemplate -beginScrollLayout;
   
   
   editorTemplate -callCustom AE<<NodeName>>FileNew AE<<NodeName>>FileReplace "filePath";
   editorTemplate -label "Objects" -addControl "objectExpression";
   editorTemplate -addSeparator;
   editorTemplate -label "Display Mode" -addControl "displayMode";
   editorTemplate -addSeparator;
   editorTemplate -label "Start Frame" -addControl "startFrame";
   editorTemplate -label "End Frame" -addControl "endFrame";
   editorTemplate -label "Time" -addControl "time";
   editorTemplate -label "Speed" -addControl "speed";
   editorTemplate -label "Preserve Start Frame" -addControl "preserveStartFrame";
   editorTemplate -label "Offset" -addControl "offset";
   editorTemplate -label "Cycle Type" -addControl "cycleType";
   editorTemplate -addSeparator;
   editorTemplate -beginNoOptimize;
   editorTemplate -label "Ignore Transforms" -addControl "ignoreXforms";
   editorTemplate -label "Ignore Instances" -addControl "ignoreInstances";
   editorTemplate -label "Ignore Visibility" -addControl "ignoreVisibility";
   editorTemplate -endNoOptimize;
   editorTemplate -addSeparator;
   editorTemplate -beginNoOptimize;
   editorTemplate -label "Draw Locators" -addControl "drawLocators";
   editorTemplate -label "Draw Transform Bounds" -addControl "drawTransformBounds";
   editorTemplate -label "Point Width" -addControl "pointWidth";
   editorTemplate -label "Line Width" -addControl "lineWidth";
   editorTemplate -endNoOptimize;
   editorTemplate -endLayout;
   
   source AEgeometryShapeTemplate;
   
   editorTemplate -beginLayout "Render Stats";
   editorTemplate -beginNoOptimize;
   editorTemplate -addControl "castsShadows";
   editorTemplate -addControl "receiveShadows";
   editorTemplate -addControl "motionBlur";
   editorTemplate -addControl "primaryVisibility";
   editorTemplate -addControl "smoothShading";
   editorTemplate -addControl "visibleInReflections";
   editorTemplate -addControl "visibleInRefractions";
   editorTemplate -interruptOptimize;
   editorTemplate -addControl "doubleSided" "checkDoubleSided";
   editorTemplate -addControl "opposite";
   editorTemplate -endNoOptimize;
   editorTemplate -endLayout;
   
   AEshapeTemplate $nodeName;
   
   if (`attributeQuery -node $nodeName -exists "vrayAbcVerbose"`)
   {
      editorTemplate -beginLayout "V-Ray Plugin" -collapse 0;
      editorTemplate -label "Ignore Reference" -addControl "vrayAbcIgnoreReference";
      editorTemplate -label "Reference Source" -addControl "vrayAbcReferenceSource";
      editorTemplate -label "Reference Frame" -addControl "vrayAbcReferenceFrame";
      editorTemplate -addSeparator;
      editorTemplate -beginNoOptimize;
      editorTemplate -label "Particle Type" -addControl "vrayAbcParticleType";
      editorTemplate -label "Particle Attribs" -addControl "vrayAbcParticleAttribs";
      editorTemplate -label "Sort IDs" -addControl "vrayAbcSortIDs";
      editorTemplate -label "Size Scale" -addControl "vrayAbcPsizeScale";
      editorTemplate -label "Min Size" -addControl "vrayAbcPsizeMin";
      editorTemplate -label "Max Size" -addControl "vrayAbcPsizeMax";
      editorTemplate -addSeparator;
      editorTemplate -label "Sprite Size X" -addControl "vrayAbcSpriteSizeX";
      editorTemplate -label "Sprite Size Y" -addControl "vrayAbcSpriteSizeY";
      editorTemplate -label "Sprite Twist" -addControl "vrayAbcSpriteTwist";
      editorTemplate -addSeparator;
      editorTemplate -label "Radius" -addControl "vrayAbcRadius";
      editorTemplate -addSeparator;
      editorTemplate -label "Point Size" -addControl "vrayAbcPointSize";
      editorTemplate -addSeparator;
      editorTemplate -label "Multi Count" -addControl "vrayAbcMultiCount";
      editorTemplate -label "Multi Radius" -addControl "vrayAbcMultiRadius";
      editorTemplate -addSeparator;
      editorTemplate -label "Line Width" -addControl "vrayAbcLineWidth";
      editorTemplate -label "Tail Length" -addControl "vrayAbcTailLength";
      editorTemplate -endNoOptimize;
      editorTemplate -addSeparator;
      editorTemplate -label "Verbose" -addControl "vrayAbcVerbose";
      editorTemplate -endLayout;
   }
   
   editorTemplate -addExtraControls;
   
   editorTemplate -endScrollLayout;
}
