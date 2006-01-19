//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arIOFilter.h"

arIOFilter::arIOFilter() :
  _id(-1),
  _inputState(0) {
  // does nothing so far
}

arIOFilter::~arIOFilter(){
  // does nothing so far
}

bool arIOFilter::configure(arSZGClient*){
  // does nothing so far
  return true;
}

bool arIOFilter::filter( arInputEventQueue* inputQueue, arInputState* inputState ) {
  if ((!inputQueue)||(!inputState)) {
    cerr << "arIOFilter error: null queue or state pointer.\n";
    return false;
  }
  _outputQueue.clear();
  _inputState = inputState;
  bool ok = true;
  while (!inputQueue->empty()) {
    arInputEvent event = inputQueue->popNextEvent();
    if (event) {
     const arInputEventType eventType = event.getType();
      const unsigned int eventIndex = event.getIndex();
       _tempQueue.clear();
//      inputState->update( event );
      ok = _processEvent( event );
      if (event) {
        _outputQueue.appendEvent( event );
        inputState->update( event ); // value may have changed
      } else {
        // Was trashed, so zero out appropriate slot in state.
        inputState->update( arInputEvent( eventType, eventIndex ) );
      }
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
  if (!_inputState) {
    cerr << "arIOFilter error: null inputState pointer.\n";
    return 0;
  }
  return _inputState->getButton( index );
}

bool arIOFilter::getOnButton( const unsigned int index ) {
  if (!_inputState) {
    cerr << "arIOFilter error: null inputState pointer.\n";
    return 0;
  }
  return _inputState->getOnButton( index );
}

bool arIOFilter::getOffButton( const unsigned int index ) {
  if (!_inputState) {
    cerr << "arIOFilter error: null inputState pointer.\n";
    return 0;
  }
  return _inputState->getOffButton( index );
}

float arIOFilter::getAxis( const unsigned int index ) const {
  if (!_inputState) {
    cerr << "arIOFilter error: null inputState pointer.\n";
    return 0.;
  }
  return _inputState->getAxis( index );
}

arMatrix4 arIOFilter::getMatrix( const unsigned int index ) const {
  if (!_inputState) {
    cerr << "arIOFilter error: null inputState pointer.\n";
    return ar_identityMatrix();
  }
  return _inputState->getMatrix( index );
}

void arIOFilter::insertNewEvent( const arInputEvent& newEvent ) {
  _tempQueue.appendEvent( newEvent );
}
