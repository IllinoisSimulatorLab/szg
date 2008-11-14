//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsAPI.h"
#include "arGraphicsHeader.h"
#include "arGraphicsPeerRPC.h"
#include "arSoundAPI.h"
#include "arSharedLib.h"
#include "arPForthFilter.h"
#include "arLogStream.h"
#include "arDistSceneGraphFramework.h"

void ar_distSceneGraphFrameworkMessageTask(void* framework) {
  arDistSceneGraphFramework* f = (arDistSceneGraphFramework*) framework;
  string messageType, messageBody;
  while (true) {
    const int messageID = f->_SZGClient.receiveMessage(&messageType, &messageBody);
    if (!messageID) {
      // szgserver disconnected, so the phleet is unreachable.  Quit.
LDie:
      // Stop the framework (only master/slave framework uses the arg.)
      f->stop(true);
      exit(0);
    }

    if (messageType=="quit") {
      f->_SZGClient.messageResponse( messageID, f->getLabel()+" quitting" );
      if (f->_vircompExecution) {
        // we have an arAppLauncher object piggy-backing on us,
        // controlling a virtual computer's execution
        f->_launcher.killApp();
      }
      goto LDie;
    }
    if (messageType=="log") {
      (void)ar_setLogLevel( messageBody );
    }
    else if (messageType=="demo") {
      f->setFixedHeadMode(messageBody=="on");
    }
    else if (messageType=="reload") {
      (void)f->_loadParameters();
    }
    else if (messageType== "user") {
      f->onUserMessage( messageID, messageBody );
      // Don't return.
      continue;
    }

    else if (messageType=="print") {
      f->getDatabase()->printStructure();
    }


    else if ( messageType == "use_nav_input_matrix" ) {
      if ( messageBody == "on" ) {
        f->_SZGClient.messageResponse( messageID, f->getLabel()+
            " enabling navigation from input matrix" );
        f->setUseNavInputMatrix( true );
      }
      else if ( messageBody == "off" ) {
        f->_SZGClient.messageResponse( messageID, f->getLabel()+
            " disabling navigation from input matrix." );
        f->setUseNavInputMatrix( false );
      }
      else {
        ar_log_error() << " ignoring unexpected use_nav_input_matrix arg '" << messageBody << "'.\n";
        f->_SZGClient.messageResponse( messageID, "ERROR: "+f->getLabel()+
            " ignoring unexpected use_nav_input_matrix arg '"+messageBody+"'." );
      }
    }

    else if ( messageType == "unit_convert_nav_input_matrix" ) {
      if ( messageBody == "on" ) {
        f->_SZGClient.messageResponse( messageID, f->getLabel()+
            " enabling unit-conversion of navigation input matrix" );
        f->setUnitConvertNavInputMatrix( true );
      }
      else if ( messageBody == "off" ) {
        f->_SZGClient.messageResponse( messageID, f->getLabel()+
            " disabling unit-conversion of navigation input matrix" );
        f->setUnitConvertNavInputMatrix( false );
      }
      else {
        ar_log_error() << " ignoring unexpected unit_convert_nav_input_matrix arg '" << messageBody << "'.\n";
        f->_SZGClient.messageResponse( messageID, "ERROR: "+f->getLabel()+
            " ignoring unexpected unit_convert_nav_input_matrix arg '"+messageBody+"'." );
      }
    }

    if (f->_peerName != "NULL" || f->_externalPeer) {
      // Message to the peer.
      (void) ar_graphicsPeerStripName(messageBody);
      arGraphicsPeer* messagePeer = f->_peerName == "NULL" ?
        f->_externalPeer : &f->_graphicsPeer;
      if (!f->_SZGClient.messageResponse(messageID,
          ar_graphicsPeerHandleMessage(messagePeer, messageType, messageBody))) {
        ar_log_warning() << "szg-rp got no message response.\n";
      }
    }
  }
}

