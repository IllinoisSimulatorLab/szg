//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MESH_H
#define AR_MESH_H

#include "arGraphicsDatabase.h"
#include "arMath.h"
#include "arGraphicsAPI.h"
#include "arGraphicsCalling.h"

#include <string>

class SZG_CALL arMesh {
 public:
  arMesh(const arMatrix4& transform = arMatrix4()) :
    _matrix(transform) {}
  virtual ~arMesh() {}

  void setTransform(const arMatrix4& matrix) { _matrix = matrix; }
  arMatrix4 getTransform() const { return _matrix; }

  // DEPRECATED. Assumes that nameParent gives a unique node name... and
  // just calls the real attachMesh method (which uses an arGraphicsNode*
  // parameter).
  // It is annoying that we have to make this virtual
  // and repeat it for each subclass (otherwise the compiler gets confused
  // because of the other attachMesh methods).
  virtual bool attachMesh(const string& name, const string& parentName);

  // Creates new scene graph nodes below the given one that contain the shape's geometry.
  virtual bool attachMesh(arGraphicsNode* node, const string& name) = 0;

  // Seperate virtual function so each subclass can have a seperate default name.
  virtual bool attachMesh(arGraphicsNode* node) = 0;

 protected:
  arMatrix4 _matrix;
};

// Made of 12 triangles.

class SZG_CALL arCubeMesh : public arMesh {
 public:
  arCubeMesh() {}
  arCubeMesh(const arMatrix4& transform) : arMesh(transform) {}
  bool attachMesh(const string& name, const string& parentName) {
    return arMesh::attachMesh(name, parentName);
  }
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent) {
    return attachMesh(parent, "cube");
  }
};

// Rectangle (to apply a texture to).

class SZG_CALL arRectangleMesh : public arMesh {
 public:
  arRectangleMesh() {}
  arRectangleMesh(const arMatrix4& transform) : arMesh(transform) {}
  bool attachMesh(const string& name, const string& parentName) {
    return arMesh::attachMesh(name, parentName);
  }
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent) {
    return attachMesh(parent, "rectangle");
  }
};

// Technically a prism.

class SZG_CALL arCylinderMesh : public arMesh {
 public:
  arCylinderMesh();
  arCylinderMesh(const arMatrix4&);

  void setAttributes(int numberDivisions,
                     float bottomRadius,
                     float topRadius);
  int getNumberDivisions() { return _numberDivisions; }
  float getBottomRadius() { return _bottomRadius; }
  float getTopRadius() { return _topRadius; }
  void toggleEnds(bool);
  bool getUseEnds() { return _useEnds; }
  bool attachMesh(const string& name, const string& parentName) {
    return arMesh::attachMesh(name, parentName);
  }
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent) {
    return attachMesh(parent, "cylinder");
  }

 private:
  int _numberDivisions; // how many polygons approximate the beast
  float _bottomRadius;
  float _topRadius;
  bool _useEnds;
};

class SZG_CALL arPyramidMesh : public arMesh {
 public:
  arPyramidMesh() {}
  bool attachMesh(const string& name, const string& parentName) {
    return arMesh::attachMesh(name, parentName);
  }
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent) {
    return attachMesh(parent, "pyramid");
  }
};

class SZG_CALL arSphereMesh : public arMesh {
 public:
  arSphereMesh(int numberDivisions=10);
  arSphereMesh(const arMatrix4&, int numberDivisions=10);

  void setAttributes(int numberDivisions);
  int getNumberDivisions() { return _numberDivisions; }
  void setSectionSkip(int skip);
  int getSectionSkip() { return _sectionSkip; }
  bool attachMesh(const string& name, const string& parentName) {
    return arMesh::attachMesh(name, parentName);
  }
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent) {
    return attachMesh(parent, "sphere");
  }
 private:
  // How many polygons are use to approximate the object.
  int _numberDivisions;
  // Whether we will draw every vertical section. Default
  // of one does this, which 2 draws every other, etc.
  int _sectionSkip;
};

class SZG_CALL arTorusMesh : public arMesh {
 public:
  arTorusMesh(int numberBigAroundQuads,
              int numberSmallAroundQuads,
              float bigRadius,
              float smallRadiusint);
  ~arTorusMesh();

  void reset(int numberBigAroundQuads,
             int numberSmallAroundQuads,
             float bigRadius,
             float smallRadius);
  int getNumberBigAroundQuads() const { return _numberBigAroundQuads; }
  int getNumberSmallAroundQuads() const { return _numberSmallAroundQuads; }
  float getBigRadius() const { return _bigRadius; }
  float getSmallRadius() const { return _smallRadius; }
  bool attachMesh(const string& name, const string& parentName) {
    return arMesh::attachMesh(name, parentName);
  }
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent) {
    return attachMesh(parent, "torus");
  }

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

  void _reset(const int, const int, const float, const float);
  void _destroy();
};

#endif
