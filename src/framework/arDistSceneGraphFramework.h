//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DIST_SCENE_GRAPH_FRAMEWORK
#define AR_DIST_SCENE_GRAPH_FRAMEWORK

#include "arGraphicsServer.h"
#include "arGraphicsPeer.h"
#include "arGraphicsClient.h"
#include "arGraphicsWindow.h"
#include "arSoundClient.h"
#include "arVRConstants.h"
#include "arHeadWandSimulator.h"
#include "arFramerateGraph.h"
#include "arSZGAppFramework.h"

/// Framework for cluster applications using a distributed scene graph.
class SZG_CALL arDistSceneGraphFramework : public arSZGAppFramework {
  friend void ar_distSceneGraphFrameworkMessageTask(void*);
  friend void ar_distSceneGraphFrameworkWindowTask(void*);
  friend void ar_distSceneGraphFrameworkDisplay();
  friend void ar_distSceneGraphFrameworkButtonFunction(int, int, int, int);
  friend void ar_distSceneGraphFrameworkMouseFunction(int, int);
  friend void ar_distSceneGraphFrameworkKeyboard(unsigned char, int, int);
 public:
  arDistSceneGraphFramework();
  ~arDistSceneGraphFramework() {}

  arGraphicsDatabase* getDatabase();

  void setUserMessageCallback(
    void (*userMessageCallback)( arDistSceneGraphFramework&, const string& messageBody ));

  // inherited pure virtual functions
  bool init(int&,char**);
  bool start();
  void stop(bool);
  void loadNavMatrix();
  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory);

  void setAutoBufferSwap(bool);
  void swapBuffers();

  void setViewer();
  void setPlayer();

  bool restart();

  void setHeadMatrixID(int);
  const string getNavNodeName() const { return "SZG_NAV_MATRIX"; }
  arDatabaseNode* getNavNode();
  
  arInputNode* getInputDevice() const { return _inputDevice; }

  // These calls are really just for use when the object is being used
  // in graphics peer mode.
  int getNodeID(const string& name);
  arDatabaseNode* getNode(int ID);
  bool lockNode(int ID);
  bool unlockNode(int ID);
  
 private:
  arGraphicsServer _graphicsServer;
  arGraphicsPeer _graphicsPeer;

  void (*_userMessageCallback)(arDistSceneGraphFramework&, const string&);
  
  int _headMatrixID;
  int _graphicsNavMatrixID;
  int _soundNavMatrixID;

  bool _disabled;

  // stuff for standalone mode only
  arGraphicsClient    _graphicsClient;
  arSoundClient       _soundClient;
  string              _standaloneControlMode;
  arHeadWandSimulator _simulator;
  arFramerateGraph    _framerateGraph;
  arGraphicsWindow    _graphicsWindow;

  // Are we operating a graphics peer?
  string _peerName;
  // What mode are we operating it in? The special modes are source (default),
  // shell, and feedback.
  string _peerMode;
  // If in shell or feedback mode, to which peer are we connecting?
  string _peerTarget;
  // To what remote root node should we attach?
  int _remoteRootID;

  bool _loadParameters();
  void _getVector3(arVector3& v, const char* param);
  void _initDatabases();
  bool _initInput();
  bool _stripSceneGraphArgs(int& argc, char** argv);
};

#endif
