//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsLanguage.h"
#include "arLogStream.h"

#include <sstream>

arGraphicsLanguage::arGraphicsLanguage():
  _transform("transform"),
  _points("points"),
  _texture("texture"),
  _boundingSphere("bounding sphere"),
  _billboard("billboard"),
  _visibility("visibility"),
  _viewer("viewer"),
  _blend("blend"),
  _normal3("normal3"),
  _color4("color4"),
  _tex2("tex2"),
  _index("index"),
  _drawable("drawable"),
  _material("material"),
  _light("light"),
  _perspCamera("persp camera"),
  _bumpMap("bump map"),
  _graphicsAdmin("graphics admin"),
  _graphicsState("graphics state"),
  _graphicsPlugin("graphics plugin") {

  AR_TRANSFORM_ID = _transform.add("ID", AR_INT);
  AR_TRANSFORM_MATRIX = _transform.add("matrix", AR_FLOAT);
  AR_TRANSFORM = _dictionary.add(&_transform);

  AR_POINTS_ID = _points.add("ID", AR_INT);
  AR_POINTS_POINT_IDS = _points.add("point IDs", AR_INT);
  AR_POINTS_POSITIONS = _points.add("positions", AR_FLOAT);
  AR_POINTS = _dictionary.add(&_points);

  AR_TEXTURE_ID = _texture.add("ID", AR_INT);
  AR_TEXTURE_FILE = _texture.add("file", AR_CHAR);
  AR_TEXTURE_ALPHA = _texture.add("alpha", AR_INT);
  AR_TEXTURE_WIDTH = _texture.add("width", AR_INT);
  AR_TEXTURE_HEIGHT = _texture.add("height", AR_INT);
  AR_TEXTURE_PIXELS = _texture.add("pixels", AR_CHAR);
  AR_TEXTURE = _dictionary.add(&_texture);

  AR_BOUNDING_SPHERE_ID = _boundingSphere.add("ID", AR_INT);
  AR_BOUNDING_SPHERE_VISIBILITY = _boundingSphere.add("visibility", AR_INT);
  AR_BOUNDING_SPHERE_RADIUS = _boundingSphere.add("radius", AR_FLOAT);
  AR_BOUNDING_SPHERE_POSITION = _boundingSphere.add("position", AR_FLOAT);
  AR_BOUNDING_SPHERE = _dictionary.add(&_boundingSphere);

  AR_BILLBOARD_ID = _billboard.add("ID", AR_INT);
  AR_BILLBOARD_VISIBILITY = _billboard.add("visibility", AR_INT);
  AR_BILLBOARD_TEXT = _billboard.add("text", AR_CHAR);
  AR_BILLBOARD = _dictionary.add(&_billboard);

  AR_VISIBILITY_ID = _visibility.add("ID", AR_INT);
  AR_VISIBILITY_VISIBILITY = _visibility.add("visibility", AR_INT);
  AR_VISIBILITY = _dictionary.add(&_visibility);

  AR_VIEWER_ID = _viewer.add("ID", AR_INT);
  AR_VIEWER_MATRIX = _viewer.add("matrix", AR_FLOAT);
  AR_VIEWER_MID_EYE_OFFSET = _viewer.add("mid eye offset", AR_FLOAT);
  AR_VIEWER_EYE_DIRECTION = _viewer.add("eye direction", AR_FLOAT);
  AR_VIEWER_EYE_SPACING = _viewer.add("eye spacing", AR_FLOAT);
  AR_VIEWER_NEAR_CLIP = _viewer.add("near clip", AR_FLOAT);
  AR_VIEWER_FAR_CLIP = _viewer.add("far clip", AR_FLOAT);
  AR_VIEWER_UNIT_CONVERSION = _viewer.add("unit conversion", AR_FLOAT);
  AR_VIEWER_FIXED_HEAD_MODE = _viewer.add("fixed head mode", AR_INT);
  AR_VIEWER = _dictionary.add(&_viewer);

  AR_BLEND_ID = _blend.add("ID", AR_INT);
  AR_BLEND_FACTOR = _blend.add("factor", AR_FLOAT);
  AR_BLEND = _dictionary.add(&_blend);

  AR_NORMAL3_ID = _normal3.add("ID", AR_INT);
  AR_NORMAL3_NORMAL_IDS = _normal3.add("normal IDs", AR_INT);
  AR_NORMAL3_NORMALS = _normal3.add("normals", AR_FLOAT);
  AR_NORMAL3 = _dictionary.add(&_normal3);

  AR_COLOR4_ID = _color4.add("ID", AR_INT);
  AR_COLOR4_COLOR_IDS = _color4.add("color IDs", AR_INT);
  AR_COLOR4_COLORS = _color4.add("colors", AR_FLOAT);
  AR_COLOR4 = _dictionary.add(&_color4);

  AR_TEX2_ID = _tex2.add("ID", AR_INT);
  AR_TEX2_TEX_IDS = _tex2.add("tex IDs", AR_INT);
  AR_TEX2_COORDS = _tex2.add("coords", AR_FLOAT);
  AR_TEX2 = _dictionary.add(&_tex2);

  AR_INDEX_ID = _index.add("ID", AR_INT);
  AR_INDEX_INDEX_IDS = _index.add("index IDs", AR_INT);
  AR_INDEX_INDICES = _index.add("indices", AR_INT);
  AR_INDEX = _dictionary.add(&_index);

  AR_DRAWABLE_ID = _drawable.add("ID", AR_INT);
  AR_DRAWABLE_TYPE = _drawable.add("type", AR_INT);
  AR_DRAWABLE_NUMBER = _drawable.add("number", AR_INT);
  AR_DRAWABLE = _dictionary.add(&_drawable);

  AR_MATERIAL_ID = _material.add("ID", AR_INT);
  AR_MATERIAL_DIFFUSE = _material.add("diffuse", AR_FLOAT);
  AR_MATERIAL_AMBIENT = _material.add("ambient", AR_FLOAT);
  AR_MATERIAL_SPECULAR = _material.add("specular", AR_FLOAT);
  AR_MATERIAL_EMISSIVE = _material.add("emissive", AR_FLOAT);
  AR_MATERIAL_EXPONENT = _material.add("exponent", AR_FLOAT);
  AR_MATERIAL_ALPHA = _material.add("alpha", AR_FLOAT);
  AR_MATERIAL = _dictionary.add(&_material);

  AR_LIGHT_ID = _light.add("ID", AR_INT);
  AR_LIGHT_LIGHT_ID = _light.add("light ID", AR_INT);
  AR_LIGHT_POSITION = _light.add("position", AR_FLOAT);
  AR_LIGHT_DIFFUSE = _light.add("diffuse", AR_FLOAT);
  AR_LIGHT_AMBIENT = _light.add("ambient", AR_FLOAT);
  AR_LIGHT_SPECULAR = _light.add("specular", AR_FLOAT);
  AR_LIGHT_ATTENUATE = _light.add("attenuate", AR_FLOAT);
  AR_LIGHT_SPOT = _light.add("spot", AR_FLOAT);
  AR_LIGHT = _dictionary.add(&_light);

  AR_PERSP_CAMERA_ID =                _perspCamera.add("ID",                 AR_INT);
  AR_PERSP_CAMERA_CAMERA_ID =        _perspCamera.add("camera ID",         AR_INT);
  AR_PERSP_CAMERA_FRUSTUM =        _perspCamera.add("frustum",         AR_FLOAT);
  AR_PERSP_CAMERA_LOOKAT =        _perspCamera.add("lookat",         AR_FLOAT);
  AR_PERSP_CAMERA = _dictionary.add(&_perspCamera);

  AR_BUMPMAP_ID =        _bumpMap.add("ID",          AR_INT);
  AR_BUMPMAP_TANGENTS = _bumpMap.add("tangents", AR_FLOAT);
  AR_BUMPMAP_BINORMALS= _bumpMap.add("binormals", AR_FLOAT);
  AR_BUMPMAP_NORMALS =        _bumpMap.add("normals",  AR_FLOAT);
  AR_BUMPMAP_FILE =        _bumpMap.add("file",          AR_CHAR);
  AR_BUMPMAP_HEIGHT =        _bumpMap.add("height",          AR_FLOAT);
  AR_BUMPMAP = _dictionary.add(&_bumpMap);

  AR_GRAPHICS_ADMIN_ACTION = _graphicsAdmin.add("action", AR_CHAR);
  AR_GRAPHICS_ADMIN_NODE_ID = _graphicsAdmin.add("node_ID", AR_INT);
  AR_GRAPHICS_ADMIN_NAME = _graphicsAdmin.add("name", AR_CHAR);
  AR_GRAPHICS_ADMIN = _dictionary.add(&_graphicsAdmin);

  AR_GRAPHICS_STATE_ID = _graphicsState.add("ID", AR_INT);
  AR_GRAPHICS_STATE_STRING = _graphicsState.add("string", AR_CHAR);
  AR_GRAPHICS_STATE_INT = _graphicsState.add("int", AR_INT);
  AR_GRAPHICS_STATE_FLOAT = _graphicsState.add("float", AR_FLOAT);
  AR_GRAPHICS_STATE = _dictionary.add(&_graphicsState);

  AR_GRAPHICS_PLUGIN_ID     = _graphicsPlugin.add("ID", AR_INT);
  AR_GRAPHICS_PLUGIN_NAME   = _graphicsPlugin.add("name", AR_CHAR);
  AR_GRAPHICS_PLUGIN_INT    = _graphicsPlugin.add("int", AR_INT);
  AR_GRAPHICS_PLUGIN_LONG   = _graphicsPlugin.add("long", AR_LONG);
  AR_GRAPHICS_PLUGIN_FLOAT  = _graphicsPlugin.add("float", AR_FLOAT);
  AR_GRAPHICS_PLUGIN_DOUBLE = _graphicsPlugin.add("double", AR_DOUBLE);
  AR_GRAPHICS_PLUGIN_STRING   = _graphicsPlugin.add("string", AR_CHAR);
  AR_GRAPHICS_PLUGIN_NUMSTRINGS   = _graphicsPlugin.add("numstrings", AR_INT);
  AR_GRAPHICS_PLUGIN        = _dictionary.add(&_graphicsPlugin);

}

