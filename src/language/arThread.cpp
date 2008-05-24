//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataUtilities.h"

#include <errno.h>
#ifdef AR_USE_WIN_32
  #include <iostream>
#endif
using namespace std;

// Originally from Walmsley, "Multi-threaded Programming in C++", class MUTEX.
// In Windows, a (non-NULL) name starting with Global\ makes the lock system-wide.
// In Vista that fails because users don't login as "Session 0", which is required.
#ifdef AR_USE_WIN_32
arLock::arLock(const char* name) : _fOwned(true) {
  _mutex = CreateMutex(NULL, FALSE, name);
  const DWORD e = GetLastError();
  if (e == ERROR_ALREADY_EXISTS && _mutex) {
    // Another app has this.  (Only do this with global things like arLogStream.)
    _fOwned = false;
  }

  if (!_mutex) {
    if (!name) {
      cerr << "arLock error: CreateMutex failed, GetLastError() == " << e << ".\n";
      // valid() now fails.
      return;
    }

    // name != NULL.

    if (e == ERROR_ALREADY_EXISTS) {
      cerr << "arLock warning: CreateMutex('" << name <<
        "') failed (already exists).\n";
      return;
    }
    if (e == ERROR_ACCESS_DENIED) {
      cerr << "arLock warning: CreateMutex('" << name <<
        "') failed (access denied); backing off.\n";
LBackoff:
      // _mutex = OpenMutex(SYNCHRONIZE, FALSE, name);
      // Fall back to a mutex of scope "app" not "the entire PC".
      _mutex = CreateMutex(NULL, FALSE, NULL);
      if (!_mutex) {
	cerr << "arLock warning: failed to create mutex.\n";
      }
    }
    else if (e == ERROR_PATH_NOT_FOUND) {
      cerr << "arLock warning: CreateMutex('" << name <<
        "') failed (backslash?); backing off.\n";
      goto LBackoff;
    }
    else {
      cerr << "arLock warning: CreateMutex('" << name <<
        "') failed; backing off.\n";
      goto LBackoff;
    }
  }
}
#else
arLock::arLock(const char*) {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  // Unlike PTHREAD_MUTEX_DEFAULT, don't deadlock if _mutex is locked twice.
  // Unlike PTHREAD_MUTEX_NORMAL, check for usage errors like unlocking by the wrong thread.
  pthread_mutex_init(&_mutex, &attr);
  pthread_mutexattr_destroy(&attr);
}
#endif

bool arLock::valid() const {
#ifdef AR_USE_WIN_32
  return _mutex != NULL;
#else
  return true;
#endif
}

arLock::~arLock() {
#ifdef AR_USE_WIN_32
  if (_fOwned && _mutex) {
    (void)ReleaseMutex(_mutex); // paranoid
    CloseHandle(_mutex);
  }
#else
  pthread_mutex_destroy( &_mutex );
#endif
}

void arLock::lock() {
#ifdef AR_USE_WIN_32
  if (!valid()) {
    cerr << "arLock warning: internal error.\n";
    return;
  }

  const DWORD msecTimeout = 3000;
  for (;;) {
    const DWORD r = WaitForSingleObject( _mutex, msecTimeout );
    switch (r) {
    case WAIT_OBJECT_0:
      _fLocked = true;
      return;
    default:
    case WAIT_ABANDONED:
      // Another thread terminated without releasing _mutex.
      cerr << "arLock warning: acquired abandoned lock.\n";
      return;
    case WAIT_TIMEOUT:
      cerr << "arLock warning: retrying timed-out lock().\n";
      break;
    case WAIT_FAILED:
      const DWORD e = GetLastError();
      if (e == ERROR_INVALID_HANDLE) {
	cerr << "arLock warning: invalid handle.\n";
	// _mutex is bad, so stop using it.
	_mutex = NULL;
	// Desperate fallback: create a fresh (unnamed) mutex.
	_mutex = CreateMutex(NULL, FALSE, NULL);
	if (_mutex)
	  continue;
	cerr << "arLock warning: failed to recreate handle.  Unrecoverable.\n";
      }
      else {
	cerr << "arLock warning: internal error, GetLastError() == " << e << ".\n";
      }
      return;
    }
  }
#else
  arSleepBackoff a(2, 300, 1.1); // Slower than a pthread_mutex_timedlock_np().  Yuck.
  for (;;) {
    switch (pthread_mutex_trylock(&_mutex)) {
    case 0:
    default:
      _fLocked = true;
      return;
    case EBUSY:
      a.sleep();
      if (a.msecElapsed() > 3000.) {
	cerr << "arLock warning: retrying timed-out lock().\n";
	a.resetElapsed();
      }
      break;
    case EINVAL:
      cerr << "arLock warning: uninitialized.\n";
      return;
    case EFAULT:
      cerr << "arLock warning: invalid pointer.\n";
      return;
    }
  }

#endif
}

