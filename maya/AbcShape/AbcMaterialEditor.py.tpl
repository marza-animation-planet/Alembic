import re
from maya import cmds
from maya import mel

_abcType = "<<NodeName>>"
_name = "AbcMaterialEditor"
_allAbcs = []
_allShaders = []

def filterList(lst, flt):
   fltExp = re.compile(flt)
   return filter(lambda x: fltExp.match(x) != None, lst)

def hasConnectableUVCoords(n):
   if cmds.attributeQuery("uvCoord", node=n, exists=1):
      conns = cmds.listConnections(n+".uvCoord", s=1, d=0)
      if conns is None or len(conns) == 0 or cmds.nodeType(conns[0]) == "uvChooser" or hasConnectableUVCoords(conns[0]):
         return 1
      else:
         return 2
   else:
      return 0

def getConnectionPoint(n):
   conns = cmds.listConnections(n+".uvCoord", s=1, d=0)
   if conns is None or len(conns) == 0:
      return (n+".uvCoord")
   else:
      srcNode = conns[0]
      if cmds.nodeType(srcNode) == "uvChooser":
         return srcNode
      else:
         return getConnectionPoint(srcNode)

def getTextureNodes(n, connectableOnly=True, processedNodes=set()):
   nodes = set()
   uvSource = ""
   
   if n in processedNodes:
      return []
   
   processedNodes.add(n)
   
   hasUV = hasConnectableUVCoords(n)
   if hasUV == 1 or (hasUV == 2 and connectableOnly is False):
      nodes.add(n)
      conns = cmds.listConnections(n+".uvCoord", s=1, d=0)
      if conns != None and len(conns) > 0:
         uvSource = conns[0]
   
   conns = cmds.listConnections(n, s=1, d=0, c=1, p=1)
   if conns is None:
      conns = []
   
   i = 0
   while i < len(conns):
      
      dstAttr = re.sub("^[^.]*[.]", "", conns[i])
      if dstAttr in ["uvCoord", "uCoord", "vCoord"]:
         i += 2
         continue
      
      srcNode = re.sub("[.].*$", "", conns[i+1])
      if srcNode == uvSource:
         i += 2
         continue
      
      if cmds.nodeType(srcNode) == "place2dTexture":
         i += 2
         continue
      
      nodes = nodes.union(getTextureNodes(srcNode, connectableOnly, processedNodes))
      
      i += 2
   
   return nodes

def isSurfaceShader(node):
   nt = cmds.nodeType(node)
   clsl = cmds.getClassification(nt)
   for cls in clsl:
      tokens = cls.split(":")
      for token in tokens:
         if token.startswith("shader/surface"):
            return True
   return False

def getSurfaceShader(sg):
   conns = cmds.listConnections(sg+".surfaceShader", s=1, d=0)
   if conns != None and len(conns) == 1:
      return conns[0]
   return None

def getShadingEngine(shader):
   conns = cmds.listConnections(shader, s=0, d=1, type="shadingEngine")
   if conns is None or len(conns) == 0:
      return None
   elif len(conns) > 1:
      if len(conns) == 2:
         if conns[0] == "initialParticleSE":
            return conns[1]
         elif conns[1] == "initialParticleSE":
            return conns[0]
      mel.eval("warning(\"Surface shader '" + shader + "' is connected to several shading engines. Use the first one.\")")
   return conns[0]

def getAbcShapeInSelection():
   global _abcType
   
   abcs = set()
   sel = cmds.ls(sl=1)
   
   if sel is None or len(sel) == 0:
      return []
   
   for item in sel:
      if cmds.nodeType(item) == _abcType:
         abcs.add(item)
      
      elif isSurfaceShader(item):
         sgs = cmds.listConnections(item, s=0, d=1, type="shadingEngine")
         if sgs:
            for sg in sgs:
               members = cmds.sets(sg, query=1)
               if members:
                  for member in members:
                     if cmds.nodeType(member) == _abcType:
                        abcs.add(member)
      
      else:
         descendents = cmds.listRelatives(item, allDescendents=1, path=1, type=_abcType)
         if descendents:
            abcs = abcs.union(set(descendents))
   
   abcs = list(abcs)
   abcs.sort()
   return abcs

