//********************************************************
// Syzygy is licensed under the BSD license v2
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
#include "arSZGAppFramework.h"
#include "arFrameworkCalling.h"

// Framework for cluster applications using a distributed scene graph.

class SZG_CALL arDistSceneGraphFramework : public arSZGAppFramework {
  friend void ar_distSceneGraphFrameworkMessageTask(void*);
  friend void ar_distSceneGraphFrameworkWindowTask(void*);
  friend void ar_distSceneGraphFrameworkDisplay();
  friend void ar_distSceneGraphFrameworkButtonFunction(int, int, int, int);
  friend void ar_distSceneGraphFrameworkMouseFunction(int, int);
  friend void ar_distSceneGraphFrameworkKeyboard(unsigned char, int, int);
  friend void ar_distSceneGraphGUIMouseFunction( arGUIMouseInfo* mouseInfo );
  friend void ar_distSceneGraphGUIKeyboardFunction( arGUIKeyInfo* keyInfo );
  friend void ar_distSceneGraphGUIWindowFunction( arGUIWindowInfo* windowInfo);
 public:
  arDistSceneGraphFramework();
  ~arDistSceneGraphFramework() {}

  // Inherited virtual functions
  bool init(int&, char**);
  bool start();
  void stop(bool);
  bool createWindows(bool useWindowing);
  void loopQuantum();
  void exitFunction();

  virtual void onUserMessage( int messageID, const string& messageBody );

  void setUserMessageCallback(void (*userMessageCallback)( arDistSceneGraphFramework&,
                 int messageID,
                                                           const string& messageBody ));
  void setUserMessageCallback(void (*userMessageCallback)( arDistSceneGraphFramework&,
                                                           const string& messageBody ));
  arGraphicsDatabase* getDatabase();

  // An external peer might get peer control messages,
  // e.g. the peerBridge that maps a peer into a clustered display.
  void setExternalPeer(arGraphicsPeer* p) { if (p) _externalPeer = p; }

  void setDataBundlePath(const string& bundlePathName,
                         const string& bundleSubDirectory);
  void setAutoBufferSwap(bool);
  void swapBuffers();
  arDatabaseNode* getNavNode();

  void loadNavMatrix();
  void setViewer();
  void setPlayer();

  // Add entries to the data bundle path (used to locate texture maps by
  // szgrender for scene-graph apps in cluster mode and by SoundRender to
  // locate sounds for both types of apps in cluster mode).
  virtual void addDataBundlePathMap(const string& bundlePathName,
                          const string& bundlePath);

 private:
  // Used in both standalone mode and phleet mode.
  arGraphicsServer _graphicsServer;
  arGraphicsPeer _graphicsPeer;
  // Which of the above is actually used?
  arGraphicsDatabase* _usedGraphicsDatabase;
  // Objects only used in standalone mode.
  arGraphicsClient    _graphicsClient;
  arSoundClient       _soundClient;

  void (*_userMessageCallback)( arDistSceneGraphFramework&,
      int messageID, const string& messageBody );
  void (*_oldUserMessageCallback)( arDistSceneGraphFramework&,
      const string& messageBody );

  arTransformNode* _graphicsNavNode;
  int _soundNavMatrixID;
  int _VRCameraID;
  // In standalone mode, have the windows been successfully created?
  bool _windowsCreated;
  // In standalone mode, if running the windowing in a different thread,
  // this signal allows us to inform the caller whether the window creation
  // succeeded.
  arSignalObject _windowsCreatedSignal;
  // Are we using the automatic buffer swap?
  bool _autoBufferSwap;

  // Are we operating a graphics peer? If not, this string will hold the
  // constructor's default value of "NULL".
  string _peerName;
  // What mode are we operating it in? The special modes are source (default),
  // shell, and feedback.
  string _peerMode;
  // If in shell or feedback mode, to which peer are we connecting?
  string _peerTarget;
  // To what remote root node should we attach?
  int _remoteRootID;
  // Maybe (as in the case of peerBridge) we should pass on
  // peer messages to someone external peer.
  arGraphicsPeer* _externalPeer;

  // Internal functions.
  bool _loadParameters();
  void _getVector3(arVector3& v, const char* param);
  void _initDatabases();
  bool _initInput();
  bool _stripSceneGraphArgs(int& argc, char** argv);
  bool _startRespond(const string& s, bool f=false);
  bool _initStandaloneMode();
  bool _startStandaloneMode();
  bool _initPhleetMode();
  bool _startPhleetMode();
};

#endif
