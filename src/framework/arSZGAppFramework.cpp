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
  _launcher(_label.c_str()),	// pointless, since derived class sets _label after constructor?
  _vircompExecution(false),
  _standalone(false),
  _standaloneControlMode("simulator"),
  _simPtr(&_simulator),
  _showSimulator(true),
  _showPerformance( false ),
  _inputFactory(),
  _callbackFilter(this),
  _defaultUserFilter(),
  _userEventFilter(NULL),
  _unitSoundConversion(1.),
  _speechNodeID(-1),
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
  _stopped(false){
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

bool arSZGAppFramework::setInputSimulator( arInputSimulator* sim ) {
  if (_parametersLoaded) {
    ar_log_error() << "arSZGAppFramework can't set a new input simulator after framework init().\n";
    return false;
  }

  if (!sim) {
    ar_log_remark() << "arSZGAppFramework: default input simulator.\n";
    _simPtr = &_simulator;
    return true;
  }

  _simPtr = sim;
  return true;
}

void arSZGAppFramework::_handleStandaloneInput() {
  // Which mode are we using? The simulator mode is the default.
  _standaloneControlMode = _SZGClient.getAttribute( "SZG_STANDALONE", "input_config" );
  if (_standaloneControlMode == "NULL") {
    _standaloneControlMode = "simulator";
  }
  if (_standaloneControlMode != "simulator") {
    if (!_loadInputDrivers()) {
      ar_log_error() << "Failed to load input devices, defaulting to simulator mode.\n";
      _standaloneControlMode = "simulator";
    }
  }
  if (_standaloneControlMode == "simulator") {
    // _simPtr defaults to &_simulator, a vanilla arInputSimulator instance.
    // If it has been set to something else in code, then we don't mess with it here.
    if (_simPtr == &_simulator) {
      arInputSimulatorFactory simFactory;
      arInputSimulator* simTemp = simFactory.createSimulator( _SZGClient );
      if (simTemp) {
        _simPtr = simTemp;
      }
    }
    _simPtr->registerInputNode( _inputDevice );
  }
}

bool arSZGAppFramework::_loadInputDrivers() {
  arInputNodeConfig inputConfig;
  int slotNumber = 0;
  const string& config = _SZGClient.getGlobalAttribute( _standaloneControlMode );

  if (!_inputDevice) {
    ar_log_error() << "Can't load input drivers; NULL input node pointer.\n";
    return false;
  }

  arInputNode& inputNode = *_inputDevice;

  if (config == "NULL") {
    ar_log_error() << "invalid value '" << _standaloneControlMode
                   << "' for SZG_STANDALONE/input_config;\n   must be either 'simulator'"
                   << "or the name of a global input device parameter (<param> in dbatch file).\n";
    return false;
  }
  if (!inputConfig.parseXMLRecord( config )) {
    ar_log_error() << "misconfigured global input device parameter (<param> in dbatch file) '"
                   << _standaloneControlMode << "'\n";
    return false;
  }
  _inputFactory.setInputNodeConfig( inputConfig );
  if (!_inputFactory.configure( _SZGClient )) {
    ar_log_error() << "failed to configure arInputFactory.\n";
    return false;
  }
  if (!_inputFactory.loadInputSources( inputNode, slotNumber )) {
    ar_log_error() << "failed to load input sources.\n";
    return false;
  }
  ar_log_debug() << "Loaded input sources in global param '" << _standaloneControlMode
                 << "'.\n";
  if (!_inputFactory.loadFilters( inputNode )) {
    ar_log_error() << "failed to load filters.\n";
    return false;
  }
  ar_log_debug() << "Loaded input filters.\n";
  return true;
}

void arSZGAppFramework::setEyeSpacing( float feet) {
  _head.setEyeSpacing( feet );
  ar_log_remark() << "arSZGAppFramework: eyeSpacing " << feet << " feet.\n";
}

void arSZGAppFramework::setClipPlanes( float nearClip, float farClip ) {
  _head.setClipPlanes( nearClip, farClip );
}

void arSZGAppFramework::setUnitConversion( float unitConv ) {
  _head.setUnitConversion( unitConv );
}

void arSZGAppFramework::setUnitSoundConversion( float unitSoundConv ) {
  _unitSoundConversion = unitSoundConv;
}

float arSZGAppFramework::getUnitConversion() {
  return _head.getUnitConversion();
}

float arSZGAppFramework::getUnitSoundConversion() {
  return _unitSoundConversion;
}

int arSZGAppFramework::getButton( const unsigned i ) const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0;
  }
  return _inputState->getButton( i );
}

