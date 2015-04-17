import maya.cmds as cmds
import maya.mel as mel
import re
import os

_attrs = {}
_groups = {}
_nodes = {}

def ReadAttributesInfo(path):
   global _attrs
   
   if len(_attrs) > 0:
      return
   
   if os.path.isfile(path):
      
      declexp = re.compile(r"\s*Attribute\s+([^\s]+)\s*\{")
      attrexp = re.compile(r"\s*([^\s]+)\s*=\s*(.*)$")
      
      inattr = False
      attrname = None
      
      f = open(path, "r")
      for l in f.readlines():
         if not inattr:
            m = declexp.match(l)
            if m:
               inattr = True
               attrname = m.group(1)
               _attrs[attrname] = {}
         else:
            m = attrexp.match(l)
            if m:
               _attrs[attrname][m.group(1)] = m.group(2).strip()
            elif l.strip() == "}":
               inattr = False
      f.close()

def ReadGroupsInfo(path):
   global _groups
   
   if len(_groups) > 0:
      return
   
   if os.path.isfile(path):
      
      declexp = re.compile(r"\s*AttributeGroup\s+([^\s]+)\s*\{")
      attrexp = re.compile(r"\s*([^\s]+)\s*=\s*(.*)$")
      
      ingroup = False
      groupname = None
      
      f = open(path, "r")
      for l in f.readlines():
         if not ingroup:
            m = declexp.match(l)
            if m:
               ingroup = True
               groupname = m.group(1)
               _groups[groupname] = {"attributes": []}
         else:
            m = attrexp.match(l)
            if m:
               if m.group(1).startswith("attribute_"):
                  _groups[groupname]["attributes"].append(m.group(2).strip())
               else:
                  _groups[groupname][m.group(1)] = m.group(2).strip()
            elif l.strip() == "}":
               ingroup = False
      f.close()

def ReadNodesInfo(path):
   global _nodes
   
   if len(_nodes) > 0:
      return
   
   if os.path.isfile(path):
      
      declexp = re.compile(r"\s*Node\s+([^\s]+)\s*\{")
      attrexp = re.compile(r"\s*([^\s]+)\s*=\s*(.*)$")
      
      innode = False
      nodename = None
      
      f = open(path, "r")
      for l in f.readlines():
         if not innode:
            m = declexp.match(l)
            if m:
               innode = True
               nodename = m.group(1)
               _nodes[nodename] = {"groups": []}
         else:
            m = attrexp.match(l)
            if m:
               if m.group(1).startswith("group"):
                  _nodes[nodename]["groups"].append(m.group(2).strip())
               else:
                  _nodes[nodename][m.group(1)] = m.group(2).strip()
            elif l.strip() == "}":
               innode = False
      f.close()

def BuildAEMenuForNode(name):
   global _nodes, _groups
   
   added = False
   
   if len(_nodes) == 0:
      r = mel.eval("whatIs vrayInit")
      m = re.match(r"^[^:]+:\s+(.*)$", r)
      
      if m:
         dn = os.path.dirname(m.group(1))
         ReadAttributesInfo(os.path.join(dn, "attributes.txt"))
         ReadGroupsInfo(os.path.join(dn, "attributeGroups.txt"))
         ReadNodesInfo(os.path.join(dn, "attributeNodes.txt"))
   
   nd = _nodes.get(cmds.nodeType(name), None)
   if nd:
      for g in nd["groups"]:
         gd = _groups.get(g, None)
         if not gd:
            continue
         
         checked = (False if len(gd["attributes"]) == 0 else cmds.attributeQuery(gd["attributes"][0], node=name, exists=1))
         item = cmds.menuItem(label=gd.get("label", g), checkBox=checked)
         #cmds.menuItem(item, edit=1, command="vray addAttributesFromGroup %s %s %d" % (name, g, 0 if checked else 1))
         cmds.menuItem(item, edit=1, command="cmds.vray('addAttributesFromGroup', '%s', '%s', %d)" % (name, g, 0 if checked else 1))
         
         added = True
   
   return added


def SetupUvChoosers():
   PostTranslate(disp=False, multiuv=True)

def CreateDispTextures():
   PostTranslate(disp=True, multiuv=False)

def PostTranslate(disp=True, multiuv=True):
   try:
      import vray.utils
   except:
      return
   
   if not cmds.pluginInfo("<<NodeName>>", query=1, loaded=1):
      return
   
   print("<<NodeName>> V-Ray Post Translate")
   
   # Setup uv switch nodes
   shapes = (cmds.<<NodeName>>VRayInfo(multiuvlist=1) if multiuv else None)
   
   if shapes:
      
      for shape in map(str, shapes):
         
         shapenode = vray.utils.findByName(shape)
         
         if not shapenode:
            continue
         
         node = vray.utils.findByName(shapenode[0].name().split('@')[0] + "@node")
         if not node:
            continue
         
         node = node[0]
         
         uvswitchs = cmds.<<NodeName>>VRayInfo(multiuv=shape, uvswitchlist=1)
         
         if uvswitchs:
            
            for name in map(str, uvswitchs):
               
               switcher = vray.utils.findByName(name)
               if not switcher:
                  continue
               
               switcher = switcher[0]
               
               index = cmds.<<NodeName>>VRayInfo(multiuv=shape, uvindex=name)
               
               if index < 0:
                  continue
               
               nodes = switcher.get("nodes")
               values = switcher.get("values")
               
               nodes.append(node)
               values.append(index)
               
               switcher.set("nodes", nodes)
               switcher.set("values", values)
   
   # Setup subdivision/displacement nodes
   disps = (cmds.<<NodeName>>VRayInfo(displist=1) if disp else None)
   
   if disps:
      
      for disp in map(str, disps):
         
         dispnode = vray.utils.exportTexture(str(disp))
         
         if not dispnode:
            continue
         
         shapes = cmds.<<NodeName>>VRayInfo(disp=disp, f=1)
         
         if shapes:
            """
            # Per-shape (why?)
            TexFloat cubeShape1@displTexFloat {
              input=checker1@luminance::luminance;
            }
            
            # Shared
            TexLuminance checker1@luminance {
              input=checker1;
            }
            
            or
            
            TexFloat cubeShape1@displTexFloat {
              input=checker1@color::alpha;
            }
            
            TexAColorOp checker1@color {
               color_a=checker1;
            }
            """
            
            lumnode = vray.utils.create("TexLuminance", disp+"@luminance")
            if not lumnode:
               continue
            
            lumnode.set("input", dispnode)
            
            for shape in map(str, shapes):
               shapenode = vray.utils.findByName(shape)
               if not shapenode:
                  continue
               
               texfnode = vray.utils.create("TexFloat", shape+"@displTexFloat")
               if not texfnode:
                  continue
               
               texfnode.set("input", lumnode) # ::luminance?
               
               shapenode[0].set("displacement_tex_float", texfnode)
         
         shapes = cmds.<<NodeName>>VRayInfo(disp=disp, c=1)
         if shapes:
            for shape in map(str, shapes):
               # connect to 'displacement_tex_color'
               shapenode = vray.utils.findByName(shape)
               if not shapenode:
                  continue
               
               shapenode[0].set("displacement_tex_color", dispnode)
   
   # Clear post translate information cache
   cmds.<<NodeName>>VRayInfo(reset=1)
