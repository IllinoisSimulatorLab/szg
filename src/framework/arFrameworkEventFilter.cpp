//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arFrameworkEventFilter.h"

arFrameworkEventFilter::arFrameworkEventFilter( arSZGAppFramework* fw ) :
  _framework( fw ) {
}

arCallbackEventFilter::arCallbackEventFilter( arSZGAppFramework* fw, arFrameworkEventCallback cb ) :
  arFrameworkEventFilter(fw),
  _callback(cb) {
}

bool arCallbackEventFilter::_processEvent( arInputEvent& inputEvent ) {
  if (!_callback) { // not an error, just no event-handling
    return true;
  }
  return _callback( inputEvent, (arIOFilter*)this, getFramework() );
}

