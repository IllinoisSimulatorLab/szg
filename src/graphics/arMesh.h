//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MESH_H
#define AR_MESH_H

#include "arGraphicsDatabase.h"
#include "arMath.h"
#include "arGraphicsAPI.h"
#include <string>

/// Common behavior for meshes in arMesh.cpp/arMesh.h.

class arMesh {
 public:
  arMesh(const arMatrix4& transform = ar_identityMatrix()) :
    _matrix(transform) {}
  virtual ~arMesh() {}
  void setTransform(const arMatrix4& matrix){ _matrix = matrix; }
  arMatrix4 getTransform(){ return _matrix; }
  /// Build the geometry.
  virtual void attachMesh(const string& name, const string& nameParent) = 0;
  void attach(const string& name, arGraphicsNode* node){
    attachMesh(name, node->getName());
  }
 protected:
  arMatrix4 _matrix;
};

/// Cube, made of 12 triangles.

class arCubeMesh : public arMesh {
 public:
  arCubeMesh() {}
  arCubeMesh(const arMatrix4& transform) : arMesh(transform) {}
  void attachMesh(const string& name, const string& nameParent);
};

/// Rectangle (to apply a texture to).

class arRectangleMesh : public arMesh {
 public:
  arRectangleMesh() {}
  arRectangleMesh(const arMatrix4& transform) : arMesh(transform) {}
  void attachMesh(const string& name, const string& nameParent);
};

/// Cylinder (technically a prism).

class arCylinderMesh : public arMesh {
 public:
  arCylinderMesh();
  arCylinderMesh(const arMatrix4&);

  void setAttributes(int,float,float);
  int getNumberDivisions(){ return _numberDivisions; }
  float getBottomRadius(){ return _bottomRadius; }
  float getTopRadius(){ return _topRadius; }
  void toggleEnds(bool);
  bool getUseEnds(){ return _useEnds; }
  void attachMesh(const string&, const string&);

 private:
  int _numberDivisions; // how many polygons approximate the beast
  float _bottomRadius;
  float _topRadius;
  bool _useEnds;
};

/// Pyramid.

class arPyramidMesh : public arMesh {
 public:
  arPyramidMesh() {}
  void attachMesh(const string& name, const string& parentName);
};

/// Sphere.
/// \todo Add draw() to other meshes too, to use them without the database.

class arSphereMesh : public arMesh {
 public:
  arSphereMesh(int numberDivisions=10);
  arSphereMesh(const arMatrix4&, int numberDivisions=10);

  void setAttributes(int);
  int getNumberDivisions(){ return _numberDivisions; }
  void setSectionSkip(int);
  int getSectionSkip(){ return _sectionSkip; }
  void attachMesh(const string& name, const string& parentName);
  void draw();

 private:
  arVector3 _spherePoint(int,int); // helper function for draw()
  int _numberDivisions; // now many polygons are use to approximate the beast
  int _sectionSkip; // whether we will draw every vertical section. Default
                    // of one does this, which 2 draws every other, etc.
};

/// Torus (donut).

class arTorusMesh : public arMesh {
 public:
  arTorusMesh(int,int,float,float);
  ~arTorusMesh();
 
  void reset(int,int,float,float);
  int getNumberBigAroundQuads(){ return _numberBigAroundQuads; }
  int getNumberSmallAroundQuads(){ return _numberSmallAroundQuads; }
  float getBigRadius(){ return _bigRadius; }
  float getSmallRadius(){ return _smallRadius; }
  void attachMesh(const string& name, const string& parentName);
  void setBumpMapName(const string& name);
  string getBumpMapName(){ return _bumpMapName; }

 private:
  int _numberBigAroundQuads;
  int _numberSmallAroundQuads;
  float _bigRadius;
  float _smallRadius;
  int _numberPoints;
  int _numberTriangles;
  
  float* _pointPositions;
  int* _triangleVertices;
  float* _surfaceNormals;
  
  float* _textureCoordinates; 

  string _bumpMapName;

  inline int _modAdd(int,int,int);
  void _reset(int,int,float,float);
  void _destroy();
};

#endif
