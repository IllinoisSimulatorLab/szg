//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_CLIENT_H
#define AR_GRAPHICS_CLIENT_H

#include "arGraphicsDatabase.h"
#include "arSyncDataClient.h"
#include "arGraphicsWindow.h"
#include "arFrameworkObject.h"
#include <string>

/// Something that renders graphics data
/// using the SZG_GEOMETRY service
/// and draws OpenGL scenes from an arGraphicsDatabase.

class arGraphicsClient{
  // Needs assignment operator and copy constructor, for pointer members.
  friend bool 
    ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*);
  friend bool ar_graphicsClientDisconnectCallback(void*);
  friend void ar_graphicsClientDrawEye(arGraphicsClient*,
                                       arScreenObject*,
                                       float);
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

  void monoEyeOffset( const string& eye );

  //AARGH! bad design...
  void loadAlphabet(const char*);
  void setTexturePath(const string&);
  void setStereoMode(bool);
  void setViewMode( const std::string& );
  void setDemoMode(bool);
  void setScreenObject(arScreenObject*);
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
  /// the setCamera function can be used to cycle through the cameras attached
  /// to a database
  void setCamera(int cameraID);
  /// the setLocalCamera function can be used to create an arbitrary camera
  /// this camera has ID = -2, inelegant for sure
  void setLocalCamera(float* frustum, float* lookat);
  /// Allows the arDistSceneGraphFramework standalone mode to have a simulator
  /// interface. I'm a little bit annoyed at how the arGraphicsClient is
  /// so greedy to be in charge (though maybe that's necessary given the
  /// way the callbacks have been defined).
  void setSimulator(arFrameworkObject* f){ _simulator = f; }

  arSyncDataClient   _cliSync;
  
 protected:
  // I THINK THAT THE _screenObject IS NO LONGER REALLY USED. THIS HAS
  // BEEN ROLLED UP INTO THE _graphicsWindow. DO NOT USE!!!!
  arScreenObject*    _screenObject;
  arGraphicsDatabase _graphicsDatabase;
  arGraphicsWindow   _graphicsWindow;
  // we can over-ride the default draw function with this
  void (*_drawFunction)(arGraphicsDatabase*);
  bool _showFramerate;
  bool _stereoMode;
  arVector3 _overrideColor;
  arFrameworkObject* _simulator;
};

#endif
