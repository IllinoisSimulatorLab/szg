// $Id: PyGraphics.i,v 1.1 2005/03/18 20:13:01 crowell Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


// The contents of this file are straight from the corresponding header files.

// ********************** from arDrawableNode.h ****************************

enum arDrawableType {DG_POINTS = 0, DG_LINES = 1, DG_LINE_STRIP = 2,
                     DG_TRIANGLES = 3, DG_TRIANGLE_STRIP = 4,
                     DG_QUADS = 5, DG_QUAD_STRIP = 6,
                     DG_POLYGON = 7};

// ********************** based on arGraphicsAPI.h ****************************

void dgSetGraphicsDatabase(arGraphicsDatabase*);

string dgGetNodeName(int);

arDatabaseNode* dgMakeNode(const string&, const string&, const string&);

int dgTransform(const string&, const string&, const arMatrix4&);
bool dgTransform(int, const arMatrix4&);

int dgPoints(const string&, const string&, int, int*, float*);
bool dgPoints(int, int, int*, float*);
int dgPoints(const string& name, const string& parent, int numPoints, 
             float* positions);
bool dgPoints(int ID, int numPoints, float* positions);

int dgTexture(const string& name, const string& parent,
              const string& filename, int alphaValue=-1);
bool dgTexture(int, const string& filename, int alphaValue=-1);

int dgTexture(const string& name, const string& parent,
               bool alpha, int w, int h, const char* pixels);
bool dgTexture(int ID, bool alpha, int w, int h, const char* pixels);

int  dgBoundingSphere(const string&, const string&, int, 
                      float, const arVector3&);
bool dgBoundingSphere(int, int, float, const arVector3&);

bool dgErase(const string&);

int dgBillboard(const string&, const string&, int, const string&);
bool dgBillboard(int, int, const string&);

int dgVisibility(const string&, const string&, int);
bool dgVisibility(int, int);

int dgBlend(const string&, const string&, float);
bool dgBlend(int, float);

int dgNormal3(const string& name, const string& parent, int numNormals,
          int* IDs, float* normals);
bool dgNormal3(int ID, int numNormals, int* IDs, float* normals);
int dgNormal3(const string& name, const string& parent, int numNormals,
          float* normals);
bool dgNormal3(int ID, int numNormals, float* normals);

int dgColor4(const string& name, const string& parent, int numColors,
          int* IDs, float* colors);
bool dgColor4(int ID, int numColors, int* IDs, float* colors);
int dgColor4(const string& name, const string& parent, int numColors,
          float* colors);
bool dgColor4(int ID, int numColors, float* colors);

int dgTex2(const string& name, const string& parent, int numTexcoords,
       int* IDs, float* coords);
bool dgTex2(int ID, int numTexcoords, int* IDs, float* coords);
int dgTex2(const string& name, const string& parent, int numTexcoords,
       float* coords);
bool dgTex2(int ID, int numTexcoords, float* coords);

int dgIndex(const string& name, const string& parent, int numIndices,
        int* IDs, int* indices);
bool dgIndex(int ID, int numIndices, int* IDs, int* indices);
int dgIndex(const string& name, const string& parent, int numIndices,
        int* indices);
bool dgIndex(int ID, int numIndices, int* indices);

int dgDrawable(const string& name, const string& parent,
           int drawableType, int numPrimitives);
bool dgDrawable(int ID, int drawableType, int numPrimitives);

int dgMaterial(const string& name, const string& parent, const arVector3& diffuse,
           const arVector3& ambient = arVector3(0.2,0.2,0.2), 
               const arVector3& specular = arVector3(0,0,0), 
               const arVector3& emissive = arVector3(0,0,0),
           float exponent = 0., 
               float alpha = 1.);
bool dgMaterial(int ID, const arVector3& diffuse,
            const arVector3& ambient = arVector3(0.2,0.2,0.2), 
                const arVector3& specular = arVector3(0,0,0), 
                const arVector3& emissive = arVector3(0,0,0),
            float exponent = 0., 
                float alpha = 1.);