// Standalone, run our own window instead of using szgrender.
void ar_distSceneGraphFrameworkWindowTask(void* f) {
  arDistSceneGraphFramework* fw = (arDistSceneGraphFramework*) f;
  // If there is only one window, then the window manager will be in single-threaded
  // mode. Consequently, the windowing needs to be started in the window thread.
  fw->_windowsCreated = fw->createWindows(true);
  // Tell the launching thread the result.
  fw->_windowsCreatedSignal.sendSignal();
  // Only proceed if, in fact, the launching succeeded.
  if (fw->_windowsCreated) {
    while (!fw->stopped()) {
      fw->loopQuantum();
    }
    fw->exitFunction();
    exit(0);
  }
}

void ar_distSceneGraphGUIMouseFunction( arGUIMouseInfo* mouseInfo ) {
  if ( !mouseInfo || !mouseInfo->getUserData() ) {
    return;
  }

  arDistSceneGraphFramework* fw
    = (arDistSceneGraphFramework*) mouseInfo->getUserData();

  // in standalone mode, button events should go to the interface
  if (fw->_standalone &&
      fw->_standaloneControlMode == "simulator") {
    if (mouseInfo->getState() == AR_MOUSE_DOWN
        || mouseInfo->getState() == AR_MOUSE_UP) {
      int whichButton = ( mouseInfo->getButton() == AR_LBUTTON ) ? 0 :
                        ( mouseInfo->getButton() == AR_MBUTTON ) ? 1 :
                        ( mouseInfo->getButton() == AR_RBUTTON ) ? 2 : 0;
      int whichState = ( mouseInfo->getState() == AR_MOUSE_DOWN ) ? 1 : 0;
      fw->_simPtr->mouseButton(whichButton, whichState,
                               mouseInfo->getPosX(), mouseInfo->getPosY());
    }
    else{
      fw->_simPtr->mousePosition(mouseInfo->getPosX(),
                                 mouseInfo->getPosY());
    }
  }
}

// Keyboard function used by arGUI in standalone mode.
void ar_distSceneGraphGUIKeyboardFunction( arGUIKeyInfo* keyInfo ) {
  if (!keyInfo || !keyInfo->getUserData() ) {
    return;
  }

  arDistSceneGraphFramework* fw
    = (arDistSceneGraphFramework*) keyInfo->getUserData();

  if ( keyInfo->getState() == AR_KEY_DOWN ) {
    switch ( keyInfo->getKey() ) {
    case AR_VK_ESC:
      // Stop the framework (parameter meaningless so far here, though it
      // does have meaning for the arMasterSlaveFramework)
      fw->stop(true);
      // Do not exit here. Instead, let that happen inside the standalone
      // thread of control.
      break;
    case AR_VK_f:
        fw->_graphicsClient.getWindowManager()->fullscreenWindow( keyInfo->getWindowID() );
      break;
      case AR_VK_F:
        fw->_graphicsClient.getWindowManager()->resizeWindow( keyInfo->getWindowID(), 600, 600 );
      break;
    case 'S':
      fw->_showSimulator = !fw->_showSimulator;
      fw->_graphicsClient.showSimulator(fw->_showSimulator);
      break;
    case 'P':
      fw->_graphicsClient.toggleFrameworkObjects();
      break;
    }

    // in standalone mode, keyboard events should also go to the interface
    if (fw->_standalone &&
      fw->_standaloneControlMode == "simulator") {
      fw->_simPtr->keyboard(keyInfo->getKey(), 1, 0, 0);
    }
  }
}

