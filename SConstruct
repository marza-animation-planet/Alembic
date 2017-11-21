import os
import re
import sys
import glob
import excons
import excons.config
from excons.tools import hdf5
from excons.tools import python
from excons.tools import ilmbase
from excons.tools import boost
from excons.tools import gl
from excons.tools import glut
from excons.tools import glew
from excons.tools import arnold
from excons.tools import mtoa
from excons.tools import maya
from excons.tools import vray
from excons.tools import houdini


excons.InitGlobals()

version_tpl = (1, 7, 4)
version_str = ".".join(map(str, version_tpl))

use_boost = True
use_tr1 = False
use_hdf5 = (excons.GetArgument("alembic-hdf5-support", 0, int) != 0)

link_static = (excons.GetArgument("alembic-link-static", 1, int) != 0)

nameprefix = excons.GetArgument("alembic-maya-prefix", default="")

regexSrcDir = "lib/SceneHelper/regex-2.7/src"
regexInc = [regexSrcDir + "/regex.h"] if sys.platform == "win32" else []
regexSrc = [regexSrcDir + "/regex.c"] if sys.platform == "win32" else []

arnoldInc, arnoldLib = excons.GetDirs("arnold", libdirname=("lib" if sys.platform == "win32" else "bin"), silent=True)

withArnold = (arnoldInc and arnoldLib)
withMaya = (excons.GetArgument("with-maya", default=None) is not None)
withVray = (excons.GetArgument("with-vray", default=None) is not None)
withHoudini = (excons.GetArgument("with-houdini", default=None) is not None)

if withArnold:
   A, M, m, p = arnold.Version(asString=False)
   if A != 5:
      withArnold = None
   elif M == 0 and m < 1:
      withArnold = None
   if not withArnold:
      print("Arnold 5 above 5.0.1.0 required")

if withMaya:
   maya.SetupMscver()

if withVray:
   vrayVer = vray.Version(asString=False, nice=True)
else:
   vrayVer = (0, 0, 0)

if withHoudini:
   houVer = houdini.Version(asString=False, full=False)
   if houVer < 15.5:
      print("Houdini %s is not supported" % houVer)
      withHoudini = None


env = excons.MakeBaseEnv()


# IlmBase setup
rv = excons.ExternalLibRequire("ilmbase", noLink=True)
if not rv["require"]:
   excons.PrintOnce("Build IlmBase from sources.", tool="alembic")
   opts = {"openexr-suffix": "-2_2",
           "ilmbase-python-staticlibs": "1"}
   excons.Call("openexr", overrides=opts, imp=["RequireImath", "RequireIlmThread", "RequirePyImath"])
   def RequireIlmBase(env):
      RequireImath(env, static=True)
      RequireIlmThread(env, static=True)
   def RequirePyIlmBase(env):
      RequirePyImath(env, staticpy=True, staticbase=True)
      RequireIlmThread(env, static=True)
else:
   pyimath_static = (excons.GetArgument("ilmbase-python-static", excons.GetArgument("ilmbase-static", 0, int), int) != 0)
   def RequireIlmBase(env):
      ilmbase.Require(ilmthread=True, iexmath=True)(env)
   def RequirePyIlmBase(env):
      if pyimath_static:
         env.Append(CPPDEFINES=["PYILMBASE_STATICLIBS"])
      ilmbase.Require(ilmthread=True, iexmath=True, python=True)(env)
      boost.Require(libs=["python"])(env)
      python.SoftRequire(env)


