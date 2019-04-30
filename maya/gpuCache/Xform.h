#ifndef ABCSHAPE_XFORM_H
#define ABCSHAPE_XFORM_H

#include <maya/MFnTransform.h>
#include <Alembic/AbcGeom/IXform.h>

void ReadXform(const Alembic::AbcGeom::XformSample &sample, MFnTransform &trans, bool reset=true);

#endif
