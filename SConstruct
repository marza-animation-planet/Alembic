import os
import sys
import re
import glob
import excons
from excons.tools import python
from excons.tools import arnold as arnoldbuilder
from excons.tools import maya as mayabuilder
from excons.tools import houdini as houdinibuilder
from excons.tools import boost
from excons.tools import gl
from excons.tools import glut

nogl    = False
arnold  = False
maya    = False
houdini = False
verbose = False
defs    = []
pydefs  = ["BOOST_PYTHON_DYNAMIC_LIB", "BOOST_PYTHON_NO_LIB"]
incdirs = ["lib"]
libdirs = []
libs    = []

#hdf5_defs = ["_LARGEFILE_SOURCE", "_LARGEFILE64_SOURCE", "_BDS_SOURCE"]
hdf5_libs = ["hdf5_hl", "hdf5", "z", "sz"]

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

boost_python_libname = "boost_python"

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
   inc_dir, lib_dir = excons.GetDirs(name, incdirname=incdirname, libdirname=libdirname, libdirarch=libdirarch, noexc=True)
   
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
   
   if os.path.isdir(inc_dir) and not inc_dir in incdirs:
      incdirs.append(inc_dir)
   
   if os.path.isdir(lib_dir) and not lib_dir in libdirs:
      libdirs.append(lib_dir)

def SafeRemove(lst, item):
   try:
      lst.remove(item)
   except:
      pass
   return lst

def UpdateSettings(path):
   global boost_python_libname, nogl
   
   try:
      deps_inc, deps_lib = excons.GetDirs("deps", noexc=False)
   except:
      deps_inc, deps_lib = None, None
   
   zlib_inc, zlib_lib = GetDirsWithDefault("zlib", incdir_default=deps_inc, libdir_default=deps_lib)
   AddDirectories(zlib_inc, zlib_lib)
   
   boost_inc, boost_lib = GetDirsWithDefault("boost", incdir_default=deps_inc, libdir_default=deps_lib)
   AddDirectories(boost_inc, boost_lib)
   
   boost_pyinc, boost_pylib = GetDirsWithDefault("boost-python", incdir_default=deps_inc, libdir_default=deps_lib)
   AddDirectories(boost_pyinc, boost_pylib)
   
   boost_python_libname = GetArgument("with-boost-python-libname", default=boost_python_libname)
   
   ilmbase_inc, ilmbase_lib = GetDirsWithDefault("ilmbase", incdir_default=deps_inc, libdir_default=deps_lib)
   AddDirectories(ilmbase_inc+"/OpenEXR", ilmbase_lib)
   
   pyilmbase_inc, pyilmbase_lib = GetDirsWithDefault("pyilmbase", incdir_default=deps_inc, libdir_default=deps_lib)
   AddDirectories(pyilmbase_inc+"/OpenEXR", pyilmbase_lib)
   
   hdf5_inc, hdf5_lib = GetDirsWithDefault("hdf5", incdir_default=deps_inc, libdir_default=deps_lib)
   AddDirectories(hdf5_inc, hdf5_lib)
   
   pyspec = GetArgument("with-python", default=None)
   
   nogl = GetArgument("no-gl", default=False, func=lambda x: int(x) != 0)
   glut_inc, glut_lib = None, None
   if not nogl:
      glut_inc, glut_lib = GetDirsWithDefault("glut", incdir_default=deps_inc, libdir_default=deps_lib)
      AddDirectories(glut_inc, glut_lib)
   
   print("===--- Write build settings to %s ---===" % os.path.abspath(path))
   c = open(excons_cache, "w")
   c.write("%s\n" % sys.platform)
   c.write("nogl=%d\n" % (1 if nogl else 0))
   if pyspec is not None:
      c.write("python=%s\n" % pyspec)
   c.write("zlib=%s,%s\n" % (zlib_inc, zlib_lib))
   c.write("boost=%s,%s\n" % (boost_inc, boost_lib))
   c.write("boost-python=%s,%s,%s\n" % (boost_pyinc, boost_pylib, boost_python_libname))
   c.write("ilmbase=%s/OpenEXR,%s\n" % (ilmbase_inc, ilmbase_lib))
   c.write("pyilmbase=%s/OpenEXR,%s\n" % (pyilmbase_inc, pyilmbase_lib))
   c.write("hdf5=%s,%s\n" % (hdf5_inc, hdf5_lib))
   if not nogl:
      c.write("glut=%s,%s\n" % (glut_inc, glut_lib))
   c.write("\n")
   c.close()

def ReadSettings(path):
   print("===--- Read build settings from %s (use no-cache=1 to ignore) ---===" % os.path.abspath(path))
   global boost_python_libname, nogl
   
   c = open(path, "r")
   for l in c.readlines():
      l = l.strip()
      try:
         key, val = l.split("=")
         if key == "boost-python":
            inc, lib, boost_python_libname = val.split(",")
            AddDirectories(inc, lib)
         elif key == "python":
            SetArgument("with-python", val)
         elif key == "nogl":
            nogl = (int(val) != 0)
         else:
            inc, lib = val.split(",")
            AddDirectories(inc, lib)
            if key in ["glut", "boost"]:
               if inc:
                  SetArgument("with-%s-inc" % key, inc)
               if lib:
                  SetArgument("with-%s-lib" % key, lib)
      except Exception, e:
         if verbose and len(l) > 0:
            print("Ignore line: \"%s\" (%s)" % (l, e))



excons_cache = os.path.abspath("excons.cache")
nocache = GetArgument("no-cache", False, lambda x: int(x) != 0)

if nocache or not os.path.isfile(excons_cache):
   UpdateSettings(excons_cache)
else:
   try:
      f = open(excons_cache, "r")
      l = f.readlines()[0].strip()
      f.close()
      if l.strip() == sys.platform:
         ReadSettings(excons_cache)
      else:
         UpdateSettings(excons_cache)
   except:
      UpdateSettings(excons_cache)


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
   }
]

if not nogl:
   prjs.extend([{"name": "AlembicAbcOpenGL",
                 "type": "staticlib",
                 "defs": defs,
                 "incdirs": incdirs,
                 "srcs": glob.glob("lib/AbcOpenGL/*.cpp"),
                 "install": {"include/AbcOpenGL": glob.glob("lib/AbcOpenGL/*.h")}
                },
                {"name": "alembicglmodule",
                 "type": "dynamicmodule",
                 "ext": python.ModuleExtension(),
                 "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
                 "defs": defs + pydefs + ["alembicglmodule_EXPORTS"],
                 "incdirs": incdirs + ["python/PyAbcOpenGL"],
                 "libdirs": libdirs,
                 "srcs": glob.glob("python/PyAbcOpenGL/*.cpp"),
                 "libs": alembicgl_libs + alembic_libs + [boost_python_libname] + pyilmbase_libs + ilmbase_libs + hdf5_libs + libs,
                 "custom": [python.SoftRequire, gl.Require]
                },
                {"name": "SimpleAbcViewer",
                 "type": "program",
                 "defs": defs,
                 "incdirs": incdirs + ["examples/bin/SimpleAbcViewer"],
                 "libdirs": libdirs,
                 "libs": alembicgl_libs + alembic_libs + ilmbase_libs + hdf5_libs + libs,
                 "srcs": glob.glob("examples/bin/SimpleAbcViewer/*.cpp"),
                 "custom": [gl.Require, glut.Require]
                }])

if arnold:
   # TODO
   pass


if maya:
   # TODO
   pass


if houdini:
   # TODO
   pass

excons.DeclareTargets(env, prjs)
