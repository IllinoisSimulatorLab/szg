//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"

#include "arGlut.h"
#include "arGraphicsHeader.h"
#include "arInputSimulator.h"
#include "arLogStream.h"
#include "arSTLalgo.h"
#include "arSZGClient.h"

// Bug (in visual studio 7?): global static const arMatrix4's are all zeros
// during arInputSimulator's constructor, but fine later.
// Static, or const static, class variables fail similarly.
// const arMatrix4 arInputSimulator::_mHeadReset(ar_TM(0,5,0));
// const arMatrix4 arInputSimulator::_mWandReset(ar_TM(2,3,-1));

const int numberButtonEventsDefault = 8;

arInputSimulator::arInputSimulator() :
  _fInit(false),
  _xMouse(0),
  _yMouse(0),
  _numButtonEvents(0),
  _buttonSelector(0),
  _interfaceState(AR_SIM_HEAD_TRANSLATE),
  _head(arVector3(0,5,0)),
  _wand(arVector3(2,3,-1)),
  _axis0(0.),
  _axis1(0.)
    {
  vector<unsigned> buttons;
  buttons.push_back(0);
  buttons.push_back(2);
  if (!setMouseButtons( buttons ))
    ar_log_error() << "arInputSimulator constructor failed to setMouseButtons.\n";
  setNumberButtonEvents( numberButtonEventsDefault );
}

bool arInputSimulator::configure( arSZGClient& SZGClient ) {
  arSlashString mouseButtonString(SZGClient.getAttribute( "SZG_INPUTSIM", "mouse_buttons" ));
  if (mouseButtonString == "NULL") {
    ar_log_warning() << "arInputSimulator: default SZG_INPUTSIM/mouse_buttons.\n";
  } else {
    const int numItems = mouseButtonString.size();
    int* mouseButtons = new int[numItems];
    if (!mouseButtons) {
      ar_log_error() << "arInputSimulator out of memory.\n";
      return false;
    }
    const int numValues = ar_parseIntString( mouseButtonString, mouseButtons, numItems );
    if (numValues != numItems) {
      ar_log_warning() << "arInputSimulator SZG_INPUTSIM/mouse_buttons defaulting to 0 and 2 for left and right.\n";
    } else {
      vector<unsigned> v(mouseButtons, mouseButtons+numValues);
      if (!setMouseButtons( v )) {
        ar_log_error() << "arInputSimulator configure() failed to setMouseButtons.\n";
      }
    }
    delete [] mouseButtons;
  }

  const int n = SZGClient.getAttributeInt( "SZG_INPUTSIM", "number_button_events" );
  if (n == 0) {
    ar_log_warning() << "arInputSimulator: SZG_INPUTSIM/number_button_events defaulting to " <<
      numberButtonEventsDefault << ".\n";
    return false;
  }
  if (n < 2) {
    ar_log_warning() << "arInputSimulator: SZG_INPUTSIM/number_button_events " << n <<
      " < 2, so defaulting to " << numberButtonEventsDefault << ".\n";
    return false;
  }
  setNumberButtonEvents( unsigned(n) );
  return true;
}

typedef map<unsigned,int>::const_iterator IterButton;

bool arInputSimulator::setMouseButtons( vector<unsigned>& mouseButtons ) {
  map<unsigned,int> buttonsPrev = _mouseButtons;
  _mouseButtons.clear();
  vector<unsigned>::iterator buttonIter;
  for (buttonIter = mouseButtons.begin(); buttonIter != mouseButtons.end(); ++buttonIter) {
    if (_mouseButtons.find( *buttonIter ) != _mouseButtons.end()) {
      ar_log_error() << "arInputSimulator: duplicate mouse button indices in setMouseButtons().\n";
      _mouseButtons = buttonsPrev;
      return false;
    }
    IterButton i = buttonsPrev.find(*buttonIter);
    _mouseButtons[*buttonIter] = (i == buttonsPrev.end()) ? 0 : i->second;
  }
  if ((_mouseButtons.find(0) == _mouseButtons.end())||(_mouseButtons.find(2) == _mouseButtons.end())) {
    ar_log_error() << "arInputSimulator: mouse buttons must include 0 (left) and 2 (right).\n";
    _mouseButtons = buttonsPrev;
    return false;
  }
  return true;
}

