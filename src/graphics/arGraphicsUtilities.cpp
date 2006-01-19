//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsUtilities.h"
#include "arGraphicsHeader.h"
#include <iostream>
using namespace std;

arNodeLevel ar_convertToNodeLevel(int level){
  switch(level){
  case -1:
    return AR_IGNORE_NODE;
  case 0:
    return AR_STRUCTURE_NODE;
  case 1:
    return AR_STABLE_NODE;
  case 2:
    return AR_OPTIONAL_NODE;
  case 3:
    return AR_TRANSIENT_NODE;
  default:
    cerr << "arGraphicsPeer error: unexpected integer conversion to "
	 << "arNodeLevel.\n";
    return AR_IGNORE_NODE;
  }
}

// Ugly preprocessor hack, but at least it shows the structure of ar_drawXXX().
#define doVertex(i) glVertex3fv(positions+3*(i))
// This if() doesn't cost much, and increases safety.
#define doIndex( i) if (indices[i]<numberPos) glVertex3fv(positions+3*indices[i])
#define doNormal(i) glNormal3fv(normals+3*(i))
#define doTex(   i) glTexCoord2fv(texCoord+2*(i))
#define doColor( i) \
  { const float* c = colors+4*i; glColor4f(c[0], c[1], c[2], c[3]*blendFactor); }

inline void ar_draw01DRaw(GLenum drawableType, int number, const int* indices,
                          int numberPos, const float* positions, const float* colors, 
                          float blendFactor){
  unsigned int opType = 0;
  if (indices)
    opType |= 1;
  if (colors)
    opType |= 2;
  glBegin(drawableType);
  int i;
  switch (opType) {
  default:
    cerr << "ar_draw01DRaw internal error.\n";
    break;
  case 0:
    // nothing
    for (i=0; i<number; i++){
      doVertex(i);
    }
    break;
  case 1:
    // indices
    for (i=0; i<number; i++){
      doIndex(i);
    }
    break;
  case 2:
    // colors
    for (i=0; i<number; i++){
      doColor(i);
      doVertex(i);
    }
    break;
  case 3:
    // indices,colors
    for (i=0; i<number; i++){
      doColor(i);
      doIndex(i);
    }
    break;
  }
  glEnd();
}

void ar_drawPoints(int number, const int* indices, 
                   int numberPos, const float* positions,
                   const float* colors, float blendFactor){
  ar_draw01DRaw(GL_POINTS, number, indices, numberPos, positions, 
                colors, blendFactor);
}

void ar_drawLines(int number, const int* indices, 
                  int numberPos, const float* positions,
                  const float* colors, float blendFactor){
  ar_draw01DRaw(GL_LINES, 2*number, indices, numberPos, positions, 
                colors, blendFactor);
}

void ar_drawLineStrip(int number, const int* indices, 
                      int numberPos, const float* positions,
		      const float* colors, float blendFactor){
  ar_draw01DRaw(GL_LINE_STRIP, 1+number, indices, numberPos, positions, 
                colors, blendFactor);
}

inline void ar_draw2DRaw(GLenum drawableType, int number,
                         const int* indices, int numberPos, const float* positions,
                         const float* normals, const float* colors, const float* texCoord,
                         float blendFactor
			 /* , int numCgParams=0,
			 const CGparameter* cgParams=0,
			 const float** cgData=0*/ ){
  // We use the vertex array path with CG and the original code path
  // without CG. Why? seems like the vertex array stuff has problems on some
  // boxen (specifically some Win2K w/ Nvidia cards)

  // UNFORTUNATELY... THIS STUFF DOES NOT WORK WITH BLENDING! (at least with
  // my blend factor hack). BUT... it could be that the blend factor hack is
  // a bad idea anyway
#ifdef USE_CG
  // Vertex Arrays
  if (normals) {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, 0, normals);
  }
  if (colors) {
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_FLOAT, 0, colors);
  }
  if (texCoord) {
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
  }
  if (numCgParams && cgParams && cgData) {
    for (int i=0; i<numCgParams; ++i) {
      cgGLEnableClientState(cgParams[i]);
      cgGLSetParameterPointer(cgParams[i], 3, GL_FLOAT, 0, cgData[i]);
    }
  }

  if (indices) {
    glBegin(drawableType);
    for (i=0; i<number; ++i) {
      glArrayElement(i);
      glVertex3fv(positions + 3*indices[i]);
    }
    glEnd();
  }
  else {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, positions);
    glDrawArrays(drawableType, 0, number);
  }

  if (numCgParams && cgParams && cgData) {
    for (int i=0; i<numCgParams; ++i)
      cgGLDisableClientState(cgParams[i]);
  }
  // clean up
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

