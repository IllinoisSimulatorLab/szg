//%Module szggraphics 0

// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include math.sip
%Include szgclient.sip
%Include opengl.sip

// global extra code
%ModuleHeaderCode
#include <string>
using namespace std;

PyObject* _getFloatPtrAttribute_AsList( float* ptr, int size ) {
  PyObject* ret = PyList_New(size);
  if (ret == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "unable to allocate list for float array object attribute");
  } else {
    for (int i=0; i<size; ++i) {
      PyList_SET_ITEM( ret, i, PyFloat_FromDouble( static_cast<double>(ptr[i]) ) );
    }
  }
  return ret;
}

int _setFloatPtrAttribute_FromList( PyObject* in, float*ptr, int size ) {
  if (!PyList_Check( in )) {
    PyErr_SetString( PyExc_TypeError, "Float C array attribute requires a list of Floats." );
    return 1;
  }
  if (PyList_Size( in ) != size) {
    PyErr_SetString( PyExc_ValueError, "Incorrect list size for C float array attribute." );
    return 1;
  }
  for (int i=0; i<size; ++i) {
    PyObject* item = PyList_GetItem( in, i );
    if (!PyFloat_Check( item )) {
      PyErr_SetString( PyExc_TypeError, "All list values for C float array attribute must be Floats." );
      return 1;
    } 
    ptr[i] = static_cast<float>( PyFloat_AsDouble( item ) );
  }
  return 0;
}

%End



class arHead {
%TypeHeaderCode
#include "arHead.h"
%End
 public:
  arHead();
  virtual ~arHead();

  virtual bool configure( arSZGClient& client );

  void setEyeSpacing( float spacing );
  float getEyeSpacing() const;

  void setMidEyeOffset( const arVector3& midEyeOffset );
  arVector3 getMidEyeOffset() const /TransferBack/;

  void setEyeDirection( const arVector3& eyeDirection );
  arVector3 getEyeDirection() const /TransferBack/;

  void setMatrix( const arMatrix4& matrix );
  virtual arMatrix4 getMatrix() const /TransferBack/;

  void setClipPlanes( float nearClip, float farClip ); 
  float getNearClip() const;
  float getFarClip() const;

  void setUnitConversion( float conv );
  float getUnitConversion() const;

  arVector3 getEyePosition( float eyeSign, const arMatrix4* useMatrix=0 ) const /TransferBack/;
  arVector3 getMidEyePosition( const arMatrix4* useMatrix=0 ) const /TransferBack/;
  arMatrix4 getMidEyeMatrix() const /TransferBack/;

  void setFixedHeadMode( bool onoff );
  bool getFixedHeadMode() const;
};


class arCamera {
%TypeHeaderCode
#include "arCamera.h"
%End
 public:
  arCamera();
  virtual ~arCamera();
  void setEyeSign( float eyeSign );
  float getEyeSign() const;
  void setScreen( arGraphicsScreen* screen );
    // C++ owns return value
  arGraphicsScreen* getScreen() const;

  virtual arMatrix4 getProjectionMatrix() /TransferBack/;
  virtual arMatrix4 getModelviewMatrix() /TransferBack/;
  virtual void loadViewMatrices();
  virtual arCamera* clone() /TransferBack/;
  virtual string type() const;
 private:
  arCamera( const arCamera& );
};


class arFrustumCamera: arCamera {
%TypeHeaderCode
#include "arFrustumCamera.h"
%End
 public:
  arFrustumCamera();
  //arFrustumCamera( const float* const frust, const float* const look );
  virtual ~arFrustumCamera();
  virtual arCamera* clone() const /TransferBack/;

  //void setFrustum( const float* frust )
  //void setLook   ( const float* look )
  void setSides  ( const arVector4& sides );
  void setPosition( const arVector3& pos );
  void setTarget( const arVector3& target );
  void setUp( const arVector3& up );

  void setSides(float left, float right, float bottom, float top);
  void setPosition(float x, float y, float z);
  void setTarget(float x, float y, float z);
  void setUp(float x, float y, float z);
  void setNearFar(float nearClip, float farClip);

  arVector4 getSides() const /TransferBack/;
  arVector3 getPosition() const /TransferBack/;
  arVector3 getTarget() const /TransferBack/;
  arVector3 getUp() const /TransferBack/;
  float getNear() const;
  float getFar() const;

  // The parameters to the projection matrix call 
  // ( glOrtho(...) or glFrustum(...) )
  float* frustum;
%GetCode
  sipPy = _getFloatPtrAttribute_AsList( sipCpp->frustum, 6 );
%End
%SetCode
  sipErr = _setFloatPtrAttribute_FromList( sipPy, sipCpp->frustum, 6 );
%End

