//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#ifndef AR_USE_WIN_32
#include <sys/types.h>
#include <signal.h>
#endif
#include "arSharedLib.h"
#include "arEventUtilities.h"
#include "arWildcatUtilities.h"
#include "arPForthFilter.h"
#include "arVRConstants.h"

/// to make the callbacks startable from GLUT, we need this
/// global variable to pass in an arMasterSlaveFramework parameter
arMasterSlaveFramework* __globalFramework = NULL;

//***********************************************************************
// GLUT callbacks
//***********************************************************************

void ar_masterSlaveFrameworkMessageTask(void* p){
  ((arMasterSlaveFramework*)p)->_messageTask();
}

void ar_masterSlaveFrameworkConnectionTask(void* p){
  ((arMasterSlaveFramework*)p)->_connectionTask();
}

void ar_masterSlaveFrameworkReshapeFunction(int width, int height){
  if (!__globalFramework)
    return;

  __globalFramework->_windowSizeX = width;
  __globalFramework->_windowSizeY = height;

  if (__globalFramework->_reshape)
    __globalFramework->_reshape( *__globalFramework, width, height );
  else
    glViewport( 0, 0, width, height );
}

void ar_masterSlaveFrameworkDisplayFunction(){
  if (__globalFramework)
    __globalFramework->_display();
}

void ar_masterSlaveFrameworkButtonFunction(int button, int state,
                                           int x, int y){
  // in standalone mode, button events should go to the interface
  if (__globalFramework->_standalone && 
      __globalFramework->_standaloneControlMode == "simulator"){
    // Must translate from the GLUT constants
    const int whichButton = 
    (button == GLUT_LEFT_BUTTON  ) ? 0:
    (button == GLUT_MIDDLE_BUTTON) ? 1 :
    (button == GLUT_RIGHT_BUTTON ) ? 2 : 0;
    const int whichState = (state == GLUT_DOWN) ? 1 : 0;
    __globalFramework->_simulator.mouseButton(whichButton, whichState, x, y);
  }
}

void ar_masterSlaveFrameworkMouseFunction(int x, int y){
  // in standalone mode, mouse motion events should go to the interface
  if (__globalFramework->_standalone &&
      __globalFramework->_standaloneControlMode == "simulator"){
    __globalFramework->_simulator.mousePosition(x,y);
  }
}

/// ESC: quit, 'f': full screen, 'F': 600x600 window
void ar_masterSlaveFrameworkKeyboardFunction(unsigned char key, int x, int y){
  if (__globalFramework){
    // do not process key strokes after we have begun shutdown
    if (__globalFramework->_exitProgram){
      return;
    }
  }
  switch (key){
  case 27: /* escape key */
    if (__globalFramework){
      // We do not block until the display thread is done... but we do
      // block on everything else.
      __globalFramework->stop(false);
    }
    // NOTE: we do not exit(0) here. Instead, that occurs in the display
    // thread!
    break;
  case 'f':
    glutFullScreen();
    break;
  case 'F':
    glutReshapeWindow(600,600);
    break;
  case 'P':
    __globalFramework->_showPerformance = !__globalFramework->_showPerformance;
    break;
  case 't':
    cout << "Frame time = " << __globalFramework->_lastFrameTime << "\n";
    break;
  }
  
  // in standalone mode, keyboard events should also go to the interface
  if (__globalFramework->_standalone &&
      __globalFramework->_standaloneControlMode == "simulator"){
    __globalFramework->_simulator.keyboard(key, 1, x, y);
  }

  // finally, keyboard events should be forwarded to the keyboard callback
  // *if* we are the master and *if* the callback has been defined.
  if (__globalFramework->getMaster() 
      && __globalFramework->_keyboardCallback){
    __globalFramework->_keyboardCallback(*__globalFramework, key, x, y);
  }
}

//***********************************************************************
// arGraphicsWindow callback classes
//***********************************************************************
class arMasterSlaveWindowInitCallback : public arWindowInitCallback {
  public:
    arMasterSlaveWindowInitCallback( arMasterSlaveFramework& fw,
                                     void (*cb)( arMasterSlaveFramework& )
                                     ) :
       _framework( &fw ),
       _callback( cb ) {}
    ~arMasterSlaveWindowInitCallback() {}
    void operator()( arGraphicsWindow& );
  private:
    arMasterSlaveFramework* _framework;
    void (*_callback)( arMasterSlaveFramework& );
};  
void arMasterSlaveWindowInitCallback::operator()( arGraphicsWindow& ) {
  if (!_framework)
    return;
  _callback( *_framework );
}

class arMasterSlaveRenderCallback : public arRenderCallback {
  public:
    arMasterSlaveRenderCallback( arMasterSlaveFramework& fw ) :
      _framework( &fw ) {}
    ~arMasterSlaveRenderCallback() {}
    void operator()( arGraphicsWindow&, arViewport& );
  private:
    arMasterSlaveFramework* _framework;
};
void arMasterSlaveRenderCallback::operator()( arGraphicsWindow&, 
                                              arViewport& v ) {
  if (!_framework)
    return;
  _framework->_draw();
}


//***********************************************************************
// arMasterSlaveFramework public methods
//***********************************************************************

arMasterSlaveFramework::arMasterSlaveFramework():
  arSZGAppFramework(),
  _transferTemplate("data"),
  _userInitCalled(false),
  _parametersLoaded(false),
  _serviceName("NULL"),
  _serviceNameBarrier("NULL"),
  _networks("NULL"),
  _init(NULL),
  _preExchange(NULL),
  _postExchange(NULL),
  _window(NULL),
  _drawCallback(NULL),
  _play(NULL),
  _reshape(NULL),
  _cleanup(NULL),
  _userMessageCallback(NULL),
  _overlay(NULL),
  _keyboardCallback(NULL),
  _stereoMode(false),
  _internalBufferSwap(true),
  _framerateThrottle(false),
  _barrierServer(NULL),
  _barrierClient(NULL),
  _master(true),
  _stateClientConnected(false),
  _inputActive(false),
  _soundActive(false),
  _inBuffer(NULL),
  _inBufferSize(-1),
  _time(0.),
  _lastFrameTime(0.1),
  _firstTimePoll(true),
  _screenshotFlag(false),
  _screenshotStartX(0),
  _screenshotStartY(0),
  _screenshotWidth(640),
  _screenshotHeight(480),
  _whichScreenshot(0),
  _pauseFlag(false),
  _connectionThreadRunning(false),
  _useGLUT(false),
  _standalone(false),
  _standaloneControlMode("simulator"){

  // also need to add fields for our default-shared data
  _transferTemplate.addAttribute("time",AR_DOUBLE);
  _transferTemplate.addAttribute("lastFrameTime",AR_DOUBLE);
  _transferTemplate.addAttribute("navMatrix",AR_FLOAT);
  _transferTemplate.addAttribute("signature",AR_INT);
  _transferTemplate.addAttribute("types",AR_INT);
  _transferTemplate.addAttribute("indices",AR_INT);
  _transferTemplate.addAttribute("matrices",AR_FLOAT);
  _transferTemplate.addAttribute("buttons",AR_INT);
  _transferTemplate.addAttribute("axes",AR_FLOAT);
  _transferTemplate.addAttribute("eye_spacing",AR_FLOAT);
  _transferTemplate.addAttribute("mid_eye_offset",AR_FLOAT);
  _transferTemplate.addAttribute("eye_direction",AR_FLOAT);
  _transferTemplate.addAttribute("head_matrix",AR_FLOAT);
  _transferTemplate.addAttribute("near_clip",AR_FLOAT);
  _transferTemplate.addAttribute("far_clip",AR_FLOAT);
  _transferTemplate.addAttribute("unit_conversion",AR_FLOAT);
  _transferTemplate.addAttribute("fixed_head_mode",AR_INT);
  _transferTemplate.addAttribute("szg_data_router",AR_CHAR);

  ar_mutex_init(&_pauseLock);
  ar_mutex_init(&_eventLock);
  // when the default color is set like this, the app's gemetry is displayed
  // instead of a default color
  _defaultColor = arVector3(-1,-1,-1);
  _masterPort[0] = -1;
  // Also, let's initialize the performance graph.
  _framerateGraph.addElement("framerate", 300, 100, arVector3(1,1,1));
  _framerateGraph.addElement("compute", 300, 100, arVector3(1,1,0));
  _framerateGraph.addElement("sync", 300, 100, arVector3(0,1,1));
  _showPerformance = false;

  _defaultCamera.setHead( &_head );
  
}

