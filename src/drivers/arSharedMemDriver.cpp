//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSharedMemDriver.h"

#ifndef AR_USE_WIN_32
#include <sys/shm.h>
#endif
#include <string>

DriverFactory(arSharedMemDriver, "arInputSource")

arSharedMemDriver::arSharedMemDriver() :
  _inited( false ),
  _stopped( true ),
  _eventThreadRunning( false ),
  _shmFoB(NULL),
  _shmWand(NULL)
{
}

arSharedMemDriver::~arSharedMemDriver() {
  _detachMemory(); // paranoid
}

inline int generateButton(int ID, const void* m) {
  return *(((int*)m) + ID + 10);
}

inline float generateAxis(int ID, const void* m) {
  return *(((float*)m) + ID + 23);
  // David Zielinski <djzielin@duke.edu> uses 23 not 42
}

arMatrix4 generateMatrix(const int ID, const void* mRaw) {
  float* m = ((float*)mRaw) + 7 + ID*10;
  float mLocal[6];
  if (ID == 1) {
    // Compute wand matrix's offset.
    // The offset for the head matrix seems to occur in the vrco trackd.
    memcpy(mLocal, m, sizeof(mLocal));
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
  return ar_translationMatrix(arVector3(m)) *
    ar_rotationMatrix('y', ar_convertToRad(m[3])) *
    ar_rotationMatrix('x', ar_convertToRad(m[4])) *
    ar_rotationMatrix('z', ar_convertToRad(m[5]));
}

bool arSharedMemDriver::init(arSZGClient&) {
  _inited = true;
  _setDeviceElements(8, 2, 3);
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

  _l.lock();
  const int idFoB = shmget(4136, 0, 0666);
  if (idFoB < 0) {
    perror("no shm segment for Flock of Birds (try ipcs -m;  run a cavelib app first)");
    return false;
  }
  const int idWand = shmget(4127, 0, 0666);
  if (idWand < 0) {
    perror("no shm segment for wand (try ipcs -m;  run a cavelib app first)");
    return false;
  }
  _shmFoB = shmat(idFoB, NULL, 0);
  if ((int)_shmFoB == -1) {
    perror("shmat failed for Flock of Birds");
    return false;
  }
  _shmWand = shmat(idWand, NULL, 0);
  if ((int)_shmWand == -1) {
    perror("shmat failed for wand");
    return false;
  }
  _l.unlock();

  return _eventThread.beginThread(ar_ShmDriverDataTask, this);
#endif
}

void arSharedMemDriver::_detachMemory() {
#ifndef AR_USE_WIN_32
  arGuard dummy(_l);
  if (_shmFoB) {
    if (shmdt(_shmFoB) < 0)
      cerr << "arSharedMemDriver warning: ignoring bogus shm pointer.\n";
    _shmFoB = NULL;
  }
  if (_shmWand) {
    if (shmdt(_shmWand) < 0)
      cerr << "arSharedMemDriver warning: ignoring bogus shm pointer.\n";
    _shmWand = NULL;
  }
#endif
}

bool arSharedMemDriver::stop() {
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
  ((arSharedMemDriver*)pv)->_dataThread();
}

void arSharedMemDriver::_dataThread() {
  _stopped = false;
  _eventThreadRunning = true;
  memset(_buttonPrev, 0, sizeof(_buttonPrev));
  while (!_stopped && _eventThreadRunning) {
    _l.lock();
    queueMatrix(0, generateMatrix(0, _shmFoB));
    queueMatrix(1, generateMatrix(1, _shmFoB));
    queueMatrix(2, generateMatrix(2, _shmFoB));
    queueAxis(0, generateAxis(0, _shmWand));
    queueAxis(1, generateAxis(1, _shmWand));

    for (int i=0; i<8; i++) {
	const int button = generateButton(i, _shmWand);
	// send only state changes
	if (button != _buttonPrev[i]) {
	  queueButton(i, button);
	  _buttonPrev[i] = button;
	}
    }

    _l.unlock();
    sendQueue();
    ar_usleep(10000); // throttle
  }
  _eventThreadRunning = false;
}
