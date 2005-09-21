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
    void saveEventQueue( bool onoff ) { _saveEventQueue = onoff; }
    void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    arSZGAppFramework* getFramework() const { return _framework; }
    void queueEvent( const arInputEvent& event );
    arInputEventQueue getEventQueue();
    void flushEventQueue();
  protected:
    // NOTE: if you want to buffer events & process them all e.g. once/frame,
    // then override this & add a call to queueEvent(). The master/slave
    // framework will call processEventQueue() automatically;
    // in other types of apps, you'll need to do it manually.
    virtual bool _processEvent( arInputEvent& inputEvent );
    arSZGAppFramework* _framework;
    bool _saveEventQueue;
    arInputEventQueue _queue;
    arLock _queueLock;
};

class arCallbackEventFilter;

typedef bool (*arFrameworkEventCallback)( arSZGAppFramework& fw,
                                          arInputEvent& event, 
                                          arCallbackEventFilter& filter );

// Visual Studio 6 complains if "theQueue" is replaced by "queue".
typedef bool (*arFrameworkEventQueueCallback)( arSZGAppFramework& fw,
                                               arInputEventQueue& theQueue );

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

#endif        //  #ifndefARFRAMEWORKEVENTFILTER_H

