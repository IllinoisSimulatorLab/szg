//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "ConsoleCallbacks.h"

// The simulated interface is modal. The states follow.
enum {
  EYE_TRANSLATE = 0,
  ROTATE_VIEW,
  WAND_TRANSLATE,
  WAND_TRANS_BUTTONS,
  WAND_ROTATE_BUTTONS,
  JOYSTICK_MOVE,
  WORLD_ROTATE
};

// The state of the simulator...
// The physical input device (i.e. mouse)
int _mousePosition[2];
int _mouseButton[3];
// A virtual input device, as driven by the mouse. Used to rotate the object
// in 3D space.
float _rotator[2];
// We can rotate the simulator display
float _worldRotation;
// There are more buttons on the simulated device than there are on the
// mouse. We have 6 simulated buttons so far, which makes sense since we
// tend to use gamepads w/ 6 DOF sensors attached for wands. At some point,
// it would be a good idea to increase the number of simulated buttons to 9!
// In any case, the "button selector" toggles between sets of buttons.
int _buttonSelector;
// Overall state in which the simulator finds itself.
int _interfaceState;

// The state of the simulated device. Two 4x4 matrices. 6 buttons. 2 axes.
arMatrix4 _wandMatrix;
arMatrix4 _headMatrix;
int _button[6];
float _caveJoystickX, _caveJoystickY; // wand joystick state

void console_init(){
  _rotator[0] = 0.;
  _rotator[1] = 0.;
  _worldRotation = 0;
  _interfaceState = EYE_TRANSLATE;
  _buttonSelector = 0;
  _mouseButton[0] = 0;
  _mouseButton[1] = 0;
  _mouseButton[2] = 0;
}

// for some reason, the glutWireCube graphic has ghostly triangles
// appear occasionally on its surface 
void console_wireCube(float size){
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

void console_drawGamepad(){
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
  glTranslatef(0.5*_caveJoystickX, -0.3 + 0.5*_caveJoystickY, 0.1);
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

void console_drawHead(){
  // the user's head
  glColor3f(1,1,0);
  glPushMatrix();
    glMultMatrixf(_headMatrix.v);
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

void console_drawWand(){
  // wand
  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
  glPushMatrix();
    glMultMatrixf((_wandMatrix).v);
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

void console_drawTextState(){
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

void console_drawInterface(float /*eye*/){

  /* use eye for stereo */
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
    console_wireCube(10);
  glPopMatrix();

  console_drawHead();
  console_drawWand();
  console_drawGamepad();
  console_drawTextState();
} 

void console_keyboard(unsigned char key, int /*state*/, int x, int y){
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
  case 27:
    exit(0);
  case ' ':
    _buttonSelector = 1 - _buttonSelector;
    break;
  case '1':
    _interfaceState = EYE_TRANSLATE;
    break;
  case '2':
    _interfaceState = ROTATE_VIEW;
    break;
  case '3':
    _interfaceState = WAND_TRANSLATE;
    break;
  case '4':
    _interfaceState = WAND_TRANS_BUTTONS;
    goto LResetWand;
  case '5':
    if (_interfaceState == WAND_ROTATE_BUTTONS){
  LResetWand:
      _rotator[0] = 0.;
      _rotator[1] = 0.;
      _wandMatrix[0] = 1.;
      _wandMatrix[1] = 0.;
      _wandMatrix[2] = 0.;
      _wandMatrix[4] = 0.;
      _wandMatrix[5] = 1.;
      _wandMatrix[6] = 0.;
      _wandMatrix[8] = 0.;
      _wandMatrix[9] = 0.;
      _wandMatrix[10] = 1.;
    }
    else
      _interfaceState = WAND_ROTATE_BUTTONS;
    break;
  case '6':
    _interfaceState = JOYSTICK_MOVE;
    _caveJoystickX = 0.;
    _caveJoystickY = 0.;
    break;
  case  '7':
    if (_interfaceState == WORLD_ROTATE){
      _worldRotation = 0.;
    }
    _interfaceState = WORLD_ROTATE;
    break;
  }
}

void console_mouseMoved(int mouseX, int mouseY){
  const float deltaX = mouseX - _mousePosition[0];
  const float deltaY = mouseY - _mousePosition[1];
  const float movementFactor = 0.03;
  _mousePosition[0] = mouseX;
  _mousePosition[1] = mouseY;
  if (_interfaceState == WAND_ROTATE_BUTTONS){
    _rotator[0] += 3*movementFactor*deltaX;
    _rotator[1] -= 3*movementFactor*deltaY;
    _wandMatrix = ar_extractTranslationMatrix(_wandMatrix)*
      ar_planeToRotation(_rotator[0], _rotator[1]);
  }
  if (_interfaceState == EYE_TRANSLATE && _mouseButton[0]){
    _headMatrix = ar_translationMatrix(movementFactor*deltaX, 
                                       -movementFactor*deltaY,0)
                  *_headMatrix;
  }
  if (_interfaceState == EYE_TRANSLATE && _mouseButton[1]){
    _headMatrix = ar_translationMatrix(0,0,movementFactor*deltaY)
      *_headMatrix;   
  }
  if (_interfaceState == ROTATE_VIEW && _mouseButton[0]){
    _headMatrix = ar_extractTranslationMatrix(_headMatrix)
      * ar_rotationMatrix('y',-3*movementFactor*deltaX)
      * ar_extractRotationMatrix(_headMatrix);
  }
  if (_interfaceState == ROTATE_VIEW && _mouseButton[1]){
    _headMatrix = ar_extractTranslationMatrix(_headMatrix)
      * ar_rotationMatrix('x',-3*movementFactor*deltaY)
      * ar_extractRotationMatrix(_headMatrix);
  }
  if (_interfaceState == WAND_TRANSLATE && _mouseButton[0]){
    _wandMatrix[12] += movementFactor*deltaX;
    _wandMatrix[13] -= movementFactor*deltaY;
  }
  if (_interfaceState == WAND_TRANSLATE && _mouseButton[1]){
    _wandMatrix[14] += movementFactor*deltaY;
  }
  if (_interfaceState == WAND_TRANS_BUTTONS){
    _wandMatrix[12] += movementFactor*deltaX;
    _wandMatrix[13] -= movementFactor*deltaY;
  }
  if (_interfaceState == JOYSTICK_MOVE
      && (_mouseButton[0] || _mouseButton[1] || _mouseButton[2])){
    const float joystickFactor = 0.3;
    float candidateX = _caveJoystickX + movementFactor*joystickFactor*deltaX;
    float candidateY = _caveJoystickY - movementFactor*joystickFactor*deltaY;
    if (candidateX > 1)
      candidateX = 1;
    else if (candidateX < -1)
      candidateX = -1;
    if (candidateY > 1)
      candidateY = 1;
    else if (candidateY < -1)
      candidateY = -1;
    _caveJoystickX = candidateX;
    _caveJoystickY = candidateY;
  }
  if (_interfaceState == WORLD_ROTATE
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

void console_mousePushed(int button, int state, int x, int y){
  _mousePosition[0] = x;
  _mousePosition[1] = y;
  _mouseButton[button] = state;
  
  switch (_interfaceState) {
  case WAND_ROTATE_BUTTONS:
  case WAND_TRANS_BUTTONS:
    _button[3*_buttonSelector + button] = state;
    break;
  case JOYSTICK_MOVE:
    _caveJoystickX = 0.;
    _caveJoystickY = 0.;
    break;
  }
}
