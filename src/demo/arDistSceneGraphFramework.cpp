//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsAPI.h"
#include "arGraphicsHeader.h"
#include "arSoundAPI.h"
#include "arJoystickDriver.h"
#include "arPForthFilter.h"
#include "arWildcatUtilities.h"
#include "arDistSceneGraphFramework.h"

void ar_distSceneGraphFrameworkMessageTask(void* framework){
  arDistSceneGraphFramework* f = (arDistSceneGraphFramework*) framework;
  string messageType, messageBody;
  while (true) {
    int sendID = f->_SZGClient.receiveMessage(&messageType, &messageBody);
    if (!sendID){
      // We have somehow been disconnected from the szgserver. We should
      // now quit. NOTE: we cannot kill the stuff elsewhere since we are
      // disconnected (i.e. we cannot manipulate the system anymore)
      
      // important that we stop the framework (the parameter is meaningless
      // so far)
      f->stop(true);
      exit(0);
    }
    if (messageType=="quit"){
      if (f->_vircompExecution){
        // we have an arAppLauncher object piggy-backing on us,
	// controlling a virtual computer's execution
	f->_launcher.killApp();
      }
      // important that we stop the framework (the parameter is meaningless
      // so far)
      f->stop(true);
      exit(0);
    } else if (messageType=="reload"){
      f->_loadParameters();
    }
  }
}

// GLUT functions for standalone mode
arDistSceneGraphFramework* __globalSceneGraphFramework;

void ar_distSceneGraphFrameworkDisplay(){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (__globalSceneGraphFramework->_standalone && 
      __globalSceneGraphFramework->_standaloneControlMode == "simulator"){
    __globalSceneGraphFramework->_simulator.advance();
  }
  __globalSceneGraphFramework->_soundClient._cliSync.consume();
  __globalSceneGraphFramework->_graphicsClient._cliSync.consume();
}

// AARGH! The button and mouse functions are largely copy-pasted from
// arMasterSlaveFramework.cpp
void ar_distSceneGraphFrameworkButtonFunction(int button, int state,
                                              int x, int y){
  // in standalone mode, button events should go to the interface
  if (__globalSceneGraphFramework->_standalone && 
      __globalSceneGraphFramework->_standaloneControlMode == "simulator"){
    // Must translate from the GLUT constants
    const int whichButton = 
    (button == GLUT_LEFT_BUTTON  ) ? 0:
    (button == GLUT_MIDDLE_BUTTON) ? 1 :
    (button == GLUT_RIGHT_BUTTON ) ? 2 : 0;
    const int whichState = (state == GLUT_DOWN) ? 1 : 0;
    __globalSceneGraphFramework->_simulator.mouseButton(whichButton, 
                                                        whichState, x, y);
  }
}

void ar_distSceneGraphFrameworkMouseFunction(int x, int y){
  // in standalone mode, mouse motion events should go to the interface
  if (__globalSceneGraphFramework->_standalone &&
      __globalSceneGraphFramework->_standaloneControlMode == "simulator"){
    __globalSceneGraphFramework->_simulator.mousePosition(x,y);
  }
}


void ar_distSceneGraphFrameworkKeyboard(unsigned char key, int x, int y){
  switch (key){
  case 27:
    // Stop the framework (parameter meaningless so far)
    __globalSceneGraphFramework->stop(true);
    exit(0);
  }
  // in standalone mode, keyboard events should also go to the interface
  if (__globalSceneGraphFramework->_standalone &&
      __globalSceneGraphFramework->_standaloneControlMode == "simulator"){
    __globalSceneGraphFramework->_simulator.keyboard(key, 1, x, y);
  }
}

