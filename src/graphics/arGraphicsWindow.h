//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_WINDOW_H
#define AR_GRAPHICS_WINDOW_H

#include "arViewport.h"
#include "arScreenObject.h"
#include "arSZGClient.h"
#include <string>
#include <list>
using namespace std;

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

enum arViewModeEnum { VIEW_NORMAL=0,
                      VIEW_ANAGLYPH=1,
                      VIEW_WALLEYED=2,
                      VIEW_CROSSEYED=3,
                      VIEW_OVERUNDER=4,
                      VIEW_INTERLACED=5,
                      VIEW_CUSTOM=6 };

class SZG_CALL arGraphicsWindow {
  public:
    arGraphicsWindow();
    virtual ~arGraphicsWindow();
    bool configure(arSZGClient*);
    void setInitCallback( arWindowInitCallback* callback );
    void setDrawCallback( arRenderCallback* callback );
    void setCameraFactory( arCamera* (*factory)() ){
      _cameraFactory = factory;
    }
    void useOGLStereo( bool onoff ) { _useOGLStereo = onoff; }
    void setDefaultEye( const std::string& eye );
    void setDefaultCamera( arCamera* camera);
    arCamera* getDefaultCamera(){ return _defaultCamera; }
    bool setViewMode( const std::string& viewModeString );
    void addViewport(const arViewport&);
    void clearViewportList();
    list<arViewport>* getViewportList();
    bool draw();
  protected:
  private:
    // not safe to copy yet.
    arGraphicsWindow( const arGraphicsWindow& x );
    arGraphicsWindow& operator=( const arGraphicsWindow& x );
    bool             _useOGLStereo;
    float            _defaultEyeSign;
    arViewModeEnum   _viewMode;
    list<arViewport> _viewportList;
    // the default camera is used, for instance, to deal w/ the
    // preset view modes and with getting the head information into
    // and out of the graphics window. BOTH OF THESE USES NEED TO BE
    // CLEANED UP AND ELIMINATED! The conflation of the arScreenObject
    // with head information is especially pernicious.
    arCamera*        _defaultCamera;

    arWindowInitCallback* _initCallback;
    arRenderCallback* _drawCallback;
    arCamera* (*_cameraFactory)();

    void _renderPass( float eyeSign );
    arCamera* _parseViewport(arSZGClient*, const string&);
    arMutex _viewportListLock;
};


#endif        //  #ifndefARGRAPHICSWINDOW_H

