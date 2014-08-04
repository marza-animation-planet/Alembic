#include "VP2.h"
#include "AbcShape.h"
#include "MathUtils.h"
#include "AlembicSceneVisitors.h"

#include <maya/MHWGeometryUtilities.h>
#include <maya/MStateManager.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MAnimControl.h>

class AbcShapeDrawData : public MUserData
{
public:
   
   AbcShapeDrawData()
      : MUserData(false), shape(0), selected(false), time(0), ignoreCulling(false)
   {
   }

   virtual ~AbcShapeDrawData()
   {
   }

   void draw(const MHWRender::MDrawContext& context) const
   {
      MStatus status;
      
      MHWRender::MStateManager* stateMgr = context.getStateManager();

      if (!stateMgr || !shape)
      {
         return;
      }

      const MMatrix view = context.getMatrix(MHWRender::MDrawContext::kWorldViewMtx, &status);
      if (status != MStatus::kSuccess)
      {
         return;
      }

      const MMatrix proj = context.getMatrix(MHWRender::MDrawContext::kProjectionMtx, &status);
      if (status != MStatus::kSuccess)
      {
         return;
      }
      
      const MMatrix tmp = context.getMatrix(MHWRender::MDrawContext::kWorldViewProjInverseMtx, &status);
      if (status != MStatus::kSuccess)
      {
         return;
      }
      
      M44d projViewInv;
      tmp.get(projViewInv.x);
      Frustum frustum(projViewInv);
      
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadMatrixd(proj.matrix[0]);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadMatrixd(view.matrix[0]);
      
      int style = context.getDisplayStyle();
      
      DrawGeometry visitor(shape->sceneGeometry(),
                           shape->ignoreTransforms(),
                           shape->ignoreInstances(),
                           shape->ignoreVisibility());
      visitor.setPointWidth(shape->pointWidth());
      visitor.setLineWidth(shape->lineWidth());
      if (!ignoreCulling)
      {
         visitor.doCull(frustum);
      }

      glPushAttrib(GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_POLYGON_BIT);
      
      // No MHWRender::MDrawContext::kBoundingBox?
      
      if (shape->displayMode() == AbcShape::DM_box ||
          shape->displayMode() == AbcShape::DM_boxes)
      {
         glDisable(GL_LIGHTING);
         
         glColor3d(color.r, color.g, color.b);
         
         if (shape->displayMode() == AbcShape::DM_box)
         {
            DrawBox(shape->scene()->selfBounds(), false, shape->lineWidth());
         }
         else
         {
            visitor.drawBounds(true);
            
            shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
         }
      }
      else if (shape->displayMode() == AbcShape::DM_points)
      {
         glDisable(GL_LIGHTING);
         
         glColor3d(color.r, color.g, color.b);
         
         visitor.drawBounds(false);
         visitor.drawAsPoints(true);
         
         shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
      }
      else
      {
         static const MHWRender::MRasterizerState *rasterState = 0;
         
         bool drawWireframe = ((style & MHWRender::MDrawContext::kWireFrame) != 0 || selected);
         
         visitor.drawBounds(false);
         
         if ((style & MHWRender::MDrawContext::kGouraudShaded) != 0)
         {
            const MHWRender::MRasterizerState* currentRasterState = 0;
            
            visitor.drawWireframe(false);
            
            if (drawWireframe)
            {
               currentRasterState = stateMgr->getRasterizerState();
               
               if (!rasterState)
               {
                  MHWRender::MRasterizerStateDesc desc(currentRasterState->desc());
                  
                  desc.depthBiasIsFloat = true;
                  desc.depthBias = 1.0f;
                  desc.slopeScaledDepthBias = 1.0f;
                  
                  rasterState = stateMgr->acquireRasterizerState(desc);
               }
               
               stateMgr->setRasterizerState(rasterState);
            }
            
            if (!setupLights(context))
            {
               glColor3d(0, 0, 0);
            }
            
            shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
            
            if (drawWireframe)
            {
               stateMgr->setRasterizerState(currentRasterState);
            }
         }
         
         if (drawWireframe)
         {
            visitor.drawWireframe(true);
            
            glDisable(GL_LIGHTING);
            
            glColor3d(color.r, color.g, color.b);
            
            shape->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
         }
      }
      
      glPopAttrib();
      
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
   }
   