/// \bug memory leak for several pointer members
arMasterSlaveFramework::~arMasterSlaveFramework() {
  // DO NOT DELETE _screenObject in here. WE MIGHT NOT OWN IT.

  arTransferFieldData::iterator iter;
  for (iter = _internalTransferFieldData.begin();
        iter != _internalTransferFieldData.end(); iter++) {
    if (iter->second.data)
      ar_deallocateBuffer( iter->second.data );
  }
  _internalTransferFieldData.clear();
  if (!_master && _inputState)
    delete _inputState;
}

void arMasterSlaveFramework::setInitCallback
  (bool (*initCallback)(arMasterSlaveFramework&, arSZGClient&)){
  _init = initCallback;
}

void arMasterSlaveFramework::setPreExchangeCallback
  (void (*preExchangeCallback)(arMasterSlaveFramework&)){
  _preExchange = preExchangeCallback;
}

void arMasterSlaveFramework::setPostExchangeCallback
  (bool (*postExchange)(arMasterSlaveFramework&)){
  _postExchange = postExchange;
}

/// The window callback is called once per window, per frame, if set.
/// If it is not set, the _drawSetUp method is called instead. This callback
/// is needed since we may want several different views in a single window,
/// with a draw callback filling each. This is especially useful for passive
/// stereo from a single box, but will not work if the draw callback includes
/// code that clears the entire buffer. Hence such code needs to moved
/// into this sort of callback.
void arMasterSlaveFramework::setWindowCallback
  (void (*windowCallback)(arMasterSlaveFramework&)){
  _graphicsWindow.setInitCallback
    ( new arMasterSlaveWindowInitCallback(*this,windowCallback) );
}

void arMasterSlaveFramework::setDrawCallback
  (void (*draw)(arMasterSlaveFramework&)){
  _drawCallback = draw;
}

void arMasterSlaveFramework::setPlayCallback
  (void (*play)(arMasterSlaveFramework&)){
  _play = play;
}

void arMasterSlaveFramework::setReshapeCallback
  (void (*reshape)(arMasterSlaveFramework&,int,int)) {
  _reshape = reshape;
}

void arMasterSlaveFramework::setExitCallback
  (void (*cleanup)(arMasterSlaveFramework&)){
  _cleanup = cleanup;
}

/// Syzygy messages currently consist of two strings, the first being
/// a type and the second being a value. The user can send messages
/// to the arMasterSlaveFramework and the application can trap them
/// using this callback. A message w/ type "user" and value "foo" will
/// be passed into this callback, if set, with "foo" going into the string.
void arMasterSlaveFramework::setUserMessageCallback
  (void (*userMessageCallback)(arMasterSlaveFramework&, const string&)){
  _userMessageCallback = userMessageCallback;
}

/// In general, the graphics window can be a complicated sequence of
/// viewports (as is necessary for simulating a CAVE or Cube). Sometimes
/// the application just wants to draw something once, which is the
/// purpose of this callback.
void arMasterSlaveFramework::setOverlayCallback
  (void (*overlay)(arMasterSlaveFramework&)){
  _overlay = overlay;
}

/// A master instance will also take keyboard input via this callback,
/// if defined.
void arMasterSlaveFramework::setKeyboardCallback
  (void (*keyboard)(arMasterSlaveFramework&, unsigned char, int, int)){
  _keyboardCallback = keyboard;
}

/// Initializes the syzygy objects, but does not start any threads
bool arMasterSlaveFramework::init(int& argc, char** argv){
  _label = string(argv[0]);
  _label = ar_stripExeName(_label);

  // Connect to the szgserver.
  _SZGClient.simpleHandshaking(false);
  _SZGClient.init(argc, argv);
  if (!_SZGClient){
    // HACK!!! For right now, if the arSZGClient fails to init, we assume
    // that we should try to run everything in "standalone" mode....
    // SO... we don't really want to return false here.
    cout << _label << " remark: RUNNING IN STANDALONE MODE. "
	 << "NO DISTRIBUTION.\n";
    _standalone = true;
    // Furthermore, in standalone mode, this instance MUST be the master!
    _setMaster(true);
    // HACK!!!!! There's cutting-and-pasting here...
    dgSetGraphicsDatabase(&_graphicsDatabase);
    dsSetSoundDatabase(&_soundServer);
    if (!_loadParameters()){
      cerr << _label << " remark: COULD NOT LOAD PARAMETERS IN STANDALONE "
	   << "MODE.\n";
    }
    if (!_initStandaloneObjects()){
      // NOTE: It is definitely possible for initialization of the standalone
      // objects to fail. For instance, what if we are unable to load
      // the joystick driver (which is a loadable module) or what if
      // the configuration of the pforth filter fails?
      return false;
    }
    _parametersLoaded = true;
    // We do not start the message-receiving thread yet (because there's no
    // way yet to operate in a distributed fashion). We also do not
    // initialize the master's objects... since they will be unused!
    return true;
  }

  // the init responses need to go to the client's initResponse stream
  stringstream& initResponse = _SZGClient.initResponse();

  // Initialize a few things.
  dgSetGraphicsDatabase(&_graphicsDatabase);
  dsSetSoundDatabase(&_soundServer);

  // Load the parameters before executing any user callbacks
  // (since this determines if we're master or slave).
  if (!_loadParameters())
    goto fail;

  // Figure out whether we should launch the other executables.
  // If so, under certain circumstances, this function may not return.
  // NOTE: regardless of whether or not we'll be launching from here
  // in trigger mode, we want to be able to use the arAppLauncher object
  // to query info about the virtual computer, which requires the arSZGClient
  // be registered with the arAppLauncher
  (void)_launcher.setSZGClient(&_SZGClient);
  if (_SZGClient.getMode("default") == "trigger"){
    // if we are the trigger node, we launch the rest of the application
    // components... and then WAIT for the exit.
    string vircomp = _SZGClient.getVirtualComputer();
    // we are, in fact, executing as part of a virtual computer
    _vircompExecution = true;

    const string defaultMode = _SZGClient.getMode("default");
    initResponse << _label << " remark: executing on virtual computer "
	         << vircomp << ",\n    with default mode "
                 << defaultMode << ".\n";
    _launcher.setAppType("distapp");

    // The render program is the (stripped) EXE name, plus parameters.
    {
      char* exeBuf = new char[_label.length()+1];
      ar_stringToBuffer(_label, exeBuf, _label.length()+1);
      char* temp = argv[0];
      argv[0] = exeBuf;
      _launcher.setRenderProgram(ar_packParameters(argc, argv));
      argv[0] = temp;
      delete exeBuf;
    }

    // Reorganizes the virtual computer.
    if (!_launcher.launchApp()){
      initResponse << _label 
         << " error: failed to launch on virtual computer "
	 << vircomp << ".\n";
      goto fail;
    }

    // Wait for the message (render nodes do this in the message task).
    // we've suceeded in initing
    _SZGClient.sendInitResponse(true);
    // there's no more starting to do... since this application instance
    // is just used as a launcher for other instances
    _SZGClient.startResponse() << _label << " trigger launched components.\n";
    _SZGClient.sendStartResponse(true);
    (void)_launcher.waitForKill();
    exit(0);
  }

  // Mode isn't trigger.

  // NOTE: sometimes user programs need to be able to determine
  // characteristics of the virtual computer. Hence, that information must be
  // available (to EVERYONE... and not just the master!)
  // Consequently, setParameters() should be called. However, it is
  // desirable to allow the user to set a BOGUS virtual computer via the
  // -szg command line flags. Consequently, it is merely a warning and
  // NOT a fatal error if setParameters() fails... i.e.
  // my_program -szg virtual=foo
  // should launch

  if (_SZGClient.getVirtualComputer() != "NULL" &&
     !_launcher.setParameters()){
    initResponse << _label 
		 << " warning: invalid virtual computer definition.\n";
  }

  // Launch the message thread here, so dkill still works
  // even if init() or start() fail. 
  // But launch after the trigger code above, lest we catch messages both
  // in the waitForKill() AND in the message thread.

  if (!_messageThread.beginThread(ar_masterSlaveFrameworkMessageTask, this)){
    initResponse << _label << " error: failed to start message thread.\n";
    goto fail;
  }

  if (!_determineMaster(initResponse))
    goto fail;

  // init the objects, either master or slave
  if (getMaster()){
    if (!_initMasterObjects(initResponse))
      goto fail;
  }
  else{
    if (!_initSlaveObjects(initResponse)){
fail:
      _SZGClient.sendInitResponse(false);
      return false;
    }
  }

  _parametersLoaded = true;
  _SZGClient.sendInitResponse(true);
  return true;
}

