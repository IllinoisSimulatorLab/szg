//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInputEventQueue.h"
#include "arSTLalgo.h"

arInputEventQueue::~arInputEventQueue() {
  _queue.clear();
}

arInputEventQueue::arInputEventQueue( const arInputEventQueue& q ) :
  _numButtons( 0 ),
  _numAxes( 0 ),
  _numMatrices( 0 ),
  _buttonSignature( q._buttonSignature ),
  _axisSignature( q._axisSignature ),
  _matrixSignature( q._matrixSignature ) {

#ifdef AR_USE_WIN_32
  std::deque<arInputEvent>::const_iterator iter;
  for (iter = q._queue.begin(); iter != q._queue.end(); ++iter)
    appendEvent( *iter );
#else
  // memory leak in deque copy, STLport, visual studio 6.
  std::copy( q._queue.begin(), q._queue.end(), _queue.begin() );
#endif
}

arInputEventQueue& arInputEventQueue::operator=( const arInputEventQueue& q ) {
  if (&q == this)
    return *this;
  _numButtons = 0;
  _numAxes = 0;
  _numMatrices = 0;
  _buttonSignature = q._buttonSignature;
  _axisSignature = q._axisSignature;
  _matrixSignature = q._matrixSignature;
  _queue.clear();

#ifdef AR_USE_WIN_32
  std::deque<arInputEvent>::const_iterator iter;
  for (iter = q._queue.begin(); iter != q._queue.end(); ++iter)
    appendEvent( *iter );
#else
  // memory leak in deque copy, STLport, visual studio 6.
  std::copy( q._queue.begin(), q._queue.end(), _queue.begin() );
#endif

  return *this;
}

void arInputEventQueue::appendEvent( const arInputEvent& inputEvent ) {
  const unsigned eventIndex = inputEvent.getIndex();
  const arInputEventType eventType = inputEvent.getType();
  switch (eventType) {
    case AR_EVENT_BUTTON:
      _numButtons++;
      if (eventIndex >= _buttonSignature) {
	setSignature( eventIndex+1, _axisSignature, _matrixSignature );
      }
      break;
    case AR_EVENT_AXIS:
      _numAxes++;
      if (eventIndex >= _axisSignature)
        setSignature( _buttonSignature, eventIndex+1, _matrixSignature );
      break;
    case AR_EVENT_MATRIX:
      _numMatrices++;
      if (eventIndex >= _matrixSignature)
        setSignature( _buttonSignature, _axisSignature, eventIndex+1 );
      break;
    default:
      ar_log_warning() << "arInputEventQueue ignoring unexpected event type "
	<< eventType << ".\n";
      return;
  }
  //ar_log_debug() << _numButtons << " " << _numAxes << " " << _numMatrices << ".\n";
  _queue.push_back( inputEvent );
}

static inline unsigned int maxuint( const unsigned int a, const unsigned int b ) {
  return (a > b)?(a):(b);
}

void arInputEventQueue::appendQueue( const arInputEventQueue& eventQueue ) {
  std::deque<arInputEvent>::const_iterator iter;
  for (iter = eventQueue._queue.begin(); iter != eventQueue._queue.end(); ++iter)
    appendEvent( *iter );
  setSignature( maxuint( _buttonSignature, eventQueue.getButtonSignature() ),
                maxuint( _axisSignature, eventQueue.getAxisSignature() ),
                maxuint( _matrixSignature, eventQueue.getMatrixSignature() ) );
}

arInputEvent arInputEventQueue::popNextEvent() {
LAgain:
  if (_queue.empty()) {
    return arInputEvent();
  }

  arInputEvent temp(_queue.front());
  _queue.pop_front();
//  _queue.erase( _queue.begin() );
  switch (temp.getType()) {
    case AR_EVENT_BUTTON:
      _numButtons--;
      break;
    case AR_EVENT_AXIS:
      _numAxes--;
      break;
    case AR_EVENT_MATRIX:
      _numMatrices--;
      break;
    default:
      ar_log_warning() << "ignoring queued input event with unexpected type.\n";
      goto LAgain;
  }
  return temp;
}

void arInputEventQueue::setSignature( unsigned int numButtons,
                                      unsigned int numAxes,
                                      unsigned int numMatrices ) {
  std::deque<arInputEvent>::iterator iter;

  /*
    ar_log_debug() << "arInputEventQueue signature was ("
      << _buttonSignature << ","
      << _axisSignature << ","
      << _matrixSignature << ").\n";
  */
  
  if (numButtons < _buttonSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_BUTTON)
        if ((int)iter->getIndex() > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numButtons) {
      ar_log_warning() << "arInputEventQueue warning: you can't set button signature to "
           << numButtons << ",\n I'm holding a button event with index "
           << maxIndex << ".\n";
      numButtons = maxIndex+1;
    }
  }
  bool changed = (_buttonSignature != numButtons);
  _buttonSignature = numButtons;
    
  if (numAxes < _axisSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_AXIS)
        if ((int)iter->getIndex() > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numAxes) {
      ar_log_warning() << "arInputEventQueue warning: you can't set axis signature to "
           << numAxes << ",\n I'm holding an axis event with index "
           << maxIndex << ".\n";
      numAxes = maxIndex+1;
    }
  }
  changed |= (_axisSignature != numAxes);
  _axisSignature = numAxes;
    
  if (numMatrices < _matrixSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_MATRIX)
        if ((int)iter->getIndex() > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numMatrices) {
      ar_log_warning() << "arInputEventQueue warning: you can't set matrix signature to "
           << numAxes << ",\n I'm holding a matrix event with index "
           << maxIndex << ".\n";
      numMatrices = maxIndex+1;
    }
  }
  changed |= (_matrixSignature != numMatrices);
  _matrixSignature = numMatrices;

  if (changed){
    ar_log_debug() << "arInputEventQueue signature is ("
      << _buttonSignature << ","
      << _axisSignature << ","
      << _matrixSignature << ").\n";
  }
}

