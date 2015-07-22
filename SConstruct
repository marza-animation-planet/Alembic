import os
import sys
import glob
import excons
from excons.tools import hdf5
from excons.tools import python
from excons.tools import ilmbase
from excons.tools import boost
from excons.tools import gl
from excons.tools import glut
from excons.tools import glew
from excons.tools import arnold
from excons.tools import maya
from excons.tools import vray


deps_inc, deps_lib = excons.GetDirs("deps", noexc=True, silent=True)

nameprefix = excons.GetArgument("name-prefix", default="")

regex_src_dir = "lib/SceneHelper/regex-2.7/src"


def RequireAlembic(withPython=False, withGL=False):
   
   def _RequireFunc(env):
      global deps_inc, deps_lib
      
      if sys.platform == "darwin":
         import platform
         vers = map(int, platform.mac_ver()[0].split("."))
         if vers[0] > 10 or vers[1] >= 9:
            # clang complains a lot, make it quieter
            env.Append(CPPFLAGS=" ".join([" -std=c++11",
                                          "-Wno-deprecated-register",
                                          "-Wno-deprecated-declarations",
                                          "-Wno-missing-field-initializers",
                                          "-Wno-unused-parameter",
                                          "-Wno-unused-value",
                                          "-Wno-unused-function",
                                          "-Wno-unused-variable",
                                          "-Wno-unused-private-field"]))
      elif sys.platform == "win32":
         env.Append(CCFLAGS=" /bigobj")
         env.Append(CPPDEFINES=["NOMINMAX"])
         # Work around the 2GB file limit with Visual Studio 10.0
         #   https://connect.microsoft.com/VisualStudio/feedback/details/627639/std-fstream-use-32-bit-int-as-pos-type-even-on-x64-platform
         #   https://groups.google.com/forum/#!topic/alembic-discussion/5ElRJkXhi9M
         if excons.mscver == "10.0":
            env.Prepend(CPPPATH=["lib/vc10fix"])
      
      defs = ["ALEMBIC_LIB_USES_BOOST"]
      incdirs = ["lib"]
      libdirs = []
      libs = []
      
      if sys.platform == "win32":
         #defs.extend(["PLATFORM_WINDOWS", "PLATFORM=WINDOWS"])
         defs.extend(["PLATFORM_WINDOWS"])
      
      elif sys.platform == "darwin":
         defs.extend(["PLATFORM_DARWIN", "PLATFORM=DARWIN"])
         libs.append("m")
      
      else:
         defs.extend(["PLATFORM_LINUX", "PLATFORM=LINUX"])
         libs.extend(["dl", "rt", "m"])
      
      if deps_inc:
         incdirs.append(deps_inc)
         if os.path.isdir(deps_inc + "/OpenEXR"):
           incdirs.append(deps_inc + "/OpenEXR")
      
      if deps_lib:
         libdirs.append(deps_lib)
      
      env.Append(CPPDEFINES=defs)
      
      env.Append(CPPPATH=incdirs)
      
      if libdirs:
         env.Append(LIBPATH=libdirs)
      
      if withGL:
         env.Append(LIBS=["AlembicAbcOpenGL"])
      
      env.Append(LIBS=["AlembicAbcMaterial",
                       "AlembicAbcGeom",
                       "AlembicAbcCollection",
                       "AlembicAbcCoreFactory",
                       "AlembicAbc",
                       "AlembicAbcCoreHDF5",
                       "AlembicAbcCoreOgawa",
                       "AlembicAbcCoreAbstract",
                       "AlembicOgawa",
                       "AlembicUtil"])
      
      reqs = []
      
      if withGL:
         if sys.platform != "darwin":
           reqs.append(glew.Require)
         reqs.append(glut.Require)
         reqs.append(gl.Require)
      
      if withPython:
         reqs.append(ilmbase.Require(ilmthread=True, iexmath=True, python=True))
         reqs.append(boost.Require(libs=["python"]))
         reqs.append(python.SoftRequire)
         reqs.append(hdf5.Require(hl=True, verbose=True))
         
      else:
         reqs.append(ilmbase.Require(ilmthread=True, iexmath=True))
         reqs.append(boost.Require())
         reqs.append(hdf5.Require(hl=True, verbose=True))
      
      for req in reqs:
         req(env)
      
      if libs:
         env.Append(LIBS=libs)
   
   
   return _RequireFunc



