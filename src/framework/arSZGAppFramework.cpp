//@+leo-ver=4-thin
//@+node:jimc.20100409112755.285:@thin framework\arSZGAppFramework.cpp
//@@language c++
//@@tabwidth -2
//@+others
//@+node:jimc.20100409112755.286:arSZGAppFramework::arSZGAppFramework
//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSZGAppFramework.h"
#include "arSoundAPI.h"
#include "arLogStream.h"
#include "arInputSimulatorFactory.h"

// Defaults include an average eye spacing of 6 cm.
arSZGAppFramework::arSZGAppFramework() :
  _inputNode(0),
  _inputState(0),
  _label("arSZGAppFramework"),
  _launcher(_label.c_str()), // pointless, since derived class sets _label after constructor?
  _vircompExecution(false),
  _standalone(false),
  _standaloneControlMode("simulator"),
  _simPtr(&_simulator),
  _showSimulator(true),
  _showPerformance( false ),
#if defined( AR_USE_MINGW ) || defined( AR_LINKING_DYNAMIC) || !defined( AR_USE_WIN_32 )
  _inputFactory(),
#endif
  _callbackFilter(this),
  _defaultUserFilter(this),
  _userEventFilter(NULL),
  _inOnInputEvent(false),
  _dataBundlePathSet(false),
  _speechNodeID(-1),
  _useNavInputMatrix(false),
  _navInputMatrixIndex(2),
  _unitConvertNavInputMatrix(true),
  _wm(NULL),
  _guiXMLParser(NULL),
  _initCalled(false),
  _startCalled(false),
  _parametersLoaded(false),
  _useExternalThread(false),
  _externalThreadRunning(false),
  _blockUntilDisplayExit(false),
  _exitProgram(false),
  _displayThreadRunning(false),
  _stopped(false) {
  setEventQueueCallback(NULL);
}
//@-node:jimc.20100409112755.286:arSZGAppFramework::arSZGAppFramework
//@+node:jimc.20100409112755.287:arSZGAppFramework

arSZGAppFramework::~arSZGAppFramework() {
  if (_inputNode != 0)
    delete _inputNode;
}
//@-node:jimc.20100409112755.287:arSZGAppFramework
//@+node:jimc.20100409112755.288:arSZGAppFramework::speak

void arSZGAppFramework::speak( const std::string& message ) {
  if (_speechNodeID == -1) {
    _speechNodeID = dsSpeak( "framework_messages", "root", message );
  } else {
    dsSpeak( _speechNodeID, message );
  }
}
//@-node:jimc.20100409112755.288:arSZGAppFramework::speak
//@+node:jimc.20100409112755.289:arSZGAppFramework::autoDataBundlePath

void arSZGAppFramework::autoDataBundlePath() {
  if (_dataBundlePathSet) {
    return;
  }
  if (_label.length() > 3 && _label.substr(_label.length()-3, 3) == ".py") {
    arPathString workDir;
    ar_getWorkingDirectory( workDir );
    if (workDir.empty()) {
      ar_log_error() << "autoDataBundlePath(): empty working directory?\n";
      cout << "autoDataBundlePath(): empty working directory?\n";
      return;
    }
    string dirName( workDir[workDir.size()-1] );
    setDataBundlePath( "SZG_PYTHON", dirName );
  } else {
    setDataBundlePath( "SZG_EXEC", _label );
  }
}
//@-node:jimc.20100409112755.289:arSZGAppFramework::autoDataBundlePath
//@+node:jimc.20100409112755.290:arSZGAppFramework::setInputSimulator


bool arSZGAppFramework::setInputSimulator( arInputSimulator* sim ) {
  if (_initCalled) {
    ar_log_error() << "ignoring input-simulator change after init.\n";
    return false;
  }

  if (!sim) {
    sim = &_simulator;
    ar_log_remark() << "default input simulator.\n";
  }
  _simPtr = sim;
  ar_log_remark() << "installed new input simulator.\n";
  return true;
}
//@-node:jimc.20100409112755.290:arSZGAppFramework::setInputSimulator
//@+node:jimc.20100409112755.291:arSZGAppFramework::_appendUserMessage

