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
#include "arSharedMemDriver.h"

// Methods used by the dynamic library mappers. 
extern "C"{
  SZG_CALL void* factory(){
    return new arSharedMemDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

arSharedMemDriver::arSharedMemDriver() :
  _inited( false ),
  _stopped( true ),
  _eventThreadRunning( false ),
  _shmHead(NULL),
  _shmWand(NULL)
{
  ar_mutex_init(&_lockShm);
}

arSharedMemDriver::~arSharedMemDriver() {
  _detachMemory(); // paranoid
}

inline int generateButton(int ID, const void* m){
  return *(((int*)m) + ID + 10);
}

inline float generateAxis(int ID, const void* m){
  return *(((float*)m) + ID + 23);
  // David Zielinski <djzielin@duke.edu> uses 23 not 42
}

inline arMatrix4 generateMatrix(const int ID, const void* mRaw){
  float mLocal[6];
  float* m = (float*)mRaw;
  if (ID == 1) {
    // The offset for the head matrix seems to occur in the vrco trackd.
    memcpy(mLocal, m + 7 + ID*10, sizeof(mLocal));
    m = mLocal;
    m[3] += 15;
    if (m[3] < -180.)
      m[3] += 360.;
    else if (m[3] > 180.)
      m[3] -= 360.;

    m[5] += 80.;
    if (m[5] < -180.)
      m[5] += 360.;
    else if (m[5] > 180.)
      m[5] -= 360.;
  }
  return ar_translationMatrix(arVector3(m[0], m[1], m[2])) *
    ar_rotationMatrix('y', ar_convertToRad(m[3])) *
    ar_rotationMatrix('x', ar_convertToRad(m[4])) *
    ar_rotationMatrix('z', ar_convertToRad(m[5]));
}



bool arSharedMemDriver::init(arSZGClient&) {
  _inited = true;
  _setDeviceElements(8, 2, 3);
  cout << "do addFilter \n"; //;; arPForthFilter
  cerr << "arSharedMemDriver remark: initialized.\n";
  return true;
}

bool arSharedMemDriver::start() {
#ifdef AR_USE_WIN_32
  cerr << "arSharedMemDriver error: unsupported under Windows.\n";
  return false;
#else
  if (!_inited) {
    cerr << "arSharedMemDriver::start() error: Not inited yet.\n";
    return false;
  }

  ar_mutex_lock(&_lockShm);
  const int trackerSharedID = shmget(4136, 0, 0666);
  if (trackerSharedID < 0){
    perror("no shm segment for head (try ipcs -m;  run a cavelib app first)");
    return false;
  }
  const int wandSharedID = shmget(4127, 0, 0666);
  if (wandSharedID < 0){
    perror("no shm segment for wand (try ipcs -m;  run a cavelib app first)");
    return false;
  }
  _shmHead = shmat(trackerSharedID, (char*)0, 0);
  if ((int)_shmHead == -1){
    perror("shmat failed for head");
    return false;
  }
  _shmWand = shmat(wandSharedID, (char*)0, 0);
  if ((int)_shmWand == -1){
    perror("shmat failed for wand");
    return false;
  }
  ar_mutex_unlock(&_lockShm);

  return _eventThread.beginThread(ar_ShmDriverDataTask,this);
#endif
}

void arSharedMemDriver::_detachMemory() {
#ifndef AR_USE_WIN_32
  ar_mutex_lock(&_lockShm);
  if (_shmHead) {
    if (shmdt(_shmHead) < 0)
      cerr << "arSharedMemDriver warning: ignoring bogus shm pointer.\n";
    _shmHead = NULL;
  }
  if (_shmWand) {
    if (shmdt(_shmWand) < 0)
      cerr << "arSharedMemDriver warning: ignoring bogus shm pointer.\n";
    _shmWand = NULL;
  }
  ar_mutex_unlock(&_lockShm);
#endif
}

bool arSharedMemDriver::stop() {
  if (_stopped)
    return true;

  _stopped = true;
  _detachMemory();
  while (_eventThreadRunning)
    ar_usleep(10000);

  return true;
}

void ar_ShmDriverDataTask(void* pv) {
  ((arSharedMemDriver*)pv)->_dataThread();
}

void arSharedMemDriver::_dataThread() {
  _stopped = false;
  _eventThreadRunning = true;
  memset(_buttonPrev, 0, sizeof(_buttonPrev));
  while (!_stopped && _eventThreadRunning) {
    ar_mutex_lock(&_lockShm);
    queueMatrix(0, generateMatrix(0, _shmHead));
    queueMatrix(1, generateMatrix(1, _shmHead));
    queueMatrix(2, generateMatrix(2, _shmHead));
    queueAxis(0, generateAxis(0, _shmWand));
    queueAxis(1, generateAxis(1, _shmWand));

  //    printf("Matrix head: \n");
  //    cout << generateMatrix(0, _shmHead) << endl;
  //    printf("\nMatrix wand: \n");
  //    cout << generateMatrix(1, _shmHead) << endl;
  //    printf("\nAxes %.2f %.2f\n",
  //      generateAxis(0, _shmWand), generateAxis(1, _shmWand));

    for (int i=0; i<8; i++){
	const int button = generateButton(i, _shmWand);
	// send only state changes
	if (button != _buttonPrev[i]){
	  queueButton(i, button);
	  _buttonPrev[i] = button;

  //    printf("\tButton %d %s\n", i, button ? "ON!" : "off");
	}
    }

    ar_mutex_unlock(&_lockShm);
    sendQueue();
    ar_usleep(10000); // throttle
  }
  _eventThreadRunning = false;
}

bool arSharedMemDriver::restart() {
  return stop() && start();
}
