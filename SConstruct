import os
import sys
import re
import ast
import glob
import excons
from excons.tools import python
from excons.tools import arnold
from excons.tools import maya
from excons.tools import houdini
from excons.tools import boost
from excons.tools import gl
from excons.tools import glut
from excons.tools import glew


defs    = []
gldefs  = ["GLEW_STATIC", "GLEW_NO_GLU"] if sys.platform != "darwin" else []
glreqs  = [glew.Require] if sys.platform != "darwin" else [gl.Require]
pydefs  = ["BOOST_PYTHON_DYNAMIC_LIB", "BOOST_PYTHON_NO_LIB"]
incdirs = ["lib"]
libdirs = []
libs    = []

if sys.platform == "win32":
   defs.extend(["PLATFORM_WINDOWS", "PLATFORM=WINDOWS"])
elif sys.platform == "darwin":
   defs.extend(["PLATFORM_DARWIN", "PLATFORM=DARWIN"])
   libs.append("m")
else:
   defs.extend(["PLATFORM_LINUX", "PLATFORM=LINUX"])
   libs.extend(["dl", "rt", "m"])

hdf5_libs = ["hdf5_hl", "hdf5", "z", "sz"]

ilmbase_libs = ["IlmThread", "Imath", "IexMath", "Iex", "Half"]

ilmbasepy_libs = ["PyImath"]

alembic_libs = ["AlembicAbcMaterial",
                "AlembicAbcGeom",
                "AlembicAbcCollection",
                "AlembicAbcCoreFactory",
                "AlembicAbc",
                "AlembicAbcCoreHDF5",
                "AlembicAbcCoreOgawa",
                "AlembicAbcCoreAbstract",
                "AlembicOgawa",
                "AlembicUtil"]

alembicgl_libs = ["AlembicAbcOpenGL"]




def AddDirectories(inc_dir, lib_dir):
   global incdirs, libdirs
   
   if inc_dir and os.path.isdir(inc_dir) and not inc_dir in incdirs:
      incdirs.append(inc_dir)
   
   if lib_dir and os.path.isdir(lib_dir) and not lib_dir in libdirs:
      libdirs.append(lib_dir)

def SafeRemove(lst, item):
   try:
      lst.remove(item)
   except:
      pass
   return lst


deps_inc, deps_lib = excons.GetDirs("deps", noexc=True, silent=True)
AddDirectories(deps_inc, deps_lib)

zlib_inc, zlib_lib = excons.GetDirsWithDefault("zlib", incdirdef=deps_inc, libdirdef=deps_lib)
AddDirectories(zlib_inc, zlib_lib)

boost_inc, boost_lib = excons.GetDirsWithDefault("boost", incdirdef=deps_inc, libdirdef=deps_lib)
AddDirectories(boost_inc, boost_lib)

hdf5_inc, hdf5_lib = excons.GetDirsWithDefault("hdf5", incdirdef=deps_inc, libdirdef=deps_lib)
AddDirectories(hdf5_inc, hdf5_lib)

ilmbase_inc, ilmbase_lib = excons.GetDirsWithDefault("ilmbase", incdirdef=deps_inc, libdirdef=deps_lib)
ilmbase_libsuffix = excons.GetArgument("ilmbase-libsuffix", "")
if ilmbase_inc and not ilmbase_inc.endswith("OpenEXR"):
  ilmbase_inc += "/OpenEXR"
if ilmbase_libsuffix:
  ilmbase_libs = map(lambda x: x+ilmbase_libsuffix, ilmbase_libs)
AddDirectories(ilmbase_inc, ilmbase_lib)

boostpy_inc, boostpy_lib = excons.GetDirsWithDefault("boost-python", incdirdef=deps_inc, libdirdef=deps_lib)
boostpy_libname = excons.GetArgument("boost-python-libname", "boost_python")
AddDirectories(boostpy_inc, boostpy_lib)

ilmbasepy_inc, ilmbasepy_lib = excons.GetDirsWithDefault("ilmbase-python", incdirdef=deps_inc, libdirdef=deps_lib)
if ilmbasepy_inc and not ilmbasepy_inc.endswith("OpenEXR"):
  ilmbasepy_inc += "/OpenEXR"
AddDirectories(ilmbasepy_inc, ilmbasepy_lib)

nameprefix = excons.GetArgument("name-prefix", default="")

# arnold, maya, python, glut, glew => excons tools
# GLEW setup (default): glew-static=1, glew-no-glu=1, glew-mx=0


