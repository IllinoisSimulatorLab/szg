// $Id: PyGraphics.i,v 1.7 2005/10/23 00:21:37 schaeffr Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


// The contents of this file are straight from the corresponding header files.

// ********************** from arGraphicsHeader.h ****************************

enum arGraphicsStateID {
  AR_G_GARBAGE_STATE = 0,
  AR_G_POINT_SIZE = 1,
  AR_G_LINE_WIDTH = 2,
  AR_G_SHADE_MODEL = 3,
  AR_G_LIGHTING = 4,
  AR_G_BLEND = 5,
  AR_G_DEPTH_TEST = 6,
  AR_G_BLEND_FUNC = 7
};

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

class arGraphicsScreen{
 public:
  arGraphicsScreen();
  virtual ~arGraphicsScreen();

  void setCenter(const arVector3& center);
  void setNormal(const arVector3& normal);
  void setUp(const arVector3& up);
  void setDimensions(float width, float height);
  arVector3 getNormal();
  arVector3 getUp();
  arVector3 getCenter() const;
  void setWidth( float width );
  void setHeight( float height );
  float getWidth() const;
  float getHeight() const;
  void setHeadMounted( bool hmd );
  bool getHeadMounted() const;
  void setTile( arVector4& tile );
  void setTile( int tileX, int numberTilesX, int tileY, int numberTilesY );
  arVector4 getTile();
  bool setUseFixedHeadMode( const std::string& usageMode );
  bool getIgnoreFixedHeadMode() const;
  bool getAlwaysFixedHeadMode() const;
  arVector3 getFixedHeadHeadPosition() const;
  void setFixedHeadPosition( const arVector3& position );
  float getFixedHeadHeadUpAngle() const;
  void setFixedHeadHeadUpAngle( float angle );
%extend{
  string __repr__(void){
    ostringstream s;
    s << "arGraphicsScreen\n";
    s << "center: arVector3" << self->getCenter() << "\n";
    s << "normal: arVector3" << self->getNormal() << "\n";
    s << "up:     arVector3" << self->getUp() << "\n";
    s << "size:   (" << self->getWidth() << " " << self->getHeight() << ")\n";
    s << "tile:   arVector4" << self->getTile() << "\n";
    return s.str();
  }
}
};

%{
#include "arHead.h"
%}

class arHead{
 public:
  arHead();
  virtual ~arHead();
  void setEyeSpacing( float spacing );
  float getEyeSpacing() const;
  void setMidEyeOffset( const arVector3& midEyeOffset );
  arVector3 getMidEyeOffset() const;
  void setEyeDirection( const arVector3& eyeDirection );
  arVector3 getEyeDirection() const;
  void setMatrix( const arMatrix4& matrix );
  virtual arMatrix4 getMatrix() const;
  void setClipPlanes( float nearClip, float farClip );
  float getNearClip() const;
  float getFarClip() const;
  void setUnitConversion( float conv );
  float getUnitConversion() const;
  arVector3 getEyePosition( float eyeSign, arMatrix4* useMatrix=0 );
  arVector3 getMidEyePosition( arMatrix4* useMatrix=0 );
  arMatrix4 getMidEyeMatrix() const;
  void setFixedHeadMode( bool onoff );
  bool getFixedHeadMode() const;
%extend{
  string __repr__(void){
    ostringstream s;
    s << "arHead\n";
    s << "eye spacing: " << self->getEyeSpacing() << "\n";
    s << "mid eye offset: " << self->getMidEyeOffset() << "\n";
    s << "eye direction: arVector3" << self->getEyeDirection() << "\n";
    s << "near clip: " << self->getNearClip() << "\n";
    s << "far clip: " << self->getFarClip() << "\n";
    s << "unit conversion: " << self->getUnitConversion() << "\n";
    s << "matrix:\n" << self->getMatrix();
    return s.str();
  }
}
};

%{
#include "arCamera.h"
#include "arPerspectiveCamera.h"
#include "arOrthoCamera.h"
#include "arVRCamera.h"
%}

