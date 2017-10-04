#ifndef ABCPROC_STRINGS_H_
#define ABCPROC_STRINGS_H_

#include <ai.h>

namespace Strings
{
   extern AtString filename;
   extern AtString objectpath;
   extern AtString nameprefix;

   extern AtString frame;
   extern AtString fps;
   extern AtString cycle;
   extern AtString start_frame;
   extern AtString end_frame;
   extern AtString speed;
   extern AtString offset;
   extern AtString preserve_start_frame;

   extern AtString samples;
   extern AtString expand_samples_iterations;
   extern AtString optimize_samples;

   //extern AtString ignore_motion_blur;
   extern AtString ignore_deform_blur;
   extern AtString ignore_transform_blur;
   extern AtString ignore_visibility;
   extern AtString ignore_transforms;
   extern AtString ignore_instances;
   extern AtString ignore_nurbs;

   extern AtString velocity_scale;
   extern AtString velocity_name;
   extern AtString acceleration_name;
   extern AtString force_velocity_blur;
   
   extern AtString ignore_reference;
   extern AtString reference_source;
   extern AtString reference_position_name;
   extern AtString reference_normal_name;
   extern AtString reference_frame;

   extern AtString force_constant_attributes;
   extern AtString ignore_attributes;
   extern AtString attributes_evaluation_time;
   extern AtString remove_attribute_prefices;
   
   extern AtString compute_tangents_for_uvs;
   
   extern AtString radius_name;
   extern AtString radius_min;
   extern AtString radius_max;
   extern AtString radius_scale;
   
   extern AtString width_min;
   extern AtString width_max;
   extern AtString width_scale;
   extern AtString nurbs_sample_rate;

   extern AtString verbose;
   extern AtString rootdrive;

   extern AtString id;
   extern AtString receive_shadows;
   extern AtString self_shadows;
   extern AtString invert_normals;
   extern AtString opaque;
   extern AtString use_light_group;
   extern AtString use_shadow_group;
   extern AtString ray_bias;
   extern AtString visibility;
   extern AtString sidedness;
   extern AtString shader;
   extern AtString light_group;
   extern AtString shadow_group;
   extern AtString trace_sets;
   extern AtString motion_start;
   extern AtString motion_end;
   extern AtString matte;
   extern AtString transform_type;
   extern AtString volume_padding;
   extern AtString step_size;
   extern AtString instance_num;
   extern AtString bounds_slack;
   extern AtString name;
   extern AtString matrix;
   extern AtString box;
   extern AtString ginstance;
   extern AtString points;
   extern AtString curves;
   extern AtString polymesh;
   extern AtString basis;
   extern AtString num_points;
   extern AtString mode;
   extern AtString node;
   extern AtString inherit_xform;
   extern AtString Pref;
   extern AtString Nref;
   extern AtString _empty;
   extern AtString filepath;
}

#endif
