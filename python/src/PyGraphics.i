// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).


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

enum arGraphicsStateValue{
  AR_G_FALSE= 0,
  AR_G_TRUE = 1,
  AR_G_SMOOTH = 2,
  AR_G_FLAT = 3,
  AR_G_ZERO = 4,
  AR_G_ONE = 5,
  AR_G_DST_COLOR = 6,
  AR_G_SRC_COLOR = 7,
  AR_G_ONE_MINUS_DST_COLOR = 8,
  AR_G_ONE_MINUS_SRC_COLOR = 9,
  AR_G_SRC_ALPHA = 10,
  AR_G_ONE_MINUS_SRC_ALPHA = 11,
  AR_G_DST_ALPHA = 12,
  AR_G_ONE_MINUS_DST_ALPHA = 13,
  AR_G_SRC_ALPHA_SATURATE = 14
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

int dgStateInt(const string& nodeName, const string& parentName,
               const string& stateName,
               arGraphicsStateValue val1,
               arGraphicsStateValue val2 = AR_G_FALSE );
bool dgStateInt(int nodeID, const string& stateName,
                arGraphicsStateValue val1, 
                arGraphicsStateValue val2 = AR_G_FALSE);
int dgStateFloat(const string& nodeName, const string& parentName,
                 const string& stateName, float value );
bool dgStateFloat(int nodeID, const string& stateName, float value);
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

int dgMaterial(const string& name, const string& parent, 
               const arVector3& diffuse,
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

int dgBumpMap(const string& name, const string& parent,
              const string& filename, float height=1.);

int dgBumpMap(int ID, const string& filename, float height=1.);

int dgPlugin(const string& name,
              const string& parent,
              const string& fileName,
               vector<int> intData,
               vector<float> floatData,
               vector<long> longData,
               vector<double> doubleData,
               vector<string> stringData );

bool dgPlugin( int ID, const string& fileName, 
               vector<int> intData,
               vector<float> floatData,
               vector<long> longData,
               vector<double> doubleData,
               vector<string> stringData );


%{
#include "arGraphicsPluginNode.h"
#include "arGraphicsPlugin.h"

class arPythonGraphicsPluginObject: public arGraphicsPlugin {
  public:
    arPythonGraphicsPluginObject( const std::string& fileName );
    virtual ~arPythonGraphicsPluginObject();
    virtual void draw( arGraphicsWindow& win, arViewport& view );
    virtual bool setState( const std::vector<int>& intData,
                           const std::vector<long>& longData,
                           const std::vector<float>& floatData,
                           const std::vector<double>& doubleData,
                           const std::vector< std::string >& stringData );
  private:
    arGraphicsPlugin* _makeObject();
    arGraphicsPlugin* _object;
    std::string _fileName;
};
    
arPythonGraphicsPluginObject::arPythonGraphicsPluginObject( const std::string& fileName ) :
  arGraphicsPlugin(),
  _fileName( fileName ) {
  _object = _makeObject();
  if (!_object) {
    ar_log_error() << "Failed to create object from plugin" << _fileName << ar_endl;
  }
}

arPythonGraphicsPluginObject::~arPythonGraphicsPluginObject() {
  if (_object) {
    delete _object;
  }
}

void arPythonGraphicsPluginObject::draw( arGraphicsWindow& win, arViewport& view ) {
  if (!_object) {
    return;
  }
  _object->draw( win, view );
}

bool arPythonGraphicsPluginObject::setState( const std::vector<int>& intData,
                       const std::vector<long>& longData,
                       const std::vector<float>& floatData,
                       const std::vector<double>& doubleData,
                       const std::vector< std::string >& stringData ) {
  if (!_object) {
    return false;
  }
  // Explanation: the arGraphicsPlugin wants non-const vectors.
  // But we only have typemaps defined for const vectors in the
  // SWIG python bindings.
  std::vector<int> iData(intData);
  std::vector<long> lData(longData);
  std::vector<float> fData(floatData);
  std::vector<double> dData(doubleData);
  std::vector< std::string > sData(stringData);
  return _object->setState( iData, lData, fData, dData, sData );
}

arGraphicsPlugin* arPythonGraphicsPluginObject::_makeObject() {
  arSharedLib* lib = arGraphicsPluginNode::getSharedLib( _fileName );
  if (!lib) {
    ar_log_error() << "arPythonGraphicsPluginObject failed to load shared library "
                   << _fileName << ar_endl;
    return NULL;
  } 
  arGraphicsPlugin* obj = (arGraphicsPlugin*)lib->createObject();
  if (!obj) {
    ar_log_error() << "arPythonGraphicsPluginObject failed to create object from shared library "
                   << _fileName << ar_endl;
    return NULL;
  }
  return obj;
}

%}

class arPythonGraphicsPluginObject: public arGraphicsPlugin {
  public:
    arPythonGraphicsPluginObject( const string& fileName );
    virtual ~arPythonGraphicsPluginObject();
    virtual void draw( arGraphicsWindow& win, arViewport& view );
    virtual bool setState( const vector<int>& intData,
                           const vector<long>& longData,
                           const vector<float>& floatData,
                           const vector<double>& doubleData,
                           const vector<string>& stringData );
};


%pythoncode %{
class arTeapotGraphicsPlugin( arPythonGraphicsPluginObject ):
  """Class for loading and drawing the arTeapotGraphicsPlugin
     shared library in Python master/slave programs."""
  def __init__(self):
    arPythonGraphicsPluginObject.__init__(self, "arTeapotGraphicsPlugin")
    self.color = [.7,.7,.7,1.] # Note color must always be a 3- or 4-element list of _floats_.
                               # (ints will _not_ work).
    self._dirty = True
  def __setattr__(self, attrName, value):
    if attrName == 'color':
      self._dirty = True
    arPythonGraphicsPluginObject.__setattr__( self, attrName, value)
  def updateState(self):
    if self._dirty:
      arPythonGraphicsPluginObject.setState( self, [], [], self.color, [], [] )
      self._dirty = False
  def draw( self, graphicsWin, viewport ):
    self.updateState()
    arPythonGraphicsPluginObject.draw( self, graphicsWin, viewport )
%}


%{
#include "arTexture.h"
%}

void ar_setTextureAllowNotPowOf2( bool onoff );
bool ar_getTextureAllowNotPowOf2();


class arTexture {
 public:
  arTexture();
  arTexture( const arTexture& rhs, unsigned int left, unsigned int bottom, 
                                   unsigned int width, unsigned int height );
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
/*  arLargeImage& operator=( const arLargeImage& x );*/
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


class arMaterial {
 public:
  arMaterial();
  arMaterial(const arMaterial&);
  ~arMaterial();

  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  arVector3 emissive;
  float exponent; 
  float alpha; 
%extend{
  string __str__(void){
    stringstream s;
    s << "arMaterial\n";
    s << "diffuse:  arVector3" << self->diffuse << "\n";
    s << "ambient:  arVector3" << self->ambient << "\n";
    s << "specular: arVector3" << self->specular << "\n";
    s << "emissive: arVector3" << self->emissive << "\n";
    s << "exponent: " << self->exponent << "\n";
    s << "alpha:    " << self->alpha << "\n";
    return s.str();
  }
}
};


%{
#include "arTexFont.h"
%}

class arTextBox {
 public:
  arTextBox();
  ~arTextBox();

  arVector3 color;
  int tabWidth;
  float lineSpacing;
  int columns;
  float width;
  arVector3 upperLeft;
};

class arTexFont {
 public:
  arTexFont();
  ~arTexFont();
  
  bool load( const string& fontFilePath,
             int transparentColor=0 );
  void setFontTexture( const arTexture& newFont );
  float characterWidth();
  float characterHeight();
  float lineHeight(arTextBox& format);
  float getTextWidth(const string& text, arTextBox& format);
  float getTextHeight(const string& text, arTextBox& format);
  void renderString(const string& text, arTextBox& format);
  bool renderFile(const string& filename, arTextBox& format);
%extend{
PyObject* lineFeed( int& currentColumn, int& currentRow, arTextBox& format ) {
  int cc = currentColumn;
  int cr = currentRow;
  self->lineFeed( cc, cr, format );
  PyObject* val = PyTuple_New( 2 );
  if (!val) {
    PyErr_SetString( PyExc_MemoryError, "unable to allocate new tuple for arTexFont.lineFeed return values." );
    return NULL;
  }
  PyTuple_SetItem( val, 0, PyInt_FromLong((long)cc) );
  PyTuple_SetItem( val, 1, PyInt_FromLong((long)cr) );
  return val;
}
PyObject* advanceCursor( int& currentColumn, int& currentRow, arTextBox& format ) {
  int cc = currentColumn;
  int cr = currentRow;
  self->advanceCursor( cc, cr, format );
  PyObject* val = PyTuple_New( 2 );
  if (!val) {
    PyErr_SetString( PyExc_MemoryError, "unable to allocate new tuple for arTexFont.advanceCursor return values." );
    return NULL;
  }
  PyTuple_SetItem( val, 0, PyInt_FromLong((long)cc) );
  PyTuple_SetItem( val, 1, PyInt_FromLong((long)cr) );
  return val;
}
PyObject* renderGlyph( int c, int& currentColumn, int& currentRow, arTextBox& format ) {
  int cc = currentColumn;
  int cr = currentRow;
  self->renderGlyph( c, cc, cr, format );
  PyObject* val = PyTuple_New( 2 );
  if (!val) {
    PyErr_SetString( PyExc_MemoryError, "unable to allocate new tuple for arTexFont.renderGlyph return values." );
    return NULL;
  }
  PyTuple_SetItem( val, 0, PyInt_FromLong((long)cc) );
  PyTuple_SetItem( val, 1, PyInt_FromLong((long)cr) );
  return val;
}
}

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
  string __str__(void){
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
#include "arAxisAlignedBoundingBox.h"
%}
class arAxisAlignedBoundingBox{
 public:
  arAxisAlignedBoundingBox(){ xSize=ySize=zSize=0; }
  ~arAxisAlignedBoundingBox(){}
 
  arVector3 center;
  float xSize;
  float ySize;
  float zSize;
%extend{
  string __str__(void){
    ostringstream s;
    s << "arAxisAlignedBoundingBox(center=";
    s << self->center << ", size=(" << self->xSize << "," << self->ySize << "," << self->zSize << "))"; 
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
  virtual ~arCamera();
  void setEyeSign( float eyeSign );
  float getEyeSign() const;
  void setScreen( arGraphicsScreen* screen );
  arGraphicsScreen* getScreen() const;
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();
};

class arFrustumCamera: public arCamera{
 public:
  arFrustumCamera();
  arFrustumCamera( const float* const frust, const float* const look );
  virtual ~arFrustumCamera();
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual std::string type() const;

  void setFrustum( const float* frust );
  void setLook   ( const float* look );
  void setSides  ( const arVector4& sides );
  void setPosition( const arVector3& pos );
  void setTarget( const arVector3& target );
  void setUp( const arVector3& up );
  void setSides(float left, float right, float bottom, float top);
  void setPosition(float x, float y, float z);
  void setTarget(float x, float y, float z);
  void setUp(float x, float y, float z);
  void setNearFar(float nearClip, float farClip);

  arVector4 getSides() const;
  arVector3 getPosition() const;
  arVector3 getTarget() const;
  arVector3 getUp() const;
  float getNear() const;
  float getFar() const;
};

class arPerspectiveCamera: public arFrustumCamera{
 public:
  arPerspectiveCamera();
  virtual ~arPerspectiveCamera();
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();

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

class arOrthoCamera: public arFrustumCamera{
 public:
  arOrthoCamera();
  virtual ~arOrthoCamera();
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();

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
/** * States that an OS event can be in.  *
 * Also used as the 'to-do' message detail in passing requests from
 * arGUIWindowManager to arGUIWindow (i.e., in this case AR_WINDOW_MOVE means
 * "move the window" rather than its normal "the window has moved").
 */
enum arGUIState {
  AR_GENERIC_STATE,         // Placeholder state (for default constructors).
  AR_KEY_DOWN,              // A key has been pressed.
  AR_KEY_UP,                // A key has been released.
  AR_KEY_REPEAT,            // A key was pressed and is continuing to be pressed.
  AR_MOUSE_DOWN,            // A mouse button was pressed.
  AR_MOUSE_UP,              // A mouse button was released.
  AR_MOUSE_MOVE,            // The mouse has moved.
  AR_MOUSE_DRAG,            // A mouse button is held down and the mouse is being moved.
  AR_WINDOW_MOVE,           // The window has been moved.
  AR_WINDOW_RESIZE,         // The window has been resized.
  AR_WINDOW_CLOSE,          // The window has been closed.
  AR_WINDOW_FULLSCREEN,     // Change the window to fullscreen.
  AR_WINDOW_DECORATE,       // Change the window's decoration state.
  AR_WINDOW_RAISE,          // Change the window's z order.
  AR_WINDOW_CURSOR,         // Change the window's cursor.
  AR_WINDOW_DRAW,           // Draw the window.
  AR_WINDOW_SWAP,           // Swap the window's buffers.
  AR_WINDOW_VIEWPORT,       // Set the window's viewport.
  AR_WINDOW_INITGL,         // Initialize the window's opengl context.
  AR_NUM_GUI_STATES         // The number of different event states.
};

class arGUIWindowInfo {
  public:
    // inherited from arGUIInfo
    int getWindowID();
    int getPosX( void ) const;
    int getPosY( void ) const;
    int getSizeX( void ) const;
    int getSizeY( void ) const;
    arGUIState getState( void ) const;
    arGUIWindowManager* getWindowManager( void ) const;

%pythoncode %{
    def __str__(self):
      return "arGUIWindowInfo ID=%d, State=%d, Position=(%d,%d), Size=(%d,%d)" % \
        (self.getWindowID(),self.getState(),self.getPosX(),self.getPosY(),self.getSizeX(),self.getSizeY())
%}
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

class arRenderCallback {
  public:
    arRenderCallback();
    virtual ~arRenderCallback();
    virtual void operator()( arGraphicsWindow& w, arViewport& v) = 0;
    void enable( bool onoff );
    bool enabled();
};

class arGUIRenderCallback : public arRenderCallback{
  public:
    arGUIRenderCallback( void );
    virtual ~arGUIRenderCallback( void );
    virtual void operator()( arGraphicsWindow&, arViewport& ) = 0;
    virtual void operator()( arGUIWindowInfo* windowInfo,
                             arGraphicsWindow* graphicsWindow ) = 0;
    virtual void operator()( arGUIWindowInfo* windowInfo ) = 0;

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