class arCamera{
 public:
  arCamera();
  virtual ~arCamera(){}
  void setEyeSign( float eyeSign );
  float getEyeSign() const;
  void setScreen( arGraphicsScreen* screen );
  arGraphicsScreen* getScreen() const;
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
};

class arPerspectiveCamera: public arCamera{
 public:
  arPerspectiveCamera();
  virtual ~arPerspectiveCamera();
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  void setSides(float left, float right, float bottom, float top);
  arVector4 getSides();
  void setNearFar(float near, float far);
  float getNear();
  float getFar();
  void setPosition(float x, float y, float z);
  arVector3 getPosition();
  void setTarget(float x, float y, float z);
  arVector3 getTarget();
  void setUp(float x, float y, float z);
  arVector3 getUp();
  float getFrustumData(int i);
  float getLookatData(int i);

%extend{
  string __repr__(void){
    ostringstream s;
    s << "arPerspectiveCamera\n";
    s << "sides: arVector4" << self->getSides() << "\n";
    s << "near clip: " << self->getNear() << "\n";
    s << "far clip: " << self->getFar() << "\n";
    s << "position: arVector3" << self->getPosition() << "\n";
    s << "target: arVector3" << self->getTarget() << "\n";
    s << "up: arVector3" << self->getUp() << "\n";
    return s.str();
  }
}
};

class arOrthoCamera: public arCamera{
 public:
  arOrthoCamera();
  virtual ~arOrthoCamera();
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual std::string type( void ) const;
  void setSides( arVector4& sides );
  void setSides(float left, float right, float bottom, float top);
  arVector4 getSides();
  void setNearFar(float nearClip, float farClip);
  float getNear();
  float getFar();
  void setPosition( arVector3& pos );
  void setPosition(float x, float y, float z);
  arVector3 getPosition();
  void setTarget( arVector3& target );
  void setTarget(float x, float y, float z);
  arVector3 getTarget();
  void setUp( arVector3& up );
  void setUp(float x, float y, float z);
  arVector3 getUp();
%extend{
  string __repr__(void){
    ostringstream s;
    s << "arOrthoCamera\n";
    s << "sides: arVector4" << self->getSides() << "\n";
    s << "near clip: " << self->getNear() << "\n";
    s << "far clip: " << self->getFar() << "\n";
    s << "position: arVector3" << self->getPosition() << "\n";
    s << "target: arVector3" << self->getTarget() << "\n";
    s << "up: arVector3" << self->getUp() << "\n";
    return s.str();
  }
}
};

class arVRCamera : public arCamera {
  public:
    arVRCamera( arHead* head=0 );
    virtual ~arVRCamera();
    virtual arMatrix4 getProjectionMatrix();
    virtual arMatrix4 getModelviewMatrix();
    virtual std::string type( void ) const;
    void setHead( arHead* head);
    arHead* getHead() const;
};

%{
#include "arViewport.h"
%}

class arViewport{
 public:
  arViewport();
  virtual ~arViewport();
  void setScreen( const arGraphicsScreen& screen );
  arGraphicsScreen* getScreen();
  void setViewport( arVector4& viewport );
  void setViewport( float left, float bottom,
                    float width, float height );
  arVector4 getViewport();
  arCamera* setCamera( arCamera* camera);
  arCamera* getCamera();
  void setEyeSign(float eyeSign);
  float getEyeSign();
  void setColorMask(GLboolean red, GLboolean green,
		    GLboolean blue, GLboolean alpha);
  void clearDepthBuffer(bool flag);
  void setDrawBuffer( GLenum buf );
  GLenum getDrawBuffer() const;
  void activate();
%extend{
  string __repr__(void){
    ostringstream s;
    s << "arViewport\n";
    s << "viewport: arVector4" << self->getViewport() << "\n";
    s << "eye sign: " << self->getEyeSign() << "\n";
    return s.str();
  }
}
};

%{
#include "arGraphicsWindow.h"
%}

