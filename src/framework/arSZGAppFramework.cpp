//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGAppFramework.h"

// NOTE: the defaults include an average eye spacing of 6 cm.
arSZGAppFramework::arSZGAppFramework() :
  _inputDevice(0),
  _inputState(0),
  _vircompExecution(false),
  _callbackFilter(this),
  _eventFilter(0),
  _eyeSpacingFeet(6/(12*2.54)),
  _nearClip(0.05),
  _farClip(1000.),
  _unitConversion(1.),
  _unitSoundConversion(1.),
  _useExternalThread(false),
  _externalThreadRunning(false),
  _blockUntilDisplayExit(false),
  _exitProgram(false),
  _displayThreadRunning(false),
  _stopped(false){
}

arSZGAppFramework::~arSZGAppFramework() {
  if (_inputDevice != 0)
    delete _inputDevice;
}

void arSZGAppFramework::setEyeSpacing( float feet) {
  _eyeSpacingFeet = feet;
  cerr << "arSZGAppFramework remark: eyeSpacing set to " << feet << " feet.\n";
}

void arSZGAppFramework::setClipPlanes( float nearClip, float farClip ) {
  _nearClip = nearClip;
  _farClip = farClip;
}

void arSZGAppFramework::setUnitConversion( float unitConv ) {
  _unitConversion = unitConv;
}

void arSZGAppFramework::setUnitSoundConversion( float unitSoundConv ) {
  _unitSoundConversion = unitSoundConv;
}

float arSZGAppFramework::getUnitConversion() {
  return _unitConversion;
}

float arSZGAppFramework::getUnitSoundConversion() {
  return _unitSoundConversion;
}

int arSZGAppFramework::getButton( const unsigned int i ) const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: NULL input state ptr.\n";
    return 0;
  }
  return _inputState->getButton( i );
}

float arSZGAppFramework::getAxis( const unsigned int i ) const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return 0.;
  }
  return _inputState->getAxis( i );
}

// NOTE: scales translation component by _unitConversion
arMatrix4 arSZGAppFramework::getMatrix( const unsigned int i ) const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return ar_identityMatrix();
  }
  arMatrix4 theMatrix( _inputState->getMatrix( i ) );
  for (int j=12; j<15; j++)
    theMatrix.v[j] *= _unitConversion;
  return theMatrix;
}

bool arSZGAppFramework::getOnButton( const unsigned int i ) const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return 0;
  }
  return _inputState->getOnButton( i );
}

bool arSZGAppFramework::getOffButton( const unsigned int i ) const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return 0;
  }
  return _inputState->getOffButton( i );
}

unsigned int arSZGAppFramework::getNumberButtons()  const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return 0;
  }
  return _inputState->getNumberButtons();
}

unsigned int arSZGAppFramework::getNumberAxes()     const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return 0;
  }
  return _inputState->getNumberAxes();
}

unsigned int arSZGAppFramework::getNumberMatrices() const {
  if (!_inputState) {
    cerr << "arSZGAppFramework warning: no input state.\n";
    return 0;
  }
  return _inputState->getNumberMatrices();
}

bool arSZGAppFramework::setNavTransCondition( char axis,
                                              arInputEventType type,
                                              unsigned int i,
                                              float threshold ) {
  return _navManager.setTransCondition( axis, type, i, threshold );
}

bool arSZGAppFramework::setNavRotCondition( char axis,
                                            arInputEventType type,
                                            unsigned int i,
                                            float threshold ) {
  return _navManager.setRotCondition( axis, type, i, threshold );
}

//bool arSZGAppFramework::setWorldRotGrabCondition( arInputEventType type,
//                                                  unsigned int i,
//                                                  float threshold ) {
//  return _navManager.setWorldRotGrabCondition( type, i, threshold );
//}

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

void arSZGAppFramework::setEventFilter( arFrameworkEventFilter* filter ) {
  if (!_inputDevice) {
    cerr << "arSZGAppFramework error: attempt to install event filter in NULL "
         << "input device.\n";
    return;
  }
  if (_eventFilter != 0) {
    _inputDevice->removeFilter( (arIOFilter*)_eventFilter );
  }
  if (filter != 0) {
    _eventFilter = filter;
  } else {
    _eventFilter = (arFrameworkEventFilter*)&_callbackFilter;
  }
  _inputDevice->addFilter( (arIOFilter*)_eventFilter, false );
  cout << "arSZGAppFramework remark: event filter set.\n";
}

void arSZGAppFramework::setEventCallback( arFrameworkEventCallback callback ) {
  _callbackFilter.setCallback( callback );
  setEventFilter(0);
  cout << "arSZGAppFramework remark: event callback set.\n";
}

static bool ___firstNavLoad = true;

