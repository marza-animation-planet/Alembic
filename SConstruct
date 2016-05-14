import os
import re
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

cpp11 = excons.GetArgument("c++11", 0, int)
msc10warn = True
msc12warn = True

regex_src_dir = "lib/SceneHelper/regex-2.7/src"
regex_inc = [regex_src_dir + "/regex.h"] if sys.platform == "win32" else []
regex_src = [regex_src_dir + "/regex.c"] if sys.platform == "win32" else []


def RequireAlembic(withPython=False, withGL=False):
   
   def _RequireFunc(env):
      global deps_inc, deps_lib, msc10warn, msc12warn
      
      if sys.platform == "darwin":
         import platform
         vers = map(int, platform.mac_ver()[0].split("."))
         # Force c++11 for OSX >= 10.9
         if cpp11 or (vers[0] > 10 or (vers[0] == 10 and vers[1] >= 9)):
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
         
         mscmaj = int(excons.mscver.split(".")[0])
         
         # Work around the 2GB file limit with Visual Studio 10.0
         #   https://connect.microsoft.com/VisualStudio/feedback/details/627639/std-fstream-use-32-bit-int-as-pos-type-even-on-x64-platform
         #   https://groups.google.com/forum/#!topic/alembic-discussion/5ElRJkXhi9M
         if mscmaj == 10:
            env.Prepend(CPPPATH=["lib/vc10fix"])
         
         if mscmaj < 10 and msc10warn:
            print("You should use at least Visual C 10.0 if you expect reading alembic file above 2GB without crash (use mscver= flag)")
            msc10warn = False
         
         if cpp11 and mscmaj < 12 and msc12warn:
            print("You should use at least Visual C 12.0 for C++11 support (use mscver= flag)")
            msc12warn = False
      
      else:
         env.Append(CPPFLAGS=" ".join([" -Wno-unused-local-typedefs",
                                       "-Wno-unused-parameter",
                                       "-Wno-unused-but-set-variable",
                                       "-Wno-unused-variable",
                                       "-Wno-ignored-qualifiers"]))
         if cpp11:
            env.Append(CPPFLAGS=" -std=c++11")
      
      defs = []
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


def RequireRegex(env):
   global regex_src_dir
   
   if sys.platform == "win32":
      env.Append(CPPDEFINES=["REGEX_STATIC"])
      env.Append(CPPPATH=[regex_src_dir])


def RequireAlembicHelper(withPython=False, withGL=False):
   def _RequireFunc(env):
      
      RequireRegex(env)
      
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
                "alias": "alembiclib",
                "srcs": glob.glob("lib/Alembic/%s/*.cpp" % libname),
                "custom": [RequireAlembic()],
                "install": {"include/Alembic/%s" % libname: glob.glob("lib/Alembic/%s/*.h" % libname)}})

prjs.append({"name": "AlembicAbcOpenGL",
             "type": "staticlib",
             "alias": "alembicgl",
             "srcs": glob.glob("lib/AbcOpenGL/*.cpp"),
             "custom": [RequireAlembic(withGL=True)],
             "install": {"include/AbcOpenGL": glob.glob("lib/AbcOpenGL/*.h")}})