env = excons.MakeBaseEnv()

prjs = [
   {"name": "AlembicUtil",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/Util/*.cpp"),
    "install": {"include/Alembic/Util": glob.glob("lib/Alembic/Util/*.h")}
   },
   {"name": "AlembicAbcCoreAbstract",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcCoreAbstract/*.cpp"),
    "install": {"include/Alembic/AbcCoreAbstract": glob.glob("lib/Alembic/AbcCoreAbstract/*.h")}
   },
   {"name": "AlembicAbcCoreOgawa",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcCoreOgawa/*.cpp"),
    "install": {"include/Alembic/AbcCoreOgawa": ["lib/Alembic/AbcCoreOgawa/All.h",
                                                 "lib/Alembic/AbcCoreOgawa/ReadWrite.h"]}
   },
   {"name": "AlembicAbcCoreHDF5",
   "defs": defs,
    "type": "staticlib",
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcCoreHDF5/*.cpp"),
    "install": {"include/Alembic/AbcCoreHDF5": ["lib/Alembic/AbcCoreHDF5/All.h",
                                                "lib/Alembic/AbcCoreHDF5/ReadWrite.h"]}
   },
   {"name": "AlembicAbc",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/Abc/*.cpp"),
    "install": {"include/Alembic/Abc": glob.glob("lib/Alembic/Abc/*.h")}
   },
   {"name": "AlembicAbcCoreFactory",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcCoreFactory/*.cpp"),
    "install": {"include/Alembic/AbcCoreFactory": glob.glob("lib/Alembic/AbcCoreFactory/*.h")}
   },
   {"name": "AlembicAbcGeom",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcGeom/*.cpp"),
    "install": {"include/Alembic/AbcGeom": glob.glob("lib/Alembic/AbcGeom/*.h")}
   },
   {"name": "AlembicAbcCollection",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcCollection/*.cpp"),
    "install": {"include/Alembic/AbcCollection": glob.glob("lib/Alembic/AbcCollection/*.h")}
   },
   {"name": "AlembicAbcMaterial",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/AbcMaterial/*.cpp"),
    "install": {"include/Alembic/AbcMaterial": SafeRemove(glob.glob("lib/Alembic/AbcMaterial/*.h"), ("lib/Alembic/AbcMaterial/InternalUtil.h"))}
   },
   {"name": "AlembicOgawa",
    "type": "staticlib",
    "defs": defs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/Alembic/Ogawa/*.cpp"),
    "install": {"include/Alembic/Ogawa": glob.glob("lib/Alembic/Ogawa/*.h")}
   },
   {"name": "alembicmodule",
    "type": "dynamicmodule",
    "ext": python.ModuleExtension(),
    "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
    "defs": defs + pydefs + ["alembicmodule_EXPORTS"],
    "incdirs": incdirs + ["python/PyAlembic"],
    "libdirs": libdirs,
    "libs": alembic_libs + [boostpy_libname] + ilmbasepy_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("python/PyAlembic/*.cpp"),
    "custom": [python.Require],
    "install": {"%s/%s" % (python.ModulePrefix(), python.Version()): "python/examples/cask/cask.py"}
   },
   {"name": "abcconvert",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcConvert/*.cpp")
   },
   {"name": "abcecho",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcEcho/AbcEcho.cpp")
   },
   {"name": "abcboundsecho",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcEcho/AbcBoundsEcho.cpp")
   },
   {"name": "abcls",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcLs/*.cpp")
   },
   {"name": "abcstitcher",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcStitcher/*.cpp")
   },
   {"name": "abctree",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcTree/*.cpp")
   },
   {"name": "abcwalk",
    "type": "program",
    "incdirs": incdirs,
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/AbcWalk/*.cpp")
   },
   # OpenGL targets
   {"name": "AlembicAbcOpenGL",
    "type": "staticlib",
    "defs": defs + gldefs,
    "incdirs": incdirs,
    "srcs": glob.glob("lib/AbcOpenGL/*.cpp"),
    "install": {"include/AbcOpenGL": glob.glob("lib/AbcOpenGL/*.h")}
   },
   {"name": "alembicglmodule",
    "type": "dynamicmodule",
    "ext": python.ModuleExtension(),
    "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
    "defs": defs + pydefs + gldefs + ["alembicglmodule_EXPORTS"],
    "incdirs": incdirs + ["python/PyAbcOpenGL"],
    "libdirs": libdirs,
    "srcs": glob.glob("python/PyAbcOpenGL/*.cpp"),
    "libs": alembicgl_libs + alembic_libs + [boostpy_libname] + ilmbasepy_libs + ilmbase_libs + hdf5_libs + libs,
    "custom": glreqs + [python.SoftRequire]
   },
   {"name": "SimpleAbcViewer",
    "type": "program",
    "defs": defs + gldefs,
    "incdirs": incdirs + ["examples/bin/SimpleAbcViewer"],
    "libdirs": libdirs,
    "libs": alembicgl_libs + alembic_libs + ilmbase_libs + hdf5_libs + libs,
    "srcs": glob.glob("examples/bin/SimpleAbcViewer/*.cpp"),
    "custom": glreqs + [glut.Require]
   },
   {"name": "SceneHelper",
    "type": "program",
    "defs": defs,
    "incdirs": incdirs + ["examples/bin/SceneHelper", "lib/SceneHelper"],
    "libdirs": libdirs,
    "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs + (["pthread"] if sys.platform != "win32" else []),
    "srcs": glob.glob("examples/bin/SceneHelper/*.cpp") + glob.glob("lib/SceneHelper/*.cpp")
   }
]

plugin_defs = defs[:]
if nameprefix:
   plugin_defs.append("NAME_PREFIX=\"\\\"%s\\\"\"" % nameprefix)

if excons.GetArgument("with-arnold", default=None) is not None:
   prjs.append({"name": "%sAlembicArnoldProcedural" % nameprefix,
                "type": "dynamicmodule",
                "ext": arnold.PluginExt(),
                "prefix": "arnold",
                "defs": plugin_defs,
                "incdirs": incdirs + ["arnold/Procedural"],
                "libdirs": libdirs,
                "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
                "srcs": glob.glob("arnold/Procedural/*.cpp"),
                "custom": [arnold.Require]})
   prjs.append({"name": "abcproc",
                "type": "dynamicmodule",
                "ext": arnold.PluginExt(),
                "prefix": "arnold",
                "defs": defs,
                "incdirs": incdirs + ["arnold/abcproc"] + ["lib/SceneHelper"],
                "libdirs": libdirs,
                "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
                "srcs": glob.glob("arnold/abcproc/*.cpp") + glob.glob("lib/SceneHelper/*.cpp"),
                "custom": [arnold.Require]})

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
   AbcShapeMtoa = "arnold/abcproc/mtoa_%s.py" % AbcShapeName
   
   if not os.path.exists(AbcShapeMel):
      replace_in_file("maya/AbcShape/AETemplate.mel.tpl", AbcShapeMel, "<<NodeName>>", AbcShapeName)
   if not os.path.exists(AbcShapeMtoa):
      replace_in_file("arnold/abcproc/mtoa.py.tpl", AbcShapeMtoa, "<<NodeName>>", AbcShapeName)
   
   prjs.extend([{"name": "%sAbcImport" % nameprefix,
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "defs": plugin_defs,
                 "incdirs": incdirs + ["maya/AbcImport"],
                 "libdirs": libdirs,
                 "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
                 "srcs": glob.glob("maya/AbcImport/*.cpp"),
                 "install": {"maya/scripts": glob.glob("maya/AbcImport/*.mel")},
                 "custom": [maya.Require, maya.Plugin]
                },
                {"name": "%sAbcExport" % nameprefix,
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "defs": plugin_defs,
                 "incdirs": incdirs + ["maya/AbcExport"],
                 "libdirs": libdirs,
                 "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
                 "srcs": glob.glob("maya/AbcExport/*.cpp"),
                 "install": {"maya/scripts": glob.glob("maya/AbcExport/*.mel")},
                 "custom": [maya.Require, maya.Plugin]
                },
                {"name": "%sAbcShape" % nameprefix,
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins",
                 "defs": plugin_defs,
                 "incdirs": incdirs + ["maya/AbcShape", "lib/SceneHelper"],
                 "libdirs": libdirs,
                 "libs": alembic_libs + ilmbase_libs + hdf5_libs + libs,
                 "srcs": glob.glob("maya/AbcShape/*.cpp") + glob.glob("lib/SceneHelper/*.cpp"),
                 "install": {"maya/scripts": glob.glob("maya/AbcShape/*.mel"),
                             "maya/python": [AbcShapeMtoa]},
                 "custom": [maya.Require, maya.Plugin]}])

excons.DeclareTargets(env, prjs)