bool arInputEventQueue::setFromBuffers( const int* const typeData,
                         const int* const indexData,
                         const int* const buttonData,
                         const unsigned int numButtons,
                         const float* const axisData,
                         const unsigned int numAxes,
                         const float* const matrixData,
                         const unsigned int numMatrices ) {
  if ((!typeData)||(!indexData)||(!buttonData)||(!axisData)||(!matrixData)) {
    ar_log_warning() << "arInputEventQueue: invalid buffer.\n";
    return false;
  }
  bool ok = true;
  const unsigned numItems = numButtons + numAxes + numMatrices;
  unsigned iButton = 0;
  unsigned iAxis = 0;
  unsigned iMatrix = 0;
  for (unsigned i=0; i<numItems; i++) {
    const int eventIndex = indexData[i];
    if (eventIndex < 0) {
      ar_log_warning() << "arInputEventQueue ignoring negative event index.\n";
      ok = false;
      continue;
    }
    const int eventType = typeData[i];
//ar_log_warning() << i << " " << eventType << " " << eventType << "  "
//     << iButton << " " << iAxis << " " << iMatrix << "  "
//     << _numButtons << " " << _numAxes << " " << _numMatrices << ".\n";
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= numButtons) {
          ar_log_warning() << "arInputEventQueue: number of buttons in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          ok = false;
        } else
          appendEvent( arButtonEvent( (unsigned int)eventIndex, buttonData[iButton++] ) );
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= numAxes) {
          ar_log_warning() << "arInputEventQueue: number of axes in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          ok = false;
        } else
          appendEvent( arAxisEvent( (unsigned int)eventIndex, axisData[iAxis++] ) );
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= numMatrices) {
          ar_log_warning() << "arInputEventQueue: number of matrices in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          ok = false;
        } else
          appendEvent( arMatrixEvent( (unsigned int)eventIndex,
                       matrixData + 16*iMatrix++ ) );
        break;
      default:
        ar_log_warning() << "arInputEventQueue ignoring unexpected event type "
             << eventType << ".\n";
        ok = false;
    }
  }
  return ok;
}

bool arInputEventQueue::saveToBuffers( int* const typeBuf,
                                       int* const indexBuf,
                                       int* const buttonBuf,
                                       float* const axisBuf,
                                       float* const matrixBuf ) const {
  if (!typeBuf || !indexBuf || !buttonBuf || !axisBuf || !matrixBuf) {
    ar_log_warning() << "arInputEventQueue: null buffer.\n";
    return false;
  }  

  bool ok = true;
  unsigned numItems = _numButtons + _numAxes + _numMatrices;
  unsigned iButton = 0;
  unsigned iAxis = 0;
  unsigned iMatrix = 0;
  for (unsigned int i=0; i<numItems; i++) {
    const arInputEvent inputEvent = _queue[i];
    const arInputEventType eventType = inputEvent.getType();
    typeBuf[i] = (int)eventType;
    indexBuf[i] = (int)inputEvent.getIndex();
//ar_log_warning() << i << " " << eventType << " " << eventType << "  "
//     << iButton << " " << iAxis << " " << iMatrix << "  "
//     << _numButtons << " " << _numAxes << " " << _numMatrices << ".\n";
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= _numButtons) {
          ar_log_warning() << "arInputEventQueue: _numButtons != number of buttons.\n"
               << "   Skipping extra.\n";
          ok = false;
        } else
          buttonBuf[iButton++] = inputEvent.getButton();
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= _numAxes) {
          ar_log_warning() << "arInputEventQueue: _numAxes != number of axes.\n"
               << "   Skipping extra.\n";
          ok = false;
        } else
          axisBuf[iAxis++] = inputEvent.getAxis();
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= _numMatrices) {
          ar_log_warning() << "arInputEventQueue: _numMatrices != number of matrices.\n"
               << "   Skipping extra.\n";
          ok = false;
        } else {
          arMatrix4 m = inputEvent.getMatrix();
          memcpy( matrixBuf + 16*iMatrix++, m.v, 16*sizeof(float) );
        }
        break;
      default:
	ar_log_warning() << "arInputEventQueue ignoring unexpected event type "
	  << eventType << ".\n";
	break;
    }
  }
  return ok;
}
  
void arInputEventQueue::clear() {
  _queue.clear();
  _numButtons = 0;
  _numAxes = 0;
  _numMatrices = 0;
}
