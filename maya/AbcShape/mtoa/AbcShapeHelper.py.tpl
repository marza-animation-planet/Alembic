import os
import re
import sys
import cask
import maya.cmds as cmds

def AutoSetup(shapes=None, verbose=False):
   if not cmds.pluginInfo("<<NodeName>>", query=1, loaded=1):
      return
   
   if shapes is None:
      shapes = cmds.ls(type="<<NodeName>>")
   else:
      shapes = filter(lambda x: cmds.nodeType(x) == "<<NodeName>>", shapes)
   
   if not shapes:
      return
   
   archives = {}
   
   for shape in shapes:
      abcpath = cmds.getAttr(shape+".filePath")
      objpath = cmds.getAttr(shape+".objectExpression")
      ignoreXforms = cmds.getAttr(shape+".ignoreXforms")
      numShapes = cmds.getAttr(shape+".numShapes")
      singleShape = (numShapes == 1 and ignoreXforms)
      
      if verbose:
         print("Process %s ('%s')" % (abcpath, objpath))
      
      # Check if file exists
      if not os.path.isfile(abcpath):
         abcpath = cmds.dirmap(cd=abcpath)
         if not os.path.isfile(abcpath):
            continue
      
      # Build object path exression
      if objpath:
         try:
            e = re.compile(objpath)
         except Exception, e:
            print("Not a valid expression '%s' (%s)" % (objpath, e))
            continue
      else:
         e = re.compile(".*")
      
      # Read alembic file
      key = (abcpath.replace("\\", "/").lower() if sys.platform == "win32" else abcpath)
      a = archives.get(key, None)
      if a is None:
         try:
            a = cask.Archive(str(abcpath))
            archives[key] = a
         except Exception, e:
            print("Could not read alembic file '%s' (%s)" % (abcpath, e))
            continue
      
      # Check for attribute name overrides
      promoteAttrs = []
      names = {}
      for bn in ("overrideBoundsMin", "overrideBoundsMax", "peakRadius", "peakWidth"):
         on = cmds.getAttr(shape+".mtoa_constant_abc_%sName" % bn)
         names[bn] = (bn if not on else on)
      
      # Reset attributes
      cmds.setAttr(shape+".mtoa_constant_abc_useOverrideBounds", False)
      cmds.setAttr(shape+".mtoa_constant_abc_padBoundsWithPeakRadius", False)
      cmds.setAttr(shape+".mtoa_constant_abc_padBoundsWithPeakWidth", False)
      cmds.setAttr(shape+".mtoa_constant_abc_promoteToObjectAttribs", "", type="string")
      
      
      def _recurseNodes(node):
         for name, child in node.children.iteritems():
            typ = child.type()
            
            if not typ in ("Xform", "PolyMesh", "SubD", "Points", "Curve"):
               if verbose:
                  print("Skip node '%s' (Unsupported type %s)." % (child.iobject.getFullName(), typ))
               continue
            
            if typ == "Xform":
               _recurseNodes(child)
            
            else:
               if not e.search(child.iobject.getFullName()):
                  if verbose:
                     print("Skip node '%s' (Doesn't match expression)." % (child.iobject.getFullName()))
                  continue
               
               if verbose:
                  print("Found shape: '%s' (%s) [%s]" % (child.iobject.getFullName(), typ, "single" if singleShape else "multi"))
               
               props = child.properties.get(".geom", None)
               if props is None:
                  continue
               
               hasPref = False
               hasNref = False
               
               gprops = props.properties.get(".arbGeomParams", None)
               if gprops:
                  if names["overrideBoundsMin"] in gprops.properties and names["overrideBoundsMax"] in gprops.properties:
                     gsm = gprops.properties[names["overrideBoundsMin"]].iobject.getHeader().getMetaData().get("geoScope")
                     gsM = gprops.properties[names["overrideBoundsMax"]].iobject.getHeader().getMetaData().get("geoScope")
                     if gsm == gsM and gsm in ("con", "uni"):
                        if singleShape:
                           cmds.setAttr(shape+".mtoa_constant_abc_useOverrideBounds", True)
                        if gsm == "uni":
                           promoteAttrs.append(names["overrideBoundsMin"])
                           promoteAttrs.append(names["overrideBoundsMax"])
                  
                  if names["peakRadius"] in gprops.properties:
                     gs = gprops.properties[names["peakRadius"]].iobject.getHeader().getMetaData().get("geoScope")
                     if gs in ("con", "uni"):
                        if singleShape:
                           cmds.setAttr(shape+".mtoa_constant_abc_padBoundsWithPeakRadius", True)
                        if gs == "uni":
                           promoteAttrs.append(names["peakRadius"])
                  
                  if names["peakWidth"] in gprops.properties:
                     gs = gprops.properties[names["peakWidth"]].iobject.getHeader().getMetaData().get("geoScope")
                     if gs in ("con", "uni"):
                        if singleShape:
                           cmds.setAttr(shape+".mtoa_constant_abc_padBoundsWithPeakWidth", True)
                        if gs == "uni":
                           promoteAttrs.append(names["peakWidth"])
                  
                  hasPref = ("Pref" in gprops.properties)
                  hasNref = ("Nref" in gprops.properties)
               
               uprops = props.properties.get(".userProperties", None)
               if uprops:
                  if names["overrideBoundsMin"] in uprops.properties and names["overrideBoundsMax"] in uprops.properties:
                     if singleShape:
                        cmds.setAttr(shape+".mtoa_constant_abc_useOverrideBounds", True)
                  
                  if names["peakRadius"] in uprops.properties:
                     if singleShape:
                        cmds.setAttr(shape+".mtoa_constant_abc_padBoundsWithPeakRadius", True)
                  
                  if names["peakWidth"] in uprops.properties:
                     if singleShape:
                        cmds.setAttr(shape+".mtoa_constant_abc_padBoundsWithPeakWidth", True)
                  
                  if "hasReferenceObject" in uprops.properties and hasPref and hasNref:
                     p = uprops.properties["hasReferenceObject"]
                     if p.values[0]:
                        cmds.setAttr(shape+".mtoa_constant_abc_outputReference", True)
                        cmds.setAttr(shape+".mtoa_constant_abc_referenceSource", 1)
                        cmds.setAttr(shape+".mtoa_constant_abc_referencePositionName", "Pref", type="string")
                        cmds.setAttr(shape+".mtoa_constant_abc_referenceNormalName", "Nref", type="string")
      
      _recurseNodes(a.top)
      
      if promoteAttrs:
         cmds.setAttr(shape+".mtoa_constant_abc_promoteToObjectAttribs", " ".join(promoteAttrs), type="string")
   
   for _, a in archives.iteritems():
      a.close()
