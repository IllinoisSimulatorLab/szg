//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <iostream>
#include <errno.h>
#include "arThread.h"
#include "arDataUtilities.h"
using namespace std;

void ar_mutex_init(arMutex* theMutex){
#ifdef AR_USE_WIN_32
	// This is probably no more tolerant of repeated calls than
	// pthread_mutex_init is (see below).
	InitializeCriticalSection(theMutex);
#else
	// "Attempting to initialise an already initialised mutex
	// results in undefined behaviour." -- man page.
	pthread_mutex_init(theMutex,NULL);
#endif
}

void ar_mutex_lock(arMutex* theMutex){
#ifdef AR_USE_WIN_32
	EnterCriticalSection(theMutex);
#else
	pthread_mutex_lock(theMutex);
#endif
}

void ar_mutex_unlock(arMutex* theMutex){
#ifdef AR_USE_WIN_32
	LeaveCriticalSection(theMutex);
#else
	pthread_mutex_unlock(theMutex);
#endif
}

arConditionVar::arConditionVar(){
#ifdef AR_USE_WIN_32
  _numberWaiting = 0;
  ar_mutex_init(&_countLock);
  _theEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
#else
  pthread_cond_init(&_conditionVar,NULL);
#endif
}

arConditionVar::~arConditionVar(){
#ifdef AR_USE_WIN_32
  CloseHandle(_theEvent);
#else
  // nothing to do, I think
#endif
}

/// Wait for the condition variable to be signaled. By default, timeout is
/// not used, but, if specified, wait for that number of milliseconds.
/// (the default value passed for timeout is -1). If we have returned because
/// we were signaled, return true. If we have returned because of timeout,
/// return false.
bool arConditionVar::wait(arMutex* externalLock, int timeout){
  bool state = true;
#ifdef AR_USE_WIN_32
  ar_mutex_lock(&_countLock);
  _numberWaiting++;
  ar_mutex_unlock(&_countLock);

  ar_mutex_unlock(externalLock);
  if (timeout >= 0){
    if (WaitForSingleObject(_theEvent, timeout) == WAIT_TIMEOUT){
      state = false;
    }
  }
  else{
    WaitForSingleObject(_theEvent, INFINITE);
  }

  //*************** note that there is a subtle race condition here
  // what if signal() is called right here and gets the lock
  // before things can be decremented? the next wait will get woken
  // spuriously. Does this matter for condition variable semantics?
  // PROBABLY NOT. THE POSIX STANDARDS SUGGEST THAT CONDITION VARS
  // CAN BE SPURIOUSLY WOKEN.

  ar_mutex_lock(&_countLock);
  _numberWaiting--;
  ar_mutex_unlock(&_countLock);

  // HMMMM.... SHOULD THIS BE MOVED ABOVE THE lock of _countLock?
  // It might be a good idea to do so! BE VERY CONSERVATIVE ABOUT THIS,
  // however.
  ar_mutex_lock(externalLock);
#else
  if (timeout >= 0){
    // GRUMBLE... WHY CAN'T PTHREADS JUST USE A WAIT INTERVAL?
    ar_timeval time1 = ar_time();
    time1.sec = time1.sec + timeout/1000;
    time1.usec = time1.usec + (timeout%1000)*1000;
    if (time1.usec >= 1000000){
      time1.sec++;
      time1.usec -= 1000000;
    }
    struct timespec ts;
    ts.tv_sec = time1.sec;
    ts.tv_nsec = time1.usec * 1000;
    if (pthread_cond_timedwait(&_conditionVar,externalLock,&ts) == ETIMEDOUT){
      state = false;
    }
  }
  else{
    pthread_cond_wait(&_conditionVar,externalLock);
  }
#endif
  return state;
}

void arConditionVar::signal(){
#ifdef AR_USE_WIN_32
  ar_mutex_lock(&_countLock);
  if (_numberWaiting > 0){
    SetEvent(_theEvent);
  }
  ar_mutex_unlock(&_countLock);
#else
  pthread_cond_signal(&_conditionVar);
#endif
}
  
  
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
  bool active = _active;
  if (_active && _automatic)
    _active = false;
  pthread_mutex_unlock( &_mutex );
  return active;
#endif
}


arWrapperType ar_threadWrapperFunction(void* threadObject)
{
  arThread* theThread = (arThread*)threadObject;

  // 1. Make a local copy of the data we need.
  // 2. Then sendSignal to allow the remote copy in *theThread to be
  //    deleted if needed (because *theThread will be deleted, because
  //    it's a local variable of exceedingly tiny scope),
  // 3. Then use the local copy.

  void (*threadFunction)(void*) = theThread->_threadFunction;
  void* parameter = theThread->_parameter;
  theThread->_signal.sendSignal();
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

arThreadID arThread::getThreadID()
{
  return _threadID;
}

bool arThread::beginThread(void (*threadFunction)(void*),void* parameter){
#ifdef AR_USE_WIN_32

  _threadID = _beginthread(threadFunction,0,parameter);
  if (_threadID < 0)
    {
    cerr << "arThread error: _beginthread failed.\n";
    if (errno == EAGAIN)
      cerr << "  ...too many threads.\n";
    else if (errno == EINVAL)
      cerr << "  ...invalid argument, win32 claims.\n";
    return false;
    }

#else

  _threadFunction = threadFunction;
  _parameter = parameter;
  int error = pthread_create(&_threadID,NULL,ar_threadWrapperFunction, this);
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
      cerr << "arThread error: pthread_create failed: possibly too many threads, "
	   << "more than PTHREAD_THREADS_MAX.\n";
    return false;
    }

#endif
  return true;
}

arSignalObject::arSignalObject(){
#ifdef AR_USE_WIN_32
  // this creates a auto-reset event object (this means that it resets to
  // unsignaled after a single thread is awakened) with initial state
  // unsignaled
  _theEvent = CreateEvent(NULL, false, false, NULL);
#else
  pthread_mutex_init(&_theMutex,NULL);
  pthread_cond_init(&_theConditionVariable,NULL);
  _fSync = false;
#endif
}

arSignalObject::~arSignalObject(){
	// nothing to do yet
}

void arSignalObject::sendSignal(){
#ifdef AR_USE_WIN_32 
  // which is more appropriate here? SetEvent or PulseEvent?
  // for an auto-reset event, SetEvent causes the event to remain 
  // signaled until someone waits on it, at which time it is reset
  // for an auto-reset event, PulseEvent releases a single thread and
  // then unsignals the event. Or immediately puts the event in the
  // unsignaled state if no thread is waiting.
  // note that SetEvent has the semantics of the Unix solution using
  // condition vars... so we better use that
  SetEvent(_theEvent);
#else
   pthread_mutex_lock(&_theMutex);
   _fSync = true;
   pthread_cond_signal(&_theConditionVariable);
   pthread_mutex_unlock(&_theMutex);
#endif
}

void arSignalObject::receiveSignal(){
#ifdef AR_USE_WIN_32
   WaitForSingleObject(_theEvent,INFINITE);
#else
   pthread_mutex_lock(&_theMutex);
   while (!_fSync) {
     pthread_cond_wait(&_theConditionVariable,&_theMutex);
   }
   _fSync = false;
   pthread_mutex_unlock(&_theMutex);
#endif
}

void arSignalObject::reset(){
#ifdef AR_USE_WIN_32
  ResetEvent(_theEvent);
#else
  pthread_mutex_lock(&_theMutex);
  _fSync = false;
  pthread_mutex_unlock(&_theMutex);
#endif
}
