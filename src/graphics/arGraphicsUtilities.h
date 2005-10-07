//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_UTILITIES_H
#define AR_GRAPHICS_UTILITIES_H

#ifdef USE_CG
  // Unfortunately, the internal includes in the Cg headers necessitate that
  // the Cg directory is what is on the include path, not the header files
  // themselves.
  #include <Cg/cgGL.h>
#else
  typedef int CGparameter;
#endif

// For arNodeLevel...
#include "arDatabaseNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

arNodeLevel ar_convertToNodeLevel(int level);

/// This function draws "number" points. There should be number values in
/// the indices array.
void ar_drawPoints(int number, int* indices, int numberPos, float* positions, 
                   float* colors, float blendFactor);
/// This function draws "number" independent lines. There should be
/// 2*number values in the indices array.
void ar_drawLines(int number, int* indices, int numberPos, float* positions,
                  float* colors, float blendFactor);
/// This function draws "number" lines in an OpenGL line strip.
/// There should be 1+number values in the indices array.
void ar_drawLineStrip(int number, int* indices, 
                      int numberPos, float* positions,
		      float* colors, float blendFactor);
/// This function draws "number" independent triangles. There should, for 
/// instance, be 3*number values in the indices array
void ar_drawTriangles(int number, int* indices, 
                      int numberPos, float* positions, 
                      float* normals, float* colors,
                      float* texCoord, float blendFactor, int numCgParams=0,
		      CGparameter* cgParams = 0, float** cgData = 0);
/// This function draws "number" triangles in an OpenGL triangle strip.
/// There should 2+number values in the indices array. And 3*(2+number)
/// values in the normals array.
void ar_drawTriangleStrip(int number, int* indices, 
                          int numberPos, float* positions,
                          float* normals, float* colors, float* texCoord,
			  float blendFactor);
/// This function draws "number" independent quads. There should be
/// 4*number values in the indices array.
void ar_drawQuads(int number, int* indices, 
                  int numberPos, float* positions, 
                  float* normals, float* colors,
                  float* texCoord, float blendFactor);
/// This function draws "number" quads in an OpenGL quad strip.
/// There should be 2+2*number values in the indices array
void ar_drawQuadStrip(int number, int* indices, 
                      int numberPos, float* positions,
                      float* normals, float* colors, float* texCoord,
		      float blendFactor);
/// This function draws a single polygon with "number" vertices.
/// There should be "number" values in the indices array
void ar_drawPolygon(int number, int* indices,
                    int numberPos, float* positions,
                    float* normals, float* colors, float* texCoord,
		    float blendFactor);

bool ar_openglStereo();

#endif