int dgLight(const string& name, const string& parent,
        int lightID, arVector4 position,
        const arVector3& diffuse,
        const arVector3& ambient = arVector3(0,0,0),
        const arVector3& specular = arVector3(1,1,1),
        const arVector3& attenuate = arVector3(1,0,0),
            const arVector3& spotDiection = arVector3(0,0,-1),
        float spotCutoff = 180.,
        float spotExponent = 0.);

bool dgLight(int ID,
         int lightID, arVector4 position,
         const arVector3& diffuse,
         const arVector3& ambient = arVector3(0,0,0),
         const arVector3& specular = arVector3(1,1,1),
         const arVector3& attenuate = arVector3(1,0,0),
             const arVector3& spotDiection = arVector3(0,0,-1),
         float spotCutoff = 180.,
         float spotExponent = 0.);

/// Attach a perspective camera to the scene graph.
int dgCamera(const string& name, const string& parent,
         int cameraID, float leftClip, float rightClip, 
         float bottomClip, float topClip, float nearClip, float farClip,
             const arVector3& eyePosition = arVector3(0,0,0),
             const arVector3& centerPosition = arVector3(0,0,-1),
             const arVector3& upDirection = arVector3(0,1,0));

bool dgCamera(int ID,
          int cameraID, float leftClip, float rightClip, 
          float bottomClip, float topClip, float nearClip, float farClip,
              const arVector3& eyePosition = arVector3(0,0,0),
              const arVector3& centerPosition = arVector3(0,0,-1),
              const arVector3& upDirection = arVector3(0,1,0));


/// Attach a bump map node to the scene graph
int dgBumpMap(const string& name, const string& parent,
          const string& filename, float height=1.);

int dgBumpMap(int ID, const string& filename, float height=1.);


// ********************** based on arMesh.h ****************************

class arMesh {
 public:
  arMesh(const arMatrix4& transform = ar_identityMatrix()) :
    _matrix(transform) {}
  virtual ~arMesh() {}
  void setTransform(const arMatrix4& matrix) { _matrix = matrix; }
  arMatrix4 getTransform() { return &_matrix; }
  /// Build the geometry.
  virtual void attachMesh(const string& name, const string& nameParent) = 0;

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arMesh, 'this',
            _PySZG.new_arMesh(t))
        _swig_setattr(self, arMesh, 'thisown', 1)
}
};

/// Cube, made of 12 triangles.

class arCubeMesh : public arMesh {
 public:
  arCubeMesh() {}
  arCubeMesh(const arMatrix4& transform) : arMesh(transform) {}
  void attachMesh(const string& name, const string& nameParent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arCubeMesh, 'this',
            _PySZG.new_arCubeMesh(t))
        _swig_setattr(self, arCubeMesh, 'thisown', 1)
}
};

/// Rectangle (to apply a texture to).

class arRectangleMesh : public arMesh {
 public:
  arRectangleMesh() {}
  arRectangleMesh(const arMatrix4& transform) : arMesh(transform) {}
  void attachMesh(const string& name, const string& nameParent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arRectangleMesh, 'this',
            _PySZG.new_arRectangleMesh(t))
        _swig_setattr(self, arRectangleMesh, 'thisown', 1)
}
};

/// Cylinder (technically a prism).

class arCylinderMesh : public arMesh {
 public:
  arCylinderMesh();
  arCylinderMesh(const arMatrix4&);

  void setAttributes(int,float,float);
  int getNumberDivisions() { return _numberDivisions; }
  float getBottomRadius() { return _bottomRadius; }
  float getTopRadius() { return _topRadius; }
  void toggleEnds(bool);
  bool getUseEnds() { return _useEnds; }
  void attachMesh(const string&, const string&);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self.getTransform(),self.getNumberDivisions(),
                self.getBottomRadius(),self.getTopRadius(),
                self.getUseEnds()]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,L):
        _swig_setattr(self, arCylinderMesh, 'this',
            _PySZG.new_arCylinderMesh(L[0]))
        _swig_setattr(self, arCylinderMesh, 'thisown', 1)
        self.setAttributes(L[1],L[2],L[3])
        self.toggleEnds(L[4])
}
};

