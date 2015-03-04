#include "params.h"
#include <cstring>

// ---

tchar* DuplicateString(const tchar *buf)
{
   if (buf)
   {
      size_t len = strlen(buf) + 1;
      tchar *str = new tchar[len];
      vutils_strcpy_n(str, buf, len);
      return str;
   }
   else
   {
      return NULL;
   }
}

// ---

ParamBase::ParamBase(const tchar *paramName, bool ownName)
   : VR::VRayPluginParameter()
   , mOwnName(ownName)
{
   mName = (mOwnName ? DuplicateString(paramName) : paramName);
}

ParamBase::ParamBase(const ParamBase &other)
   : VR::VRayPluginParameter(other)
   , mOwnName(other.mOwnName)
{
   mName = (mOwnName ? DuplicateString(other.mName) : other.mName);
}

ParamBase::~ParamBase()
{
   if (mOwnName)
   {
      delete[] mName;
   }
}

const tchar* ParamBase::getName(void)
{
   return mName;
}

// ---

ListParamBase::ListParamBase(const tchar *paramName, bool ownName)
   : ParamBase(paramName, ownName)
   , mListLevel(0)
{
}

ListParamBase::ListParamBase(const ListParamBase &other)
   : ParamBase(other)
   , mListLevel(other.mListLevel)
{
}

ListParamBase::~ListParamBase()
{
}

VR::ListHandle ListParamBase::openList(int i)
{
   mListLevel++;
   return (VR::ListHandle) NULL;
}

void ListParamBase::closeList(VR::ListHandle list)
{
   mListLevel--;
}

// ---

VectorListParam::VectorListParam(const tchar *paramName, bool ownName)
   : ListParamT<VR::Vector>(paramName, ownName)
{
}

VectorListParam::VectorListParam(const VectorListParam &rhs)
   : ListParamT<VR::Vector>(rhs)
{
}

VectorListParam::~VectorListParam()
{
}

PluginBase* VectorListParam::getPlugin(void)
{
   return static_cast<PluginBase*>(this);
}

VR::VRayParameterType VectorListParam::getType(int index, double time)
{
   return VR::paramtype_vector;
}

VR::Vector VectorListParam::getVector(int index, double time)
{
   return getValue(index, time);
}

VR::VectorList VectorListParam::getVectorList(double time)
{
   return getValueList(time);
}

void VectorListParam::setVector(const VR::Vector &value, int index, double time)
{
   setValue(value, index, time);
}

void VectorListParam::setVectorList(const VR::VectorRefList &value, double time)
{
   setValueList(value, time);
}

VR::VRayPluginParameter* VectorListParam::clone()
{
   return new VectorListParam(*this);
}

// ---

ColorListParam::ColorListParam(const tchar *paramName, bool ownName)
   : ListParamT<VR::Color>(paramName, ownName)
{
}

ColorListParam::ColorListParam(const ColorListParam &rhs)
   : ListParamT<VR::Color>(rhs)
{
}

ColorListParam::~ColorListParam()
{
}

PluginBase* ColorListParam::getPlugin(void)
{
   return static_cast<PluginBase*>(this);
}

VR::VRayParameterType ColorListParam::getType(int index, double time)
{
   return VR::paramtype_color;
}

VR::Color ColorListParam::getColor(int index, double time)
{
   return getValue(index, time);
}

VR::AColor ColorListParam::getAColor(int index, double time)
{
   return VR::AColor(getValue(index, time));
}

VR::ColorList ColorListParam::getColorList(double time)
{
   return getValueList(time);
}

void ColorListParam::setColor(const VR::Color &value, int index, double time)
{
   setValue(value, index, time);
}

void ColorListParam::setAColor(const VR::AColor &value, int index, double time)
{
   setValue(value.color, index, time);
}

void ColorListParam::setColorList(const VR::ColorRefList &value, double time)
{
   setValueList(value, time);
}

VR::VRayPluginParameter* ColorListParam::clone()
{
   return new ColorListParam(*this);
}

// ---

IntListParam::IntListParam(const tchar *paramName, bool ownName)
   : ListParamT<int>(paramName, ownName)
{
}

IntListParam::IntListParam(const IntListParam &rhs)
   : ListParamT<int>(rhs)
{
}

IntListParam::~IntListParam()
{
}

PluginBase* IntListParam::getPlugin(void)
{
   return static_cast<PluginBase*>(this);
}

VR::VRayParameterType IntListParam::getType(int index, double time)
{
   return VR::paramtype_int;
}

