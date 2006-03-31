//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This must be the first non-comment line!
#include "arPrecompiled.h"
#include "arInputSimulator.h"
#include "arGraphicsHeader.h"
#include "arSTLalgo.h"
#include "arSZGClient.h"
#include "arLogStream.h"

arInputSimulator::arInputSimulator() :
  _numButtonEvents(0),
  _simulatorRotation(0.),
  _buttonSelector(0) {
  // set the values of the internal simulator state appropriately
  _mousePosition[0] = 0;
  _mousePosition[1] = 0;
  std::vector<unsigned int> buttons;
  buttons.push_back(0);
  buttons.push_back(2);
  if (!setMouseButtons( buttons )) {
    ar_log_error() << "arInputSimulator warning: setMouseButtons() failed in constructor.\n";
  }
  setNumberButtonEvents( 8 );
  _rotator[0] = 0.;
  _rotator[1] = 0.;
  _simulatorRotation = 0;
  _buttonSelector = 0;
  _interfaceState = AR_SIM_HEAD_TRANSLATE;

  // set the initial values of the simulated device appropriately
  _matrix[0] = ar_translationMatrix(0,5,0);
  _matrix[1] = ar_translationMatrix(2,3,-1);
  _axis[0] = 0;
  _axis[1] = 0;
}

bool arInputSimulator::configure( arSZGClient& SZGClient ) {
  arSlashString mouseButtonString(SZGClient.getAttribute( "SZG_INPUTSIM", "mouse_buttons" ));
  const int numItems = mouseButtonString.size();
  int* mouseButtons = new int[numItems];
  if (!mouseButtons) {
    ar_log_error() << "arInputSimulator error: new[] failed.\n";
    return false;
  }
  const int numValues = ar_parseIntString( mouseButtonString, mouseButtons, numItems );
  if (numValues != numItems) {
    ar_log_warning() << "arInputSimulator warning: SZG_INPUTSIM/mouse_buttons defaulting to 0 and 2 for left and right.\n";
    delete[] mouseButtons;
  } else {
    std::vector<unsigned int> mouseButtonsVector;
    for (int i=0; i<numValues; ++i) {
      mouseButtonsVector.push_back( mouseButtons[i] );
    }
    delete[] mouseButtons;
    if (!setMouseButtons( mouseButtonsVector )) {
      ar_log_error() << "arInputSimulator error: setMouseButtons() failed in configure().\n";
    }
  }
  const int numButtonEvents = SZGClient.getAttributeInt( "SZG_INPUTSIM", "number_button_events" );
  if (numButtonEvents < 2) {
    ar_log_warning() << "arInputSimulator warning: too few SZG_INPUTSIM/number_button_events (" << numButtonEvents
                     << ") < 2, so defaulting to 8.\n";
    return false;
  }
  setNumberButtonEvents( unsigned(numButtonEvents) );
  return true;
}

bool arInputSimulator::setMouseButtons( std::vector<unsigned int>& mouseButtons ) {
  std::map<unsigned int,int> oldMouseButtons = _mouseButtons;
  _mouseButtons.clear();
  std::vector<unsigned int>::iterator buttonIter;
  for (buttonIter = mouseButtons.begin(); buttonIter != mouseButtons.end(); ++buttonIter) {
    if (_mouseButtons.find( *buttonIter ) != _mouseButtons.end()) {
      ar_log_error() << "arInputSimulator error: duplicate mouse button indices found in setMouseButtons().\n";
      _mouseButtons = oldMouseButtons;
      return false;
    }
    std::map<unsigned int,int>::iterator findIter = oldMouseButtons.find( *buttonIter );
    if (findIter != oldMouseButtons.end()) {
      _mouseButtons[*buttonIter] = findIter->second;
    } else {
      _mouseButtons[*buttonIter] = 0;
    }
  }
  if ((_mouseButtons.find(0) == _mouseButtons.end())||(_mouseButtons.find(2) == _mouseButtons.end())) {
    ar_log_error() << "arInputSimulator error: mouse buttons must include 0 (left button) and 2"
                   << " (right button) in setMouseButtons().\n";
    _mouseButtons = oldMouseButtons;
    return false;
  }
  return true;
}

std::vector<unsigned int> arInputSimulator::getMouseButtons() {
  std::vector<unsigned int> buttons;
  std::map<unsigned int, int>::iterator iter;
  for (iter = _mouseButtons.begin(); iter != _mouseButtons.end(); ++iter) {
    buttons.push_back( iter->first );
  }
  return buttons;
}