// NOTE: These window events come specifically from the OS, not from any
// arGUIWindowManager calls.
void ar_distSceneGraphGUIWindowFunction(arGUIWindowInfo* windowInfo) {
  if (!windowInfo || !windowInfo->getUserData()) {
    return;
  }

  arDistSceneGraphFramework* fw
    = (arDistSceneGraphFramework*) windowInfo->getUserData();

  arGUIWindowManager* windowManager = fw->_graphicsClient.getWindowManager();
  switch(windowInfo->getState()) {
  case AR_WINDOW_RESIZE:
    windowManager->setWindowViewport(windowInfo->getWindowID(),
                                     0, 0,
                                     windowInfo->getSizeX(),
                                     windowInfo->getSizeY());
    break;
  case AR_WINDOW_CLOSE:
    // Stop the framework (parameter is meaningless so far, though it does
    // have meaning on the arMasterSlaveFramework side.
    fw->stop(true);
    // Do not exit here. Instead, let that happen inside the main thread of
    // control for standalone mode.
    break;
  default:
    break;
  }
}


arDistSceneGraphFramework::arDistSceneGraphFramework() :
  arSZGAppFramework(),
  _usedGraphicsDatabase(NULL),
  _userMessageCallback(NULL),
  _oldUserMessageCallback(NULL),
  _graphicsNavNode(NULL),
  _soundNavMatrixID(-1),
  _VRCameraID(-1),
  _windowsCreated(false),
  _autoBufferSwap(true),
  _peerName("NULL"),
  _peerMode("source"),
  _peerTarget("NULL"),
  _remoteRootID(0),
  _externalPeer(NULL) {
}

// Syzygy messages consist of two strings, the first being
// a type and the second being a value. The user can send messages
// to the arMasterSlaveFramework and the application can trap them
// using this callback. A message w/ type "user" and value "foo" will
// be passed into this callback, if set, with "foo" going into the string.
void arDistSceneGraphFramework::setUserMessageCallback
  (void (*userMessageCallback)(arDistSceneGraphFramework&, int messageID, const string&)) {
  _userMessageCallback = userMessageCallback;
  _oldUserMessageCallback = NULL;
}

void arDistSceneGraphFramework::setUserMessageCallback
  (void (*userMessageCallback)(arDistSceneGraphFramework&, const string&)) {
  _oldUserMessageCallback = userMessageCallback;
  _userMessageCallback = NULL;
}


void arDistSceneGraphFramework::onUserMessage( int messageID, const string& messageBody ) {
  if (_userMessageCallback) {
    _userMessageCallback( *this, messageID, messageBody );
  } else if (_oldUserMessageCallback) {
    _oldUserMessageCallback( *this, messageBody );
  }
}

// Control additional locations where the databases (both sound and graphics)
// search for their data (sounds and textures respectively). For instance,
// it might be convenient to put these with other application data (SZG_DATA
// would be the bundlePathName) instead of in SZG_TEXTURE/path or SZG_SOUND/path.
// For python programs, it might be convenient to put the data in SZG_PYTHON.
void arDistSceneGraphFramework::setDataBundlePath(const string& bundlePathName,
                                                  const string& bundleSubDirectory) {
  _dataBundlePathSet = true;
  // The graphics client is only used for drawing in "standalone" mode.
  // It isn't used at all in "Phleet" mode. However, in the "standalone"
  // mode case, the connection process in the arGraphicsServer code
  // never occurs (client/server are just connected from the beginning).
  // Hence, we need to do this here, as well.
  _graphicsServer.setDataBundlePath(bundlePathName, bundleSubDirectory);
  _graphicsClient.setDataBundlePath(bundlePathName, bundleSubDirectory);

  _soundServer.setDataBundlePath(bundlePathName, bundleSubDirectory);
  _soundClient.setDataBundlePath(bundlePathName, bundleSubDirectory);

  ar_log_debug() << "set bundle path to " << bundlePathName << "/" << bundleSubDirectory << ar_endl;
}

// Returns a pointer to the graphics database being used, so that programs
// can alter it directly.
arGraphicsDatabase* arDistSceneGraphFramework::getDatabase() {
  // Standalone mode does not support peers.
  return (_peerName == "NULL" || _standalone) ?
    &_graphicsServer : (arGraphicsDatabase*)&_graphicsPeer;
}

