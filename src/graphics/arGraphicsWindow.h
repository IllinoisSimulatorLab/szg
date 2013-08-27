//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_WINDOW_H
#define AR_GRAPHICS_WINDOW_H

#include "arViewport.h"
#include "arSZGClient.h"
#include "arViewport.h"
#include "arGraphicsScreen.h"
#include "arCamera.h"
#include "arGraphicsCalling.h"

#include <string>
#include <vector>
using namespace std;

SZG_CALL void ar_defaultWindowInitCallback();

class arGraphicsWindow;

// These callback wrap the actual draw callbacks, so extra parameters
// can be passed in upon creation. This lets arGraphicsWindow code
// be used with arMasterSlaveFramework, whose draw callback needs
// an arMasterSlaveFramework&.

class SZG_CALL arWindowInitCallback {
  public:
    virtual ~arWindowInitCallback() {}
    virtual void operator()( arGraphicsWindow& w ) = 0;
};
class SZG_CALL arDefaultWindowInitCallback : public arWindowInitCallback {
  public:
    virtual ~arDefaultWindowInitCallback() {}
    virtual void operator()( arGraphicsWindow& );
};

class SZG_CALL arRenderCallback {
  public:
    virtual ~arRenderCallback() {}
    virtual void operator()( arGraphicsWindow& w, arViewport& v) = 0;
    void enable( bool onoff ) { _enabled = onoff; }
    bool enabled() const { return _enabled; }
  private:
    bool _enabled;
};
class SZG_CALL arDefaultRenderCallback : public arRenderCallback {
  public:
    virtual ~arDefaultRenderCallback() {}
    virtual void operator()( arGraphicsWindow&, arViewport& );
};

class SZG_CALL arGraphicsWindow {
  public:
    arGraphicsWindow( arCamera* cam=0 );
    virtual ~arGraphicsWindow();
    bool configure( arSZGClient& client );
    void setInitCallback( arWindowInitCallback* callback );
    void setDrawCallback( arRenderCallback* callback );
    // Set the camera for all viewports as well as future ones.
    // Only a pointer is passed in: cameras are externally owned.
    arCamera* setCamera( arCamera* cam=0 );
    arCamera* getCamera() { return _defaultCamera; }
    void setScreen( const arGraphicsScreen& screen ) { _defaultScreen = screen; }
    arGraphicsScreen* getScreen( void ) { return &_defaultScreen; }
    // This sets the camera for just a single viewport
    arCamera* setViewportCamera( unsigned int vpindex, arCamera* cam );
    // Sets the camera for two adjacent (in the list) viewports
    arCamera* setStereoViewportsCamera( unsigned int startVPIndex, arCamera* cam );
    arCamera* getViewportCamera( unsigned int vpindex );
    void useOGLStereo( bool onoff ) { _useOGLStereo = onoff; }
    bool getUseOGLStereo() const { return _useOGLStereo; }
    void addViewport(const arViewport&);
    // NOTE: the following two routines invalidate any externally held pointers
    // to individual viewport cameras (a pointer to the window default camera,
    // returned by getCamera() or setCamera(), will still be valid).
    bool setViewMode( const std::string& viewModeString );
    void clearViewportList();
    void lockViewports() { _viewportLock.lock("arGraphicsWindow::lockViewports"); }
    void unlockViewports() { _viewportLock.unlock(); }

    std::vector<arViewport>* getViewports();
    arViewport* getViewport( unsigned int vpindex );
    float getCurrentEyeSign() const { return _currentEyeSign; }
    void setPixelDimensions( int posX, int posY, int sizeX, int sizeY );
    void getPixelDimensions( int& posX, int& posY, int& sizeX, int& sizeY ) const;
    bool draw();

  private:
    // not safe to copy yet.
    arGraphicsWindow( const arGraphicsWindow& x );
    arGraphicsWindow& operator=( const arGraphicsWindow& x );
    void _renderPass( GLenum oglDrawBuf );
    void _applyColorFilter();
    bool _configureCustomViewport( const std::string& screenName, arSZGClient& client, bool masterViewport=false );
    void _addViewportNoLock( const arViewport& );
    bool _setViewModeNoLock( const std::string& viewModeString );
    void _clearViewportListNoLock();
    arCamera* _setCameraNoLock( arCamera* cam );

    bool             _useOGLStereo;
    bool             _useColorFilter;
    arVector3        _colorScaleFactors;
    arVector4        _contrastFilterParameters;
    std::vector<arViewport> _viewportVector;
    arLock _viewportLock;
    arWindowInitCallback* _initCallback;
    arRenderCallback* _drawCallback;
    // This is the 'master' screen, used by viewports by default
    arGraphicsScreen _defaultScreen;
    // This is the 'master' camera, used by viewports by default
    arCamera* _defaultCamera;
    float _currentEyeSign;
    // todo: decopypaste from arGUIInfo.h
    int _posX;      // The x position of the window.
    int _posY;      // The y position of the window.
    int _sizeX;     // The width of the window.
    int _sizeY;     // The height of the window.
};

#endif
