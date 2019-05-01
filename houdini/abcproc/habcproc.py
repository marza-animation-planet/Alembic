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


def r_countShapes(node, objectpath, excludepaths=[]):
    global AbcShapeTypes

    count = 0

    path = node.path()
    pathlen = len(path)

    if pathlen < len(objectpath):
        if not objectpath.startswith(path):
            return 0
    elif not path.startswith(objectpath):
        return 0

    for excludepath in excludepaths:
        if pathlen >= len(excludepath) and path.startswith(excludepath):
            return 0

    if node.type() in AbcShapeTypes:
        count = 1
    else:
        for _, cnode in node.children.iteritems():
            count += r_countShapes(cnode, objectpath, excludepaths)

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
        node.parm('abc_start_frame').set(s)
    else:
        node.parm('abc_start_frame').set(0)
    if e:
        node.parm('ar_end_frame').set(e)
        node.parm('abc_end_frame').set(e)
    else:
        node.parm('abc_end_frame').set(0)


def pathChanged(node):
    rfparm = node.parm("ar_read_abc_from")
    if rfparm and rfparm.evalAsInt() == 1:
        node.parm("ar_filename").set(getSourceAlembicPath(node))
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


def getSourceAlembicPath(node):
    abcsrcparm = node.parm("ar_read_abc_from")
    if abcsrcparm is None:
        return ""
    else:
        if abcsrcparm.evalAsInt() == 0:
            return ""
        else:
            sopnode = hou.node(node.parm("ar_source_sop").evalAsString())
            if sopnode is None:
                return ""
            else:
                try:
                    filename = sopnode.parm("fileName").evalAsString()
                    if not filename:
                        print("Source alembic SOP doesn't have a 'fileName' parm")
                        return ""
                    elif not filename.endswith(".abc"):
                        print("Source alembic SOP doesn't point to an alembic file (%s)" % filename)
                        return ""
                    else:
                        return filename
                except Exception, e:
                    print("Failed to get source alembic file path (%s)" % e)
                    return ""


def selectObject(node, shapesOnly=False, exclusive=True, outputParm="ar_objectpath"):
    global AbcShapeTypes

    types = (set(["Xform"]).union(AbcShapeTypes) if not shapesOnly else AbcShapeTypes)

    filename = node.parm("ar_filename").evalAsString()
    objectpath = node.parm(outputParm).evalAsString()
    picked = (() if objectpath in ("", "/") else tuple(filter(lambda y: len(y) > 0, map(lambda x: x.strip(), objectpath.split(" ")))))
    selections = hou.ui.selectFromTree(listNodesInAbc(filename, types), picked=picked, exclusive=exclusive)
    if selections:
        selections = " ".join(selections)
        node.setParms({outputParm: selections})


def countShapes(node):
    global CountCache

    filename = node.parm("ar_filename").evalAsString()
    objectpath = node.parm("ar_objectpath").evalAsString()
    excludepaths = filter(lambda y: len(y) > 0, map(lambda x: x.strip(), node.parm("ar_excludepath").evalAsString().split(" ")))

    count = 0
    npath = _normalizePath(filename)
    key = npath + ":" + objectpath + ":" + ",".join(excludepaths)

    count = CountCache.get(key, -1)
    if count < 0:
        try:
            a = getArchive(npath, normalizePath=False)
            count = r_countShapes(a.top, objectpath, excludepaths)
            releaseArchive(npath, normalizePath=False)
        except:
            count = 0
        CountCache[key] = count

    return count


def isSingleShape(node):
    filename = node.parm("ar_filename").evalAsString()
    objectpath = node.parm("ar_objectpath").evalAsString()
    excludepaths = filter(lambda y: len(y) > 0, map(lambda x: x.strip(), node.parm("ar_excludepath").evalAsString().split(" ")))

    if objectpath and objectpath != "/":
        npath = _normalizePath(filename)

        try:
            a = getArchive(npath, normalizePath=False)
            spl = objectpath.split("/")[1:]

            curnode = a.top
            nextnode = None

            for name in spl:
                nextnode = curnode.children.get(name, None)
                if not nextnode:
                    curnode = None
                    break
                else:
                    nextpath = nextnode.path()
                    pathlen = len(nextpath)
                    for excludepath in excludepaths:
                        if pathlen >= len(excludepath) and nextpath.startswith(excludepath):
                            nextnode = None
                            break
                    curnode = nextnode
                    if curnode is None:
                        break

            rv = (curnode is not None and curnode.type() in AbcShapeTypes)

            releaseArchive(npath, normalizePath=False)

            return rv

        except:
            pass

    return False


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
    node.parm('abc_start_frame').set(startF)
    node.parm('ar_end_frame').set(endF)
    node.parm('abc_end_frame').set(endF)
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
        def isInFolders(p, fldset):
            idx = -1
            flds = p.containingFolders()
            while flds and not flds in fldset:
                flds = flds[:idx]
                idx -= 1
            return (True if flds else False)
        return filter(lambda x: isInFolders(x, fldset), node.parms())

def filterUserAttribute(p, includePrefices, overrideSuffix):
    node = p.node()
    name = p.name()
    if overrideSuffix and name.endswith(overrideSuffix) and node.parm(name.replace(overrideSuffix, "")) is not None:
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

