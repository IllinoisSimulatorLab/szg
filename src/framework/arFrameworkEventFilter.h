//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FRAMEWORK_EVENT_FILTER_H
#define AR_FRAMEWORK_EVENT_FILTER_H

#include "arIOFilter.h"
#include "arFrameworkCalling.h"

class arSZGAppFramework;

class SZG_CALL arFrameworkEventFilter : public arIOFilter {
  public:
    arFrameworkEventFilter( arSZGAppFramework* fw = 0 );
    virtual ~arFrameworkEventFilter() { _queue.clear(); }
    void saveEventQueue( bool onoff ) { _saveEventQueue = onoff; }
    void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    arSZGAppFramework* getFramework() const { return _framework; }
    void queueEvent( const arInputEvent& event );
    arInputEventQueue getEventQueue();
    void flushEventQueue();
  protected:
    // To buffer events & process them once per frame,
    // override this and call queueEvent(). The master/slave
    // framework will call processEventQueue() for you.
    virtual bool _processEvent( arInputEvent& inputEvent );
    arSZGAppFramework* _framework;
    bool _saveEventQueue;
    arInputEventQueue _queue;
    arLock _queueLock; // guards _queue
};

class arCallbackEventFilter;

typedef bool (*arFrameworkEventCallback)(
  arSZGAppFramework&, arInputEvent&, arCallbackEventFilter& );

typedef bool (*arFrameworkEventQueueCallback)(
  arSZGAppFramework&, arInputEventQueue& );

class SZG_CALL arCallbackEventFilter : public arFrameworkEventFilter {
  public:
    arCallbackEventFilter( arSZGAppFramework* fw = 0, 
                           arFrameworkEventCallback cb = 0 );
    virtual ~arCallbackEventFilter() {}
    void setCallback( arFrameworkEventCallback cb ) { _callback = cb; }
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
  private:
    arFrameworkEventCallback _callback;
};

#endif
