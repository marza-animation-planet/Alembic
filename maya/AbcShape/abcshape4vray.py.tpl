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

def CreateDispTextures():
   try:
      import vray.utils
   except:
      return
   
   import maya.cmds
   
   disps = maya.cmds.<<NodeName>>VRayDisp(displist=1)
   
   if disps:
      
      for disp in map(str, disps):
         print("Export disp texture \"%s\"" % disp)
         
         dispnode = vray.utils.exportTexture(str(disp))
         
         if not dispnode:
            continue
         
         shapes = maya.cmds.<<NodeName>>VRayDisp(disp=disp, f=1)
         
         if shapes:
            """
            # Per-shape (why?)
            TexFloat cubeShape1@displTexFloat {
              input=checker1_luminance@luminance::luminance;
            }
            
            # Shared
            TexLuminance checker1_luminance@luminance {
              input=checker1;
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
         
         shapes = maya.cmds.<<NodeName>>VRayDisp(disp=disp, c=1)
         if shapes:
            for shape in map(str, shapes):
               # connect to 'displacement_tex_color'
               shapenode = vray.utils.findByName(shape)
               if not shapenode:
                  continue
               
               shapenode[0].set("displacement_tex_color", dispnode)
      
      maya.cmds.<<NodeName>>VRayDisp(reset=1)