/// Starts the application, using the GLUT event loop. NOTE: this call does
/// return in the event of success, since glutMainLoop() does not return.
bool arMasterSlaveFramework::start(){
  // call _start and tell it that, yes, we are using GLUT.
  // AN ANNOYING HACK
  return _start(true);
}

/// Starts the application, but does not create a window or start the GLUT 
/// event loop.
bool arMasterSlaveFramework::startWithoutGLUT(){
  // call _start and tell it that, no, we are not using GLUT
  // AN ANNOYING HACK
  return _start(false);
}

/// Begins halting many significant parts of the object and blocks until
/// they have, indeed, halted. The parameter will be false if we have been
/// called from the GLUT keyboard function (that way be will not wait for 
/// the display thread to finish, resulting in a deadlock, since the
/// keyboard function is called from that thread). The parameter will be
/// true, on the other hand, if we are calling from the message-receiving
/// thread (which is different from the display thread, and there the
/// stop(...) should not return until the display thread is done. 
/// THIS DOES NOT HALT EVERYTHING YET! JUST THE
/// STUFF THAT SEEMS TO CAUSE SEGFAULTS OR OTHER PROBLEMS ON EXIT.
void arMasterSlaveFramework::stop(bool blockUntilDisplayExit){

  _blockUntilDisplayExit = blockUntilDisplayExit;
  if (_vircompExecution){
    // arAppLauncher object piggy-backing on a renderer execution
    _launcher.killApp();
  }
  // to avoid an uncommon race condition setting the _exitProgram
  // flag must go within this lock
  ar_mutex_lock(&_pauseLock);
  _exitProgram = true;
  _pauseFlag = false;
  _pauseVar.signal();
  ar_mutex_unlock(&_pauseLock);
  // recall that we can be a master AND not have any distribution occuring
  if (getMaster()){
    // it IS NOT valid to combine this conditional w/ the above because
    // of the else
    if (!_standalone){
      _barrierServer->stop();
      _soundServer.stop(); 
    }
  }
  else{
    _barrierClient->stop();
  }
  while (_connectionThreadRunning
         || (_useGLUT && _displayThreadRunning && blockUntilDisplayExit)
         || (_useExternalThread && _externalThreadRunning)){
    ar_usleep(100000);
  }
  cout << _label << " remark: everything exited.\n";
  _stopped = true;
}

/// The sequence of events that should occur before the window is drawn.
/// Made available as a public method so that applications can create custom
/// event loops.
void arMasterSlaveFramework::preDraw(){
  // don't even start the function if we are, in fact, in shutdown mode
  if (_exitProgram){
    return;
  }

  // want to time this function...
  ar_timeval preDrawStart = ar_time();
  // Catch the pause/ un-pause requests.
  ar_mutex_lock(&_pauseLock);
  while (_pauseFlag){
    _pauseVar.wait(&_pauseLock);
  }
  ar_mutex_unlock(&_pauseLock);

  // the pause might have been triggered because shutdown has started
  if (_exitProgram){
    return;
  }

  // important that this occurs before the pre-exchange callback
  // the user might want to use current input data via
  // arMasterSlaveFramework::getButton() or some other method in that callback
  if (getMaster()){
    _pollInputData();
    if (_preExchange){
      _preExchange(*this);
    }
  }

  // might not be running in a distributed fashion
  if (!_standalone){
    if (!(getMaster() ? _sendData() : _getData())){
      _lastComputeTime = ar_difftime(ar_time(), preDrawStart);
      return;
    }
  }

  if (_play){
    setPlayTransform();
    _play(*this);
  }

  if (_postExchange){
    _postExchange(*this);
  }

  if (_standalone){
    // play the sounds locally
    _soundClient->_cliSync.consume();
  }

  // must let the framerate graph know about the current frametime.
  // NOTE: this is computed in _pollInputData... hmmm... doesn't make this
  // very useful for the slaves...
  arPerformanceElement* framerateElement 
    = _framerateGraph.getElement("framerate");
  framerateElement->pushNewValue(1000.0/_lastFrameTime);
  arPerformanceElement* computeElement
    = _framerateGraph.getElement("compute");
  // compute time is in microseconds and the graph element is scaled to
  // 100 ms.
  computeElement->pushNewValue(_lastComputeTime/1000.0);
  arPerformanceElement* syncElement
    = _framerateGraph.getElement("sync");
  // sync time is in microseconds and the graph element is scaled to
  // 100 ms.
  syncElement->pushNewValue(_lastSyncTime/1000.0);

  // must get performance metrics
  _lastComputeTime = ar_difftime(ar_time(), preDrawStart);
}

/// Draw the window. Note that for VR this can be somewhat complex,
/// given that we may be doing a stereo window, a window with multiple
/// viewports, and anaglyph window, etc.
void arMasterSlaveFramework::drawWindow(){
  // if the shutdown flag has been triggered, go ahead and return
  if (_exitProgram){
    return;
  }
  _graphicsWindow.draw();

  if (_standalone && _standaloneControlMode == "simulator") {
    _simulator.drawWithComposition();
  }
  if (_showPerformance){
    _framerateGraph.drawWithComposition();
  }

  // we draw the application's overlay last, if such exists. This is only
  // done on the master instance.
  if (getMaster() && _overlay){
    _overlay(*this);
  }
}

/// The sequence of events that should occur after the window is drawn,
/// but before the synchronization is called.
void arMasterSlaveFramework::postDraw(){  
  // if shutdown has been triggered, just return
  if (_exitProgram){
    return;
  }
  ar_timeval postDrawStart = ar_time();
  // sometimes, for testing of synchronization, we want to be able to throttle
  // down the framerate
  if (_framerateThrottle){
    ar_usleep(200000);
  }
  else{
    // NEVER, NEVER, NEVER, NEVER PUT A SLEEP IN THE MAIN LOOP
    // CAN'T COUNT ON THIS BEING LESS THAN 10 MS ON SOME SYSTEMS!!!!!
    //ar_usleep(2000);
  }
  // the synchronization. NOTE: we DO NOT synchronize if we are standalone.
  if (!_standalone && !_sync())
    cerr << _label << " warning: sync failed.\n";
  _lastSyncTime = ar_difftime(ar_time(), postDrawStart);
}

arMatrix4 arMasterSlaveFramework::getProjectionMatrix(float eyeSign){
  _defaultCamera.setEyeSign( eyeSign );
  return _defaultCamera.getProjectionMatrix();
}

arMatrix4 arMasterSlaveFramework::getModelviewMatrix(float eyeSign){
  _defaultCamera.setEyeSign( eyeSign );
  return _defaultCamera.getModelviewMatrix();
}

bool arMasterSlaveFramework::addTransferField(string fieldName, void* data,
                                              arDataType dataType, int size){ 
  if (_userInitCalled){
    cerr << _label << " warning: ignoring addTransferField() after init().\n";
    return false;
  }
  if (size <= 0){
    cerr << _label << " warning: ignoring addTransferField() with size "
         << size << ".\n";
    return false;
  }
  if (!data){
    cerr << _label
         << " warning: ignoring addTransferField() with NULL data ptr.\n";
    return false;
  }
  const string realName = "USER_" + fieldName;
  if (_transferTemplate.getAttributeID(realName) >= 0){
    cerr << _label
         << " warning: ignoring addTransferField() with duplicate name.\n";
    return false;
  }

  _transferTemplate.addAttribute(realName, dataType);
  const arTransferFieldDescriptor descriptor(dataType, data, size);
  _transferFieldData.insert(arTransferFieldData::value_type
			    (realName,descriptor));
  return true;
}

bool arMasterSlaveFramework::addInternalTransferField( string fieldName,
                                            arDataType dataType, int size ) {
  if (_userInitCalled){
    cerr << _label << " warning: ignoring addTransferField() after init().\n";
    return false;
  }
  if (size <= 0){
    cerr << _label << " warning: ignoring addTransferField() with size "
         << size << ".\n";
    return false;
  }
  const string realName = "USER_" + fieldName;
  if (_transferTemplate.getAttributeID(realName) >= 0){
    cerr << _label
         << " warning: ignoring addTransferField() with duplicate name.\n";
    return false;
  }
  void* data = ar_allocateBuffer( dataType, size );
  if (!data) {
    cerr << "arExperiment error: memory panic.\n";
    return false;
  }
  _transferTemplate.addAttribute(realName, dataType);
  const arTransferFieldDescriptor descriptor(dataType, data, size);
  _internalTransferFieldData.insert(arTransferFieldData::value_type
			    (realName,descriptor));

  return true;
}

