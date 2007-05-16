//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_THREAD_H
#define AR_THREAD_H

#include "arLanguageCalling.h"

//******************************************
// mutexes
//******************************************

#ifdef AR_USE_WIN_32

  // needed to make python bindings
  #ifndef HANDLE
  #include "arPrecompiled.h"
  #endif

#include <process.h>

#else

#include <pthread.h>

#endif

// Recursive mutex: called twice by the same thread, doesn't deadlock.
// Loosely from Walmsley, "Multi-threaded Programming in C++," class MUTEX.
class SZG_CALL arLock {
  friend class arConditionVar;
 public:
  arLock(const char* name = NULL);
  ~arLock();
  void lock();
#ifdef UNUSED
  bool tryLock();
#endif
  void unlock();
  bool valid() const;

 protected:
#ifdef AR_USE_WIN_32
  HANDLE _mutex;
  bool _fOwned;
#else
  pthread_mutex_t _mutex;
#endif
};

//**************************************
// thread-safe types
//**************************************

class SZG_CALL arBoolAtom {
 public:
  arBoolAtom(bool x=false) : _x(x) {}
  operator bool() const
    { _l.lock(); const bool x = _x; _l.unlock(); return x; }
  bool set(bool x)
    { _l.lock(); _x = x; _l.unlock(); return x; }
 private:
  bool _x;
  mutable arLock _l;
};

class SZG_CALL arIntAtom {
 public:
  arIntAtom(int x=0) : _x(x) {}
  operator int() const
    { _l.lock(); const int x = _x; _l.unlock(); return x; }
  int set(int x)
    { _l.lock(); _x = x; _l.unlock(); return x; }
  friend int operator++(arIntAtom& a) // prefix operator only
    { a._l.lock(); const int x = ++(a._x); a._l.unlock(); return x; }
  friend int operator--(arIntAtom& a) // prefix operator only
    { a._l.lock(); const int x = --(a._x); a._l.unlock(); return x; }
 private:
  int _x;
  mutable arLock _l;
};

//*****************************************
// signals
//*****************************************

// Signal.
// "Sticky" (like Win32 manual-reset events):
// once signalled, remain so until a
// receiveSignal() call is issued.
// However, sometimes we want to de-signal the object.

class SZG_CALL arSignalObject{
public:
  arSignalObject();
  void sendSignal();
  void receiveSignal();
  void reset();

private:

#ifdef AR_USE_WIN_32
  HANDLE _event;
#else
  pthread_mutex_t _mutex;
  pthread_cond_t  _conditionVar;
  bool            _fSync;
#endif

};

//**************************************
// simple condition variables 
//**************************************

class SZG_CALL arConditionVar{
 public:
  arConditionVar();
  ~arConditionVar();
  bool wait(arLock&, const int msecTimeout = -1);
  void signal();       // semantics of pthread_cond_signal
 private:
#ifdef AR_USE_WIN_32
  arLock _lCount; // guards _numberWaiting and _event
  int _numberWaiting;
  HANDLE _event;
#else
  pthread_cond_t _conditionVar;
#endif
};
  
//**************************************
// event - condition variable with a memory
//**************************************

//Implementation lifted from EVENT class of
// Walmsley, "Multi-threaded Programming in C++"
class SZG_CALL arThreadEvent {
  public:
    arThreadEvent( bool automatic = true );
    ~arThreadEvent();
    void signal();
    void wait();
    void reset();
    bool test();
  private:
#ifdef AR_USE_WIN_32
    HANDLE _event;
#else
    pthread_cond_t _event;
    pthread_mutex_t _mutex;
    bool _automatic;
    bool _active;
#endif
};

//**************************************
// threads
//**************************************

#ifdef AR_USE_WIN_32
  typedef unsigned long arThreadID;
  typedef void arWrapperType;
#else
  typedef pthread_t arThreadID;
  typedef void* arWrapperType;
#endif

// Thread.

class SZG_CALL arThread {
  // Needs assignment operator and copy constructor, for pointer members.
  friend arWrapperType ar_threadWrapperFunction(void*);

public:
  arThread() {}
  arThread(void (*threadFunction)(void*),void* parameter=NULL);

  arThreadID getThreadID() const;
  bool beginThread(void (*threadFunction)(void*),void* parameter=NULL);

private:
  void (*_threadFunction)(void*);
  void* _parameter;
  arThreadID _threadID;
  arSignalObject _signal; // avoid race condition during initialization
};

#endif
