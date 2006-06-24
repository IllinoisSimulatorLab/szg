//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInterfaceObject.h"

void ar_interfaceObjectIOPollTask(void* object){
   ((arInterfaceObject*)object)->_ioPollTask();
}

/// Dead zone for joystick axes.
static inline float joystickScale(float j) {
  return (j>-.5 && j<.5) ? 0. : -2.*j;
}

// As joystick is pushed, translate _navMatrix (POV) in plane of gamepad.
// Also, if button 1 is held down, rotate _objectMatrix as wand rotates.

void arInterfaceObject::_ioPollTask(){
  while (true) {
    const int b1 = _inputDevice->getButton(0);
    const int b2 = _inputDevice->getButton(1);
    const int b3 = _inputDevice->getButton(2);
    const float j0 = _inputDevice->getAxis(0);
    const float j1 = _inputDevice->getAxis(1);
    const arMatrix4 wandMatrix = _inputDevice->getMatrix(1);

    // note: the CAVE coordinate system and normal wand set-up has
    // the wand pointed in the negative z-direction when the bird's
    // rotation is the identity matrix

    arVector3 wandDirection = 
      !ar_extractRotationMatrix(_navMatrix)
      * ar_extractRotationMatrix(wandMatrix)
      * arVector3(0,0,-1);

    arVector3 wandDirectionLateral =
      !ar_extractRotationMatrix(_navMatrix)
      * ar_extractRotationMatrix(wandMatrix)
      * arVector3(1,0,0);

    ar_mutex_lock(&_infoLock);

    float speed = _speedMultiplier * .1;
    if (b2)
      speed /= 4.;     // slower for fine work
    if (b3)
      speed *= 8.;     // turbo-switch for flying around in a hurry

    wandDirection *= joystickScale(j1) * speed;
    wandDirectionLateral *= joystickScale(j0) * speed;

    _navMatrix = ar_translationMatrix(wandDirectionLateral + wandDirection) *
      _navMatrix;

    static bool grabState = false;
    static arMatrix4 grabMatrix(ar_identityMatrix());
    if (b1 && !grabState){
      // Begin grabbing.
      grabState = true;
      grabMatrix = !ar_extractRotationMatrix(wandMatrix) * _objectMatrix;
    }
    else if (b1 && grabState){
      // Continued grabbing.
      _objectMatrix = ar_extractRotationMatrix(wandMatrix) * grabMatrix;
    }
    else if (!b1 && grabState){
      // End grabbing.
      grabState = false;
    }
    ar_mutex_unlock(&_infoLock);
    ar_usleep(10000);
  }
}
      
arInterfaceObject::arInterfaceObject() :
  _inputDevice(NULL),
  _speedMultiplier(1.)
{
  ar_mutex_init(&_infoLock);
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
  _navMatrix = arg;
  ar_mutex_unlock(&_infoLock);
}

arMatrix4 arInterfaceObject::getNavMatrix(){
  ar_mutex_lock(&_infoLock);
  arMatrix4 result = _navMatrix;
  ar_mutex_unlock(&_infoLock);
  return result;
}

void arInterfaceObject::setObjectMatrix(const arMatrix4& arg){
  ar_mutex_lock(&_infoLock);
  _objectMatrix = arg;
  ar_mutex_unlock(&_infoLock);
}

arMatrix4 arInterfaceObject::getObjectMatrix(){
  ar_mutex_lock(&_infoLock);
  arMatrix4 result = _objectMatrix;
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
