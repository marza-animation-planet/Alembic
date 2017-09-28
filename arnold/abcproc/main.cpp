#include <ai.h>
#include <dso.h>
#include <visitors.h>
#include <globallock.h>


// ---

AI_PROCEDURAL_NODE_EXPORT_METHODS(AbcProcMtd);


node_parameters
{
   // Common parameters
   AiParameterStr(Dso::p_filename, "");
   AiParameterStr(Dso::p_objectpath, "");

   AiParameterFlt(Dso::p_frame, 0.0f);
   AiParameterArray(Dso::p_samples, AiArray(0, 1, AI_TYPE_FLOAT));
   AiParameterBool(Dso::p_relative_samples, false);
   AiParameterFlt(Dso::p_fps, 0.0f);
   AiParameterEnum(Dso::p_cycle, CT_hold, CycleTypeNames);
   AiParameterFlt(Dso::p_start_frame, std::numeric_limits<float>::max());
   AiParameterFlt(Dso::p_end_frame, -std::numeric_limits<float>::max());
   AiParameterFlt(Dso::p_speed, 1.0f);
   AiParameterFlt(Dso::p_offset, 0.0f);
   AiParameterBool(Dso::p_preserve_start_frame, false);

   //AiParameterBool(Dso::p_ignore_motion_blur, false);
   AiParameterBool(Dso::p_ignore_deform_blur, false);
   AiParameterBool(Dso::p_ignore_transform_blur, false);
   AiParameterBool(Dso::p_ignore_visibility, false);
   AiParameterBool(Dso::p_ignore_transforms, false);
   AiParameterBool(Dso::p_ignore_instances, false);
   AiParameterBool(Dso::p_ignore_nurbs, false);

   AiParameterFlt(Dso::p_velocity_scale, 1.0f);
   AiParameterStr(Dso::p_velocity_name, "");
   AiParameterStr(Dso::p_acceleration_name, "");
   AiParameterBool(Dso::p_force_velocity_blur, false)

   AiParameterBool(Dso::p_output_reference, false);
   AiParameterEnum(Dso::p_reference_source, RS_attributes_then_file, ReferenceSourceNames);
   AiParameterStr(Dso::p_reference_position_name, "Pref");
   AiParameterStr(Dso::p_reference_normal_name, "Nref");
   AiParameterStr(Dso::p_reference_filename, "");
   AiParameterFlt(Dso::p_reference_frame, -std::numeric_limits<float>::max());

   AiParameterArray(Dso::p_demote_to_object_attribute, AiArray(0, 1, AI_TYPE_STRING));

   AiParameterInt(Dso::p_samples_expand_iterations, 0);
   AiParameterBool(Dso::p_optimize_samples, false);

   AiParameterStr(Dso::p_nameprefix, "");

   // Multi shapes parameters
   AiParameterFlt(Dso::p_bounds_padding, 0.0f);
   AiParameterBool(Dso::p_compute_velocity_expanded_bounds, false);
   AiParameterBool(Dso::p_use_override_bounds, false);
   AiParameterStr(Dso::p_override_bounds_min_name, "overrideBoundsMin");
   AiParameterStr(Dso::p_override_bounds_max_name, "overrideBoundsMax");
   AiParameterBool(Dso::p_pad_bounds_with_peak_radius, false);
   AiParameterStr(Dso::p_peak_radius_name, "peakRadius");
   AiParameterBool(Dso::p_pad_bounds_with_peak_width, false);
   AiParameterStr(Dso::p_peak_width_name, "peakWidth");
   AiParameterArray(Dso::p_override_attributes, AiArray(0, 1, AI_TYPE_STRING));

   // Single shape parameters
   AiParameterBool(Dso::p_read_object_attributes, false);
   AiParameterBool(Dso::p_read_primitive_attributes, false);
   AiParameterBool(Dso::p_read_point_attributes, false);
   AiParameterBool(Dso::p_read_vertex_attributes, false);
   AiParameterEnum(Dso::p_attributes_frame, AF_render, AttributeFrameNames);
   AiParameterArray(Dso::p_attribute_prefices_to_remove, AiArray(0, 1, AI_TYPE_STRING));
   AiParameterArray(Dso::p_compute_tangents, AiArray(0, 1, AI_TYPE_STRING));
   AiParameterStr(Dso::p_radius_name, "");
   AiParameterFlt(Dso::p_radius_min, 0.0f);
   AiParameterFlt(Dso::p_radius_max, 1000000.0f);
   AiParameterFlt(Dso::p_radius_scale, 1.0f);
   AiParameterFlt(Dso::p_width_min, 0.0f);
   AiParameterFlt(Dso::p_width_max, 1000000.0f);
   AiParameterFlt(Dso::p_width_scale, 1.0f);
   AiParameterInt(Dso::p_nurbs_sample_rate, 5);

   // Others
   AiParameterBool(Dso::p_verbose, false);
   AiParameterStr(Dso::p_rootdrive, "");
}

procedural_init
{
   AiMsgDebug("[abcproc] ProcInit [thread %p]", AiThreadSelf());
   Dso *dso = new Dso(node);
   *user_ptr = dso;
   return 1;
}