/// Pyramid.

class arPyramidMesh : public arMesh {
 public:
  arPyramidMesh() {}
  void attachMesh(const string& name, const string& parentName);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arPyramidMesh, 'this',
            _PySZG.new_arPyramidMesh())
        _swig_setattr(self, arPyramidMesh, 'thisown', 1)
        self.setTransform(t)
}
};

/// Sphere.
/// \todo Add draw() to other meshes too, to use them without the database.

class arSphereMesh : public arMesh {
 public:
  arSphereMesh(int numberDivisions=10);
  arSphereMesh(const arMatrix4&, int numberDivisions=10);

  void setAttributes(int);
  int getNumberDivisions() { return _numberDivisions; }
  void setSectionSkip(int);
  int getSectionSkip() { return _sectionSkip; }
  void attachMesh(const string& name, const string& parentName);
  void draw();

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self.getTransform(),self.getNumberDivisions(),
                self.getSectionSkip()]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,L):
        _swig_setattr(self, arSphereMesh, 'this',
            _PySZG.new_arSphereMesh(L[0],L[1]))
        _swig_setattr(self, arSphereMesh, 'thisown', 1)
        self.setSectionSkip(L[2])
}
};

/// Torus (donut).

class arTorusMesh : public arMesh {
 public:
  arTorusMesh(int,int,float,float);
  ~arTorusMesh();
 
  void reset(int,int,float,float);
  int getNumberBigAroundQuads() { return __numberBigAroundQuads; }
  int getNumberSmallAroundQuads() { return __numberSmallAroundQuads; }
  float getBigRadius() { return __bigRadius; }
  float getSmallRadius() { return __smallRadius; }
  void attachMesh(const string& name, const string& parentName);
  void setBumpMapName(const string& name);
  string getBumpMapName() { return _bumpMapName; }

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self.getTransform(),self.getNumberBigAroundQuads(),
                self.getNumberSmallAroundQuads(),self.getBigRadius(),
                self.getSmallRadius(),self.getBumpMapName()]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,L):
        _swig_setattr(self, arTorusMesh, 'this',
            _PySZG.new_arTorusMesh(L[1],L[2],L[3],L[4]))
        _swig_setattr(self, arTorusMesh, 'thisown', 1)
        self.setTransform(L[0])
        self.setBumpMapName(L[5])
}
};


%{
#include "arTexture.h"
%}

class arTexture {
 public:
  arTexture();
  virtual ~arTexture();
%extend{
  bool isValid() const {
    return self->getPixels()!=0 && self->numbytes()!=0;
  }
}
  void activate();
  void reactivate();
  void deactivate();

  int getWidth()  const;
  int getHeight() const;
  int getDepth()  const;   ///< bytes per pixel
  int numbytes() const; ///< size of _pixels
  void setTextureFunc( int texfunc );
  void mipmap(bool fEnable);
  void repeating(bool fEnable);
  void grayscale(bool fEnable);
  void dummy();

  bool readImage(const string& fileName, 
                 const string& subdirectory, 
                 const string& path,
                 int alpha = -1, bool complain = true);
  bool readPPM(const string& fileName, 
               const string& subdirectory, 
               const string& path,
               int alpha = -1, bool complain = true);
  bool writePPM(const string& fileName, const string& subdirectory, 
                const string& path);
  bool readJPEG(const string& fileName, 
                const string& subdirectory, 
                const string& path,
                int alpha = -1, bool complain = true);
  bool writeJPEG(const string& fileName, const string& subdirectory, 
                 const string& path);
  
  bool flipHorizontal();
  unsigned int glName() const; ///< OpenGL's name for texture
};