void arSZGAppFramework::_appendUserMessage( int messageID, const std::string& messageBody ) {
  arGuard _( _userMessageLock, "arSZGAppFramework::_appendUserMessage" );
  _userMessageQueue.push_back( arUserMessageInfo( messageID, messageBody ) );
}
//@-node:jimc.20100409112755.291:arSZGAppFramework::_appendUserMessage
//@+node:jimc.20100409112755.292:arSZGAppFramework::_handleStandaloneInput

void arSZGAppFramework::_handleStandaloneInput() {
  _standaloneControlMode = _SZGClient.getAttribute( "SZG_STANDALONE", "input_config" );
  ar_log_debug() << "SZG_STANDALONE/input_config = " << _standaloneControlMode << ar_endl;
  if (_standaloneControlMode == "NULL") {
    _standaloneControlMode = "simulator";
  }
  if (_standaloneControlMode != "simulator") {
    if (!_loadInputDrivers()) {
      _standaloneControlMode = "simulator";
      ar_log_warning() << "Failed to load input devices; reverting to " <<
        _standaloneControlMode << ".\n";
    }
  }
  if (_standaloneControlMode == "simulator") {
    // _simPtr defaults to &_simulator, a vanilla arInputSimulator instance.
    // If it has been set to something else in code, then we don't mess with it here.
    if (_simPtr == &_simulator) {
      arInputSimulatorFactory simFactory;
      arInputSimulator* tmp = simFactory.createSimulator( _SZGClient );
      if (tmp) {
        _simPtr = tmp;
      }
    }
    _simPtr->registerInputNode( _inputNode );
  }
}
//@-node:jimc.20100409112755.292:arSZGAppFramework::_handleStandaloneInput
//@+node:jimc.20100409112755.293:arSZGAppFramework::_loadInputDrivers

bool arSZGAppFramework::_loadInputDrivers() {
#if defined( AR_USE_WIN_32 ) && !defined( AR_USE_MINGW )
  ar_log_error() << "failed to load input drivers: standalone app linked statically with Visual C++.\n";
  return false;
#else
  const string& config = _SZGClient.getGlobalAttribute( _standaloneControlMode );
  if (!_inputNode) {
    ar_log_error() << "failed to load input drivers: NULL input node.\n";
    return false;
  }
  if (config == "NULL") {
    ar_log_error() << "expected 'simulator' or a global input device parameter (<param> in dbatch file) for SZG_STANDALONE/input_config, not '" << _standaloneControlMode << "'.\n";
    return false;
  }
  arInputNodeConfig inputConfig;
  if (!inputConfig.parseXMLRecord( config )) {
    ar_log_error() << "bad global input device parameter (<param> in dbatch file) '" <<
      _standaloneControlMode << "'\n";
    return false;
  }
  _inputFactory.setInputNodeConfig( inputConfig );
  if (!_inputFactory.configure( _SZGClient )) {
    ar_log_error() << "failed to configure arInputFactory.\n";
    return false;
  }
  unsigned slotNumber = 0;
  if (!_inputFactory.loadInputSources( *_inputNode, slotNumber )) {
    ar_log_error() << "failed to load input sources.\n";
    return false;
  }
  ar_log_debug() << "Loaded input sources from '" << _standaloneControlMode << "'.\n";
  if (!_inputFactory.loadFilters( *_inputNode )) {
    ar_log_error() << "failed to load filters.\n";
    return false;
  }
  ar_log_debug() << "Loaded input filters.\n";
  return true;
#endif
}
//@-node:jimc.20100409112755.293:arSZGAppFramework::_loadInputDrivers
//@+node:jimc.20100409112755.294:arSZGAppFramework::setEyeSpacing

void arSZGAppFramework::setEyeSpacing( float feet) {
  if (!_initCalled) {
    ar_log_warning() << "setEyeSpacing ineffective before init: database will overwrite value.\n";
  }
  _head.setEyeSpacing( feet );
  ar_log_remark() << "eyeSpacing " << feet << " feet.\n";
}
//@-node:jimc.20100409112755.294:arSZGAppFramework::setEyeSpacing
//@+node:jimc.20100409112755.295:arSZGAppFramework::setClipPlanes

