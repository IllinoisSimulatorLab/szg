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
  _inputDevice(0),
  _inputState(0),
  _label("arSZGAppFramework"),
  _launcher(_label.c_str()), // pointless, since derived class sets _label after constructor?
  _vircompExecution(false),
  _standalone(false),
  _standaloneControlMode("simulator"),
  _simPtr(&_simulator),
  _showSimulator(true),
  _showPerformance( false ),
#ifndef AR_LINKING_STATIC
  _inputFactory(),
#endif
  _callbackFilter(this),
  _defaultUserFilter(this),
  _userEventFilter(NULL),
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

arSZGAppFramework::~arSZGAppFramework() {
  if (_inputDevice != 0)
    delete _inputDevice;
}

void arSZGAppFramework::speak( const std::string& message ) {
  if (_speechNodeID == -1) {
    _speechNodeID = dsSpeak( "framework_messages", "root", message );
  } else {
    dsSpeak( _speechNodeID, message );
  }
}

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
  return true;
}

void arSZGAppFramework::_appendUserMessage( int messageID, const std::string& messageBody ) {
  arGuard _( _userMessageLock, "arSZGAppFramework::_appendUserMessage" );
  _userMessageQueue.push_back( arUserMessageInfo( messageID, messageBody ) );
}

void arSZGAppFramework::_handleStandaloneInput() {
  _standaloneControlMode = _SZGClient.getAttribute( "SZG_STANDALONE", "input_config" );
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
    _simPtr->registerInputNode( _inputDevice );
  }
}