class arGraphicsWindow{
 public:
    arGraphicsWindow( arCamera* cam=0 );
    virtual ~arGraphicsWindow();
    void setScreen( const arGraphicsScreen& screen );
    arGraphicsScreen* getScreen( void );
    bool getUseOGLStereo() const { return _useOGLStereo; }
    void addViewport(const arViewport&);
    bool setViewMode( const std::string& viewModeString );
    void clearViewportList();
    void lockViewports();
    void unlockViewports();
    arViewport* getViewport( unsigned int vpindex );
    float getCurrentEyeSign() const;
    void setPixelDimensions( int posX, int posY, int sizeX, int sizeY );
    void getPixelDimensions( int& posX, int& posY, int& sizeX, int& sizeY );
    bool draw();
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
#include "arGUIWindow.h"
%}

typedef unsigned int arCursor;
typedef unsigned int arZOrder;
#define AR_CURSOR_ARROW    0x0000
#define AR_CURSOR_HELP     0x0001
#define AR_CURSOR_WAIT     0x0002
#define AR_CURSOR_NONE     0x0003
#define AR_ZORDER_NORMAL   0x0000
#define AR_ZORDER_TOP      0x0001
#define AR_ZORDER_TOPMOST  0x0002

class arGUIWindowConfig{
 public:
   arGUIWindowConfig( int x = 50, int y = 50, 
                       int width = 640, int height = 480,
                       int bpp = 16, int Hz = 0, bool decorate = true,
                       arZOrder zorder = AR_ZORDER_TOP,
                       bool fullscreen = false, bool stereo = false,
                       const std::string& title = "SyzygyWindow",
                       const std::string& XDisplay = ":0.0",
                       arCursor cursor = AR_CURSOR_ARROW );

   ~arGUIWindowConfig( void );

   void setPos( int x, int y );
   void setPosX( int x );
   void setPosY( int y );
   void setWidth( int width );
   void setHeight( int height );
   void setSize( int width, int height );
   void setBpp( int bpp );
   void setHz( int Hz );
   void setDecorate( bool decorate );
   void setFullscreen( bool fullscreen );
   void setZOrder( arZOrder zorder );
   void setTitle( const std::string& title );
   void setXDisplay( const std::string& XDisplay );
   void setCursor( arCursor cursor );

   int getPosX( void ) const;
   int getPosY( void ) const;
   int getWidth( void ) const;
   int getHeight( void ) const;
   int getBpp( void ) const;
   int getHz( void ) const;
   bool getDecorate( void ) const;
   bool getFullscreen( void ) const;
   bool getZOrder( void ) const;
   std::string getTitle( void ) const;
   std::string getXDisplay( void ) const;
   arCursor getCursor( void ) const;
%extend{
  string __repr__(void){
    ostringstream s;
    s << "arGUIWindowconfig\n";
    s << "position: (" << self->getPosX() << " " << self->getPosY() << ")\n";
    s << "size:     (" << self->getWidth() << " " 
      << self->getHeight() << ")\n";
    s << "bit depth: " << self->getBpp() << "\n";
    s << "decoration: " << self->getDecorate() << "\n";
    s << "zorder: " << self->getZOrder() << "\n";
    s << "title: " << self->getTitle() << "\n";
    return s.str();
  }
}
};

class arGUIWindow{
 public:
  arGUIWindow( int ID, arGUIWindowConfig windowConfig);
  virtual ~arGUIWindow( void );
  void registerDrawCallback( arGUIRenderCallback* drawCallback );
  int beginEventThread( void );
  int swap( void );
  int resize( int newWidth, int newHeight );
  int move( int newX, int newY );
  int setViewport( int newX, int newY, int newWidth, int newHeight );
  int fullscreen( void );
  int makeCurrent( bool release = false );
  void minimize( void );
  void restore( void );
  void decorate( const bool decorate );
  arCursor setCursor( arCursor cursor );
  void setVisible( const bool visible );
  bool getVisible( void ) const;
  std::string getTitle( void ) const;
  void setTitle( const std::string& title );
  int getID( void ) const;
  int getWidth( void ) const;
  int getHeight( void ) const;
  int getPosX( void ) const;
  int getPosY( void ) const;
  bool isStereo( void )      const;
  bool isFullscreen( void )  const;
  bool isDecorated( void )   const;
  arZOrder getZOrder( void ) const;
  bool running( void ) const;
  bool eventsPending( void ) const;
  arCursor getCursor( void ) const;
  int getBpp( void ) const;
  const arGUIWindowConfig& getWindowConfig( void ) const;
  arGraphicsWindow* getGraphicsWindow( void );
  void returnGraphicsWindow( void );
  void setGraphicsWindow( arGraphicsWindow* graphicsWindow );
};