void arSZGAppFramework::setClipPlanes( float nearClip, float farClip ) {
  _head.setClipPlanes( nearClip, farClip );
}
//@-node:jimc.20100409112755.295:arSZGAppFramework::setClipPlanes
//@+node:jimc.20100409112755.296:arSZGAppFramework::setUnitConversion

void arSZGAppFramework::setUnitConversion( float unitConv ) {
  if (_initCalled) {
    ar_log_warning() << "setUnitConversion ineffective after init, if using framework-based navigation.\n";
  }
  _head.setUnitConversion( unitConv );
}
//@-node:jimc.20100409112755.296:arSZGAppFramework::setUnitConversion
//@+node:jimc.20100409112755.297:arSZGAppFramework::getUnitConversion

float arSZGAppFramework::getUnitConversion() {
  return _head.getUnitConversion();
}
//@-node:jimc.20100409112755.297:arSZGAppFramework::getUnitConversion
//@+node:jimc.20100409112755.298:arSZGAppFramework::_checkInput

bool arSZGAppFramework::_checkInput() const {
  if (_inputState)
    return true;

  ar_log_warning() << "no input state.\n";
  return false;
}
//@-node:jimc.20100409112755.298:arSZGAppFramework::_checkInput
//@+node:jimc.20100409112755.299:arSZGAppFramework::getButton

int arSZGAppFramework::getButton( const unsigned i ) const {
  return _checkInput() ? _inputState->getButton( i ) : 0;
}
//@-node:jimc.20100409112755.299:arSZGAppFramework::getButton
//@+node:jimc.20100409112755.300:arSZGAppFramework::getAxis

float arSZGAppFramework::getAxis( const unsigned i ) const {
  return _checkInput() ? _inputState->getAxis( i ) : 0.;
}
//@-node:jimc.20100409112755.300:arSZGAppFramework::getAxis
//@+node:jimc.20100409112755.301:arSZGAppFramework::getMatrix

// Scale translation by _unitConversion.
arMatrix4 arSZGAppFramework::getMatrix( const unsigned i, bool doUnitConversion ) const {
  if (!_checkInput()) {
    return arMatrix4();
  }
  arMatrix4 m( _inputState->getMatrix( i ) );
  if (doUnitConversion) {
    for (int j=12; j<15; ++j)
      m.v[j] *= _head.getUnitConversion();
    }
  return m;
}
//@-node:jimc.20100409112755.301:arSZGAppFramework::getMatrix
//@+node:jimc.20100409112755.302:arSZGAppFramework::getOnButton

bool arSZGAppFramework::getOnButton( const unsigned i ) const {
  return _checkInput() && _inputState->getOnButton( i );
}
//@-node:jimc.20100409112755.302:arSZGAppFramework::getOnButton
//@+node:jimc.20100409112755.303:arSZGAppFramework::getOffButton

bool arSZGAppFramework::getOffButton( const unsigned i ) const {
  return _checkInput() && _inputState->getOffButton( i );
}
//@-node:jimc.20100409112755.303:arSZGAppFramework::getOffButton
//@+node:jimc.20100409112755.304:arSZGAppFramework::getNumberButtons

unsigned arSZGAppFramework::getNumberButtons() const {
  return _checkInput() ? _inputState->getNumberButtons() : 0;
}
//@-node:jimc.20100409112755.304:arSZGAppFramework::getNumberButtons
//@+node:jimc.20100409112755.305:arSZGAppFramework::getNumberAxes

unsigned arSZGAppFramework::getNumberAxes() const {
  return _checkInput() ? _inputState->getNumberAxes() : 0;
}
//@-node:jimc.20100409112755.305:arSZGAppFramework::getNumberAxes
//@+node:jimc.20100409112755.306:arSZGAppFramework::getNumberMatrices

unsigned arSZGAppFramework::getNumberMatrices() const {
  return _checkInput() ? _inputState->getNumberMatrices() : 0;
}
//@-node:jimc.20100409112755.306:arSZGAppFramework::getNumberMatrices
//@+node:jimc.20100409112755.307:arSZGAppFramework::setNavTransCondition

