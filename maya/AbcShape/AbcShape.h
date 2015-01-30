#ifndef ABCSHAPE_ABCSHAPE_H_
#define ABCSHAPE_ABCSHAPE_H_

#include "common.h"

#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MBoundingBox.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MPlug.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MTime.h>
#include <maya/M3dView.h>
#include <maya/MDGMessage.h>


class AbcShape : public MPxSurfaceShape
{
public:
    
    static const MTypeId ID;
    static MCallbackId CallbackID;
    
    static MObject aFilePath;
    static MObject aObjectExpression;
    static MObject aDisplayMode;
    static MObject aTime;
    static MObject aSpeed;
    static MObject aPreserveStartFrame;
    static MObject aStartFrame;
    static MObject aEndFrame;
    static MObject aOffset;
    static MObject aCycleType;
    static MObject aIgnoreXforms;
    static MObject aIgnoreInstances;
    static MObject aIgnoreVisibility;
    static MObject aNumShapes;
    static MObject aPointWidth;
    static MObject aLineWidth;
    static MObject aDrawTransformBounds;
    static MObject aDrawLocators;
    static MObject aOutBoxMin;
    static MObject aOutBoxMax;
    static MObject aAnimated;
    
#ifdef ABCSHAPE_VRAY_SUPPORT
    // V-Ray specific attributes
    static MObject aOutApiType;
    static MObject aVRayGeomResult;
    static MObject aVRayGeomInfo;
#endif
    
    static void* creator();
    
    static MStatus initialize();
    
    enum CycleType
    {
        CT_hold = 0,
        CT_loop,
        CT_reverse,
        CT_bounce
    };
    
    enum DisplayMode
    {
        DM_box = 0,
        DM_boxes,
        DM_points,
        DM_geometry
    };
    
public:
    
    friend class AbcShapeOverride;
    
    AbcShape();
    virtual ~AbcShape();
    
    virtual void postConstructor();
    
    virtual MStatus compute(const MPlug &, MDataBlock &);
    
    virtual bool isBounded() const;
    virtual MBoundingBox boundingBox() const;
    
    virtual bool getInternalValueInContext(const MPlug &plug,
                                           MDataHandle &handle,
                                           MDGContext &ctx);
    virtual bool setInternalValueInContext(const MPlug &plug,
                                           const MDataHandle &handle,
                                           MDGContext &ctx);
    virtual void copyInternalData(MPxNode *source);
    
    inline AlembicScene* scene() { return mScene; }
    inline const AlembicScene* scene() const { return mScene; }
    inline const SceneGeometryData* sceneGeometry() const { return &mGeometry; }
    inline bool ignoreInstances() const { return mIgnoreInstances; }
    inline bool ignoreTransforms() const { return mIgnoreTransforms; }
    inline bool ignoreVisibility() const { return mIgnoreVisibility; }
    inline DisplayMode displayMode() const { return mDisplayMode; }
    inline float lineWidth() const { return mLineWidth; }
    inline float pointWidth() const { return mPointWidth; }
    inline bool drawTransformBounds() const { return mDrawTransformBounds; }
    inline bool drawLocators() const { return mDrawLocators; }
    inline unsigned int numShapes() const { return mNumShapes; }
    inline bool isAnimated() const { return mAnimated; }
    
    bool ignoreCulling() const;

private:
    
    double getFPS() const;
    double computeAdjustedTime(double inputTime, double speed, double timeOffset) const;
    double computeRetime(double inputTime, double firstTime, double lastTime, CycleType cycleType) const;
    double getSampleTime() const;
    
    void printInfo(bool detailed=false) const;
    void printSceneBounds() const;
    
    void syncInternals();
    void syncInternals(MDataBlock &block);
    
    void updateObjects();
    void updateRange();
    void updateWorld();
    void updateGeometry();
    
    enum UpdateLevel
    {
        UL_none = 0,
        UL_geometry,
        UL_world,
        UL_range,
        UL_objects
    };
    
private:
    
