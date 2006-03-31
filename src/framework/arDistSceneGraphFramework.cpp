//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsAPI.h"
#include "arGraphicsHeader.h"
#include "arGraphicsPeerRPC.h"
#include "arSoundAPI.h"
#include "arSharedLib.h"
#include "arPForthFilter.h"
#include "arLogStream.h"
#include "arDistSceneGraphFramework.h"

void ar_distSceneGraphFrameworkMessageTask(void* framework){
  arDistSceneGraphFramework* f = (arDistSceneGraphFramework*) framework;
  string messageType, messageBody;
  while (true) {
    int messageID = f->_SZGClient.receiveMessage(&messageType, &messageBody);
    if (!messageID){
      // We have somehow been disconnected from the szgserver. We should
      // now quit. NOTE: we cannot kill the stuff elsewhere since we are
      // disconnected (i.e. we cannot manipulate the system anymore)
      
      // Important that we stop the framework (the parameter is meaningless
      // so far for this framework, though it is important for the master/slave
      // framework.)
      f->stop(true);
      exit(0);
    }
    if (messageType=="quit"){
      if (f->_vircompExecution){
        // we have an arAppLauncher object piggy-backing on us,
	// controlling a virtual computer's execution
	f->_launcher.killApp();
      }
      // stop the framework
      f->stop(true);
      exit(0);
    }
    else if (messageType=="demo") {
      f->setFixedHeadMode(messageBody=="on");
    }
    else if (messageType=="reload"){
      f->_loadParameters();
    }
    else if (messageType== "user"){
      if (f->_userMessageCallback){
        f->_userMessageCallback(*f, messageBody);
      }
      // The message processing loop must *continue* here, not *return*!
      continue;
    }
    else if (messageType=="print"){
      f->getDatabase()->printStructure();
    }
    
    if (f->_peerName != "NULL" || f->_externalPeer){
      // Handle messages to the peer.
      // Strip the initial leading peer name.
      (void) ar_graphicsPeerStripName(messageBody);
      arGraphicsPeer* messagePeer = f->_peerName == "NULL" ?
        f->_externalPeer : &f->_graphicsPeer;
      const string responseBody(ar_graphicsPeerHandleMessage(
        messagePeer, messageType, messageBody));
      if (!f->_SZGClient.messageResponse(messageID, responseBody)){
        cerr << "szg-rp error: message response failed.\n";
      }
    }
  }
}

// In standalone mode, we might actually like to run our own window,
// instead of having an szgrender do it.
void ar_distSceneGraphFrameworkWindowTask(void* f){
  arDistSceneGraphFramework* fw = (arDistSceneGraphFramework*) f;
  // If there is only one window, then the window manager will be in single-threaded 
  // mode. Consequently, the windowing needs to be started in the window thread.
  fw->_windowsCreated = fw->createWindows(true);
  // Tell the launching thread the result.
  fw->_windowsCreatedSignal.sendSignal();
  // Only proceed if, in fact, the launching succeeded.
  if (fw->_windowsCreated){
    while (!fw->stopped()){ 
      fw->loopQuantum();
    }
    fw->exitFunction();
    exit(0);
  }
}

