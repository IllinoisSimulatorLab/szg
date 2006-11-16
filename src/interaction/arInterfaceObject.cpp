//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInterfaceObject.h"

void ar_interfaceObjectIOPollTask(void* object){
   ((arInterfaceObject*)object)->_ioPollTask();
}

static inline float joystickScale(float j) {
  // Input: [-1,1] joystick deflection.
  // Output: 0 ("dead zone") when input is in [-.2,.2];
  //   outside that, expand back to [-1,1], as square of input.

  const float dead = 0.07; // Casino Royale premieres in two days
  const float expand = 1. - dead;
  const float sign = j<0. ? -1. : 1.;
  j *= sign;
  const float undead = j<dead ? 0. : (j - dead) / expand;
  return undead * undead * sign;
}

// As joystick is moved, translate _mNav (POV) in plane of gamepad.
// Also, if button 1 is held, rotate _mObj with wand.

void arInterfaceObject::_ioPollTask(){
  while (true) {
    const float speedLateral = -joystickScale(_inputDevice->getAxis(0));
    const float speedForward = -joystickScale(_inputDevice->getAxis(1));
    const arMatrix4 mWand(_inputDevice->getMatrix(1));
    const arMatrix4 mWandRot(ar_extractRotationMatrix(mWand));

    // In CAVE coords, normal wand setup points wand along -z axis
    // when the bird's rotation is the identity matrix.

    const arMatrix4 m(!ar_extractRotationMatrix(_mNav) * mWandRot);
    const arVector3 vWandForward(m * arVector3(0,0,-1) * speedForward);
    const arVector3 vWandLateral(m * arVector3(1,0,0)  * speedLateral);

    ar_mutex_lock(&_infoLock);

    const float inertia = 0.98; // Between 0.01 and 0.99.  Should be database parameter.
    arVector3 vMove((vWandLateral + vWandForward) * _speedMultiplier * .2);
    vMove = (1. - inertia) * vMove + inertia * _vMovePrev;
    _vMovePrev = vMove;
    _mNav = ar_translationMatrix(vMove) * _mNav;

    // Grabbing.

    const int b1 = _inputDevice->getButton(0);
    if (b1 && !_grabbed){
      // Begin grabbing.
      _grabbed = true;
      _mGrab = !mWandRot * _mObj;
    }
    else if (b1 && _grabbed){
      // Continue grabbing.
      _mObj = mWandRot * _mGrab;
    }
    else if (!b1 && _grabbed){
      // End grabbing.
      _grabbed = false;
    }

    ar_mutex_unlock(&_infoLock);
    ar_usleep(10000);
  }
}
      
arInterfaceObject::arInterfaceObject() :
  _inputDevice(NULL),
  _speedMultiplier(1.),
  _vMovePrev(0,0,0),
  _grabbed(false)
{
  ar_mutex_init(&_infoLock);
  _matrices.clear();
  _buttons.clear();
  _axes.clear();
}

arInterfaceObject::~arInterfaceObject() {
  _matrices.clear();
  _buttons.clear();
  _axes.clear();
}
  
void arInterfaceObject::setInputDevice(arInputNode* device){
  _inputDevice = device;
}

bool arInterfaceObject::start(){
  if (!_inputDevice){
    cerr << "arInterfaceObject error: input device undefined.\n";
    return false;
  }

  if (!_IOPollThread.beginThread(ar_interfaceObjectIOPollTask, this)) {
    cerr << "arInterfaceObject error: failed to start IOPoll thread.\n";
    return false;
  }

  return true;
}

void arInterfaceObject::setNavMatrix(const arMatrix4& arg){
  ar_mutex_lock(&_infoLock);
    _mNav = arg;
  ar_mutex_unlock(&_infoLock);
}

arMatrix4 arInterfaceObject::getNavMatrix(){
  ar_mutex_lock(&_infoLock);
    const arMatrix4 result(_mNav);
  ar_mutex_unlock(&_infoLock);
  return result;
}

void arInterfaceObject::setObjectMatrix(const arMatrix4& arg){
  ar_mutex_lock(&_infoLock);
  _mObj = arg;
  ar_mutex_unlock(&_infoLock);
}

arMatrix4 arInterfaceObject::getObjectMatrix(){
  ar_mutex_lock(&_infoLock);
    const arMatrix4 result(_mObj);
  ar_mutex_unlock(&_infoLock);
  return result;
}

void arInterfaceObject::setSpeedMultiplier(float multiplier){
  _speedMultiplier = multiplier;
}

void arInterfaceObject::setNumMatrices( const int num ) {
  _matrices = std::vector<arMatrix4>( num );
}

void arInterfaceObject::setNumButtons( const int num ) {
  _buttons = std::vector<int>( num );
}

void arInterfaceObject::setNumAxes( const int num ) {
  _axes = std::vector<float>( num );
}

int arInterfaceObject::getNumMatrices() const {
  return _matrices.size();
}

int arInterfaceObject::getNumButtons() const {
  return _buttons.size();
}

int arInterfaceObject::getNumAxes() const {
  return _axes.size();
}

bool arInterfaceObject::setMatrix( const int num, const arMatrix4& mat ) {
  if ((num < 0)||(num >= (int)_matrices.size()))
    return false;
  _matrices[num] = mat;
  return true;
}

bool arInterfaceObject::setButton( const int num, const int but ) {
  if ((num < 0)||(num >= (int)_buttons.size()))
    return false;
  _buttons[num] = but;
  return true;
}

bool arInterfaceObject::setAxis( const int num, const float val ) {
  if ((num < 0)||(num >= (int)_axes.size()))
    return false;
  _axes[num] = val;
  return true;
}

void arInterfaceObject::setMatrices( const arMatrix4* matPtr ) {
  for (int i=0; i<(int)_matrices.size(); i++)
    _matrices[i] = *matPtr++;
}

void arInterfaceObject::setButtons( const int* butPtr ) {
  for (int i=0; i<(int)_buttons.size(); i++)
    _buttons[i] = *butPtr++;
}

void arInterfaceObject::setAxes( const float* axisPtr ) {
  for (int i=0; i<(int)_axes.size(); i++)
    _axes[i] = *axisPtr++;
}

arMatrix4 arInterfaceObject::getMatrix( const int num ) const {
  static arMatrix4 ident(ar_identityMatrix());
  if ((num < 0)||(num >= (int)_matrices.size()))
    return ident;
  return _matrices[num];
}

int arInterfaceObject::getButton( const int num ) const {
  if ((num < 0)||(num >= (int)_buttons.size()))
    return 0;
  return _buttons[num];
}

float arInterfaceObject::getAxis( const int num ) const {
  if ((num < 0)||(num >= (int)_axes.size()))
    return 0.;
  return _axes[num];
}
