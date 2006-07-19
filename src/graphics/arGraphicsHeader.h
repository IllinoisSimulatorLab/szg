//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_HEADER_H
#define AR_GRAPHICS_HEADER_H

#ifdef AR_USE_WIN_32
  #include "arPrecompiled.h"
  #include <GL\gl.h>
  #include <GL\glu.h>
#else
  #ifdef AR_USE_DARWIN
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
  #else
    // Linux/SGI
    #include <GL/gl.h>
    #include <GL/glu.h>
  #endif
#endif

#include "arGraphicsCalling.h"

// node type IDs (as distinct from record IDs in the graphics language)
enum {
  AR_G_TRANSFORM_NODE = 0,
  AR_G_TEXTURE_NODE = 1,
  AR_G_BOUNDING_SPHERE_NODE = 2,
  AR_G_BILLBOARD_NODE = 3,
  AR_G_VISIBILITY_NODE = 4,
  AR_G_VIEWER_NODE = 5,
  AR_G_BLEND_NODE = 6,
  AR_G_LIGHT_NODE = 7,
  AR_G_MATERIAL_NODE = 8,
  AR_G_PERSP_CAMERA_NODE = 9,
  AR_G_POINTS_NODE = 10,
  AR_G_NORMAL3_NODE = 11,
  AR_G_COLOR4_NODE = 12,
  AR_G_TEX2_NODE = 13,
  AR_G_INDEX_NODE = 14,
  AR_G_DRAWABLE_NODE = 15,
  AR_G_BUMP_MAP_NODE = 16,
  AR_G_GRAPHICS_STATE_NODE = 17,
  AR_G_GRAPHICS_PLUGIN_NODE = 18
};

enum arGraphicsStateID {
  AR_G_GARBAGE_STATE = 0,
  AR_G_POINT_SIZE = 1,
  AR_G_LINE_WIDTH = 2,
  AR_G_SHADE_MODEL = 3,
  AR_G_LIGHTING = 4,
  AR_G_BLEND = 5,
  AR_G_DEPTH_TEST = 6,
  AR_G_BLEND_FUNC = 7
};

enum arGraphicsStateValue {
  AR_G_FALSE= 0,
  AR_G_TRUE = 1,
  AR_G_SMOOTH = 2,
  AR_G_FLAT = 3,
  AR_G_ZERO = 4,
  AR_G_ONE = 5,
  AR_G_DST_COLOR = 6,
  AR_G_SRC_COLOR = 7,
  AR_G_ONE_MINUS_DST_COLOR = 8,
  AR_G_ONE_MINUS_SRC_COLOR = 9,
  AR_G_SRC_ALPHA = 10,
  AR_G_ONE_MINUS_SRC_ALPHA = 11,
  AR_G_DST_ALPHA = 12,
  AR_G_ONE_MINUS_DST_ALPHA = 13,
  AR_G_SRC_ALPHA_SATURATE = 14
};

#endif
