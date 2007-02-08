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
#include "arDataUtilities.h"
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
  ar_mutex_init(&_lockShm);
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

bool arSharedMemSinkDriver::init(arSZGClient&) {
  _inited = true;
  //;; _setDeviceElements(8, 2, 3);
  return true;
}

bool arSharedMemSinkDriver::start() {
#ifdef AR_USE_WIN_32
  cerr << "arSharedMemSinkDriver error: unsupported under Windows.\n";
  return false;
#else
  if (!_inited) {
    cerr << "arSharedMemSinkDriver::start() error: Not inited yet.\n";
    return false;
  }

  // TrackerDaemonKey 4136
  // ControllerDaemonKey 4127
  ar_mutex_lock(&_lockShm);
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
  ar_mutex_unlock(&_lockShm);

  return _eventThread.beginThread(ar_ShmDriverDataTask,this);
#endif
}

void arSharedMemSinkDriver::_detachMemory() {
#ifndef AR_USE_WIN_32
  ar_mutex_lock(&_lockShm);
  if (_shmFoB) {
    if (shmdt(_shmFoB) < 0)
      cerr << "arSharedMemSinkDriver warning: ignoring bogus shm pointer.\n";
    _shmFoB = NULL;
  }
  if (_shmWand) {
    if (shmdt(_shmWand) < 0)
      cerr << "arSharedMemSinkDriver warning: ignoring bogus shm pointer.\n";
    _shmWand = NULL;
  }
  ar_mutex_unlock(&_lockShm);
#endif
}

bool arSharedMemSinkDriver::stop() {
  if (_stopped)
    return true;
  _stopped = true;

  _detachMemory();
  while (_eventThreadRunning)
    ar_usleep(10000);
  return true;
}

void ar_ShmDriverDataTask(void* pv) {
  ((arSharedMemSinkDriver*)pv)->_dataThread();
}

void arSharedMemSinkDriver::_dataThread() {
  _stopped = false;
  _eventThreadRunning = true;
  memset(_buttonPrev, 0, sizeof(_buttonPrev));

#ifdef worked_sorta_but_sent_only_to_cassatt_not_syzygy
  // ;;;; Start reading data from FoB, a la DeviceClient
  arInputNode fobNode;
  arNetInputSource netInputSource;
  netInputSource.setSlot(42);;;;
#endif
#if 0
  if (!fobNode.init(SZGClient in DeviceServer))
    cerr << "arSharedMemSinkDriver warning: FoB init failed.\n";
  else if (!fobNode.start(SZGClient in DeviceServer))
    cerr << "arSharedMemSinkDriver warning: FoB start failed.\n";
#endif

  // todo: Init reading data from USB-wand, a la DeviceClient.

  while (!_stopped && _eventThreadRunning) {

#ifdef not_yet_dude //;;;;
    // Read data from FoB
    arInputState fob = fobNode._inputState; // local copy
    const unsigned cm = fob.getNumberMatrices();
    if (fob.getNumberButtons() != 0 || fob.getNumberAxes() != 0 || fob.getNumberMatrices() == 0)
      cerr << "arSharedMemSinkDriver warning: FoB has bad signature ?/?/?.\n";
    for (int i=0; i<cm; i++)
      cout << "matrix " << i << ": " << fob.getMatrix(i) << endl;
#endif

    // Get data from devices.

    // ;;todo: Read data from fobNode.
    // ;;todo: Read data from USB-wand, a la DeviceClient.

    const arMatrix4 rgm[3] = {
      ar_translationMatrix(0., 4.5, 3.+3.*drand48()),
      ar_translationMatrix(0., 4. + 1.*drand48(), 0.),
      ar_identityMatrix()
    };; // test pattern
    const float rgv[2] = { -0.03, 0.03 };; // test pattern
    /*
    static int _ = 0; if (++_ > 1000000) _ = 0;
    */
    const int rgbutton[8] = {
      /* _ &   1 ? 1 : 0,
      _ &   2 ? 1 : 0,
      _ &   4 ? 1 : 0,
      _ &   8 ? 1 : 0,
      _ &  16 ? 1 : 0,
      _ &  32 ? 1 : 0,
      _ &  64 ? 1 : 0,
      _ & 128 ? 1 : 0 */ 0,0,0,0,0,0,0,0 };; // test pattern

/*
    // Send data to Phleet.
    queueMatrix(0, rgm[0]);
    queueMatrix(1, rgm[1]);
    queueMatrix(2, rgm[2]);
    queueAxis(0, rgv[0]);
    queueAxis(1, rgv[1]);
    for (int i=0; i<8; i++){
      const int button = rgbutton[i];
      // send only state changes
      if (button != _buttonPrev[i]){
        queueButton(i, button);
      }
    }
    sendQueue();
*/

    // Send data to shm
    ar_mutex_lock(&_lockShm);

      setMatrix(0, _shmFoB, rgm[0]);
      setMatrix(1, _shmFoB, rgm[1]);
      setMatrix(2, _shmFoB, rgm[2]);

      setAxis(0, _shmWand, rgv[0]);;
      setAxis(1, _shmWand, rgv[1]);;

      for (int i=0; i<8; i++){
	const int button = rgbutton[i];
	// send only state changes
	if (button != _buttonPrev[i]){
	  setButton(i, _shmWand, button);
	  _buttonPrev[i] = button;
	}
      }

    ar_mutex_unlock(&_lockShm);

    // update state changes
    for (int i=0; i<8; i++)
      _buttonPrev[i] = rgbutton[i];

    ar_usleep(10000); // throttle

    // bug, happened exactly once: "Critical System Error ... killed: process or stack limit exceeded"
  }

  _eventThreadRunning = false;
}

// Now it works:  testdata sent to phleet,
// testdata fights with tracker.beckman for matrices,
// testdata fights with wand.beckman for buttons and joystick.
