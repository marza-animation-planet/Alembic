#ifndef SCENEHELPER_ALEMBICNODESAMPLEDATA_H_
#define SCENEHELPER_ALEMBICNODESAMPLEDATA_H_

/*
template <class IClass>
struct SampleData : public SchemaSampleData<IClass>
{
};
*/

template <class IObject>
struct SampleSet : public SampleSetBase<IObject>
{
};

#endif