void arDistSceneGraphFramework::setAutoBufferSwap(bool autoBufferSwap) {
  if (_initCalled) {
    ar_log_warning() << "ignoring setAutoBufferSwap() after init.\n";
    return;
  }

  // Select synchronization style in _initDatabases.
  _autoBufferSwap = autoBufferSwap;
}

void arDistSceneGraphFramework::swapBuffers() {
  if (_autoBufferSwap) {
    ar_log_warning() << "ignoring manual buffer swap in automatic swap mode.\n";
    return;
  }

  if (_standalone) {
    // Not in auto buffer swap mode.

    // Swap the (message) buffer.
    _graphicsServer._syncServer.swapBuffers();
    // The windowing is occuring in this thread, so run the event loop quantum.
    loopQuantum();
    if (stopped()) {
      // Services stopped.
      exitFunction();
      exit(0);
    }
  }
  else if (_peerName == "NULL") {
    // No sync in the peer case! (i.e. _peerName != "NULL")
    _graphicsServer._syncServer.swapBuffers();
  }
  else{
    // We are in "peer mode" and there is no need to swap (message) buffers.
    // Throttle applications that would
    // otherwise rely on the swap timing of the remote display for a throttle.
    ar_usleep(10000);
  }
}

arDatabaseNode* arDistSceneGraphFramework::getNavNode() {
  return _graphicsNavNode;
}

void arDistSceneGraphFramework::loadNavMatrix() {
  // Navigate whether or not we are in peer mode (in the
  // case of peer mode, only explicit setNavMatrix calls
  // will have an effect since the input device is
  // "disconnected").
  const arMatrix4 navMatrix( ar_getNavInvMatrix() );
  _graphicsNavNode->setTransform(navMatrix);
  if (_soundNavMatrixID != -1) {
    dsTransform( _soundNavMatrixID, navMatrix );
  }
}

void arDistSceneGraphFramework::setViewer() {
  _head.setMatrix( _inputDevice->getMatrix(AR_VR_HEAD_MATRIX_ID) );
  // It does not make sense to do this for the peer case.
  if (_peerName == "NULL") {
    // An alter(...) on a "graphics admin" record, as occurs in setVRCameraID,
    // cannot occur in a arGraphicsPeer.
    _graphicsServer.setVRCameraID(_VRCameraID);
    dgViewer( _VRCameraID, _head );
  }
}

void arDistSceneGraphFramework::setPlayer() {
  dsPlayer(_inputDevice->getMatrix(AR_VR_HEAD_MATRIX_ID),
           _head.getMidEyeOffset(),
           _head.getUnitConversion());
}

bool arDistSceneGraphFramework::init(int& argc, char** argv) {
  if (!_okToInit(argv[0]))
    return false;

  // Strip -dsg args.
  if (!_stripSceneGraphArgs(argc, argv)) {
    return false;
  }

  // Connect to the szgserver.
  _SZGClient.simpleHandshaking(false);
  if (!_SZGClient.init(argc, argv)) {
    // Already warned.
    return false;
  }

  autoDataBundlePath();

  ar_log_debug() << "Syzygy version: " << ar_versionString() << ar_endl;

  if (!_SZGClient) {
    // Standalone.
    const bool ok = _initStandaloneMode();
    _initCalled = true;
    return ok;
  }

  string currDir;
  if (!ar_getWorkingDirectory( currDir )) {
    ar_log_critical() << "Failed to get working directory.\n";
  } else {
    ar_log_critical() << "Directory: " << currDir << ar_endl;
  }

  // Connected to an szgserver.
  ar_log().setStream(_SZGClient.initResponse());
  const bool ok = _initPhleetMode();
  _initCalled = true;
  ar_log().setStream(cout);
  return ok;
}

bool arDistSceneGraphFramework::start() {
  if (!_okToStart())
    return false;

  if (_standalone) {
    const bool ok = _startStandaloneMode();
    _startCalled = true;
    return ok;
  }

  ar_log().setStream(_SZGClient.startResponse());
  const bool ok = _startPhleetMode();
  _startCalled = true;
  ar_log().setStream(cout);
  return ok;
}