def getAbcShapeUVSetIndex(abc, uv):
   uvSetName = ""
   
   if cmds.attributeQuery("uvSet", node=abc, exists=1,  multi=1) == 0 or \
      cmds.attributeQuery("uvSetName", node=abc, exists=1) == 0 or \
      cmds.attributeQuery("uvSetCount", node=abc, exists=1) == 0:
      return -1
   
   pl = cmds.attributeQuery("uvSetName", node=abc, listParent=1)
   if pl is None or len(pl) != 1 or pl[0] != "uvSet":
      return -1
   
   for index in xrange(cmds.getAttr("%s.uvSetCount" % abc)):
      uvSetName = cmds.getAttr("%s.uvSet[%d].uvSetName" % (abc, index))
      
      if not uvSetName:
         break
      
      elif uvSetName == uv:
         return index
   
   return -1

def getAbcShapeUVSetNames(abc):
   uvSetNames = []
   
   if cmds.attributeQuery("uvSet", node=abc, exists=1,  multi=1) == 0 or \
      cmds.attributeQuery("uvSetName", node=abc, exists=1) == 0 or \
      cmds.attributeQuery("uvSetCount", node=abc, exists=1) == 0:
      return []
   
   pl = cmds.attributeQuery("uvSetName", node=abc, listParent=1)
   if pl is None or len(pl) != 1 or pl[0] != "uvSet":
      return []
   
   for index in xrange(cmds.getAttr("%s.uvSetCount" % abc)):
      uvSetName = cmds.getAttr("%s.uvSet[%d].uvSetName" % (abc, index))
      if uvSetName:
         uvSetNames.append(uvSetName)
   
   return uvSetNames

def getAbcShapeShader(abc):
   conns = cmds.listConnections(abc+".instObjGroups", s=0, d=1, type="shadingEngine")
   if conns is None or len(conns) == 0:
      return None
   else:
      return getSurfaceShader(conns[0])

def setAbcShapeShader(abc, shader):
   sg = getShadingEngine(shader)
   
   if sg is None:
      mel.eval("warning(\"Cannot assign shader '" + shader + "' to %s '" + abc + "'. Missing shadingEngine node.\")" % cmds.nodeType(abc))
      return
   
   cmds.sets(abc, e=1, forceElement=sg)

def listSurfaceShaders():
   nodes = cmds.ls(materials=1)
   if nodes is None:
      return []
   
   shaders = []
   for node in nodes:
      itemlist = cmds.getClassification(cmds.nodeType(node))
      if itemlist is None:
         continue
      
      for item in itemlist:
         clsl = item.split(":")
         
         for cls in clsl:
            if cls.startswith("shader/surface"):
               conns = cmds.listConnections(node, source=0, destination=1, type="shadingEngine")
               if not conns:
                  if cmds.attributeQuery("outColor", exists=1, node=node):
                     sg = cmds.sets(empty=1, noSurfaceShader=1, renderable=1, name=node+"SG")
                     cmds.connectAttr(node+".outColor", sg+".surfaceShader", force=1)
                  
                  elif cmds.attributeQuery("outValue", exists=1, node=node):
                     sg = cmds.sets(empty=1, noSurfaceShader=1, renderable=1, name=node+"SG")
                     cmds.connectAttr(node+".outValue", sg+".surfaceShader", force=1)
               
               shaders.append(node)
               break
            
            elif cls.startswith("shader/displacement"):
               conns = cmds.listConnections(node, source=0, destination=1, type="shadingEngine")
               if not conns:
                  if cmds.attributeQuery("displacement", exists=1, node=node):
                     sg = cmds.sets(empty=1, noSurfaceShader=1, renderable=1, name=node+"SG")
                     cmds.connectAttr(node+".displacement", sg+".displacementShader", force=1)
                  
                  elif cmds.attributeQuery("outColor", exists=1, node=node):
                     sg = cmds.sets(empty=1, noSurfaceShader=1, renderable=1, name=node+"SG")
                     cmds.connectAttr(node+".outColor", sg+".displacementShader", force=1)
               
               break
            
            elif cls.startswith("shader/volume"):
               conns = cmds.listConnections(node, source=0, destination=1, type="shadingEngine")
               if not conns and cmds.attributeQuery("outColor", exists=1, node=node):
                  sg = cmds.sets(empty=1, noSurfaceShader=1, renderable=1, name=node+"SG")
                  cmds.connectAttr(node+".outColor", sg+".volumeShader", force=1)
               
               break
   
   shaders.sort()
   return shaders