bool arMasterSlaveFramework::setInternalTransferFieldSize( string fieldName,
                                          arDataType dataType, int newSize ) {
  if (!getMaster()) {
    cerr << "arMasterSlaveFramework warning: ignoring setInternalTransferFieldSize() "
         << "on slave.\n";
    return false;
  }
  const string realName = "USER_" + fieldName;
  arTransferFieldData::iterator iter;
  iter = _internalTransferFieldData.find( realName );
  if (iter == _internalTransferFieldData.end()) {
    cerr << "arMasterSlaveFramework error: internal transfer field "
         << fieldName << " not found.\n";
    return false;
  }
  arTransferFieldDescriptor& p = iter->second;
  if (dataType != p.type) {
    cerr << _label << " error: wrong type "
         << arDataTypeName(dataType) << " specified for transfer field "
         << fieldName << "; should be " << arDataTypeName(p.type)
         << endl;
    return false;
  }
  int currSize = p.size;
  if (newSize == currSize)
    return true;
  ar_deallocateBuffer( p.data );
  p.data = ar_allocateBuffer(  p.type, newSize );
  if (!p.data) {
    cerr << "arMasterSlaveFramework error: failed to resize " << fieldName << endl;
    return false;
  }
  p.size = newSize;
  return true;
}

void* arMasterSlaveFramework::getTransferField( string fieldName,
                                                arDataType dataType,
                                                int& size ) {
  const string realName = "USER_" + fieldName;
  arTransferFieldData::iterator iter;
  iter = _internalTransferFieldData.find( realName );
  if (iter == _internalTransferFieldData.end()) {
    iter = _transferFieldData.find( realName );
    if (iter == _transferFieldData.end()) {
      cerr << _label << " error: transfer field "
           << fieldName << " not found.\n";
      return (void*)0;
    }
  }
  arTransferFieldDescriptor& p = iter->second;
  if (dataType != p.type) {
    cerr << _label << " error: wrong type "
         << arDataTypeName(dataType) << " specified for transfer field "
         << fieldName << "; should be " << arDataTypeName(p.type)
         << endl;
    return (void*)0;
  }
  size = p.size;
  return p.data;
}

void arMasterSlaveFramework::setPlayTransform(){
  if (soundActive()) {
    _speakerObject.loadMatrices( _inputState->getMatrix(0) );
  }
}

void arMasterSlaveFramework::draw(){
  _graphicsDatabase.draw();
}

//************************************************************************
// Various small utility functions
//************************************************************************

void arMasterSlaveFramework::_setMaster(bool master){
  _master = master;
}

bool arMasterSlaveFramework::_sync(){
  if (_master){
    // VERY IMPORTANT THAT WE DO NOT CALL localSync() IF NO ONE IS
    // CONNECTED. THE localSync() CALL CONTAINS A THROTTLE IN CASE
    // NO ONE IS CONNECTED. IF WE CALLED IT, WE WOULD CAUSE A
    // PERFORMANCE HIT IN THE COMMON CASE OF JUST ONE APPLICATION INSTANCE.
    if (_stateServer->getNumberConnectedActive() > 0){
      _barrierServer->localSync();
    }
    return true;
  }

  return !_stateClientConnected || _barrierClient->sync();
}

//************************************************************************
// Slightly more involved, but still miscellaneous, utility functions.
// These deal with graphics.
//************************************************************************

void arMasterSlaveFramework::_createGLUTWindow(){
  // create the window
  char** argv = new char*[1];
  argv[0] = "application";
  int argc = 1;
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA |
    (_stereoMode ? GLUT_STEREO : 0));
  glutInitWindowPosition(0,0);
  char label[256]; /// \todo fixed size buffer
  ar_stringToBuffer(_label, label, sizeof(label));
  glutInitWindowPosition(_windowPositionX, _windowPositionY);
  if (_windowSizeX>0 && _windowSizeY>0){
    glutInitWindowSize(_windowSizeX, _windowSizeY);
    glutCreateWindow(label);
    glutReshapeFunc(ar_masterSlaveFrameworkReshapeFunction);
  }
  else{
    glutInitWindowSize(640,480);
    glutCreateWindow(label);
    glutReshapeFunc(ar_masterSlaveFrameworkReshapeFunction);
    glutFullScreen();
  }

  glutSetCursor(GLUT_CURSOR_NONE);
  glutDisplayFunc(ar_masterSlaveFrameworkDisplayFunction);
  glutKeyboardFunc(ar_masterSlaveFrameworkKeyboardFunction);
  glutIdleFunc(ar_masterSlaveFrameworkDisplayFunction);
  glutMouseFunc(ar_masterSlaveFrameworkButtonFunction);
  glutMotionFunc(ar_masterSlaveFrameworkMouseFunction);
  glutPassiveMotionFunc(ar_masterSlaveFrameworkMouseFunction);
}

/// Check and see if we are supposed to take a screenshot. If so, go
/// ahead and take it!
void arMasterSlaveFramework::_handleScreenshot(){
  if (_screenshotFlag){
    char numberBuffer[8];
    sprintf(numberBuffer,"%i", _whichScreenshot);
    string screenshotName = string("screenshot")+numberBuffer+string(".jpg");
    char* buf1 = new char[_screenshotWidth*_screenshotHeight*3];
    glReadBuffer(_stereoMode ? GL_FRONT_LEFT : GL_FRONT);
    glReadPixels(_screenshotStartX, _screenshotStartY,
                 _screenshotWidth, _screenshotHeight,
                 GL_RGB,GL_UNSIGNED_BYTE,buf1);
    arTexture texture;
    texture.setPixels(buf1,_screenshotWidth,_screenshotHeight);
    if (!texture.writeJPEG(screenshotName.c_str(),_dataPath)){
      cerr << "arMasterSlaveFramework remark: screenshot write failed.\n";
    }
    // the pixels are copied into arTexture's memory so we must delete them
    // here.
    delete [] buf1;
    ++_whichScreenshot;
    _screenshotFlag = false;
  }
}

//************************************************************************
// Functions for data transfer
//************************************************************************

bool arMasterSlaveFramework::_sendData(){

  if (!_master) {
    cerr << _label << " warning: ignoring slave's _sendData.\n";
    return false;
  }

  arTransferFieldData::iterator i;
  // Pack user data.
  for (i = _transferFieldData.begin(); i != _transferFieldData.end(); ++i) {
    const void* pdata = i->second.data;
    if (!pdata) {
      cerr << _label << " warning: aborting _sendData with nil data.\n";
      return false;
    }
    if (!_transferData->dataIn(i->first, pdata,
                               i->second.type, i->second.size)) {
      cerr << _label << " warning: problem in _sendData.\n";
    }
  }
  // Pack internally-stored user data.
  for (i = _internalTransferFieldData.begin();
       i != _internalTransferFieldData.end(); ++i) {
    const void* pdata = i->second.data;
    if (!pdata) {
      cerr << _label << " warning: aborting _sendData with nil data.\n";
      return false;
    }
    if (!_transferData->dataIn(i->first, pdata,
                               i->second.type, i->second.size)) {
      cerr << _label << " warning: problem in _sendData.\n";
    }
  }
  // Pack the arMasterSlaveDataRouter's managed data
  _dataRouter.internalDumpState();
  int dataRouterDumpSize;
  char* dataRouterInfo = _dataRouter.getTransferBuffer(dataRouterDumpSize);
  _transferData->dataIn("szg_data_router", dataRouterInfo,
                        AR_CHAR, dataRouterDumpSize);
  // Pack other data.
  _packInputData();

  // only activates it if necessary (and possible)
  ar_activateWildcatFramelock();

  // Activate pending connections.
  if (_barrierServer->checkWaitingSockets()){
    _barrierServer->activatePassiveSockets(_stateServer);
  }

  // Send data, if there are any receivers.
  if (_stateServer->getNumberConnectedActive() > 0 &&
      !_stateServer->sendData(_transferData)){
    cout << _label << " remark: state server failed to send data.\n";
    return false;
  }
  return true;
}