procedural_num_nodes
{
   AiMsgDebug("[abcproc] ProcNumNodes [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   
   return int(dso->numShapes());
}

procedural_get_node
{
   // This function won't get call for the same procedural node from different threads
   
   AiMsgDebug("[abcproc] ProcGetNode [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   
   if (i == 0)
   {
      // generate the shape(s)
      if (dso->mode() == PM_multi)
      {
         // only generates new procedural nodes (read transform and bound information)
         MakeProcedurals procNodes(dso);
         dso->scene()->visit(AlembicNode::VisitDepthFirst, procNodes);
         
         if (procNodes.numNodes() != dso->numShapes())
         {
            AiMsgWarning("[abcproc] %lu procedural(s) generated (%lu expected)", procNodes.numNodes(), dso->numShapes());
         }
         
         CollectWorldMatrices refMatrices(dso);
         if (dso->referenceScene() && !dso->ignoreTransforms())
         {
            dso->referenceScene()->visit(AlembicNode::VisitDepthFirst, refMatrices);
         }
         
         for (size_t i=0; i<dso->numShapes(); ++i)
         {
            AtNode *node = procNodes.node(i);
            Alembic::Abc::M44d W;
            AtMatrix mtx;
            
            if (node && refMatrices.getWorldMatrix(procNodes.path(i), W))
            {
               for (int r=0; r<4; ++r)
               {
                  for (int c=0; c<4; ++c)
                  {
                     mtx[r][c] = W[r][c];
                  }
               }
               
               // Store reference object world matrix to avoid having to re-compute it later
               AiNodeDeclare(node, "Mref", "constant MATRIX");
               AiNodeSetMatrix(node, "Mref", mtx);
            }
            
            dso->setGeneratedNode(i, node);
         }
      }
      else
      {
         AtNode *output = 0;
         std::string masterNodeName;
         
         AbcProcGlobalLock::Acquire();
         bool isInstance = dso->isInstance(&masterNodeName);
         AbcProcGlobalLock::Release();
         
         // Keep track of procedural node name
         const char *procName = AiNodeGetName(dso->procNode());
         
         if (!isInstance)
         {
            MakeShape visitor(dso);
            // dso->scene()->visit(AlembicNode::VisitFilteredFlat, visitor);
            dso->scene()->visit(AlembicNode::VisitDepthFirst, visitor);
            
            output = visitor.node();
            
            if (output)
            {
               dso->transferUserParams(output);
               
               AbcProcGlobalLock::Acquire();
               
               if (dso->isInstance(&masterNodeName))
               {
                  std::string name = AiNodeGetName(output);
                  
                  AiMsgWarning("[abcproc] Master node '%s' created in another thread. Ignore %s node '%s'.",
                               masterNodeName.c_str(),
                               AiNodeEntryGetName(AiNodeGetNodeEntry(output)),
                               name.c_str());
                  
                  // reset name to avoid clashes when creating instance
                  name += "_disabled";
                  AiNodeSetStr(output, Strings::name, dso->uniqueName(name).c_str());
                  AiNodeSetByte(output, Strings::visibility, 0);
                  AiNodeSetDisabled(output, true);
                  
                  isInstance = true;
               }
               else
               {
                  dso->setMasterNodeName(AiNodeGetName(output));
               }
               
               AbcProcGlobalLock::Release();
            }
         }
         
         if (isInstance)
         {
            AtNode *master = AiNodeLookUpByName(masterNodeName.c_str());
            
            if (master)
            {
               if (dso->verbose())
               {
                  AiMsgInfo("[abcproc] Create a new instance of \"%s\"", AiNodeGetName(master));
               }
               
               AbcProcGlobalLock::Acquire();
               
               // rename source procedural node if needed
               if (!strcmp(AiNodeGetName(dso->procNode()), procName))
               {
                  // procedural node hasn't been renamed yet
                  std::string name = "_";
                  name += procName;
                  AiNodeSetStr(dso->procNode(), Strings::name, dso->uniqueName(name).c_str());
               }
               // use procedural name for newly generated instance
               output = AiNode(Strings::ginstance, procName, dso->procNode());
               
               AbcProcGlobalLock::Release();
               
               AiNodeSetBool(output, Strings::inherit_xform, false);
               AiNodeSetPtr(output, Strings::node, master);
               
               // It seems that when ginstance node don't  inherit from the procedural
               //   attributes like standard shapes
               dso->transferInstanceParams(output);
               
               dso->transferUserParams(output);
            }
            else
            {
               AiMsgWarning("[abcproc] Master node '%s' not yet expanded. Ignore instance.", masterNodeName.c_str());
            }
         }
         
         dso->setGeneratedNode(0, output);
      }
   }
   
   return dso->generatedNode(i);
}

procedural_cleanup
{
   AiMsgDebug("[abcproc] ProcCleanup [thread %p]", AiThreadSelf());
   Dso *dso = (Dso*) user_ptr;
   delete dso;
   return 1;
}

// ---

#ifdef ALEMBIC_WITH_HDF5

#include <H5public.h>

#ifdef H5_HAVE_THREADSAFE

#ifdef _WIN32

// ToDo

#else // _WIN32

#include <pthread.h>

static void __attribute__((constructor)) on_dlopen(void)
{
   H5dont_atexit();
}

static void __attribute__((destructor)) on_dlclose(void)
{
   // Hack around thread termination segfault
   // -> alembic procedural is unloaded before thread finishes
   //    resulting on HDF5 keys to leak
   
   extern pthread_key_t H5TS_errstk_key_g;
   extern pthread_key_t H5TS_funcstk_key_g;
   extern pthread_key_t H5TS_cancel_key_g;
   
   pthread_key_delete(H5TS_cancel_key_g);
   pthread_key_delete(H5TS_funcstk_key_g);
   pthread_key_delete(H5TS_errstk_key_g);
}

#endif // _WIN32

#endif // H5_HAVE_THREADSAFE

#endif // ALEMBIC_WITH_HDF5


node_loader
{
  if (i > 0)
  {
    return false;
  }

  node->methods = AbcProcMtd;
  node->output_type = AI_TYPE_NONE;
  node->name = "abcproc";
  node->node_type = AI_NODE_SHAPE_PROCEDURAL;
  strcpy(node->version, AI_VERSION);

  return true;
}