    MString mFilePath;
    MString mObjectExpression;
    MTime mTime;
    double mOffset;
    double mSpeed;
    CycleType mCycleType;
    double mStartFrame;
    double mEndFrame;
    double mSampleTime;
    bool mIgnoreInstances;
    bool mIgnoreTransforms;
    bool mIgnoreVisibility;
    AlembicScene *mScene;
    DisplayMode mDisplayMode;
    SceneGeometryData mGeometry;
    unsigned int mNumShapes;
    float mPointWidth;
    float mLineWidth;
    bool mPreserveStartFrame;
    bool mDrawTransformBounds;
    bool mDrawLocators;
    int mUpdateLevel;
    AlembicSceneFilter mSceneFilter;
    bool mAnimated;
    
#ifdef ABCSHAPE_VRAY_SUPPORT
    VR::DefStringParam mVRFileName;                        // [""]
    VR::DefStringParam mVRObjectPath;                      // [""]
    VR::DefFloatParam mVRAnimSpeed;                        // [1.0]
    VR::DefIntParam mVRAnimType;                           // [0, (0: loop, 1: once, 2: ping-ping, 3: still)]
    VR::DefFloatParam mVRAnimOffset;                       // [0.0]
    VR::DefBoolParam mVRAnimOverride;                      // [false]
    VR::DefIntParam mVRAnimStart;                          // [0]
    VR::DefIntParam mVRAnimLength;                         // [0]
    VR::DefBoolParam mVRPrimaryVisibility;                 // [true]
    VR::DefBoolParam mVRUseAlembicOffset;                  // [false]
    VR::DefBoolParam mVRUseFaceSets;                       // [true]
    VR::DefBoolParam mVRUseFullNames;                      // [false]
    VR::DefBoolParam mVRComputeBBox;                       // [false]
    // VR::DefFloatParam mVRScale;                            // [1.0]
    // VR::DefBoolParam mVRFlipAxis;                          // [false]
    // VR::DefBoolParam mVRSmoothUVBorders;                   // [true]
    VR::DefBoolParam mVRSmoothUV;                          // [true]
    // VR::DefBoolParam mVRComputeNormals;                    // [true]
    // VR::DefFloatParam mVRSmoothAngle;                      // [30.0]
    // VR::DefBoolParam mVRFlipNormals;                       // [false]
    // VR::DefIntParam mVRNumPreviewFaces;                    // [10000]
    // VR::DefFloatParam mVRHairWidthMultiplier;              // [1.0]
    // VR::DefFloatParam mVRParticleWidthMultiplier;          // [1.0]
    // VR::DefIntParam mVRVisibilityListType;                 // [0, (0: exclude, 1: include)]
    // VR::DefStringListParam mVRVisibilityListNames;         // [()]
    // VR::DefIntListParam mVRVisibilityListIds;              // [()]
    // VR::DefIntParam mVRHairVisibilityListType;             // [0, (0: exclude, 1: include)]
    // VR::DefStringListParam mVRHairVisibilityListNames;     // [()]
    // VR::DefIntListParam mVRHairVisibilityListIds;          // [()]
    // VR::DefIntParam mVRParticleVisibilityListType;         // [0, (0: exclude, 1: include)]
    // VR::DefStringListParam mVRParticleVisibilityListNames; // [()]
    // VR::DefIntListParam mVRParticleVisibilityListIds;      // [()]
    // VR::DefIntParam mVRSortVoxels;                         // [0]
    // VR::DefIntParam mVRFirstMapChannel;                    // [1]
    // VR::DefIntParam mVRParticleRenderMode;                 // [0, (0: spheres, 1: points)]
    // VR::DefStringParam mVRVelocityColorSet;                // [""]    
    VR::DefPluginParam mVRMesh;
    VR::DefIntParam mVRPreserveMapBorders;
    VR::DefBoolParam mVRStaticSubdiv;
    VR::DefBoolParam mVRClassicCatmark;
    VR::DefBoolParam mVRUseGlobals;
    VR::DefBoolParam mVRViewDep;
    VR::DefFloatParam mVREdgeLength;
    VR::DefIntParam mVRMaxSubdivs;
    VR::DefBoolParam mVRUseBounds;
    VR::DefColorParam mVRMinBound;
    VR::DefColorParam mVRMaxBound;
    VR::DefBoolParam mVRCacheNormals;
    VR::DefBoolParam mVRStaticDisp;
    VR::DefIntParam mVRPrecision;
    VR::DefBoolParam mVRDisp2D;
    VR::DefBoolParam mVRTightBounds;
    VR::DefIntParam mVRResolution;
    VR::DefBoolParam mVRFilterTexture;
    VR::DefFloatParam mVRFilterBlur;
    VR::DefIntParam mVRVectorDisp;
    // VR::DefBoolParam mVRObjectSpaceDisp;
    VR::DefBoolParam mVRKeepContinuity;
    VR::DefFloatParam mVRWaterLevel;
    VR::DefFloatParam mVRDispAmount;
    VR::DefFloatParam mVRDispShift;
#endif
};

class AbcShapeUI : public MPxSurfaceShapeUI
{
public:
    
    static void *creator();
    
public:
    
    enum DrawToken
    {
        kDrawBox = 0,
        kDrawPoints,
        kDrawGeometry,
        kDrawGeometryAndWireframe
    };
    
    AbcShapeUI();
    virtual ~AbcShapeUI();
    
    virtual void getDrawRequests(const MDrawInfo &info,
                                 bool objectAndActiveOnly,
                                 MDrawRequestQueue &queue);
    
    virtual void draw(const MDrawRequest &request, M3dView &view) const;
    
    virtual bool select(MSelectInfo &selectInfo,
                        MSelectionList &selectionList,
                        MPointArray &worldSpaceSelectPts) const;
    
    // Compute frustum from maya view projection and modelview matrices
    bool computeFrustum(M3dView &view, Frustum &frustum) const;
    
    // Compute frustum straight from OpenGL projection and modelview matrices
    bool computeFrustum(Frustum &frustum) const;
    
    void getWorldMatrix(M3dView &view, Alembic::Abc::M44d &worldMatrix) const;

private:
    
    void drawBox(AbcShape *shape, const MDrawRequest &request, M3dView &view) const;
    void drawPoints(AbcShape *shape, const MDrawRequest &request, M3dView &view) const;
    void drawGeometry(AbcShape *shape, const MDrawRequest &request, M3dView &view) const;
};

#endif