#else
  unsigned int opType = 0;
  if (indices)
    opType |= 1;
  if (colors)
    opType |= 2;
  if (texCoord)
    opType |= 4;
  glBegin(drawableType);
  int i;
  switch (opType) {
  default:
    cerr << "ar_draw2DRaw internal error.\n";
    break;
  case 0:
    // nothing
    for (i=0; i<number; i++){
      doNormal(i);
      doVertex(i);
    }
    break;
  case 1:
    // indices
    for (i=0; i<number; i++){
      doNormal(i);
      doIndex(i);
    }
    break;
  case 2:
    // colors
    for (i=0; i<number; i++){
      doNormal(i);
      doColor(i);
      doVertex(i);
    }
    break;
  case 3:
    // indices,colors
    for (i=0; i<number; i++){
      doNormal(i);
      doColor(i);
      doIndex(i);
    }
    break;
  case 4:
    // texCoord
    for (i=0; i<number; i++){
      doNormal(i);
      doTex(i);
      doVertex(i);
    }
    break;
  case 5:
    // indices,texCoord
    for (i=0; i<number; i++){
      doNormal(i);
      doTex(i);
      doIndex(i);
    }
    break;
  case 6:
    // colors,texCoord
    for (i=0; i<number; i++){
      doNormal(i);
      doColor(i);
      doTex(i);
      doVertex(i);
    }
    break;
  case 7:
    // indices,colors,texCoord
    for (i=0; i<number; i++){
      doNormal(i);
      doColor(i);
      doTex(i);
      doIndex(i);
    }
    break;
  }
  glEnd();
#endif
}

void ar_drawTriangles(int number, const int* indices, 
                      int numberPos, const float* positions, 
                      const float* normals, const float* colors, const float* texCoord,
                      float blendFactor){
  ar_draw2DRaw(GL_TRIANGLES, 3*number, indices, numberPos, positions, normals,
	       colors, texCoord, blendFactor);
}
    
void ar_drawTriangleStrip(int number, const int* indices, 
                          int numberPos, const float* positions,
                          const float* normals, const float* colors, const float* texCoord,
                          float blendFactor){
  ar_draw2DRaw(GL_TRIANGLE_STRIP, 2+number, indices, numberPos, positions, 
               normals, colors, texCoord, blendFactor);
}

void ar_drawQuads(int number, const int* indices, 
                  int numberPos, const float* positions, const float* normals,
                  const float* colors, const float* texCoord, float blendFactor){
  ar_draw2DRaw(GL_QUADS, 4*number, indices, numberPos, positions, normals,
	       colors, texCoord, blendFactor);
}

void ar_drawQuadStrip(int number, const int* indices, 
                      int numberPos, const float* positions,
                      const float* normals, const float* colors, const float* texCoord,
                      float blendFactor){
  ar_draw2DRaw(GL_QUAD_STRIP, 2 + 2*number, indices, numberPos, positions, 
               normals, colors, texCoord, blendFactor);
}

void ar_drawPolygon(int number, const int* indices, 
                    int numberPos, const float* positions,
                    const float* normals, const float* colors, const float* texCoord,
                    float blendFactor){
  ar_draw2DRaw(GL_POLYGON, number, indices, numberPos, positions, normals,
	       colors, texCoord, blendFactor);
}

bool ar_openglStereo() {
  GLboolean glStereoSupported = 0;
  glGetBooleanv( GL_STEREO, &glStereoSupported );
  return (bool)glStereoSupported;
}
