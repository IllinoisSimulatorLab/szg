//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_UTILITIES_H
#define AR_GRAPHICS_UTILITIES_H

#include "arDatabaseNode.h"     // For arNodeLevel
#include "arGraphicsCalling.h"

arNodeLevel ar_convertToNodeLevel(int level);

// Draw "number" points. There should be number values in the indices array.
void ar_drawPoints(int number, const int* indices, int numberPos, const float* positions,
                   const float* colors, float blendFactor);
// Draw "number" independent lines. There should be 2*number values in the indices array.
void ar_drawLines(int number, const int* indices, int numberPos, const float* positions,
                  const float* colors, float blendFactor);
// Draw "number" lines in an OpenGL line strip.
// There should be 1+number values in the indices array.
void ar_drawLineStrip(int number, const int* indices,
                      int numberPos, const float* positions,
                      const float* colors, float blendFactor);
// Draw "number" independent triangles. There should, for
// instance, be 3*number values in the indices array
void ar_drawTriangles(int number, const int* indices,
                      int numberPos, const float* positions,
                      const float* normals, const float* colors,
                      const float* texCoord, float blendFactor);
// Draw "number" triangles in an OpenGL triangle strip.
// There should 2+number values in the indices array. And 3*(2+number)
// values in the normals array.
void ar_drawTriangleStrip(int number, const int* indices,
                          int numberPos, const float* positions,
                          const float* normals, const float* colors, const float* texCoord,
                          float blendFactor);
// Draw "number" independent quads. There should be 4*number values in the indices array.
void ar_drawQuads(int number, const int* indices,
                  int numberPos, const float* positions,
                  const float* normals, const float* colors,
                  const float* texCoord, float blendFactor);
// Draw "number" quads in an OpenGL quad strip.
// There should be 2+2*number values in the indices array
void ar_drawQuadStrip(int number, const int* indices,
                      int numberPos, const float* positions,
                      const float* normals, const float* colors, const float* texCoord,
                      float blendFactor);
// Draw a single polygon with "number" vertices.
// There should be "number" values in the indices array.
void ar_drawPolygon(int number, const int* indices,
                    int numberPos, const float* positions,
                    const float* normals, const float* colors, const float* texCoord,
                    float blendFactor);

bool ar_openglStereo();

#endif