%{
#include "arGUIWindowManager.h"
%}

class arGUIWindowManager{
 public:
  arGUIWindowManager( void (*windowCallback)( arGUIWindowInfo* ) = NULL,
                      void (*keyboardCallback)( arGUIKeyInfo* ) = NULL,
                      void (*mouseCallback)( arGUIMouseInfo* ) = NULL,
                      void (*windowInitGLCallback)( arGUIWindowInfo* ) = NULL,
                      bool threaded = true );
  virtual ~arGUIWindowManager( void );
  int startWithSwap( void );
  int startWithoutSwap( void );
  int addWindow( const arGUIWindowConfig& windowConfig,
                 bool useWindowing = true );
  int createWindows(const arGUIWindowingConstruct* windowingConstruct=NULL,  
                    bool useWindowing = true );
  void registerWindowCallback( void (*windowCallback) ( arGUIWindowInfo* ) );
  void registerKeyboardCallback( void (*keyboardCallback) ( arGUIKeyInfo* ) );
  void registerMouseCallback( void (*mouseCallback) ( arGUIMouseInfo* ) );
  void registerWindowInitGLCallback( void (*windowInitGLCallback)( arGUIWindowInfo* ) );
  int registerDrawCallback( const int windowID, 
                            arGUIRenderCallback* drawCallback );
  int processWindowEvents( void );
  arGUIInfo* getNextWindowEvent( const int windowID );
  int drawWindow( const int windowID, bool blocking = false );
  int drawAllWindows( bool blocking = false );
  int consumeWindowEvents( const int windowID, bool blocking = false );
  int consumeAllWindowEvents( bool blocking = false );
  int swapWindowBuffer( const int windowID, bool blocking = false );
  int swapAllWindowBuffers( bool blocking = false );
  int resizeWindow( const int windowID, int width, int height );
  int moveWindow( const int windowID, int x, int y );
  int setWindowViewport( const int windowID, int x, int y, int width, int height );
  int fullscreenWindow( const int windowID );
  int decorateWindow( const int windowID, bool decorate );
  int setWindowCursor( const int windowID, arCursor cursor );
  int raiseWindow( const int windowID, arZOrder zorder );
  bool windowExists( const int windowID );
  arVector3 getWindowSize( const int windowID );
  arVector3 getWindowPos( const int windowID );
  arVector3 getMousePos( const int windowID );
  arCursor getWindowCursor( const int windowID );
  bool isStereo( const int windowID );
  bool isFullscreen( const int windowID );
  bool isDecorated( const int windowID );
  arZOrder getZOrder( const int windowID );
  int getBpp( const int windowID );
  std::string getTitle( const int windowID );
  std::string getXDisplay( const int windowID );
  void setTitle( const int windowID, const std::string& title );
  void setAllTitles( const std::string& baseTitle, bool overwrite=true );
  arGraphicsWindow* getGraphicsWindow( const int windowID );
  void returnGraphicsWindow( const int windowID );
  void setGraphicsWindow( const int windowID, arGraphicsWindow* graphicsWindow );
  int getNumWindows( void ) const;
  bool hasActiveWindows( void ) const;
  bool isFirstWindow( const int windowID ) const;
  int getFirstWindowID( void ) const;
  bool isThreaded( void ) const;
  void setThreaded( bool threaded );
  int deleteWindow( const int windowID );
  int deleteAllWindows( void );
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