# static or shared?
def RequireAlembic(static=True, withPython=False, withGL=False, linkCore=True, linkGL=True):

   def _RequireFunc(env):
      if sys.platform == "darwin":
         env.Append(CPPFLAGS=" ".join([" -Wno-unused-parameter",
                                       "-Wno-unused-value",
                                       "-Wno-unused-function",
                                       "-Wno-unused-variable"]))
      elif sys.platform == "win32":
         env.Append(CCFLAGS=" /bigobj")
         env.Append(CPPDEFINES=["NOMINMAX"])

         mscmaj = int(excons.mscver.split(".")[0])

         # Work around the 2GB file limit with Visual Studio 10.0
         #   https://connect.microsoft.com/VisualStudio/feedback/details/627639/std-fstream-use-32-bit-int-as-pos-type-even-on-x64-platform
         #   https://groups.google.com/forum/#!topic/alembic-discussion/5ElRJkXhi9M
         if mscmaj == 10:
            env.Prepend(CPPPATH=["lib/vc10fix"])

         if mscmaj < 10:
            excons.WarnOnce("You should use at least Visual C 10.0 if you expect reading alembic file above 2GB without crash (use mscver= flag)", tool="alembic")

      else:
         env.Append(CPPFLAGS=" ".join([" -Wno-unused-local-typedefs",
                                       "-Wno-unused-parameter",
                                       "-Wno-unused-but-set-variable",
                                       "-Wno-unused-variable",
                                       "-Wno-ignored-qualifiers"]))

      defs = []
      incdirs = ["lib"]
      libdir = excons.OutputBaseDirectory() + "/lib"
      libprefix = ("lib" if (sys.platform != "win32" or static) else "")
      libext = (".lib" if sys.platform == "win32" else (".a" if static else excons.SharedLibraryLinkExt()))
      libs = []

      if sys.platform == "win32":
         defs.extend(["PLATFORM_WINDOWS", "PLATFORM=WINDOWS"])
         if not static:
            defs.append("ALEMBIC_DLL")
            if withGL:
               defs.append("ABC_OPENGL_DLL")

      elif sys.platform == "darwin":
         defs.extend(["PLATFORM_DARWIN", "PLATFORM=DARWIN"])
         libs.append("m")

      else:
         defs.extend(["PLATFORM_LINUX", "PLATFORM=LINUX"])
         libs.extend(["dl", "rt", "m"])

      env.Append(CPPDEFINES=defs)

      env.Append(CPPPATH=incdirs)

      env.Append(LIBPATH=[libdir])

      if withGL and linkGL:
         excons.Link(env, libdir + "/" + libprefix + "AlembicAbcOpenGL" + libext, static=static, force=True, silent=True)

      if linkCore:
         excons.Link(env, libdir + "/" + libprefix + "Alembic" + libext, static=static, force=True, silent=True)

      reqs = []

      if withGL:
         if sys.platform != "darwin":
           reqs.append(glew.Require)
         reqs.append(glut.Require)
         reqs.append(gl.Require)

      if withPython:
         reqs.append(RequirePyIlmBase)
         if use_hdf5:
            reqs.append(hdf5.Require(hl=True, verbose=True))

      else:
         reqs.append(RequireIlmBase)
         reqs.append(boost.Require())
         if use_hdf5:
            reqs.append(hdf5.Require(hl=True, verbose=True))

      for req in reqs:
         req(env)

      if libs:
         env.Append(LIBS=libs)


   return _RequireFunc


def RequireRegex(env):
   global regexSrcDir

   if sys.platform == "win32":
      env.Append(CPPDEFINES=["REGEX_STATIC"])
      env.Append(CPPPATH=[regexSrcDir])


def RequireAlembicHelper(static=True, withPython=False, withGL=False):
   def _RequireFunc(env):

      RequireRegex(env)

      env.Append(CPPPATH=["lib/SceneHelper"])

      env.Append(LIBS=["SceneHelper"])

      requireAlembic = RequireAlembic(static=static, withPython=withPython, withGL=withGL)

      requireAlembic(env)


   return _RequireFunc


prjs = []

# Base library
sublibs = ["Abc", "AbcCollection", "AbcCoreAbstract", "AbcCoreFactory", "AbcCoreLayer", "AbcCoreOgawa", "AbcGeom", "AbcMaterial", "Ogawa", "Util"]
if use_hdf5:
   sublibs.append("AbcCoreHDF5")

out_headers_dir = "%s/include/Alembic" % excons.OutputBaseDirectory()

cfgin = "lib/Alembic/Util/Config.h.in"
cfgout = "%s/Util/Config.h" % out_headers_dir

opts = {"PROJECT_VERSION_MAJOR": str(version_tpl[0]),
        "PROJECT_VERSION_MINOR": str(version_tpl[1]),
        "PROJECT_VERSION_PATCH": str(version_tpl[2]),
        "ALEMBIC_WITH_HDF5": ("" if use_hdf5 else "//") + "#define ALEMBIC_WITH_HDF5",
        "ALEMBIC_LIB_USES_BOOST": ("" if use_boost else "//") + "#define ALEMBIC_LIB_USES_BOOST",
        "ALEMBIC_LIB_USES_TR1": ("" if use_tr1 else "//") + "#define ALEMBIC_LIB_USES_TR1"}

