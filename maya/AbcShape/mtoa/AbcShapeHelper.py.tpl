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
                  hasPref = ("Pref" in gprops.properties)
                  hasNref = ("Nref" in gprops.properties)
               
               uprops = props.properties.get(".userProperties", None)
               if uprops:
                  if "hasReferenceObject" in uprops.properties and hasPref and hasNref:
                     p = uprops.properties["hasReferenceObject"]
                     if p.values[0]:
                        cmds.setAttr(shape+".aiOutputReference", True)
                        cmds.setAttr(shape+".aiReferenceSource", 1)
                        cmds.setAttr(shape+".aiReferencePositionName", "Pref", type="string")
                        cmds.setAttr(shape+".aiReferenceNormalName", "Nref", type="string")
      
      _recurseNodes(a.top)
   
   for _, a in archives.iteritems():
      a.close()
