//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_WINDOW_H
#define AR_GRAPHICS_WINDOW_H

#include "arViewport.h"
#include "arSZGClient.h"
#include "arViewport.h"
#include "arGraphicsScreen.h"
#include "arCamera.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <string>
#include <vector>
using namespace std;

SZG_CALL void ar_defaultWindowInitCallback();

class arGraphicsWindow;

// Why do these callback classes exist? They act as wrappers for the
// actual draw callbacks. For instance, extra parameters can be passed-in
// upon creation, etc. This allows arGraphicsWindow code to be used with
// the arMasterSlaveFramework (which needs a arMasterSlaveFramework& passed
// in to its draw callback), for instance.

class SZG_CALL arWindowInitCallback {
  public:
    arWindowInitCallback() {}
    virtual ~arWindowInitCallback() {}
    virtual void operator()( arGraphicsWindow& w ) = 0;
  protected:
  private:
};

class SZG_CALL arRenderCallback {
  public:
    arRenderCallback() {}
    virtual ~arRenderCallback() {}
    virtual void operator()( arGraphicsWindow& w, arViewport& v) = 0;
    void enable( bool onoff ) { _enabled = onoff; }
    bool enabled() { return _enabled; }
  protected:
  private:
    bool _enabled;
};

class SZG_CALL arDefaultWindowInitCallback : public arWindowInitCallback {
  public:
    arDefaultWindowInitCallback() {}
    virtual ~arDefaultWindowInitCallback() {}
    virtual void operator()( arGraphicsWindow& );
  protected:
  private:
};

class SZG_CALL arDefaultRenderCallback : public arRenderCallback {
  public:
    arDefaultRenderCallback() {}
    virtual ~arDefaultRenderCallback() {}
    virtual void operator()( arGraphicsWindow&, arViewport& );
  protected:
  private:
};

class SZG_CALL arGraphicsWindow {
  public:
    arGraphicsWindow( arCamera* cam=0 );
    virtual ~arGraphicsWindow();
    bool configure( arSZGClient& client );
    void setInitCallback( arWindowInitCallback* callback );
    void setDrawCallback( arRenderCallback* callback );
    // This sets the camera for all viewports as well as future ones
    // Note that only a pointer is passed in, cameras are externally owned.
    arCamera* setCamera( arCamera* cam=0 );
    arCamera* getCamera(){ return _defaultCamera; }
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
    void lockViewports() { ar_mutex_lock( &_viewportLock ); }
    void unlockViewports() { ar_mutex_unlock( &_viewportLock ); }

    std::vector<arViewport>* getViewports();
    arViewport* getViewport( unsigned int vpindex );
    float getCurrentEyeSign() const { return _currentEyeSign; }
    bool draw();
  protected:
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
    arMutex _viewportLock;
    arWindowInitCallback* _initCallback;
    arRenderCallback* _drawCallback;
    arGraphicsScreen _defaultScreen;
    // This is the 'master' camera, used by viewports by default
    arCamera* _defaultCamera;
    float _currentEyeSign;
};


#endif        //  #ifndefARGRAPHICSWINDOW_H