GenerateConfig = excons.config.AddGenerator(env, "abccfg", opts, pattern="\$\{([^}]+)\}|#cmakedefine\s+([^\s]+)")

configh = GenerateConfig(cfgout, cfgin)

lib_headers = []
lib_sources = {}

for sl in sublibs:
   lib_headers += env.Install("%s/%s" % (out_headers_dir, sl), excons.glob("lib/Alembic/%s/*.h" % sl))
   lib_sources[sl] = excons.glob("lib/Alembic/%s/*.cpp" % sl)

lib_headers += env.Install("%s/AbcOpenGL" % out_headers_dir, excons.glob("abcview/lib/AbcOpenGL/*.h"))

# Base libraries

prjs.append({"name": ("libAlembic" if sys.platform == "win32" else "Alembic"),
             "type": "staticlib",
             "alias": "alembic-static",
             "desc": "Alembic library",
             "srcs": lib_sources,
             "custom": [RequireAlembic(static=True, linkCore=False)]})

prjs.append({"name": "Alembic",
             "type": "sharedlib",
             "alias": "alembic-shared",
             "desc": "Alembic library",
             "symvis": "hidden",
             "defs": ["ALEMBIC_EXPORTS"],
             "version": version_str,
             "install_name": "libAlembic.1.dylib",
             "soname": "libAlembic.so.1",
             "srcs": lib_sources,
             "custom": [RequireAlembic(static=False, linkCore=False)]})

prjs.append({"name": ("libAlembicAbcOpenGL" if sys.platform == "win32" else "AlembicAbcOpenGL"),
             "type": "staticlib",
             "alias": "alembicgl-static",
             "desc": "Alembic OpenGL library",
             "srcs": excons.glob("abcview/lib/AbcOpenGL/*.cpp"),
             "custom": [RequireAlembic(static=True, withGL=True, linkGL=False)]})

prjs.append({"name": "AlembicAbcOpenGL",
             "type": "sharedlib",
             "alias": "alembicgl-shared",
             "desc": "Alembic OpenGL library",
             "symvis": "hidden",
             "defs": ["ABC_OPENGL_EXPORTS"],
             "version": "1.1.0",
             "install_name": "libAlembicAbcOpenGL.1.dylib",
             "soname": "libAlembicAbcOpenGL.so.1",
             "srcs": excons.glob("abcview/lib/AbcOpenGL/*.cpp"),
             "custom": [RequireAlembic(static=False, withGL=True, linkGL=False)]})

# Python modules
pydefs = []
if excons.GetArgument("boost-python-static", excons.GetArgument("boost-static", 0, int), int) != 0:
   pydefs.append("PYALEMBIC_USE_STATIC_BOOST_PYTHON")

prjs.extend([{"name": ("alembicmodule" if sys.platform != "win32" else "alembic"),
              "type": "dynamicmodule",
              "desc": "Alembic python module",
              "ext": python.ModuleExtension(),
              "alias": "alembic-python",
              "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
              "rpaths": ["../.."],
              "bldprefix": "python-%s" % python.Version(),
              "defs": pydefs + ["alembicmodule_EXPORTS"],
              "incdirs": ["python/PyAlembic"],
              "srcs": excons.glob("python/PyAlembic/*.cpp"),
              "custom": [RequireAlembic(static=link_static, withPython=True)],
              "install": {"%s/%s" % (python.ModulePrefix(), python.Version()): ["cask/cask.py"]}
             },
             {"name": ("alembicglmodule" if sys.platform != "win32" else "alembicgl"),
              "type": "dynamicmodule",
              "desc": "Alembic OpenGL python module",
              "ext": python.ModuleExtension(),
              "alias": "alembic-python",
              "prefix": "%s/%s" % (python.ModulePrefix(), python.Version()),
              "rpaths": ["../.."],
              "bldprefix": "python-%s" % python.Version(),
              "defs": pydefs + ["alembicglmodule_EXPORTS"],
              "incdirs": ["abcview/python/PyAbcOpenGL"],
              "srcs": excons.glob("abcview/python/PyAbcOpenGL/*.cpp"),
              "custom": [RequireAlembic(static=link_static, withPython=True, withGL=True)]}])

# Command line tools
prog_srcs = {"AbcEcho": ["bin/AbcEcho/AbcEcho.cpp"],
             "AbcBoundsEcho": ["bin/AbcEcho/AbcBoundsEcho.cpp"]}

