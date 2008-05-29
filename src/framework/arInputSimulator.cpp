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

arInputSimulator::arInputSimulator() :
  _fInit(false),
  _numButtonEvents(0),
  _rotSim(0.),
  _buttonSelector(0),
  _interfaceState(AR_SIM_HEAD_TRANSLATE) {
  _mouseXY[0] = 0;
  _mouseXY[1] = 0;
  vector<unsigned> buttons;
  buttons.push_back(0);
  buttons.push_back(2);
  if (!setMouseButtons( buttons ))
    ar_log_error() << "arInputSimulator setMouseButtons() failed in constructor.\n";
  setNumberButtonEvents( 8 );

  // Reset the state of the simulated trackers+wand device.
  _mHead = _mHeadReset = ar_TM(0,5,0);
  _mWand = _mWandReset = ar_TM(2,3,-1);
  _axis[0] = _axis[1] = 0;
}

bool arInputSimulator::configure( arSZGClient& SZGClient ) {
  arSlashString mouseButtonString(SZGClient.getAttribute( "SZG_INPUTSIM", "mouse_buttons" ));
  if (mouseButtonString != "NULL") {
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
        ar_log_error() << "arInputSimulator setMouseButtons() failed in configure().\n";
      }
    }
    delete[] mouseButtons;
  } else {
    ar_log_warning() << "SZG_INPUTSIM/mouse_buttons undefined, using defaults.\n";
  }

  const int numButtonEvents =
    SZGClient.getAttributeInt( "SZG_INPUTSIM", "number_button_events" );
  if (numButtonEvents < 2) {
    ar_log_warning() << "SZG_INPUTSIM/number_button_events " << numButtonEvents <<
      " < 2, so defaulting to 8.\n";
    return false;
  }
  setNumberButtonEvents( unsigned(numButtonEvents) );
  return true;
}

typedef map<unsigned,int>::const_iterator IterButton;