bool arMasterSlaveFramework::_getData(){

  if (_master) {
    cerr << _label << " warning: ignoring master's _getData.\n";
    return false;
  }

  if (!_stateClientConnected){
    // Not connected to master, so there's no data to get.
    return true;
  }

  // We are connected, so request activation from the master.
  // This call blocks until an activation occurs.
  if (!_barrierClient->checkActivation()){
    _barrierClient->requestActivation();
  }

  // only activates it if necessary (and possible)
  ar_activateWildcatFramelock();

  // Read data, since we will be receiving data from the master.
  if (!_stateClient.getData(_inBuffer,_inBufferSize)){
    cerr << _label << " warning: state client failed to receive data.\n";
    _stateClientConnected = false;
    // we need to disable framelock now, so that it can be appropriately 
    // re-enabled upon reconnection to the master
    // wildcat framelock is only deactivated if this makes sense
    ar_deactivateWildcatFramelock();
    return false;
  }

  _transferData->unpack(_inBuffer);
  arTransferFieldData::iterator i;
  // unpack the user data
  for (i = _transferFieldData.begin(); i != _transferFieldData.end(); ++i) {
    void* pdata = i->second.data;
    if (pdata){
      _transferData->dataOut(i->first, pdata, i->second.type, i->second.size);
    }
  }
  // unpack internally-stored user data
  for (i = _internalTransferFieldData.begin();
       i != _internalTransferFieldData.end();
       ++i) {
    int currSize = i->second.size;
    int numItems = _transferData->getDataDimension( i->first );
    if (numItems == 0) {
      ar_deallocateBuffer( i->second.data );
      i->second.data = NULL;
    }
    if (numItems != currSize) {
      ar_deallocateBuffer( i->second.data );
      i->second.data = ar_allocateBuffer( i->second.type, numItems );
      if (!i->second.data) {
        cerr << "arMasterSlaveFramework error: failed to allocate memory "
             << "for transfer field " <<  i->first << endl;
        return false;
      }
      i->second.size = numItems;
    }
    if (i->second.data){
      _transferData->dataOut(i->first, i->second.data, i->second.type, i->second.size);
    }
  }
  // unpack the data intended for the data router
  _dataRouter.setRemoteStreamConfig(_stateClient.getRemoteStreamConfig());
  _dataRouter.routeMessages(
                 (char*)_transferData->getDataPtr("szg_data_router", AR_CHAR),
                 _transferData->getDataDimension("szg_data_router"));
  // unpack the other data
  _unpackInputData();
  return true;
}

void arMasterSlaveFramework::_eventCallback( arInputEvent& event ) {
  ar_mutex_lock( &_eventLock );
  _eventQueue.appendEvent( event );
  ar_mutex_unlock( &_eventLock );
}

void arMasterSlaveFramework::_pollInputData(){

  // Should ensure that start() has been called already.

  if (!_master) {
    cerr << _label << " error: slave tried to _pollInputData.\n";
    return;
  }

  if (!_inputActive)
    return;

  if (_firstTimePoll){
    _startTime = ar_time();
    _time = 0;
    _firstTimePoll = false;
  }
  else{
    double temp = ar_difftime(ar_time(),_startTime)/1000.0;
    _lastFrameTime = temp - _time;
    // Set a lower bound for low-resolution system clocks.
    if (_lastFrameTime < 0.005)
      _lastFrameTime = 0.005;
    _time = temp;
  }

  // If we are in standalone mode, get the input events now.
  // AARGH!!!! This could be done more generally...
  if (_standalone && _standaloneControlMode == "simulator"){
    _simulator.advance();
  }

  _inputState->updateLastButtons();
  _inputDevice->processBufferedEvents();
  
  _head.setMatrix( getMatrix(AR_VR_HEAD_MATRIX_ID,false) );
}

void arMasterSlaveFramework::_packInputData(){
  const arMatrix4 navMatrix(ar_getNavMatrix());
  if (!_transferData->dataIn("time",&_time,AR_DOUBLE,1) ||
      !_transferData->dataIn("lastFrameTime",&_lastFrameTime,AR_DOUBLE,1) ||
      !_transferData->dataIn("navMatrix",navMatrix.v,AR_FLOAT,16) ||
      !_transferData->dataIn("eye_spacing",&_head._eyeSpacing,AR_FLOAT,1) ||
      !_transferData->dataIn("mid_eye_offset",_head._midEyeOffset.v,AR_FLOAT,3) ||
      !_transferData->dataIn("eye_direction",_head._eyeDirection.v,AR_FLOAT,3) ||
      !_transferData->dataIn("head_matrix",_head._matrix.v,AR_FLOAT,16) ||
      !_transferData->dataIn("near_clip",&_head._nearClip,AR_FLOAT,1) ||
      !_transferData->dataIn("far_clip",&_head._farClip,AR_FLOAT,1) ||
      !_transferData->dataIn("unit_conversion",&_head._unitConversion,AR_FLOAT,1) ||
      !_transferData->dataIn("fixed_head_mode",&_head._fixedHeadMode,AR_INT,1)) {
    cerr << _label << " warning: problem in _packInputData.\n";
  }
  if (!ar_saveInputStateToStructuredData( _inputState, _transferData )) {
    cerr << _label << " warning: failed to pack input state data.\n";
  }
}

void arMasterSlaveFramework::_unpackInputData(){
  _transferData->dataOut("time",&_time,AR_DOUBLE,1);
  _transferData->dataOut("lastFrameTime",&_lastFrameTime,AR_DOUBLE,1);
  _transferData->dataOut("eye_spacing",&_head._eyeSpacing,AR_FLOAT,1);
  _transferData->dataOut("mid_eye_offset",_head._midEyeOffset.v,AR_FLOAT,3);
  _transferData->dataOut("eye_direction",_head._eyeDirection.v,AR_FLOAT,3);
  _transferData->dataOut("head_matrix",_head._matrix.v,AR_FLOAT,16);
  _transferData->dataOut("near_clip",&_head._nearClip,AR_FLOAT,1);
  _transferData->dataOut("far_clip",&_head._farClip,AR_FLOAT,1);
  _transferData->dataOut("unit_conversion",&_head._unitConversion,AR_FLOAT,1);
  _transferData->dataOut("fixed_head_mode",&_head._fixedHeadMode,AR_INT,1);
  arMatrix4 navMatrix;
  _transferData->dataOut("navMatrix",navMatrix.v,AR_FLOAT,16);
  ar_setNavMatrix( navMatrix );
  if (!ar_setInputStateFromStructuredData( _inputState, _transferData )) {
    cerr << _label << " warning: failed to unpack input state data.\n";
  }
}

//************************************************************************
// functions pertaining to starting the application
//************************************************************************

/// Determines whether or not we are the master instance
bool arMasterSlaveFramework::_determineMaster
  (stringstream& initResponse){
  // each master/slave application has it's own unique service,
  // since each has its own unique protocol
  _serviceName =
    _SZGClient.createComplexServiceName(string("SZG_MASTER_")+_label);
  _serviceNameBarrier =
    _SZGClient.createComplexServiceName(
      string("SZG_MASTER_")+_label+"_BARRIER");
  // if running on a virtual computer, use that info to determine if we are
  // the master or not. otherwise, it's first come-first served in terms of
  // who the master is. NOTE: we allow the virtual computer to not define
  // a master... in which case it is more or less random who gets to be the
  // master.
  if (_SZGClient.getVirtualComputer() != "NULL"
      && _launcher.getMasterName() != "NULL"){
    if (_launcher.isMaster() || _SZGClient.getMode("default") == "master"){
      _setMaster(true);
      if (!_SZGClient.registerService(_serviceName,"graphics",1,_masterPort)){
        initResponse << _label 
                     << " error: component failed to be master.\n";
        return false;
      }
    }
    else{
      _setMaster(false);
    }
  }
  else{
    // we'll be the master if we are the first to register the service
    _setMaster(_SZGClient.registerService(_serviceName,"graphics",1,
                                          _masterPort));
  }

  return true;
}

