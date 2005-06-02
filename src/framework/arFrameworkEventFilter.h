//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FRAMEWORK_EVENT_FILTER_H
#define AR_FRAMEWORK_EVENT_FILTER_H

#include "arIOFilter.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arFrameworkCalling.h"

class arSZGAppFramework;

class SZG_CALL arFrameworkEventFilter : public arIOFilter {
  public:
    arFrameworkEventFilter( arSZGAppFramework* fw = 0 );
    virtual ~arFrameworkEventFilter() {
      _queue.clear();
    }
    void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    arSZGAppFramework* getFramework() const { return _framework; }
    void queueEvent( const arInputEvent& event );
    bool processEventQueue();
    void flushEventQueue();
  protected:
    // NOTE: if you want to buffer events & process them all e.g. once/frame,
    // then override this & add a call to queueEvent(). The master/slave
    // framework will call processEventQueue() and flushEventQueue() automatically;
    // in other types of apps, you'll need to do it manually.
    virtual bool _processEvent( arInputEvent& /*inputEvent*/ ) { return true; }
    virtual bool _processEventQueue( arInputEventQueue& /*queue*/ ) { return true; }
    arInputEventQueue _queue;
    arMutex _queueMutex;
  private:
    arSZGAppFramework* _framework;
};

class arCallbackEventFilter;

typedef bool (*arFrameworkEventCallback)( arInputEvent& event, 
                                          arCallbackEventFilter* filter );

typedef bool (*arFrameworkEventQueueCallback)( arInputEventQueue& queue, 
                                               arCallbackEventFilter* filter );

class SZG_CALL arCallbackEventFilter : public arFrameworkEventFilter {
  public:
    arCallbackEventFilter( arSZGAppFramework* fw = 0, 
                           arFrameworkEventCallback cb = 0,
                           arFrameworkEventQueueCallback qcb = 0 );
    virtual ~arCallbackEventFilter() {}
    void setCallback( arFrameworkEventCallback cb ) { _callback = cb; }
    void setQueueCallback( arFrameworkEventQueueCallback cb ) { _queueCallback = cb; }
  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
    virtual bool _processEventQueue( arInputEventQueue& queue );
  private:
    arFrameworkEventCallback _callback;
    arFrameworkEventQueueCallback _queueCallback;
};

#endif        //  #ifndefARFRAMEWORKEVENTFILTER_H

