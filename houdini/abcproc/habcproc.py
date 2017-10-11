import hou
import sys
import math
import cask
import alembic

AbcShapeTypes = set(["PolyMesh", "SubD", "Curve", "NuPatch", "Points"])
ArchiveCache = {}
CountCache = {}
Verbose = False


def r_findNodes(node, validTypes=None):
    nodes = set()

    if not validTypes or node.type() in validTypes:
        nodes.add(node.path())

    for _, cnode in node.children.iteritems():
        nodes = nodes.union(r_findNodes(cnode, validTypes))

    return nodes


def r_findShapes(node):
    global AbcShapeTypes

    return r_findNodes(node, AbcShapeTypes)


def r_countShapes(node, objectpath):
    global AbcShapeTypes

    count = 0

    path = node.path()

    if len(path) < len(objectpath):
        if not objectpath.startswith(path):
            return 0
    elif not path.startswith(objectpath):
        return 0

    if node.type() in AbcShapeTypes:
        count = 1
    else:
        for _, cnode in node.children.iteritems():
            count += r_countShapes(cnode, objectpath)

    return count


def _normalizePath(path):
    return (path if sys.platform != "win32" else path.lower().replace("\\", "/"))


def getArchive(path, normalizePath=True):
    global ArchiveCache, Verbose

    if not path:
        return None

    a = None

    qp = (path if not normalizePath else _normalizePath(path))
    tpl = ArchiveCache.get(qp, None)

    if tpl is None:
        try:
            a = cask.Archive(path)
            if Verbose:
                print("Load ABC archive \"%s\"" % qp)
            ArchiveCache[qp] = (a, 1)
        except:
            # Don't create entry
            pass
    else:
        a, cnt = tpl
        if a is not None:
            if Verbose:
                print("Reference ABC archive \"%s\" (%d)" % (qp, cnt + 1))
            ArchiveCache[qp] = (a, cnt + 1)

    return a


def acquireArchive(node):
    path = node.parm("ar_filename").evalAsString()
    getArchive(path)
    node.setUserData("abcproc_prevpath", path)


def releaseArchive(path, normalizePath=True, force=False):
    global ArchiveCache, CountCache, Verbose

    qp = (path if not normalizePath else _normalizePath(path))
    a, cnt = ArchiveCache.get(qp, (None, 0))

    if a is not None:
        cnt -= 1
        if force or cnt <= 0:
            if Verbose:
                print("Release ABC archive \"%s\"%s" % (qp, (" [forced]" if force else "")))
            a.close()
            del(ArchiveCache[qp])

            # Clear count cache
            for k in CountCache.keys()[:]:
                if k.split(":")[0] == qp:
                    del(CountCache[k])
        else:
            if Verbose:
                print("Dereference ABC archive \"%s\" (%d)" % (qp, cnt))
            ArchiveCache[qp] = (a, cnt)


def getNode(archive, path, node=None):
    if not archive or not path:
        return None

    lst = path.split("/")

    if len(lst) == 0:
        return node

    if lst[0] == "":
        if path == "":
            return (archive.top if node is None else node)
        else:
            if node is not None:
                return None
            curnode = archive.top
            lst = lst[1:]
    else:
        curnode = (archive.top if node is None else node)

    while len(lst) > 0:
        if lst[0] in curnode.children:
            curnode = curnode.children[lst[0]]
            lst = lst[1:]
        else:
            return None

    return curnode


def getFrameRange(path, frameRange=(None, None), fps=24):
    startFrame, endFrame = frameRange
    npath = _normalizePath(path)

    try:
        a = getArchive(npath, normalizePath=False)
        a.fps = fps
        startFrame, endFrame = a.frame_range()
        releaseArchive(npath, normalizePath=False)
    except:
        pass

    return startFrame, endFrame