// Shut components down.  Ignore blockUntilDisplayExit because 
// standalone mode runs only a local display and hence needs no
// stop message from the message thread (which isn't connected
// to anything).
void arDistSceneGraphFramework::stop(bool) {
  // Tell the app we're stopping.
  _exitProgram = true;
  if (_peerName == "NULL" || _standalone) {
    _graphicsServer.stop();
  }
  else{
    _graphicsPeer.stop();
  }
  _soundServer.stop();
  arSleepBackoff a(50, 100, 1.1);
  while (_useExternalThread && _externalThreadRunning)
    a.sleep();
  ar_log_remark() << "threads stopped.\n";
  _stopped = true;
}

// Standalone.  Display own window instead of using szgrender.
// This framework ignores parameter "useWindowing."
bool arDistSceneGraphFramework::createWindows(bool) {
  if (!_standalone)
    return false;

  if (!_graphicsClient.start(_SZGClient, false)) {
    ar_log_error() << "failed to start windowing.\n";
#ifdef AR_USE_DARWIN
    ar_log_error() << "  (Ensure that X11 is running.)\n";
#endif
    return false;
  }
  return true;
}

// Standalone, display a window (as opposed to through szgrender).
void arDistSceneGraphFramework::loopQuantum() {
  if (!_standalone)
    return;

  const ar_timeval time1 = ar_time();
  if (_standaloneControlMode == "simulator")
    _simPtr->advance();

  // In here, through callbacks to the arSyncDataClient in
  // the arGraphicsClient, is where scene graph event processing,
  // drawing, and synchronization happens.
  if (!_soundClient.silent())
    _soundClient._cliSync.consume();
  _graphicsClient._cliSync.consume();

  // Events like closing the window and hitting the ESC key that cause the
  // window to close occur inside processWindowEvents(). Once they happen,
  // stopped() will return true, ending the loop containing this loopQuantum.
  _wm->processWindowEvents();
  arPerformanceElement* el = _framerateGraph.getElement("fps");
  el->pushNewValue(1000000.0/ar_difftimeSafe(ar_time(), time1));
}

// Standalone (own window, not szgrender).
void arDistSceneGraphFramework::exitFunction() {
  if (_standalone) {
    _wm->deleteAllWindows();
  }
}

// If the attribute exists, stuff its value into v.
// Otherwise report v's (unchanged) default value.
// Sort of copypasted with arDistSceneGraphFramework::_getVector3.
void arDistSceneGraphFramework::_getVector3(arVector3& v, const char* param) {
  // use the screen name passed from the distributed system
  const string screenName(_SZGClient.getMode("graphics"));
  if (!_SZGClient.getAttributeFloats(screenName, param, v.v, 3)) {
    ar_log_remark() << "screen " << screenName << "/" << param  <<
      " defaulting to " << v << ".\n";
  }
}

bool arDistSceneGraphFramework::_loadParameters() {
  _dataPath = _SZGClient.getDataPath();
  addDataBundlePathMap( "SZG_DATA", _dataPath );
  addDataBundlePathMap( "SZG_PYTHON", _SZGClient.getDataPathPython() );
  _head.configure( _SZGClient );
  _loadNavParameters();
  arGraphicsPluginNode::setSharedLibSearchPath(
    _SZGClient.getAttribute("SZG_EXEC", "path") );
  _parametersLoaded = true;
  return true;
}

void arDistSceneGraphFramework::addDataBundlePathMap(
    const string& bundlePathName,
    const string& bundlePath) {
  _soundServer.addDataBundlePathMap( bundlePathName, bundlePath );
  _soundClient.addDataBundlePathMap( bundlePathName, bundlePath );
  _graphicsServer.addDataBundlePathMap( bundlePathName, bundlePath );
  _graphicsClient.addDataBundlePathMap( bundlePathName, bundlePath );
}