bool arSZGAppFramework::_loadInputDrivers() {
#ifdef AR_LINKING_STATIC
  ar_log_error() << "failed to load input drivers: standalone app linked statically.\n";
  return false;
#else
  const string& config = _SZGClient.getGlobalAttribute( _standaloneControlMode );
  if (!_inputDevice) {
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
  if (!_inputFactory.loadInputSources( *_inputDevice, slotNumber )) {
    ar_log_error() << "failed to load input sources.\n";
    return false;
  }
  ar_log_debug() << "Loaded input sources from '" << _standaloneControlMode << "'.\n";
  if (!_inputFactory.loadFilters( *_inputDevice )) {
    ar_log_error() << "failed to load filters.\n";
    return false;
  }
  ar_log_debug() << "Loaded input filters.\n";
  return true;
#endif
}

void arSZGAppFramework::setEyeSpacing( float feet) {
  if (!_initCalled) {
    ar_log_warning() << "setEyeSpacing ineffective before init: database will overwrite value.\n";
  }
  _head.setEyeSpacing( feet );
  ar_log_remark() << "eyeSpacing " << feet << " feet.\n";
}

void arSZGAppFramework::setClipPlanes( float nearClip, float farClip ) {
  _head.setClipPlanes( nearClip, farClip );
}

void arSZGAppFramework::setUnitConversion( float unitConv ) {
  if (_initCalled) {
    ar_log_warning() << "setUnitConversion ineffective after init, if using framework-based navigation.\n";
  }
  _head.setUnitConversion( unitConv );
}

float arSZGAppFramework::getUnitConversion() {
  return _head.getUnitConversion();
}

bool arSZGAppFramework::_checkInput() const {
  if (_inputState)
    return true;

  ar_log_warning() << "no input state.\n";
  return false;
}

int arSZGAppFramework::getButton( const unsigned i ) const {
  return _checkInput() ? _inputState->getButton( i ) : 0;
}

float arSZGAppFramework::getAxis( const unsigned i ) const {
  return _checkInput() ? _inputState->getAxis( i ) : 0.;
}

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

bool arSZGAppFramework::getOnButton( const unsigned i ) const {
  return _checkInput() && _inputState->getOnButton( i );
}

bool arSZGAppFramework::getOffButton( const unsigned i ) const {
  return _checkInput() && _inputState->getOffButton( i );
}

unsigned arSZGAppFramework::getNumberButtons() const {
  return _checkInput() ? _inputState->getNumberButtons() : 0;
}

unsigned arSZGAppFramework::getNumberAxes() const {
  return _checkInput() ? _inputState->getNumberAxes() : 0;
}

unsigned arSZGAppFramework::getNumberMatrices() const {
  return _checkInput() ? _inputState->getNumberMatrices() : 0;
}

bool arSZGAppFramework::setNavTransCondition( char axis,
                                              arInputEventType type,
                                              unsigned i,
                                              float threshold ) {
  return _navManager.setTransCondition( axis, type, i, threshold );
}

bool arSZGAppFramework::setNavRotCondition( char axis,
                                            arInputEventType type,
                                            unsigned i,
                                            float threshold ) {
  return _navManager.setRotCondition( axis, type, i, threshold );
}

void arSZGAppFramework::setNavTransSpeed( float speed ) {
  if (!_initCalled) {
    ar_log_error() << "ignoring setNavTransSpeed before init.\n";
    return;
  }
  _setNavTransSpeed( speed );
}

void arSZGAppFramework::setNavRotSpeed( float speed ) {
  if (!_initCalled) {
    ar_log_error() << "ignoring setNavRotSpeed before init.\n";
    return;
  }
  _setNavRotSpeed( speed );
}

void arSZGAppFramework::setNavEffector( const arEffector& effector ) {
  if (!_initCalled) {
    ar_log_error() << "ignoring setNavEffector before init.\n";
    return;
  }
  _setNavEffector( effector );
}

void arSZGAppFramework::_setNavTransSpeed( float speed ) {
  _navManager.setTransSpeed( speed );
  ar_log_remark() << "translation speed is " << speed << ".\n";
}

void arSZGAppFramework::_setNavRotSpeed( float speed ) {
  _navManager.setRotSpeed( speed );
  ar_log_remark() << "rotation speed is " << speed << ".\n";
}

void arSZGAppFramework::_setNavEffector( const arEffector& effector ) {
  _navManager.setEffector( effector );
}

void arSZGAppFramework::ownNavParam( const std::string& paramName ) {
  _ownedParams.insert( paramName );
}

void arSZGAppFramework::navUpdate() {
  if (_useNavInputMatrix) {
    ar_setNavMatrix( getMatrix( _navInputMatrixIndex, _unitConvertNavInputMatrix ) );
    return;
  }
  _navManager.update( getInputState() );
}

void arSZGAppFramework::navUpdate( const arInputEvent& event ) {
  if (_useNavInputMatrix) {
    ar_setNavMatrix( getMatrix( _navInputMatrixIndex, _unitConvertNavInputMatrix ) );
    return;
  }
  _navManager.update( event );
}

void arSZGAppFramework::navUpdate( const arMatrix4& navMatrix ) {
  if (_useNavInputMatrix) {
    ar_setNavMatrix( getMatrix( _navInputMatrixIndex, _unitConvertNavInputMatrix ) );
    return;
  }
  ar_setNavMatrix( navMatrix );
}

bool arSZGAppFramework::setEventFilter( arFrameworkEventFilter* filter ) {
  if (!_inputDevice) {
    ar_log_error() << "failed to install event filter in NULL input device.\n";
    return false;
  }

  if (!filter) {
    filter = &_defaultUserFilter;
  }
  const bool ok = _userEventFilter ?
    _inputDevice->replaceFilter( _userEventFilter->getID(), (arIOFilter*)filter, false ) :
    (_inputDevice->addFilter( (arIOFilter*)filter, false ) != -1);
  if (ok) {
    _userEventFilter = filter;
  }
  return ok;
}

void arSZGAppFramework::setEventCallback( arFrameworkEventCallback callback ) {
  _callbackFilter.setCallback( callback );
}

void arSZGAppFramework::setEventQueueCallback( arFrameworkEventQueueCallback callback ) {
  _eventQueueCallback = callback;
  _callbackFilter.saveEventQueue( callback != NULL );
}

void arSZGAppFramework::processEventQueue() {
  // Get AND clear buffered events.
  arInputEventQueue q = _callbackFilter.getEventQueue();
  onProcessEventQueue( q );
}

void arSZGAppFramework::onProcessEventQueue( arInputEventQueue& q ) {
  if (_eventQueueCallback) {
    _eventQueueCallback( *this, q );
  }
}

void arSZGAppFramework::_installFilters() {
  setEventFilter( &_defaultUserFilter );
  _inputDevice->addFilter( (arIOFilter*)&_callbackFilter, false );
}

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

// todo: negate the sense of this, for clarity
bool arSZGAppFramework::_paramNotOwned( const std::string& theString ) {
  return _ownedParams.find( theString ) == _ownedParams.end();
}

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