def syncUserAttributes(srcNode, dstNode, verbose=False, includeFolders=[], includePrefices=["ar_user_"], ignorePrefices=["ar_"], targetFolder="Procedural", stripPrefix=True, overrideSuffix="_override", ignoreNames=[]):
    if verbose:
        print("Synching %s attributes: %s -> %s" % (includePrefices, srcNode.path(), dstNode.path()))

    ptg = dstNode.parmTemplateGroup()

    upd = False
    delayLink = []

    dstParms = parmsInFolder(dstNode, targetFolder)
    dstNames = set(map(lambda x: x.name(), dstParms))

    srcParms = filter(lambda x: filterUserAttribute(x, includePrefices, overrideSuffix), parmsInFolder(srcNode, includeFolders))
    srcNames = set(map(lambda x: x.name(), srcParms))

    ignoreSet = set(ignoreNames + ["ar_user_options", "ar_user_options_enable", "abcproc_parms"])
    ignoreTypes = (hou.parmTemplateType.Folder, hou.parmTemplateType.FolderSet, hou.parmTemplateType.Button, hou.parmTemplateType.Separator)

    relPath = dstNode.relativePathTo(srcNode)

    # When strip prefix is disabled, be carefull of overlaps between ignore and include prefices
    if not stripPrefix:
        _ignorePrefices = []
        for ignp in ignorePrefices:
            remove = False
            for incp in includePrefices:
                if incp.startswith(ignp):
                    remove = True
                    break
            if not remove:
                _ignorePrefices.append(ignp)
        ignorePrefices = _ignorePrefices

    for parm in srcParms:
        removedPrefix = None
        name = parm.name()
        if name in ignoreSet:
            if verbose:
                print("Ignored: '%s'" % name)
            continue

        if stripPrefix:
            for prefix in includePrefices:
                if name.startswith(prefix):
                    name = name.replace(prefix, "")
                    removedPrefix = prefix
                    break

        skip = False
        for prefix in ignorePrefices:
            if name.startswith(prefix):
                skip = True
                break
        if skip:
            if verbose:
                print("  Skip '%s'" % name)
            continue

        if verbose:
            print("  " + parm.name())

        dontCreate = False

        tpl = parm.parmTemplate()

        # For multi component parms like color, vector or float[2], a single template is shared
        if tpl.name() in ignoreSet:
            dontCreate = True
        else:
            print("  Add parm '%s' template '%s' to ignore set" % (parm.name(), tpl.name()))
            ignoreSet.add(tpl.name())

        isString = (tpl.dataType() == hou.parmData.String)

        scr = "ch%s(\"%s/%s\")" % ("s" if isString else "", relPath, parm.name())

        if not name in dstNames:
            if not dontCreate:
                if verbose:
                    print("    => Add new param and link")
                newtpl = tpl.clone()
                tplname = tpl.name()
                if removedPrefix:
                    tplname = tplname.replace(removedPrefix, "")
                newtpl.setName(tplname)
                ptg.appendToFolder(targetFolder.split("/"), newtpl)
            else:
                if verbose:
                    print("    => Link new param")
            delayLink.append((name, scr))
            upd = True

        else:
            if verbose:
                print("    => Param already exists")
            dstParm = dstNode.parm(name)
            refParm = dstParm.getReferencedParm()
            # Check against both parm and its referenced parm (if any)
            if refParm != parm and refParm != parm.getReferencedParm():
                if verbose:
                    print("    => Not referencing the right attribute, re-link parm")
                dstParm.setExpression(scr, hou.exprLanguage.Hscript, True)

    if upd:
        dstNode.setParmTemplateGroup(ptg)
        for name, scr in delayLink:
            dstNode.parm(name).setExpression(scr, hou.exprLanguage.Hscript, True)

        # Update dstParms and dstNames
        dstParms = parmsInFolder(dstNode, targetFolder)
        dstNames = set(map(lambda x: x.name(), dstParms))

    if verbose:
        print("Remove un-wanted parms from '%s'" % dstNode.path())

    # Remove parameters deleted from srcNode too
    for pn in dstNames:
        parm = dstNode.parm(pn)

        if not parm:
            continue

        if not parm.isSpare():
            continue

        tpl = parm.parmTemplate()

        if tpl.type() in ignoreTypes:
            continue

        #if parm.name() in ignoreSet:
        if tpl.name() in ignoreSet:
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
            print("  => Remove attribute '%s'" % pn)

        dstNode.removeSpareParmTuple(parm.tuple())

    return True


def autobumpVisibility(node):
    try:
        import arnold
        rv = 0
        for n, m in [("camera", arnold.AI_RAY_CAMERA), \
                     ("shadow", arnold.AI_RAY_SHADOW), \
                     ("diffuse_reflect", arnold.AI_RAY_DIFFUSE_REFLECT), \
                     ("specular_reflect", arnold.AI_RAY_SPECULAR_REFLECT), \
                     ("diffuse_transmit", arnold.AI_RAY_DIFFUSE_TRANSMIT), \
                     ("specular_transmit", arnold.AI_RAY_SPECULAR_TRANSMIT), \
                     ("volume", arnold.AI_RAY_VOLUME)]:
            p = node.parm("ar_autobump_visibility_" + n)
            if p is not None and p.eval():
                rv = rv | m
        return rv
    except:
        return 1


def getDisplacementShader(node):
    if node:
        cat = node.type().category().name()
        if cat == "Object":
            p = node.parm("shop_materialpath")
            if p is None:
                return ""
            else:
                np = hou.node(p.eval())
                if np is None:
                    return ""
                else:
                    node = np
                    cat = node.type().category().name()
        if cat != "Shop":
            return ""
        matl = filter(lambda x: x.type().name() == "arnold_material", node.allSubChildren())
        for mat in matl:
            iidx = mat.inputIndex("displacement")
            if iidx < 0:
                continue
            conns = mat.inputConnectors()[iidx]
            for conn in conns:
                inode = conn.inputNode()
                if inode is not None:
                    return inode.path()
    return ""


def updateDisplacementShader(node, sync=True):
    node.parm("ar_user_disp_map").set(getDisplacementShader(node))
    node.parm("ar_user_disp_map_override").set(1)
    if sync:
        node.parm("sync_user_parms").pressButton()