int IntListParam::getBool(int index, double time)
{
   return (getValue(index, time) != 0);
}

int IntListParam::getInt(int index, double time)
{
   return getValue(index, time);
}

float IntListParam::getFloat(int index, double time)
{
   return float(getValue(index, time));
}

double IntListParam::getDouble(int index, double time)
{
   return double(getValue(index, time));
}

VR::IntList IntListParam::getIntList(double time)
{
   return getValueList(time);
}

void IntListParam::setBool(int value, int index, double time)
{
   setValue(value, index, time);
}

void IntListParam::setInt(int value, int index, double time)
{
   setValue(value, index, time);
}

void IntListParam::setFloat(float value, int index, double time)
{
   setValue(int(value), index, time);
}

void IntListParam::setDouble(double value, int index, double time)
{
   setValue(int(value), index, time);
}

void IntListParam::setIntList(const VR::IntRefList &value, double time)
{
   setValueList(value, time);
}

VR::VRayPluginParameter* IntListParam::clone()
{
  return new IntListParam(*this);
}

// ---

FloatListParam::FloatListParam(const tchar *paramName, bool ownName)
   : ListParamT<float>(paramName, ownName)
{
}

FloatListParam::FloatListParam(const FloatListParam &rhs)
   : ListParamT<float>(rhs)
{
}

FloatListParam::~FloatListParam()
{
}

PluginBase* FloatListParam::getPlugin(void)
{
   return static_cast<PluginBase*>(this);
}

VR::VRayParameterType FloatListParam::getType(int index, double time)
{
   return VR::paramtype_float;
}

int FloatListParam::getBool(int index, double time)
{
   return (getValue(index, time) != 0.0f);
}

int FloatListParam::getInt(int index, double time)
{
   return int(getValue(index, time));
}

float FloatListParam::getFloat(int index, double time)
{
   return getValue(index, time);
}

double FloatListParam::getDouble(int index, double time)
{
   return double(getValue(index, time));
}

VR::FloatList FloatListParam::getFloatList(double time)
{
   return getValueList(time);
}

void FloatListParam::setBool(int value, int index, double time)
{
   setValue(value != 0 ? 1.0f : 0.0f, index, time);
}

void FloatListParam::setInt(int value, int index, double time)
{
   setValue(float(value), index, time);
}

void FloatListParam::setFloat(float value, int index, double time)
{
   setValue(value, index, time);
}

void FloatListParam::setDouble(double value, int index, double time)
{
   setValue(float(value), index, time);
}

void FloatListParam::setFloatList(const VR::FloatRefList &value, double time)
{
   setValueList(value, time);
}

VR::VRayPluginParameter* FloatListParam::clone()
{
  return new FloatListParam(*this);
}

// ---

AlembicLoaderParams::Cache::Cache()
   : filename("")
   , objectPath("")
   , referenceFilename("")
   , startFrame(0.0f)
   , endFrame(0.0f)
   , speed(1.0f)
   , offset(0.0f)
   , fps(24.0f)
   , preserveStartFrame(false)
   , cycle(0)
   , ignoreTransforms(false)
   , ignoreVisibility(false)
   , ignoreInstances(false)
   , ignoreTransformBlur(false)
   , ignoreDeformBlur(false)
   , verbose(false)
   , subdivEnable(false)
   , subdivUVs(true)
   , preserveMapBorders(-1)
   , staticSubdiv(false)
   , classicCatmark(false)
   , osdSubdivEnable(false)
   , osdSubdivLevel(0)
   , osdSubdivType(0)
   , osdSubdivUVs(1)
   , osdPreserveMapBorders(1)
   , osdPreserveGeometryBorders(0)
   , useGlobals(1)
   , viewDep(1)
   , edgeLength(4.0f)
   , maxSubdivs(256)
   , displacementType(0)
   , displacementTexColor(0)
   , displacementTexFloat(0)
   , displacementAmount(1.0f)
   , displacementShift(0.0f)
   , keepContinuity(0)
   , waterLevel(-1e+30)
   , vectorDisplacement(0)
   , mapChannel(0)
   , useBounds(0)
   , minBound(0.0f, 0.0f, 0.0f)
   , maxBound(1.0f, 1.0f, 1.0f)
   , imageWidth(0)
   , cacheNormals(0)
   , objectSpaceDisplacement(0)
   , staticDisplacement(0)
   , displace2d(0)
   , resolution(256)
   , precision(8)
   , tightBounds(0)
   , filterTexture(0)
   , filterBlur(0.001)
   , particleType(6)
   , particleAttribs("")
   , spriteSizeX(1.0f)
   , spriteSizeY(1.0f)
   , spriteTwist(0.0f)
   , spriteOrientation(0)
   , radius(1.0f)
   , pointSize(1.0f)
   , pointRadii(0)
   , pointWorldSize(0)
   , multiCount(1)
   , multiRadius(0.0f)
   , lineWidth(1.0f)
   , tailLength(1.0f)
{
}