# ui related methods

def UI_updateAbcList(abcs):
   global _name
   
   cmds.textScrollList("%s_abcs" % _name, edit=1, removeAll=1)
   for abc in abcs:
      cmds.textScrollList("%s_abcs" % _name, edit=1, append=abc)

def UI_updateShaderList(shaders):
   global _name
   
   cmds.textScrollList("%s_shds" % _name, edit=1, removeAll=1)
   for shader in shaders:
      cmds.textScrollList("%s_shds" % _name, edit=1, append=shader)

def UI_refresh(*args):
   global _allAbcs, _allShaders, _name
   
   _allAbcs = getAbcShapeInSelection()
   flt = cmds.textField("%s_abcFilter" % _name, query=1, text=1)
   UI_updateAbcList(filterList(_allAbcs, ".*" if flt == "" else flt))
   
   _allShaders = listSurfaceShaders()
   flt = cmds.textField("%s_shdFilter" % _name, query=1, text=1)
   UI_updateShaderList(filterList(_allShaders, ".*" if flt == "" else flt))
   
   cmds.textScrollList("%s_uvs" % _name, edit=1, removeAll=1)
   cmds.textScrollList("%s_texs" % _name, edit=1, removeAll=1)
   
   cmds.textScrollList("%s_shds" % _name, edit=1, enable=0)
   cmds.textScrollList("%s_texs" % _name, edit=1, enable=0)
   cmds.textScrollList("%s_uvs" % _name, edit=1, enable=0)

def UI_selectAbc(*args):
   global _name
   
   abcs = cmds.textScrollList("%s_abcs" % _name, query=1, selectItem=1)
   if abcs is None:
      abcs = []
   
   shaders = cmds.textScrollList("%s_shds" % _name, query=1, allItems=1)
   if shaders is None:
      shaders = []
   
   cmds.textScrollList("%s_shds" % _name, edit=1, enable=(len(abcs)>0))
   cmds.textScrollList("%s_uvs" % _name, edit=1, removeAll=1)
   
   if len(abcs) == 1:
      abc = abcs[0]
      
      # fill UVs
      uvs = getAbcShapeUVSetNames(abc)
      for uv in uvs:
         cmds.textScrollList("%s_uvs" % _name, edit=1, append=uv)
      
      # select linked shader
      shader = getAbcShapeShader(abc)
      if shader in shaders:
         cmds.textScrollList("%s_shds" % _name, edit=1, selectItem=shader)
      else:
         cmds.textScrollList("%s_shds" % _name, edit=1, deselectAll=1)
   
   elif len(abcs) > 1:
      sameShader = True
      sharedShader = getAbcShapeShader(abcs[0])
      
      for abc in abcs[1:]:
         shader = getAbcShapeShader(abc)
         if shader != sharedShader:
            sameShader = False
            break
      
      if sameShader and sharedShader != None:
         cmds.textScrollList("%s_shds" % _name, edit=1, selectItem=sharedShader)
      else:
         cmds.textScrollList("%s_shds" % _name, edit=1, deselectAll=1)
   
   UI_selectShader()

def UI_selectShader(*args):
   global _name
   
   abcs = cmds.textScrollList("%s_abcs" % _name, query=1, selectItem=1)
   if abcs is None:
      abcs = []
   
   shaders = cmds.textScrollList("%s_shds" % _name, query=1, selectItem=1)
   if shaders is None:
      shaders = []
   
   cmds.textScrollList("%s_texs" % _name, edit=1, removeAll=1)
   
   if len(abcs) != 1:
      # need to review this a little
      cmds.textScrollList("%s_texs" % _name, edit=1, enable=0)
      if len(shaders) > 0:
         for abc in abcs:
            setAbcShapeShader(abc, shaders[0])
      
   else:
      abc = abcs[0]
      if len(shaders) == 0:
         cmds.textScrollList("%s_texs"%_name, edit=1, enable=0)
         
      else:
         selTex = None
         shader = shaders[0]
         setAbcShapeShader(abc, shader)
         texs = getTextureNodes(shader, True, set())
         cmds.textScrollList("%s_texs" % _name, edit=1, enable=(len(texs)>0))
         
         for tex in texs:
            cmds.textScrollList("%s_texs" % _name, edit=1, append=tex)
            if selTex is None:
               selTex = tex
         
         if selTex != None:
            cmds.textScrollList("%s_texs" % _name, edit=1, selectItem=selTex)
   
   UI_selectTex()