// In standalone mode, we might actually like to run our own window,
// instead of having an szgrender do it.
void ar_distSceneGraphFrameworkWindowTask(void*){
  // Annoying to have to make dummy parameters
  int argc=1;
  char* argv[1];
  
  argv[0] = "dummy";
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(640,480);
  string label = __globalSceneGraphFramework->getLabel();
  glutCreateWindow(label.c_str());
  glutDisplayFunc(ar_distSceneGraphFrameworkDisplay);
  glutIdleFunc(ar_distSceneGraphFrameworkDisplay);
  glutMouseFunc(ar_distSceneGraphFrameworkButtonFunction);
  glutMotionFunc(ar_distSceneGraphFrameworkMouseFunction);
  glutPassiveMotionFunc(ar_distSceneGraphFrameworkMouseFunction);
  glutKeyboardFunc(ar_distSceneGraphFrameworkKeyboard);
  __globalSceneGraphFramework->_graphicsClient.init();
  glutMainLoop();
}

arDistSceneGraphFramework::arDistSceneGraphFramework() :
  arSZGAppFramework(),
  _midEyeOffset(0,0,0),
  _eyeDirection(1,0,0),
  _headMatrixID(AR_VR_HEAD_MATRIX_ID),
  _graphicsNavMatrixID(-1),
  _soundNavMatrixID(-1),
  _standalone(false),
  _standaloneControlMode("simulator"),
  _peerName("NULL"),
  _peerMode("source"),
  _peerTarget("NULL"){
}

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
  // Has no meaning if we are a peer. Standalone mode does not support peers.
  if (_peerName == "NULL" || _standalone){
    _graphicsServer._syncServer.autoBufferSwap(autoBufferSwap);
  }
}

void arDistSceneGraphFramework::swapBuffers(){
  // Has no meaning if we are a peer. Peer has no meaning in standalone.
  if (_peerName == "NULL" || _standalone){
    _graphicsServer._syncServer.swapBuffers();
  }
  else{
    // This is a necessary throttle for applications that would otherwise
    // rely on the swap timing of the remote display for a throttle.
    // NOTE: we are in peer mode here!
    ar_usleep(10000);
  }
}

void arDistSceneGraphFramework::setViewer(){
  dgViewer(_inputDevice->getMatrix(_headMatrixID),
	   _midEyeOffset, _eyeDirection, _eyeSpacingFeet,
	   _nearClip, _farClip, _unitConversion);
  if (_standalone){
    // In the STANDALONE case,
    // Want to make sure that any changes to the camera variables
    // get passed in to the arGraphicsWindow cameras. This is a HACK
    // and is copy-pasted from arMasterSlaveFramework.
    list<arViewport>* viewportList 
      = _graphicsClient.getGraphicsWindow()->getViewportList();
    list<arViewport>::iterator i;
    for (i=viewportList->begin(); i !=viewportList->end(); i++){
      arScreenObject* temp = (arScreenObject*) (*i).getCamera();
      temp->setEyeSpacing( _eyeSpacingFeet );
      temp->setClipPlanes( _nearClip, _farClip );
      temp->setUnitConversion( _unitConversion );
    } 
  }
}

void arDistSceneGraphFramework::setPlayer(){
  dsPlayer(_inputDevice->getMatrix(_headMatrixID),
	   _midEyeOffset,
	   _unitSoundConversion);
}

// WHAT IN THE HECK DOES THIS METHOD DO?
void arDistSceneGraphFramework::setHeadMatrixID(int id) {
  _headMatrixID = id;
}

arDatabaseNode* arDistSceneGraphFramework::getNavNode(){
  // Standalone mode does not support peers.
  if (_peerName == "NULL" || _standalone){
    return _graphicsServer.getNode("SZG_NAV_MATRIX");
  }
  else{
    return _graphicsPeer.getNode("SZG_NAV_MATRIX");
  }
}

void arDistSceneGraphFramework::loadNavMatrix() {
  const arMatrix4 navMatrix( ar_getNavInvMatrix() );
  if (_graphicsNavMatrixID != -1)
    dgTransform( _graphicsNavMatrixID, navMatrix );
  if (_soundNavMatrixID != -1)
    dsTransform( _soundNavMatrixID, navMatrix );
}