prog_names = ["AbcDiff",
              "AbcEcho",
              "AbcBoundsEcho",
              "AbcLs",
              "AbcStitcher",
              "AbcTree",
              "AbcWalk"]
if use_hdf5:
   prog_names.append("AbcConvert")

for progname in prog_names:
   prjs.append({"name": progname.lower(),
                "type": "program",
                "alias": "alembic-tools",
                "srcs": prog_srcs.get(progname, excons.glob("bin/%s/*.cpp" % progname)),
                "custom": [RequireAlembic(static=link_static)]})

# Requires Qt
# prjs.append({"name": "abcview",
#              "type": "program",
#              "alias": "alembic-tools",
#              "srcs": excons.glob("abcview/bin/AbcView/*.cpp"),
#              "custom": [RequireAlembic(static=link_static, withGL=True)]})

# Helper library and test
prjs.extend([{"name": "SceneHelper",
              "type": "staticlib",
              "desc": "Alembic scene handling utility library",
              "srcs": excons.glob("lib/SceneHelper/*.cpp") + regexSrc,
              "custom": [RequireAlembicHelper(static=link_static)],
              "install": {"include/SceneHelper": excons.glob("lib/SceneHelper/*.h") + regexInc}
             },
             {"name": "SceneHelperTest",
              "type": "program",
              "desc": "SceneHelper library test program",
              "incdirs": ["examples/bin/SceneHelper"],
              "srcs": excons.glob("examples/bin/SceneHelper/*.cpp"),
              "custom": [RequireAlembicHelper(static=link_static)]}])

# DCC tools

defs = []
if nameprefix:
   defs.append("NAME_PREFIX=\"\\\"%s\\\"\"" % nameprefix)

if withArnold:
   prjs.append({"name": "%sAlembicArnoldProcedural" % nameprefix,
                "type": "dynamicmodule",
                "desc": "Original arnold alembic procedural",
                "ext": arnold.PluginExt(),
                "prefix": "arnold/%s" % arnold.Version(compat=True),
                "rpaths": ["../../lib"],
                "bldprefix": "arnold-%s" % arnold.Version(),
                "defs": defs,
                "incdirs": ["arnold/Procedural"],
                "srcs": excons.glob("arnold/Procedural/*.cpp") + regexSrc,
                "custom": [arnold.Require, RequireRegex, RequireAlembic(static=link_static)]})

   prjs.append({"name": "abcproc",
                "type": "dynamicmodule",
                "alias": "alembic-arnold",
                "desc": "Arnold alembic procedural",
                "ext": arnold.PluginExt(),
                "prefix": "arnold/%s" % arnold.Version(compat=True),
                "rpaths": ["../../lib"],
                "bldprefix": "arnold-%s" % arnold.Version(),
                "incdirs": ["arnold/abcproc"],
                "srcs": excons.glob("arnold/abcproc/*.cpp") + excons.glob("arnold/abcproc/visitors/*.cpp"),
                "custom": [arnold.Require, RequireAlembicHelper(static=link_static)]})