string arGraphicsLanguage::typeFromID(int ID) {
  arDataTemplate* temp = _dictionary.find(ID);
  if (temp) {
    return temp->getName();
  }
  return "NULL";
}

const char* arGraphicsLanguage::_stringFromID(const int id) const {
  const int cnames = 22;

  const int ids[] = {
    AR_TRANSFORM,
    AR_POINTS,
    AR_TEXTURE,
    AR_BOUNDING_SPHERE,
    AR_ERASE,
    AR_BILLBOARD,
    AR_VISIBILITY,
    AR_VIEWER,
    AR_MAKE_NODE,
    AR_BLEND,
    AR_NORMAL3,
    AR_COLOR4,
    AR_TEX2,
    AR_INDEX,
    AR_DRAWABLE,
    AR_MATERIAL,
    AR_LIGHT,
    AR_PERSP_CAMERA,
    AR_BUMPMAP,
    AR_GRAPHICS_ADMIN,
    AR_GRAPHICS_STATE,
    AR_GRAPHICS_PLUGIN,
    };

  static const char* names[cnames+1] = {
    "AR_TRANSFORM",
    "AR_POINTS",
    "AR_TEXTURE",
    "AR_BOUNDING_SPHERE",
    "AR_ERASE",
    "AR_BILLBOARD",
    "AR_VISIBILITY",
    "AR_VIEWER",
    "AR_MAKE_NODE",
    "AR_BLEND",
    "AR_NORMAL3",
    "AR_COLOR4",
    "AR_TEX2",
    "AR_INDEX",
    "AR_DRAWABLE",
    "AR_MATERIAL",
    "AR_LIGHT",
    "AR_PERSP_CAMERA",
    "AR_BUMPMAP",
    "AR_GRAPHICS_ADMIN",
    "AR_GRAPHICS_STATE",
    "AR_GRAPHICS_PLUGIN",
    "(unknown!)"
    };

  // Linear search is slow, but this is merely for diagnostics.
  for (int i=0; i<cnames; ++i)
    if (id == ids[i])
      return names[i];
  return names[cnames]; // id was not found
}

string arGraphicsLanguage::numstringFromID(const int id) const {
  return ar_intToString(id) + " (" + _stringFromID(id) + ")";
}

bool arGraphicsLanguage::checkNodeID(const int idExpected,
    const int id, const char* name) const {
  if (id == idExpected)
    return true;

  ar_log_error() << name << " expected " <<
    numstringFromID(idExpected) << ", not " << numstringFromID(id) << ".\n";

  // Caller could pass in its getName() for an even more specific warning.
  return false;
}
// todo: copypaste checkNodeID from graphics to sound.
