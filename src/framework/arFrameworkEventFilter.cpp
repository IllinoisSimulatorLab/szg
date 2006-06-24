//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arFrameworkEventFilter.h"

arFrameworkEventFilter::arFrameworkEventFilter( arSZGAppFramework* fw ) :
  _framework( fw ),
  _saveEventQueue( false ) {
}

void arFrameworkEventFilter::queueEvent( const arInputEvent& event ) {
  _queueLock.lock();
  _queue.appendEvent( event );
  _queueLock.unlock();
}

arInputEventQueue arFrameworkEventFilter::getEventQueue() {
  _queueLock.lock();
  arInputEventQueue queue( _queue );
  _queue.clear();
  _queueLock.unlock();
  return queue;
}

void arFrameworkEventFilter::flushEventQueue() {
  _queueLock.lock();
  _queue.clear();
  _queueLock.unlock();
}

bool arFrameworkEventFilter::_processEvent( arInputEvent& inputEvent ) {
  if (_saveEventQueue) {
    queueEvent( inputEvent );
  }
  return true;
}

arCallbackEventFilter::arCallbackEventFilter( arSZGAppFramework* fw,
                                              arFrameworkEventCallback cb ) :
  arFrameworkEventFilter(fw),
  _callback(cb) {
}

bool arCallbackEventFilter::_processEvent( arInputEvent& inputEvent ) {
  bool stat(true);
  if (_callback) {
    if (!_framework) {
      cerr << "arCallbackEventFilter error: NULL framework pointer in _processEvent().\n";
      return false;
    }
    stat = _callback( *_framework, inputEvent, *this );
  }
  if (_saveEventQueue) {
    queueEvent( inputEvent );
  }
  return stat;
}