#ifdef UNUSED
// Try to lock().  Returns true also if "you" lock()'ed already.
bool arLock::tryLock() {
  // Don't test _fLocked.  It may have been set by someone else.
#ifdef AR_USE_WIN_32
  // BUG: this just checks if it was locked already, it doesn't actually LOCK it.
  return WaitForSingleObject( _mutex, 0 ) != WAIT_TIMEOUT;
#else
  return pthread_mutex_trylock( &_mutex ) == 0;
  // if != 0, may equal EBUSY (locked already), EINVAL, or EFAULT.
#endif
}
#endif

void arLock::unlock() {
#ifdef AR_USE_WIN_32
  if (!ReleaseMutex(_mutex)) {
    cerr << "arLock warning: failed to unlock.\n";
    CloseHandle(_mutex);
    _mutex = NULL;
  };
#else
  pthread_mutex_unlock( &_mutex );
#endif
}

arConditionVar::arConditionVar(){
#ifdef AR_USE_WIN_32
  _numberWaiting = 0;
  _event = CreateEvent(NULL,FALSE,FALSE,NULL);
#else
  pthread_cond_init(&_conditionVar,NULL);
#endif
}

arConditionVar::~arConditionVar(){
#ifdef AR_USE_WIN_32
  CloseHandle(_event);
#else
  pthread_cond_destroy(&_conditionVar);
#endif
}

// Wait for the condition variable to be signaled.
// Return true if signaled, false if timed out.
bool arConditionVar::wait(arLock& l, const int msecTimeout){
#ifdef AR_USE_WIN_32

  _lCount.lock();
    ++_numberWaiting;
  _lCount.unlock();
  l.unlock();
  bool ok = true;
  if (msecTimeout >= 0){
    ok = WaitForSingleObject(_event, msecTimeout) != WAIT_TIMEOUT;
  }
  else{
    WaitForSingleObject(_event, INFINITE);
  }

  // Subtle race condition: if signal() is called here and gets the lock
  // before --_numberWaiting, the next wait will be woken spuriously.
  // Might not matter for condition variable semantics:
  // POSIX suggests that condition vars can be spuriously woken.

  _lCount.lock();
    --_numberWaiting;
  _lCount.unlock();

  l.lock();
  return ok;

#else

  if (msecTimeout < 0){
    pthread_cond_wait(&_conditionVar, &l._mutex);
    return true;
  }

  // Workaround for pthreads' lack of wait intervals.
  ar_timeval time1 = ar_time();
  // todo: define a function to add a pure msec to an ar_timeval.  At least 3 places would use it.  Grep for 1000000 and usec.
  time1.sec += msecTimeout/1000;
  time1.usec += (msecTimeout%1000)*1000;
  if (time1.usec >= 1000000){
    time1.usec -= 1000000;
    time1.sec++;
  }
  struct timespec ts;
  ts.tv_sec = time1.sec;
  ts.tv_nsec = time1.usec * 1000;
  return pthread_cond_timedwait(&_conditionVar, &l._mutex, &ts) != ETIMEDOUT;

#endif
}

void arConditionVar::signal(){
#ifdef AR_USE_WIN_32
  _lCount.lock();
  if (_numberWaiting > 0){
    SetEvent(_event);
  }
  _lCount.unlock();
#else
  pthread_cond_signal(&_conditionVar);
#endif
}
  
// From Walmsley, "Multi-threaded Programming in C++", class EVENT.

arThreadEvent::arThreadEvent( bool automatic ) {
#ifdef AR_USE_WIN_32
  _event = CreateEvent( NULL, (BOOL)!automatic, FALSE, NULL );
#else
  pthread_cond_init( &_event, (pthread_condattr_t*)NULL );
  pthread_mutex_init( &_mutex, (pthread_mutexattr_t*)NULL );
  _automatic = automatic;
  _active = false;
#endif
}

arThreadEvent::~arThreadEvent() {
#ifdef AR_USE_WIN_32
  CloseHandle( _event );
#else
  pthread_cond_destroy( &_event );
  pthread_mutex_destroy( &_mutex );
#endif
}

void arThreadEvent::signal() {
#ifdef AR_USE_WIN_32
  SetEvent( _event );
#else
  pthread_mutex_lock( &_mutex );
  _active = true;
  if (_automatic)
    pthread_cond_signal( &_event );
  else
    pthread_cond_broadcast( &_event );
  pthread_mutex_unlock( &_mutex );
#endif
}