def setFrameRange(node):
    abc_file = _normalizePath(node.parm('ar_filename').eval())

    try:
        a = getArchive(abc_file, normalizePath=False)
        a.fps = hou.fps()
        s, e = a.frame_range()
        releaseArchive(abc_file, normalizePath=False)
    except:
        s, e = None, None

    if s:
        node.parm('ar_start_frame').set(s)
    if e:
        node.parm('ar_end_frame').set(e)


def pathChanged(node):
    newpath = node.parm("ar_filename").evalAsString()
    oldpath = node.userData("abcproc_prevpath")
    if oldpath != newpath:
        releaseArchive(oldpath)
        acquireArchive(node)
    setFrameRange(node)


def initPath(node):
    pp = node.userData("abcproc_prevpath")
    if pp is not None:
        node.destroyUserData("abcproc_prevpath")
    pathChanged(node)


def listNodesInAbc(path, validTypes=None):
    rv = []
    npath = _normalizePath(path)

    try:
        a = getArchive(npath, normalizePath=False)
        rv = list(r_findNodes(a.top, validTypes))
        releaseArchive(npath, normalizePath=False)
    except:
        pass

    return rv


def listShapesInAbc(path):
    global AbcShapeTypes

    return listNodesInAbc(path, AbcShapeTypes)


def selectObject(node, shapesOnly=False):
    global AbcShapeTypes

    types = (set(["Xform"]).union(AbcShapeTypes) if not shapesOnly else AbcShapeTypes)

    filename = node.parm("ar_filename").evalAsString()
    objectpath = node.parm("ar_objectpath").evalAsString()
    picked = (() if objectpath in ("", "/") else (objectpath,))
    selections = hou.ui.selectFromTree(listNodesInAbc(filename, types), picked=picked, exclusive=True)
    if selections:
        node.setParms({"ar_objectpath": selections[0]})


def countShapes(node):
    global CountCache

    filename = node.parm("ar_filename").evalAsString()
    objectpath = node.parm("ar_objectpath").evalAsString()

    count = 0
    npath = _normalizePath(filename)
    key = npath + ":" + objectpath

    count = CountCache.get(key, -1)
    if count < 0:
        try:
            a = getArchive(npath, normalizePath=False)
            count = r_countShapes(a.top, objectpath)
            releaseArchive(npath, normalizePath=False)
        except:
            count = 0
        CountCache[key] = count

    return count


# In the TRUE sense of the word
def isSingleShape(node):
    filename = node.parm("ar_filename").evalAsString()
    objectpath = node.parm("ar_objectpath").evalAsString()

    if objectpath and objectpath != "/":
        npath = _normalizePath(filename)

        try:
            a = getArchive(npath, normalizePath=False)
            spl = objectpath.split("/")[1:]
            print(spl)

            curnode = a.top
            nextnode = None

            for name in spl:
                nextnode = curnode.children.get(name, None)
                if not nextnode:
                    curnode = None
                    break
                else:
                    curnode = nextnode

            rv = (curnode is not None and curnode.type() in AbcShapeTypes)

            releaseArchive(npath, normalizePath=False)

            return rv

        except:
            pass

    return False


# NOT SURE THIS ONE IS NEEDED ANYMORE
def hasMotionSamples(node):
    samples = node.parm("ar_samples")
    ignore_mb = (node.parm("ar_ignore_deform_blur").evalAsInt() and node.parm("ar_ignore_transform_blur").evalAsInt())
    return (True if (not ignore_mb and samples.evalAsInt() > 0) else False)