bool arInputSimulator::setMouseButtons( vector<unsigned>& mouseButtons ) {
  map<unsigned,int> buttonsPrev = _mouseButtons;
  _mouseButtons.clear();
  vector<unsigned>::iterator buttonIter;
  for (buttonIter = mouseButtons.begin(); buttonIter != mouseButtons.end(); ++buttonIter) {
    if (_mouseButtons.find( *buttonIter ) != _mouseButtons.end()) {
      ar_log_error() << "duplicate mouse button indices in setMouseButtons().\n";
      _mouseButtons = buttonsPrev;
      return false;
    }
    IterButton i = buttonsPrev.find(*buttonIter);
    _mouseButtons[*buttonIter] = (i == buttonsPrev.end()) ? 0 : i->second;
  }
  if ((_mouseButtons.find(0) == _mouseButtons.end())||(_mouseButtons.find(2) == _mouseButtons.end())) {
    ar_log_error() << "mouse buttons must include 0 (left) and 2 (right).\n";
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

// Draw current state, standalone or as an overlay.
void arInputSimulator::draw() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-0.1,0.1,-0.1,0.1,0.3,1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,5,23,0,5,9,0,1,0);
  glMultMatrixf(ar_RM('y', _rotSim).v);

  // wireframe cube
  glLineWidth(1.); // in case embedding app changed this
  glColor3f(1,1,1);
  glPushMatrix();
  glTranslatef(0,5,0);
  _wireCube(10);
  glPopMatrix();

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

// Called from time to time to post the current state of the simulated
// device to any arInputNode that has been connected.
void arInputSimulator::advance(){
  // We should send only data that changed,
  // but with an exception to send everything every few seconds for
  // clients who join late. TODO: This needs to be handled at a lower
  // level. When a remote client connects to the input device, it needs
  // to have the current state transfered to it...
  _driver.queueMatrix(0,_mHead);
  _driver.queueMatrix(1,_mWand);
  _driver.queueAxis(0,_axis[0]);
  _driver.queueAxis(1,_axis[1]);

  if (_newButtonEvents.size() != _lastButtonEvents.size()) {
    ar_log_error() << "numbers of new & last button values out of sync.\n";
    return;
  }

  for (unsigned i=0; i<_newButtonEvents.size(); ++i) {
    if (_newButtonEvents[i] != _lastButtonEvents[i]){
      _lastButtonEvents[i] = _newButtonEvents[i];
      // Queue the button events where received, i.e. in the 
      // mouseButton method instead of sending the diff here
      // (i.e. polling, which is unreliable for transient events).
    }
  }
  _driver.sendQueue();
}

// Process keyboard events to drive the simulated interface.
void arInputSimulator::keyboard(unsigned char key, int, int /*x*/, int /*y*/) {
  // Don't assign x,y to _mouseXY[0,1].  x,y may be invalid.
  // See arMasterSlaveFramework.cpp

  for (vector<int>::iterator i = _lastButtonEvents.begin(); i != _lastButtonEvents.end(); ++i)
    *i = 0;

  // Ugly: clicking middle mouse button of a 3-button mouse suddenly
  // changes rowLength from 2 to 3 (_mouseButtons grew).
  // Todo: query the OS (platform specific) for number of mouse buttons.
  const unsigned rowLength = _mouseButtons.size();
  unsigned numRows = unsigned(_numButtonEvents) / rowLength;
  if (_numButtonEvents % rowLength != 0)
    ++numRows;

  // Same keystroke twice in a row ("double-click").  Reset that attribute.
  const bool fReset = _interfaceState == key - '1';

  switch (key) {
  case ' ':
    {
    if (rowLength == 0 || numRows == 0) {
      ar_log_error() << "no buttons in keyboard().\n";
      return;
    }
    // "Feature:" any buttons held down (_newButtonEvents[i] != 0) will STAY down
    // while spacebar cycles to the next subset of buttons.
    if (++_buttonSelector >= numRows)
      _buttonSelector = 0;
    return;
    }

  case '1' + AR_SIM_WAND_ROTATE_BUTTONS:
    if (fReset) {
      _mWand = ar_ETM(_mWand);
    }
    break;

  case '1' + AR_SIM_USE_JOYSTICK:
    if (fReset) {
      _axis[0] = _axis[1] = 0.;
    }
    break;

  case '1' + AR_SIM_WAND_ROLL_BUTTONS:
    /*
    if (fReset) {
      extract only the roll component, and zero that.
      equivalently, extract the first two components and ignore the roll component.
      _mWand = ar_ETM(_mWand) * ... ;
    }
    */
    break;

  case '1' + AR_SIM_SIMULATOR_ROTATE_BUTTONS:
    if (fReset) {
      _rotSim = 0.;
    }
    break;

  case '1' + AR_SIM_HEAD_ROTATE_BUTTONS:
    if (fReset) {
      _mHead = ar_ETM(_mHead);
    }
    break;

  case '1' + AR_SIM_HEAD_TRANSLATE:
    if (fReset) {
      _mHead = _mHeadReset * ar_ERM(_mHead);
    }
    break;

  case '1' + AR_SIM_WAND_TRANSLATE:
  case '1' + AR_SIM_WAND_TRANS_BUTTONS:
    if (fReset) {
      _mWand = _mWandReset * ar_ERM(_mWand);
    }
    break;
  }

  if (key < '1')
    ar_log_error() << "arInputSimulator internal error.\n";
  _interfaceState = arHeadWandSimState(key - '1');
}

// Process mouse button events.
void arInputSimulator::mouseButton(int button, int state, int x, int y){
  _mouseXY[0] = x;
  _mouseXY[1] = y;

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
  case AR_SIM_HEAD_ROTATE_BUTTONS:
  case AR_SIM_WAND_ROTATE_BUTTONS:
  case AR_SIM_WAND_TRANS_BUTTONS:
  case AR_SIM_SIMULATOR_ROTATE_BUTTONS:
  case AR_SIM_WAND_ROLL_BUTTONS:
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
    _axis[0] = _axis[1] = 0.;
    break;
  case AR_SIM_HEAD_TRANSLATE:
  case AR_SIM_WAND_TRANSLATE:
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
    _mouseXY[0] = x;
    _mouseXY[1] = y;
  }

  const float gain = 0.03;
  const float gainRotWand = 1.3;
  const float gainRollWand = 0.7;
  const float dx = gain * (x - _mouseXY[0]);
  const float dy = gain * (y - _mouseXY[1]);
  _mouseXY[0] = x;
  _mouseXY[1] = y;
  const bool leftButton   = (_mouseButtons.find(0) != _mouseButtons.end()) && _mouseButtons[0];
  const bool middleButton = (_mouseButtons.find(1) != _mouseButtons.end()) && _mouseButtons[1];
  const bool rightButton  = (_mouseButtons.find(2) != _mouseButtons.end()) && _mouseButtons[2];
  const arMatrix4 mSideways(ar_TM(dx, -dy, 0));
  const arMatrix4 mForwards(ar_TM(dx, 0, dy));

  switch (_interfaceState) {

  case AR_SIM_HEAD_TRANSLATE:
    if (leftButton && rightButton) {
      // dx would happen twice, translating left-right twice as fast.
      // Just ignore this silly case.
      break;
    }
    if (leftButton)
      _mHead = mSideways * _mHead;
    if (rightButton)
      _mHead = mForwards * _mHead;
    break;

  case AR_SIM_WAND_TRANSLATE:
    if (leftButton && rightButton)
      break;
    if (leftButton)
      _mWand = mSideways * _mWand;
    if (rightButton)
      _mWand = mForwards * _mWand;
    break;

  case AR_SIM_WAND_TRANS_BUTTONS:
    _mWand = mSideways * _mWand;
    break;

  case AR_SIM_HEAD_ROTATE_BUTTONS:
    {
    const arMatrix4 mRot(ar_ERM(_mHead));
    const arVector3 aRot(mRot * arVector3(1,0,0));
    _mHead =
      ar_ETM(_mHead) *
      ar_RM('y',  -dx) *
      ar_RM(aRot, -dy) *
      mRot;
    break;
    }

  case AR_SIM_WAND_ROTATE_BUTTONS:
    {
    const arMatrix4 mRot(ar_ERM(_mWand));
    const arVector3 aRot(mRot * arVector3(1,0,0));
    _mWand =
      ar_ETM(_mWand) *
      ar_RM('y',  -dx*gainRotWand) * // cave coords
      ar_RM(aRot, -dy*gainRotWand) * // wand coords
      mRot;
    // Bug:
    // if wand has rolled, dy rotates in wand's coords,
    // but dx still rotates in cave's coords.  Inconsistent and unusable.
    // Preferred solution: dy rotates in cave's coords.
    // Wrong solution: changing aRot to 'x' is cave coords, but breaks unrolled rotation.
    //
    // Is it intractable to extract rot and roll from just a matrix?
    // Instead, separately store translation and euler angles,
    // combining into a matrix only at the last minute?
    break;
    }

  case AR_SIM_WAND_ROLL_BUTTONS:
    {
    const arMatrix4 mRot(ar_ERM(_mWand));
    const arVector3 aRot(mRot * arVector3(0,0,1));
    _mWand = ar_ETM(_mWand) *
      ar_RM(aRot, dx*gainRollWand) *
      mRot;
    break;
    }

  case AR_SIM_SIMULATOR_ROTATE_BUTTONS:
    _rotSim += dx;
    if (_rotSim < -180.)
      _rotSim += 360.;
    if (_rotSim > 180.)
      _rotSim -= 360.;
    break;

  case AR_SIM_USE_JOYSTICK:
    if (leftButton || middleButton || rightButton) {
      const float gainJoystick = 0.4;
      _axis[0] = clamp(_axis[0] + gainJoystick*dx, -1, 1);
      _axis[1] = clamp(_axis[1] - gainJoystick*dy, -1, 1);
    }
    break;

  }
}