void AlembicLoaderParams::Cache::setupCache(VR::VRayParameterList *paramList)
{
   paramList->setParamCache("filename", &filename);
   paramList->setParamCache("objectPath", &objectPath);
   paramList->setParamCache("referenceFilename", &referenceFilename);
   paramList->setParamCache("startFrame", &startFrame);
   paramList->setParamCache("endFrame", &endFrame);
   paramList->setParamCache("speed", &speed);
   paramList->setParamCache("offset", &offset);
   paramList->setParamCache("fps", &fps);
   paramList->setParamCache("preserveStartFrame", &preserveStartFrame);
   paramList->setParamCache("cycle", &cycle);
   paramList->setParamCache("ignoreTransforms", &ignoreTransforms);
   paramList->setParamCache("ignoreVisibility", &ignoreVisibility);
   paramList->setParamCache("ignoreInstances", &ignoreInstances);
   paramList->setParamCache("ignoreTransformBlur", &ignoreTransformBlur);
   paramList->setParamCache("ignoreDeformBlur", &ignoreDeformBlur);
   paramList->setParamCache("verbose", &verbose);
   
   paramList->setParamCache("subdiv_enable", &subdivEnable);
   paramList->setParamCache("subdiv_uvs", &subdivUVs);
   paramList->setParamCache("preserve_map_borders", &preserveMapBorders);
   paramList->setParamCache("static_subdiv", &staticSubdiv);
   paramList->setParamCache("classic_catmark", &classicCatmark);
   
   paramList->setParamCache("osd_subdiv_enable", &osdSubdivEnable);
   paramList->setParamCache("osd_subdiv_level", &osdSubdivLevel);
   paramList->setParamCache("osd_subdiv_type", &osdSubdivType);
   paramList->setParamCache("osd_subdiv_uvs", &osdSubdivUVs);
   paramList->setParamCache("osd_preserve_map_borders", &osdPreserveMapBorders);
   paramList->setParamCache("osd_preserve_geometry_borders", &osdPreserveGeometryBorders);
   
   paramList->setParamCache("use_globals", &useGlobals);
   paramList->setParamCache("view_dep", &viewDep);
   paramList->setParamCache("edge_length", &edgeLength);
   paramList->setParamCache("max_subdivs", &maxSubdivs);
   
   paramList->setParamCache("displacement_type", &displacementType);
   paramList->setParamCache("displacement_tex_color", &displacementTexColor);
   paramList->setParamCache("displacement_tex_float", &displacementTexFloat);
   paramList->setParamCache("displacement_amount", &displacementAmount);
   paramList->setParamCache("displacement_shift", &displacementShift);
   paramList->setParamCache("keep_continuity", &keepContinuity);
   paramList->setParamCache("water_level", &waterLevel);
   paramList->setParamCache("vector_displacement", &vectorDisplacement);
   //paramList->setParamCache("map_channel", &mapChannel);
   paramList->setParamCache("use_bounds", &useBounds);
   paramList->setParamCache("min_bound", &minBound);
   paramList->setParamCache("max_bound", &maxBound);
   paramList->setParamCache("image_width", &imageWidth);
   paramList->setParamCache("cache_normals", &cacheNormals);
   //paramList->setParamCache("object_space_displacement", &objectSpaceDisplacement);
   paramList->setParamCache("static_displacement", &staticDisplacement);
   paramList->setParamCache("displace_2d", &displace2d);
   paramList->setParamCache("resolution", &resolution);
   paramList->setParamCache("precision", &precision);
   paramList->setParamCache("tight_bounds", &tightBounds);
   paramList->setParamCache("filter_texture", &filterTexture);
   paramList->setParamCache("filter_blur", &filterBlur);
   
   paramList->setParamCache("particle_type", &particleType);
   paramList->setParamCache("particle_attribs", &particleAttribs);
   paramList->setParamCache("sprite_size_x", &spriteSizeX);
   paramList->setParamCache("sprite_size_y", &spriteSizeY);
   paramList->setParamCache("sprite_twist", &spriteTwist);
   paramList->setParamCache("sprite_orientation", &spriteOrientation);
   paramList->setParamCache("radius", &radius);
   paramList->setParamCache("point_size", &pointSize);
   paramList->setParamCache("point_radii", &pointRadii);
   paramList->setParamCache("point_world_size", &pointWorldSize);
   paramList->setParamCache("multi_count", &multiCount);
   paramList->setParamCache("multi_radius", &multiRadius);
   paramList->setParamCache("line_width", &lineWidth);
   paramList->setParamCache("tail_length", &tailLength);
}