vector<unsigned> arInputSimulator::getMouseButtons() const {
  vector<unsigned> buttons;
  for (IterButton i = _mouseButtons.begin(); i != _mouseButtons.end(); ++i) {
    buttons.push_back( i->first );
  }
  return buttons;
}

void arInputSimulator::setNumberButtonEvents( unsigned numButtonEvents ) {
  if (numButtonEvents > _numButtonEvents) {
    const unsigned diff = numButtonEvents - _numButtonEvents;
    _lastButtonEvents.insert( _lastButtonEvents.end(), diff, 0 );
    _newButtonEvents.insert( _newButtonEvents.end(), diff, 0 );
  } else if (numButtonEvents < _numButtonEvents) {
    const unsigned diff = _numButtonEvents - numButtonEvents;
    _lastButtonEvents.erase( _lastButtonEvents.end()-diff, _lastButtonEvents.end() );
    _newButtonEvents.erase( _newButtonEvents.end()-diff, _newButtonEvents.end() );
  }
  _numButtonEvents = numButtonEvents;
  _driver.setSignature(_numButtonEvents,2,2);
  _buttonLabels.clear();
  for (unsigned i=0; i<_numButtonEvents; ++i) {
    _buttonLabels.push_back( char((i % 10) + '0') );
  }
}

arInputSimulator::~arInputSimulator(){
  _mouseButtons.clear();
  _lastButtonEvents.clear();
  _newButtonEvents.clear();
}

// Connect to an arInputNode.
void arInputSimulator::registerInputNode(arInputNode* node){
  node->addInputSource(&_driver, false);
}

// Draw current state, either standalone or as an overlay.
void arInputSimulator::draw() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-0.1,0.1,-0.1,0.1,0.3,1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,5,23,0,5,9,0,1,0);
  glTranslatef(0,5,0);
  glMultMatrixf(arMatrix4(_box).v);
  glTranslatef(0,-5,0);


  glPushMatrix();
    glTranslatef(0,5,0);
    _wireCube(10);
  glPopMatrix();

  glLineWidth(1.);
  _drawHead();
  _drawWand();
  _drawGamepad();
  _drawTextState();
}

// Overlay the display.  Not const because pre,postComposition can't be.
void arInputSimulator::drawWithComposition() {
  // Lower right corner.
  preComposition(0.66, 0, 0.33, 0.33);
  draw();
  postComposition();
}

// Called occasionally to post the current state of the simulated
// device to newly connected arInputNodes.
void arInputSimulator::advance(){
  // We should send only data that changed,
  // but with an exception to send everything every few seconds for
  // clients who join late.
  //
  // TODO: Handle this at a lower level.
  // When a remote client connects to the input device,
  // it needs to get the current state.
  _driver.queueMatrix(0, arMatrix4(_head));
  _driver.queueMatrix(1, arMatrix4(_wand));
  _driver.queueAxis(0, _axis0);
  _driver.queueAxis(1, _axis1);

  if (_newButtonEvents.size() != _lastButtonEvents.size()) {
    ar_log_error() << "arInputSimulator: numbers of new & last button values out of sync.\n";
    return;
  }

  for (unsigned i=0; i<_newButtonEvents.size(); ++i) {
    if (_newButtonEvents[i] != _lastButtonEvents[i]){
      _lastButtonEvents[i] = _newButtonEvents[i];
      // Queue the button events where received, i.e. in
      // mouseButton() instead of sending the diff here
      // (i.e. polling, which is unreliable for transient events).
    }
  }
  _driver.sendQueue();
}