/// Sometimes we may want to run the program by itself, without connecting
/// to the distributed system.
bool arMasterSlaveFramework::_initStandaloneObjects(){
  // Create the input node. NOTE: there are, so far, two different ways
  // to control a standalone master/slave application. An embedded
  // wandsimserver and a joystick interface
  _inputActive = true;
  _inputDevice = new arInputNode(true);
  _inputState = &_inputDevice->_inputState;
  // Which mode are we using? The simulator mode is the default.
  _standaloneControlMode = _SZGClient.getAttribute("SZG_DEMO", "control_mode",
                                                   "|simulator|joystick|");
  if (_standaloneControlMode == "simulator"){ 
    _simulator.registerInputNode(_inputDevice);
  }
  else{
    // the joystick is the only other option so far
    arSharedLib* joystickObject = new arSharedLib();
    string sharedLibLoadPath = _SZGClient.getAttribute("SZG_EXEC","path");
    string pforthProgramName = _SZGClient.getAttribute("SZG_PFORTH",
                                                       "program_names");
    string error;
    if (!joystickObject->createFactory("arJoystickDriver", sharedLibLoadPath,
                                       "arInputSource", error)){
      cout << error;
      return false;
    }
    arInputSource* driver = (arInputSource*) joystickObject->createObject();
    // The input node is not responsible for clean-up
    _inputDevice->addInputSource(driver, false);
    if (pforthProgramName == "NULL"){
      cout << "arMasterSlaveFramework remark: no pforth program for "
	   << "standalone joystick.\n";
    }
    else{
      string pforthProgram = _SZGClient.getGlobalAttribute(pforthProgramName);
      if (pforthProgram == "NULL"){
        cout << "arMasterSlaveFramework remark: no pforth program exists for "
	     << "name = " << pforthProgramName << "\n";
      }
      else{
        arPForthFilter* filter = new arPForthFilter();
        ar_PForthSetSZGClient( &_SZGClient );
      
        if (!filter->configure( pforthProgram )){
	  cout << "arMasterSlaveFramework remark: failed to configure pforth\n"
	       << "filter with program =\n "
	       << pforthProgram << "\n";
          return false;
        }
        // The input node is not responsible for clean-up
        _inputDevice->addFilter(filter, false);
      }
    }
  }
  // NOTE: this will probably fail under the current hacked regime...
  // BUT... if this doesn't occur, then the various filters and such
  // complain constantly.
  _inputDevice->init(_SZGClient);
  _inputDevice->start();

  // the below sound initialization must occur here (and not in
  // startStandaloneObjects) since this occurs before the user-defined init.
  // Probably the key thing is that the _soundClient needs to hit its
  // registerLocalConnection method before any dsLoop, etc. calls are made.
  arSpeakerObject* speakerObject = new arSpeakerObject();

  // NOTE: using "new" to create the _soundClient since, otherwise, the 
  // constructor creates a conflict with programs that want to use fmod 
  // themselves. (of course, the conflict is still present in 
  // standalone mode...)
  _soundClient = new arSoundClient();
  _soundClient->setSpeakerObject(speakerObject);
  _soundClient->configure(&_SZGClient);
  _soundClient->_cliSync.registerLocalConnection(&_soundServer._syncServer);
  
  // sound is inactive for the time being
  _soundActive = false;

  // this always succeeds
  return true;
}

/// This is lousy factoring. I think it would be a good idea to eventually
/// fold init and start in together.
bool arMasterSlaveFramework::_startStandaloneObjects(){
  _soundServer.start();
  _soundActive = true;
  return true;
}

bool arMasterSlaveFramework::_initMasterObjects(stringstream& initResponse){
  // attempt to initialize the barrier server
  _barrierServer = new arBarrierServer();
  if (!_barrierServer) {
    initResponse << _label 
                  << "error: master failed to construct barrier server.\n";
    return false;
  }
  _barrierServer->setServiceName(_serviceNameBarrier);
  _barrierServer->setChannel("graphics");
  if (!_barrierServer->init(_SZGClient)){
    initResponse << _label << " error: failed to initialize barrier "
	         << "server.\n";
  }
  _barrierServer->registerLocal();
  _barrierServer->setSignalObject(&_swapSignal);

  // attempt to initialize the input device
  _inputActive = false;
  _inputDevice = new arInputNode(true);
  if (!_inputDevice){
    initResponse << _label 
                 << " error: master failed to construct input device.\n";
    return false;
  }
  _inputState = &_inputDevice->_inputState;
  _inputDevice->addInputSource(&_netInputSource,false);
  _netInputSource.setSlot(0);
  if (!_inputDevice->init(_SZGClient)){
    initResponse << _label << " error: master failed to init input device.\n";
    delete _inputDevice;
    _inputDevice = NULL;
    return false;
  }

  // attempt to initialize the sound device
  _soundActive = false;
  if (!_soundServer.init(_SZGClient)){
    initResponse << _label << " error: sound server failed to init.\n"
                 << "  (Is another application running?)\n";
    return false;
  }
  
  // we've succeeded in initializing the various objects
  initResponse << _label << " remark: master's objects initialized.\n";
  return true;
}

/// Starts the objects needed by the master.
bool arMasterSlaveFramework::_startMasterObjects(stringstream& startResponse){
  // go ahead and start the master's service 
  _stateServer = new arDataServer(1000);
  if (!_stateServer) {
    startResponse << _label 
                  << " error: master failed to construct state server.\n";
    return false;
  }
  _stateServer->smallPacketOptimize(true);
  _stateServer->setInterface("INADDR_ANY");
  // the _stateServer's initial ports were set in _determineMaster
  _stateServer->setPort(_masterPort[0]);
  // TODO TODO TODO TODO TODO TODO TODO
  // there is a very annoying duplication of connection code in many places!
  // (i.e. of this try-to-connect-to-brokered ports 10 times)
  int tries = 0;
  bool success = false;
  while (tries<10 && !success){
    if (!_stateServer->beginListening(&_transferLanguage)){
      startResponse << _label << " warning: failed to listen on port "
		    << _masterPort[0] << ".\n";
      _SZGClient.requestNewPorts(_serviceName,"graphics",1,_masterPort);
      _stateServer->setPort(_masterPort[0]);
      tries++;
    }
    else{
      success = true;
    }
  }
  if (success){
    if (!_SZGClient.confirmPorts(_serviceName,"graphics",1,_masterPort)){
      startResponse << _label << " error: failed to confirm brokered "
	            << "port..\n";
      return false;
    }
  }
  else{
    // we failed to bind to the ports
    return false;
  }

  // start the barrier server
  if (!_barrierServer->start()) {
    startResponse << _label << " error: failed to start barrier server.\n";
    return false;
  }

  // start the input device client 
  if (!_inputDevice->start()) {
    startResponse << _label << " error: failed to start input device.\n";
    delete _inputDevice;
    _inputDevice = NULL;
    return false;
  }
  _inputActive = true;

  // start the sound server
  if (!_soundServer.start()){
    startResponse << _label << " error: sound server failed to listen.\n"
                  << "  (Is another application running?)\n";
    return false;
  }
  _soundActive = true;

  startResponse << _label << " remark: master's objects started.\n";
  return true;
}

bool arMasterSlaveFramework::_initSlaveObjects(stringstream& initResponse){
  // slave instead of master
  // in this case, the component must know which networks on which it
  // should attempt to connect
  _networks = _SZGClient.getNetworks("graphics");
  _inBufferSize = 1000;
  _inBuffer = new ARchar[_inBufferSize];
  _barrierClient = new arBarrierClient;
  if (!_barrierClient) {
    initResponse << _label 
                  << "error: slave failed to construct barrier client.\n";
    return false;
  }
  _barrierClient->setNetworks(_networks);
  _barrierClient->setServiceName(_serviceNameBarrier);
  if (!_barrierClient->init(_SZGClient)){
    initResponse << _label << " error: barrier client failed to start.\n";
    return false;
  }
  _inputState = new arInputState();
  if (!_inputState) {
    initResponse << _label 
                 << "error: slave failed to construct input state.\n";
    return false;
  }
  // of course, the state client should not have weird delays on small
  // packets, as would be the case in the Win32 TCP/IP stack without
  // doing the following
  _stateClient.smallPacketOptimize(true);
  
  // we've succeeded in the init
  initResponse << _label << " remark: the initialization of the slave "
	       << "objects succeeded.\n";
  return true;
}

/// Starts the objects needed by the slaves
bool arMasterSlaveFramework::_startSlaveObjects(stringstream& startResponse){
  // the barrier client is the only object to start
  if (!_barrierClient->start()){
    startResponse << _label << " error: barrier client failed to start.\n";
    return false;
  }
  startResponse << _label << " remark: slave objects successfully started.\n";
  return true;
}

