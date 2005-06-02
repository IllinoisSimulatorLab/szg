//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_HEADER_H
#define AR_GRAPHICS_HEADER_H

#ifdef AR_USE_WIN_32

// this includes both widows.h *and* the right winsock2.h
#include "arPrecompiled.h"
#include <GL\gl.h>
#include <GL\glu.h>
#include <GL\glut.h>

#else
  
#ifdef AR_USE_DARWIN

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#else

// THE LINUX/SGI CASE
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#endif

#endif

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

// also want to include the node type IDs enum (as distinct from the
// record IDs in the graphics language)
enum{
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
  AR_G_BUMP_MAP_NODE = 16
};

#endif
