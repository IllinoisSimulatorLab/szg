//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_CLIENT_H
#define AR_GRAPHICS_CLIENT_H

#include "arGraphicsDatabase.h"
#include "arSyncDataClient.h"
#include "arGraphicsWindow.h"
#include "arVRCamera.h"
#include "arFrameworkObject.h"
#include "arGUIWindow.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include "arGraphicsCalling.h"

#include <string>

// Render graphics data using the SZG_GEOMETRY service
// and draw OpenGL scenes from an arGraphicsDatabase.

class arGraphicsClient;
class arGraphicsClientRenderCallback;
bool ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*);
bool ar_graphicsClientDisconnectCallback(void*);
void ar_graphicsClientDraw(arGraphicsClient*, arGraphicsWindow&, arViewport&);
bool ar_graphicsClientConsumptionCallback(void*, ARchar*);
bool ar_graphicsClientActionCallback(void*);
bool ar_graphicsClientNullCallback(void*);
bool ar_graphicsClientPostSyncCallback(void*);

class SZG_CALL arGraphicsClient{
  // Needs assignment operator and copy constructor, for pointer members.

  friend class arGraphicsClientRenderCallback;
  friend bool ar_graphicsClientConnectionCallback(void*, arTemplateDictionary*);
  friend bool ar_graphicsClientDisconnectCallback(void*);
  friend void ar_graphicsClientDraw(arGraphicsClient*, arGraphicsWindow&, arViewport&);
  friend bool ar_graphicsClientConsumptionCallback(void*, ARchar*);
  friend bool ar_graphicsClientActionCallback(void*);
  friend bool ar_graphicsClientNullCallback(void*);
  friend bool ar_graphicsClientPostSyncCallback(void*);

 public:
  arGraphicsClient();
  ~arGraphicsClient() {}

  void setWindowManager(arGUIWindowManager* wm) { _windowManager = wm; }
  arGUIWindowManager* getWindowManager() { return _windowManager; }
  arGraphicsWindow* getGraphicsWindow(int ID) { return _windowManager->getGraphicsWindow( ID ); }
  bool configure(arSZGClient*);
  //arGraphicsWindow* getGraphicsWindow() { return &_graphicsWindow; }

  // Gets called after the graphics window has been created to do some
  // initialization.
  // void init();

//  void monoEyeOffset( const string& eye );

  //AARGH! bad design...
  void loadAlphabet(const char*);
  void setTexturePath(const string&);
  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName, const string& bundlePath);
  //void setStereoMode(bool);
  //void setViewMode( const std::string& );
  //void setFixedHeadMode(bool);
  //void showFramerate(bool);

  // To over-ride the super-controlled
  // defaults... this is a rough hack and the interface is likely to change
  //void setDrawFunction(void (*drawFunction)(arGraphicsDatabase*));

  void setNetworks(string networks);
  bool start(arSZGClient&, bool startSynchronization=true);

  bool empty() { return _graphicsDatabase.empty(); }
  void reset() { _graphicsDatabase.reset(); }

  void setOverrideColor(arVector3 overrideColor);
  // copy the head from the arViewerNode to here
  bool updateHead();

  // Give the arDistSceneGraphFramework standalone mode a simulator
  // interface. I'm a little bit annoyed at how the arGraphicsClient is
  // so greedy to be in charge (though maybe that's necessary given the
  // way the callbacks have been defined).
  void setSimulator(arFrameworkObject* f) { _simulator = f; }
  void showSimulator(bool show) { _showSimulator = show; }
  void toggleFrameworkObjects() {
    _drawFrameworkObjects = !_drawFrameworkObjects;
  }
  void drawFrameworkObjects(bool fDraw) {
    _drawFrameworkObjects = fDraw;
  }
  void addFrameworkObject(arFrameworkObject* f) {
    _frameworkObjects.push_back(f);
  }
  void drawAllWindows() {
    _windowManager->drawAllWindows(true); // Simultaneously if threaded.  Blocks.
  }
  void requestScreenshot(const string& path, int x, int y, int width, int height);
  bool screenshotRequested();
  void takeScreenshot(bool fStereo);
  void render(arGUIWindowInfo&, arGraphicsWindow&);

  arSyncDataClient   _cliSync;

 protected:
  arGraphicsDatabase _graphicsDatabase;
  arGraphicsWindow   _graphicsWindow;

  arGUIWindowManager* _windowManager;
  arGUIXMLParser*     _guiParser;

  //bool _fixedHeadMode;
  arHead _defaultHead;
  arVector3 _overrideColor;
  arFrameworkObject* _simulator;
  bool _showSimulator;
  bool _drawFrameworkObjects;
  list<arFrameworkObject*> _frameworkObjects;

  string _screenshotPath;
  int  _screenshotX;
  int  _screenshotY;
  int  _screenshotWidth;
  int  _screenshotHeight;
  bool _doScreenshot;
  int _whichScreenshot;
};

#endif