bool arMasterSlaveFramework::_startObjects(){
  // THIS IS GUARANTEED TO BEGIN AFTER THE USER-DEFINED INIT!

  // want to be able to write to the start stream
  stringstream& startResponse = _SZGClient.startResponse();

  // Create the language.
  _transferLanguage.add(&_transferTemplate);
  _transferData = new arStructuredData(&_transferTemplate);
  if (!_transferData) {
    startResponse << _label 
      << " error: master failed to construct _transferData.\n";
    return false;
  }
  // Go ahead and start the arMasterSlaveDataRouter (this just creates the
  // language to be used by that device... using the registered
  // arFrameworkObjects). Always succeeds as of now.
  (void) _dataRouter.start();

  // By now, we know whether or not we are the master
  if (_master){
    if (!_startMasterObjects(startResponse)){
      return false;
    }
  }
  else{
    if (!_startSlaveObjects(startResponse)){
      return false;
    }
  }

  // Both master and slave need a connection thread
  if (!_connectionThread.beginThread(ar_masterSlaveFrameworkConnectionTask, 
                                     this)) {
    startResponse << _label << " error: failed to start connection thread.\n";
    return false;
  }

  _graphicsDatabase.loadAlphabet(_textPath);
  _graphicsDatabase.setTexturePath(_texturePath);
  return true;
}

/// What we need to do to start in standalone mode... this should be factored
/// BACK into my normal stuff. HACK HACK HACK HACK HACK HACK HACK HACK HACK
bool arMasterSlaveFramework::_startStandalone(bool useGLUT){
  _useGLUT = useGLUT;
  _graphicsWindow.setDrawCallback( new arMasterSlaveRenderCallback( *this ) );
  if (_useGLUT){
    _createGLUTWindow();
  }
  __globalFramework = this;
  if (_init && !_init(*this, _SZGClient)){
    cerr << _label << " error: failed to called user-defined init.\n";
    return false;
  }
  // this is from _startObjects... a CUT_AND_PASTE!!
  _graphicsDatabase.loadAlphabet(_textPath);
  _graphicsDatabase.setTexturePath(_texturePath);  
  _startStandaloneObjects();
  _userInitCalled = true;
  if (_useGLUT){
    _displayThreadRunning = true;
    glutMainLoop(); // never returns
  }
  return true;
}

/// Functionality common to start() and startWithoutGLUT().
bool arMasterSlaveFramework::_start(bool useGLUT){
  // A HACK!
  if (_standalone){
    return _startStandalone(useGLUT);
  }

  _useGLUT = useGLUT;
  if (!_parametersLoaded){
    _SZGClient.initResponse() << _label 
      << " error: start() method called before init() method.\n";
    _SZGClient.sendInitResponse(false);
    return false;
  }

  // accumulate the start response messages
  stringstream& startResponse = _SZGClient.startResponse();

  // make sure we get the screen resource
  // this lock should only be grabbed AFTER an application launching.
  // i.e. DO NOT do this in init(), which is called even on the trigger
  // instance... but do it here (which is only reached on render instances)
  string screenLock = _SZGClient.getComputerName()+"/"
                      +_SZGClient.getMode("graphics");
  int graphicsID;
  if (!_SZGClient.getLock(screenLock, graphicsID)){
    startResponse << _label 
         << " error: failed to get screen resource held by component "
	 << graphicsID << ".\n(Kill that component to proceed.)\n";
    _SZGClient.sendStartResponse(false);
    return false;
  }
  
  _graphicsWindow.setDrawCallback( new arMasterSlaveRenderCallback( *this ) );
  
  if (_useGLUT){
    _createGLUTWindow();
  }
  // Unfortuantely, GLUT does not have facilities for passing a pointer
  // so, we are forced to use globals. Not really too limiting since
  // GLUT already assumes only one GLUT will be running per application.
  // NOTE: this should be done regardless of whether we are using GLUT.
  __globalFramework = this;

  // do the user-defined init
  // NOTE: in some cases, the user-defined init must occur AFTER
  // the window has been created.
  // NOTE: so that _init can know if this instance is the master or
  // a slave, it is important to call this AFTER _startDetermineMaster(...)
  if (_init && !_init(*this, _SZGClient)){
    startResponse << "Programmer-defined init of component failed.\n";
    _SZGClient.sendStartResponse(false);
    return false;
  }

  // set-up the various objects and start services
  if (!_startObjects()){
    startResponse << "The various objects failed to successfully start.\n";
    _SZGClient.sendStartResponse(false);
    return false;
  }

  _userInitCalled = true;

  // the start succeeded
  _SZGClient.sendStartResponse(true);

  // HMMM... This is a somewhat problematic place to put the function
  // that tries to find the wildcat framelock. It seems fairly likely
  // that this must occur AFTER the graphics window has been mapped!
  // BUT... this call only occurs after the mapping of the graphics window
  // when we are using GLUT to manage the window! DOH!!!
  ar_findWildcatFramelock();

  if (_useGLUT){
    _displayThreadRunning = true;
    glutMainLoop(); // never returns
  }
  return true;
}

//**************************************************************************
// Other system-level functions
//**************************************************************************

bool arMasterSlaveFramework::_loadParameters(){

  // The graphics configuration is in a bit of a state of transition.
  // Some of the global window values will still be gotten from the
  // SZG_SCREENx data structure (like window size and view mode)... but
  // this data sctructure is moving towards defining the camera for a 
  // viewport.
  cout << "arMasterSlaveFramework remark: reloading parameters.\n";

  // some things just depend on the SZG_RENDER
  _texturePath = _SZGClient.getAttribute("SZG_RENDER","texture_path");
  string received(_SZGClient.getAttribute("SZG_RENDER","text_path"));
  ar_stringToBuffer(ar_pathAddSlash(received), _textPath, sizeof(_textPath));

  // We set a few window-wide attributes based on screen name. THIS IS AN
  // UGLY HACK!!!! (stereo, window size, window position, wildcat framelock)
  const string screenName(_SZGClient.getMode("graphics"));

  // Which screen should we use to define the view?
  _defaultScreen.configure( screenName, _SZGClient );
  _defaultCamera.setScreen( &_defaultScreen );
  // It could be that we've already set a custom camera
  // (as in the non-euclidean math visualizations)
  if (!_graphicsWindow.getCamera()){
    _graphicsWindow.setCamera( &_defaultCamera );
  }
  else{
    cout << "arMasterSlaveFramework remark: using a custom camera.\n";
  }
  _graphicsWindow.configure(_SZGClient);

  _stereoMode = _graphicsWindow.getUseOGLStereo();
  // NOTE: quad-buffered stereo is NOT compatible with custom viewports
//  _stereoMode = _SZGClient.getAttribute(screenName, "stereo",
//    "|false|true|") == "true";
//  _graphicsWindow.useOGLStereo( _stereoMode );
  
  float temp[2];
  if (_SZGClient.getAttributeFloats(screenName, "size", temp, 2)){
    _windowSizeX = int(temp[0]);
    _windowSizeY = int(temp[1]);
  }
  else {
    // default for when there's no configuration
    _windowSizeX = 640;
    _windowSizeY = 480;
  }

  if (_SZGClient.getAttributeFloats(screenName, "position", temp, 2)){
    _windowPositionX = int(temp[0]);
    _windowPositionY = int(temp[1]);
  }
  else{
    _windowPositionX = 0;
    _windowPositionY = 0;
  }

  if (getMaster()) {
    _head.configure(_SZGClient);
  }

  ar_useWildcatFramelock(_SZGClient.getAttribute(screenName, 
    "wildcat_framelock", "|false|true|") == "true");

  // Don't think the speaker object configuration actually does anything
  // yet!!!!
  if (!_speakerObject.configure(&_SZGClient)) {
    return false;
  }

  // Load the other parameters.
  _loadNavParameters();

  _dataPath = _SZGClient.getAttribute("SZG_DATA","path");
  if (_dataPath == "NULL"){
    _dataPath = string("");
  }
  return true;
}

