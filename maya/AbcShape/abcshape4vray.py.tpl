
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
