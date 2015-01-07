#ifndef ABCSHAPE_VP2_H_
#define ABCSHAPE_VP2_H_

#include <maya/MPxDrawOverride.h>
#include <maya/MDagPath.h>
#include <maya/MUserData.h>
#if MAYA_API_VERSION >= 201400
#  include <maya/MFrameContext.h>
#endif

class AbcShapeOverride : public MHWRender::MPxDrawOverride
{
public:
   
   static MString Classification;
   static MString Registrant;
   
   static MHWRender::MPxDrawOverride* create(const MObject& obj);

   static void draw(const MHWRender::MDrawContext& context, const MUserData* data);

public:
   
   AbcShapeOverride(const MObject &obj);
   virtual ~AbcShapeOverride();
   
   virtual MHWRender::DrawAPI supportedDrawAPIs() const;

   virtual bool isBounded(const MDagPath& objPath, const MDagPath& cameraPath) const;

   virtual MBoundingBox boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const;

#if MAYA_API_VERSION >= 201400
   virtual MUserData* prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, const MHWRender::MFrameContext& frameContext, MUserData* oldData);
#else
   virtual MUserData* prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, MUserData* oldData);
#endif
};

#endif