void ar_distSceneGraphGUIMouseFunction( arGUIMouseInfo* mouseInfo ) {
  if( !mouseInfo || !mouseInfo->getUserData() ) {
    return;
  }

  arDistSceneGraphFramework* fw 
    = (arDistSceneGraphFramework*) mouseInfo->getUserData();

  // in standalone mode, button events should go to the interface
  if (fw->_standalone && 
      fw->_standaloneControlMode == "simulator"){
    if (mouseInfo->getState() == AR_MOUSE_DOWN 
        || mouseInfo->getState() == AR_MOUSE_UP){
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
void ar_distSceneGraphGUIKeyboardFunction( arGUIKeyInfo* keyInfo ){
  if (!keyInfo || !keyInfo->getUserData() ){
    return;
  }

  arDistSceneGraphFramework* fw 
    = (arDistSceneGraphFramework*) keyInfo->getUserData();
  
  if ( keyInfo->getState() == AR_KEY_DOWN ){
    switch ( keyInfo->getKey() ){
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
      fw->_standaloneControlMode == "simulator"){
      fw->_simPtr->keyboard(keyInfo->getKey(), 1, 0, 0);
    }
  }
}

// NOTE: These window events come specifically from the OS, not from any
// arGUIWindowManager calls.
void ar_distSceneGraphGUIWindowFunction(arGUIWindowInfo* windowInfo){
  if (!windowInfo || !windowInfo->getUserData()){
    return;
  }

  arDistSceneGraphFramework* fw
    = (arDistSceneGraphFramework*) windowInfo->getUserData();

  arGUIWindowManager* windowManager = fw->_graphicsClient.getWindowManager();
  switch(windowInfo->getState()){
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
  _graphicsNavNode(NULL),
  _soundNavMatrixID(-1),
  _VRCameraID(-1),
  _windowsCreated(false),
  _autoBufferSwap(true),
  _peerName("NULL"),
  _peerMode("source"),
  _peerTarget("NULL"),
  _remoteRootID(0),
  _externalPeer(NULL){
}

/// Syzygy messages consist of two strings, the first being
/// a type and the second being a value. The user can send messages
/// to the arMasterSlaveFramework and the application can trap them
/// using this callback. A message w/ type "user" and value "foo" will
/// be passed into this callback, if set, with "foo" going into the string.
void arDistSceneGraphFramework::setUserMessageCallback
  (void (*userMessageCallback)(arDistSceneGraphFramework&, const string&)){
  _userMessageCallback = userMessageCallback;
}

/// Control additional locations where the databases (both sound and graphics)
/// search for their data (sounds and textures respectively). For instance,
/// it might be convenient to put these with other application data (SZG_DATA
/// would be the bundlePathName) instead of in SZG_TEXTURE/path or SZG_SOUND/path.
/// For python programs, it might be convenient to put the data in SZG_PYTHON.
void arDistSceneGraphFramework::setDataBundlePath(const string& bundlePathName,
                                                  const string& bundleSubDirectory) {
  _graphicsServer.setDataBundlePath(bundlePathName, bundleSubDirectory);
  // The graphics client is only used for drawing in "standalone" mode.
  // It isn't used at all in "Phleet" mode. However, in the "standalone"
  // mode case, the connection process in the arGraphicsServer code
  // never occurs (client/server are just connected from the beginning).
  // Hence, we need to do this here, as well.
  _graphicsClient.setDataBundlePath(bundlePathName, bundleSubDirectory);
  // Must be done analogously for the sound (both texture files and sound
  // files live in the bundles).
  _soundServer.setDataBundlePath(bundlePathName, bundleSubDirectory);
  _soundClient.setDataBundlePath(bundlePathName, bundleSubDirectory);
}

/// Returns a pointer to the graphics database being used, so that programs
/// can alter it directly.
arGraphicsDatabase* arDistSceneGraphFramework::getDatabase(){
  // Standalone mode does not support peers.
  if (_peerName == "NULL" || _standalone){
    return &_graphicsServer;
  }
  else{
    return &_graphicsPeer;
  }
}

void arDistSceneGraphFramework::setAutoBufferSwap(bool autoBufferSwap){
  if (_initCalled){
    ar_log_critical() << "arDistSceneGraphFramework error: setAutoBufferSwap(...) MUST BE CALLED "
                      << "BEFORE init(...). PLEASE CHANGE YOUR CODE.\n";
    return;
  }
  // This variable selects between different synchronization styles in _initDatabases.
  _autoBufferSwap = autoBufferSwap;
}

void arDistSceneGraphFramework::swapBuffers(){
  if (_autoBufferSwap){
    ar_log_critical() <<  "arDistSceneGraphFramework error: attempting to swap buffers "
                      <<  "manually in automatic swap mode.\n";
    return;
  }
  if (_standalone){
    // If we've made it here, then we are *not* in auto buffer swap mode, but
    // we *are* in standalone mode.
    
    // Must swap the (message) buffer.
    _graphicsServer._syncServer.swapBuffers();
    // The windowing is occuring in this thread, hence execute the event loop
    // quantum.
    loopQuantum();
    // Should exit if the services have stopped.
    if (stopped()){
      exitFunction();
      exit(0); 
    }
  }
  else if (_peerName == "NULL"){
    // No synchronization occurs in the peer case! (i.e. _peerName != "NULL")
    _graphicsServer._syncServer.swapBuffers();
  }
  else{
    // We are in "peer mode" and there is no need to swap (message) buffers.
    // The following sleep is a necessary throttle for applications that would 
    // otherwise rely on the swap timing of the remote display for a throttle.
    ar_usleep(10000);
  }
}

arDatabaseNode* arDistSceneGraphFramework::getNavNode(){
  return _graphicsNavNode;
}

void arDistSceneGraphFramework::loadNavMatrix() {
  // Navigate whether or not we are in peer mode (in the
  // case of peer mode, only explicit setNavMatrix calls
  // will have an effect since the input device is
  // "disconnected").
  const arMatrix4 navMatrix( ar_getNavInvMatrix() );
  _graphicsNavNode->setTransform(navMatrix);
  if (_soundNavMatrixID != -1){
    dsTransform( _soundNavMatrixID, navMatrix );
  }
}

void arDistSceneGraphFramework::setViewer(){
  _head.setMatrix( _inputDevice->getMatrix(AR_VR_HEAD_MATRIX_ID) );
  // It does not make sense to do this for the peer case.
  if (_peerName == "NULL"){
    // An alter(...) on a "graphics admin" record, as occurs in setVRCameraID,
    // cannot occur in a arGraphicsPeer.
    _graphicsServer.setVRCameraID(_VRCameraID);
    dgViewer( _VRCameraID, _head );
  } 
}

void arDistSceneGraphFramework::setPlayer(){
  dsPlayer(_inputDevice->getMatrix(AR_VR_HEAD_MATRIX_ID),
	   _head.getMidEyeOffset(),
	   _unitSoundConversion);
}

bool arDistSceneGraphFramework::init(int& argc, char** argv){
  if (_initCalled){
    ar_log_critical() << "arDistSceneGraphFramework error: MUST CALL init(...) ONLY ONCE.\n"
                      << "  PLEASE CHANGE YOUR CODE.\n";
    return false;
  }
  if (_startCalled){
    ar_log_critical() << "arDistSceneGraphFramework error: init(...) MUST BE CALLED BEFORE start().\n"
                      << "  PLEASE CHANGE YOUR CODE.\n";
    return false;
  }
  _label = string(argv[0]);
  _label = ar_stripExeName(_label); // remove pathname and .EXE
  // See if there are special -dsg args in the arg list and strip them out.
  if (!_stripSceneGraphArgs(argc, argv)){
    return false;
  }

  // Connect to the szgserver.
  _SZGClient.simpleHandshaking(false);
  // If the szgClient cannot be initialized, fail. Do not print anything out
  // since it already did.
  if (!_SZGClient.init(argc, argv)){
    return false; 
  }
  
  bool state = false;
  if (!_SZGClient){
    // We must be in standalone mode.
    state = _initStandaloneMode();
    _initCalled = true;
    return state;
  }

  // Successfully connected to an szgserver.
  ar_log().setStream(_SZGClient.initResponse());
  state = _initPhleetMode();
  _initCalled = true;
  ar_log().setStream(cout);
  return state;
}

bool arDistSceneGraphFramework::start(){
  if (!_initCalled){
    ar_log_critical() << "arDistSceneGraphFramework error: MUST CALL init(...) BEFORE start().\n"
                      << "  PLEASE CHANGE YOUR CODE.\n";
    return false;
  }
  if (_startCalled){
    ar_log_critical() << "arDistSceneGraphFramework error: start() MUST ONLY BE CALLED ONCE.\n"
                      << "  PLEASE CHANGE YOUR CODE.\n";
    return false;
  }
  // We have two pathways depending upon whether or not we are in standalone
  // mode.
  bool state = false;
  if (_standalone){
    state = _startStandaloneMode();
    _startCalled = true;
    return state;
  }
  else{
    // Distributed, i.e. connected to an szgserver and not in standalone.
    ar_log().setStream(_SZGClient.startResponse());
    state = _startPhleetMode();
    _startCalled = true;
    ar_log().setStream(cout);
    return state;
  }
}

/// Does the best it can to shut components down. Don't actually do anything
/// with the parameter (blockUntilDisplayExit) because we only run a 
/// local display in standalone mode and hence do not need to worry about
/// getting a stop message from the message thread (which isn't connected 
/// to anything in this case).
void arDistSceneGraphFramework::stop(bool){
  // Must let the application know that we are stopping
  _exitProgram = true;
  // Only stop the appropriate database.
  if (_peerName == "NULL" || _standalone){
    _graphicsServer.stop();
  }
  else{
    _graphicsPeer.stop();
  }
  _soundServer.stop();
  while (_useExternalThread && _externalThreadRunning){
    ar_usleep(100000);
  }
  ar_log_remark() << "arDistSceneGraphFramework remark: threads done.\n";
  _stopped = true;
}

/// Used in standalone mode only. our application will display its
/// own window instead of using szgrender. The "useWindowing" parameter is ignored
/// by this framework.
bool arDistSceneGraphFramework::createWindows(bool){
  if (!_standalone)
    // This method shouldn't have been called.
    return false;

  // Start the windowing. 
  const bool ok = _graphicsClient.start(_SZGClient, false);
  if (!ok){
    ar_log_critical() << "arDistSceneGraphFramework error: failed to start windowing.\n"; 
#ifdef AR_USE_DARWIN
    ar_log_critical() << "  (Please ensure that X11 is running.)\n";
#endif	
  }
  return ok;
}

/// Used in standalone mode. In that case, our application must actually dislay its
/// own window (as opposed to through a szgrender as in phleet mode).
void arDistSceneGraphFramework::loopQuantum(){
  if (_standalone){
    const ar_timeval time1 = ar_time();
    if (_standaloneControlMode == "simulator")
      _simPtr->advance();

    // Inside here, through callbacks to the arSyncDataClient embedded inside
    // the arGraphicsClient, is where all the scene graph event processing,
    // drawing, and synchronization happens.
    _soundClient._cliSync.consume();
    _graphicsClient._cliSync.consume();

    // Events like closing the window and hitting the ESC key that cause the
    // window to close occur inside processWindowEvents(). Once they happen,
    // stopped() will return true and this loop will exit.
    _wm->processWindowEvents();
    arPerformanceElement* framerateElement =
      _framerateGraph.getElement("framerate");
    framerateElement->pushNewValue(
      1000000.0/ar_difftimeSafe(ar_time(), time1));
  }
}

/// Used in standalone mode only. In that case, our application will display its
/// own window instead of using szgrender.
void arDistSceneGraphFramework::exitFunction(){
  if (_standalone){
    _wm->deleteAllWindows();
  }
}

// If the attribute exists, stuff its value into v.
// Otherwise report v's (unchanged) default value.
// Sort of copypasted with arScreenObject::_getVector3.
void arDistSceneGraphFramework::_getVector3(arVector3& v, const char* param){
  // use the screen name passed from the distributed system
  const string screenName(_SZGClient.getMode("graphics"));
  if (!_SZGClient.getAttributeFloats(screenName, param, v.v, 3)) {
    ar_log_remark() << _label << " remark: " << screenName
                    << "/" << param  << " defaulting to " << v << ar_endl;
  }
}

bool arDistSceneGraphFramework::_loadParameters(){
  // Configure data access for this box.
  _dataPath = _SZGClient.getAttribute("SZG_DATA", "path");
  if (_dataPath == "NULL"){
    ar_log_warning() << _label << " warning: SZG_DATA/path undefined.\n";
  }
  // Must make sure everybody gets the right bundle map,
  // both for standalone and for normal operation.
  _graphicsServer.addDataBundlePathMap("SZG_DATA", _dataPath);
  _graphicsClient.addDataBundlePathMap("SZG_DATA", _dataPath);
  _soundServer.addDataBundlePathMap("SZG_DATA", _dataPath);
  _soundClient.addDataBundlePathMap("SZG_DATA", _dataPath);
  // Some data access might occur along the Python bundle path as well.
  string pythonPath = _SZGClient.getAttribute("SZG_PYTHON","path");
  // Do not warn if this is undefined.
  if (pythonPath != "NULL"){
    _graphicsServer.addDataBundlePathMap("SZG_PYTHON", pythonPath);
    _graphicsClient.addDataBundlePathMap("SZG_PYTHON", pythonPath);
    _soundServer.addDataBundlePathMap("SZG_PYTHON", pythonPath);
    _soundClient.addDataBundlePathMap("SZG_PYTHON", pythonPath);
  }

  _head.configure( _SZGClient );

  _loadNavParameters();
  
  _parametersLoaded = true;

  return true;
}

void arDistSceneGraphFramework::_initDatabases(){
  // If we are operating as a peer, use that. Otherwise, use the graphics
  // server.
  if (_peerName != "NULL"){
    dgSetGraphicsDatabase(&_graphicsPeer);
    _usedGraphicsDatabase = &_graphicsPeer;
  }
  else{
    dgSetGraphicsDatabase(&_graphicsServer);
    _usedGraphicsDatabase = &_graphicsServer;
  }
  dsSetSoundDatabase(&_soundServer);
  // If we are in standalone mode, we must start up the local connections.
  // This must occur before any alterations occur to the local 
  // graphics server since there is NO connection process in the standalone
  // case.
  if (_standalone){
    // In standalone mode, with manual buffer swapping, we need to be in "nosync" mode. 
    // This allows us to do things in only one thread, which is important for Python
    // code. (Otherwise there would be a deadlock)
    _graphicsServer._syncServer.setMode(_autoBufferSwap ? AR_SYNC_AUTO_SERVER : AR_NOSYNC_MANUAL_SERVER);
    _graphicsClient._cliSync.registerLocalConnection
      (&_graphicsServer._syncServer);
    _soundClient._cliSync.registerLocalConnection
      (&_soundServer._syncServer);
    // We'll be using the sound client locally to play. Must init it.
    (void)_soundClient.init();
  }
  else if (_peerName == "NULL"){
    // In phleet mode (but not peer mode), the _syncServer *always* syncs with its client.
    _graphicsServer._syncServer.setMode(_autoBufferSwap ? AR_SYNC_AUTO_SERVER : AR_SYNC_MANUAL_SERVER);
  }
  
  // Set up a default viewing position.
  _VRCameraID = dgViewer( "root", _head );
  
  // Set up a default listening position.
  dsPlayer( _head.getMatrix(), _head.getMidEyeOffset(), _unitSoundConversion );

  if (_graphicsNavNode == NULL){
    _graphicsNavNode 
      = (arTransformNode*) _usedGraphicsDatabase->getRoot()->newNode("transform", getNavNodeName());
    _graphicsNavNode->setTransform(ar_identityMatrix());
  }
  if (_soundNavMatrixID == -1){
    _soundNavMatrixID 
      = dsTransform(getNavNodeName(),"root",ar_identityMatrix());
  }
}

bool arDistSceneGraphFramework::_initInput(){
  _inputDevice = new arInputNode;
  if (!_inputDevice) {
    ar_log_error() << "arDistSceneGraphFramework error: failed to create input "
		   << "device.\n";
    return false;
  }
  _inputState = &(_inputDevice->_inputState);

  if (!_standalone){
    _inputDevice->addInputSource(&_netInputSource,false);
    _netInputSource.setSlot(0);
  }
  else{
    // we are in the standalone case and are using either an embedded
    // inputsimulator or a joystick
    _standaloneControlMode = _SZGClient.getAttribute("SZG_DEMO",
						     "control_mode",
						     "|simulator|joystick|");
    if (_standaloneControlMode == "simulator"){
      _simPtr->registerInputNode(_inputDevice);
    }
    else{
      // the joystick is the only other option so far
      arSharedLib* joystickObject = new arSharedLib();
      // It is annoying that, in here, we do not configure the arInputNode
      // like it is done in DeviceServer. This looses us alot of flexibility
      // that we might otherwise have.
      string sharedLibLoadPath = _SZGClient.getAttribute("SZG_EXEC","path");
      string pforthProgramName = _SZGClient.getAttribute("SZG_PFORTH",
                                                         "program_names");
      string error;
      if (!joystickObject->createFactory("arJoystickDriver", sharedLibLoadPath,
                                         "arInputSource", error)){
        ar_log_error() << error;
        return false;
      }
      arInputSource* driver = (arInputSource*) joystickObject->createObject();
      // The input node is not responsible for clean-up
      _inputDevice->addInputSource(driver, false);
      if (pforthProgramName == "NULL"){
        ar_log_remark() << "arDistSceneGraphFramework remark: no pforth program for "
	                << "standalone joystick.\n";
      }
      else{
        string pforthProgram 
          = _SZGClient.getGlobalAttribute(pforthProgramName);
        if (pforthProgram == "NULL"){
          ar_log_remark() << "arDistSceneGraphFramework remark: no pforth program exists "
	                  << "for name = " << pforthProgramName << "\n";
	}
        else{
          arPForthFilter* filter = new arPForthFilter();
          ar_PForthSetSZGClient( &_SZGClient );
      
          if (!filter->loadProgram( pforthProgram )){
	    ar_log_remark() << "arDistSceneGraphFramework remark: failed to configure "
		            << "pforth\nfilter with program =\n "
	                    << pforthProgram << "\n";
            return false;
	  }
          // The input node is not responsible for clean-up
          _inputDevice->addFilter(filter, false);
	}
      }
    }
  }

  _installFilters();

  if (!_inputDevice->init(_SZGClient) && !_standalone){
    ar_log_warning() << _label << " warning: failed to init input device.\n";
    if (!_SZGClient.sendInitResponse(false)){
      ar_log_error() << _label << " error: maybe szgserver died.\n";
    }
    return false;
  }
  return true;
}

bool arDistSceneGraphFramework::_stripSceneGraphArgs(int& argc, char** argv){
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i],"-dsg")){
      // we have found an arg that might need to be removed.
      if (i+1 >= argc){
        ar_log_critical() << "arDistSceneGraphFramework error: "
	                  << "-dsg flag is last in arg list. Abort.\n";
	return false;
      }
      // we need to figure out which args these are.
      // THIS IS COPY-PASTED FROM arSZGClient (where other key=value args
      // are parsed). SHOULD IT BE GLOBALIZED?
      const string thePair(argv[i+1]);
      unsigned int location = thePair.find('=');
      if (location == string::npos){
        ar_log_critical() << "arDistSceneGraphFramework error: "
	                  << "context pair does not contain =.\n";
        return false;
      }
      unsigned int length = thePair.length();
      if (location == length-1){
        ar_log_critical() << "arDistSceneGraphFramework error: context pair ends with =.\n";
        return false;
      }
      // So far, the only special key-value pair is:
      // peer=peer_name. If this exists, then we know we should
      // operate in a reality-peer-fashion.
      string key(thePair.substr(0,location));
      string value(thePair.substr(location+1, thePair.length()-location));
      if (key == "peer"){
        _peerName = value;
        ar_log_remark() << "arDistSceneGraphFramework remark: setting peer name = "
	                << _peerName << "\n";
        _graphicsPeer.setName(_peerName);
      }
      else if (key == "mode"){
        if (value != "feedback"){
          ar_log_critical() << "arDistSceneGraphFramework error: "
	                    << "value (" << value << ") for mode is illegal.\n";
	  return false;
	}
	_peerMode = value;
      }
      else if (key == "target"){
        _peerTarget = value;
      }
      else if (key == "root"){
	_remoteRootID = atoi(value.c_str());
      }
      else{
	ar_log_critical() << "arDistSceneGraphFramework error: key in arg pair is illegal "
	     << "(" << key << ").\n";
	return false;
      }
      
      // if they are parsed, then they should also be erased
      for (int j=i; j<argc-2; j++){
        argv[j] = argv[j+2];
      }
      // reset the arg count and i
      argc = argc-2;
      i--;
    }
  }
  return true;
}

/// Simplifies the start message processing. With the default parameter
/// value for f (false), this posts the given message to the start response
/// string and returns false. Otherwise, it just returns true.
bool arDistSceneGraphFramework::_startRespond(const string& s, bool f){
  if (!f){
    ar_log_error() << _label << " error: " << s << ar_endl;
  }
  if (!_SZGClient.sendStartResponse(f)){
    ar_log_error() << _label << " error: maybe szgserver died.\n";
  }
  return f;
}

bool arDistSceneGraphFramework::_initStandaloneMode(){
  _standalone = true;
  _loadParameters();
  _initDatabases();
  if (!_initInput()){
    return false;
  }
  ar_log_remark() << "arDistSceneGraphFramework remark: standalone mode objects successfully initialized.\n";
  return true;
}

/// Start the various framework objects in the manner demanded by standalone mode.
bool arDistSceneGraphFramework::_startStandaloneMode(){
  _simPtr->configure(_SZGClient);
  // Must configure the window manager here and pass it to the graphicsClient
  // before configure (because, inside graphicsClient configure, it is used
  // with the arGUIXMLParser).
  // By default, we ask for a non-threaded window manager.
  _wm = new arGUIWindowManager(ar_distSceneGraphGUIWindowFunction,
			       ar_distSceneGraphGUIKeyboardFunction,
			       ar_distSceneGraphGUIMouseFunction,
			       NULL,
			       false);
  _wm->setUserData(this);
  // The arGraphicsClient is the one that actually does the drawing and manages 
  // the windows.
  _graphicsClient.setWindowManager(_wm);
  _graphicsClient.configure(&_SZGClient);
  _graphicsClient.setSimulator(_simPtr);
  _framerateGraph.addElement("framerate", 300, 100, arVector3(1,1,1));
  _graphicsClient.addFrameworkObject(&_framerateGraph);
  arSpeakerObject* speakerObject = new arSpeakerObject();
  _soundClient.setSpeakerObject(speakerObject);
  _soundClient.configure(&_SZGClient);
  _soundServer.start();
  // NOTE: peer mode is meaningless in standalone.
  _graphicsServer.start();
  _inputDevice->start();
  // In standalone mode, we need to start a thread for the display.
  arThread graphicsThread;
  // In standalone mode, only begin an external thread if buffers will be automatically swapped.
  if (_autoBufferSwap){
    graphicsThread.beginThread(ar_distSceneGraphFrameworkWindowTask, this);
    // Wait until the thread has had a chance to try to create the windows.
    _windowsCreatedSignal.receiveSignal();
    // _windowsCreated is set by createWindows() in ar_distSceneGraphFrameworkTask.
    if (!_windowsCreated){
      // Important to *stop* the services that have already been started (before exiting).
      // Hmmm.... maybe this actually makes segfaults...
      //stop(true);
      return false;
    }
  }
  else{
    if (!createWindows(true)){
      // Important to *stop* the services that have already been started (before exiting).
      // Hmmm... maybe this actually makes segfaults...
      //stop(true);
      return false;
    }
  }
  ar_log_remark() << "arDistSceneGraphFramework remark: standalone mode objects successfully started.\n";
  return true;
}

bool arDistSceneGraphFramework::_initPhleetMode(){
  _loadParameters();
  
  if (!_messageThread.beginThread(ar_distSceneGraphFrameworkMessageTask,
                                  this)) {
    ar_log_error() << _label << " error: failed to start message thread.\n";
    if (!_SZGClient.sendInitResponse(false)){
      ar_log_error() << _label << " error: maybe szgserver died.\n";
    }
    return false;
  }
  
  // Figure out whether we should launch the other executables.
  if (_SZGClient.getMode("default") == "trigger"){
    const string vircomp = _SZGClient.getVirtualComputer();
    const string defaultMode = _SZGClient.getMode("default");
    ar_log_remark() << _label << " remark: executing on virtual computer "
      << vircomp << ",\n    with default mode " 
      << defaultMode << ".\n";
    
    (void)_launcher.setSZGClient(&_SZGClient);
    _vircompExecution = true;
    _launcher.setAppType("distgraphics");
    _launcher.setRenderProgram("szgrender");
    
    // Reorganizes the system...
    if (!_launcher.launchApp()){
      ar_log_error() << _label 
		     << " error: failed to launch on virtual computer "
                     << vircomp << ".\n";
      if (!_SZGClient.sendInitResponse(false)){
        ar_log_error() << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
  }
  
  // Gets the sound and graphics database going. These must be initialized
  // AFTER the virtual computer reshuffle (launchApp). Some OS configurations
  // may only allow ONE connection to the sound card, meaning that 
  // _soundClient.init() may block until a previous application goes away.
  // Thus, if _soundClient.init() occurs before launchApp (which will kill
  // other running applications, at least if virtual computers are being used
  // correctly) there can be a deadlock!
  _initDatabases();
  
  // Get the input device going
  if (!_initInput()){
    return false;
  }
    
  // NOTE: we choose either graphics server or peer, based on init.
  if (_peerName == "NULL"){
    if (!_graphicsServer.init(_SZGClient)){
      ar_log_error() << _label << " error: graphics server failed to init.\n";
      ar_log_error() << "  (THE LIKELY REASON IS THAT YOU ARE RUNNING ANOTHER "
	             << "APP).\n";
      if (!_SZGClient.sendInitResponse(false)){
        ar_log_error() << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
  }
  else{
    if (!_graphicsPeer.init(_SZGClient)){
      ar_log_error() << _label << " error: graphics peer failed to init.\n";
      if (!_SZGClient.sendInitResponse(false)){
        ar_log_error() << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
  }
  
  // The sound server is disabled in the peer case. Otherwise, multiple
  // apps using this framework cannot run simultaneously.
  if (_peerName == "NULL"){
    if (!_soundServer.init(_SZGClient)){
      ar_log_error() << _label << " error: sound server failed to init.\n"
                     << "  (Is another application running?)\n";
      if (!_SZGClient.sendInitResponse(false)){
        ar_log_error() << _label << " error: maybe szgserver died.\n";
      }
      return false;
    }
  }
  
  // init(...) succeeded.
  if (!_SZGClient.sendInitResponse(true)){
    ar_log_error() << _label << " error: maybe szgserver died.\n";
  }
  return true;
}

bool arDistSceneGraphFramework::_startPhleetMode(){
  ar_log_critical() << "Application is running in cluster mode. Visuals supplied by szgrender.\n";
  // Start the graphics database.
  if (_peerName == "NULL" || _standalone){
    // Not in peer mode.
    if (!_graphicsServer.start()){
      return _startRespond("graphics server failed to listen.");
    }
  }
  else{
    // Peer mode.
    if (!_graphicsPeer.start()){
      return _startRespond("graphics peer failed to start.");
    }
    
    // Connect to the specified target, if required.
    if (_peerTarget != "NULL"){
      if (_graphicsPeer.connectToPeer(_peerTarget) < 0){
        return _startRespond("graphics peer failed to connect to requested target=" + _peerTarget + ".");
      }
      // The following sets up a feedback relationship between the local and remote peers!
      _graphicsPeer.pushSerial(_peerTarget, _remoteRootID, 0, 
			       AR_TRANSIENT_NODE,
			       AR_TRANSIENT_NODE,
			       AR_TRANSIENT_NODE);
    }
  }
  
  // Don't start the input device if we are in peer mode!
  if (_peerName == "NULL" && !_inputDevice->start()){
    return _startRespond("failed to start input device.");
  }
  
  
  // Don't start the sound server if we are in peer mode! Otherwise, multiple
  // peers won't be able to simultaneously run (using this framework).
  if (_peerName == "NULL" && !_soundServer.start()){
    return _startRespond("sound server failed to listen.");
  }
  
  // Returns true. (_standRespond is a little cute)
  return _startRespond("", true);
}