  // The parameters to gluLookat(...)
  float* lookat;
%GetCode
  sipPy = _getFloatPtrAttribute_AsList( sipCpp->lookat, 9 );
%End
%SetCode
  sipErr = _setFloatPtrAttribute_FromList( sipPy, sipCpp->lookat, 9 );
%End

  private:
    arFrustumCamera( const arFrustumCamera& );
};


class arPerspectiveCamera: arFrustumCamera {
%TypeHeaderCode
#include "arPerspectiveCamera.h"
%End
 public:
  arPerspectiveCamera();
  virtual ~arPerspectiveCamera();
  float getFrustumData(int i);
  float getLookatData(int i);
  string __str__();
%MethodCode
    ostringstream s;
    s << "arPerspectiveCamera\n";
    s << "sides: arVector4" << sipCpp->getSides() << "\n";
    s << "near clip: " << sipCpp->getNear() << "\n";
    s << "far clip: " << sipCpp->getFar() << "\n";
    s << "position: arVector3" << sipCpp->getPosition() << "\n";
    s << "target: arVector3" << sipCpp->getTarget() << "\n";
    s << "up: arVector3" << sipCpp->getUp() << "\n";
    sipRes = new string( s.str() );
%End
  private:
    arPerspectiveCamera( const arPerspectiveCamera& );
};


class arOrthoCamera: arFrustumCamera {
%TypeHeaderCode
#include "arOrthoCamera.h"
%End
 public:
  arOrthoCamera();
  virtual ~arOrthoCamera();

  string __str__();
%MethodCode
    ostringstream s;
    s << "arOrthoCamera\n";
    s << "sides: arVector4" << sipCpp->getSides() << "\n";
    s << "near clip: " << sipCpp->getNear() << "\n";
    s << "far clip: " << sipCpp->getFar() << "\n";
    s << "position: arVector3" << sipCpp->getPosition() << "\n";
    s << "target: arVector3" << sipCpp->getTarget() << "\n";
    s << "up: arVector3" << sipCpp->getUp() << "\n";
    sipRes = new string( s.str() );
%End
  private:
    arOrthoCamera( const arOrthoCamera& );
};

class arVRCamera : arCamera {
  public:
    arVRCamera( arHead* head=0 );
    virtual ~arVRCamera();
    virtual string type() const;
    void setHead( arHead* head /TransferBack/ );
    arHead* getHead() const;
  private:
    arVRCamera( const arVRCamera& );
};


class arGraphicsScreen {
%TypeHeaderCode
#include "arGraphicsScreen.h"
%End
 public:
  arGraphicsScreen();
  virtual ~arGraphicsScreen();
  // These five functions are inherited from arCamera
  virtual bool configure( arSZGClient& client );
  virtual bool configure( const string& screenName, arSZGClient& client );

  void setCenter(const arVector3& center);
  void setNormal(const arVector3& normal);
  void setUp(const arVector3& up);
  void setDimensions(float width, float height);
  arVector3 getNormal() const /TransferBack/;
  arVector3 getUp() const /TransferBack/;
  arVector3 getCenter() const /TransferBack/;

  void setWidth( float width );
  void setHeight( float height );
  float getWidth() const;
  float getHeight() const;

  void setHeadMounted( bool hmd );
  bool getHeadMounted() const;

  void setTile( arVector4& tile );

  void setTile( int tileX, int numberTilesX, int tileY, int numberTilesY );
  arVector4 getTile();

  bool setUseFixedHeadMode( const string& usageMode );
  bool getIgnoreFixedHeadMode() const;
  bool getAlwaysFixedHeadMode() const;

  arVector3 getFixedHeadHeadPosition() const /TransferBack/;
  void setFixedHeadPosition( const arVector3& position );
  float getFixedHeadHeadUpAngle() const;
  void setFixedHeadHeadUpAngle( float angle );

 private:
  arGraphicsScreen( const arGraphicsScreen& );
};


class arViewport {
%TypeHeaderCode
#include "arViewport.h"
%End
 public:
  arViewport();
  arViewport( float left, float bottom, float width, float height,
              const arGraphicsScreen& screen,
              arCamera* cam,
              float eyeSign,
              GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha,
              GLenum oglDrawBuf,
              bool clearZBuf );
  arViewport( const arViewport& x );
  //arViewport& operator=( const arViewport& x );
  virtual ~arViewport();

  void setViewport( arVector4& viewport );
  void setViewport( float left, float bottom, float width, float height );
  arVector4 getViewport() const /TransferBack/;

  // If set from Python, Python must keep ownership
  void setScreen( const arGraphicsScreen& screen );