bool arSZGAppFramework::setNavTransCondition( char axis,
                                              arInputEventType type,
                                              unsigned i,
                                              float threshold ) {
  return _navManager.setTransCondition( axis, type, i, threshold );
}
//@-node:jimc.20100409112755.307:arSZGAppFramework::setNavTransCondition
//@+node:jimc.20100409112755.308:arSZGAppFramework::setNavRotCondition

bool arSZGAppFramework::setNavRotCondition( char axis,
                                            arInputEventType type,
                                            unsigned i,
                                            float threshold ) {
  return _navManager.setRotCondition( axis, type, i, threshold );
}
//@-node:jimc.20100409112755.308:arSZGAppFramework::setNavRotCondition
//@+node:jimc.20100409112755.309:arSZGAppFramework::setNavTransSpeed

void arSZGAppFramework::setNavTransSpeed( float speed ) {
  if (!_initCalled) {
    ar_log_error() << "ignoring setNavTransSpeed before init.\n";
    return;
  }
  _setNavTransSpeed( speed );
}
//@-node:jimc.20100409112755.309:arSZGAppFramework::setNavTransSpeed
//@+node:jimc.20100409112755.310:arSZGAppFramework::setNavRotSpeed

void arSZGAppFramework::setNavRotSpeed( float speed ) {
  if (!_initCalled) {
    ar_log_error() << "ignoring setNavRotSpeed before init.\n";
    return;
  }
  _setNavRotSpeed( speed );
}
//@-node:jimc.20100409112755.310:arSZGAppFramework::setNavRotSpeed
//@+node:jimc.20100409112755.311:arSZGAppFramework::setNavEffector

void arSZGAppFramework::setNavEffector( const arEffector& effector ) {
  if (!_initCalled) {
    ar_log_error() << "ignoring setNavEffector before init.\n";
    return;
  }
  _setNavEffector( effector );
}
//@-node:jimc.20100409112755.311:arSZGAppFramework::setNavEffector
//@+node:jimc.20100409112755.312:arSZGAppFramework::_setNavTransSpeed

void arSZGAppFramework::_setNavTransSpeed( float speed ) {
  _navManager.setTransSpeed( speed );
  ar_log_remark() << "translation speed is " << speed << ".\n";
}
//@-node:jimc.20100409112755.312:arSZGAppFramework::_setNavTransSpeed
//@+node:jimc.20100409112755.313:arSZGAppFramework::_setNavRotSpeed

void arSZGAppFramework::_setNavRotSpeed( float speed ) {
  _navManager.setRotSpeed( speed );
  ar_log_remark() << "rotation speed is " << speed << ".\n";
}
//@-node:jimc.20100409112755.313:arSZGAppFramework::_setNavRotSpeed
//@+node:jimc.20100409112755.314:arSZGAppFramework::_setNavEffector

void arSZGAppFramework::_setNavEffector( const arEffector& effector ) {
  _navManager.setEffector( effector );
}
//@-node:jimc.20100409112755.314:arSZGAppFramework::_setNavEffector
//@+node:jimc.20100409112755.315:arSZGAppFramework::ownNavParam

void arSZGAppFramework::ownNavParam( const std::string& paramName ) {
  _ownedParams.insert( paramName );
}
//@-node:jimc.20100409112755.315:arSZGAppFramework::ownNavParam
//@+node:jimc.20100409112755.316:arSZGAppFramework::navUpdate

void arSZGAppFramework::navUpdate() {
  if (_useNavInputMatrix) {
    ar_setNavMatrix( getMatrix( _navInputMatrixIndex, _unitConvertNavInputMatrix ) );
    return;
  }
  _navManager.update( getInputState() );
}
//@-node:jimc.20100409112755.316:arSZGAppFramework::navUpdate
//@+node:jimc.20100409112755.317:arSZGAppFramework::navUpdate

void arSZGAppFramework::navUpdate( const arInputEvent& event ) {
  if (_useNavInputMatrix) {
    ar_setNavMatrix( getMatrix( _navInputMatrixIndex, _unitConvertNavInputMatrix ) );
    return;
  }
  _navManager.update( event );
}
//@-node:jimc.20100409112755.317:arSZGAppFramework::navUpdate
//@+node:jimc.20100409112755.318:arSZGAppFramework::navUpdate