def RequireAlembicHelper(withPython=False, withGL=False):
   global regex_src_dir
   
   def _RequireFunc(env):
      
      if sys.platform == "win32":
         env.Append(CPPDEFINES=["REGEX_STATIC"])
         env.Append(CPPPATH=[regex_src_dir])
      
      env.Append(CPPPATH=["lib/SceneHelper"])
      
      env.Append(LIBS=["SceneHelper"])
      
      requireAlembic = RequireAlembic(withPython=withPython, withGL=withGL)
      
      requireAlembic(env)
   
   
   return _RequireFunc



env = excons.MakeBaseEnv()

prjs = []

# Base libraries
for libname in ["Util",
                "Ogawa",
                "AbcCoreAbstract",
                "AbcCoreFactory",
                "AbcCoreOgawa",
                "AbcCoreHDF5",
                "Abc",
                "AbcGeom",
                "AbcCollection",
                "AbcMaterial"]:
   
   prjs.append({"name": "Alembic%s" % libname,
                "type": "staticlib",
                "srcs": glob.glob("lib/Alembic/%s/*.cpp" % libname),
                "custom": [RequireAlembic()],
                "install": {"include/Alembic/%s" % libname: glob.glob("lib/Alembic/%s/*.h" % libname)}})

prjs.append({"name": "AlembicAbcOpenGL",
             "type": "staticlib",
             "srcs": glob.glob("lib/AbcOpenGL/*.cpp"),
             "custom": [RequireAlembic(withGL=True)],
             "install": {"include/AbcOpenGL": glob.glob("lib/AbcOpenGL/*.h")}})


# Python modules
prjs.extend([{"name": ("alembicmodule" if sys.platform != "win32" else "alembic"),
              "type": "dynamicmodule",
              "ext": python.ModuleExtension(),
              "alias": "pyalembic",
              "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
              "bldprefix": "python-%s" % python.Version(),
              "defs": ["alembicmodule_EXPORTS"],
              "incdirs": ["python/PyAlembic"],
              "srcs": glob.glob("python/PyAlembic/*.cpp"),
              "custom": [RequireAlembic(withPython=True)],
              "install": {"%s/%s" % (python.ModulePrefix(), python.Version()): "python/examples/cask/cask.py"}
             },
             {"name": ("alembicglmodule" if sys.platform != "win32" else "alembicgl"),
              "type": "dynamicmodule",
              "ext": python.ModuleExtension(),
              "alias": "pyalembicgl",
              "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
              "bldprefix": "python-%s" % python.Version(),
              "defs": ["alembicglmodule_EXPORTS"],
              "incdirs": ["python/PyAbcOpenGL"],
              "srcs": glob.glob("python/PyAbcOpenGL/*.cpp"),
              "custom": [RequireAlembic(withPython=True, withGL=True)]}])


# Command line tools
prog_srcs = {"AbcEcho": ["examples/bin/AbcEcho/AbcEcho.cpp"],
             "AbcBoundsEcho": ["examples/bin/AbcEcho/AbcBoundsEcho.cpp"]}

for progname in ["AbcConvert",
                 "AbcEcho",
                 "AbcBoundsEcho",
                 "AbcLs",
                 "AbcStitcher",
                 "AbcTree",
                 "AbcWalk"]:
   
   prjs.append({"name": progname.lower(),
                "type": "program",
                "srcs": prog_srcs.get(progname, glob.glob("examples/bin/%s/*.cpp" % progname)),
                "custom": [RequireAlembic()]})

