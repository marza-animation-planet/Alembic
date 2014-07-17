import alembic

def VisitNodes(inode, func, depth=0, *funcArgs, **funcKwargs):
  if not func(inode, depth, *funcArgs, **funcKwargs):
      return
  for ichild in inode.children:
      VisitNodes(ichild, func, depth+1, *funcArgs, **funcKwargs)

def CollectUVNames(mesh, checkUVMetadata=True):
  masterName = None
  allUVs = set()
  
  schema = mesh.getSchema()
  uvs = schema.getUVsParam()
  
  if uvs.valid():
      allUVs = set()
      masterName = uvs.getMetaData().getSourceName()
      if not masterName:
        masterName = "map1"
      
      allUVs.add(masterName)
      
      params = schema.getArbGeomParams()
      if params.valid():
        for propHeader in params.propertyheaders:
          if not alembic.AbcGeom.IV2fGeomParam.matches(propHeader):
            continue
          
          param = alembic.AbcGeom.IV2fGeomParam(params, propHeader.getName())
          if param.getScope() == alembic.AbcGeom.GeometryScope.kFacevaryingScope and (not checkUVMetadata or propHeader.isUV()):
            if propHeader.getName() in allUVs:
              print("* WARNING * Already exist")
            allUVs.add(propHeader.getName())
  
  return (masterName, allUVs)

def GetNodeFullPath(inode):
  fullpath = inode.getName()
  iparent = inode.getParent()
  while iparent and alembic.AbcGeom.IXform.matches(iparent.getHeader()):
    fullpath = "%s|%s" % (iparent.getName(), fullpath)
    iparent = iparent.getParent()
  return fullpath

def AddNamespace(path, ns):
  if ns in ("", ":"):
    return path
  if not ns.endswith(":"):
    ns += ":"
  return "|".join(map(lambda x: (ns+x if x else x), path.split("|")))

def GetMayaMesh(path):
  try:
    import maya.OpenMaya as api
    
    sl = api.MSelectionList()
    sl.add(path)
    
    dagpath = api.MDagPath()
    sl.getDagPath(0, dagpath)
    
    mesh = api.MFnMesh(dagpath)
    
    return mesh
    
  except Exception, e:
    print(e)
    return None

def FixUVs(inode, depth, namespace=":", checkUVMetadata=True):
    header = inode.getHeader()
    
    node = None
    imesh = None
    
    if alembic.AbcGeom.IPolyMesh.matches(header):
        node = GetMayaMesh(AddNamespace(GetNodeFullPath(inode), namespace))
        if node:
          imesh = alembic.AbcGeom.IPolyMesh(inode, alembic.Abc.WrapExistingFlag.kWrapExisting)
    
    elif alembic.AbcGeom.ISubD.matches(header):
        node = GetMayaMesh(AddNamespace(GetNodeFullPath(inode), namespace))
        if node:
          imesh = alembic.AbcGeom.ISubD(inode, alembic.Abc.WrapExistingFlag.kWrapExisting)
    
    if imesh:
      masterUV, allUVs = CollectUVNames(imesh, checkUVMetadata=checkUVMetadata)
      
      # meshUVs = []
      # node.getUVSetNames(meshUVs)
      # for meshUV in meshUVs:
      #   if not meshUV in allUVs:
      #     if meshUVs.index(meshUV) == 0:
      #       node.copyUVSetWithName(masterUV, meshUV)
      #       node.deleteUVSet(masterUV)
      #       node.renameUVSet(meshUV, masterUV)
      #       # This doesn't work
      #     else:
      #       node.deleteUVSet(meshUV)
      
      if node.currentUVSetName() != masterUV:
        node.setCurrentUVSetName(masterUV)
    
    return True

def test():
  abc = "cubes2.abc"
  iarch = alembic.Abc.IArchive(abc)
  VisitNodes(iarch.getTop(), FixUVs, checkUVMetadata=True)

if __name__ == "__main__":
  test()