float arSZGAppFramework::getAxis( const unsigned i ) const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0.;
  }
  return _inputState->getAxis( i );
}

// NOTE: scales translation component by _unitConversion.
arMatrix4 arSZGAppFramework::getMatrix( const unsigned i, bool doUnitConversion ) const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return ar_identityMatrix();
  }
  arMatrix4 theMatrix( _inputState->getMatrix( i ) );
  if (doUnitConversion) {
  for (int j=12; j<15; j++)
      theMatrix.v[j] *= _head.getUnitConversion();
  }
  return theMatrix;
}

bool arSZGAppFramework::getOnButton( const unsigned i ) const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0;
  }
  return _inputState->getOnButton( i );
}

bool arSZGAppFramework::getOffButton( const unsigned i ) const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0;
  }
  return _inputState->getOffButton( i );
}

unsigned arSZGAppFramework::getNumberButtons() const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0;
  }
  return _inputState->getNumberButtons();
}

unsigned arSZGAppFramework::getNumberAxes() const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0;
  }
  return _inputState->getNumberAxes();
}

unsigned arSZGAppFramework::getNumberMatrices() const {
  if (!_inputState) {
    ar_log_warning() << "arSZGAppFramework: no input state.\n";
    return 0;
  }
  return _inputState->getNumberMatrices();
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
  _navManager.setTransSpeed( speed );
}

void arSZGAppFramework::setNavRotSpeed( float speed ) {
  _navManager.setRotSpeed( speed );
}

void arSZGAppFramework::setNavEffector( const arEffector& effector ) {
  _navManager.setEffector( effector );
}

void arSZGAppFramework::ownNavParam( const std::string& paramName ) {
  _ownedParams.insert( paramName );
}

void arSZGAppFramework::navUpdate() {
  _navManager.update( getInputState() );
}

void arSZGAppFramework::navUpdate( arInputEvent& event ) {
  _navManager.update( event );
}

bool arSZGAppFramework::setEventFilter( arFrameworkEventFilter* filter ) {
  if (!_inputDevice) {
    ar_log_error() << "arSZGAppFramework: can't install event filter in NULL input device.\n";
    return false;
  }
  if (!filter) {
    filter = &_defaultUserFilter;
  }
  const bool stat = _userEventFilter ?
    _inputDevice->replaceFilter( _userEventFilter->getID(), (arIOFilter*)filter, false ) :
    (_inputDevice->addFilter( (arIOFilter*)filter, false ) != -1);
  if (stat) {
    _userEventFilter = filter;
  }
  return stat;
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
  arInputEventQueue theQueue = _callbackFilter.getEventQueue();
  onProcessEventQueue( theQueue );
}

void arSZGAppFramework::onProcessEventQueue( arInputEventQueue& theQueue ) {
  if (_eventQueueCallback) {
    _eventQueueCallback( *this, theQueue );
  }
}

void arSZGAppFramework::_installFilters() {
  setEventFilter( &_defaultUserFilter ); 
  _inputDevice->addFilter( (arIOFilter*)&_callbackFilter, false );
}

static bool ___firstNavLoad = true;

