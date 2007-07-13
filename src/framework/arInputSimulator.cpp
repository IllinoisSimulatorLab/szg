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
    ar_log_warning() << "arInputSimulator setMouseButtons() failed in constructor.\n";
  setNumberButtonEvents( 8 );
  _rotWand[0] = 0.;
  _rotWand[1] = 0.;

  // Initialize the simulated device.
  _matrix[0] = ar_translationMatrix(0,5,0);
  _matrix[1] = ar_translationMatrix(2,3,-1);
  _axis[0] = 0;
  _axis[1] = 0;
}

bool arInputSimulator::configure( arSZGClient& SZGClient ) {
  arSlashString mouseButtonString(SZGClient.getAttribute( "SZG_INPUTSIM", "mouse_buttons" ));
  if (mouseButtonString != "NULL") {
    const int numItems = mouseButtonString.size();
    int* mouseButtons = new int[numItems];
    if (!mouseButtons) {
      ar_log_warning() << "arInputSimulator out of memory.\n";
      return false;
    }
    const int numValues = ar_parseIntString( mouseButtonString, mouseButtons, numItems );
    if (numValues != numItems) {
      ar_log_warning() << "arInputSimulator SZG_INPUTSIM/mouse_buttons defaulting to 0 and 2 for left and right.\n";
      delete[] mouseButtons;
    } else {
      vector<unsigned> mouseButtonsVector;
      for (int i=0; i<numValues; ++i) {
        mouseButtonsVector.push_back( mouseButtons[i] );
      }
      delete[] mouseButtons;
      if (!setMouseButtons( mouseButtonsVector )) {
        ar_log_warning() << "arInputSimulator setMouseButtons() failed in configure().\n";
      }
    }
  } else {
    ar_log_warning() << "SZG_INPUTSIM/mouse_buttons undefined, using defaults.\n";
  }

  const int numButtonEvents = SZGClient.getAttributeInt( "SZG_INPUTSIM", "number_button_events" );
  if (numButtonEvents < 2) {
    ar_log_warning() << "arInputSimulator: SZG_INPUTSIM/number_button_events " << numButtonEvents
                     << " < 2, so defaulting to 8.\n";
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
      ar_log_warning() << "arInputSimulator: duplicate mouse button indices in setMouseButtons().\n";
      _mouseButtons = buttonsPrev;
      return false;
    }
    IterButton i = buttonsPrev.find(*buttonIter);
    _mouseButtons[*buttonIter] = (i == buttonsPrev.end()) ? 0 : i->second;
  }
  if ((_mouseButtons.find(0) == _mouseButtons.end())||(_mouseButtons.find(2) == _mouseButtons.end())) {
    ar_log_warning() << "arInputSimulator: mouse buttons must include 0 (left) and 2 (right).\n";
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
  ostringstream os;
  for (unsigned i=0; i<_numButtonEvents; ++i) {
    os.str("");
    os << i;
    _buttonLabels.push_back( char(os.str()[0]) );
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
  glMultMatrixf(ar_rotationMatrix('y',_rotSim).v);

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
  _driver.queueMatrix(0,_matrix[0]);
  _driver.queueMatrix(1,_matrix[1]);
  _driver.queueAxis(0,_axis[0]);
  _driver.queueAxis(1,_axis[1]);

  if (_newButtonEvents.size() != _lastButtonEvents.size()) {
    ar_log_warning() << "arInputSimulator: numbers of new & last button values out of sync.\n";
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
  switch (key) {
  case ' ':
    {
    if (rowLength == 0 || numRows == 0) {
      ar_log_warning() << "arInputSimulator: no buttons in keyboard().\n";
      break;
    }
    // "Feature:" any buttons held down (_newButtonEvents[i] != 0) will STAY down
    // while spacebar cycles to the next subset of buttons.
    if (++_buttonSelector >= numRows)
      _buttonSelector = 0;
    break;
    }
  case '1':
    _interfaceState = AR_SIM_HEAD_TRANSLATE;
    break;
  case '2':
    _interfaceState = AR_SIM_HEAD_ROTATE;
    break;
  case '3':
    _interfaceState = AR_SIM_WAND_TRANSLATE;
    break;
  case '4':
    _interfaceState = AR_SIM_WAND_TRANS_BUTTONS;
    goto LResetWand;
  case '5':
    if (_interfaceState == AR_SIM_WAND_ROTATE_BUTTONS) {
      // Hit 5 twice to reset wand's rotation.
      // todo: do this "double clicking" on other states too.
LResetWand:
      _rotWand[0] = 0.;
      _rotWand[1] = 0.;
      _matrix[1][0] = 1.;
      _matrix[1][1] = 0.;
      _matrix[1][2] = 0.;
      _matrix[1][4] = 0.;
      _matrix[1][5] = 1.;
      _matrix[1][6] = 0.;
      _matrix[1][8] = 0.;
      _matrix[1][9] = 0.;
      _matrix[1][10] = 1.;
    }
    else
      _interfaceState = AR_SIM_WAND_ROTATE_BUTTONS;
    break;
  case '6':
    _interfaceState = AR_SIM_USE_JOYSTICK;
    _axis[0] = 0.;
    _axis[1] = 0.;
    break;
  case  '7':
    if (_interfaceState == AR_SIM_SIMULATOR_ROTATE) {
      _rotSim = 0.;
    }
    _interfaceState = AR_SIM_SIMULATOR_ROTATE;
    break;
  }
}

// Process mouse button events.
void arInputSimulator::mouseButton(int button, int state, int x, int y){
  _mouseXY[0] = x;
  _mouseXY[1] = y;

  if (button < 0) {
    ar_log_warning() << "arInputSimulator ignoring negative button index.\n";
    return;
  }
  unsigned buttonIndex = (unsigned)button;
  IterButton iFind = _mouseButtons.find( buttonIndex );
  const bool haveIndex = iFind != _mouseButtons.end();
  if (haveIndex)
    _mouseButtons[buttonIndex] = state;
  else {
    vector<unsigned> buttons = getMouseButtons();
    buttons.push_back( buttonIndex );
    if (!setMouseButtons( buttons )) {
      ar_log_warning() << "arInputSimulator failed to add button index " << buttonIndex
                       << ".\n";
    } else {
      ar_log_remark() << "arInputSimulator added button index " << buttonIndex
                      << " to index list.\n";
      return;
    }
  }

  switch (_interfaceState) {
  case AR_SIM_WAND_ROTATE_BUTTONS:
  case AR_SIM_WAND_TRANS_BUTTONS:
    // Change only _newButton here, so we can send only the button event diff.
    if (haveIndex) {
      unsigned buttonOffset = 0;
      for (IterButton i = _mouseButtons.begin(); i != iFind && i != _mouseButtons.end(); ++i) {
        ++buttonOffset;
      }
      const unsigned eventIndex = _mouseButtons.size() * _buttonSelector + buttonOffset;
      if (eventIndex >= _newButtonEvents.size()) {
        ar_log_warning() << "arInputSimulator ignoring out-of-range button event index " <<
	  eventIndex << ".\n";
        return;
      }
      _newButtonEvents[eventIndex] = state;
      // Queue the button events here, instead of advance().
      _driver.queueButton(eventIndex, state);
    }
    break;
  case AR_SIM_USE_JOYSTICK:
    _axis[0] = 0.;
    _axis[1] = 0.;
    break;
  case AR_SIM_HEAD_TRANSLATE:
  case AR_SIM_HEAD_ROTATE:
  case AR_SIM_WAND_TRANSLATE:
  case AR_SIM_SIMULATOR_ROTATE:
    // do nothing
    break;
  }
}

static inline float clamp(const float x, const float xMin, const float xMax) {
  return x<xMin ? xMin : x>xMax ? xMax : x;
}

// Process mouse movement events to drive the simulated interface.
void arInputSimulator::mousePosition(int x, int y){
//cerr << "mousePosition: " << x << ", " << y << endl;
  if (!_fInit) {
    _fInit = true;
    _mouseXY[0] = x;
    _mouseXY[1] = y;
  }

  const float gain = 0.03;
  const float dx = gain * (x - _mouseXY[0]);
  const float dy = gain * (y - _mouseXY[1]);
  _mouseXY[0] = x;
  _mouseXY[1] = y;
  const int leftButton   = (_mouseButtons.find(0) == _mouseButtons.end()) ? 0 : _mouseButtons[0];
  const int middleButton = (_mouseButtons.find(1) == _mouseButtons.end()) ? 0 : _mouseButtons[1];
  const int rightButton  = (_mouseButtons.find(2) == _mouseButtons.end()) ? 0 : _mouseButtons[2];

  switch (_interfaceState) {

  case AR_SIM_HEAD_TRANSLATE:
    if (leftButton)
      _matrix[0] = ar_translationMatrix(dx, -dy,0) * _matrix[0];
    if (rightButton)
      _matrix[0] = ar_translationMatrix(dx, 0, dy) * _matrix[0];
    break;

  case AR_SIM_HEAD_ROTATE:
    if (leftButton) {
      _matrix[0] = ar_extractTranslationMatrix(_matrix[0]) *
	ar_rotationMatrix('y', -dx) *
	ar_extractRotationMatrix(_matrix[0]);
     }
    if (rightButton) {
      const arVector3 rotAxis(ar_extractRotationMatrix(_matrix[0]) * arVector3(1,0,0));
      _matrix[0] = ar_extractTranslationMatrix(_matrix[0]) *
	ar_rotationMatrix(rotAxis, -dy) *
	ar_extractRotationMatrix(_matrix[0]);
    }
    break;

  case AR_SIM_WAND_TRANSLATE:
    if (leftButton) {
      _matrix[1][12] += dx;
      _matrix[1][13] -= dy;
    }
    if (rightButton) {
      _matrix[1][12] += dx;
      _matrix[1][14] += dy;
    }
    break;

  case AR_SIM_WAND_TRANS_BUTTONS:
    _matrix[1][12] += dx;
    _matrix[1][13] -= dy;
    break;

  case AR_SIM_USE_JOYSTICK:
    if (leftButton || middleButton || rightButton) {
      const float gainJoystick = 0.4;
      _axis[0] = clamp(_axis[0] + gainJoystick*dx, -1, 1);
      _axis[1] = clamp(_axis[1] - gainJoystick*dy, -1, 1);
    }
    break;

  case AR_SIM_SIMULATOR_ROTATE:
    if (leftButton || middleButton || rightButton) {
      _rotSim += dx;
      if (_rotSim < -180)
	_rotSim += 360;
      if (_rotSim > 180)
	_rotSim -= 360;
    }
    break;

  case AR_SIM_WAND_ROTATE_BUTTONS:
    const float gainRotWand = 1.7;
    _rotWand[0] += gainRotWand*dx;
    _rotWand[1] -= gainRotWand*dy;
    _matrix[1] = ar_extractTranslationMatrix(_matrix[1]) *
      ar_planeToRotation(_rotWand[0], _rotWand[1]);
    break;
  }
}

void arInputSimulator::_wireCube(float size) const {
  size /= 2;
  glBegin(GL_LINES);
  glVertex3f(size,size,-size);
  glVertex3f(size,-size,-size);

  glVertex3f(size,-size,-size);
  glVertex3f(-size,-size,-size);

  glVertex3f(-size,-size,-size);
  glVertex3f(-size,size,-size);

  glVertex3f(-size,size,-size);
  glVertex3f(size,size,-size);

  glVertex3f(size,size,size);
  glVertex3f(size,-size,size);

  glVertex3f(size,-size,size);
  glVertex3f(-size,-size,size);

  glVertex3f(-size,-size,size);
  glVertex3f(-size,size,size);

  glVertex3f(-size,size,size);
  glVertex3f(size,size,size);

  glVertex3f(size,size,size);
  glVertex3f(size,size,-size);

  glVertex3f(-size,size,size);
  glVertex3f(-size,size,-size);

  glVertex3f(size,-size,size);
  glVertex3f(size,-size,-size);

  glVertex3f(-size,-size,size);
  glVertex3f(-size,-size,-size);
  glEnd();
}

void arInputSimulator::_drawGamepad() const {
  const unsigned rowLength = _mouseButtons.size();
  unsigned numRows = unsigned(_numButtonEvents / rowLength);
  if (_numButtonEvents % rowLength != 0)
    ++numRows;
  if (rowLength == 0 || numRows == 0) {
    ar_log_warning() << "arInputSimulator: no buttons in _drawGamepad().\n";
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
    // blue background for the wand controller
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
    // cube for the gamepad's joystick
    glPushMatrix();
      glTranslatef(0.5*_axis[0], -0.3 + 0.5*_axis[1], 0.1);
      glColor3f(0.3,1,0);
      glutSolidSphere(0.25,8,8);
    glPopMatrix();
    // joystick's boundary
    glBegin(GL_LINE_STRIP);
      glColor3f(1,1,1);
      glVertex3f(-0.5,0.2,0.3);
      glVertex3f(-0.5,-0.8,0.3);
      glVertex3f(0.5,-0.8,0.3);
      glVertex3f(0.5,0.2,0.3);
      glVertex3f(-0.5,0.2,0.3);
    glEnd();
    // Indicates which row of buttons is controlled by the mouse selector button
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
          ar_log_warning() << "arInputSimulator: _buttonLabels too short in _drawGamepad().\n";
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
    glMultMatrixf(_matrix[0].v);
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
    glMultMatrixf(_matrix[1].v);
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

  const char* const hint[7] = {
      "1: Move head",
      "2: Turn head",
      "3: Move wand",
      "4: Move wand + buttons",
      "5: Turn wand + buttons",
      "6: Joystick",
      "7: Turn Sim"
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
