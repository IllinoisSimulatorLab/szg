//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CUBE_ENVIRONMENT_H
#define AR_CUBE_ENVIRONMENT_H

#include "arMesh.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

// Texture-mapped prism: n walls (often 4), a floor, and a ceiling.

class SZG_CALL arCubeEnvironment{
 public:
  // this class adds texture coordinates to arCubeMesh... one picture
  // per wall
  arCubeEnvironment();
  ~arCubeEnvironment();

  void setHeight(float);
  void setRadius(float);
  void setOrigin(float x, float z, float height);
  void setNumberWalls(int);
  void setCorner(int,float,float);
  void setWallTexture(int, const string&);
  void attachMesh(const string&, const string&);

 private:
  int       _numberWalls;
  float     _vertBound;
  float     _radius;
  float*    _cornerX;
  float*    _cornerZ;
  string*   _texFileName;
  arVector3 _origin;

  void      _calculateRegularWalls();
};

#endif