void arInputSimulator::_wireCube(const float size) const {
  glPushMatrix();
  glScalef(size/2,size/2,size/2);
  glBegin(GL_LINES);
    glVertex3f( 1, 1,-1); glVertex3f( 1,-1,-1);
    glVertex3f( 1,-1,-1); glVertex3f(-1,-1,-1);
    glVertex3f(-1,-1,-1); glVertex3f(-1, 1,-1);
    glVertex3f(-1, 1,-1); glVertex3f( 1, 1,-1);
    glVertex3f( 1, 1, 1); glVertex3f( 1,-1, 1);
    glVertex3f( 1,-1, 1); glVertex3f(-1,-1, 1);
    glVertex3f(-1,-1, 1); glVertex3f(-1, 1, 1);
    glVertex3f(-1, 1, 1); glVertex3f( 1, 1, 1);
    glVertex3f( 1, 1, 1); glVertex3f( 1, 1,-1);
    glVertex3f(-1, 1, 1); glVertex3f(-1, 1,-1);
    glVertex3f( 1,-1, 1); glVertex3f( 1,-1,-1);
    glVertex3f(-1,-1, 1); glVertex3f(-1,-1,-1);
  glEnd();
  glPopMatrix();
}

void arInputSimulator::_drawGamepad() const {
  const unsigned rowLength = _mouseButtons.size();
  unsigned numRows = unsigned(_numButtonEvents) / rowLength;
  if (_numButtonEvents % rowLength != 0)
    ++numRows;
  if (rowLength == 0 || numRows == 0) {
    ar_log_error() << "no buttons in _drawGamepad().\n";
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
    glColor4f(0,0,1,.25);
    glBegin(GL_QUADS);
      glVertex3f(-.5*padWidth, GAMEPAD_YOFFSET + padHeight, 0.1);
      glVertex3f( .5*padWidth, GAMEPAD_YOFFSET + padHeight, 0.1);
      glVertex3f( .5*padWidth, GAMEPAD_YOFFSET, 0.1);
      glVertex3f(-.5*padWidth, GAMEPAD_YOFFSET, 0.1);
    glEnd();
    glDisable( GL_BLEND );
    // Marker for the joystick.
    glPushMatrix();
      glTranslatef(0.5*_axis[0], -0.3 + 0.5*_axis[1], 0.1);
      glColor3f(0.3,1,0);
      glutSolidSphere(0.25,8,8);
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
      glTranslatef(-.5*padWidth,BUTTON_YBORDER+BUTTON_SPACING*_buttonSelector,0.1);
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
        glutSolidSphere(.4*BUTTON_SPACING,8,8);
        if (i >= _buttonLabels.size()) {
          ar_log_error() << "_buttonLabels too short in _drawGamepad().\n";
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
    glMultMatrixf(_mHead.v);
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
    glMultMatrixf(_mWand.v);
    glPushMatrix();
    glColor3f(0,1,0);
    glScalef(2,0.17,0.17);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glColor3f(0,0,1);
    glScalef(0.17,2,0.17);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glColor3f(1,0,0);
    glScalef(0.17,0.17,2);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0,0,-1);
    glColor3f(1,0,1);
    glutSolidSphere(0.4,10,10);
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
  glColor3f(0.5,0.5,0.8);
  glBegin(GL_QUADS);
  glVertex3f(-1, .88, 0.00001);
  glVertex3f( 1, .88, 0.00001);
  glVertex3f( 1, 1,   0.00001);
  glVertex3f(-1, 1,   0.00001);
  glEnd();

  const char* const hint[8] = {
      "1: Move  head",
      "2: Point head + buttons",
      "3: Move  wand",
      "4: Move  wand + buttons",
      "5: Point wand + buttons",
      "6: Joystick",
      "7: Turn sim   + buttons",
      "8: Roll wand  + buttons"
      // todo, when roll works with rotation: "8 roll wand, 9: Roll head"
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
