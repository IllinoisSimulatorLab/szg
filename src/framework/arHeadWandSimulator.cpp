//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This must be the first non-comment line!
#include "arPrecompiled.h"
#include "arHeadWandSimulator.h"
#include "arGraphicsHeader.h"

arHeadWandSimulator::arHeadWandSimulator(){
  // set the values of the internal simulator state appropriately
  _mousePosition[0] = 0;
  _mousePosition[1] = 0;
  _mouseButton[0] = 0;
  _mouseButton[1] = 0;
  _mouseButton[2] = 0;
  _rotator[0] = 0.;
  _rotator[1] = 0.;
  _worldRotation = 0;
  _buttonSelector = 0;
  _interfaceState = AR_SIM_EYE_TRANSLATE;
  
  // set the initial values of the simulated device appropriately
  _matrix[0] = ar_translationMatrix(0,5,2); 
  _matrix[1] = ar_translationMatrix(2,3,0);
  for (int i=0; i<6; i++){
    _button[i] = 0;
    _newButton[i] = 0;
  }
  _axis[0] = 0;
  _axis[1] = 0;

  // we have 6 buttons, 2 axes, and 2 matrices
  _driver.setSignature(6,2,2);
}

arHeadWandSimulator::~arHeadWandSimulator(){
  // does nothing as of yet
}

/// The simulator can connect to an arInputNode so that it is able to
/// communicate with other entities in the system.
void arHeadWandSimulator::registerInputNode(arInputNode* node){
  node->addInputSource(&_driver, false); 
}

/// The simulator can draw it's current state. This can be used as a 
/// stand-alone display, like in wandsimserver, or it can be used as an
/// overlay.
void arHeadWandSimulator::draw(){

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glFrustum(-0.1,0.1,-0.1,0.1,0.3,1000);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  gluLookAt(0,5,23,0,5,9,0,1,0);

  arMatrix4 rotMatrix = ar_rotationMatrix('y',_worldRotation);
  glMultMatrixf(rotMatrix.v);
 
  // wireframe cube
  glColor3f(1,1,1);
  glPushMatrix();
  glTranslatef(0,5,0);
  _wireCube(10);
  glPopMatrix();

  // the rest of the stuff
  _drawHead();
  _drawWand();
  _drawGamepad();
  _drawTextState();
}

/// Draws the display... but with a nice mode of composition, so that it can
/// be used as an overlay.
void arHeadWandSimulator::drawWithComposition(){
  // The simulated input device display is drawn in the lower right corner.
  preComposition(0.66, 0, 0.33, 0.33);
  draw();
  postComposition();
  return;
}

/// Called from time to time to post the current state of the simulated
/// device to any arInputNode that has been connected.
void arHeadWandSimulator::advance(){
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
  for (int i=0; i<6; i++){
    if (_newButton[i] != _button[i]){
      _button[i] = _newButton[i];
      _driver.queueButton(i,_button[i]);
    }
  }
  _driver.sendQueue();
}

