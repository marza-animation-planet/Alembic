#include "Xform.h"
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MEulerRotation.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>


static void ResetXform(MFnTransform &trans)
{
   MSpace::Space space = MSpace::kPreTransform;

   const MVector vec(0, 0, 0);
   const MPoint point(0, 0, 0);
   const double zero[3] = {0, 0, 0};
   const double one[3] = {1, 1, 1};
   const MQuaternion quat;
   MTransformationMatrix::RotationOrder order = MTransformationMatrix::kXYZ;

   trans.setScalePivot(point, space, false);
   trans.setScale(one);
   trans.setShear(zero);
   trans.setScalePivotTranslation(vec, space);
   trans.setRotatePivot(point, space, false);
   trans.setRotateOrientation(quat, space, false);
   trans.setRotationOrder(order, true);
   trans.setRotation(zero, order);
   trans.setRotatePivotTranslation(vec, space);
   trans.setTranslation(vec, space);
}

static bool IsComplexXform(const Alembic::AbcGeom::XformSample &sample)
{
   int state = 0;

   bool scPivot = false;
   bool roPivot = false;
   bool xAxis = false;
   bool yAxis = false;
   bool zAxis = false;

   size_t numOps = sample.getNumOps();

   for (size_t i = 0; i < numOps; ++i)
   {
      Alembic::AbcGeom::XformOp op = sample[i];

      switch (op.getType())
      {
      case Alembic::AbcGeom::kScaleOperation:
         if (state < 12)
         {
            state = 12;
         }
         else
         {
            return true;
         }
         break;

      case Alembic::AbcGeom::kTranslateOperation:
         switch (op.getHint())
         {
         case Alembic::AbcGeom::kTranslateHint:
            if (state < 1)
            {
               state = 1;
            }
            else
            {
               return true;
            }
            break;

         case Alembic::AbcGeom::kScalePivotPointHint:
            if (state < 10 && !scPivot)
            {
               scPivot = true;
               state = 10;
            }
            // we have encounted this pivot before,
            // this one undoes the first one
            else if (state < 13 && scPivot)
            {
               scPivot = false;
               state = 13;
            }
            else
            {
               return true;
            }
            break;

         case Alembic::AbcGeom::kScalePivotTranslationHint:
            if (state < 9)
            {
               state = 9;
            }
            else
            {
               return true;
            }
            break;

         case Alembic::AbcGeom::kRotatePivotPointHint:
            if (state < 3 && !roPivot)
            {
               roPivot = true;
               state = 3;
            }
            // we have encounted this pivot before,
            // this one undoes the first one
            else if (state < 8 && roPivot)
            {
               roPivot = false;
               state = 8;
            }
            else
            {
               return true;
            }
            break;

         case Alembic::AbcGeom::kRotatePivotTranslationHint:
            if (state < 2)
            {
               state = 2;
            }
            else
            {
               return true;
            }
            break;

         default:
            return true;
         }
         break;

      case Alembic::AbcGeom::kRotateXOperation:
      case Alembic::AbcGeom::kRotateYOperation:
      case Alembic::AbcGeom::kRotateZOperation:
      case Alembic::AbcGeom::kRotateOperation:
         // if any axis is animated, we have a complex xform
         if (op.isXAnimated() || op.isYAnimated() || op.isZAnimated())
         {
            return true;
         }
         else
         {
            Alembic::Abc::V3d v = op.getAxis();

            switch (op.getHint())
            {
            case Alembic::AbcGeom::kRotateHint:
               if (v.x == 1 && v.y == 0 && v.z == 0 && !xAxis && state <= 4)
               {
                  state = 4;
                  xAxis = true;
               }
               else if (v.x == 0 && v.y == 1 && v.z == 0 && !yAxis && state <= 4)
               {
                  state = 4;
                  yAxis = true;
               }
               else if (v.x == 0 && v.y == 0 && v.z == 1 && !zAxis && state <= 4)
               {
                  state = 4;
                  zAxis = true;
               }
               else
               {
                  return true;
               }
               break;

            case Alembic::AbcGeom::kRotateOrientationHint:
               if (v.x == 1 && v.y == 0 && v.z == 0 && state < 7)
               {
                  state = 7;
               }
               else if (v.x == 0 && v.y == 1 && v.z == 0 && state < 6)
               {
                  state = 6;
               }
               else if (v.x == 0 && v.y == 0 && v.z == 1 && state < 5)
               {
                  state = 5;
               }
               else
               {
                  return true;
               }
               break;

            default:
               return true;
            }
         }
         break;

      case Alembic::AbcGeom::kMatrixOperation:
         if (op.getHint() == Alembic::AbcGeom::kMayaShearHint && state < 11)
         {
            state = 11;
         }
         else
         {
            return true;
         }
         break;

      default:
         return true;
      }
   }

   // if we encounter a scale or rotate pivot point,
   // it needs to be inverted again otherwise the matrix is complex
   return (scPivot || roPivot);
}