def UI_selectTex(*args):
   global _name
   
   abcs = cmds.textScrollList("%s_abcs" % _name, query=1, selectItem=1)
   if abcs is None:
      abcs = []
   
   texs = cmds.textScrollList("%s_texs" % _name, query=1, selectItem=1)
   if texs is None:
      texs = []
   
   if len(abcs) != 1:
      cmds.textScrollList("%s_uvs" % _name, edit=1, enable=0, deselectAll=1)
   
   else:
      # strip parent names, only check shape base name
      abc = abcs[0].split('|')[-1]
      cmds.textScrollList("%s_uvs" % _name, edit=1, enable=(len(texs)>0))
      
      primaryUV = None
      sharedUV = None
      
      for tex in texs:
         uv = None
         cp = getConnectionPoint(tex)
         cn = re.sub("[.].*$", "", cp)
         
         if cp == cn:
            conns = cmds.listConnections(cp+".uvSets", s=1, d=0, sh=1, c=1, p=1)
            
            if conns:
               i = 0
               
               while i < len(conns):
                  m = re.match(r"^([^.]+)\.(.*)$", conns[i+1])
                  # strip parent names, only compare to shape base name
                  if m and m.group(1).split("|")[-1] == abc:
                     m = re.match(r"^uvSet\[(\d+)\]\.uvSetName$", m.group(2))
                     
                     uv = cmds.getAttr(conns[i])
                     
                     if m and int(m.group(1)) == 0:
                        primaryUV = uv
                     
                     break
                  
                  i += 2
         
         if uv is None:
            if primaryUV is None:
               if cmds.getAttr(abc+".uvSetCount") > 0:
                  primaryUV = cmds.getAttr(abc+".uvSet[0].uvSetName")
            uv = primaryUV
         
         if sharedUV is None:
            sharedUV = uv
         
         elif uv != sharedUV:
            sharedUV = None
            break
      
      if sharedUV is None:
         cmds.textScrollList("%s_uvs" % _name, edit=1, deselectAll=1)
      else:
         # sharedUV could be "" ?
         cmds.textScrollList("%s_uvs" % _name, edit=1, selectItem=sharedUV)
   
   UI_selectUV()

def UI_selectUV(*args):
   global _name
   
   abcs = cmds.textScrollList("%s_abcs" % _name, query=1, selectItem=1)
   if abcs is None or len(abcs) != 1:
      return
   
   texs = cmds.textScrollList("%s_texs" % _name, query=1, selectItem=1)
   if texs is None or len(texs) == 0:
      return
   
   uvs = cmds.textScrollList("%s_uvs" % _name, query=1, selectItem=1)
   if uvs is None or len(uvs) != 1:
      return
   
   abc = abcs[0]
   # only check base name
   abcbn = abc.split('|')[-1]
   uv = uvs[0]
   
   isPrimaryUV = True
   if cmds.getAttr(abc+".uvSetCount") > 0:
      isPrimaryUV = (uv == cmds.getAttr(abc+".uvSet[0].uvSetName"))
   
   for tex in texs:
      cp = getConnectionPoint(tex)
      cn = re.sub("[.].*$", "", cp)
      
      if isPrimaryUV:
         if cn == cp:
            # disconnect abc shape from uvChooser
            conns = cmds.listConnections(cp+".uvSets", s=1, d=0, sh=1, p=1, c=1)
            if conns != None:
               i = 0
               while i < len(conns):
                  srcAttr = conns[i+1]
                  srcNode = re.sub("[.].*$", "", srcAttr)
                  # only compare to base name
                  if srcNode.split('|')[-1] == abcbn:
                     cmds.disconnectAttr(conns[i+1], conns[i])
                     break
                  i += 2
               
               # delete uvChooser if no more shape connected
               conns = cmds.listConnections(cp+".uvSets", s=1, d=0, sh=1)
               if conns is None or len(conns) == 0:
                  cmds.delete(cp)
      
      else:
         if cn == cp:
            conns = cmds.listConnections(cp+".uvSets", s=1, d=0, sh=1, p=1, c=1)
            if conns != None:
               i = 0
               while i < len(conns):
                  srcAttr = conns[i+1]
                  srcNode = re.sub("[.].*$", "", srcAttr)
                  # only compare to base name
                  if srcNode.split('|')[-1] == abcbn:
                     if cmds.getAttr(conns[i]) != uv:
                        cmds.disconnectAttr(conns[i+1], conns[i])
                        break
                     else:
                        # already linked to right uv set
                        return
                  i += 2
         
         else:
            cn = cmds.createNode("uvChooser", ss=1)
            cmds.connectAttr(cn+".uvCoord", cp)
         
         # do connect
         uvi = getAbcShapeUVSetIndex(abc, uv)
         if uvi == -1:
            mel.eval("warning(\"Invalid uv set name '" + uv + "' for shape '" + abc + "'\")")
            return
         
         i = 0
         done = False
         while not done:
            conns = cmds.listConnections("%s.uvSets[%d]" % (cn, i), s=1, d=0, sh=1)
            if conns is None or len(conns) == 0:
               cmds.connectAttr("%s.uvSet[%d].uvSetName" % (abc, uvi), "%s.uvSets[%d]" % (cn, i))
               done = True
            i += 1

