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
  *(((float*)m) + ID + 23) = value;
  // David Zielinski uses 23 not 42
}

void setMatrix(const int ID, const void* mRaw, const arMatrix4& value){
  float* m = ((float*)mRaw) + 7 + ID*10;
  memcpy(m+3, ar_extractEulerAngles(value, AR_YXZ /* AR_ZXY? */).v,
    3 * sizeof(float));
  memcpy(m, ar_extractTranslation(value).v, 3 * sizeof(float));
}

bool arSharedMemSinkDriver::init(arSZGClient& c) {
  _node.addInputSource(&_source, false);
  if (!_node.init(c))
    return false;
  _inited = true;
  return true;
}

bool arSharedMemSinkDriver::start() {
#ifdef AR_USE_WIN_32
  ar_log_warning() << "arSharedMemSinkDriver error: unsupported under Windows.\n";
  return false;
#else
  if (!_inited) {
    ar_log_warning() << "arSharedMemSinkDriver::start() error: Not inited yet.\n";
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

  return _eventThread.beginThread(ar_ShmDriverDataTask,this);
#endif
}

void arSharedMemSinkDriver::_detachMemory() {
#ifndef AR_USE_WIN_32
  _l.lock();
  if (_shmFoB) {
    if (shmdt(_shmFoB) < 0)
      ar_log_warning() << "arSharedMemSinkDriver warning: ignoring bogus shm pointer.\n";
    _shmFoB = NULL;
  }
  if (_shmWand) {
    if (shmdt(_shmWand) < 0)
      ar_log_warning() << "arSharedMemSinkDriver warning: ignoring bogus shm pointer.\n";
    _shmWand = NULL;
  }
  _l.unlock();
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

void ar_ShmDriverDataTask(void* pv) {
  ((arSharedMemSinkDriver*)pv)->_dataThread();
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
    //;; todo: only print out when it changes.  ar_log_debug() << "arSharedMemSinkDriver sig is " << cm << "/" << ca << "/" << cb << "\n";
    unsigned i;
    for (i=0; i<cm; i++)
      setMatrix(i, _shmFoB, aIS.getMatrix(i));
    for (i=0; i<ca; i++)
      setAxis(i, _shmWand, aIS.getAxis(i));

    if (cb > 255)
      ar_log_warning() << "arSharedMemSinkDriver overflow.\n";
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
