#include <ai.h>
#include <dso.h>
#include <visitors.h>
#include <globallock.h>


// ---

AI_PROCEDURAL_NODE_EXPORT_METHODS(AbcProcMtd);


node_parameters
{
   // Common parameters
   AiParameterStr(Strings::filename, Strings::_empty);
   AiParameterStr(Strings::objectpath, Strings::_empty);
   AiParameterStr(Strings::nameprefix, Strings::_empty);

   // use node's own 'motion_start' and 'motion_end' parameters
   AiParameterFlt(Strings::frame, 0.0f);
   AiParameterFlt(Strings::fps, 0.0f);
   AiParameterEnum(Strings::cycle, CT_hold, CycleTypeNames);
   AiParameterFlt(Strings::start_frame, std::numeric_limits<float>::max());
   AiParameterFlt(Strings::end_frame, -std::numeric_limits<float>::max());
   AiParameterFlt(Strings::speed, 1.0f);
   AiParameterFlt(Strings::offset, 0.0f);
   AiParameterBool(Strings::preserve_start_frame, false);

   AiParameterInt(Strings::samples, 1);
   AiParameterInt(Strings::expand_samples_iterations, 0);
   AiParameterBool(Strings::optimize_samples, false);

   AiParameterBool(Strings::ignore_deform_blur, false);
   AiParameterBool(Strings::ignore_transform_blur, false);
   AiParameterBool(Strings::ignore_visibility, false);
   AiParameterBool(Strings::ignore_transforms, false);
   AiParameterBool(Strings::ignore_instances, false);
   AiParameterBool(Strings::ignore_nurbs, false);

   AiParameterFlt(Strings::velocity_scale, 1.0f);
   AiParameterStr(Strings::velocity_name, Strings::_empty);
   AiParameterStr(Strings::acceleration_name, Strings::_empty);
   AiParameterBool(Strings::force_velocity_blur, false)

   AiParameterBool(Strings::output_reference, false);
   AiParameterEnum(Strings::reference_source, RS_attributes_then_file, ReferenceSourceNames);
   AiParameterStr(Strings::reference_position_name, Strings::Pref);
   AiParameterStr(Strings::reference_normal_name, Strings::Nref);
   AiParameterStr(Strings::reference_filename, Strings::_empty);
   AiParameterFlt(Strings::reference_frame, -std::numeric_limits<float>::max());

   AiParameterEnum(Strings::attributes_evaluation_time, AET_render, AttributesEvaluationTimeNames);
   // Only applies to attributes read from the .abc files, not attributes already existing on arnold node
   AiParameterArray(Strings::remove_attribute_prefices, AiArray(0, 1, AI_TYPE_STRING));
   // Names in 'ignore_attributes' and 'force_constant_attributes' are without any prefix to remove
   AiParameterArray(Strings::ignore_attributes, AiArray(0, 1, AI_TYPE_STRING));
   // This only applies to GeoParams contained in alembic file
   AiParameterArray(Strings::force_constant_attributes, AiArray(0, 1, AI_TYPE_STRING));

   AiParameterArray(Strings::compute_tangents_for_uvs, AiArray(0, 1, AI_TYPE_STRING));

   AiParameterStr(Strings::radius_name, Strings::_empty);
   AiParameterFlt(Strings::radius_min, 0.0f);
   AiParameterFlt(Strings::radius_max, 1000000.0f);
   AiParameterFlt(Strings::radius_scale, 1.0f);

   AiParameterFlt(Strings::width_min, 0.0f);
   AiParameterFlt(Strings::width_max, 1000000.0f);
   AiParameterFlt(Strings::width_scale, 1.0f);

   AiParameterInt(Strings::nurbs_sample_rate, 5);

   // Others
   AiParameterBool(Strings::verbose, false);
   AiParameterStr(Strings::rootdrive, Strings::_empty);

   AiMetaDataSetBool(nentry, Strings::filename, Strings::filepath, true);
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