void arInputSimulator::setNumberButtonEvents( unsigned int numButtonEvents ) {
  if (numButtonEvents > _numButtonEvents) {
    const unsigned int diff = numButtonEvents - _numButtonEvents;
    _lastButtonEvents.insert( _lastButtonEvents.end(), diff, 0 );
    _newButtonEvents.insert( _newButtonEvents.end(), diff, 0 );
  } else if (numButtonEvents < _numButtonEvents) {
    const unsigned int diff = _numButtonEvents - numButtonEvents;
    _lastButtonEvents.erase( _lastButtonEvents.end()-diff, _lastButtonEvents.end() );
    _newButtonEvents.erase( _newButtonEvents.end()-diff, _newButtonEvents.end() );
  }
  _numButtonEvents = numButtonEvents;
  // # buttons, # axes, # matrices
  _driver.setSignature(_numButtonEvents,2,2);
  _buttonLabels.clear();
  ostringstream os;
  for (unsigned int i=0; i<_numButtonEvents; ++i) {
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

/// Connect to an arInputNode, to
/// communicate with other entities in the system.
void arInputSimulator::registerInputNode(arInputNode* node){
  node->addInputSource(&_driver, false);
}

/// Draw current state. This can be used as a
/// stand-alone display, like in inputsimulator, or as an overlay.
void arInputSimulator::draw() const {

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-0.1,0.1,-0.1,0.1,0.3,1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  gluLookAt(0,5,23,0,5,9,0,1,0);

  arMatrix4 rotMatrix = ar_rotationMatrix('y',_simulatorRotation);
  glMultMatrixf(rotMatrix.v);

  // Must guard against the embedding program changing the line width.
  glLineWidth(1.0);
  // wireframe cube
  glColor3f(1,1,1);
  glPushMatrix();
  glTranslatef(0,5,0);
  glLineWidth(1.);
  _wireCube(10);
  glPopMatrix();

  // the rest of the stuff
  _drawHead();
  _drawWand();
  _drawGamepad();
  _drawTextState();
}

/// Draws the display... but with a nice mode of composition, so that it can
/// be used as an overlay.  Not const because pre,postComposition can't be.
void arInputSimulator::drawWithComposition() {
  // The simulated input device display is drawn in the lower right corner.
  preComposition(0.66, 0, 0.33, 0.33);
  draw();
  postComposition();
}

/// Called from time to time to post the current state of the simulated
/// device to any arInputNode that has been connected.
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
  // OK... I'm implementing the "only send data that changes" for the
  // buttons... We'll see how robust this is...
  //
  // (following should never happen).
  if (_newButtonEvents.size() != _lastButtonEvents.size()) {
    ar_log_error() << "arInputSimulator error: numbers of new & last button values have gotten out of sync.\n";
    return;
  }
  for (unsigned int i=0; i<_newButtonEvents.size(); ++i) {
    if (_newButtonEvents[i] != _lastButtonEvents[i]){
      _lastButtonEvents[i] = _newButtonEvents[i];
      // It seems to be a better idea to queue the button events where they are received, i.e. in the 
      // mouseButton method instead of sending the diff here (i.e. polling, which is unreliable for transient events).
    }
  }
  _driver.sendQueue();
}

/// Process keyboard events to drive the simulated interface.
void arInputSimulator::keyboard(unsigned char key, int, int x, int y) {
  // change the control state
  _mousePosition[0] = x;
  _mousePosition[1] = y;
  std::vector<int>::iterator iter;
  for (iter = _lastButtonEvents.begin(); iter != _lastButtonEvents.end(); ++iter) {
    *iter = 0;
  }
  unsigned int rowLength = _mouseButtons.size();
  unsigned int numRows = (unsigned int)(_numButtonEvents / rowLength);
  if (_numButtonEvents % rowLength != 0) {
    ++numRows;
  }
  switch (key) {
  case ' ':
    if ((rowLength == 0)||(numRows == 0)) {
      ar_log_error() << "arInputSimulator error: no buttons in keyboard().\n";
      return;
    }
    ++_buttonSelector;
    if (_buttonSelector >= numRows) {
      _buttonSelector = 0;
    }
    break;
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
LResetWand:
      _rotator[0] = 0.;
      _rotator[1] = 0.;
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
      _simulatorRotation = 0.;
    }
    _interfaceState = AR_SIM_SIMULATOR_ROTATE;
    break;
  }
}