def UI_all(*args):
   global _name
   
   items = cmds.textScrollList("%s_abcs" % _name, query=1, allItems=1)
   cmds.textScrollList("%s_abcs" % _name, edit=1, deselectAll=1)
   if items != None:
      for item in items:
         cmds.textScrollList("%s_abcs" % _name, edit=1, selectItem=item)
   
   UI_selectAbc()

def UI_select(*args):
   global _name
   
   items = cmds.textScrollList("%s_abcs" % _name, query=1, selectItem=1)
   if items is None or len(items) == 0:
      cmds.select(clear=1)
   else:
      cmds.select(items, replace=1)

def UI_updateFilter(*args):
   global _allAbcs, _allShaders, _name
   
   selAbcs = cmds.textScrollList("%s_abcs" % _name, query=1, selectItem=1)
   if selAbcs is None:
      selAbcs = []
   
   selShds = cmds.textScrollList("%s_shds" % _name, query=1, selectItem=1)
   if selShds is None:
      selShds = []
   
   selTexs = cmds.textScrollList("%s_texs" % _name, query=1, selectItem=1)
   if selTexs is None:
      selTexs = []
   
   flt = cmds.textField("%s_abcFilter" % _name, query=1, text=1)
   fabcs = filterList(_allAbcs, ".*" if flt == "" else flt)
   UI_updateAbcList(fabcs);
   
   flt = cmds.textField("%s_shdFilter" % _name, query=1, text=1)
   fshds = filterList(_allShaders, ".*" if flt == "" else flt)
   UI_updateShaderList(fshds);
   
   for selAbc in selAbcs:
      if selAbc in fabcs:
         cmds.textScrollList("%s_abcs" % _name, edit=1, selectItem=selAbc)
   
   if cmds.textScrollList("%s_abcs" % _name, query=1, numberOfSelectedItems=1) > 0:
      for selShd in selShds:
         if selShd in fshds:
            cmds.textScrollList("%s_shds" % _name, edit=1, selectItem=selShd)
   
   UI_selectShader()
   
   if cmds.textScrollList("%s_shds" % _name, query=1, numberOfSelectedItems=1) > 0:
      allTexs = cmds.textScrollList("%s_texs" % _name, query=1, allItems=1)
      if allTexs != None:
         for selTex in selTexs:
            if selTex in allTexs:
               cmds.textScrollList("%s_texs" % _name, edit=1, selectItem=selTex)
   
   UI_selectTex()

def UI_selectNode(lst):
   def callback(*args):
      nodes = cmds.textScrollList(lst, query=1, selectItem=1)
      if nodes != None and len(nodes) == 1:
         cmds.select(nodes[0], replace=1)
   
   return callback

