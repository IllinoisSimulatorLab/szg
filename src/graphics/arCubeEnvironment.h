//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CUBE_ENVIRONMENT_H
#define AR_CUBE_ENVIRONMENT_H
// part of the Graphics Language Library, some simple tools for communicating
// 3D graphics between programs
// Written under the GNU General Public License
// -- BJS

#include "arMesh.h"

/// Texture-mapped prism: n walls (often 4), a floor, and a ceiling.

class arCubeEnvironment{
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
