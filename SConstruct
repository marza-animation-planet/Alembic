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


build_arnold_plugins  = False
build_maya_plugins    = False
build_houdini_plugins = False
boost_python_libname  = "boost_python"


defs    = []
gldefs  = ["GLEW_STATIC", "GLEW_NO_GLU"] if sys.platform != "darwin" else []
glreqs  = [glew.Require] if sys.platform != "darwin" else [gl.Require]
pydefs  = ["BOOST_PYTHON_DYNAMIC_LIB", "BOOST_PYTHON_NO_LIB"]
incdirs = ["lib"]
libdirs = []
libs    = []

#hdf5_defs = ["_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_BDS_SOURCE"]
hdf5_libs = ["hdf5_hl", "hdf5", "z", "sz"]

ilmbase_libs_base = ["IlmThread", "Imath", "IexMath", "Iex", "Half"]
ilmbase_libs = ["IlmThread", "Imath", "IexMath", "Iex", "Half"]

pyilmbase_libs = ["PyImath"]

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

if sys.platform == "win32":
   defs.extend(["PLATFORM_WINDOWS", "PLATFORM=WINDOWS"])
elif sys.platform == "darwin":
   defs.extend(["PLATFORM_DARWIN", "PLATFORM=DARWIN"])
   libs.append("m")
else:
   defs.extend(["PLATFORM_LINUX", "PLATFORM=LINUX"])
   libs.extend(["dl", "rt", "m"])


def GetArgument(name, default=None, func=None):
   val = ARGUMENTS.get(name, default)
   if func:
      return func(val)
   else:
      return val

def SetArgument(name, value):
   ARGUMENTS[name] = value

def GetDirsWithDefault(name, incdirname="include", libdirname="lib", libdirarch=None, incdir_default=None, libdir_default=None):
   inc_dir, lib_dir = excons.GetDirs(name, incdirname=incdirname, libdirname=libdirname, libdirarch=libdirarch, noexc=True, silent=True)
   
   if inc_dir is None:
      inc_dir = incdir_default
   
   if lib_dir is None:
      lib_dir = libdir_default
   
   if inc_dir is None or lib_dir is None:
      print("%s directories not set (use with-deps=, with-deps-inc=, with-deps-lib= or with-ilmbase=, with-ilmbase-inc=, with-ilmbase-lib=" % name)
      sys.exit(-1)
   
   return (inc_dir, lib_dir)

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


def ReadSettings(path, silent=False):
   if not silent:
      print("===--- Read build settings from %s (use no-cache=1 to ignore) ---===" % os.path.abspath(path))
   
   platformexp = re.compile(r"^\[([^]]+)\]$")
   
   curplat = None
   platsettings = {}
   allsettings = {}
   
   c = open(path, "r")
   for l in c.readlines():
      l = l.strip()
      m = platformexp.match(l)
      if m:
         if curplat is not None:
            allsettings[curplat] = platsettings
         curplat = m.group(1)
         platsettings = {}
      else:
         if l.startswith("#"):
            continue
         spl = l.split("=")
         if len(spl) == 2:
            key = spl[0].strip()
            val = spl[1].strip()
            try:
               platsettings[key] = ast.literal_eval(val)
            except Exception, e:
               print("Invalid value for setting \"%s\" in platform \"%s\": \"%s\" (%s)" % (key, curplat, val, e))
   
   if curplat is not None:
      allsettings[curplat] = platsettings
   
   return allsettings