# Python modules
prjs.extend([{"name": ("alembicmodule" if sys.platform != "win32" else "alembic"),
              "type": "dynamicmodule",
              "ext": python.ModuleExtension(),
              "alias": "pyalembic",
              "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
              "rpaths": ["../.."],
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
              "rpaths": ["../.."],
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
                "alias": "alembictools",
                "srcs": prog_srcs.get(progname, glob.glob("examples/bin/%s/*.cpp" % progname)),
                "custom": [RequireAlembic()]})

prjs.append({"name": "SimpleAbcViewer",
             "type": "program",
             "alias": "alembictools",
             "srcs": glob.glob("examples/bin/SimpleAbcViewer/*.cpp"),
             "custom": [RequireAlembic(withGL=True)]})

# Helper library and test
prjs.extend([{"name": "SceneHelper",
              "type": "staticlib",
              "srcs": glob.glob("lib/SceneHelper/*.cpp") + regex_src,
              "custom": [RequireAlembicHelper()],
              "install": {"include/SceneHelper": glob.glob("lib/SceneHelper/*.h") + regex_inc}
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

arnoldInc, arnoldLib = excons.GetDirs("arnold", libdirname=("lib" if sys.platform == "win32" else "bin"), silent=True)
withArnold = (arnoldInc and arnoldLib)
withMaya = (excons.GetArgument("with-maya", default=None) is not None)
withVray = (excons.GetArgument("with-vray", default=None) is not None)

if withArnold:
   prjs.append({"name": "%sAlembicArnoldProcedural" % nameprefix,
                "type": "dynamicmodule",
                "ext": arnold.PluginExt(),
                "prefix": "arnold",
                "rpaths": ["../lib"],
                "bldprefix": "arnold-%s" % arnold.Version(),
                "defs": defs,
                "incdirs": ["arnold/Procedural"],
                "srcs": glob.glob("arnold/Procedural/*.cpp") + regex_src,
                "custom": [arnold.Require, RequireRegex, RequireAlembic()]})
   
   prjs.append({"name": "abcproc",
                "type": "dynamicmodule",
                "ext": arnold.PluginExt(),
                "prefix": "arnold",
                "rpaths": ["../lib"],
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
                "rpaths": ["../lib"],
                "bldprefix": "vray-%s" % vray.Version(),
                "incdirs": ["vray"],
                "srcs": glob.glob("vray/*.cpp"),
                "custom": [vray.Require, RequireAlembicHelper()]})

if withMaya:
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
                 "alias": "maya",
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "rpaths": ["../../lib"],
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCIMPORT_VERSION=\"\\\"%s\\\"\"" % impver] if impver else []),
                 "incdirs": ["maya/AbcImport"],
                 "srcs": glob.glob("maya/AbcImport/*.cpp") + regex_src,
                 "custom": [RequireRegex, RequireAlembic(), maya.Require, maya.Plugin],
                 "install": {"maya/scripts": glob.glob("maya/AbcImport/*.mel")}
                },
                {"name": "%sAbcExport" % nameprefix,
                 "alias": "maya",
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "rpaths": ["../../lib"],
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCEXPORT_VERSION=\"\\\"%s\\\"\"" % expver] if expver else []),
                 "incdirs": ["maya/AbcExport"],
                 "srcs": glob.glob("maya/AbcExport/*.cpp"),
                 "custom": [RequireAlembic(), maya.Require, maya.Plugin],
                 "install": {"maya/scripts": glob.glob("maya/AbcExport/*.mel")}
                },
                {"name": "%sAbcShape" % nameprefix,
                 "alias": "maya",
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins" + ("/vray" if withVray else ""),
                 "rpaths": ["../../lib" if not withVray else "../../../lib"],
                 "bldprefix": "maya-%s" % maya.Version() + ("/vray-%s" % vray.Version() if withVray else ""),
                 "defs": defs + (["ABCSHAPE_VERSION=\"\\\"%s\\\"\"" % shpver] if shpver else []) +
                                (["ABCSHAPE_VRAY_SUPPORT"] if withVray else []),
                 "incdirs": ["maya/AbcShape"],
                 "srcs": glob.glob("maya/AbcShape/*.cpp"),
                 "custom": ([vray.Require] if withVray else []) + [RequireAlembicHelper(), maya.Require, maya.Plugin],
                 "install": {"maya/scripts": glob.glob("maya/AbcShape/*.mel"),
                             "maya/python": [AbcShapeMtoa, AbcMatEditPy] + ([AbcShapePy] if withVray else [])}}])
   
   if withArnold:
      mtoa_inc, mtoa_lib = excons.GetDirs("mtoa", libdirname=("lib" if sys.platform == "win32" else "bin"))
      mtoa_defs = defs[:]
      mtoa_ext = ""
      mtoa_ver = "unknown"
      
      if mtoa_inc and mtoa_lib:
         if sys.platform == "darwin":
            mtoa_defs.append("_DARWIN")
            mtoa_ext = ".dylib"
         
         elif sys.platform == "win32":
            mtoa_defs.append("_WIN32")
            mtoa_ext = ".dll"
         
         else:
            mtoa_defs.append("_LINUX")
            mtoa_ext = ".so"
         
         m = re.search(r"\d+\.\d+\.\d+(\.\d+)?", mtoa_inc)
         if m is None:
            m = re.search(r"\d+\.\d+\.\d+(\.\d+)?", mtoa_lib)
         if m is not None:
            mtoa_ver = m.group(0)
         
         AbcShapeMtoa = "maya/AbcShape/mtoa/%sMtoa.py" % AbcShapeName
         AbcShapeHelper = "maya/AbcShape/mtoa/%sHelper.py" % AbcShapeName
         
         if not os.path.exists(AbcShapeMtoa) or os.stat(AbcShapeMtoa).st_mtime < os.stat("maya/AbcShape/mtoa/AbcShapeMtoa.py.tpl").st_mtime:
            replace_in_file("maya/AbcShape/mtoa/AbcShapeMtoa.py.tpl", AbcShapeMtoa, "<<NodeName>>", AbcShapeName)
         
         if not os.path.exists(AbcShapeHelper) or os.stat(AbcShapeHelper).st_mtime < os.stat("maya/AbcShape/mtoa/AbcShapeHelper.py.tpl").st_mtime:
            replace_in_file("maya/AbcShape/mtoa/AbcShapeHelper.py.tpl", AbcShapeHelper, "<<NodeName>>", AbcShapeName)
         
         prjs.append({"name": "%sAbcShapeMtoa" % nameprefix,
                      "type": "dynamicmodule",
                      "prefix": "maya/plug-ins/mtoa",
                      "rpaths": ["../../../lib"],
                      "bldprefix": "maya-%s/mtoa-%s" % (maya.Version(), mtoa_ver),
                      "ext": mtoa_ext,
                      "defs": mtoa_defs,
                      "srcs": glob.glob("maya/AbcShape/mtoa/*.cpp"),
                      "incdirs": [mtoa_inc],
                      "libdirs": [mtoa_lib],
                      "libs": ["mtoa_api"],
                      "install": {"maya/plug-ins/mtoa": [AbcShapeMtoa],
                                  "maya/python": [AbcShapeHelper]},
                      "custom": [RequireAlembicHelper(), arnold.Require, maya.Require]})


excons.DeclareTargets(env, prjs)

Export("RequireAlembic")
Export("RequireAlembicHelper")