void arDistSceneGraphFramework::_initDatabases() {
  _usedGraphicsDatabase = (_peerName!="NULL") ?
    (arGraphicsDatabase*)&_graphicsPeer :
    (arGraphicsDatabase*)&_graphicsServer;
  dgSetGraphicsDatabase(_usedGraphicsDatabase);
  dsSetSoundDatabase(&_soundServer);

  if (_standalone) {
    // Start local connections before any alterations occur to the
    // local graphics server, since no connections occur.

    // With manual buffer swapping, use "nosync" mode so
    // Python can use only one thread.  This avoids deadlock.
    _graphicsServer._syncServer.setMode(_autoBufferSwap ? AR_SYNC_AUTO_SERVER : AR_NOSYNC_MANUAL_SERVER);
    _graphicsClient._cliSync.registerLocalConnection(&_graphicsServer._syncServer);
    _soundClient._cliSync.registerLocalConnection(&_soundServer._syncServer);
    if (!_soundClient.init())
      ar_log_error() << "silent.\n";
  }
  else if (_peerName == "NULL") {
    // In phleet mode (but not peer mode), _syncServer always syncs with its client.
    _graphicsServer._syncServer.setMode(_autoBufferSwap ? AR_SYNC_AUTO_SERVER : AR_SYNC_MANUAL_SERVER);
  }

  // Set up a default viewing position.
  _VRCameraID = dgViewer( "root", _head );

  // Set up a default listening position.
  dsPlayer( _head.getMatrix(), _head.getMidEyeOffset(), _head.getUnitConversion() );

  if (_graphicsNavNode == NULL) {
    _graphicsNavNode = (arTransformNode*) _usedGraphicsDatabase->getRoot()->
      newNode("transform", getNavNodeName());
    _graphicsNavNode->setTransform(arMatrix4());
  }
  if (_soundNavMatrixID == -1) {
    _soundNavMatrixID = dsTransform(getNavNodeName(), "root", arMatrix4());
  }
}

bool arDistSceneGraphFramework::_initInput() {
  _inputDevice = new arInputNode;
  if (!_inputDevice) {
    ar_log_error() << "failed to create input device.\n";
    return false;
  }
  _inputState = &(_inputDevice->_inputState);

  if (!_standalone) {
    _inputDevice->addInputSource(&_netInputSource, false);
    if (!_netInputSource.setSlot(0)) {
      ar_log_error() << "failed to set slot 0.\n";
      return false;
    }
    _inputState = &(_inputDevice->_inputState);

  } else {  // Standalone
    _handleStandaloneInput();
  }

  _installFilters();

  if (!_inputDevice->init(_SZGClient) && !_standalone) {
    ar_log_error() << "failed to init input device.\n";
    if (!_SZGClient.sendInitResponse(false)) {
      cerr << _label << " error: maybe szgserver died.\n";
    }
    return false;
  }
  return true;
}