if withVray:
   cppflags = ""
   if sys.platform.startswith("linux"):
      cppflags = " -Wno-write-strings -Wno-maybe-uninitialized -Wno-sign-compare"
   prjs.append({"name": "%svray_AlembicLoader" % ("lib" if sys.platform != "win32" else ""),
                "type": "dynamicmodule",
                "alias": "alembic-vray",
                "desc": "V-Ray alembic procedural",
                "ext": vray.PluginExt(),
                "prefix": "vray/%d.%d" % (vrayVer[0], vrayVer[1]),
                "rpaths": ["../lib"],
                "bldprefix": "vray-%s" % vray.Version(),
                "incdirs": ["vray"],
                "cppflags": cppflags,
                "srcs": excons.glob("vray/*.cpp"),
                "custom": [vray.Require, RequireAlembicHelper(static=link_static)]})

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
   trsver = excons.GetArgument("maya-abctranslator-version", None)
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
                 "alias": "alembic-maya",
                 "type": "dynamicmodule",
                 "desc": "Maya alembic import plugin",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins/%s" % maya.Version(nice=True),
                 "rpaths": ["../../../lib"],
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCIMPORT_VERSION=\"\\\"%s\\\"\"" % impver] if impver else []),
                 "incdirs": ["maya/AbcImport"],
                 "srcs": excons.glob("maya/AbcImport/*.cpp") + regexSrc,
                 "custom": [RequireRegex, RequireAlembic(static=link_static), maya.Require, maya.Plugin]
                },
                {"name": "%sAbcExport" % nameprefix,
                 "alias": "alembic-maya",
                 "desc": "Maya alembic export plugin",
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins/%s" % maya.Version(nice=True),
                 "rpaths": ["../../../lib"],
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCEXPORT_VERSION=\"\\\"%s\\\"\"" % expver] if expver else []),
                 "incdirs": ["maya/AbcExport"],
                 "srcs": excons.glob("maya/AbcExport/*.cpp"),
                 "custom": [RequireAlembic(static=link_static), maya.Require, maya.Plugin]
                },
                {"name": "%sAbcShape" % nameprefix,
                 "alias": "alembic-maya",
                 "desc": "Maya alembic proxy shape plugin",
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins/%s" % maya.Version(nice=True) + ("/vray-%d.%d" % (vrayVer[0], vrayVer[1]) if withVray else ""),
                 "rpaths": ["../../../lib" if not withVray else "../../../../lib"],
                 "bldprefix": "maya-%s" % maya.Version() + ("/vray-%s" % vray.Version() if withVray else ""),
                 "defs": defs + (["ABCSHAPE_VERSION=\"\\\"%s\\\"\"" % shpver] if shpver else []) +
                                (["ABCSHAPE_VRAY_SUPPORT"] if withVray else []),
                 "incdirs": ["maya/AbcShape"],
                 "srcs": excons.glob("maya/AbcShape/*.cpp"),
                 "custom": ([vray.Require] if withVray else []) + [RequireAlembicHelper(static=link_static), maya.Require, gl.Require, maya.Plugin],
                 "install": {"maya/scripts": excons.glob("maya/AbcShape/*.mel"),
                             "maya/python": [AbcShapeMtoa, AbcMatEditPy] + ([AbcShapePy] if withVray else [])}
                },
                {"name": "%sAbcFileTranslator" % nameprefix,
                 "alias": "alembic-maya",
                 "desc": "Maya file translator for alembic import and export",
                 "type": "dynamicmodule",
                 "ext": maya.PluginExt(),
                 "prefix": "maya/plug-ins/%s" % maya.Version(nice=True),
                 "rpaths": ["../../../lib"],
                 "bldprefix": "maya-%s" % maya.Version(),
                 "defs": defs + (["ABCFILETRANSLATOR_VERSION=\"\\\"%s\\\"\"" % trsver] if trsver else []),
                 "incdirs": ["maya/AbcFileTranslator"],
                 "srcs": excons.glob("maya/AbcFileTranslator/*.cpp"),
                 "custom": [RequireAlembic(static=link_static), maya.Require, maya.Plugin],
                 "install": {"maya/scripts": excons.glob("maya/AbcFileTranslator/*.mel")}}])

   if withArnold:
      A, M, m = mtoa.Version(asString=False)
      # So far MtoA 2.>=0 supported only
      if A >= 2:
         AbcShapeMtoaAE = "maya/AbcShape/mtoa/%sMtoa.py" % AbcShapeName

         if not os.path.exists(AbcShapeMtoaAE) or os.stat(AbcShapeMtoaAE).st_mtime < os.stat("maya/AbcShape/mtoa/AbcShapeMtoa.py.tpl").st_mtime:
            replace_in_file("maya/AbcShape/mtoa/AbcShapeMtoa.py.tpl", AbcShapeMtoaAE, "<<NodeName>>", AbcShapeName)

         prjs.append({"name": "%sAbcShapeMtoa" % nameprefix,
                      "type": "dynamicmodule",
                      "alias": "alembic-mtoa",
                      "desc": "AbcShape translator for MtoA",
                      "prefix": "maya/plug-ins/%s/mtoa-%s" % (maya.Version(nice=True), mtoa.Version(compat=True)),
                      "rpaths": ["../../../../lib"],
                      "bldprefix": "maya-%s/mtoa-%s" % (maya.Version(), mtoa.Version()),
                      "ext": mtoa.ExtensionExt(),
                      "defs": defs,
                      "srcs": excons.glob("maya/AbcShape/mtoa/*.cpp"),
                      "install": {"maya/plug-ins/%s/mtoa-%s" % (maya.Version(nice=True), mtoa.Version(compat=True)): [AbcShapeMtoaAE]},
                      "custom": [RequireAlembicHelper(static=link_static), mtoa.Require, arnold.Require, maya.Require]})