// Process keyboard events.
void arInputSimulator::keyboard(unsigned char key, int, int /*x*/, int /*y*/) {
  // Use x,y only in mouseButton, not here - they may be invalid.
  // See arMasterSlaveFramework.cpp .

  for (vector<int>::iterator i = _lastButtonEvents.begin(); i != _lastButtonEvents.end(); ++i)
    *i = 0;

  // Ugly: clicking middle mouse button of a 3-button mouse suddenly
  // changes rowLength from 2 to 3 (_mouseButtons grew).
  // Todo: query the OS (platform specific) for number of mouse buttons.
  const unsigned rowLength = _mouseButtons.size();
  unsigned numRows = unsigned(_numButtonEvents) / rowLength;
  if (_numButtonEvents % rowLength != 0)
    ++numRows;

  if (key < '1' || key > '9') {
    switch (key) {
    case ' ':
      {
      if (rowLength == 0 || numRows == 0) {
	ar_log_error() << "arInputSimulator keyboard has no buttons.\n";
	return;
      }
      // "Feature:" any buttons held down (_newButtonEvents[i] != 0) will STAY down
      // while spacebar cycles to the next subset of buttons.
      if (++_buttonSelector >= numRows)
	_buttonSelector = 0;
      return;
      }
    }
    return;
  }

  const bool fReset = _interfaceState == key - '1';
  _interfaceState = arHeadWandSimState(key - '1');
  if (fReset) {
    // Same keystroke twice in a row ("double-click").  Reset that attribute.
    switch (_interfaceState) {
    case AR_SIM_USE_JOYSTICK:
      _axis0 = _axis1 = 0.;
      break;

    case AR_SIM_HEAD_TRANSLATE:
      _head.resetPos();
      break;
    case AR_SIM_WAND_TRANSLATE_HORIZONTAL:
    case AR_SIM_WAND_TRANSLATE_VERTICAL:
      _wand.resetPos();
      break;

    case AR_SIM_HEAD_ROTATE:
      _head.setAziEle(0,0);
      break;
    case AR_SIM_WAND_ROTATE:
      _wand.setAziEle(0,0);
      break;
    case AR_SIM_BOX_ROTATE:
      _box.setAziEle(0,0);
      break;

    case AR_SIM_HEAD_ROLL:
      _head.setRoll(0);
      break;
    case AR_SIM_WAND_ROLL:
      _wand.setRoll(0);
      break;
    }
  }
}

// Process mouse button events.
void arInputSimulator::mouseButton(int button, int state, int x, int y){
  _xMouse = x;
  _yMouse = y;

  if (button < 0) {
    ar_log_error() << "arInputSimulator ignoring negative button index.\n";
    return;
  }
  unsigned buttonIndex = (unsigned)button;
  IterButton iFind = _mouseButtons.find( buttonIndex );
  const bool haveIndex = iFind != _mouseButtons.end();
  if (haveIndex)
    _mouseButtons[buttonIndex] = state;
  else {
    vector<unsigned> buttons(getMouseButtons());
    buttons.push_back( buttonIndex );
    if (setMouseButtons( buttons )) {
      ar_log_remark() << "arInputSimulator added button index " << buttonIndex << " to list.\n";
      return;
    }
    ar_log_error() << "arInputSimulator failed to add button index " << buttonIndex << ".\n";
  }

  switch (_interfaceState) {
  case AR_SIM_HEAD_ROTATE:
  case AR_SIM_WAND_ROTATE:
  case AR_SIM_WAND_TRANSLATE_VERTICAL:
  case AR_SIM_WAND_TRANSLATE_HORIZONTAL:
  case AR_SIM_BOX_ROTATE:
  case AR_SIM_HEAD_ROLL:
  case AR_SIM_WAND_ROLL:
    // Change only _newButton here, to send only the button event diff.
    if (haveIndex) {
      unsigned buttonOffset = 0;
      for (IterButton i = _mouseButtons.begin(); i != iFind && i != _mouseButtons.end(); ++i) {
        ++buttonOffset;
      }
      const unsigned eventIndex = _mouseButtons.size() * _buttonSelector + buttonOffset;
      if (eventIndex >= _newButtonEvents.size()) {
        ar_log_error() << "arInputSimulator ignoring out-of-range button event index " <<
	  eventIndex << ".\n";
        return;
      }
      _newButtonEvents[eventIndex] = state;
      // Queue the button events here, instead of advance().
      _driver.queueButton(eventIndex, state);
    }
    break;
  case AR_SIM_USE_JOYSTICK:
    // "Auto-centering."  Reset on button-up.  (But not button-down?)
    _axis0 = _axis1 = 0.;
    break;
  case AR_SIM_HEAD_TRANSLATE:
    // do nothing
    break;
  }
}

