//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFrameworkEventFilter.h"

arFrameworkEventFilter::arFrameworkEventFilter( arSZGAppFramework* fw ) :
  _framework( fw ) {
  ar_mutex_init( &_queueMutex );
}

void arFrameworkEventFilter::queueEvent( const arInputEvent& event ) {
  ar_mutex_lock( &_queueMutex );
  _queue.appendEvent( event );
  ar_mutex_unlock( &_queueMutex );
}

bool arFrameworkEventFilter::processEventQueue() {
  ar_mutex_lock( &_queueMutex );
  arInputEventQueue queue( _queue );
  _queue.clear();
  ar_mutex_unlock( &_queueMutex );
  return _processEventQueue( queue );
}

void arFrameworkEventFilter::flushEventQueue() {
  ar_mutex_lock( &_queueMutex );
  _queue.clear();
  ar_mutex_unlock( &_queueMutex );
}

arCallbackEventFilter::arCallbackEventFilter( arSZGAppFramework* fw,
                                              arFrameworkEventCallback cb,
                                              arFrameworkEventQueueCallback qcb ) :
  arFrameworkEventFilter(fw),
  _callback(cb),
  _queueCallback(qcb) {
}

bool arCallbackEventFilter::_processEvent( arInputEvent& inputEvent ) {
  bool stat(true);
  if (_callback) {
    stat = _callback( inputEvent, this );
  }
  if (_queueCallback) {
    queueEvent( inputEvent );
  }
  return stat;
}

bool arCallbackEventFilter::_processEventQueue( arInputEventQueue& queue ) {
  if (!_queueCallback) { // not an error, just no event-handling
    return true;
  }
  bool stat = _queueCallback( queue, this );
  return stat;
}

