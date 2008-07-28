// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include math.sip
%Include opengl.sip

// global extra code
%ModuleHeaderCode
#include "arAxisAlignedBoundingBox.h"
#include "arHead.h"
#include "arTexFont.h"
#include "arMaterial.h"
#include "arLargeImage.h"
#include "arTexture.h"
#include <string>
using namespace std;
%End

class arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
 public:
  arMesh(const arMatrix4& transform = arMatrix4());
  virtual ~arMesh();

  void setTransform(const arMatrix4& matrix);
  arMatrix4 getTransform() const /TransferBack/;

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
};


// Made of 12 triangles.
class arCubeMesh : arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
 public:
  arCubeMesh();
  arCubeMesh(const arMatrix4& transform);
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);
};

// Rectangle (to apply a texture to).
class arRectangleMesh : arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
 public:
  arRectangleMesh();
  arRectangleMesh(const arMatrix4& transform);
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);
};

// Technically a prism.
class arCylinderMesh : arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
 public:
  arCylinderMesh();
  arCylinderMesh(const arMatrix4&);

  void setAttributes(int numberDivisions,
                     float bottomRadius,
                     float topRadius);
  int getNumberDivisions();
  float getBottomRadius();
  float getTopRadius();
  void toggleEnds(bool);
  bool getUseEnds();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);
};

class arPyramidMesh : arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
 public:
  arPyramidMesh();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);
};

class arSphereMesh : arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
 public:
  arSphereMesh(int numberDivisions=10);
  arSphereMesh(const arMatrix4&, int numberDivisions=10);

  void setAttributes(int numberDivisions);
  int getNumberDivisions();
  void setSectionSkip(int skip);
  int getSectionSkip();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);
};

class arTorusMesh : arMesh {
%TypeHeaderCode
#include "arMesh.h"
%End
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
  int getNumberBigAroundQuads() const;
  int getNumberSmallAroundQuads() const;
  float getBigRadius() const;
  float getSmallRadius() const;
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);
};