bool arDistSceneGraphFramework::init(int& argc, char** argv){
  _label = string(argv[0]);
  _label = ar_stripExeName(_label); // remove pathname and .EXE
  // See if there are special -dsg args in the arg list and strip them out.
  if (!_stripSceneGraphArgs(argc, argv)){
    return false;
  }

  // connect to the szgserver
  _SZGClient.simpleHandshaking(false);
  _SZGClient.init(argc, argv);
  if (!_SZGClient){
    // We must be in standalone mode.
    _standalone = true;
    // This global variable is needed for GLUT, unfortunately
    __globalSceneGraphFramework = this;
    cout << _label << " remark: RUNNING IN STANDALONE MODE. "
	 << "NO DISTRIBUTION.\n";
    _loadParameters();
    _initDatabases();
    if (!_initInput()){
      return false;
    }
    // we are done because we are in standalone mode
    return true;
  }

  // This is what happens if we are, in fact, connected to an szgserver

  // we want to send to the init response stream
  stringstream& initResponse = _SZGClient.initResponse();
  _loadParameters();

  if (!_messageThread.beginThread(ar_distSceneGraphFrameworkMessageTask,
                                  this)) {
    initResponse << _label << " error: failed to start message thread.\n";
    _SZGClient.sendInitResponse(false);
    return false;
  }

  // Figure out whether we should launch the other executables.
  if (_SZGClient.getMode("default") == "trigger"){
    const string vircomp = _SZGClient.getVirtualComputer();
    const string defaultMode = _SZGClient.getMode("default");
    initResponse << _label << " remark: executing on virtual computer "
	         << vircomp << ",\n    with default mode " 
                 << defaultMode << ".\n";
   
    (void)_launcher.setSZGClient(&_SZGClient);
    _vircompExecution = true;
    _launcher.setAppType("distgraphics");
    _launcher.setRenderProgram("szgrender");

    // Reorganizes the system...
    if (!_launcher.launchApp()){
      initResponse << _label 
                   << " error: failed to launch on virtual computer "
                   << vircomp << ".\n";
      _SZGClient.sendInitResponse(false);
      return false;
    }
  }

  // Gets the sound and graphics database going.
  _initDatabases();

  // Get the input device going
  if (!_initInput()){
    return false;
  }

  // THIS IS A LITTLE AWKWARD! Maybe at some point I'll want to combine
  // the init and start methods together!

  // NOTE: we choose either graphics server or peer, based on init.
  if (_peerName == "NULL" || _standalone){
    if (!_graphicsServer.init(_SZGClient)){
      initResponse << _label << " error: graphics server failed to init.\n";
      _SZGClient.sendInitResponse(false);
      return false;
    }
  }
  else{
    if (!_graphicsPeer.init(_SZGClient)){
      initResponse << _label << " error: graphics peer failed to init.\n";
      _SZGClient.sendInitResponse(false);
      return false;
    }
  }

  // SOUND SERVER DOES NOT RUN IN THE PEER CASE (OTHERWISE WE CANNOT HAVE
  // MULITPLE PEERS BASED ON THE SCENE GRAPH FRAMEWORK)
  if (_peerName == "NULL" || _standalone){
    if (!_soundServer.init(_SZGClient)){
      initResponse << _label << " error: sound server failed to init.\n"
                   << "  (Is another application running?)\n";
      _SZGClient.sendInitResponse(false);
      return false;
    }
  }

  // we succeeded in initing
  _SZGClient.sendInitResponse(true);
  return true;
}