AlembicLoaderParams::AlembicLoaderParams()
   : VR::VRayParameterListDesc()
{
   addParamString("filename", "", -1, "Alembic file path");
   addParamString("objectPath", "/", -1, "Object expression");
   addParamFloat("startFrame", 0.0f, -1, "Animation start frame");
   addParamFloat("endFrame", 0.0f, -1, "Animation end frame");
   addParamFloat("offset", 0.0f, -1, "Animation range offset");
   addParamFloat("speed", 1.0f, -1, "Animation speed");
   addParamFloat("fps", 24.0f, -1, "Animation frame rate");
   addParamInt("cycle", 0, -1, "Cycle mode", "enum=(0:hold;1:loop;2:reverse;3:bounce)");
   addParamBool("preserveStartFrame", false, -1, "Keep startFrame invariant when speed changes");
   addParamString("referenceFilename", "", -1, "Alembic reference file path");
   addParamBool("ignoreVisibility", false, -1, "Treat all contained objects as visible");
   addParamBool("ignoreTransforms", false, -1, "Ignore transformations");
   addParamBool("ignoreInstances", false, -1, "Ignore object instances");
   addParamBool("ignoreDeformBlur", false, -1, "Ignore deformation blur");
   addParamBool("ignoreTransformBlur", false, -1, "Ignore transformation blur");
   addParamBool("verbose", false, -1, "Verbose ouput");
  
   // Should have attributes to control how much of the remaining attributes are transfered to generated
   //   shapes when expanding to multiple shapes with transforms
   
   // V-Ray mesh attributes
   //
   // Subdivision
   addParamBool("subdiv_enable", false, -1, "Enable subdivision"); // maya: vraySubdivEnable
   //   On GeomStaticMesh
   addParamBool("subdiv_uvs", true, 1, "Subdivide UVs"); // maya: vraySubdivUVs
   //   On GeomStaticSmoothedMesh
   addParamInt("preserve_map_borders", -1, -1, "UV boundaries subdivision mode. -1: not set, 0: none, 1: internal, 2: all", "enum=(0:None;1:Internal;2:All)");
   addParamBool("static_subdiv", false, -1, "Static subdivision");
   addParamBool("classic_catmark", false, -1, "Catmull-Clark subdivision");
   // OpenSubdiv
   //   On GeomStaticMesh
   addParamBool("osd_subdiv_enable", false, -1, "Enable OpenSubdiv");
   addParamInt("osd_subdiv_level", 0, -1, "OpenSubdiv subdivision level. 0 means no subdivision");
   addParamInt("osd_subdiv_type", 0, -1, "OpenSubdiv subdivision type", "enum=(0:Catmull-Clark;1:Loop)");
   addParamBool("osd_subdiv_uvs", true, -1, "OpenSubdiv smooth uvs");
   addParamInt("osd_preserve_map_borders", 1, -1, "OpenSubdiv UV boundaries subdivision mode", "enum=(0:None;1:Internal;2:All)");
   addParamBool("osd_preserve_geometry_borders", false, -1, "OpenSubdiv preserve geometry boundaries");
   // Subdivision Quality
   //   On GeomStaticSmoothedMesh and GeomDisplacedMesh
   addParamBool("use_globals", true, -1, "Use global displacement quality settings");
   addParamBool("view_dep", true, -1, "View dependent subdivision");
   addParamFloat("edge_length", 4.0f, -1, "Approximate edge length for sub-triangle ");
   addParamInt("max_subdivs", 256, -1, "Maximum subdivision level");
   // Displacement
   addParamInt("displacement_type", 0, -1, "Displacement type", "enum=(0:None;1:Displaced)");
   //   On GeomStaticSmoothedMesh and GeomDisplacedMesh (subdiv, subdiv + 3D disp)
   addParamTextureFloat("displacement_tex_float", -1, "Displacement texture");
   addParamTexture("displacement_tex_color", -1, "Displacement texture");
   addParamFloat("displacement_amount", 1.0f, -1, "Displacement amount");
   addParamFloat("displacement_shift", 0.0f, -1, "Displacement shift");
   addParamBool("keep_continuity", false, -1, "Try to preserve displaced surface continuity");
   addParamFloat("water_level", -1e+30, -1, "Clip geometry below this level");
   addParamInt("vector_displacement", 0, -1, "1: base 0.5 displacement, 2: absolute tangent space, 3: object space", "enum=(0:Normal;1:Vector)");
   addParamInt("map_channel", 0, -1, "Map channel to use for vector displacement");
   addParamBool("use_bounds", false, -1, "Use explicit displacement bounds");
   addParamColor("min_bound", VR::Color(0.0f, 0.0f, 0.0f), -1, "Lowest value for displacement texture");
   addParamColor("max_bound", VR::Color(1.0f, 1.0f, 1.0f), -1, "Highest value for displacement texture");
   addParamInt("image_width", 0, -1, "Override render image with globals for subdiv depth computation");
   addParamBool("cache_normals", false, -1, "Cache normals of generated triangles");
   addParamBool("object_space_displacement", false, -1, "If true, parent transformation affects amount of displacement. (3D displacement only)");
   //   On GeomDisplacedMesh (no subdiv + disp or subd + 2D disp)
   addParamBool("static_displacement", false, -1, "Insert displaced geometry as static geometry in the rayserver.");
   addParamBool("displace_2d", false, -1, "Enable 2D displacement. Overrides 'vector_displacement'");
   addParamInt("resolution", 256, -1, "Resolution at which to sample displacement map for 2D displacement.");
   addParamInt("precision", 8, -1, "Increase for curved surfaces to avoid artifacts");
   addParamBool("tight_bounds", false, -1, "Compute tighter bounds for displaced triangles (faster render, slower initializaion)");
   addParamBool("filter_texture", false, -1, "Filter the texture for 2D displacement");
   addParamFloat("filter_blur", 0.001, -1, "1.0 to average the whole texture.");
   
   // maya:vrayUserAttributes, maya:vrayObjectID -> transfered to V-Ray node
   //
   // On GeomStaticMesh too
   //   reference_mesh
   //   reference_transform
   //   map_channels
   //   map_channels_names
   //   smooth_derivs (for channels) (bool or name list / integer list)
   //   velocities
   //   smooth_uv_borders
   //   edge_creases_vertices
   //   edge_creases_sharpness
   //   vertex_creases_vertices
   //   vertex_creases_sharpness
   
   // V-Ray particle attributes
   //
   addParamInt("particle_type", 6, -1, "Invalid values will default to points (6)", "enum=(3:multipoints;4:multistreaks;6:points;7:spheres;8:sprites;9:streaks)");
   addParamString("particle_attribs", "", -1, "Alembic point attributes names to remap. ';' separated list of <vray_name>=<alembic_name>, where vray_name is one the list parameter name found on GeomParticleSystem plugin but for positions, velocities and ids)");
   addParamFloat("sprite_size_x", 1.0f, -1, "The width of sprite particles in world units.");
   addParamFloat("sprite_size_y", 1.0f, -1, "The height of sprite particles in world units.");
   addParamFloat("sprite_twist", 0.0f, -1, "The twist of sprite particles in degrees.");
   addParamInt("sprite_orientation", 0, -1, "0 - orient sprites towards the camera center, 1 - orient sprites parallel to the camera plane", "enum=(0:Towards camera center;1:Parallel to camera's image plane)");
   addParamFloat("radius", 1.0f, -1, "The constant particle radius when radii is empty.");
   addParamFloat("point_size", 1.0f, -1, "The size of point and multipoint particles, in pixels.");
   addParamBool("point_radii", false, -1, "If true, the point size will be taken form radii parameter.");
   addParamBool("point_world_size", false, -1, "If true, the point size is in world space, not screen space.");
   addParamInt("multi_count", 1, -1, "The number of particles generated for each input particle, when the render type is multipoints or multistreaks.");
   addParamFloat("multi_radius", 0.0f, -1, "The maximum distance between the original and a generated particle when the render type is multipoints or multistreaks.");
   addParamFloat("line_width", 1.0f, -1, "The width of streak particles, in pixels.");
   addParamFloat("tail_length", 1.0f, -1, "The length of streak particles, in world units, the actual length depends on the particle velocity as well.");
}

AlembicLoaderParams::~AlembicLoaderParams()
{
}
