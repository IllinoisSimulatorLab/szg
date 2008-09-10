//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_LANGUAGE_H
#define AR_GRAPHICS_LANGUAGE_H

#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arDatabaseLanguage.h"
#include "arGraphicsCalling.h"

// ID's shared between an arGraphicsServer and its arGraphicsClients
// when rendering an arGraphicsDatabase.

class SZG_CALL arGraphicsLanguage:public arDatabaseLanguage{
 public:
  arGraphicsLanguage();
  ~arGraphicsLanguage() {}

  string typeFromID(int ID);

  // these ints must be public, or at least efficiently readable by the
  // public. these hold the various field IDs. And we want to be able to
  // efficiently access those fields
  int AR_TRANSFORM;
  int AR_TRANSFORM_ID;
  int AR_TRANSFORM_MATRIX;

  int AR_POINTS;
  int AR_POINTS_ID;
  int AR_POINTS_POINT_IDS;
  int AR_POINTS_POSITIONS;

  int AR_TEXTURE;
  int AR_TEXTURE_ID;
  int AR_TEXTURE_FILE;
  int AR_TEXTURE_ALPHA;
  int AR_TEXTURE_WIDTH;
  int AR_TEXTURE_HEIGHT;
  int AR_TEXTURE_PIXELS;

  int AR_BOUNDING_SPHERE;
  int AR_BOUNDING_SPHERE_ID;
  int AR_BOUNDING_SPHERE_VISIBILITY;
  int AR_BOUNDING_SPHERE_RADIUS;
  int AR_BOUNDING_SPHERE_POSITION;

  int AR_BILLBOARD;
  int AR_BILLBOARD_ID;
  int AR_BILLBOARD_VISIBILITY;
  int AR_BILLBOARD_TEXT;

  int AR_VISIBILITY;
  int AR_VISIBILITY_ID;
  int AR_VISIBILITY_VISIBILITY;

  int AR_VIEWER;
  int AR_VIEWER_ID;
  int AR_VIEWER_MATRIX;          // transformation matrix of head
  int AR_VIEWER_MID_EYE_OFFSET;  // offset of "between eyes" from head position
  int AR_VIEWER_EYE_DIRECTION;   // unit vector from midpoint between eyes
                                 // to right eye
  int AR_VIEWER_EYE_SPACING;     // interpupillary distance
  int AR_VIEWER_NEAR_CLIP;       // near clipping plane (measured from
                                 // tracker, not from midpoint between eyes)
  int AR_VIEWER_FAR_CLIP;        // far clipping plane
  int AR_VIEWER_UNIT_CONVERSION; // and here we can have a global scale factor
  int AR_VIEWER_FIXED_HEAD_MODE;       // and here we can have a global demo mode flag

  int AR_BLEND;
  int AR_BLEND_ID;
  int AR_BLEND_FACTOR;

  int AR_NORMAL3;                // 3 dimensional normals
  int AR_NORMAL3_ID;
  int AR_NORMAL3_NORMAL_IDS;
  int AR_NORMAL3_NORMALS;

  int AR_COLOR4;                 // RGBA colors
  int AR_COLOR4_ID;
  int AR_COLOR4_COLOR_IDS;
  int AR_COLOR4_COLORS;

  int AR_TEX2;                   // 2D texture coordinates
  int AR_TEX2_ID;
  int AR_TEX2_TEX_IDS;
  int AR_TEX2_COORDS;

  int AR_INDEX;                  // a list of point indices
  int AR_INDEX_ID;
  int AR_INDEX_INDEX_IDS;
  int AR_INDEX_INDICES;

  int AR_DRAWABLE;               // a certain type of geometry like
                                 // points, lines, triangles, etc.
  int AR_DRAWABLE_ID;
  int AR_DRAWABLE_TYPE;
  int AR_DRAWABLE_NUMBER;

  int AR_MATERIAL;               // a color with respect to the
                                 // OpenGL lighting model
  int AR_MATERIAL_ID;
  int AR_MATERIAL_DIFFUSE;
  int AR_MATERIAL_AMBIENT;
  int AR_MATERIAL_SPECULAR;
  int AR_MATERIAL_EMISSIVE;
  int AR_MATERIAL_EXPONENT;
  int AR_MATERIAL_ALPHA;

  int AR_LIGHT;                  // an OpenGL light
  int AR_LIGHT_ID;
  int AR_LIGHT_LIGHT_ID;
  int AR_LIGHT_POSITION;
  int AR_LIGHT_DIFFUSE;
  int AR_LIGHT_AMBIENT;
  int AR_LIGHT_SPECULAR;
  int AR_LIGHT_ATTENUATE;
  int AR_LIGHT_SPOT;

  int AR_PERSP_CAMERA;           // a perspective camera
  int AR_PERSP_CAMERA_ID;
  int AR_PERSP_CAMERA_CAMERA_ID;
  int AR_PERSP_CAMERA_FRUSTUM;
  int AR_PERSP_CAMERA_LOOKAT;

  int AR_BUMPMAP;                // a Cg-enabled bump map
  int AR_BUMPMAP_ID;
  int AR_BUMPMAP_TANGENTS;
  int AR_BUMPMAP_BINORMALS;
  int AR_BUMPMAP_NORMALS;
  int AR_BUMPMAP_FILE;
  int AR_BUMPMAP_HEIGHT;

  int AR_GRAPHICS_ADMIN;        // Used in managing the arGraphicsPeer
  int AR_GRAPHICS_ADMIN_ACTION;
  int AR_GRAPHICS_ADMIN_NODE_ID;
  int AR_GRAPHICS_ADMIN_NAME;

  int AR_GRAPHICS_STATE;        // Used in manipulating rendering state inside
  int AR_GRAPHICS_STATE_ID;     // the scene graph. Stuff like point size,
  int AR_GRAPHICS_STATE_STRING; // line width, etc.
  int AR_GRAPHICS_STATE_INT;
  int AR_GRAPHICS_STATE_FLOAT;

  int AR_GRAPHICS_PLUGIN;     // A node defined in a dll.
  int AR_GRAPHICS_PLUGIN_ID;
  int AR_GRAPHICS_PLUGIN_NAME;
  int AR_GRAPHICS_PLUGIN_INT;
  int AR_GRAPHICS_PLUGIN_LONG;
  int AR_GRAPHICS_PLUGIN_FLOAT;
  int AR_GRAPHICS_PLUGIN_DOUBLE;
  int AR_GRAPHICS_PLUGIN_STRING;
  int AR_GRAPHICS_PLUGIN_NUMSTRINGS;

 protected:
  arDataTemplate _transform;
  arDataTemplate _points;
  arDataTemplate _texture;
  arDataTemplate _boundingSphere;
  arDataTemplate _billboard;
  arDataTemplate _visibility;
  arDataTemplate _viewer;
  arDataTemplate _blend;
  arDataTemplate _normal3;
  arDataTemplate _color4;
  arDataTemplate _tex2;
  arDataTemplate _index;
  arDataTemplate _drawable;
  arDataTemplate _material;
  arDataTemplate _light;
  arDataTemplate _perspCamera;
  arDataTemplate _bumpMap;
  arDataTemplate _graphicsAdmin;
  arDataTemplate _graphicsState;
  arDataTemplate _graphicsPlugin;
public:
  const char* _stringFromID(const int) const;
  string numstringFromID(const int) const;
  bool checkNodeID(const int idExpected, const int id, const char* name) const;
};

#endif
