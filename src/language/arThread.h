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
typedef CRITICAL_SECTION arMutex;

#else

#include <pthread.h>
typedef pthread_mutex_t arMutex;

#endif

void SZG_CALL ar_mutex_init(arMutex*);
void SZG_CALL ar_mutex_lock(arMutex*);
void SZG_CALL ar_mutex_unlock(arMutex*);

// From Walmsley, "Multi-threaded Programming in C++," class MUTEX.
class SZG_CALL arLock {
  public:
    arLock(const char* name = NULL);
    ~arLock();
    void lock();
    bool tryLock();
    void unlock();
    bool valid() const;
  protected:
#ifdef AR_USE_WIN_32
    HANDLE _mutex;
#else
    pthread_mutex_t _mutex;
#endif
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
  ~arSignalObject();
  void sendSignal();
  void receiveSignal();
  void reset();

private:

#ifdef AR_USE_WIN_32
  HANDLE _theEvent;
#else
  pthread_mutex_t _theMutex;
  pthread_cond_t  _theConditionVariable;
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
  bool wait(arMutex* externalLock, int timeout = -1);
  void signal();       // semantics of pthread_cond_signal
 private:
#ifdef AR_USE_WIN_32
  arMutex _countLock;
  int    _numberWaiting;
  HANDLE _theEvent;
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
  ~arThread() {}

  arThreadID getThreadID();
  bool beginThread(void (*threadFunction)(void*),void* parameter=NULL);

private:
  void (*_threadFunction)(void*);
  void* _parameter;
  arThreadID _threadID;
  arSignalObject _signal; // avoid race condition during initialization
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

#endif