void arSZGAppFramework::navUpdate( const arMatrix4& navMatrix ) {
  if (_useNavInputMatrix) {
    ar_setNavMatrix( getMatrix( _navInputMatrixIndex, _unitConvertNavInputMatrix ) );
    return;
  }
  ar_setNavMatrix( navMatrix );
}
//@-node:jimc.20100409112755.318:arSZGAppFramework::navUpdate
//@+node:jimc.20100409112755.319:arSZGAppFramework::setEventFilter

bool arSZGAppFramework::setEventFilter( arFrameworkEventFilter* filter ) {
  if (!_inputNode) {
    ar_log_error() << "failed to install event filter in NULL input device.\n";
    return false;
  }

  if (!filter) {
    filter = &_defaultUserFilter;
  }
  const bool ok = _userEventFilter ?
    _inputNode->replaceFilter( _userEventFilter->getID(), (arIOFilter*)filter, false ) :
    (_inputNode->addFilter( (arIOFilter*)filter, false ) != -1);
  if (ok) {
    _userEventFilter = filter;
  }
  return ok;
}
//@-node:jimc.20100409112755.319:arSZGAppFramework::setEventFilter
//@+node:jimc.20100409112755.320:arSZGAppFramework::setEventCallback

void arSZGAppFramework::setEventCallback( arFrameworkEventCallback callback ) {
  _callbackFilter.setCallback( callback );
}
//@-node:jimc.20100409112755.320:arSZGAppFramework::setEventCallback
//@+node:jimc.20100409112755.321:arSZGAppFramework::setEventQueueCallback

void arSZGAppFramework::setEventQueueCallback( arFrameworkEventQueueCallback callback ) {
  _eventQueueCallback = callback;
  _callbackFilter.saveEventQueue( callback != NULL );
}
//@-node:jimc.20100409112755.321:arSZGAppFramework::setEventQueueCallback
//@+node:jimc.20100409112755.322:arSZGAppFramework::processEventQueue

void arSZGAppFramework::processEventQueue() {
  // Get AND clear buffered events.
  arInputEventQueue q = _callbackFilter.getEventQueue();
  onProcessEventQueue( q );
}
//@-node:jimc.20100409112755.322:arSZGAppFramework::processEventQueue
//@+node:jimc.20100409112755.323:arSZGAppFramework::onProcessEventQueue

void arSZGAppFramework::onProcessEventQueue( arInputEventQueue& q ) {
  if (_eventQueueCallback) {
    _eventQueueCallback( *this, q );
  }
}
//@-node:jimc.20100409112755.323:arSZGAppFramework::onProcessEventQueue
//@+node:jimc.20100409112755.324:arSZGAppFramework::postInputEventQueue

void arSZGAppFramework::postInputEventQueue( arInputEventQueue& q ) {
  if (_inOnInputEvent) {
    ar_log_error() << "FATAL: postInputEventQueue() called in event-handling code\n";
    stop(false);
  }
  _inputNode->postEventQueue( q );
}
//@-node:jimc.20100409112755.324:arSZGAppFramework::postInputEventQueue
//@+node:jimc.20100409112755.325:arSZGAppFramework::postInputEvent

void arSZGAppFramework::postInputEvent( arInputEvent& event ) {
  arInputEventQueue q;
  q.appendEvent( event );
  postInputEventQueue( q );
}
//@-node:jimc.20100409112755.325:arSZGAppFramework::postInputEvent
//@+node:jimc.20100409112755.326:arSZGAppFramework::_installFilters

void arSZGAppFramework::_installFilters() {
  setEventFilter( &_defaultUserFilter );
  _inputNode->addFilter( (arIOFilter*)&_callbackFilter, false );
}
//@-node:jimc.20100409112755.326:arSZGAppFramework::_installFilters
//@+node:jimc.20100409112755.327:arSZGAppFramework::_loadNavParameters

static bool ___firstNavLoad = true; // todo: make this a member of arSZGAppFramework