static inline float clamp(const float x, const float xMin, const float xMax) {
  return x<xMin ? xMin : x>xMax ? xMax : x;
}

// The mouse moved.
void arInputSimulator::mousePosition(int x, int y){
  if (!_fInit) {
    _fInit = true;
    _xMouse = x;
    _yMouse = y;
  }

  const float gain = 0.03;
  const float gainRot = 1.2;
  const float gainRoll = 0.5;
  const float gainJoystick = 0.4;
  const float dx = gain * (x - _xMouse);
  const float dy = gain * (y - _yMouse);
  _xMouse = x;
  _yMouse = y;
  const bool leftButton   = (_mouseButtons.find(0) != _mouseButtons.end()) && _mouseButtons[0];
  const bool middleButton = (_mouseButtons.find(1) != _mouseButtons.end()) && _mouseButtons[1];
  const bool rightButton  = (_mouseButtons.find(2) != _mouseButtons.end()) && _mouseButtons[2];
  const arVector3 vVertical(dx, -dy, 0);
  const arVector3 vHorizontal(dx, 0, dy);

  switch (_interfaceState) {

  case AR_SIM_HEAD_TRANSLATE:
    if (leftButton && rightButton) {
      // dx would happen twice, translating left-right twice as fast.
      // Just ignore this silly case.
      break;
    }
    if (leftButton)
      _head.translate(vVertical);
    if (rightButton)
      _head.translate(vHorizontal);
    break;

  case AR_SIM_WAND_TRANSLATE_HORIZONTAL:
    _wand.translate(vHorizontal);
    break;
  case AR_SIM_WAND_TRANSLATE_VERTICAL:
    _wand.translate(vVertical);
    break;

  case AR_SIM_HEAD_ROTATE:
    _head.rotate(-dx, -dy, 0);
    break;
  case AR_SIM_WAND_ROTATE:
    _wand.rotate(-dx*gainRot, -dy*gainRot, 0);
    break;
  case AR_SIM_BOX_ROTATE:
    _box.rotate(-dx*gainRot, -dy*gainRot, 0);
    break;

  case AR_SIM_HEAD_ROLL:
    _head.rotate(0, 0, -dx*gainRoll);
    break;
  case AR_SIM_WAND_ROLL:
    _wand.rotate(0, 0, -dx*gainRoll);
    break;

  case AR_SIM_USE_JOYSTICK:
    if (leftButton || middleButton || rightButton) {
      _axis0 = clamp(_axis0 + dx*gainJoystick, -1, 1);
      _axis1 = clamp(_axis1 - dy*gainJoystick, -1, 1);
    }
    break;

  }

  // Bug: a clever sequence of rotate-roll-rotate-roll can leave the
  // wand pointing forwards-and-up, but with reversed mouse-y control.
  // Is there a way to "normalize" azi/ele/roll to avoid this?
  //
  // The workaround is easy enough: doubleclick-reset both rotation and roll.
}