bool arDistSceneGraphFramework::_stripSceneGraphArgs(int& argc, char** argv) {
  for (int i=0; i<argc; i++) {
    if (!strcmp(argv[i], "-dsg")) {
      //this arg might need to be removed.
      if (i+1 >= argc) {
        ar_log_error() << "-dsg flag is last in arg list.\n";
        return false;
      }

      // we need to figure out which args these are.
      // THIS IS COPY-PASTED FROM arSZGClient (where other key=value args
      // are parsed). SHOULD IT BE GLOBALIZED?
      const string thePair(argv[i+1]);
      unsigned int location = thePair.find('=');
      if (location == string::npos) {
        ar_log_error() << "context pair has no '='.\n";
        return false;
      }
      unsigned int length = thePair.length();
      if (location == length-1) {
        ar_log_error() << "context pair ends with '='.\n";
        return false;
      }
      // So far, the only special key-value pair is:
      // peer=peer_name. If this exists, then we know we should
      // operate in a reality-peer-fashion.
      string key(thePair.substr(0, location));
      string value(thePair.substr(location+1, thePair.length()-location));
      if (key == "peer") {
        _peerName = value;
        ar_log_remark() << "setting peer name = " << _peerName << "\n";
        _graphicsPeer.setName(_peerName);
      }
      else if (key == "mode") {
        if (value != "feedback") {
          ar_log_error() << "illegal mode value '" << value << "'.\n";
          return false;
        }
        _peerMode = value;
      }
      else if (key == "target") {
        _peerTarget = value;
      }
      else if (key == "root") {
        _remoteRootID = atoi(value.c_str());
      }
      else{
        ar_log_error() << "arg pair has illegal key '" << key << "'.\n";
        return false;
      }

      // if they are parsed, then they should also be erased
      for (int j=i; j<argc-2; j++) {
        argv[j] = argv[j+2];
      }
      // reset the arg count and i
      argc -= 2;
      i--;
    }
  }
  return true;
}

// Simplifies the start message processing. With the default parameter
// value for f (false), this posts the given message to the start response
// string and returns false. Otherwise, it just returns true.
bool arDistSceneGraphFramework::_startRespond(const string& s, bool f) {
  if (!f) {
    ar_log_error() << " scene-graph framework: " << s << "\n";
  }
  if (!_SZGClient.sendStartResponse(f)) {
    cerr << _label << " error: maybe szgserver died.\n";
  }
  return f;
}

bool arDistSceneGraphFramework::_initStandaloneMode() {
  _standalone = true;
  (void)_loadParameters();
  _initDatabases();
  if (!_initInput()) {
    return false;
  }
  ar_log_remark() << "standalone objects inited.\n";
  return true;
}

// Start the various framework objects in the manner demanded by standalone mode.
bool arDistSceneGraphFramework::_startStandaloneMode() {
  // Configure the window manager and pass it to the graphicsClient
  // before configure (because, inside graphicsClient configure, it is used
  // with the arGUIXMLParser).
  // Default: non-threaded window manager.
  _wm = new arGUIWindowManager(ar_distSceneGraphGUIWindowFunction,
                               ar_distSceneGraphGUIKeyboardFunction,
                               ar_distSceneGraphGUIMouseFunction,
                               NULL,
                               false);
  _wm->setUserData(this);
  // arGraphicsClient is what actually draws and manages the windows.
  _graphicsClient.setWindowManager(_wm);
  _graphicsClient.configure(&_SZGClient);
  if (_standaloneControlMode == "simulator") {
    _simPtr->configure(_SZGClient);
    _graphicsClient.setSimulator(_simPtr);
  }
  _framerateGraph.addElement("fps", 300, 100, arVector3(1, 1, 1));
  _graphicsClient.addFrameworkObject(&_framerateGraph);
  arSpeakerObject* speakerObject = new arSpeakerObject();
  if (!_soundClient.silent()) {
    _soundClient.setSpeakerObject(speakerObject);
    _soundClient.configure(&_SZGClient);
    _soundServer.start();
  }
  // Peer mode is meaningless here.
  _graphicsServer.start();
  _inputDevice->start();
  arThread graphicsThread; // For display.
  if (_autoBufferSwap) {
    graphicsThread.beginThread(ar_distSceneGraphFrameworkWindowTask, this);
    // Wait until the thread has tried to create the windows.
    _windowsCreatedSignal.receiveSignal();
    // _windowsCreated is set by createWindows() in ar_distSceneGraphFrameworkTask.
    if (!_windowsCreated) {

      // Stop the services already started, before exiting.
      // Does this cause segfaults?
      //stop(true);

      return false;
    }
  }
  else{
    if (!createWindows(true)) {

      // Stop started services, before exiting.  Does this segfault?
      //stop(true);

      return false;
    }
  }
  ar_log_remark() << "standalone objects started.\n";
  return true;
}

