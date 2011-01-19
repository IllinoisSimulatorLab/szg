//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_THREAD_H
#define AR_THREAD_H

#include "arLanguageCalling.h"

#include <string>

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
  virtual ~arLock();
  void lock(const char* locker = NULL);
  void unlock();
  bool valid() const;
  bool locked() const { return _fLocked; }
#ifdef UNUSED
  bool tryLock();
#endif

 protected:
#ifdef AR_USE_WIN_32
  HANDLE _mutex;
  bool _fOwned; // Owned by this app.  Does NOT mean "locked."
#else
  pthread_mutex_t _mutex;
#endif
  void _setName( const char* );
 private:
  char* _name;
  const char* _locker; // name of thread that has the lock
  bool _fLocked;
  void _logretry( const char* );
};

class arGlobalLock : public arLock {
 public:
  arGlobalLock( const char* name );
};

// Lock that implicitly unlocks when out of scope.
class SZG_CALL arGuard {
 public:
  arGuard(arLock& l, const char* name = "anonymous arGuard"): _l(l)
    { _l.lock(name); }
  ~arGuard()
    { _l.unlock(); }
 private:
  arLock& _l;
};

//**************************************
// thread-safe types
// operators ++ and -- save these classes from being sheer paranoia
//**************************************

class SZG_CALL arBoolAtom {
 public:
  arBoolAtom(bool x=false) : _x(x), _l("arBoolAtom") {}
  arBoolAtom& operator=(const bool x)
    { arGuard g(_l, "operator="); _x = x; return *this; }
  operator bool() const
    { arGuard g(_l, "bool()"); const bool x = _x; return x; }
  bool set(bool x)
    { arGuard g(_l, "set"); _x = x; return x; }
 private:
  bool _x;
  mutable arLock _l;
};

class SZG_CALL arIntAtom {
 public:
  arIntAtom(int x=0) : _x(x), _l("arIntAtom") {}
  arIntAtom& operator=(const int x)
    { arGuard g(_l, "operator="); _x = x; return *this; }
  operator int() const
    { arGuard g(_l, "int()"); const int x = _x; return x; }
  int set(int x)
    { arGuard g(_l, "set"); _x = x; return x; }
  friend int operator++(arIntAtom& a) // prefix operator only
    { arGuard g(a._l, "++"); const int x = ++(a._x); return x; }
  friend int operator--(arIntAtom& a) // prefix operator only
    { arGuard g(a._l, "--"); const int x = --(a._x); return x; }
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

class SZG_CALL arConditionVar {
 public:
  arConditionVar(const std::string&);
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
  const std::string _threadName;
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
  arThread(void (*threadFunction)(void*), void* parameter=NULL);

  arThreadID getThreadID() const;
  bool beginThread(void (*threadFunction)(void*), void* parameter=NULL);

private:
  void (*_threadFunction)(void*);
  void* _parameter;
  arThreadID _threadID;
  arSignalObject _signal; // avoid race condition during initialization
};

#endif