/// Process keyboard events to drive the simulated interface.
void arHeadWandSimulator::keyboard(unsigned char key, int, int x, int y){
  // change the control state
  _mousePosition[0] = x;
  _mousePosition[1] = y;
  _button[0] = 0;
  _button[1] = 0;
  _button[2] = 0;
  _button[3] = 0;
  _button[4] = 0;
  _button[5] = 0;
  switch (key) {
  case ' ':
    _buttonSelector = 1 - _buttonSelector;
    break;
  case '1':
    _interfaceState = AR_SIM_EYE_TRANSLATE;
    break;
  case '2':
    _interfaceState = AR_SIM_ROTATE_VIEW;
    break;
  case '3':
    _interfaceState = AR_SIM_WAND_TRANSLATE;
    break;
  case '4':
    _interfaceState = AR_SIM_WAND_TRANS_BUTTONS;
    goto LResetWand;
  case '5':
    if (_interfaceState == AR_SIM_WAND_ROTATE_BUTTONS){
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
    _interfaceState = AR_SIM_JOYSTICK_MOVE;
    _axis[0] = 0.;
    _axis[1] = 0.;
    break;
  case  '7':
    if (_interfaceState == AR_SIM_WORLD_ROTATE){
      _worldRotation = 0.;
    }
    _interfaceState = AR_SIM_WORLD_ROTATE;
    break;
  }
}

/// Process mouse button events to drive the simulated interface.
void arHeadWandSimulator::mouseButton(int button, int state, int x, int y){
  _mousePosition[0] = x;
  _mousePosition[1] = y;
  _mouseButton[button] = state;
  
  switch (_interfaceState) {
  case AR_SIM_WAND_ROTATE_BUTTONS:
  case AR_SIM_WAND_TRANS_BUTTONS:
    // NOTE: only change _newButton here, so that we can send only the
    // button event diff!
    _newButton[3*_buttonSelector + button] = state;
    break;
  case AR_SIM_JOYSTICK_MOVE:
    _axis[0] = 0.;
    _axis[1] = 0.;
    break;
  case AR_SIM_EYE_TRANSLATE:
  case AR_SIM_ROTATE_VIEW:
  case AR_SIM_WAND_TRANSLATE:
  case AR_SIM_WORLD_ROTATE:
    // do nothing
    break;
  }
}

/// Process mouse movement events to drive the simulated interface.
void arHeadWandSimulator::mousePosition(int x, int y){
  const float deltaX = x - _mousePosition[0];
  const float deltaY = y - _mousePosition[1];
  const float movementFactor = 0.03;
  _mousePosition[0] = x;
  _mousePosition[1] = y;
  if (_interfaceState == AR_SIM_WAND_ROTATE_BUTTONS){
    _rotator[0] += 3*movementFactor*deltaX;
    _rotator[1] -= 3*movementFactor*deltaY;
    _matrix[1] = ar_extractTranslationMatrix(_matrix[1])*
      ar_planeToRotation(_rotator[0], _rotator[1]);
  }
  if (_interfaceState == AR_SIM_EYE_TRANSLATE && _mouseButton[0]){
    _matrix[0] = ar_translationMatrix(movementFactor*deltaX, 
                                       -movementFactor*deltaY,0)
                  *_matrix[0];
  }
  if (_interfaceState == AR_SIM_EYE_TRANSLATE && _mouseButton[1]){
    _matrix[0] = ar_translationMatrix(0,0,movementFactor*deltaY)
      *_matrix[0];   
  }
  if (_interfaceState == AR_SIM_ROTATE_VIEW && _mouseButton[0]){
    _matrix[0] = ar_extractTranslationMatrix(_matrix[0])
      * ar_rotationMatrix('y',-3*movementFactor*deltaX)
      * ar_extractRotationMatrix(_matrix[0]);
  }
  if (_interfaceState == AR_SIM_ROTATE_VIEW && _mouseButton[1]){
    arVector3 rotAxis = ar_extractRotationMatrix(_matrix[0])*arVector3(1,0,0);
    _matrix[0] = ar_extractTranslationMatrix(_matrix[0])
      * ar_rotationMatrix(rotAxis,-3*movementFactor*deltaY)
      * ar_extractRotationMatrix(_matrix[0]);
//    _matrix[0] = ar_extractTranslationMatrix(_matrix[0])
//      * ar_rotationMatrix('x',-3*movementFactor*deltaY)
//      * ar_extractRotationMatrix(_matrix[0]);
  }
  if (_interfaceState == AR_SIM_WAND_TRANSLATE && _mouseButton[0]){
    _matrix[1][12] += movementFactor*deltaX;
    _matrix[1][13] -= movementFactor*deltaY;
  }
  if (_interfaceState == AR_SIM_WAND_TRANSLATE && _mouseButton[1]){
    _matrix[1][14] += movementFactor*deltaY;
  }
  if (_interfaceState == AR_SIM_WAND_TRANS_BUTTONS){
    _matrix[1][12] += movementFactor*deltaX;
    _matrix[1][13] -= movementFactor*deltaY;
  }
  if (_interfaceState == AR_SIM_JOYSTICK_MOVE
      && (_mouseButton[0] || _mouseButton[1] || _mouseButton[2])){
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
  if (_interfaceState == AR_SIM_WORLD_ROTATE
      && (_mouseButton[0] || _mouseButton[1] || _mouseButton[2])){
    _worldRotation += movementFactor*deltaX;
    if (_worldRotation < -180){
      _worldRotation += 360;
    }
    if (_worldRotation > 180){
      _worldRotation -= 360;
    }
  }
}

void arHeadWandSimulator::_wireCube(float size){
  float offset = size/2;
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

void arHeadWandSimulator::_drawGamepad(){
  // Always want to surround draws with push/pop
  glPushMatrix();
  arMatrix4 tempMatrix = ar_translationMatrix(4,0,5);
  glMultMatrixf(tempMatrix.v);
  // blue background for the wand controller graphics
  glColor3f(0,0,1);
  glBegin(GL_QUADS);
  glVertex3f(-1,1.7,0.1);
  glVertex3f(1,1.7,0.1);
  glVertex3f(1,-0.8,0.1);
  glVertex3f(-1,-0.8,0.1);
  glEnd();
  // one cube for the wand's joystick
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
  if (!_buttonSelector){
    // first 3 buttons
    glTranslatef(-1,0.5,0.1);
  }
  else{
    // next 3 buttons
    glTranslatef(-1,1.2,0.1);
  }
  glColor3f(1,1,1);
  glutSolidSphere(0.15,8,8);
  glPopMatrix();
  // 6 wand buttons
  for (int i=0; i<6; i++){
    glPushMatrix();
    glTranslatef(-0.7 + 0.7*(i%3),
                 0.5 + 0.7*(i/3),
                 0.1);
    if (_button[i] == 0)
      glColor3f(1,0,0);
    else
      glColor3f(0,1,0);
    glutSolidSphere(0.25,8,8);
    glPopMatrix();
  }
  glEnable(GL_LIGHTING);
  // always want to surround draw with push/pop
  glPopMatrix();
}

void arHeadWandSimulator::_drawHead(){
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

void arHeadWandSimulator::_drawWand(){
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

void arHeadWandSimulator::_drawTextState(){
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
      "[7] Rotate World"
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
