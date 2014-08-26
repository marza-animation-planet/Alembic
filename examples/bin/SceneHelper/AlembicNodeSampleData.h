#ifndef SCENEHELPERTEST_ALEMBICNODESAMPLEDATA_H_
#define SCENEHELPERTEST_ALEMBICNODESAMPLEDATA_H_

template <class IClass>
struct SampleSet : public SampleSetBase<IClass>
{
};

template <> struct SampleSet<Alembic::AbcGeom::IPolyMesh> : public SampleSetBase<Alembic::AbcGeom::IPolyMesh>
{
   TimeSampleList<Alembic::AbcGeom::IV2fGeomParam> uvSamples;
   TimeSampleList<Alembic::AbcGeom::IN3fGeomParam> nSamples;
};

template <> struct SampleSet<Alembic::AbcGeom::ISubD> : public SampleSetBase<Alembic::AbcGeom::ISubD>
{
   TimeSampleList<Alembic::AbcGeom::IV2fGeomParam> uvSamples;
};

#endif