prjs.append({"name": "SimpleAbcViewer",
             "type": "program",
             "srcs": glob.glob("examples/bin/SimpleAbcViewer/*.cpp"),
             "custom": [RequireAlembic(withGL=True)]})

# Helper library and test
prjs.extend([{"name": "SceneHelper",
              "type": "staticlib",
              "srcs": glob.glob("lib/SceneHelper/*.cpp") +
                      ([regex_src_dir + "/regex.c"] if sys.platform == "win32" else []),
              "custom": [RequireAlembicHelper()],
              "install": {"include/SceneHelper": glob.glob("lib/SceneHelper/*.h") +
                                                ([regex_src_dir + "/regex.h"] if sys.platform == "win32" else [])}
             },
             {"name": "SceneHelperTest",
              "type": "program",
              "incdirs": ["examples/bin/SceneHelper"],
              "srcs": glob.glob("examples/bin/SceneHelper/*.cpp"),
              "custom": [RequireAlembicHelper()]}])

# DCC tools

defs = []
if nameprefix:
   defs.append("NAME_PREFIX=\"\\\"%s\\\"\"" % nameprefix)

withArnold = (excons.GetArgument("with-arnold", default=None) is not None)
withVray = (excons.GetArgument("with-vray", default=None) is not None)

if withArnold:
   prjs.append({"name": "%sAlembicArnoldProcedural" % nameprefix,
                "type": "dynamicmodule",
                "ext": arnold.PluginExt(),
                "prefix": "arnold",
                "bldprefix": "arnold-%s" % arnold.Version(),
                "defs": defs,
                "incdirs": ["arnold/Procedural"],
                "srcs": glob.glob("arnold/Procedural/*.cpp"),
                "custom": [arnold.Require, RequireAlembicHelper()]})
   
   prjs.append({"name": "abcproc",
                "type": "dynamicmodule",
                "ext": arnold.PluginExt(),
                "prefix": "arnold",
                "bldprefix": "arnold-%s" % arnold.Version(),
                "incdirs": ["arnold/abcproc"],
                "srcs": glob.glob("arnold/abcproc/*.cpp"),
                "custom": [arnold.Require, RequireAlembicHelper()]})

if withVray:
   prjs.append({"name": "%svray_AlembicLoader" % ("lib" if sys.platform != "win32" else ""),
                "type": "dynamicmodule",
                "alias": "AlembicLoader",
                "ext": vray.PluginExt(),
                "prefix": "vray",
                "bldprefix": "vray-%s" % vray.Version(),
                "incdirs": ["vray"],
                "srcs": glob.glob("vray/*.cpp"),
                "custom": [vray.Require, RequireAlembicHelper()]})

