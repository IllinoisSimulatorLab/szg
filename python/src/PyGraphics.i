// $Id: PyGraphics.i,v 1.5 2005/08/04 21:24:13 schaeffr Exp $
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
  void activate(bool forceRebind = false);
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
};

%{
#include "arLargeImage.h"
%}

class arLargeImage {
public:
  arLargeImage( unsigned int tileWidth=256, unsigned int tileHeight=0 );
  arLargeImage( const arTexture& x, unsigned int tileWidth=256, unsigned int tileHeight=0 );
  arLargeImage( const arLargeImage& x );
  arLargeImage& operator=( const arLargeImage& x );
  virtual ~arLargeImage();

  bool setTileSize( unsigned int tileWidth, unsigned int tileHeight=0 );
  void setImage( const arTexture& image );
  unsigned int getWidth() const { return originalImage.getWidth(); }
  unsigned int getHeight() const { return originalImage.getHeight(); }
  unsigned int numTilesWide() const { return _numTilesWide; }
  unsigned int numTilesHigh() const { return _numTilesHigh; }
  virtual void makeTiles();
  arTexture* getTile( unsigned int colNum, unsigned int rowNum );
  virtual void draw();

  arTexture originalImage;
};

%{
#include "arGraphicsScreen.h"
%}

class arGraphicsScreen {
 public:
  arGraphicsScreen();
  virtual ~arGraphicsScreen() {}

  void setCenter(const arVector3& center);
  void setNormal(const arVector3& normal);
  void setUp(const arVector3& up);
  void setDimensions(float width, float height);
  arVector3 getNormal() const { return _normal; }
  arVector3 getUp() const { return _up; }
  arVector3 getCenter() const { return _center; }

  void setWidth( float width ) { setDimensions( width, _height ); }
  void setHeight( float height ) { setDimensions( _width, height ); }
  float getWidth() const { return _width; }
  float getHeight() const { return _height; }

  void setHeadMounted( bool hmd ) { _headMounted = hmd; }
  bool getHeadMounted() const { return _headMounted; }

  void setTile( arVector4& tile ) { setTile( int( tile[ 0 ] ), int( tile[ 1 ] ),
                                             int( tile[ 2 ] ), int( tile[ 3 ] ) ); }

  void setTile( int tileX, int numberTilesX, int tileY, int numberTilesY );

  bool setUseFixedHeadMode( const std::string& usageMode );
  bool getIgnoreFixedHeadMode() const { return _ignoreFixedHeadMode; }
  bool getAlwaysFixedHeadMode() const { return _alwaysFixedHeadMode; }

  arVector3 getFixedHeadHeadPosition() const { return _fixedHeadPosition; }
  void setFixedHeadPosition( const arVector3& position ) { _fixedHeadPosition = position; }
  float getFixedHeadHeadUpAngle() const { return _fixedHeadUpAngle; }
  void setFixedHeadHeadUpAngle( float angle ) { _fixedHeadUpAngle = angle; }
};

%{
#include "arViewport.h"
%}

class arViewport {
 public:
  void setScreen( const arGraphicsScreen& screen ) { _screen = screen; }
  arGraphicsScreen* getScreen() { return &_screen; }
  // NOTE: the viewport now owns its camera.
  // It makes a copy here & returns the address of the copy

  // NOTE also: we need to write a Python-callback-based camera class
  // before these will be of any use.
  // arCamera* setCamera( arCamera* camera);
  // arCamera* getCamera();
  arVector4 getViewport() const;
  void setEyeSign(float eyeSign);
  float getEyeSign();
  void setColorMask(GLboolean red, GLboolean green,
		    GLboolean blue, GLboolean alpha);
  void clearDepthBuffer(bool flag);
  // e.g. GL_BACK_LEFT
  void setDrawBuffer( GLenum buf ) { _oglDrawBuffer = buf; }
  GLenum getDrawBuffer() const { return _oglDrawBuffer; }
};