static void ReadComplexXform(const Alembic::AbcGeom::XformSample &sample, MFnTransform &trans)
{
   Alembic::Abc::M44d m = sample.getMatrix();
   MTransformationMatrix tm(MMatrix(m.x));
   trans.set(tm);
}

static void ReadSimpleXform(const Alembic::AbcGeom::XformSample &sample, MFnTransform &trans)
{
   bool scPivot = false;
   bool roPivot = false;

   MSpace::Space space = MSpace::kPreTransform;
   MTransformationMatrix::RotationOrder rotOrder[2];
   rotOrder[0] = MTransformationMatrix::kInvalid;
   rotOrder[1] = MTransformationMatrix::kInvalid;

   size_t numOps = sample.getNumOps();

   for (size_t i = 0; i < numOps; ++i)
   {
      Alembic::AbcGeom::XformOp op = sample[i];

      switch (op.getType())
      {
      case Alembic::AbcGeom::kScaleOperation:
         {
            double scale[3];

            scale[0] = op.getChannelValue(0);
            scale[1] = op.getChannelValue(1);
            scale[2] = op.getChannelValue(2);

            trans.setScale(scale);
         }
         break;

      case Alembic::AbcGeom::kRotateXOperation:
      case Alembic::AbcGeom::kRotateYOperation:
      case Alembic::AbcGeom::kRotateZOperation:
      case Alembic::AbcGeom::kRotateOperation:
         {
            Alembic::Abc::V3d axis = op.getAxis();
            double x = axis.x;
            double y = axis.y;
            double z = axis.z;
            double angle = 0.0;

            MEulerRotation rot;
            trans.getRotation(rot);

            if (op.getHint() == Alembic::AbcGeom::kRotateHint)
            {
               if (x == 1 && y == 0 && z == 0)
               {
                  // "rotateX"
                  rot.x = Alembic::AbcGeom::DegreesToRadians(op.getAngle());

                  // we have encountered the first rotation, set it
                  // to the 2 X possibilities
                  if (rotOrder[0] == MTransformationMatrix::kInvalid)
                  {
                     rotOrder[0] = MTransformationMatrix::kYZX;
                     rotOrder[1] = MTransformationMatrix::kZYX;
                  }
                  // we have filled in the two possibilities, now choose
                  // which one we should use
                  else if (rotOrder[1] != MTransformationMatrix::kInvalid)
                  {
                     if (rotOrder[1] == MTransformationMatrix::kYXZ)
                     {
                        rotOrder[0] = rotOrder[1];
                     }

                     rotOrder[1] = MTransformationMatrix::kInvalid;
                  }
               }
               else if (x == 0 && y == 1 && z == 0)
               {
                  // "rotateY"
                  rot.y = Alembic::AbcGeom::DegreesToRadians(op.getAngle());

                  // we have encountered the first rotation, set it
                  // to the 2 X possibilities
                  if (rotOrder[0] == MTransformationMatrix::kInvalid)
                  {
                     rotOrder[0] = MTransformationMatrix::kZXY;
                     rotOrder[1] = MTransformationMatrix::kXZY;
                  }
                  // we have filled in the two possibilities, now choose
                  // which one we should use
                  else if (rotOrder[1] != MTransformationMatrix::kInvalid)
                  {
                     if (rotOrder[1] == MTransformationMatrix::kZYX)
                     {
                        rotOrder[0] = rotOrder[1];
                     }

                     rotOrder[1] = MTransformationMatrix::kInvalid;
                  }
               }
               else if (x == 0 && y == 0 && z == 1)
               {
                  // "rotateZ"
                  rot.z = Alembic::AbcGeom::DegreesToRadians(op.getAngle());

                  // we have encountered the first rotation, set it
                  // to the 2 X possibilities
                  if (rotOrder[0] == MTransformationMatrix::kInvalid)
                  {
                     rotOrder[0] = MTransformationMatrix::kXYZ;
                     rotOrder[1] = MTransformationMatrix::kYXZ;
                  }
                  // we have filled in the two possibilities, now choose
                  // which one we should use
                  else if (rotOrder[1] != MTransformationMatrix::kInvalid)
                  {
                     if (rotOrder[1] == MTransformationMatrix::kXZY)
                     {
                        rotOrder[0] = rotOrder[1];
                     }

                     rotOrder[1] = MTransformationMatrix::kInvalid;
                  }
               }
               trans.setRotation(rot);
            }
            // kRotateOrientationHint
            else
            {
               MQuaternion quat;

               if (x == 1 && y == 0 && z == 0)
               {
                  // "rotateAxisX"
                  quat.setToXAxis(Alembic::AbcGeom::DegreesToRadians(op.getAngle()));
               }
               else if (x == 0 && y == 1 && z == 0)
               {
                  // "rotateAxisY"
                  quat.setToYAxis(Alembic::AbcGeom::DegreesToRadians(op.getAngle()));
               }
               else if (x == 0 && y == 0 && z == 1)
               {
                  // "rotateAxisZ"
                  quat.setToZAxis(Alembic::AbcGeom::DegreesToRadians(op.getAngle()));
               }

               MQuaternion curq = trans.rotateOrientation(space);
               trans.setRotateOrientation(curq*quat, space, false);
            }
         }
         break;

      // shear
      case Alembic::AbcGeom::kMatrixOperation:
         {
            double shear[3];

            shear[0] = op.getChannelValue(4);
            shear[1] = op.getChannelValue(8);
            shear[2] = op.getChannelValue(9);

            trans.setShear(shear);
         }
         break;

      case Alembic::AbcGeom::kTranslateOperation:
         {
            Alembic::Util::uint8_t hint = op.getHint();

            switch (hint)
            {
            case Alembic::AbcGeom::kTranslateHint:
               {
                  MVector vec;

                  vec.x = op.getChannelValue(0);
                  vec.y = op.getChannelValue(1);
                  vec.z = op.getChannelValue(2);

                  trans.setTranslation(vec, space);
               }
               break;

            case Alembic::AbcGeom::kScalePivotPointHint:
               {
                  MPoint point;
                  point.w = 1.0;

                  point.x = op.getChannelValue(0);
                  point.y = op.getChannelValue(1);
                  point.z = op.getChannelValue(2);

                  // we only want to apply this to the first one
                  // the second one is the inverse
                  if (!scPivot)
                  {
                     trans.setScalePivot(point, space, false);
                  }

                  scPivot = !scPivot;
               }
               break;

            case Alembic::AbcGeom::kScalePivotTranslationHint:
               {
                  MVector vec;

                  vec.x = op.getChannelValue(0);
                  vec.y = op.getChannelValue(1);
                  vec.z = op.getChannelValue(2);

                  trans.setScalePivotTranslation(vec, space);
               }
               break;

            case Alembic::AbcGeom::kRotatePivotPointHint:
               {
                  MPoint point;
                  point.w = 1.0;

                  point.x = op.getChannelValue(0);
                  point.y = op.getChannelValue(1);
                  point.z = op.getChannelValue(2);

                  // only set rotate pivot on the first one, the second
                  // one is just the inverse
                  if (!roPivot)
                  {
                     trans.setRotatePivot(point, space, false);
                  }

                  roPivot = !roPivot;
               }
               break;

            case Alembic::AbcGeom::kRotatePivotTranslationHint:
               {
                  MVector vec;

                  vec.x = op.getChannelValue(0);
                  vec.y = op.getChannelValue(1);
                  vec.z = op.getChannelValue(2);

                  trans.setRotatePivotTranslation(vec, space);
               }
               break;

            default:
               break;
            }
         }
         break;

      default:
         break;
      }
   }

   // no rotates were found so default to XYZ
   if (rotOrder[0] == MTransformationMatrix::kInvalid)
   {
      rotOrder[0] =  MTransformationMatrix::kXYZ;
   }

   trans.setRotationOrder(rotOrder[0], false);
}

void ReadXform(const Alembic::AbcGeom::XformSample &sample, MFnTransform &trans, bool reset)
{
   if (reset)
   {
      ResetXform(trans);
   }

   if (IsComplexXform(sample))
   {
      ReadComplexXform(sample, trans);
   }
   else
   {
      ReadSimpleXform(sample, trans);
   }
}