bool arDistSceneGraphFramework::start(){
  // We have two pathways depending upon whether or not we are in standalone
  // mode.
  if (_standalone){
    _graphicsClient.configure(&_SZGClient);
    _graphicsClient.setSimulator(&_simulator);
    arSpeakerObject* speakerObject = new arSpeakerObject();
    _soundClient.setSpeakerObject(speakerObject);
    _soundClient.configure(&_SZGClient);
    _soundServer.start();
    // NOTE: peer mode is meaningless in standalone.
    _graphicsServer.start();
    _inputDevice->start();
    // in standalone mode, we need to start a thread for the display
    arThread graphicsThread;
    graphicsThread.beginThread(ar_distSceneGraphFrameworkWindowTask, this);
    return true;
  }
  
  // The distributed case (i.e. connected to an szgserver and not in
  // standalone).

  // we want to pack the start response stream
  stringstream& startResponse = _SZGClient.startResponse();

  if (!_inputDevice->start()) {
    startResponse << _label << " error: failed to start input device.\n";
    _SZGClient.sendStartResponse(false);
    return false;
  }

  // We might be in peer mode.
  if (_peerName == "NULL" || _standalone){
    if (!_graphicsServer.start()) {
      startResponse << _label << " error: graphics server failed to listen.\n";
      _SZGClient.sendStartResponse(false);
      return false;
    }
  }
  else{
    // Peer mode.
    // Are we using the local database?
    if (_peerMode == "shell"){
      _graphicsPeer.useLocalDatabase(false);
    }
    if (!_graphicsPeer.start()) {
      startResponse << _label << " error: graphics peer failed to start.\n";
      _SZGClient.sendStartResponse(false);
      return false;
    }
    // Connect to the specified target, if required.
    if (_peerMode == "shell" || _peerMode == "feedback"){
      if (_peerTarget == "NULL"){
        startResponse << _label << " error: target not specified.\n";
	return false;
      }
      else{
        if (!_graphicsPeer.connectToPeer(_peerTarget)){
	  startResponse << _label << " error: graphics peer failed to connect "
	  	        << "to requested target=" << _peerTarget << ".\n";
	  return false;
        }
        // In either case, we'll be sending data to the target.
        _graphicsPeer.sending(_peerTarget, true);
        // In the feedback case, we want a dump and relay.
        if (_peerMode == "feedback"){
          _graphicsPeer.pullSerial(_peerTarget, true);
	}
      }
    }
    else{
      if (_peerTarget != "NULL"){
	startResponse << _label << " error: target improperly specified for "
		      << "source mode.\n";
	return false;
      }
    }
  }

  // SOUND SERVER DOES NOT RUN IN THE PEER CASE (OTHERWISE WE CANNOT HAVE
  // MULITPLE PEERS BASED ON THE SCENE GRAPH FRAMEWORK)
  if (_peerName == "NULL" || _standalone){
    if (!_soundServer.start()) {
      startResponse << _label << " warning: sound server failed to listen.\n";
      _SZGClient.sendStartResponse(false);
      return false;
    }
  }

  // we succeeded in starting
  _SZGClient.sendStartResponse(true);
  return true;
}

/// Does the best it can to shut components down. Don't actually do anything
/// with the parameter (blockUntilDisplayExit) because we only run a 
/// local display in standalone mode and hence do not need to worry about
/// getting a stop message from the message thread (which isn't connected 
/// to anything in this case).
void arDistSceneGraphFramework::stop(bool){
  // important to send an extra buffer swap. otherwise, there
  // can be a deadlock in the szgrender exiting. THIS IS A HACK!
  // Actually, I'm a bit unsure about whether this should be in the code...
  // There could be a simultaneous issue of a swapBuffers() by the
  // application... Trying to see if it can be left out...
  // NOTE: I originally DID NOT have this in here. Trying to debug a
  // weird hardware-realted/frustum-related (!!) system lock-up on windows
  // systems that seems to occur on cosmos exit (i.e. manual sync app exit)
  //_graphicsServer._syncServer.swapBuffers();

  // must let the application know that we are stopping
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
  cout << "arDistSceneGraphFramework remark: threads done.\n";
  _stopped = true;
}

bool arDistSceneGraphFramework::restart(){
  // Stop the framework and restart (the stop parameter is meaningless so far)
  stop(true);
  return start();
}