bool arDistSceneGraphFramework::_initPhleetMode() {
  (void)_loadParameters();

  if (!_messageThread.beginThread(ar_distSceneGraphFrameworkMessageTask, this)) {
    ar_log_error() << "failed to start message thread.\n";
    if (!_SZGClient.sendInitResponse(false)) {
      cerr << _label << " error: maybe szgserver died.\n";
    }
    return false;
  }

  // Should we launch the other executables?
  if (_SZGClient.getMode("default") == "trigger") {
    const string vircomp = _SZGClient.getVirtualComputer();
    const string defaultMode = _SZGClient.getMode("default");

    (void)_launcher.setSZGClient(&_SZGClient);
    _vircompExecution = true;
    _launcher.setAppType("distgraphics");
    _launcher.setRenderProgram("szgrender");

    // Reorganize the cluster.
    if (!_launcher.launchApp()) {
      ar_log_error() << "failed to launch on virtual computer " << vircomp << ".\n";
      if (!_SZGClient.sendInitResponse(false)) {
        cerr << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
    ar_log_remark() << "launched on virtual computer " << vircomp <<
      ", default mode " << defaultMode << ".\n";
  }

  // Init sound and graphics databases, AFTER the virtual computer
  // reshuffle (launchApp). Some OS's allow only ONE connection
  // to the sound card, blocking _soundClient.init() until a previous
  // app ends.  This might deadlock, if _soundClient.init() occurs before
  // launchApp (which kills other apps on the v.c.).
  _initDatabases();

  // Init input device.
  if (!_initInput()) {
    return false;
  }

  // Choose either graphics server or peer, based on init.
  // No sound server in peer mode, so multiple peers can run.
  if (_peerName == "NULL") {
    if (!_graphicsServer.init(_SZGClient)) {
      ar_log_error() << "graphics server failed to init. (Is another app running?)\n";
      if (!_SZGClient.sendInitResponse(false)) {
        cerr << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
    if (!_soundServer.init(_SZGClient)) {
      ar_log_error() << "audio failed to init. (Is another app running?)\n";
      if (!_SZGClient.sendInitResponse(false)) {
        cerr << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
  }
  else{
    if (!_graphicsPeer.init(_SZGClient)) {
      ar_log_error() << "graphics peer failed to init.\n";
      if (!_SZGClient.sendInitResponse(false)) {
        cerr << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
  }

  // init() succeeded.
  if (!_SZGClient.sendInitResponse(true)) {
    cerr << _label << " error: maybe szgserver died.\n";
  }
  return true;
}

bool arDistSceneGraphFramework::_startPhleetMode() {
  ar_log_critical() << "cluster mode.\n";
  // szgrender will display graphics.

  // Start the graphics database.
  if (_peerName == "NULL" || _standalone) {
    // Not in peer mode.
    if (!_graphicsServer.start()) {
      return _startRespond("graphics server failed to listen.");
    }
  }
  else{
    // Peer mode.
    if (!_graphicsPeer.start()) {
      return _startRespond("graphics peer failed to start.");
    }

    // Connect to the specified target, if required.
    if (_peerTarget != "NULL") {
      if (_graphicsPeer.connectToPeer(_peerTarget) < 0) {
        return _startRespond("graphics peer failed to connect to requested target=" + _peerTarget + ".");
      }
      // Set up feedback to the remote peer.
      _graphicsPeer.pushSerial(_peerTarget, _remoteRootID, 0,
        AR_TRANSIENT_NODE, AR_TRANSIENT_NODE, AR_TRANSIENT_NODE);
    }
  }

  // Don't start the input device or sound server if in peer mode,
  // so multiple peers can run.
  if (_peerName == "NULL" && !_inputDevice->start()) {
    return _startRespond("failed to start input device.");
  }

  if (_peerName == "NULL" && !_soundServer.start()) {
    return _startRespond("sound server failed to listen.");
  }

  return _startRespond("", true);
}