if withHoudini and withArnold:
   # HtoA
   prjs.append({"name": "alembic-htoa",
                "type": "install",
                "install": {"houdini/otls/%s" % houVer: excons.glob("houdini/abcproc/*.otl"),
                            "houdini/python": excons.glob("houdini/abcproc/*.py")}})



excons.AddHelpTargets({"alembic-static": "Alembic static library",
                       "alembic-shared": "Alembic shared library",
                       "alembicgl-static": "Alembic OpenGL static library",
                       "alembicgl-shared": "Alembic OpenGL shared library",
                       "alembic-libs": "Alembic static and shared libraries",
                       "alembic-tools": "Alembic command line tools",
                       "alembic-maya": "All alembic maya plugins",
                       "alembic-arnold": "Arnold procedural",
                       "alembic-mtoa": "MtoA translator for alembic AbcShape",
                       "alembic-htoa": "HtoA OTL for Arnold alembic procedural",
                       "alembic-vray": "V-Ray procedural",
                       "eco": "Alembic ecosystem package"})

excons.AddHelpOptions(alembic="""ALEMBIC OPTIONS
  alembic-hdf5-support=0|1  : Build with HDF5 support.                   [0]
  alembic-link-static=0|1   : Link static alembic library into binaries. [1]
  alembic-maya-prefix=<str> : Maya plugin name prefix.                   ['']""")

targets = excons.DeclareTargets(env, prjs)

env.Alias("alembic-libs", ["alembic-static", "alembic-shared", "alembicgl-static", "alembicgl-shared"])
env.Depends(targets["alembic-static"], lib_headers)
env.Depends(targets["alembic-shared"], lib_headers)
env.Depends(targets["alembicgl-static"], lib_headers)
env.Depends(targets["alembicgl-shared"], lib_headers)

Export("RequireAlembic")
Export("RequireAlembicHelper")

deftargets = ["alembic-libs", "alembic-tools", "alembic-python"]
if withArnold:
   deftargets.append("alembic-arnold")
   if withHoudini:
      deftargets.append("alembic-htoa")
if withVray:
   deftargets.append("alembic-vray")
if withMaya:
   deftargets.append("alembic-maya")
   if withArnold:
      deftargets.append("alembic-mtoa")
Default(deftargets)

if "eco" in COMMAND_LINE_TARGETS:
   outbd = excons.OutputBaseDirectory()
   ecop = "/" + excons.EcosystemPlatform()

   ecotgts = {"tools": targets["alembic-tools"],
              "python": targets["alembic-python"] + ["cask/cask.py"]}

   outdirs = {"tools": ecop + "/bin",
              "python": ecop + "/lib/python/%s" % python.Version()}

   if withArnold:
      ecotgts["arnold"] = targets["abcproc"]
      outdirs["arnold"] = ecop + "/arnold/%s" % arnold.Version(compat=True)

      if withHoudini:
         from pprint import pprint as pp
         ecotgts["htoa-otls"] = excons.glob("houdini/abcproc/*.otl")
         outdirs["htoa-otls"] = ecop + "/houdini/otls/%s" % houdini.Version(asString=False, full=False)
         ecotgts["htoa-scripts"] = excons.glob("houdini/abcproc/*.py")
         outdirs["htoa-scripts"] = ecop + "/houdini/python"

   if withMaya:
      ecotgts["maya-scripts"] = [outbd + "/maya/python", outbd + "/maya/scripts"],
      outdirs["maya-scripts"] = ecop + "/maya"

      ecotgts["maya-plugins"] = targets["alembic-maya"]
      outdirs["maya-plugins"] = ecop + "/maya/plug-ins/%s" % maya.Version(nice=True)

      if withArnold:
         tgtname = nameprefix + "AbcShapeMtoa"
         if tgtname in targets:
            ecotgts["mtoa"] = targets[tgtname] + [AbcShapeMtoaAE]
            outdirs["mtoa"] = ecop + "/maya/plug-ins/%s/mtoa-%s" % (maya.Version(nice=True), mtoa.Version(compat=True))

   excons.EcosystemDist(env, "alembic.env", outdirs, targets=ecotgts, name="alembic", version=".".join(map(str, version_tpl)))


