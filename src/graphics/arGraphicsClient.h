//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_CLIENT_H
#define AR_GRAPHICS_CLIENT_H

#include "arGraphicsDatabase.h"
#include "arSyncDataClient.h"
#include "arGraphicsWindow.h"
#include "arVRCamera.h"
#include "arFrameworkObject.h"
#include <string>

/// Something that renders graphics data
/// using the SZG_GEOMETRY service
/// and draws OpenGL scenes from an arGraphicsDatabase.

class SZG_CALL arGraphicsClient{
  // Needs assignment operator and copy constructor, for pointer members.
  friend bool 
    ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*);
  friend bool ar_graphicsClientDisconnectCallback(void*);
  friend void ar_graphicsClientDraw(arGraphicsClient* client, arCamera*);
  friend bool ar_graphicsClientConsumptionCallback(void*, ARchar*);
  friend bool ar_graphicsClientActionCallback(void*);
  friend bool ar_graphicsClientNullCallback(void*);
  friend bool ar_graphicsClientPostSyncCallback(void*);
 public:
  arGraphicsClient();
  ~arGraphicsClient();

  bool configure(arSZGClient*);
  arGraphicsWindow* getGraphicsWindow(){ return &_graphicsWindow; }
  // Gets called after the graphics window has been created to do some
  // initialization (so far only used for the wildcats)
  void init();

//  void monoEyeOffset( const string& eye );

  //AARGH! bad design...
  void loadAlphabet(const char*);
  void setTexturePath(const string&);
  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName, const string& bundlePath);
  void setStereoMode(bool);
  void setViewMode( const std::string& );
  void setFixedHeadMode(bool);
  void showFramerate(bool);

  // Sometimes, we want to be able to over-ride the super-controlled
  // defaults... this is a rough hack so far and the interface is
  // likely to change
  void setDrawFunction(void (*drawFunction)(arGraphicsDatabase*));

  void setNetworks(string networks);
  bool start(arSZGClient&);

  bool empty() { return _graphicsDatabase.empty(); }
  void reset() { _graphicsDatabase.reset(); }

  void setOverrideColor(arVector3 overrideColor);
  // copy the head from the arViewerNode to here
  bool updateHead();

  /// the setCamera function can be used to cycle through the cameras attached
  /// to a database
  arCamera* setWindowCamera(int cameraID);
  arCamera* setViewportCamera(unsigned int vpid, int cameraID);
  arCamera* setStereoViewportsCamera(unsigned int vpid, int cameraID);
  /// the setLocalCamera function can be used to create an arbitrary camera
  /// this camera has ID = -2, inelegant for sure
  arCamera* setWindowLocalCamera( const float* const frust, const float* const look );
  arCamera* setViewportLocalCamera( unsigned int vpid, const float* const frust, const float* const look );
  arCamera* setStereoViewportsLocalCamera( unsigned int vpid, const float* const frust, const float* const look );
  /// Allows the arDistSceneGraphFramework standalone mode to have a simulator
  /// interface. I'm a little bit annoyed at how the arGraphicsClient is
  /// so greedy to be in charge (though maybe that's necessary given the
  /// way the callbacks have been defined).
  void setSimulator(arFrameworkObject* f){ _simulator = f; }
  void toggleFrameworkObjects(){ 
    _drawFrameworkObjects = !_drawFrameworkObjects; 
  }
  void drawFrameworkObjects(bool state){
    _drawFrameworkObjects = state;
  }
  void addFrameworkObject(arFrameworkObject* f){ 
    _frameworkObjects.push_back(f); 
  }

  arSyncDataClient   _cliSync;
  
 protected:
  arGraphicsDatabase _graphicsDatabase;
  arGraphicsWindow   _graphicsWindow;
  bool _fixedHeadMode;
  arHead _defaultHead;
  arVRCamera _defaultCamera;
  // we can over-ride the default draw function with this
  void (*_drawFunction)(arGraphicsDatabase*);
  bool _showFramerate;
  bool _stereoMode;
  arVector3 _overrideColor;
  arFrameworkObject* _simulator;
  bool _drawFrameworkObjects;
  list<arFrameworkObject*> _frameworkObjects;
};

#endif