int arDistSceneGraphFramework::getNodeID(const string& name){
  if (_peerName == "NULL"){
    return _graphicsServer.getNodeID(name);
  }  
  if (_peerMode == "source"){
    return _graphicsPeer.getNodeID(name);
  }
  return _graphicsPeer.getNodeIDRemote(_peerTarget, name);
}

arDatabaseNode* arDistSceneGraphFramework::getNode(int ID){
  // only makes sense if we are running normally OR if we are in
  // "feedback" mode and consequently have a database copy.
  if (_peerName == "NULL"){
    return _graphicsServer.getNode(ID);
  }
  else if (_peerMode == "feedback"){
    return _graphicsPeer.getNode(ID);
  }
  return NULL;
}

bool arDistSceneGraphFramework::lockNode(int ID){
  if (_peerName == "NULL" || _peerMode == "source"){
    // does it make sense to locally lock a node? Maybe... but this isn't
    // implemented yet.
    return false;
  }
  return _graphicsPeer.lockRemoteNode(_peerTarget, ID);
}

bool arDistSceneGraphFramework::unlockNode(int ID){
  if (_peerName == "NULL" || _peerMode == "source"){
    // does it make sense to locally unlock a node? Maybe... but this isn't
    // implemented yet.
    return false;
  }
  return _graphicsPeer.unlockRemoteNode(_peerTarget, ID);
}

// If the attribute exists, stuff its value into v.
// Otherwise report v's (unchanged) default value.
// Sort of copypasted with arScreenObject::_getVector3.
void arDistSceneGraphFramework::_getVector3(arVector3& v, const char* param){
  // use the screen name passed from the distributed system
  const string screenName(_SZGClient.getMode("graphics"));
  if (!_SZGClient.getAttributeFloats(screenName, param, v.v, 3)) {
    // Less noisy -- this isn't all that informative in practice.
    //_SZGClient.initResponse() << _label << " remark: " << screenName
    //             << "/" << param  << " defaulting to " << v << endl;
  }
}

bool arDistSceneGraphFramework::_loadParameters(){
  // we need to send to the init response stream
  stringstream& initResponse = _SZGClient.initResponse();
  
  // data config
  _dataPath = _SZGClient.getAttribute("SZG_DATA", "path");
  if (_dataPath == "NULL"){
    initResponse << _label << " warning: SZG_DATA/path undefined.\n";
  }

  // use the screen name passed from the distributed system
  string screenName(_SZGClient.getMode("graphics"));
  // Eye/head parameters, copypasted with arScreenObject::configure().
  if (!_SZGClient.getAttributeFloats(screenName, "eye_spacing", 
                                     &_eyeSpacingFeet))
    initResponse << _label << " remark: "
		 << screenName << "/eye_spacing defaulting to "
	         << _eyeSpacingFeet << endl;
  _getVector3(_eyeDirection, "eye_direction");
  _getVector3(_midEyeOffset, "mid_eye_offset");

  // in standalone mode, we might actually use a window

  ar_useWildcatFramelock(_SZGClient.getAttribute(screenName, 
                                                 "wildcat_framelock",
                                                 "|false|true|") == "true");
  bool stereoMode 
    = _SZGClient.getAttribute(screenName, "stereo", "|false|true|") == "true";
  _graphicsClient.setStereoMode(stereoMode);
  
  _loadNavParameters();

  return true;
}