void arSZGAppFramework::_loadNavParameters() {
  ar_log_debug() << "loading SZG_NAV parameters.\n";
  _useNavInputMatrix = _SZGClient.getAttribute("SZG_NAV", "use_nav_input_matrix") == "true";
  _unitConvertNavInputMatrix = _SZGClient.getAttribute("SZG_NAV", "unit_convert_nav_input_matrix") == "true";
  string temp = _SZGClient.getAttribute("SZG_NAV", "nav_input_matrix_index");
  int matrixIndex(2);
  if (temp != "NULL") {
    bool stat = ar_stringToIntValid( temp, matrixIndex );
    if (!stat || (matrixIndex < 0)) {
      matrixIndex = 2;
      ar_log_error() << "Invalid value '" << temp << "' for SZG_NAV/nav_input_matrix_index; defaulting to "
        << matrixIndex << ar_endl;
    }
  }
  _navInputMatrixIndex = (unsigned)matrixIndex;
  if (___firstNavLoad ||_paramNotOwned( "effector" )) {
    int params[5] = { 1, 1, 0, 6, 0 };
    temp = _SZGClient.getAttribute("SZG_NAV", "effector");
    if (temp != "NULL") {
      if (ar_parseIntString( temp, params, 5 ) != 5) {
        ar_log_error() << "failed to load SZG_NAV/effector.\n";
      }
    }
    temp = "effector is ";
    for (unsigned i=0; i<5; i++) {
      temp += ar_intToString(params[i]) + (i==4 ? ".\n" : "/");
    }
    ar_log_remark() << temp;
    _setNavEffector(
      arEffector( params[0], params[1], params[2], 0, params[3], params[4], 0 ) );
  }
  float speed = 5.;
  if (___firstNavLoad || _paramNotOwned( "translation_speed" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "translation_speed");
    if (temp != "NULL") {
      if (!ar_stringToFloatValid( temp, speed )) {
        ar_log_error() << "failed to convert SZG_NAV/translation_speed.\n";
      }
    }
    _setNavTransSpeed( speed*_head.getUnitConversion() );
  }
  if (___firstNavLoad || _paramNotOwned( "rotation_speed" )) {
    speed = 30.;
    temp = _SZGClient.getAttribute("SZG_NAV", "rotation_speed");
    if (temp != "NULL") {
      if (!ar_stringToFloatValid( temp, speed )) {
        ar_log_error() << "failed to convert SZG_NAV/rotation_speed.\n";
      }
    }
    _setNavRotSpeed( speed );
  }
  arInputEventType theType;
  unsigned index = 1;
  float threshold = 0.;

  if (___firstNavLoad || _paramNotOwned( "x_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "x_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavTransCondition( 'x', theType, index, threshold );
        ar_log_remark() << "x_translation condition is " << temp << ".\n";
      } else {
        setNavTransCondition( 'x', AR_EVENT_AXIS, 0, 0.2 );
        ar_log_error() << "failed to load SZG_NAV/x_translation, defaulting to axis/0/0.2.\n";
      }
    } else {
      setNavTransCondition( 'x', AR_EVENT_AXIS, 0, 0.2 );
      ar_log_remark() << "SZG_NAV/x_translation defaulting to axis/0/0.2.\n";
    }
  }

  if (___firstNavLoad || _paramNotOwned( "z_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "z_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavTransCondition( 'z', theType, index, threshold );
        ar_log_remark() << "z_translation condition is " << temp << ".\n";
      } else {
        setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );
        ar_log_error() << "failed to load SZG_NAV/z_translation '" << temp << "', defaulting to axis/1/0.2.\n";
      }
    } else {
      setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );
      ar_log_remark() << "SZG_NAV/z_translation defaulting to axis/1/0.2.\n";
    }
  }

  if (___firstNavLoad || _paramNotOwned( "y_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "y_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavTransCondition( 'y', theType, index, threshold );
        ar_log_remark() << "y_translation condition is " << temp << ".\n";
      } else {
        ar_log_error() << "unexpected SZG_NAV/y_translation '" << temp << "'.\n";
      }
    }
  }

  if (___firstNavLoad || _paramNotOwned( "x_rotation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "x_rotation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavRotCondition( 'x', theType, index, threshold );
        ar_log_remark() << "x_rotation condition is " << temp << ".\n";
      } else {
        ar_log_error() << "unexpected SZG_NAV/x_rotation '" << temp << "'.\n";
      }
    }
  }

  if (___firstNavLoad || _paramNotOwned( "y_rotation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "y_rotation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavRotCondition( 'y', theType, index, threshold );
        ar_log_remark() << "y_rotation condition is " << temp << ".\n";
      } else {
        ar_log_error() << "unexpected SZG_NAV/y_rotation '" << temp << "'.\n";
      }
    }
  }

  if (___firstNavLoad || _paramNotOwned( "z_rotation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "z_rotation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavRotCondition( 'z', theType, index, threshold );
        ar_log_remark() << "z_rotation condition is " << temp << ".\n";
      } else {
        ar_log_error() << "unexpected SZG_NAV/z_rotation '" << temp << "'.\n";
      }
    }
  }

  ___firstNavLoad = false;
}
//@-node:jimc.20100409112755.327:arSZGAppFramework::_loadNavParameters
//@+node:jimc.20100409112755.328:arSZGAppFramework::_parseNavParamString