def ApplySettings(settings):
   global boost_python_libname, build_arnold_plugins, build_maya_plugins, build_houdini_plugins, ilmbase_libs_base, ilmbase_libs
   
   build_arnold_plugins = False
   build_maya_plugins = False
   build_houdini_plugins = False
   boost_python_libname = "boost_python"
   
   platsettings = settings.get(sys.platform, {})
   
   print("=== Build settings ===")
   import pprint
   pprint.pprint(platsettings)
   
   for key, val in platsettings.iteritems():
      if key == "boost-python":
         inc, lib, boost_python_libname = val
         AddDirectories(inc, lib)
      
      elif key == "ilmbase":
         inc, lib, suffix = val
         AddDirectories(inc, lib)
         ilmbase_libs = map(lambda x: x+suffix, ilmbase_libs_base)
         
      elif key == "python":
         SetArgument("with-python", str(val))
      
      elif key == "maya":
         ver, inc, lib = val
         build_maya_plugins = True
         if ver:
            SetArgument("maya-ver", str(ver))
         else:
            SetArgument("with-maya-inc", inc)
            SetArgument("with-maya-lib", lib)
      
      elif key == "arnold":
         build_arnold_plugins = True
         inc, lib = val
         SetArgument("with-arnold-inc", inc)
         SetArgument("with-arnold-lib", lib)
      
      elif key == "glut":
         inc, lib = val
         SetArgument("with-glut-inc", inc)
         SetArgument("with-glut-lib", lib)
      
      elif key == "glew":
         inc, lib = val
         SetArgument("with-glew-inc", inc)
         SetArgument("with-glew-lib", lib)
         SetArgument("glew-static", "1")
         SetArgument("glew-mx", "0")
         SetArgument("glew-no-glu", "1")
         AddDirectories(inc, None)
      
      else:
         inc, lib = val
         AddDirectories(inc, lib)

def WriteSettings(path, settings):
   print("===--- Write build settings to %s ---===" % os.path.abspath(path))
   
   c = open(excons_cache, "w")
   for plat, platsettings in settings.iteritems():
      c.write("[%s]\n" % plat)
      for key, val in platsettings.iteritems():
         c.write("%s=%s\n" % (key, str(val)))
      c.write("\n")
   c.write("\n")
   c.close()