void arDistSceneGraphFramework::_initDatabases(){
  // If we are operating as a peer, use that. Otherwise, use the graphics
  // server.
  if (_peerName != "NULL"){
    dgSetGraphicsDatabase(&_graphicsPeer);
  }
  else{
    dgSetGraphicsDatabase(&_graphicsServer);
  }
  dsSetSoundDatabase(&_soundServer);
  // if we are in standalone mode, we must start up the local connections.
  // NOTE: we cannot be in peer mode on standalone.
  if (_standalone){
    _graphicsClient._cliSync.registerLocalConnection
      (&_graphicsServer._syncServer);
    _soundClient._cliSync.registerLocalConnection
      (&_soundServer._syncServer);
  }
  
  // Set up a default viewing position.
  dgViewer(arMatrix4(1,0,0,0,
		     0,1,0,5,
		     0,0,1,0,
		     0,0,0,1),
	   _midEyeOffset, _eyeDirection, _eyeSpacingFeet,
	   _nearClip, _farClip, _unitConversion);

  
  // Set up a default listening position.
  dsPlayer(arMatrix4(1,0,0,0,
		     0,1,0,5,
		     0,0,1,0,
		     0,0,0,1),
	   _midEyeOffset,
	   _unitSoundConversion);

  if (_graphicsNavMatrixID == -1){
    _graphicsNavMatrixID 
      = dgTransform(getNavNodeName(),"root",ar_identityMatrix());
  }
  if (_soundNavMatrixID == -1){
    _soundNavMatrixID 
      = dsTransform(getNavNodeName(),"root",ar_identityMatrix());
  }
}

bool arDistSceneGraphFramework::_initInput(){
  stringstream& initResponse = _SZGClient.initResponse();
  _inputDevice = new arInputNode;
  if (!_inputDevice) {
    cerr << "arDistSceneGraphFramework error: failed to create input "
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
    // wandsimserver or a joystick
    _standaloneControlMode = _SZGClient.getAttribute("SZG_DEMO",
						     "control_mode",
						     "|simulator|joystick|");
    if (_standaloneControlMode == "simulator"){
      _simulator.registerInputNode(_inputDevice);
    }
    else{
      // the joystick is the only other option so far
      arJoystickDriver* driver = new arJoystickDriver();
      // The input node is not responsible for clean-up
      _inputDevice->addInputSource(driver, false);
      arPForthFilter* filter = new arPForthFilter();
      if (!filter->configure( &_SZGClient )){
        return false;
      }
      // The input node is not responsible for clean-up
      _inputDevice->addFilter(filter, false);
    }
  }

  if (!_inputDevice->init(_SZGClient) && !_standalone){
    initResponse << _label << " warning: failed to init input device.\n";
    _SZGClient.sendInitResponse(false);
    return false;
  }
  return true;
}

bool arDistSceneGraphFramework::_stripSceneGraphArgs(int& argc, char** argv){
  for (int i=0; i<argc; i++){
    if (!strcmp(argv[i],"-dsg")){
      // we have found an arg that might need to be removed.
      if (i+1 >= argc){
        cout << "arDistSceneGraphFramework error: "
	     << "-dsg flag is last in arg list. Abort.\n";
	return false;
      }
      // we need to figure out which args these are.
      // THIS IS COPY-PASTED FROM arSZGClient (where other key=value args
      // are parsed). SHOULD IT BE GLOBALIZED?
      const string thePair(argv[i+1]);
      unsigned int location = thePair.find('=');
      if (location == string::npos){
        cout << "arDistSceneGraphFramework error: "
	     << "context pair does not contain =.\n";
        return false;
      }
      unsigned int length = thePair.length();
      if (location == length-1){
        cout << "arDistSceneGraphFramework error: context pair ends with =.\n";
        return false;
      }
      // So far, the only special key-value pair is:
      // peer=peer_name. If this exists, then we know we should
      // operate in a reality-peer-fashion.
      string key(thePair.substr(0,location));
      string value(thePair.substr(location+1, thePair.length()-location));
      if (key == "peer"){
        _peerName = value;
        cout << "arDistSceneGraphFramework remark: setting peer name = "
	     << _peerName << "\n";
        _graphicsPeer.setName(_peerName);
      }
      else if (key == "mode"){
        if (value != "source" && value != "shell" && value != "feedback"){
          cout << "arDistSceneGraphFramework error: "
	       << "value for mode is illegal.\n";
	  return false;
	}
	_peerMode = value;
      }
      else if (key == "target"){
        _peerTarget = value;
      }
      else{
	cout << "arDistSceneGraphFramework error: key in arg pair is illegal "
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