void arInputSimulator::_wireCube(const float size) const {
  glPushMatrix();
  glScalef(size/2,size/2,size/2);

  glLineWidth(1.);
  // Pale blue sky.
  glColor3f(.6,.8,.8);
  glBegin(GL_LINES);
    glVertex3f( 1, 1,-1); glVertex3f( 1,-1,-1);
    glVertex3f(-1,-1,-1); glVertex3f(-1, 1,-1);
    glVertex3f(-1, 1,-1); glVertex3f( 1, 1,-1);
    glVertex3f( 1, 1, 1); glVertex3f( 1,-1, 1);
    glVertex3f(-1,-1, 1); glVertex3f(-1, 1, 1);
    glVertex3f(-1, 1, 1); glVertex3f( 1, 1, 1);
    glVertex3f( 1, 1, 1); glVertex3f( 1, 1,-1);
    glVertex3f(-1, 1, 1); glVertex3f(-1, 1,-1);
  glEnd();

  // Green floor.
  glLineWidth(2.);
  glColor3f(0,1,0);
  glBegin(GL_LINE_LOOP);
    glVertex3f( 1,-1,-1);
    glVertex3f( 1,-1, 1);
    glVertex3f(-1,-1, 1);
    glVertex3f(-1,-1,-1);
  glEnd();

  glPopMatrix();
}

void arInputSimulator::_drawGamepad() const {
  const unsigned rowLength = _mouseButtons.size();
  unsigned numRows = unsigned(_numButtonEvents) / rowLength;
  if (_numButtonEvents % rowLength != 0)
    ++numRows;
  if (rowLength == 0 || numRows == 0) {
    ar_log_error() << "arInputSimulator: no buttons.\n";
    return;
  }

  const float GAMEPAD_YOFFSET = -.8;
  const float GAMEPAD_WIDTH = 1.;
  const float BUTTON_SPACING = 1.1;
  const float BUTTON_YBORDER = .8;
  const float padWidth = max(1.4F, rowLength*BUTTON_SPACING);
  const float padHeight = GAMEPAD_WIDTH + numRows*BUTTON_SPACING;

  glPushMatrix();
    glMultMatrixf(ar_translationMatrix(4,0,5).v);

    // Blue background for the wand.
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f(0,0,1, .15);
    const float zPad = .01;
    glBegin(GL_QUADS);
      glVertex3f(-.5*padWidth, GAMEPAD_YOFFSET + padHeight, zPad);
      glVertex3f( .5*padWidth, GAMEPAD_YOFFSET + padHeight, zPad);
      glVertex3f( .5*padWidth, GAMEPAD_YOFFSET, zPad);
      glVertex3f(-.5*padWidth, GAMEPAD_YOFFSET, zPad);
    glEnd();
    glDisable( GL_BLEND );

    // Marker for the joystick.
    glPushMatrix();
      glTranslatef(0.5*_axis0, -0.3 + 0.5*_axis1, zPad);
      glColor3f(0.3,1,0);
      glutSolidSphere(0.25,12,12);
    glPopMatrix();

    // Joystick's boundary.
    glBegin(GL_LINE_STRIP);
      glColor3f(1,1,1);
      glVertex3f(-0.5,0.2,0.3);
      glVertex3f(-0.5,-0.8,0.3);
      glVertex3f(0.5,-0.8,0.3);
      glVertex3f(0.5,0.2,0.3);
      glVertex3f(-0.5,0.2,0.3);
    glEnd();

    // Indicate which row of buttons is controlled by the mouse buttons
    glPushMatrix();
      glTranslatef(-.5*padWidth,BUTTON_YBORDER+BUTTON_SPACING*_buttonSelector, zPad);
      glColor3f(1,1,1);
      glutSolidSphere(0.15,8,8);
    glPopMatrix();
    const float xlo = (-.5*((float)rowLength) + .5) * BUTTON_SPACING;
    float x = xlo;
    float y = BUTTON_YBORDER;
    unsigned rowCount = 0;
    const float labelScale = .004*BUTTON_SPACING;
    for (unsigned i = 0; i<_newButtonEvents.size(); ++i) {
      glPushMatrix();
        glTranslatef(x,y,0.1);
        if (_newButtonEvents[i] == 0)
          glColor3f(0.8,0,0);
        else
          glColor3f(0,0.6,0);
        glutSolidSphere(.4*BUTTON_SPACING,12,12);
        if (i >= _buttonLabels.size()) {
          ar_log_error() << "arInputSimulator: too few button labels.\n";
        } else {
          glDisable(GL_DEPTH_TEST);
          glColor3f( 1, 1, 1 );
          glPushMatrix();
            glTranslatef( -.25*BUTTON_SPACING, -.25*BUTTON_SPACING, 0. );
            glScalef( labelScale, labelScale, labelScale );
            glutStrokeCharacter( GLUT_STROKE_MONO_ROMAN, _buttonLabels[i] );
          glPopMatrix();
          glEnable(GL_DEPTH_TEST);
        }
      glPopMatrix();
      x += BUTTON_SPACING;
      ++rowCount;
      if (rowCount % rowLength == 0) {
        rowCount = 0;
        x = xlo;
        y += BUTTON_SPACING;
      }
    }
    glEnable(GL_LIGHTING);
  glPopMatrix();
}