void arSZGAppFramework::_loadNavParameters() {
  // we want to move the error messages to the window where
  // the command was launched
  stringstream& initStream = _SZGClient.initResponse();
  initStream << "arSZGAppFramework remark: loading SZG_NAV parameters.\n";
  std::string temp;
  if ((___firstNavLoad)||_paramNotOwned( "effector" )) {
    int params[5] = { 1, 1, 0, 2, 0 };
    temp = _SZGClient.getAttribute("SZG_NAV", "effector");
    if (temp != "NULL") {
      if (ar_parseIntString( temp, params, 5 ) != 0) {
        initStream << "arSZGAppFramework error: "
		   << "failed to read SZG_NAV/effector.\n";
      }
    }
    initStream << "arSZGAppFramework remark: setting effector to ";
    for (unsigned int i=0; i<7; i++) {
      initStream << params[i] << ((i==6)?("\n"):("/"));
    }
    _navManager.setEffector( arEffector( params[0], params[1], params[2], 0,
                                         params[3], params[4], 0 ) );
  }
  float speed(5.);
  if ((___firstNavLoad)||_paramNotOwned( "translation_speed" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "translation_speed");
    if (temp != "NULL") {
      if (!ar_stringToFloatValid( temp, speed )) {
        initStream << "arSZGAppFramework error: "
		   << "unable to convert SZG_NAV/translation_speed.\n";
      }
    }
    initStream << "arSZGAppFramework remark: "
	       << "setting translation speed to " << speed << endl;
    setNavTransSpeed( speed*_unitConversion );
  }
  if ((___firstNavLoad)||_paramNotOwned( "rotation_speed" )) {
    speed = 30.;
    temp = _SZGClient.getAttribute("SZG_NAV", "rotation_speed");
    if (temp != "NULL") {
      if (!ar_stringToFloatValid( temp, speed )) {
        initStream << "arSZGAppFramework error: "
		   << "unable to convert SZG_NAV/rotation_speed.\n";
      }
    }
    initStream << "arSZGAppFramework remark: "
	       << "setting rotation speed to " << speed << endl;
    setNavRotSpeed( speed );
  }
  arInputEventType theType;
  unsigned int index = 1;
  float threshold;
  if ((___firstNavLoad)||_paramNotOwned( "x_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "x_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold,
                                initStream )) {
        setNavTransCondition( 'x', theType, index, threshold );
        initStream << "arSZGAppFramework remark: "
		   << "set x_translation condition to "
                   << temp << endl;
      } else {
        setNavTransCondition( 'x', AR_EVENT_AXIS, 0, 0.2 );      
        initStream << "arSZGAppFramework error: "
		   << "failed to read SZG_NAV/x_translation.\n"
                   << "   defaulting to axis/0/0.2.\n";
      }
    } else {
      setNavTransCondition( 'x', AR_EVENT_AXIS, 0, 0.2 );      
      initStream << "arSZGAppFramework remark: "
		 << "SZG_NAV/x_translation defaulting to axis/0/0.2.\n";
    }
  }
  if ((___firstNavLoad)||_paramNotOwned( "z_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "z_translation");
    if (temp != "NULL") {
      if (_parseNavParamString( temp, theType, index, threshold,
                                initStream  )) {
        setNavTransCondition( 'z', theType, index, threshold );
        initStream << "arSZGAppFramework remark: "
		   << "set z_translation condition to "
                   << temp << endl;
      } else {
        setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );      
        initStream << "arSZGAppFramework error: "
		   << "failed to read SZG_NAV/z_translation.\n"
                   << "   defaulting to axis/1/0.2.\n";
      }
    } else {
      setNavTransCondition( 'z', AR_EVENT_AXIS, 1, 0.2 );      
      initStream << "arSZGAppFramework remark: "
		 << "SZG_NAV/z_translation defaulting to axis/1/0.2.\n";
    }
  }
  if ((___firstNavLoad)||_paramNotOwned( "y_translation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "y_translation");
    if (temp != "NULL") {
      if (!_parseNavParamString( temp, theType, index, threshold,
                                 initStream  )) {
        initStream << "arSZGAppFramework error: "
		   << "failed to read SZG_NAV/y_translation.\n";
      } else {
        setNavTransCondition( 'y', theType, index, threshold );
        initStream << "arSZGAppFramework remark: "
		   << "set y_translation condition to "
                   << temp << endl;
      }
    }
  }
  if ((___firstNavLoad)||_paramNotOwned( "y_rotation" )) {
    temp = _SZGClient.getAttribute("SZG_NAV", "y_rotation");
    if (temp != "NULL") {
      if (!_parseNavParamString( temp, theType, index, threshold,
                                 initStream )) {
        initStream << "arSZGAppFramework error: "
		   << "failed to read SZG_NAV/y_rotation.\n";
      } else {
        setNavRotCondition( 'y', theType, index, threshold );
        initStream << "arSZGAppFramework remark: "
		   << "set y_rotation condition to "
                   << temp << endl;
      }
    }
  }
  ___firstNavLoad = false;
}

bool arSZGAppFramework::_parseNavParamString( const string& theString,
                                              arInputEventType& theType,
                                              unsigned int& index,
                                              float& threshold,
                                              stringstream& initStream ) {
  int ind;
  std::vector<std::string> params;
  if (!ar_getTokenList( theString, params, '/' )) {
    initStream << "arSZGAppFramework error: failed to parse SZG_NAV string "
               << theString << endl;
    return false;
  }
  if (params.size() != 3) {
    initStream << "arSZGAppFramework error: "
	       << "SZG_NAV string must contain 3 items,\n"
               << "    event_type(string)/index(unsigned int)"
	       << "/threshold(float).\n";
    return false;
  }
  if (!ar_stringToIntValid( params[1], ind ) ||
      !ar_stringToFloatValid( params[2], threshold )) {
    initStream << "arSZGAppFramework error: "
	       << "SZG_NAV string conversion failed\n"
               << "    (value = " << theString << ").\n";
    return false;
  }
  if (params[0] == "axis")
    theType = AR_EVENT_AXIS;
  else if (params[0] == "button")
    theType = AR_EVENT_BUTTON;
  else {
    initStream << "arSZGAppFramework error: SZG_NAV string invalid event type "
               << params[0] << endl;
    return false;
  }
  if (ind < 0) {
    initStream << "arSZGAppFramework error: SZG_NAV index field (" 
               << ind << ") " << "< 0.\n";
    return false;
  }
  index = (unsigned int)ind;
  return true;
}    

bool arSZGAppFramework::_paramNotOwned( const std::string& theString ) {
  return _ownedParams.find( theString ) == _ownedParams.end();
}