void arMasterSlaveFramework::_messageTask(){
  // might be a good idea to shut this down cleanly... BUT currently
  // there's no way to shut the arSZGClient down cleanly.
  string messageType, messageBody;
  while (!_exitProgram){
    // NOTE: it is possible for receiveMessage to fail, precisely in the
    // case that the szgserver has *hard-shutdown* our client object.
    if (!_SZGClient.receiveMessage(&messageType,&messageBody)){
      // NOTE: the shutdown procedure is the same as for the "quit" message.
      stop(true);
      if (!_useExternalThread){
        exit(0);
      }
      // We DO NOT want to hit this again! (since things are DISCONNECTED)
      break;
    }
    
    if (messageType=="quit"){
      // at this point, we do our best to bring everything to an orderly halt.
      // this kind of care keeps some programs from seg-faulting on exit,
      // which is bad news on Win32 since it brings up a dialog box that
      // must be clicked!
      // NOTE: we block here until the display thread is finished.'
      stop(true);
      if (!_useExternalThread){
        exit(0);
      }
    }
    else if (messageType== "performance"){
      if (messageBody == "on"){
	_showPerformance = true;
      }
      else{
        _showPerformance = false;
      }
    }
    else if (messageType=="reload"){
      (void)_loadParameters();
    }
    else if (messageType== "user"){
      if (_userMessageCallback){
        _userMessageCallback(*this, messageBody);
      }
    }
    else if (messageType=="color"){
      if (messageBody=="NULL" || messageBody=="off"){
        _defaultColor = arVector3(-1,-1,-1);
      }
      else{
	float tmp[3];
	ar_parseFloatString(messageBody, tmp, 3);
	/// \todo error checking
        memcpy(_defaultColor.v, tmp, 3*sizeof(AR_FLOAT));
      }
    }

    else if (messageType=="screenshot"){
      if (_dataPath == "NULL"){
	cerr << _label << " warning: screenshot failed, no SZG_DATA/path.\n";
      }
      else{
        _screenshotFlag = true;
        if (messageBody != "NULL"){
          int tmp[4];
          ar_parseIntString(messageBody, tmp, 4);
	  /// \todo error checking
          _screenshotStartX = tmp[0];
	  _screenshotStartY = tmp[1];
	  _screenshotWidth  = tmp[2];
	  _screenshotHeight = tmp[3];
	}
      }
    }

    else if (messageType=="look"){
      if (messageBody=="NULL"){
	// the default camera
        _graphicsWindow.setCamera( &_defaultCamera );
      }
      else{
	// Activate a new camera, which may be better for screenshots.
        // tmp = 6 glFrustum params followed by 9 gluLookat params
        float tmp[15];
        int numberArgs = ar_parseFloatString(messageBody, tmp, 15);
		// The Irix compiler does not like things like foo(&constructor(...)),
		// consequently, use temp
		arPerspectiveCamera temp( tmp, tmp+6 );
        _graphicsWindow.setCamera( &temp );
      }
    }
    
    else if (messageType=="demo") {
      bool onoff = (messageBody=="on")?(true):(false);
      setFixedHeadMode(onoff);
    }
    
    else if (messageType=="viewmode") {
      setViewMode( messageBody );
    }
    //*********************************************************
    // There's quite a bit of copy-pasting between the
    // messages accepted by szgrender and here... how can we
    // reuse messaging functionality?????
    //*********************************************************
    if (messageType=="delay"){
      if (messageBody=="on"){
	_framerateThrottle = true;
      }
      else{
	_framerateThrottle = false;
      }
    }
    else if (messageType=="pause"){
      if (messageBody == "on"){
        ar_mutex_lock(&_pauseLock);
        // do not pause if we are exitting
        if (!_exitProgram){
	  _pauseFlag = true;
	}
	ar_mutex_unlock(&_pauseLock);
      }
      else if (messageBody == "off"){
        ar_mutex_lock(&_pauseLock);
        _pauseFlag = false;
        _pauseVar.signal();
	ar_mutex_unlock(&_pauseLock);
      }
      else
        cerr << _label << " warning: ignoring unexpected pause arg \""
	     << messageBody << "\".\n";
    }

  }
}

void arMasterSlaveFramework::_connectionTask(){
  // shutdown depends on the value of _connectionThreadRunning
  _connectionThreadRunning = true;
  if (_master){
    // THE MASTER'S METHOD OF HANDLING CONNECTIONS
    while (!_exitProgram){
      // TODO TODO TODO TODO TODO TODO
      // As a hack, since non-blocking connection accept has yet to be
      // implemented, we have to pretend the connection thread isn't running
      // during the blocking call
      _connectionThreadRunning = false;
      arSocket* theSocket = _stateServer->acceptConnectionNoSend();
      _connectionThreadRunning = true;
      if (_exitProgram){
        break;
      }
      if (!theSocket || _stateServer->getNumberConnected() <= 0){
        // something bad happened.  Don't keep trying infinitely.
        _exitProgram = true;
	break;
      }
      cout << _label << " remark: slave connected to master";
      const int num = _stateServer->getNumberConnected();
      if (num > 1)
	cout << " (" << num << " in all)";
      cout << ".\n";
    }
  }
  else{
    // THE SLAVE'S METHOD OF HANDLING CONNECTIONS
    while (!_exitProgram){
      // make sure barrier is connected first
      while (!_barrierClient->checkConnection() && !_exitProgram){
        ar_usleep(100000);
      }
      if (_exitProgram)
        break;

      // TODO TODO TODO TODO TODO TODO
      // As a hack, since discoverService is currently a blocking call
      // and the arSZGClient does not have a shutdown mechanism in place
      _connectionThreadRunning = false;
      const arPhleetAddress result =
        _SZGClient.discoverService(_serviceName, _networks, true);
      _connectionThreadRunning = true;
      if (_exitProgram)
        break;

      cout << _label << " remark: connecting to "
	   << result.address << ":" << result.portIDs[0] << "\n";
      if (!result.valid ||
          !_stateClient.dialUpFallThrough(result.address, result.portIDs[0])){
	cout << _label << " warning: brokering process failed. Will retry.\n";
	continue;
      }
      // Bond the appropriate data channel to this sync channel.
      _barrierClient->setBondedSocketID(_stateClient.getSocketIDRemote());
      _stateClientConnected = true;
      cout << _label << " remark: slave connected to master.\n";
      while (_stateClientConnected && !_exitProgram){
        ar_usleep(300000);
      }
      if (_exitProgram)
        break;
      cout << _label << " remark: slave disconnected.";
      _stateClient.closeConnection();
    }
  }

  _connectionThreadRunning = false;
}

//**************************************************************************
// Functions directly pertaining to drawing
//**************************************************************************

void arMasterSlaveFramework::_draw(){
  if (!_drawCallback) {
    cerr << _label << " warning: forgot to setDrawCallback().\n";
    return;
  }
  if (_defaultColor[0] == -1){
    _drawCallback(*this);
  }
  else{
    // we just want a colored background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1,1,-1,1,0,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glColor3fv(&_defaultColor[0]);
    glBegin(GL_QUADS);
    glVertex3f(1,1,-0.5);
    glVertex3f(-1,1,-0.5);
    glVertex3f(-1,-1,-0.5);
    glVertex3f(1,-1,-0.5);
    glEnd(); 
    glPopAttrib();
  }
}

/// Used by the GLUT display callback
void arMasterSlaveFramework::_display(){
  // handle the stuff that occurs before drawing
  preDraw();
  
  // draw the window
  drawWindow();

  // it seems like glFlush/glFinish are a little bit unreliable... not
  // every vendor has done a good job of implementing these. 
  // Consequently, we do a small pixel read to force drawing to complete
  char buffer[32];
  glReadPixels(0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,buffer);

  // occurs after drawing and right before the buffer swap
  postDraw();
  
  if (_internalBufferSwap){
    // sometimes, it might be most convenient to let an external
    // library do the buffer swap, even though this degrades synchronization
    glutSwapBuffers();
  }

  // if we are supposed to take a screenshot, go ahead and do so.
  _handleScreenshot();

  // if we are in shutdown mode, we want to stop everything and
  // then go away. NOTE: there are special problems since _display()
  // and the keyboard function where the ESC press is caught are in the
  // same thread
  if (_exitProgram){
    // guaranteed to get here--user-defined cleanup will be called
    // at the right time
    if (_cleanup){
      _cleanup(*this);
    }

    // wildcat framelock is only deactivated if this makes sense...
    // and if it makes sense, it is important to do so before exiting.
    // also important that we do this in the display thread
    ar_deactivateWildcatFramelock();

    cout << _label << " remark: GLUT display thread done.\n";

    _displayThreadRunning = false;
    // if stop(...) is called from the GLUT keyboard function, we
    // want to exit here. (_blockUntilDisplayExit will be false)
    // Otherwise, we want to wait here for the exit to occur in the
    // message thread
    while (_blockUntilDisplayExit){
      ar_usleep(100000);
    }
    exit(0);
  }
}

// OUCH! This might or might not be safe under the current regime!
bool arMasterSlaveFramework::setViewMode( const string& viewMode ) {
//  cerr << _label << " remark: setting view mode to "
//       << viewMode << endl;
  return _graphicsWindow.setViewMode( viewMode );
}