void arSZGAppFramework::_loadNavParameters() {
  ar_log_remark() << "arSZGAppFramework: loading SZG_NAV parameters.\n";
  std::string temp;
  if ((___firstNavLoad)||_paramNotOwned( "effector" )) {
    int params[5] = { 1, 1, 0, 2, 0 };
    temp = _SZGClient.getAttribute("SZG_NAV", "effector");
    if (temp != "NULL") {
      if (ar_parseIntString( temp, params, 5 ) != 5) {
        ar_log_warning() << "arSZGAppFramework failed to read SZG_NAV/effector.\n";
      }
    }
    ar_log_remark() << "arSZGAppFramework: setting effector to ";
    for (unsigned i=0; i<7; i++) {
      ar_log_remark() << params[i] << ((i==6)?("\n"):("/"));
    }
    _navManager.setEffector( arEffector( params[0], params[1], params[2], 0,
                                         params[3], params[4], 0 ) );
  }
  float speed(5.);
  if ((___firstNavLoad) || _paramNotOwned( "translation_speed" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "translation_speed");
    if (temp != "NULL") {
      if (!ar_stringToFloatValid( temp, speed )) {
        ar_log_warning() << "arSZGAppFramework failed to convert SZG_NAV/translation_speed.\n";
      }
    }
    ar_log_remark() << "arSZGAppFramework: translation speed is " << speed << ".\n";
    setNavTransSpeed( speed*_head.getUnitConversion() );
  }
  if ((___firstNavLoad) || _paramNotOwned( "rotation_speed" )) {
    speed = 30.;
    temp = _SZGClient.getAttribute("SZG_NAV", "rotation_speed");
    if (temp != "NULL") {
      if (!ar_stringToFloatValid( temp, speed )) {
        ar_log_warning() << "arSZGAppFramework failed to convert SZG_NAV/rotation_speed.\n";
      }
    }
    ar_log_remark() << "arSZGAppFramework: rotation speed is " << speed << ".\n";
    setNavRotSpeed( speed );
  }
  arInputEventType theType;
  unsigned index = 1;
  float threshold = 0.;
  
  if ((___firstNavLoad)||_paramNotOwned( "x_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "x_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavTransCondition( 'x', theType, index, threshold );
	ar_log_remark() << "arSZGAppFramework: x_translation condition is "
			<< temp << ar_endl;
      } else {
        setNavTransCondition( 'x', AR_EVENT_AXIS, 0, 0.2 );  
	ar_log_warning() << "arSZGAppFramework failed to read SZG_NAV/x_translation.\n"
                         << "   defaulting to axis/0/0.2.\n";
      }
    } else {
      setNavTransCondition( 'x', AR_EVENT_AXIS, 0, 0.2 );  
      ar_log_remark() << "arSZGAppFramework: "
		      << "SZG_NAV/x_translation defaulting to axis/0/0.2.\n";
    }
  }
  
  if ((___firstNavLoad) || _paramNotOwned( "z_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "z_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold )) {
        setNavTransCondition( 'z', theType, index, threshold );
	ar_log_remark() << "arSZGAppFramework: z_translation condition is "
                        << temp << ar_endl;
      } else {
        setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );
	ar_log_warning() << "arSZGAppFramework failed to read SZG_NAV/z_translation.\n"
                         << "   defaulting to axis/1/0.2.\n";
      }
    } else {
      setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );  
      ar_log_remark() << "arSZGAppFramework: "
		      << "SZG_NAV/z_translation defaulting to axis/1/0.2.\n";
    }
  }
  if ((___firstNavLoad) || _paramNotOwned( "y_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "y_translation");
    if (temp != "NULL") {
      if (!_parseNavParamString( temp, theType, index, threshold )) {
	ar_log_warning() << "arSZGAppFramework failed to read SZG_NAV/y_translation.\n";
      } else {
        setNavTransCondition( 'y', theType, index, threshold );
	ar_log_remark() << "arSZGAppFramework: y_translation condition is "
			<< temp << ar_endl;
      }
    }
  }
  if ((___firstNavLoad) || _paramNotOwned( "y_rotation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "y_rotation");
    if (temp != "NULL") {
      if (!_parseNavParamString( temp, theType, index, threshold )) {
	ar_log_warning() << "arSZGAppFramework failed to read SZG_NAV/y_rotation.\n";
      } else {
        setNavRotCondition( 'y', theType, index, threshold );
	ar_log_remark() << "arSZGAppFramework: y_rotation condition is "
                        << temp << ar_endl;
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
    ar_log_warning() << "arSZGAppFramework failed to parse SZG_NAV string '"
	             << theString << "'.\n";
    return false;
  }
  if (params.size() != 3) {
    ar_log_warning() << "arSZGAppFramework: "
	             << "SZG_NAV string must contain 3 items,\n"
	             << "event_type(string)/index(unsigned int)/threshold(float).\n";
    return false;
  }
  int ind = -1;
  if (!ar_stringToIntValid( params[1], ind ) ||
      !ar_stringToFloatValid( params[2], threshold )) {
    ar_log_warning() << "arSZGAppFramework failed to convert string SZG_NAV '"
	             << theString << "'.\n";
    return false;
  }
  if (params[0] == "axis")
    theType = AR_EVENT_AXIS;
  else if (params[0] == "button")
    theType = AR_EVENT_BUTTON;
  else {
    ar_log_warning() << "arSZGAppFramework: SZG_NAV string invalid event type "
	             << params[0] << ar_endl;
    return false;
  }
  if (ind < 0) {
    ar_log_error() << "arSZGAppFramework: SZG_NAV index field (" 
                   << ind << ") " << "< 0.\n";
    return false;
  }
  index = unsigned(ind);
  return true;
}    

// todo: negate the sense of this, for clarity
bool arSZGAppFramework::_paramNotOwned( const std::string& theString ) {
  return _ownedParams.find( theString ) == _ownedParams.end();
}