%{
#include "arGraphicsWindow.h"
%}

class arGraphicsWindow {
  public:
    // This sets the camera for all viewports as well as future ones
    // Note that only a pointer is passed in, cameras are externally owned.
    // arCamera* setCamera( arCamera* cam=0 );
    // arCamera* getCamera(){ return _defaultCamera; }
    void setScreen( const arGraphicsScreen& screen ) { _defaultScreen = screen; }
    arGraphicsScreen* getScreen( void ) { return &_defaultScreen; }
    // This sets the camera for just a single viewport
    // arCamera* setViewportCamera( unsigned int vpindex, arCamera* cam );
    // Sets the camera for two adjacent (in the list) viewports
    // arCamera* setStereoViewportsCamera( unsigned int startVPIndex, arCamera* cam );
    // arCamera* getViewportCamera( unsigned int vpindex );
    bool getUseOGLStereo() const { return _useOGLStereo; }
    void addViewport(const arViewport&);
    // NOTE: the following two routines invalidate any externally held pointers
    // to individual viewport cameras (a pointer to the window default camera,
    // returned by getCamera() or setCamera(), will still be valid).
    bool setViewMode( const std::string& viewModeString );
    // std::vector<arViewport>* getViewports();
    arViewport* getViewport( unsigned int vpindex );
    float getCurrentEyeSign() const { return _currentEyeSign; }
};

%extend arGraphicsWindow {

PyObject* getPixelDimensions(void) {
  PyObject* dims = PyTuple_New( 4 );
  if (!dims) {
    PyErr_SetString( PyExc_MemoryError, "unable to allocate new tuple for arGraphicsWindow pixel dimensions." );
    return NULL;
  }
  int left, bottom, width, height;
  self->getPixelDimensions( left, bottom, width, height );
  PyTuple_SetItem( dims, 0, PyInt_FromLong((long)left) );
  PyTuple_SetItem( dims, 1, PyInt_FromLong((long)bottom) );
  PyTuple_SetItem( dims, 2, PyInt_FromLong((long)width) );
  PyTuple_SetItem( dims, 3, PyInt_FromLong((long)height) );
  return dims;
}

} // end %extend arGraphicsWindow

%{
#include "arGUIInfo.h"
%}

class arGUIWindowInfo {
  public:
    // inherited from arGUIInfo
    int getWindowID();
    int getPosX( void ) const;
    int getPosY( void ) const;
    int getSizeX( void ) const;
    int getSizeY( void ) const;
};


%{

// checking to see if the broken-ness of vertex arrays is a pyopengl
// bug. May not be, as these dont work either; I get a crash in
// ar_drawVertexArrays().

#include "arGraphicsHeader.h"

void ar_vertexPointer( double* vertPtr ) {
  glVertexPointer( 3, GL_DOUBLE, 0, (const GLvoid*)vertPtr );
}

void ar_texCoordPointer( double* texPtr ) {
  glVertexPointer( 2, GL_DOUBLE, 0, (const GLvoid*)texPtr );
}

void ar_enableVertexArrays() {
  glEnableClientState( GL_VERTEX_ARRAY );
}

void ar_enableTexCoordArrays() {
  glEnableClientState( GL_TEXTURE_COORD_ARRAY );
}

void ar_disableVertexArrays() {
  glDisableClientState( GL_VERTEX_ARRAY );
}

void ar_disableTexCoordArrays() {
  glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void ar_drawVertexArrays( int mode, int numVertices ) {
  glDrawArrays( mode, 0, numVertices );
}

%}

void ar_vertexPointer( double* vertPtr );
void ar_texCoordPointer( double* texPtr );
void ar_enableVertexArrays();
void ar_enableTexCoordArrays();
void ar_disableVertexArrays();
void ar_disableTexCoordArrays();
void ar_drawVertexArrays( int mode, int numVertices );