   bool setupLights(const MHWRender::MDrawContext& context) const
   {
      MStatus status;

      unsigned int nlights = std::min<unsigned int>(GL_MAX_LIGHTS, context.numberOfActiveLights(&status));
      if (status != MStatus::kSuccess)
      {
         return false;
      }
      
      for (unsigned int i=0; i<GL_MAX_LIGHTS; ++i)
      {
         glDisable(GL_LIGHT0+i);
      }
      
      if (nlights > 0)
      {
         GLfloat black[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
         GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f };
         
         GLfloat *ldiff = 0;
         GLfloat *lamb = 0;
         GLfloat col[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
         GLfloat pos[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
         GLfloat dir[3] = { 0.0f, 0.0f, 0.0f };
         float cutoff = 180.0f;
         float exponent = 0.0f;
         
         // Lights are specified in world space and needs to be converted to view space.
         
         const MMatrix worldToView = context.getMatrix(MHWRender::MDrawContext::kViewMtx, &status);
         if (status != MStatus::kSuccess)
         {
            return false;
         }
         
         glMatrixMode(GL_MODELVIEW);
         glPushMatrix();
         glLoadMatrixd(worldToView.matrix[0]);

         glEnable(GL_LIGHTING);
         glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
         glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

         for (unsigned int i=0; i<nlights; ++i)
         {
            MFloatPoint position;
            MFloatVector direction;
            float intensity;
            MColor color;
            bool hasDirection;
            bool hasPosition;
            
            status = context.getLightInformation(i, position, direction, intensity, color, hasDirection, hasPosition);
            
            col[0] = intensity * color[0];
            col[1] = intensity * color[1];
            col[2] = intensity * color[2];
            pos[0] = position[0];
            pos[1] = position[1];
            pos[2] = position[2];
            dir[0] = direction[0];
            dir[1] = direction[1];
            dir[2] = direction[2];
            cutoff = 180.0f;
            exponent = 0.0f;
            ldiff = col;
            lamb = black;
            
            if (status != MStatus::kSuccess)
            {
               return false;
            }

            if (hasDirection)
            {
               if (hasPosition)
               {
                  // Spot light 
                  cutoff = 20.0f;
                  
                  MHWRender::MLightParameterInformation *lpi = context.getLightParameterInformation(i);
                  
                  if (lpi)
                  {
                     MStringArray params;
                     MFloatArray vals;
                     
                     lpi->parameterList(params);
                     
                     for (unsigned int p=0; p<params.length(); ++p)
                     {
                        MHWRender::MLightParameterInformation::StockParameterSemantic semantic = lpi->parameterSemantic(params[p]);
                        MHWRender::MLightParameterInformation::ParameterType type = lpi->parameterType(params[p]);
                        
                        if (semantic == MHWRender::MLightParameterInformation::kCosConeAngle &&
                            type == MHWRender::MLightParameterInformation::kFloat)
                        {
                           lpi->getParameter(params[p], vals);
                           cutoff = acosf(vals[0]);
                        }
                        else if (semantic == MHWRender::MLightParameterInformation::kDropoff &&
                                 type == MHWRender::MLightParameterInformation::kFloat)
                        {
                           lpi->getParameter(params[p], vals);
                           exponent = vals[0];
                        }
                     }
                  }
               }
               else
               {
                  // Directional light
                  pos[0] = -dir[0];
                  pos[1] = -dir[1];
                  pos[2] = -dir[2];
                  pos[3] = 0.0f;
                  
                  dir[0] = 0.0f;
                  dir[1] = 0.0f;
                  dir[2] = 0.0f;
               }
            }
            else if (hasPosition)
            {
               // Point light
               dir[0] = 0.0f;
               dir[1] = 0.0f;
               dir[2] = 0.0f;
            }
            else if (!hasPosition)
            {
               // Ambient light
               pos[0] = 0.0f;
               pos[1] = 0.0f;
               pos[2] = 0.0f;
               
               ldiff = black;
               lamb = col;
            }
            
            glLightfv(GL_LIGHT0+i, GL_AMBIENT, lamb);
            glLightfv(GL_LIGHT0+i, GL_DIFFUSE, ldiff);
            glLightfv(GL_LIGHT0+i, GL_POSITION, pos);
            glLightfv(GL_LIGHT0+i, GL_SPOT_DIRECTION, dir);
            glLightf(GL_LIGHT0+i, GL_SPOT_EXPONENT, exponent);
            glLightf(GL_LIGHT0+i, GL_SPOT_CUTOFF, cutoff);
            
            glEnable(GL_LIGHT0+i);
         }
         
         glPopMatrix();
         
         return true;
      }
      else
      {
         glDisable(GL_LIGHTING);
         return false;
      }
   }

   AbcShape *shape;
   MColor color;
   bool selected;
   double time; // current time
   bool ignoreCulling;
};

// ---

MHWRender::MPxDrawOverride* AbcShapeOverride::create(const MObject &obj)
{
   return new AbcShapeOverride(obj);
}

void AbcShapeOverride::draw(const MHWRender::MDrawContext& context, const MUserData* data)
{
   const AbcShapeDrawData *drawData = dynamic_cast<const AbcShapeDrawData*>(data);
   
   if (drawData)
   {
      drawData->draw(context);
   }
}

// ---

MString AbcShapeOverride::Classification = "drawdb/geometry/AbcShape";
MString AbcShapeOverride::Registrant = "AbcShapeDrawOverride";

AbcShapeOverride::AbcShapeOverride(const MObject &obj)
   : MHWRender::MPxDrawOverride(obj, AbcShapeOverride::draw)
{
}

AbcShapeOverride::~AbcShapeOverride()
{
}

MHWRender::DrawAPI AbcShapeOverride::supportedDrawAPIs() const
{
   return MHWRender::kOpenGL;
}

bool AbcShapeOverride::isBounded(const MDagPath &, const MDagPath &) const
{
   return false;
}

MBoundingBox AbcShapeOverride::boundingBox(const MDagPath &, const MDagPath &) const
{
   return MBoundingBox();
}

MUserData* AbcShapeOverride::prepareForDraw(const MDagPath &objPath, const MDagPath &, MUserData *oldData)
{
   MStatus status;
   MFnDependencyNode node(objPath.node(), &status);
   
   if (!status)
   {
      return NULL;
   }
   
   AbcShape* abcShape = dynamic_cast<AbcShape*>(node.userNode());
   
   if (!abcShape)
   {
      return NULL;
   }
   
   AbcShapeDrawData* data = dynamic_cast<AbcShapeDrawData*>(oldData);
   if (!data)
   {
      data = new AbcShapeDrawData();
   }
   
   data->shape = abcShape;
   data->color = MHWRender::MGeometryUtilities::wireframeColor(objPath);
   data->time =  MAnimControl::currentTime().as(MTime::kSeconds);
   data->ignoreCulling = abcShape->ignoreCulling();
   
   switch (MHWRender::MGeometryUtilities::displayStatus(objPath))
   {
   case MHWRender::kActive:
   case MHWRender::kLead:
   case MHWRender::kHilite:
      data->selected = true;
      break;
   default:
      data->selected = false;
   }
   
   abcShape->updateInternals();
   
   return data;
}