void arThreadEvent::wait() {
#ifdef AR_USE_WIN_32
  WaitForSingleObject( _event, INFINITE );
#else
  pthread_mutex_lock( &_mutex );
  while (!_active)
    pthread_cond_wait( &_event, &_mutex );
  if (_automatic)
    _active = false;
  pthread_mutex_unlock( &_mutex );
#endif
}

void arThreadEvent::reset() {
#ifdef AR_USE_WIN_32
  ResetEvent( _event );
#else
  pthread_mutex_lock( &_mutex );
  _active = false;
  pthread_mutex_unlock( &_mutex );
#endif
}

bool arThreadEvent::test() {
#ifdef AR_USE_WIN_32
  return WaitForSingleObject( _event, 0 ) != WAIT_TIMEOUT;
#else
  pthread_mutex_lock( &_mutex );
  const bool active = _active;
  if (_automatic)
    _active = false;
  pthread_mutex_unlock( &_mutex );
  return active;
#endif
}

arWrapperType ar_threadWrapperFunction(void* threadObject)
{
  // Local copy.
  arThread* theThread = (arThread*)threadObject;
  void (*threadFunction)(void*) = theThread->_threadFunction;
  void* parameter = theThread->_parameter;

  // Let the remote copy in *theThread be deleted if needed
  // (because *theThread will be deleted, because its scope is tiny).
  theThread->_signal.sendSignal();

  // Use the local copy.
  threadFunction(parameter);
#ifndef AR_USE_WIN_32
  return NULL;
#endif
}

arThread::arThread(void (*threadFunction)(void*),void* parameter){
  // This usage is dangerous because if beginThread fails,
  // the caller cannot know about it.
  // It may be acceptable in main(), but avoid it in the deep guts of syzygy.
  (void)beginThread(threadFunction, parameter);
}

arThreadID arThread::getThreadID() const
{
  return _threadID;
}

bool arThread::beginThread(void (*threadFunction)(void*),void* parameter){
#ifdef AR_USE_WIN_32

  _threadID = _beginthread(threadFunction,0,parameter);
  // Weird. On error, _beginthread returns an unsigned minus one.
  if (_threadID == arThreadID(-1L))
    {
    cerr << "arThread error: _beginthread failed: ";
    if (errno == EAGAIN)
      cerr << "too many threads.\n";
    else if (errno == EINVAL)
      cerr << "invalid argument.\n";
    return false;
    }

#else

  _threadFunction = threadFunction;
  _parameter = parameter;
  const int error = pthread_create(&_threadID,NULL,ar_threadWrapperFunction, this);
  //*************************************************************************
  // the windows threads are formed automatically in a detached stated
  // so let's do the same thing on this side 
  //*************************************************************************
  pthread_detach(_threadID);
  _signal.receiveSignal();
  if (error)
    {
    cerr << "arThread error: pthread_create failed.\n";
    if (error == EAGAIN)
      cerr << "arThread error: pthread_create failed: PTHREAD_THREADS_MAX exceeded.\n";
    return false;
    }

#endif
  return true;
}

arSignalObject::arSignalObject(){
#ifdef AR_USE_WIN_32
  // Create an auto-reset event object, which resets to
  // unsignaled after a single thread is awakened,
  // with initial state unsignaled.
  _event = CreateEvent(NULL, false, false, NULL);
#else
  pthread_mutex_init(&_mutex,NULL);
  pthread_cond_init(&_conditionVar,NULL);
  _fSync = false;
#endif
}

void arSignalObject::sendSignal(){
#ifdef AR_USE_WIN_32 
  // Which is more appropriate here? SetEvent or PulseEvent?
  // For an auto-reset event, SetEvent leaves the event 
  // signaled until someone waits on it, at which time it is reset
  // for an auto-reset event. PulseEvent releases a single thread and
  // then unsignals the event. Or immediately unsignals the event
  // if no thread is waiting.
  //
  // SetEvent has the semantics of the Unix solution using
  // condition vars, so we use that.
  SetEvent(_event);
#else
   pthread_mutex_lock(&_mutex);
   _fSync = true;
   pthread_cond_signal(&_conditionVar);
   pthread_mutex_unlock(&_mutex);
#endif
}

void arSignalObject::receiveSignal(){
#ifdef AR_USE_WIN_32
   WaitForSingleObject(_event,INFINITE);
#else
   pthread_mutex_lock(&_mutex);
   while (!_fSync) {
     pthread_cond_wait(&_conditionVar,&_mutex);
   }
   _fSync = false;
   pthread_mutex_unlock(&_mutex);
#endif
}

void arSignalObject::reset(){
#ifdef AR_USE_WIN_32
  ResetEvent(_event);
#else
  pthread_mutex_lock(&_mutex);
  _fSync = false;
  pthread_mutex_unlock(&_mutex);
#endif
}
