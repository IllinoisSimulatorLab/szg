//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInputEventQueue.h"
#include "arSTLalgo.h"

arInputEventQueue::~arInputEventQueue() {
  clear();
}

arInputEventQueue::arInputEventQueue( const arInputEventQueue& q ) :
  _numButtons( 0 ),
  _numAxes( 0 ),
  _numMatrices( 0 ),
  _buttonSignature( q._buttonSignature ),
  _axisSignature( q._axisSignature ),
  _matrixSignature( q._matrixSignature ) {

  std::deque<arInputEvent>::const_iterator iter;
  for (iter = q._queue.begin(); iter != q._queue.end(); ++iter)
    appendEvent( *iter );
}

arInputEventQueue& arInputEventQueue::operator=( const arInputEventQueue& q ) {
  if (&q == this)
    return *this;
  clear();
  _buttonSignature = q._buttonSignature;
  _axisSignature = q._axisSignature;
  _matrixSignature = q._matrixSignature;

  std::deque<arInputEvent>::const_iterator iter;
  for (iter = q._queue.begin(); iter != q._queue.end(); ++iter)
    appendEvent( *iter );

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
      if (eventIndex >= _axisSignature) {
        setSignature( _buttonSignature, eventIndex+1, _matrixSignature );
      }
      break;
    case AR_EVENT_MATRIX:
      _numMatrices++;
      if (eventIndex >= _matrixSignature) {
        setSignature( _buttonSignature, _axisSignature, eventIndex+1 );
      }
      break;
    default:
      ar_log_error() << "arInputEventQueue ignoring unexpected event type " <<
        eventType << ".\n";
      return;
  }
  _queue.push_back( inputEvent );
}

static inline unsigned int maxuint( const unsigned int a, const unsigned int b ) {
  return (a > b)?(a):(b);
}

// Concatenate two queues.
void arInputEventQueue::appendQueue( const arInputEventQueue& rhs ) {
  std::deque<arInputEvent>::const_iterator iter;
  for (iter = rhs._queue.begin(); iter != rhs._queue.end(); ++iter)
    appendEvent( *iter );
//  setSignature( maxuint( _buttonSignature, rhs.getButtonSignature() ),
//                maxuint( _axisSignature, rhs.getAxisSignature() ),
//                maxuint( _matrixSignature, rhs.getMatrixSignature() ) );
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
      ar_log_error() << "ignoring queued input event with unexpected type.\n";
      goto LAgain;
  }
  return temp;
}

void arInputEventQueue::setSignature( unsigned numButtons,
                                      unsigned numAxes,
                                      unsigned numMatrices ) {
  std::deque<arInputEvent>::const_iterator iter;

  if (numButtons < _buttonSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_BUTTON)
        if (int(iter->getIndex()) > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numButtons) {
      ar_log_error() << "arInputEventQueue failed to shrink button signature to "
           << numButtons << ": has a button event with index " << maxIndex << ".\n";
      numButtons = maxIndex+1;
    }
  }
#ifdef OBNOXIOUSLY_VERBOSE
  bool changed = (_buttonSignature != numButtons);
#endif
  _buttonSignature = numButtons;

  if (numAxes < _axisSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_AXIS)
        if (int(iter->getIndex()) > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numAxes) {
      ar_log_error() << "arInputEventQueue failed to shrink axis signature to "
           << numAxes << ": has an axis event with index " << maxIndex << ".\n";
      numAxes = maxIndex+1;
    }
  }
#ifdef OBNOXIOUSLY_VERBOSE
  changed |= (_axisSignature != numAxes);
#endif
  _axisSignature = numAxes;

  if (numMatrices < _matrixSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_MATRIX)
        if (int(iter->getIndex()) > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numMatrices) {
      ar_log_error() << "arInputEventQueue failed to shrink matrix signature to "
           << numMatrices << ": has a matrix event with index " << maxIndex << ".\n";
      numMatrices = maxIndex+1;
    }
  }
#ifdef OBNOXIOUSLY_VERBOSE
  changed |= (_matrixSignature != numMatrices);
#endif
  _matrixSignature = numMatrices;

#ifdef OBNOXIOUSLY_VERBOSE
  if (changed) {
    ar_log_debug() << "arInputEventQueue sig is ("
      << _buttonSignature << ","
      << _axisSignature << ","
      << _matrixSignature << ").\n";
  }
#endif
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
    ar_log_error() << "arInputEventQueue: invalid buffer.\n";
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
      ar_log_error() << "arInputEventQueue ignoring negative event index.\n";
      ok = false;
      continue;
    }
    const int eventType = typeData[i];
//ar_log_error() << i << " " << eventType << " " << eventType << "  "
//     << iButton << " " << iAxis << " " << iMatrix << "  "
//     << _numButtons << " " << _numAxes << " " << _numMatrices << ".\n";
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= numButtons) {
          ar_log_error() << "arInputEventQueue: number of buttons in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          ok = false;
        } else
          appendEvent( arButtonEvent( (unsigned int)eventIndex, buttonData[iButton++] ) );
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= numAxes) {
          ar_log_error() << "arInputEventQueue: number of axes in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          ok = false;
        } else
          appendEvent( arAxisEvent( (unsigned int)eventIndex, axisData[iAxis++] ) );
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= numMatrices) {
          ar_log_error() << "arInputEventQueue: number of matrices in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          ok = false;
        } else
          appendEvent( arMatrixEvent( (unsigned int)eventIndex,
                       matrixData + 16*iMatrix++ ) );
        break;
      default:
        ar_log_error() << "arInputEventQueue ignoring unexpected event type "
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
    ar_log_error() << "arInputEventQueue: null buffer.\n";
    return false;
  }

  bool ok = true;
  const unsigned numItems = _numButtons + _numAxes + _numMatrices;
  if (numItems != size())
    ar_log_error() << "arInputEventQueue internal miscount," <<
      _numButtons << "+" << _numAxes << "+" << _numMatrices << " != " << size() << ".\n";

  unsigned iButton = 0;
  unsigned iAxis = 0;
  unsigned iMatrix = 0;
  for (unsigned int i=0; i<numItems; i++) {
    const arInputEvent ev = _queue[i];
    const arInputEventType eventType = ev.getType();
    typeBuf[i] = (int)eventType;
    indexBuf[i] = (int)ev.getIndex();
//ar_log_error() << i << " " << eventType << " " << eventType << "  "
//     << iButton << " " << iAxis << " " << iMatrix << "  "
//     << _numButtons << " " << _numAxes << " " << _numMatrices << ".\n";
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= _numButtons) {
          ar_log_error() << "arInputEventQueue: _numButtons != number of buttons.\n"
               << "   Skipping extra.\n";
          ok = false;
        } else {
          buttonBuf[iButton++] = ev.getButton();
  }
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= _numAxes) {
          ar_log_error() << "arInputEventQueue: _numAxes != number of axes.\n"
               << "   Skipping extra.\n";
          ok = false;
        } else {
          axisBuf[iAxis++] = ev.getAxis();
  }
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= _numMatrices) {
          ar_log_error() << "arInputEventQueue: _numMatrices != number of matrices.\n"
               << "   Skipping extra.\n";
          ok = false;
        } else {
          memcpy( matrixBuf + 16*iMatrix++, ev.getMatrix().v, 16*sizeof(float) );
        }
        break;
      default:
  ar_log_error() << "arInputEventQueue ignoring unexpected event type "
    << eventType << ".\n";
  break;
    }
  }
  return ok;
}

void arInputEventQueue::clear() {
  _numButtons = 0;
  _numAxes = 0;
  _numMatrices = 0;
  _queue.clear();
}