/// Process mouse button events to drive the simulated interface.
void arInputSimulator::mouseButton(int button, int state, int x, int y){
  _mousePosition[0] = x;
  _mousePosition[1] = y;

  if (button < 0) {
    ar_log_error() << "arInputSimulator error: button index < 0 in mouseButton().\n";
    return;
  }
  unsigned int buttonIndex = (unsigned int)button;
  std::map<unsigned int,int>::iterator findIter = _mouseButtons.find( buttonIndex );
  bool haveIndex = findIter != _mouseButtons.end();
  if (!haveIndex) {
    std::vector<unsigned int> buttons = getMouseButtons();
    buttons.push_back( buttonIndex );
    if (!setMouseButtons( buttons )) {
      ar_log_warning() << "arInputSimulator warning: failed to add button index " << buttonIndex
                       << " to index list.\n";
    } else {
      ar_log_remark() << "arInputSimulator remark: added button index " << buttonIndex
                      << " to index list.\n";
      return;
    }
  }
  if (haveIndex) {
    _mouseButtons[buttonIndex] = state;
  }

  switch (_interfaceState) {
  case AR_SIM_WAND_ROTATE_BUTTONS:
  case AR_SIM_WAND_TRANS_BUTTONS:
    // NOTE: only change _newButton here, so that we can send only the
    // button event diff!
    if (haveIndex) {
      unsigned int buttonOffset(0);
      std::map<unsigned int,int>::iterator iter;
      for (iter = _mouseButtons.begin(); (iter != findIter) && (iter != _mouseButtons.end()); ++iter) {
        ++buttonOffset;
      }
      unsigned int eventIndex = _mouseButtons.size()*_buttonSelector + buttonOffset;
      if (eventIndex >= _newButtonEvents.size()) {
        ar_log_error() << "arInputSimulator error: button event index out of range in mouseButton().\n";
        return;
      }
      _newButtonEvents[eventIndex] = state;
      // This is where we queue the button events (instead of advance(...))
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

/// Process mouse movement events to drive the simulated interface.
void arInputSimulator::mousePosition(int x, int y){
  // Ensure that _mousePosition has been initialized before using it as the
  // 'previous' mouse position.  This is wrong, in the rare case
  // that the previous mouse position was in fact (0,0).
  if( _mousePosition[0] == 0 && _mousePosition[1] == 0 ) {
    _mousePosition[0] = x;
    _mousePosition[1] = y;
  }

  const float deltaX = x - _mousePosition[0];
  const float deltaY = y - _mousePosition[1];
  const float movementFactor = 0.03;
  _mousePosition[0] = x;
  _mousePosition[1] = y;
  int leftButton = (_mouseButtons.find(0) == _mouseButtons.end())?(0):(_mouseButtons[0]);
  int middleButton = (_mouseButtons.find(1) == _mouseButtons.end())?(0):(_mouseButtons[1]);
  int rightButton = (_mouseButtons.find(2) == _mouseButtons.end())?(0):(_mouseButtons[2]);

  if (_interfaceState == AR_SIM_WAND_ROTATE_BUTTONS){
    _rotator[0] += 3*movementFactor*deltaX;
    _rotator[1] -= 3*movementFactor*deltaY;
    _matrix[1] = ar_extractTranslationMatrix(_matrix[1])*
      ar_planeToRotation(_rotator[0], _rotator[1]);
  }
  if (_interfaceState == AR_SIM_HEAD_TRANSLATE && leftButton){
    _matrix[0] = ar_translationMatrix(movementFactor*deltaX,
                                      -movementFactor*deltaY,0)
                                      *_matrix[0];
  }
  if (_interfaceState == AR_SIM_HEAD_TRANSLATE && rightButton){
    _matrix[0] = ar_translationMatrix(movementFactor*deltaX,
                                      0,movementFactor*deltaY)
                                      *_matrix[0];
  }
  if (_interfaceState == AR_SIM_HEAD_ROTATE && leftButton){
    _matrix[0] = ar_extractTranslationMatrix(_matrix[0])
      * ar_rotationMatrix('y',-3*movementFactor*deltaX)
      * ar_extractRotationMatrix(_matrix[0]);
  }
  if (_interfaceState == AR_SIM_HEAD_ROTATE && rightButton){
    arVector3 rotAxis = ar_extractRotationMatrix(_matrix[0])*arVector3(1,0,0);
    _matrix[0] = ar_extractTranslationMatrix(_matrix[0])
      * ar_rotationMatrix(rotAxis,-3*movementFactor*deltaY)
      * ar_extractRotationMatrix(_matrix[0]);
//    _matrix[0] = ar_extractTranslationMatrix(_matrix[0])
//      * ar_rotationMatrix('x',-3*movementFactor*deltaY)
//      * ar_extractRotationMatrix(_matrix[0]);
  }
  if (_interfaceState == AR_SIM_WAND_TRANSLATE && leftButton){
    _matrix[1][12] += movementFactor*deltaX;
    _matrix[1][13] -= movementFactor*deltaY;
  }
  if (_interfaceState == AR_SIM_WAND_TRANSLATE && rightButton){
    _matrix[1][12] += movementFactor*deltaX;
    _matrix[1][14] += movementFactor*deltaY;
  }
  if (_interfaceState == AR_SIM_WAND_TRANS_BUTTONS){
    _matrix[1][12] += movementFactor*deltaX;
    _matrix[1][13] -= movementFactor*deltaY;
  }
  if (_interfaceState == AR_SIM_USE_JOYSTICK
      && (leftButton || middleButton || rightButton)){
    const float joystickFactor = 0.3;
    float candidateX = _axis[0] + movementFactor*joystickFactor*deltaX;
    float candidateY = _axis[1] - movementFactor*joystickFactor*deltaY;
    if (candidateX > 1)
      candidateX = 1;
    else if (candidateX < -1)
      candidateX = -1;
    if (candidateY > 1)
      candidateY = 1;
    else if (candidateY < -1)
      candidateY = -1;
    _axis[0] = candidateX;
    _axis[1] = candidateY;
  }
  if (_interfaceState == AR_SIM_SIMULATOR_ROTATE
      && (leftButton || middleButton || rightButton)){
    _simulatorRotation += movementFactor*deltaX;
    if (_simulatorRotation < -180){
      _simulatorRotation += 360;
    }
    if (_simulatorRotation > 180){
      _simulatorRotation -= 360;
    }
  }
}

void arInputSimulator::_wireCube(float size) const {
  const float offset = size/2;
  glBegin(GL_LINES);
  glVertex3f(offset,offset,-offset);
  glVertex3f(offset,-offset,-offset);

  glVertex3f(offset,-offset,-offset);
  glVertex3f(-offset,-offset,-offset);

  glVertex3f(-offset,-offset,-offset);
  glVertex3f(-offset,offset,-offset);

  glVertex3f(-offset,offset,-offset);
  glVertex3f(offset,offset,-offset);

  glVertex3f(offset,offset,offset);
  glVertex3f(offset,-offset,offset);

  glVertex3f(offset,-offset,offset);
  glVertex3f(-offset,-offset,offset);

  glVertex3f(-offset,-offset,offset);
  glVertex3f(-offset,offset,offset);

  glVertex3f(-offset,offset,offset);
  glVertex3f(offset,offset,offset);

  glVertex3f(offset,offset,offset);
  glVertex3f(offset,offset,-offset);

  glVertex3f(-offset,offset,offset);
  glVertex3f(-offset,offset,-offset);

  glVertex3f(offset,-offset,offset);
  glVertex3f(offset,-offset,-offset);

  glVertex3f(-offset,-offset,offset);
  glVertex3f(-offset,-offset,-offset);
  glEnd();
}

void arInputSimulator::_drawGamepad() const {
  const float GAMEPAD_YOFFSET(-.8);
  const float GAMEPAD_WIDTH(1.);
  const float BUTTON_SPACING(1.1);
  const float BUTTON_YBORDER(.8);
  const unsigned int rowLength = _mouseButtons.size();
  unsigned int numRows = (unsigned int)(_numButtonEvents / rowLength);
  if (_numButtonEvents % rowLength != 0)
    ++numRows;
  if (rowLength == 0 || numRows == 0) {
    ar_log_error() << "arInputSimulator error: no buttons in _drawGamepad().\n";
    return;
  }

  const float padWidth = max((float)1.4,rowLength*BUTTON_SPACING);
  const float padHeight =  GAMEPAD_WIDTH+numRows*BUTTON_SPACING;
  // Always want to surround draws with push/pop
  glPushMatrix();
    arMatrix4 tempMatrix = ar_translationMatrix(4,0,5);
    glMultMatrixf(tempMatrix.v);
    // blue background for the wand controller graphics
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f(0,0,1,.5);
    glBegin(GL_QUADS);
      glVertex3f(-.5*padWidth,GAMEPAD_YOFFSET + padHeight,0.1);
      glVertex3f(.5*padWidth, GAMEPAD_YOFFSET + padHeight,0.1);
      glVertex3f(.5*padWidth, GAMEPAD_YOFFSET,0.1);
      glVertex3f(-.5*padWidth,GAMEPAD_YOFFSET,0.1);
    glEnd();
    glDisable( GL_BLEND );
    // one cube for the pad's joystick
    glPushMatrix();
      glTranslatef(0.5*_axis[0], -0.3 + 0.5*_axis[1], 0.1);
      glColor3f(0.3,1,0);
      glutSolidSphere(0.25,8,8);
    glPopMatrix();
    // a boundary region within which the joystick moves
    glBegin(GL_LINE_STRIP);
      glColor3f(1,1,1);
      glVertex3f(-0.5,0.2,0.3);
      glVertex3f(-0.5,-0.8,0.3);
      glVertex3f(0.5,-0.8,0.3);
      glVertex3f(0.5,0.2,0.3);
      glVertex3f(-0.5,0.2,0.3);
    glEnd();
    // A button selector sphere that indicates which
    // set of 3 buttons is controlled by the mouse selector button
    glPushMatrix();
      glTranslatef(-.5*padWidth,BUTTON_YBORDER+BUTTON_SPACING*_buttonSelector,0.1);
      glColor3f(1,1,1);
      glutSolidSphere(0.15,8,8);
    glPopMatrix();
    std::vector<int>::iterator const_iter;
    float xlo((-.5*((float)rowLength) + .5)*BUTTON_SPACING);
    float x(xlo);
    float y(BUTTON_YBORDER);
    unsigned int rowCount(0);
    float labelScale = .004*BUTTON_SPACING;
    for (unsigned int i = 0; i<_newButtonEvents.size(); ++i) {
      glPushMatrix();
        glTranslatef(x,y,0.1);
        if (_newButtonEvents[i] == 0)
          glColor3f(1,0,0);
        else
          glColor3f(0,1,0);
        glutSolidSphere(.4*BUTTON_SPACING,8,8);
        if (i >= _buttonLabels.size()) {
          ar_log_error() << "arInputSimulator error: _buttonLabels too short in _drawGamepad().\n";
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
    // 6 wand buttons
  //  for (int i=0; i<6; i++){
  //    glPushMatrix();
  //    glTranslatef(-0.7 + 0.7*(i%3),
  //                 0.5 + 0.7*(i/3),
  //                 0.1);
  //    if (_button[i] == 0)
  //      glColor3f(1,0,0);
  //    else
  //      glColor3f(0,1,0);
  //    glutSolidSphere(0.25,8,8);
  //    glPopMatrix();
  //  }
    glEnable(GL_LIGHTING);
  // always want to surround draw with push/pop
  glPopMatrix();
}

void arInputSimulator::_drawHead() const {
  // the user's head
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
      glutSolidSphere(0.1,8,8);
    glPopMatrix();
    glTranslatef(-0.5,0,-0.8);
    glColor3f(0,1,1);
    glutSolidSphere(0.4,8,8);
    glTranslatef(0,0,-0.4);
    glColor3f(1,0,0);
    glutSolidSphere(0.1,8,8);
  glPopMatrix();
}

void arInputSimulator::_drawWand() const {
  // wand
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
  // text state info
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glColor3f(0.5,0.5,0.8);
  glBegin(GL_QUADS);
  glVertex3f(-1,0.88,0.00001);
  glVertex3f(1,0.88,0.00001);
  glVertex3f(1,1,0.00001);
  glVertex3f(-1,1,0.00001);
  glEnd();

  const char* hint[7] = {
      "[1] Translate head",
      "[2] Rotate head",
      "[3] Translate wand",
      "[4] Translate wand + buttons",
      "[5] Rotate wand + buttons",
      "[6] Use joystick",
      "[7] Rotate Simulator"
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