bool arSZGAppFramework::_parseNavParamString( const string& theString,
                                              arInputEventType& theType,
                                              unsigned& index,
                                              float& threshold ) {
  std::vector<std::string> params;
  if (!ar_getTokenList( theString, params, '/' )) {
    ar_log_error() << "failed to parse SZG_NAV string '" << theString << "'.\n";
    return false;
  }

  if (params.size() != 3) {
    ar_log_error() << "SZG_NAV string without 3 items,\n"
                     << "event_type(string)/index(unsigned int)/threshold(float).\n";
    return false;
  }

  int ind = -1;
  if (!ar_stringToIntValid( params[1], ind ) ||
      !ar_stringToFloatValid( params[2], threshold )) {
    ar_log_error() << "failed to convert SZG_NAV string '" << theString << "'.\n";
    return false;
  }

  if (params[0] == "axis")
    theType = AR_EVENT_AXIS;
  else if (params[0] == "button")
    theType = AR_EVENT_BUTTON;
  else {
    ar_log_error() << "SZG_NAV string has invalid event type " << params[0] << ".\n";
    return false;
  }

  if (ind < 0) {
    ar_log_error() << "negative SZG_NAV index field " << ind << ".\n";
    return false;
  }

  index = unsigned(ind);
  return true;
}
//@-node:jimc.20100409112755.328:arSZGAppFramework::_parseNavParamString
//@+node:jimc.20100409112755.329:arSZGAppFramework::_paramNotOwned

// todo: negate the sense of this, for clarity
bool arSZGAppFramework::_paramNotOwned( const std::string& theString ) {
  return _ownedParams.find( theString ) == _ownedParams.end();
}
//@-node:jimc.20100409112755.329:arSZGAppFramework::_paramNotOwned
//@+node:jimc.20100409112755.330:arSZGAppFramework::_okToInit

bool arSZGAppFramework::_okToInit(const char* exename) {
  if (_initCalled) {
    ar_log_error() << "ignoring duplicate init().\n";
    return false;
  }
  if (_startCalled) {
    ar_log_error() << "ignoring init() after start().\n";
    return false;
  }
  _label = ar_stripExeName( exename );
  return true;
}
//@-node:jimc.20100409112755.330:arSZGAppFramework::_okToInit
//@+node:jimc.20100409112755.331:arSZGAppFramework::_okToStart

bool arSZGAppFramework::_okToStart() const {
  if (!_initCalled) {
    ar_log_error() << "ignoring start() before init().\n";
    return false;
  }

  if (_startCalled) {
    ar_log_error() << "ignoring duplicate start().\n";
    return false;
  }

  return true;
}
//@-node:jimc.20100409112755.331:arSZGAppFramework::_okToStart
//@-others
//@-node:jimc.20100409112755.285:@thin framework\arSZGAppFramework.cpp
//@-leo
