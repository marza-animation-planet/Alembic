#ifndef ABCSHAPE_ALEMBICNODESAMPLEDATA_H_
#define ABCSHAPE_ALEMBICNODESAMPLEDATA_H_


template <class IClass>
struct BaseSampleData
{
   typedef typename IClass::schema_type ISchemaClass;
   typedef typename SchemaUtils<ISchemaClass>::ISampleClass ISampleClass;
   
   static void Get(IClass iObj, Alembic::AbcCoreAbstract::index_t index, ISampleClass &data)
   {
      iObj.getSchema().get(data, Alembic::Abc::ISampleSelector(index));
   }
   
   static void Reset(ISampleClass &data)
   {
      data.reset();
   }
   
   static bool Valid(const ISampleClass &data)
   {
      return SchemaUtils<ISchemaClass>::IsValid(data);
   }
   
   static bool Constant(IClass iObj)
   {
      return iObj.getSchema().isConstant();
   }
};

// ---

template <> struct SampleData<Alembic::AbcGeom::IPolyMesh>
{
   BaseSampleData<Alembic::AbcGeom::IPolyMesh>::ISampleClass data;
   
   void get(Alembic::AbcGeom::IPolyMesh iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      BaseSampleData<Alembic::AbcGeom::IPolyMesh>::Get(iObj, index, data);
   }
   
   void reset(Alembic::AbcGeom::IPolyMesh)
   {
      BaseSampleData<Alembic::AbcGeom::IPolyMesh>::Reset(data);
   }
   
   bool valid(Alembic::AbcGeom::IPolyMesh) const
   {
      return BaseSampleData<Alembic::AbcGeom::IPolyMesh>::Valid(data);
   }
   
   bool constant(Alembic::AbcGeom::IPolyMesh iObj) const
   {
      return BaseSampleData<Alembic::AbcGeom::IPolyMesh>::Constant(iObj);
   }
};

template <> struct SampleData<Alembic::AbcGeom::ISubD>
{
   BaseSampleData<Alembic::AbcGeom::ISubD>::ISampleClass data;
   
   void get(Alembic::AbcGeom::ISubD iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      BaseSampleData<Alembic::AbcGeom::ISubD>::Get(iObj, index, data);
   }
   
   void reset(Alembic::AbcGeom::ISubD)
   {
      BaseSampleData<Alembic::AbcGeom::ISubD>::Reset(data);
   }
   
   bool valid(Alembic::AbcGeom::ISubD) const
   {
      return BaseSampleData<Alembic::AbcGeom::ISubD>::Valid(data);
   }
   
   bool constant(Alembic::AbcGeom::ISubD iObj) const
   {
      return BaseSampleData<Alembic::AbcGeom::ISubD>::Constant(iObj);
   }
};

template <> struct SampleData<Alembic::AbcGeom::IPoints>
{
   BaseSampleData<Alembic::AbcGeom::IPoints>::ISampleClass data;
   
   void get(Alembic::AbcGeom::IPoints iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      BaseSampleData<Alembic::AbcGeom::IPoints>::Get(iObj, index, data);
   }
   
   void reset(Alembic::AbcGeom::IPoints)
   {
      BaseSampleData<Alembic::AbcGeom::IPoints>::Reset(data);
   }
   
   bool valid(Alembic::AbcGeom::IPoints) const
   {
      return BaseSampleData<Alembic::AbcGeom::IPoints>::Valid(data);
   }
   
   bool constant(Alembic::AbcGeom::IPoints iObj) const
   {
      return BaseSampleData<Alembic::AbcGeom::IPoints>::Constant(iObj);
   }
};

template <> struct SampleData<Alembic::AbcGeom::ICurves>
{
   BaseSampleData<Alembic::AbcGeom::ICurves>::ISampleClass data;
   
   void get(Alembic::AbcGeom::ICurves iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      BaseSampleData<Alembic::AbcGeom::ICurves>::Get(iObj, index, data);
   }
   
   void reset(Alembic::AbcGeom::ICurves)
   {
      BaseSampleData<Alembic::AbcGeom::ICurves>::Reset(data);
   }
   
   bool valid(Alembic::AbcGeom::ICurves) const
   {
      return BaseSampleData<Alembic::AbcGeom::ICurves>::Valid(data);
   }
   
   bool constant(Alembic::AbcGeom::ICurves iObj) const
   {
      return BaseSampleData<Alembic::AbcGeom::ICurves>::Constant(iObj);
   }
};

template <> struct SampleData<Alembic::AbcGeom::INuPatch>
{
   BaseSampleData<Alembic::AbcGeom::INuPatch>::ISampleClass data;
   
   void get(Alembic::AbcGeom::INuPatch iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      BaseSampleData<Alembic::AbcGeom::INuPatch>::Get(iObj, index, data);
   }
   
   void reset(Alembic::AbcGeom::INuPatch)
   {
      BaseSampleData<Alembic::AbcGeom::INuPatch>::Reset(data);
   }
   
   bool valid(Alembic::AbcGeom::INuPatch) const
   {
      return BaseSampleData<Alembic::AbcGeom::INuPatch>::Valid(data);
   }
   
   bool constant(Alembic::AbcGeom::INuPatch iObj) const
   {
      return BaseSampleData<Alembic::AbcGeom::INuPatch>::Constant(iObj);
   }
};

template <> struct SampleData<Alembic::AbcGeom::IXform>
{
   BaseSampleData<Alembic::AbcGeom::IXform>::ISampleClass data;
   
   void get(Alembic::AbcGeom::IXform iObj, Alembic::AbcCoreAbstract::index_t index)
   {
      BaseSampleData<Alembic::AbcGeom::IXform>::Get(iObj, index, data);
   }
   
   void reset(Alembic::AbcGeom::IXform)
   {
      BaseSampleData<Alembic::AbcGeom::IXform>::Reset(data);
   }
   
   bool valid(Alembic::AbcGeom::IXform) const
   {
      return BaseSampleData<Alembic::AbcGeom::IXform>::Valid(data);
   }
   
   bool constant(Alembic::AbcGeom::IXform iObj) const
   {
      return BaseSampleData<Alembic::AbcGeom::IXform>::Constant(iObj);
   }
};

#endif