def computeFrame(node, frame=None, ignoreCycle=False):
    if frame is None:
        frame = hou.frame()
    sf = node.parm('ar_start_frame').eval()
    ef = node.parm('ar_end_frame').eval()
    o = node.parm('ar_offset').eval()
    s = node.parm('ar_speed').eval()
    c = node.parm('ar_cycle').eval()
    f = hou.fps()

    if node.parm('ar_preserve_start_frame').eval() and math.fabs(s) > 0.0001:
        o += sf * (s - 1.0) / s

    outframe = s * (frame - o)

    if ignoreCycle:
        return outframe

    playlength = ef - sf
    eps = 0.001 * f # 0.001s -> eps frame

    if c == 1:
        # Loop
        if outframe < (sf - eps) or outframe > (ef + eps):
            # Normalized relative frame ([sf,ef] -> [0,1])
            nrelframe = (outframe - sf) / playlength
            fraction = math.fabs(nrelframe - math.floor(nrelframe))
            outframe = sf + fraction * playlength

    elif c == 2:
        # Reverse
        if outframe > (sf + eps) and outframe < (ef - eps):
            nrelframe = (outframe - sf) / playlength
            fraction = math.fabs(nrelframe - math.floor(nrelframe))
            outframe = ef - fraction * playlength
        elif outframe < (sf + eps):
            outframe = ef
        else:
            outframe = sf

    elif c == 3:
        # Bounce
        if outframe < (sf - eps) or outframe > (ef + eps):
            nrelframe = (outframe - sf) / playlength
            repeat = int(math.floor(nrelframe))
            fraction = math.fabs(nrelframe - repeat)
            if repeat % 2 == 0:
                outframe = sf + fraction * playlength
            else:
                outframe = ef - fraction * playlength
    else:
        # Hold
        if outframe < (sf - eps):
            outframe = sf
        elif outframe > (ef + eps):
            outframe = ef

    return outframe


def computeTime(node, frame=None, ignoreCycle=False):
    outframe = computeFrame(node, frame, ignoreCycle)
    return hou.frameToTime(outframe)


def isClipped(node):
    # cycle values
    #   0 -> hold
    #   1 -> loop
    #   2 -> reverse
    #   3 -> bounce
    #   4 -> clip
    if node.parm("ar_cycle").eval() == 4:
        f0 = computeFrame(node)
        f1 = computeFrame(node, ignoreCycle=True)
        return (f1 < f0 or f1 > f0)
    else:
        return False


def createArnoldAbc(path=None, mode='multiProc', objectPath='/', frameRange=(None, None), offset=0):
    node = hou.node('/obj').createNode('abcproc')

    node.parm('ar_filename').set(path)

    startF, endF = getFrameRange(path, (0, 0))

    if frameRange[0]:
        startF = frameRange[0]

    if frameRange[1]:
        endF = frameRange[1]

    node.parm('ar_start_frame').set(startF)
    node.parm('ar_end_frame').set(endF)

    #if mode != 'multiProc' and mode != 'singleShape':
    #    mode = 'multiProc'
    #node.parm('abc_mode').set(mode)
    #node.parm('ar_objectpath').set('/' if mode == 'multiProc' else objectPath)

    node.parm('ar_objectpath').set(objectPath)
    node.parm('ar_offset').set(offset)


def getCurrentRop():
    ropType = hou.nodeType(hou.nodeTypeCategories()["Driver"], "arnold")

    rop = None

    if ropType:
        try:
            rop = hou.node(hou.hscriptExpression("$DRIVERPATH"))
            if rop and rop.type() != ropType:
                rop = None
        except:
            rop = None

        if not rop:
            insts = ropType.instances()
            for inst in insts:
                if inst.isBypassed():
                    continue
                if inst.isLocked():
                    continue
                if not inst.parm("ar_ass_export_enable").evalAsInt():
                    rop = inst
                    break
                elif rop is None:
                    rop = inst

    return rop


def _isMbEnabled(rop):
    if not rop:
        return False
    else:
        return (rop.parm('ar_mb_dform_enable').eval() or rop.parm('ar_mb_xform_enable').eval())


def getMbSteps(rop=None):
    steps = 0
    if not rop:
        rop = getCurrentRop()
        if not rop:
            return 1

    if rop.parm('ar_mb_dform_enable').eval():
        steps = rop.parm('ar_mb_dform_keys').eval()

    if rop.parm('ar_mb_xform_enable').eval():
        xform_steps = rop.parm('ar_mb_xform_keys').eval()
        if steps > 0 and xform_steps != steps:
            print("Warning: Transform and deform motion steps mismatch, use transform motion steps")
        steps = xform_steps

    return (1 if steps <= 0 else steps)