if excons.GetArgument("with-maya", default=None) is not None:
   def replace_in_file(src, dst, srcStr, dstStr):
      fdst = open(dst, "w")
      fsrc = open(src, "r")
      for line in fsrc.readlines():
         fdst.write(line.replace(srcStr, dstStr))
      fdst.close()
      fsrc.close()
   
   AbcShapeName = "%sAbcShape" % nameprefix
   AbcShapeMel = "maya/AbcShape/AE%sTemplate.mel" % AbcShapeName
   AbcMatEditPy = "maya/AbcShape/%sAbcMaterialEditor.py" % nameprefix
   AbcMatEditMel = "maya/AbcShape/%sAbcMaterialEditor.mel" % nameprefix
   AbcShapeMtoa = "arnold/abcproc/mtoa_%s.py" % AbcShapeName
   if withVray:
      AbcShapePy = "maya/AbcShape/%sabcshape4vray.py" % nameprefix.lower()
   
   impver = excons.GetArgument("maya-abcimport-version", None)
   expver = excons.GetArgument("maya-abcexport-version", None)
   shpver = excons.GetArgument("maya-abcshape-version", None)
   
   if not os.path.exists(AbcShapeMel) or os.stat(AbcShapeMel).st_mtime < os.stat("maya/AbcShape/AETemplate.mel.tpl").st_mtime:
      replace_in_file("maya/AbcShape/AETemplate.mel.tpl", AbcShapeMel, "<<NodeName>>", AbcShapeName)
   
   if not os.path.exists(AbcShapeMtoa) or os.stat(AbcShapeMtoa).st_mtime < os.stat("arnold/abcproc/mtoa.py.tpl").st_mtime:
      replace_in_file("arnold/abcproc/mtoa.py.tpl", AbcShapeMtoa, "<<NodeName>>", AbcShapeName)
   
   if not os.path.exists(AbcMatEditMel) or os.stat(AbcMatEditMel).st_mtime < os.stat("maya/AbcShape/AbcMaterialEditor.mel.tpl").st_mtime:
      replace_in_file("maya/AbcShape/AbcMaterialEditor.mel.tpl", AbcMatEditMel, "<<Prefix>>", nameprefix)
   
   if not os.path.exists(AbcMatEditPy) or os.stat(AbcMatEditPy).st_mtime < os.stat("maya/AbcShape/AbcMaterialEditor.py.tpl").st_mtime:
      replace_in_file("maya/AbcShape/AbcMaterialEditor.py.tpl", AbcMatEditPy, "<<NodeName>>", AbcShapeName)
   
   if withVray:
      if not os.path.exists(AbcShapePy) or os.stat(AbcShapePy).st_mtime < os.stat("maya/AbcShape/abcshape4vray.py.tpl").st_mtime:
         replace_in_file("maya/AbcShape/abcshape4vray.py.tpl", AbcShapePy, "<<NodeName>>", AbcShapeName)
    
   # AbcImport / AbcExport do not use SceneHelper but need regex library
   # it doesn't hurt to link SceneHelper
   
   prjs.extend([{"name": "%sAbcImport" % nameprefix,
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCIMPORT_VERSION=\"\\\"%s\\\"\"" % impver] if impver else []),
                 "incdirs": ["maya/AbcImport"],
                 "srcs": glob.glob("maya/AbcImport/*.cpp"),
                 "custom": [maya.Require, maya.Plugin, RequireAlembicHelper()],
                 "install": {"maya/scripts": glob.glob("maya/AbcImport/*.mel")}
                },
                {"name": "%sAbcExport" % nameprefix,
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCEXPORT_VERSION=\"\\\"%s\\\"\"" % expver] if expver else []),
                 "incdirs": ["maya/AbcExport"],
                 "srcs": glob.glob("maya/AbcExport/*.cpp"),
                 "custom": [maya.Require, maya.Plugin, RequireAlembicHelper()],
                 "install": {"maya/scripts": glob.glob("maya/AbcExport/*.mel")}
                },
                {"name": "%sAbcShape" % nameprefix,
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins" + ("/vray" if withVray else ""),
                 "bldprefix": "maya-%s" % maya.Version() + ("/vray-%s" % vray.Version() if withVray else ""),
                 "defs": defs + (["ABCSHAPE_VERSION=\"\\\"%s\\\"\"" % shpver] if shpver else []) +
                                (["ABCSHAPE_VRAY_SUPPORT"] if withVray else []),
                 "incdirs": ["maya/AbcShape"],
                 "srcs": glob.glob("maya/AbcShape/*.cpp"),
                 "custom": [maya.Require, maya.Plugin] + ([vray.Require] if withVray else []) + [RequireAlembicHelper()],
                 "install": {"maya/scripts": glob.glob("maya/AbcShape/*.mel"),
                             "maya/python": [AbcShapeMtoa, AbcMatEditPy] + ([AbcShapePy] if withVray else [])}}])

excons.DeclareTargets(env, prjs)

Export("RequireAlembic")
Export("RequireAlembicHelper")