  // C++ owns return value
  arGraphicsScreen* getScreen();

  // The viewport owns its camera.  It makes a copy here and returns the copy's address.
  // C++ owns return value
  arCamera* setCamera( arCamera* a0 );

  // C++ owns return value
  arCamera* getCamera();

  void setEyeSign(float eyeSign);
  float getEyeSign();
  void setColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
  void clearDepthBuffer(bool flag);
  // e.g. GL_BACK_LEFT
  void setDrawBuffer( GLenum buf );
  GLenum getDrawBuffer();
  void activate();
};


class arWindowInitCallback {
%TypeHeaderCode
#include "arGraphicsWindow.h"
%End
  public:
    arWindowInitCallback();
    virtual ~arWindowInitCallback();
    virtual void operator()( arGraphicsWindow& w ) = 0;
};

class arRenderCallback {
%TypeHeaderCode
#include "arGraphicsWindow.h"
%End
  public:
    arRenderCallback();
    virtual ~arRenderCallback();
    virtual void operator()( arGraphicsWindow& w, arViewport& v) = 0;
    void enable( bool onoff );
    bool enabled();
};

class arDefaultWindowInitCallback : arWindowInitCallback {
%TypeHeaderCode
#include "arGraphicsWindow.h"
%End
  public:
    arDefaultWindowInitCallback();
    virtual ~arDefaultWindowInitCallback();
    virtual void operator()( arGraphicsWindow& );
};

class arDefaultRenderCallback : arRenderCallback {
%TypeHeaderCode
#include "arGraphicsWindow.h"
%End
  public:
    arDefaultRenderCallback();
    virtual ~arDefaultRenderCallback();
    virtual void operator()( arGraphicsWindow&, arViewport& );
};

class arGraphicsWindow {
%TypeHeaderCode
#include "arGraphicsWindow.h"
%End
  public:
    arGraphicsWindow( arCamera* cam=0 );
    virtual ~arGraphicsWindow();
    bool configure( arSZGClient& client );
    void setInitCallback( arWindowInitCallback* callback );
    void setDrawCallback( arRenderCallback* callback );

    // Set the camera for all viewports as well as future ones.
    // The camera passed is cloned, the return value is a pointer to the clone.
    // C++ owns return value
    arCamera* setCamera( arCamera* a0=0 );

    // C++ owns return value
    arCamera* getCamera();

    void setScreen( const arGraphicsScreen& screen );
    // C++ owns return value
    arGraphicsScreen* getScreen();

    // This sets the camera for just a single viewport
  // C++ owns return value
    arCamera* setViewportCamera( unsigned int a0, arCamera* a1 );

    // Sets the camera for two adjacent (in the list) viewports
  // C++ owns return value
    arCamera* setStereoViewportsCamera( unsigned int startVPIndex, arCamera* cam );

  // C++ owns return value
    arCamera* getViewportCamera( unsigned int a0 );

    void useOGLStereo( bool onoff );
    bool getUseOGLStereo() const;
    void addViewport(const arViewport&);
    // NOTE: the following two routines invalidate any externally held pointers
    // to individual viewport cameras (a pointer to the window default camera,
    // returned by getCamera() or setCamera(), will still be valid).
    bool setViewMode( const string& viewModeString );
    void clearViewportList();
    void lockViewports();
    void unlockViewports();

    SIP_PYTUPLE getViewports();
%MethodCode
      vector<arViewport>* vp = sipCpp->getViewports();
      // Create the tuple.
      if ((sipRes = PyTuple_New(vp->size())) == NULL) {
        PyErr_SetString( PyExc_RuntimeError, "getViewports() failed to create output tuple." );
        sipIsErr = 1;
      } else {
        // Set the tuple elements.
        vector<arViewport>::iterator iter;
        int i(0);
        for (iter = vp->begin(); iter != vp->end(); ++iter) {
          // create a wrapper for the arViewport but _leave ownership with C++_!!!
          PyObject *vpObj = sipConvertFromInstance( &*iter, sipClass_arViewport, NULL );
          if (vpObj == NULL) {
            Py_DECREF( sipRes );
            sipRes = NULL;
            sipIsErr = 1;
            break;
          }
          PyTuple_SET_ITEM( sipRes, i++, vpObj );
        }
      }
%End

  // C++ owns return value
    arViewport* getViewport( unsigned int vpindex );
    float getCurrentEyeSign() const;
    void setPixelDimensions( int posX, int posY, int sizeX, int sizeY );
    void getPixelDimensions( int& posX /Out/, int& posY /Out/, int& sizeX /Out/, int& sizeY /Out/ ) const;
    bool draw();
  private:
    arGraphicsWindow( const arGraphicsWindow& x );
};