def getMbSampleRangeAndLength(rop=None):
    if not rop:
        rop = getCurrentRop()
        if not rop:
            return (0.0, 0.0, 0.0)

    # Standard HtoA ROP doesn't have sample/shutter control separated
    which = ("shutter" if rop.parm("ar_mb_sample") is None else "sample")

    sample_pos = rop.parm('ar_mb_%s' % which).eval()

    if sample_pos == 'start':
        l = rop.parm('ar_mb_%s_length' % which).eval()
        return (0.0, l, l)

    elif sample_pos == 'center':
        l = rop.parm('ar_mb_%s_length' % which).eval()
        return (-0.5 * l, 0.5 * l, l)

    elif sample_pos == 'end':
        l = rop.parm('ar_mb_%s_length' % which).eval()
        return (-l, 0.0, l)

    else:
        s, e = rop.parmTuple('ar_mb_%s_range' % which).eval()
        return (s, e, e - s)


def getMbSampleLength(rop=None):
    if not rop:
        rop = getCurrentRop()
        if not rop:
            return 0.0

    # Standard HtoA ROP doesn't have sample/shutter control separated
    which = ("shutter" if rop.parm("ar_mb_sample") is None else "sample")

    if rop.parm('ar_mb_%s' % which).eval() == 'custom':
        s, e = rop.parmTuple('ar_mb_%s_range' % which).eval()
        return (e - s)
    else:
        return rop.parm('ar_mb_%s_length' % which).eval()


def getMbSamplePos(rop=None):
    # Return in relative frames
    sp = []
    if not rop:
        rop = getCurrentRop()

    if rop and _isMbEnabled(rop):
        steps = getMbSteps(rop)

        s, e, sample_len = getMbSampleRangeAndLength(rop)

        if steps == 1:
            sp.append(s)

        else:
            incr = float(sample_len) / float(steps - 1)
            for i in xrange(steps):
                sp.append(s)
                s += incr

    return sp


def getMbSampleFrame(offset=0, rop=None):
    if not rop:
        rop = getCurrentRop()

    samples = []

    if rop:
        cur_frame = hou.frame() + offset

        for pos in getMbSamplePos():
            samp_frame = cur_frame + pos
            samples.append(samp_frame)

    return samples


def getMbCenterPos(rop=None):
    # Return in relative frame
    if not rop:
        rop = getCurrentRop()

    if _isMbEnabled(rop):
        shutter_pos = rop.parm('ar_mb_shutter').eval()
        shutter_len = rop.parm('ar_mb_shutter_length').eval()

        samples = getMbSamplePos()

        if len(samples) <= 1:
            return 0.0

        else:
            start = samples[0]
            end = samples[-1]

            sample_len = getMbSampleLength(rop)

            if shutter_pos == 'start':
                sstart = start
                send = start + shutter_len * sample_len

            elif shutter_pos == 'center':
                center = 0.5 * (start + end)
                sstart = center - 0.5 * shutter_len * sample_len
                send = center + 0.5 * shutter_len * sample_len

            elif shutter_pos == 'end':
                sstart = end - shutter_len * sample_len
                send = end

            else:
                sstart = start + rop.parm('ar_mb_shutter_rangex').eval() * sample_len
                send = start + rop.parm('ar_mb_shutter_rangey').eval() * sample_len

            return 0.5 * (sstart + send)

    else:
        return 0.0


def getMbCenterFrame(offset=0, rop=None):
    return (hou.frame() + offset + getMbCenterPos(rop))