void arInputSimulator::_drawHead() const {
  glColor3f(1,1,0);
  glPushMatrix();
    glMultMatrixf(arMatrix4(_head).v);
    glutWireSphere(1,10,10);
    // two eyes
    glColor3f(0,1,1);
    glPushMatrix();
      glTranslatef(0.5,0,-0.8);
      glutSolidSphere(0.4,8,8);
      glTranslatef(0,0,-0.4);
      glColor3f(1,0,0);
      glutSolidSphere(0.15,5,5);
    glPopMatrix();
    glTranslatef(-0.5,0,-0.8);
    glColor3f(0,1,1);
    glutSolidSphere(0.4,8,8);
    glTranslatef(0,0,-0.4);
    glColor3f(1,0,0);
    glutSolidSphere(0.15,5,5);
  glPopMatrix();
}

void arInputSimulator::_drawWand() const {
  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glPushMatrix();
    glMultMatrixf(arMatrix4(_wand).v);

    // fwd-aft bar
    glColor3f(1,0,0);
    glPushMatrix();
      glScalef(0.17,0.17,2);
      glutSolidCube(1);
    glPopMatrix();

    // ball at tip
    glColor3f(1,0,1);
    glPushMatrix();
      glTranslatef(0,0,-1);
      glutSolidSphere(0.4,10,10);
    glPopMatrix();

    // up-down bar
    glColor3f(0,1,.15);
    glPushMatrix();
      glScalef(0.17,2,0.17);
      glutSolidCube(1);
    glPopMatrix();

    // ball at tip
    glColor3f(0,1,.6);
    glPushMatrix();
      glTranslatef(0,1,0);
      glutSolidSphere(0.2,10,10);
    glPopMatrix();

    // left-right bar
    glColor3f(0,0,.8);
    glPushMatrix();
      glScalef(2,0.17,0.17);
      glutSolidCube(1);
    glPopMatrix();

  glPopMatrix();
}

void arInputSimulator::_drawTextState() const {
  // One-line hints.
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glColor3f(0.2,0.2,0.5);
  glBegin(GL_QUADS);
  glVertex3f(-1, .88, 0.00001);
  glVertex3f( 1, .88, 0.00001);
  glVertex3f( 1, 1,   0.00001);
  glVertex3f(-1, 1,   0.00001);
  glEnd();

  // Same order as enum arHeadWandSimState.
  const char* const hint[9] = {
      "1: Move head (L or R button)",
      "2: Aim head",
      "3: Move wand horizontal",
      "4: Move wand vertical",
      "5: Aim wand",
      "6: Joystick (any button)",
      "7: Turn box",
      "8: Roll head",
      "9: Roll wand"
    };
  glColor3f(1,1,1);
  glPushMatrix();
    glTranslatef(-0.95,0.91,0.000001);
    glScalef(0.0006,0.0006,0.0006);
    for (const char* pch=hint[_interfaceState]; *pch; ++pch)
      glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *pch);
  glPopMatrix();
  glEnable(GL_DEPTH_TEST);
}
