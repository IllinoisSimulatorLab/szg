//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <string>
#include <iostream>
#include <sstream>
#ifndef AR_USE_WIN_32
#include <sys/shm.h>
#endif
#include "arInputNode.h"
#include "arNetInputSource.h"

#include "arSharedMemSinkDriver.h"

// Methods used by the dynamic library mappers. 
extern "C"{
  SZG_CALL void* factory(){
    return new arSharedMemSinkDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSink", buffer, size);
  }
}

arSharedMemSinkDriver::arSharedMemSinkDriver() :
  _inited( false ),
  _stopped( true ),
  _eventThreadRunning( false ),
  _shmFoB(NULL),
  _shmWand(NULL)
{
}

arSharedMemSinkDriver::~arSharedMemSinkDriver() {
  _detachMemory(); // paranoid
}

/// Inverse of arSharedMemDriver::generateButton, Axis, Matrix.

inline void setButton(int ID, void* m, int value){
  *(((int*)m) + ID + 10) = value;
}

inline void setAxis(int ID, void* m, float value){
  // Outer bounds.
  if (value > 1.)
    value = 1.;
  else if (value < -1.)
    value = -1.;
  // Dead zone.
  else if (value > -.03 && value < .03)
    value = 0.;

  *(((float*)m) + ID + 23) = value;
  *(((float*)m) + ID + 42) = value;
  // David Zielinski uses 23
  // cassatt.beckman.uiuc.edu uses 42, but sometimes 23.  Not sure why.
}

void setMatrix(const int ID, const void* mRaw, const arMatrix4& value){
  float* m = ((float*)mRaw) + 7 + ID*10;
  const arVector3 t(ar_extractTranslation(value));
  memcpy(m, t.v, 3 * sizeof(float));
  const arVector3 a(180. / M_PI * ar_extractEulerAngles(value, AR_YXZ));
  memcpy(m+3, a.v, 3 * sizeof(float));
  // YXZ order agrees with arSharedMemDriver.cpp generateMatrix().
  // Cavelibs cavevars prints angles in the order pitch yaw roll, i.e. ele azi roll.
  // Ascension provides: azi ele roll.
}

bool arSharedMemSinkDriver::init(arSZGClient& c) {
  _node.addInputSource(&_source, false);
  if (!_node.init(c))
    return false;
  _inited = true;
  return true;
}

void ar_ShmSinkDriverDataTask(void* pv) {
  ((arSharedMemSinkDriver*)pv)->_dataThread();
}

bool arSharedMemSinkDriver::start() {
#ifdef AR_USE_WIN_32
  ar_log_error() << "arSharedMemSinkDriver unsupported in Windows.\n";
  return false;
#else
  if (!_inited) {
    ar_log_error() << "arSharedMemSinkDriver can't start before init.\n";
    return false;
  }

  // TrackerDaemonKey 4136
  // ControllerDaemonKey 4127
  _l.lock();
  const int idFoB = shmget(4136, 0, 0666);
  if (idFoB < 0){
    perror("no shm segment for Flock of Birds (try ipcs -m;  run a cavelib app first)");
    return false;
  }
  const int idWand = shmget(4127, 0, 0666);
  if (idWand < 0){
    perror("no shm segment for wand (try ipcs -m;  run a cavelib app first)");
    return false;
  }
  _shmFoB = shmat(idFoB, NULL, 0);
  if ((int)_shmFoB == -1){
    perror("shmat failed for Flock of Birds");
    return false;
  }
  _shmWand = shmat(idWand, NULL, 0);
  if ((int)_shmWand == -1){
    perror("shmat failed for wand");
    return false;
  }
  _l.unlock();

  if (!_node.start()) {
    return false;
  }

  return _eventThread.beginThread(ar_ShmSinkDriverDataTask,this);
#endif
}

void arSharedMemSinkDriver::_detachMemory() {
#ifndef AR_USE_WIN_32
  arGuard dummy(_l);
  if (_shmFoB) {
    if (shmdt(_shmFoB) < 0)
      ar_log_error() << "arSharedMemSinkDriver ignoring bogus shm pointer.\n";
    _shmFoB = NULL;
  }
  if (_shmWand) {
    if (shmdt(_shmWand) < 0)
      ar_log_error() << "arSharedMemSinkDriver ignoring bogus shm pointer.\n";
    _shmWand = NULL;
  }
#endif
}

bool arSharedMemSinkDriver::stop() {
  if (_stopped)
    return true;
  _stopped = true;

  _detachMemory();
  arSleepBackoff a(5, 20, 1.1);
  while (_eventThreadRunning)
    a.sleep();
  return true;
}

#ifdef AR_USE_WIN_32
// Test pattern
static inline float drand48(){
  return float(rand()) / float(RAND_MAX);
  }
#endif

void arSharedMemSinkDriver::_dataThread() {
  _stopped = false;
  _eventThreadRunning = true;
  memset(_buttonPrev, 0, sizeof(_buttonPrev));
  while (!_stopped && _eventThreadRunning) {

    // Read data from FoB
    arInputState aIS = _node._inputState; // local copy
    const unsigned cm = aIS.getNumberMatrices();
    const unsigned ca = aIS.getNumberAxes();
    const unsigned cb = aIS.getNumberButtons();
    //;; todo: print only when changed.  ar_log_debug() << "arSharedMemSinkDriver sig is " << cm << "/" << ca << "/" << cb << "\n";
    unsigned i;
    for (i=0; i<cm; i++)
      setMatrix(i, _shmFoB, aIS.getMatrix(i));
    for (i=0; i<ca; i++)
      setAxis(i, _shmWand, aIS.getAxis(i));

    if (cb > 255)
      ar_log_error() << "arSharedMemSinkDriver: more than 255 buttons.\n";
    int rgbutton[256] = {0};

    // Send data to shm
    _l.lock();

    for (i=0; i<cb; ++i) {
        rgbutton[i] = aIS.getButton(i);
        const int button = rgbutton[i];
        // send only state changes
        if (button != _buttonPrev[i]){
          setButton(i, _shmWand, button);
          _buttonPrev[i] = button;
        }
      }

    _l.unlock();

    // update state changes
    for (i=0; i<8; ++i) {
      _buttonPrev[i] = rgbutton[i];
    }

    ar_usleep(10000); // throttle

    // bug, happened exactly once: "Critical System Error ... killed: process or stack limit exceeded"
  }

  _eventThreadRunning = false;
}
