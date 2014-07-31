#ifndef ABCSHAPE_MATHUTILS_H_
#define ABCSHAPE_MATHUTILS_H_

#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImathBox.h>
#include <OpenEXR/ImathMatrix.h>

using Imath::V3d;
using Imath::V4d;
using Imath::M44d;
using Imath::Box3d;

class Plane
{
public:
   
   Plane();
   Plane(const V3d &n, double d);
   Plane(const V3d &p0, const V3d &p1, const V3d &p2, const V3d &above);
   Plane(const Plane &rhs);
   
   Plane& operator=(const Plane &rhs);
   
   void set(const V3d &n, double d);
   void set(const V3d &p0, const V3d &p1, const V3d &p2, const V3d &above);
   
   double distanceTo(const V3d &P) const;
   
private:
   
   V3d mNormal;
   double mDist;
};

// ---

class Frustum
{
public:
   
   enum Side
   {
      Left = 0,
      Right,
      Top,
      Bottom,
      Near,
      Far
   };
   
public:
   
   Frustum();
   Frustum(M44d &projViewInv);
   Frustum(const Frustum &rhs);
   
   Frustum& operator=(const Frustum &rhs);
   
   bool isPointInside(const V3d &p) const;
   bool isPointOutside(const V3d &p) const;
   
   bool isBoxTotallyInside(const Box3d &b) const;
   bool isBoxTotallyOutside(const Box3d &b) const;

private:
   
   Plane mPlanes[6];
};

#endif
