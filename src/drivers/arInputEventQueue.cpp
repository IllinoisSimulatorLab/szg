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
    //    COPY ALGORITHM LEAKS MEMORY! (at least for dequeue, STLport, VC++6, Win32).
//    std::copy( q._queue.begin(), q._queue.end(), _queue.begin() );
  std::deque<arInputEvent>::const_iterator iter;
  for (iter = q._queue.begin(); iter != q._queue.end(); ++iter)
    appendEvent( *iter );
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
    //    COPY ALGORITHM LEAKS MEMORY! (at least for dequeue, STLport, VC++6, Win32).
//    std::copy( q._queue.begin(), q._queue.end(), _queue.begin() );
  std::deque<arInputEvent>::const_iterator iter;
  for (iter = q._queue.begin(); iter != q._queue.end(); ++iter)
    appendEvent( *iter );
  return *this;
}

void arInputEventQueue::appendEvent( const arInputEvent& inputEvent ) {
  unsigned int eventIndex = inputEvent.getIndex();
  switch (inputEvent.getType()) {
    case AR_EVENT_BUTTON:
      _numButtons++;
      if (eventIndex >= _buttonSignature)
        setSignature( eventIndex+1, _axisSignature, _matrixSignature );
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
      cerr << "arInputEventQueue error: invalid event type.\n";
      return;
  }
//cerr << _numButtons << " " << _numAxes << " " << _numMatrices << endl;
  _queue.push_back( inputEvent );
}

static unsigned int maxuint( const unsigned int a, const unsigned int b ) {
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
  if (_queue.empty()) {
    return arInputEvent();
  }
  arInputEvent temp = _queue.front();
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
  }
  return temp;
}

void arInputEventQueue::setSignature( unsigned int numButtons,
                                      unsigned int numAxes,
                                      unsigned int numMatrices ) {
  bool changed(false);
  std::deque<arInputEvent>::iterator iter;
  
  if (numButtons < _buttonSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_BUTTON)
        if ((int)iter->getIndex() > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numButtons) {
      cerr << "arInputEventQueue warning: you can't set button signature to "
           << numButtons << ",\n I'm holding a button event with index "
           << maxIndex << endl;
      numButtons = maxIndex+1;
    }
  }
  if (_buttonSignature != numButtons)
    changed = true;
  _buttonSignature = numButtons;
    
  if (numAxes < _axisSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_AXIS)
        if ((int)iter->getIndex() > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numAxes) {
      cerr << "arInputEventQueue warning: you can't set axis signature to "
           << numAxes << ",\n I'm holding an axis event with index "
           << maxIndex << endl;
      numAxes = maxIndex+1;
    }
  }
  if (_axisSignature != numAxes)
    changed = true;
  _axisSignature = numAxes;
    
  if (numMatrices < _matrixSignature) {
    int maxIndex = -1;
    for (iter = _queue.begin(); iter != _queue.end(); ++iter) {
      if (iter->getType() == AR_EVENT_MATRIX)
        if ((int)iter->getIndex() > maxIndex)
          maxIndex = iter->getIndex();
    }
    if (maxIndex >= (int)numMatrices) {
      cerr << "arInputEventQueue warning: you can't set matrix signature to "
           << numAxes << ",\n I'm holding a matrix event with index "
           << maxIndex << endl;
      numMatrices = maxIndex+1;
    }
  }
  if (_matrixSignature != numMatrices)
    changed = true;
  _matrixSignature = numMatrices;
  
  if (changed){
    //cerr << "arInputEventQueue remark: signature set to ( "
    //     << _buttonSignature << ", " << _axisSignature << ", " 
    //     << _matrixSignature << " ).\n";
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
    cerr << "arInputEventQueue error: invalid buffer.\n";
    return false;
  }
  const unsigned int numItems( numButtons + numAxes + numMatrices );
  bool status(true);
  unsigned int iButton = 0;
  unsigned int iAxis = 0;
  unsigned int iMatrix = 0;
  for (unsigned int i=0; i<numItems; i++) {
    int eventIndex = indexData[i];
    if (eventIndex < 0) {
      cerr << "arInputEventQueue warning: ignoring negative event index.\n";
      status = false;
      continue;
    }
    int eventType = typeData[i];
//cerr << i << " " << eventType << " " << eventType << "  "
//     << iButton << " " << iAxis << " " << iMatrix << "  "
//     << _numButtons << " " << _numAxes << " " << _numMatrices << endl;
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= numButtons) {
          cerr << "arInputEventQueue warning: number of buttons in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          status = false;
        } else
          appendEvent( arButtonEvent( (unsigned int)eventIndex, buttonData[iButton++] ) );
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= numAxes) {
          cerr << "arInputEventQueue warning: number of axes in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          status = false;
        } else
          appendEvent( arAxisEvent( (unsigned int)eventIndex, axisData[iAxis++] ) );
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= numMatrices) {
          cerr << "arInputEventQueue warning: number of matrices in index field\n"
               << " exceeds specified input buffer size. Ignoring extras.\n";
          status = false;
        } else
          appendEvent( arMatrixEvent( (unsigned int)eventIndex,
                       matrixData + 16*iMatrix++ ) );
        break;
      default:
        cerr << "arInputEventQueue warning: ignoring event type "
             << eventType << endl;
        status = false;
    }
  }
  return status;
}

bool arInputEventQueue::saveToBuffers( int* const typeBuf,
                                       int* const indexBuf,
                                       int* const buttonBuf,
                                       float* const axisBuf,
                                       float* const matrixBuf ) const {
  if ((!typeBuf)||(!indexBuf)||(!buttonBuf)||(!axisBuf)||(!matrixBuf)) {
    cerr << "arInputEventQueue error: invalid buffer.\n";
    return false;
  }  
  unsigned int numItems( _numButtons + _numAxes + _numMatrices );
  bool status(true);
  unsigned int iButton = 0;
  unsigned int iAxis = 0;
  unsigned int iMatrix = 0;
  for (unsigned int i=0; i<numItems; i++) {
    arInputEvent inputEvent = _queue[i];
    arInputEventType eventType = inputEvent.getType();
    typeBuf[i] = (int)eventType;
    indexBuf[i] = (int)inputEvent.getIndex();
//cerr << i << " " << eventType << " " << eventType << "  "
//     << iButton << " " << iAxis << " " << iMatrix << "  "
//     << _numButtons << " " << _numAxes << " " << _numMatrices << endl;
    switch (eventType) {
      case AR_EVENT_BUTTON:
        if (iButton >= _numButtons) {
          cerr << "arInputEventQueue warning: _numButtons != number of buttons.\n"
               << "   Skipping extra.\n";
          status = false;
        } else
          buttonBuf[iButton++] = inputEvent.getButton();
        break;
      case AR_EVENT_AXIS:
        if (iAxis >= _numAxes) {
          cerr << "arInputEventQueue warning: _numAxes != number of axes.\n"
               << "   Skipping extra.\n";
          status = false;
        } else
          axisBuf[iAxis++] = inputEvent.getAxis();
        break;
      case AR_EVENT_MATRIX:
        if (iMatrix >= _numMatrices) {
          cerr << "arInputEventQueue warning: _numMatrices != number of matrices.\n"
               << "   Skipping extra.\n";
          status = false;
        } else {
          arMatrix4 m = inputEvent.getMatrix();
          memcpy( matrixBuf + 16*iMatrix++, m.v, 16*sizeof(float) );
        }
        break;
    }
  }
  return status;
}
  
void arInputEventQueue::clear() {
  _queue.clear();
  _numButtons = 0;
  _numAxes = 0;
  _numMatrices = 0;
}