def showUI():
   global _name, _abcType
   
   if not cmds.pluginInfo(_abcType, query=1, loaded=1):
      cmds.loadPlugin(_abcType)
   
   if cmds.window("AbcMaterialEditor", query=1, exists=1):
      cmds.deleteUI("AbcMaterialEditor")
   
   win = cmds.window("AbcMaterialEditor", title="Abc Material Editor", menuBar=1, width=640, height=480)
   
   menu = cmds.menu(label="Option", tearOff=0)
   cmds.menuItem(label="Refresh", command=UI_refresh)
   
   form = cmds.formLayout(numberOfDivisions=100)
   
   abcLbl = cmds.text(label="Alembic Shapes")
   abcFlt = cmds.textField("%s_abcFilter"%_name)
   abcLst = cmds.textScrollList("%s_abcs"%_name, allowMultiSelection=1)
   
   refreshBtn = cmds.button(label="Refresh")
   allBtn = cmds.button(label="All")
   selBtn = cmds.button(label="Select")
   
   shdLbl = cmds.text(label="Shaders")
   shdFlt = cmds.textField("%s_shdFilter"%_name)
   shdLst = cmds.textScrollList("%s_shds"%_name, allowMultiSelection=0)
   
   texLbl = cmds.text(label="Textures")
   texLst = cmds.textScrollList("%s_texs"%_name, allowMultiSelection=1, enable=0)
   
   uvLbl = cmds.text(label="UV sets")
   uvLst = cmds.textScrollList("%s_uvs"%_name, allowMultiSelection=0, enable=0)
   
   cmds.textScrollList(abcLst, edit=1, selectCommand=UI_selectAbc, doubleClickCommand=UI_selectNode(abcLst))
   cmds.textScrollList(shdLst, edit=1, selectCommand=UI_selectShader, doubleClickCommand=UI_selectNode(shdLst))
   cmds.textScrollList(texLst, edit=1, selectCommand=UI_selectTex, doubleClickCommand=UI_selectNode(texLst))
   cmds.textScrollList(uvLst, edit=1, selectCommand=UI_selectUV)
   cmds.button(refreshBtn, edit=1, command=UI_refresh)
   cmds.button(allBtn, edit=1, command=UI_all)
   cmds.button(selBtn, edit=1, command=UI_select)
   cmds.textField(abcFlt, edit=1, changeCommand=UI_updateFilter)
   cmds.textField(shdFlt, edit=1, changeCommand=UI_updateFilter)
   
   cmds.formLayout(form, edit=1,
                           attachForm = [(abcLbl, "top", 5), (abcLbl, "left", 5),
                                                (abcFlt, "left", 5),
                                                (abcLst, "left", 5),
                                                (refreshBtn, "left", 5), (refreshBtn, "bottom", 5),
                                                (allBtn, "bottom", 5),
                                                (selBtn, "bottom", 5),
                                                (shdLbl, "top", 5),
                                                (shdLst, "bottom", 5),
                                                (texLbl, "top", 5), (texLbl, "right", 5),
                                                (texLst, "right", 5),
                                                (uvLbl, "right", 5),
                                                (uvLst, "right", 5), (uvLst, "bottom", 5)],
                           attachPosition = [(abcLbl, "right", 2, 33),
                                                      (abcFlt, "right", 2, 33),
                                                      (abcLst, "right", 2, 33),
                                                      (refreshBtn, "right", 0, 11),
                                                      (allBtn, "left", 0, 11), (allBtn, "right", 0, 22),
                                                      (selBtn, "left", 0, 22), (selBtn, "right", 2, 33),
                                                      (shdLbl, "left", 2, 33), (shdLbl, "right", 2, 66),
                                                      (shdFlt, "left", 2, 33), (shdFlt, "right", 2, 66),
                                                      (shdLst, "left", 2, 33), (shdLst, "right", 2, 66),
                                                      (texLbl, "left", 2, 66),
                                                      (texLst, "left", 2, 66), (texLst, "bottom", 2, 50),
                                                      (uvLbl, "left", 2, 66), (uvLbl, "top", 2, 50),
                                                      (uvLst, "left", 2, 66)],
                           attachNone = [(abcLbl, "bottom"),
                                                (abcFlt, "bottom"),
                                                (refreshBtn, "top"),
                                                (allBtn, "top"),
                                                (selBtn, "top"),
                                                (shdLbl, "bottom"),
                                                (shdFlt, "bottom"),
                                                (uvLbl, "bottom")],
                           attachControl = [(abcFlt, "top", 5, abcLbl),
                                                    (abcLst, "top", 5, abcFlt), (abcLst, "bottom", 5, refreshBtn),
                                                    (shdFlt, "top", 5, shdLbl),
                                                    (shdLst, "top", 5, shdFlt),
                                                    (texLst, "top", 5, texLbl),
                                                    (uvLst, "top", 5, uvLbl)])
   
   UI_refresh()
   cmds.showWindow(win)

