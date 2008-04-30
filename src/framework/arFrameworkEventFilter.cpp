//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"
#include "arFrameworkEventFilter.h"
#include "arSZGAppFramework.h"

arFrameworkEventFilter::arFrameworkEventFilter( arSZGAppFramework* fw ) :
  _framework( fw ),
  _saveEventQueue( false ) {
}

void arFrameworkEventFilter::queueEvent( const arInputEvent& event ) {
  arGuard dummy(_queueLock);
  _queue.appendEvent( event );
}

arInputEventQueue arFrameworkEventFilter::getEventQueue() {
  arGuard dummy(_queueLock);
  arInputEventQueue queue( _queue );
  _queue.clear();
  return queue;
}

void arFrameworkEventFilter::flushEventQueue() {
  arGuard dummy(_queueLock);
  _queue.clear();
}

bool arFrameworkEventFilter::_processEvent( arInputEvent& inputEvent ) {
  if (_framework) {
    if (!_framework->onInputEvent( inputEvent, *this )) {
      return false;
    }
  }
  if (_saveEventQueue && (inputEvent.getType() != AR_EVENT_GARBAGE)) {
    queueEvent( inputEvent );
  }
  return true;
}

arCallbackEventFilter::arCallbackEventFilter(
  arSZGAppFramework* fw, arFrameworkEventCallback cb ) :
  arFrameworkEventFilter(fw), _callback(cb) {
}

bool arCallbackEventFilter::_processEvent( arInputEvent& inputEvent ) {
  bool ok = true;
  if (_callback) {
    if (!_framework) {
      ar_log_error() << "arCallbackEventFilter: no framework.\n";
      return false;
    }
    ok = _callback( *_framework, inputEvent, *this );
  }
  if (_saveEventQueue && (inputEvent.getType() != AR_EVENT_GARBAGE)) {
    queueEvent( inputEvent );
  }
  return ok;
}