def UpdateSettings(path, replace=True):
   if os.path.isfile(path):
      allsettings = ReadSettings(path, silent=True)
   else:
      allsettings = {}
   
   curplatsettings = ({} if replace else allsettings.get(sys.platform, {}))
   platsettings = {}
   
   deps_inc, deps_lib = excons.GetDirs("deps", noexc=True, silent=True)
   
   def_inc, def_lib = (deps_inc, deps_lib) if replace else curplatsettings.get("zlib", (None, None))
   zlib_inc, zlib_lib = GetDirsWithDefault("zlib", incdir_default=def_inc, libdir_default=def_lib)
   if zlib_inc or zlib_lib:
      platsettings["zlib"] = (zlib_inc, zlib_lib)
   
   def_inc, def_lib = (deps_inc, deps_lib) if replace else curplatsettings.get("boost", (None, None))
   boost_inc, boost_lib = GetDirsWithDefault("boost", incdir_default=def_inc, libdir_default=def_lib)
   if boost_inc or boost_lib:
      platsettings["boost"] = (boost_inc, boost_lib)
   
   def_inc, def_lib, def_libname = (deps_inc, deps_lib, "boost_python") if replace else curplatsettings.get("boost-python", (None, None, None))
   boost_pyinc, boost_pylib = GetDirsWithDefault("boost-python", incdir_default=def_inc, libdir_default=def_lib)
   boost_pylibname = GetArgument("with-boost-python-libname", default=def_libname)
   if boost_pyinc or boost_pylib or boost_pylibname:
      platsettings["boost-python"] = (boost_pyinc, boost_pylib, boost_pylibname)
   
   def_inc, def_lib, def_suffix = (deps_inc, deps_lib, "") if replace else curplatsettings.get("ilmbase", (None, None, ""))
   ilmbase_inc, ilmbase_lib = GetDirsWithDefault("ilmbase", incdir_default=def_inc, libdir_default=def_lib)
   ilmbase_libname_suffix = GetArgument("with-ilmbase-libname-suffix", default=def_suffix)
   if ilmbase_inc or ilmbase_lib or ilmbase_libname_suffix:
      if not ilmbase_inc.endswith("OpenEXR"):
         ilmbase_inc += "/OpenEXR"
      platsettings["ilmbase"] = (ilmbase_inc, ilmbase_lib, ilmbase_libname_suffix)
   
   def_inc, def_lib = (deps_inc, deps_lib) if replace else curplatsettings.get("ilmbase-python", (None, None))
   ilmbase_pyinc, ilmbase_pylib = GetDirsWithDefault("ilmbase-python", incdir_default=def_inc, libdir_default=def_lib)
   if ilmbase_pyinc or ilmbase_pylib:
      if not ilmbase_pyinc.endswith("OpenEXR"):
         ilmbase_pyinc += "/OpenEXR"
      platsettings["ilmbase-python"] = (ilmbase_pyinc, ilmbase_pylib)
   
   def_inc, def_lib = (deps_inc, deps_lib) if replace else curplatsettings.get("hdf5", (None, None))
   hdf5_inc, hdf5_lib = GetDirsWithDefault("hdf5", incdir_default=def_inc, libdir_default=def_lib)
   if hdf5_inc or hdf5_lib:
      platsettings["hdf5"] = (hdf5_inc, hdf5_lib)
   
   arnold_inc, arnold_lib = excons.GetDirs("arnold", libdirname=("bin" if sys.platform != "win32" else "lib"), silent=True)
   if arnold_inc and arnold_lib:
      platsettings["arnold"] = (arnold_inc, arnold_lib)
   
   maya_inc, maya_lib = excons.GetDirs("maya", noexc=True, silent=True)
   maya_ver = GetArgument("maya-ver", None)
   if (maya_inc and maya_lib) or maya_ver:
      if not maya_ver:
         maya_ver = ""
      if not maya_inc:
         maya_inc = ""
      if not maya_lib:
         maya_lib = ""
      platsettings["maya"] = (maya_ver, maya_inc, maya_lib)
   
   pyspec = GetArgument("with-python", default=None)
   if pyspec is not None:
      platsettings["python"] = pyspec
   
   glut_inc, glut_lib = excons.GetDirs("glut", noexc=True, silent=True)
   if glut_inc or glut_lib:
      platsettings["glut"] = ("" if not glut_inc else glut_inc, "" if not glut_lib else glut_lib)
   
   if sys.platform != "darwin":
      def_inc, def_lib = (deps_inc, deps_lib) if replace else curplatsettings.get("glew", (None, None))
      glew_inc, glew_lib = GetDirsWithDefault("glew", incdir_default=def_inc, libdir_default=def_lib)
      if glew_inc or glew_lib:
         platsettings["glew"] = ("" if not glew_inc else glew_inc, "" if not glew_lib else glew_lib)
   
   settingschanged = False
   
   if replace:
      allsettings[sys.platform] = platsettings
      settingschanged = True
   else:
      for k, v in platsettings.iteritems():
         if not k in curplatsettings or v != curplatsettings[k]:
            curplatsettings[k] = v
            settingschanged = True
      if settingschanged:
         allsettings[sys.platform] = curplatsettings
   
   ApplySettings(allsettings)
   
   if settingschanged:
      WriteSettings(path, allsettings)


excons_cache = os.path.abspath("excons.cache")
nocache = GetArgument("no-cache", False, lambda x: int(x) != 0)
nameprefix = GetArgument("name-prefix", "")

UpdateSettings(excons_cache, replace=nocache)


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
    "libs": alembic_libs + [boost_python_libname] + pyilmbase_libs + ilmbase_libs + hdf5_libs + libs,
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
    "libs": alembicgl_libs + alembic_libs + [boost_python_libname] + pyilmbase_libs + ilmbase_libs + hdf5_libs + libs,
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

if build_arnold_plugins:
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

if build_maya_plugins:
   def replace_in_mel(src, dst, srcStr, dstStr):
      fdst = open(dst, "w")
      fsrc = open(src, "r")
      for line in fsrc.readlines():
         fdst.write(line.replace(srcStr, dstStr))
      fdst.close()
      fsrc.close()
   
   AbcShapeName = "%sAbcShape" % nameprefix
   AbcShapeMel = "maya/AbcShape/AE%sTemplate.mel" % AbcShapeName
   
   if not os.path.exists(AbcShapeMel):
      replace_in_mel("maya/AbcShape/AETemplate.mel.tpl", AbcShapeMel, "<<NodeName>>", AbcShapeName)
   
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
                 "install": {"maya/scripts": glob.glob("maya/AbcShape/*.mel")},
                 "custom": [maya.Require, maya.Plugin]}])

if build_houdini_plugins:
   # TODO
   pass

excons.DeclareTargets(env, prjs)