def parmsInFolder(node, folders):
    if type(folders) in (str, unicode):
        folders = [folders]
    if len(folders) == 0:
        return node.parms()
    else:
        fldset = set(map(lambda x: tuple(x.split("/")), folders))
        return filter(lambda x: x.containingFolders() in fldset, node.parms())


def syncUserAttributes(srcNode, dstNode, verbose=False, includeFolders=[], includePrefices=["ar_user_"], ignorePrefices=["ar_"], targetFolder="Procedural", stripPrefix=True, overrideSuffix="_override", ignoreNames=[]):
    if verbose:
        print("Synching %s attributes: %s -> %s" % (includePrefices, srcNode.path(), dstNode.path()))

    ptg = dstNode.parmTemplateGroup()

    upd = False
    delayLink = []

    dstParms = parmsInFolder(dstNode, targetFolder)
    dstNames = set(map(lambda x: x.name(), dstParms))

    def includeFilter(p):
        node = p.node()
        name = p.name()
        if name.endswith(overrideSuffix) and node.parm(name.replace(overrideSuffix, "")) is not None:
            # Skip toggle attributes
            return False
        if len(includePrefices) == 0:
            return True
        else:
            for includePrefix in includePrefices:
                if name.startswith(includePrefix):
                    # Check for override toggle
                    ovrp = node.parm(name + overrideSuffix)
                    if ovrp is None or ovrp.eval():
                        return True
                    else:
                        return False
            return False

    srcParms = filter(includeFilter, parmsInFolder(srcNode, includeFolders))
    srcNames = set(map(lambda x: x.name(), srcParms))

    ignoreSet = set(ignoreNames + ["ar_user_options", "ar_user_options_enable", "abcproc_parms"])

    relPath = dstNode.relativePathTo(srcNode)

    for parm in srcParms:
        name = parm.name()
        if name in ignoreSet:
            continue

        if stripPrefix:
            for prefix in includePrefices:
                if name.startswith(prefix):
                    name = name.replace(prefix, "")
                    break

        skip = False
        for prefix in ignorePrefices:
            if name.startswith(prefix):
                skip = True
                break
        if skip:
            continue

        if verbose:
            print("  " + parm.name())

        isString = (parm.parmTemplate().dataType() == hou.parmData.String)

        scr = "ch%s(\"%s/%s\")" % ("s" if isString else "", relPath, parm.name())

        if not name in dstNames:
            if verbose:
                print("    => Add new param and link")
            newtpl = parm.parmTemplate().clone()
            newtpl.setName(name)
            ptg.appendToFolder(targetFolder.split("/"), newtpl)
            delayLink.append((name, scr))
            upd = True

        else:
            if verbose:
                print("    => Param already exists")
            dstParm = dstNode.parm(name)
            refParm = dstParm.getReferencedParm()
            if refParm != parm:
                if verbose:
                    print("  => Not referencing the right attribute")
                dstParm.setExpression(scr, hou.exprLanguage.Hscript, True)

    if upd:
        dstNode.setParmTemplateGroup(ptg)
        for name, scr in delayLink:
            dstNode.parm(name).setExpression(scr, hou.exprLanguage.Hscript, True)

        # Update dstParms and dstNames
        dstParms = parmsInFolder(dstNode, targetFolder)
        dstNames = set(map(lambda x: x.name(), dstParms))

    # Remove parameters deleted from srcNode too
    for parm in dstParms:
        if not parm.isSpare():
            # Cannot delete non-spare parameters
            continue

        if parm.name() in ignoreSet:
            continue

        skip = False
        for prefix in ignorePrefices:
            if parm.name().startswith(prefix):
                skip = True
                break
        if skip:
            continue

        if stripPrefix:
            for prefix in includePrefices:
                name = prefix + parm.name()
                if name in srcNames:
                    skip = True
                    break
        else:
            skip = (parm.name() in srcNames)
        if skip:
            continue

        if verbose:
            print("  => Remove attribute \"" + parm.name() + "\"")

        dstNode.removeSpareParmTuple(parm.tuple())

    return True
