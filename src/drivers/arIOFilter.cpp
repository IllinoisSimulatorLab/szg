//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arIOFilter.h"

arIOFilter::arIOFilter() :
  _id(-1),
  _inputState(NULL) {
}

bool arIOFilter::configure(arSZGClient*){
  return true;
}

bool arIOFilter::filter( arInputEventQueue* inputQueue, arInputState* inputState ) {
  if (!inputQueue || !inputState) {
    ar_log_error() << "arIOFilter: NULL queue or state.\n";
    return false;
  }

  _outputQueue.clear();
  _inputState = inputState;
  bool ok = true;
  while (!inputQueue->empty()) {
    arInputEvent event = inputQueue->popNextEvent();
    if (event) {
//    const arInputEventType eventType = event.getType();
//    const unsigned eventIndex = event.getIndex();
      _tempQueue.clear();
//    inputState->update( event );
      ok = _processEvent( event ); // may modify "event"
      // bug: should this be "ok &= ..." ?
      if (event) {
        _outputQueue.appendEvent( event );
        inputState->update( event ); // value may have changed
      }
      // I'm thinking this was a mistake; if event was deleted,
      // it should have no effect on input state.
//      else {
        // Was trashed, so zero out appropriate slot in state.
//        inputState->update( arInputEvent( eventType, eventIndex ) );
//      }
      // _tempQueue only contains events (if any) inserted by call
      // to insertNewEvent() inside _processEvent().
      _outputQueue.appendQueue( _tempQueue );
      while (!_tempQueue.empty()) {
        event = _tempQueue.popNextEvent();
        if (event) {
          inputState->update( event );
        }
      }
    }
  }
  inputQueue->appendQueue( _outputQueue );
  _inputState = 0;
  return ok;
}

int arIOFilter::getButton( const unsigned int index ) const {
  return _valid() ? _inputState->getButton( index ) : 0;
}

bool arIOFilter::getOnButton( const unsigned int index ) {
  return _valid() ? _inputState->getOnButton( index ) : false;
}

bool arIOFilter::getOffButton( const unsigned int index ) {
  return _valid() ? _inputState->getOffButton( index ) : false;
}

float arIOFilter::getAxis( const unsigned int index ) const {
  return _valid() ? _inputState->getAxis( index ) : 0.;
}

arMatrix4 arIOFilter::getMatrix( const unsigned int index ) const {
  return _valid() ? _inputState->getMatrix( index ) : arMatrix4();
}

void arIOFilter::insertNewEvent( const arInputEvent& newEvent ) {
  _tempQueue.appendEvent( newEvent );
}

bool arIOFilter::_valid() const {
  if (_inputState)
    return true;

  ar_log_error() << "arIOFilter: NULL _inputState.\n";
  return false;
}
