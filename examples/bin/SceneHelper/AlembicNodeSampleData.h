#ifndef SCENEHELPERTEST_ALEMBICNODESAMPLEDATA_H_
#define SCENEHELPERTEST_ALEMBICNODESAMPLEDATA_H_

template <class IClass>
struct SampleData : public SchemaSampleData<IClass>
{
};

template <class IClass>
struct MeshUVs
{
   Alembic::AbcGeom::IV2fGeomParam prop;
   Alembic::AbcGeom::IV2fGeomParam::Sample data;
   
   void get(IClass iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      prop = iObj.getSchema().getUVsParam();
      if (prop.valid())
      {
         if (!prop.isConstant() || !data.valid())
         {
            prop.getExpanded(data, Alembic::Abc::ISampleSelector(index));
         }
      }
   }
   
   void reset(IClass)
   {
      if (prop.valid() && !prop.isConstant())
      {
         data.reset();
      }
   }
   
   bool valid(IClass) const
   {
      return (!prop.valid() || data.valid());
   }
   
   bool constant(IClass) const
   {
      return (!prop.valid() || prop.isConstant());
   }
};

struct MeshNormals
{
   Alembic::AbcGeom::IN3fGeomParam prop;
   Alembic::AbcGeom::IN3fGeomParam::Sample data;
   
   void get(Alembic::AbcGeom::IPolyMesh iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      prop = iObj.getSchema().getNormalsParam();
      if (prop.valid())
      {
         if (!prop.isConstant() || !data.valid())
         {
            prop.getExpanded(data, Alembic::Abc::ISampleSelector(index));
         }
      }
   }
   
   void reset(Alembic::AbcGeom::IPolyMesh)
   {
      if (prop.valid() && !prop.isConstant())
      {
         data.reset();
      }
   }
   
   bool valid(Alembic::AbcGeom::IPolyMesh) const
   {
      return (!prop.valid() || data.valid());
   }
   
   bool constant(Alembic::AbcGeom::IPolyMesh) const
   {
      return (!prop.valid() || prop.isConstant());
   }
};

template <> struct SampleData<Alembic::AbcGeom::IPolyMesh> : public SchemaSampleData<Alembic::AbcGeom::IPolyMesh>
{
   MeshUVs<Alembic::AbcGeom::IPolyMesh> uv;
   MeshNormals N;
   
   void get(Alembic::AbcGeom::IPolyMesh iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      SchemaSampleData<Alembic::AbcGeom::IPolyMesh>::get(iObj, index);
      uv.get(iObj, index);
      N.get(iObj, index);
   }
   
   void reset(Alembic::AbcGeom::IPolyMesh iObj)
   {
      SchemaSampleData<Alembic::AbcGeom::IPolyMesh>::reset(iObj);
      uv.reset(iObj);
      N.reset(iObj);
   }
   
   bool valid(Alembic::AbcGeom::IPolyMesh iObj) const
   {
      return (SchemaSampleData<Alembic::AbcGeom::IPolyMesh>::valid(iObj) && uv.valid(iObj) && N.valid(iObj));
   }
   
   bool constant(Alembic::AbcGeom::IPolyMesh iObj) const
   {
      return (SchemaSampleData<Alembic::AbcGeom::IPolyMesh>::constant(iObj) && uv.constant(iObj) && N.constant(iObj));
   }
};

template <> struct SampleData<Alembic::AbcGeom::ISubD> : public SchemaSampleData<Alembic::AbcGeom::ISubD>
{
   MeshUVs<Alembic::AbcGeom::ISubD> uv;
   
   void get(Alembic::AbcGeom::ISubD iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      SchemaSampleData<Alembic::AbcGeom::ISubD>::get(iObj, index);
      uv.get(iObj, index);
   }
   
   void reset(Alembic::AbcGeom::ISubD iObj)
   {
      SchemaSampleData<Alembic::AbcGeom::ISubD>::reset(iObj);
      uv.reset(iObj);
   }
   
   bool valid(Alembic::AbcGeom::ISubD iObj) const
   {
      return (SchemaSampleData<Alembic::AbcGeom::ISubD>::valid(iObj) && uv.valid(iObj));
   }
   
   bool constant(Alembic::AbcGeom::ISubD iObj) const
   {
      return (SchemaSampleData<Alembic::AbcGeom::ISubD>::constant(iObj) && uv.constant(iObj));
   }
};

#endif
