//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arInputNode.h"
#include "arEventUtilities.h"

arInputNode::arInputNode( bool bufferEvents ) :
  _eventCallback(NULL),
  _currentChannel(0),
  _bufferInputEvents(bufferEvents),
  _complained(false),
  _initOK(false)
{
  ar_mutex_init(&_dataSerializationLock);
}

arInputNode::~arInputNode() {
//  arSourceIterator sourceIter;
//  arSinkIterator sinkIter;
//  arFilterIterator filterIter;
//  std::vector<bool>::iterator iter;
  
// This stuff makes bad things happen, have to add it more carefully
//  for (sourceIter = _inputSourceList.begin(), iter = _iOwnSources.begin();
//         sourceIter != _inputSourceList.end() && iter != _iOwnSources.end();
//         sourceIter++, iter++) {
//    if (*iter)
//      delete *sourceIter;
//  }
//  for (sinkIter = _inputSinkList.begin(), iter = _iOwnSinks.begin();
//         sinkIter != _inputSinkList.end() && iter != _iOwnSinks.end();
//         sinkIter++, iter++) {
//    if (*iter)
//      delete *sinkIter;
//  }
//  for (filterIter = _inputFilterList.begin(), iter = _iOwnFilters.begin();
//         filterIter != _inputFilterList.end() && iter != _iOwnFilters.end();
//         filterIter++, iter++) {
//    if (*iter)
//      delete *filterIter;
//  }
  _inputFilterList.clear();
  _inputSinkList.clear();
  _inputSourceList.clear();
  _iOwnSources.clear();
  _iOwnFilters.clear();
  _iOwnSinks.clear();
  _filterStates.clear();
  _eventBuffer.clear();
}

bool arInputNode::init(arSZGClient& szgClient){
  _initOK = false;

  // Initialize the input sources registered to this node.
  for (arSourceIterator i = _inputSourceList.begin();
       i != _inputSourceList.end();
       ++i){
    // If one device initialization fails, the whole thing fails.
    if (!(*i)->init(szgClient))
      return false;

    _inputState.addInputDevice( (*i)->getNumberButtons(),
                                (*i)->getNumberAxes(),
                                (*i)->getNumberMatrices() );
  }

  // Initialize the input sinks registered to this node.
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
    // If one device initialization fails, the whole thing fails.
    if (!(*j)->init(szgClient)){
      return false;
    }
  }
  _initOK = true;
  return true;
}

bool arInputNode::start(){
  if (!_initOK)
    // Don't complain -- init() already did so.
    return false;

  _complained = false;
  bool ok = true;
  for (arSourceIterator i = _inputSourceList.begin();
       i != _inputSourceList.end();
       ++i){
    ok &= (*i)->start();
  }
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
    if ( (*j)->_autoActivate ){
      // we do not want the file logging input sink, which is
      // always built into DeviceServer to auto-activate
      ok &= (*j)->start();
    }
  }
  return ok;
}

bool arInputNode::stop(){
  if (!_initOK)
    // Don't complain -- init() already did so.
    return false;

  bool ok = true;
  for (arSourceIterator i = _inputSourceList.begin();
       i != _inputSourceList.end();
       ++i){
    ok &= (*i)->stop();
  }
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
    ok &= (*j)->stop();
  }
  return ok;
}

bool arInputNode::restart(){
  if (!_initOK)
    // Don't complain -- init() already did so.
    return false;

  bool ok = true;
  for (arSourceIterator i = _inputSourceList.begin();
       i != _inputSourceList.end();
       ++i){
    ok &= (*i)->restart();
  }
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
    ok &= (*j)->restart();
  }
  return ok;
}

void arInputNode::receiveData(int channelNumber, arStructuredData* data) {
  _lock();

  if (channelNumber < 0) {
    cerr << "arInputNode error: negative channel number.\n";
    return;
  }
  //  cerr << "arInputNode remark: channel # " << channelNumber << endl;
  
  _remapData( (unsigned int) channelNumber, data );

  _eventQueue.clear();
  if (!ar_setEventQueueFromStructuredData( &_eventQueue, data )) {
    cerr << "arInputNode error: failed to convert received data to event queue.\n";
    return;
  }
  
  if (_bufferInputEvents) {
    _eventBuffer.appendQueue( _eventQueue );
    _eventQueue.clear();
    _unlock();
    return;
  }
  
  _filterEventQueue( _eventQueue );

  if (!ar_saveEventQueueToStructuredData( &_eventQueue, data ))
    cerr << "arInputNode error: failed to convert event queue to arStructuredData.\n";

  // Update node's arInputState (empties queue)
  _updateState( _eventQueue );
  
  // finally, send this along to the connected input sinks
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
    (*j)->receiveData(channelNumber,data);
  }
  _unlock();
}

void arInputNode::processBufferedEvents() {
  _lock();

  _filterEventQueue( _eventBuffer );

  // Update node's arInputState (empties queue)
  _updateState( _eventBuffer );
  
  _unlock();
}

/// \bug This is called too many times, resetting the signature n times instead of just once.

bool arInputNode::sourceReconfig(int whichChannel){
  // this gets called when one of the connected devices has changed
  // its signature... i.e. number of buttons/axes/matrices
  _lock();

  if (whichChannel < 0 || whichChannel >= (int)_inputSourceList.size()) {
    cerr << "arInputNode warning: ignoring out-of-range channel "
         << whichChannel << ".\n";
    return false;
  }

  // Find the whichChannel'th member of _inputSourceList
  // (a linked list, so do it the slow way!).
  unsigned int j=0;
  unsigned int numButton = 0;
  unsigned int numAxis = 0;
  unsigned int numMatrix = 0;
  arSourceIterator i;
  for (i = _inputSourceList.begin(); i != _inputSourceList.end(); ++i, ++j) {
    if ((int)j == whichChannel) {
      numButton = (*i)->getNumberButtons();
      numAxis = (*i)->getNumberAxes();
      numMatrix = (*i)->getNumberMatrices();
      break;
    }
  }    
  _inputState.remapInputDevice( whichChannel, numButton, numAxis, numMatrix );

  _unlock();
  return true;
}

void arInputNode::addInputSource( arInputSource* theSource, bool iOwnIt ){
  if (!theSource) {
    cerr << "arInputNode warning: ignoring NULL source.\n";
    return;
  }

  _inputSourceList.push_back( theSource );
  theSource->_setInputSink( _currentChannel++, this );
  _iOwnSources.push_back( iOwnIt );
}

void arInputNode::addFilter( arIOFilter* theFilter, bool iOwnIt ){
  if (!theFilter) {
    cerr << "arInputNode warning: ignoring NULL filter.\n";
    return;
  }

  _inputFilterList.push_back( theFilter );
  _iOwnFilters.push_back( iOwnIt );
  if (_filterStates.empty())
    _filterStates.push_back( _inputState );
  else
    _filterStates.push_back( _filterStates.back() );
}

void arInputNode::removeFilter(  arIOFilter* theFilter ) {
  _lock();
  unsigned int filterNumber = 0;
  arFilterIterator f;
  for (f = _inputFilterList.begin(); f != _inputFilterList.end(); ++f) {
    arIOFilter* fptr = (arIOFilter*)&(*f);
    if (fptr == theFilter) {
      _inputFilterList.erase(f);
      _iOwnFilters.erase( _iOwnFilters.begin() + filterNumber );
      _filterStates.erase( _filterStates.begin() + filterNumber );
      _unlock();
      return;
    }
    ++filterNumber;
  }
  _unlock();
}


void arInputNode::addInputSink( arInputSink* theSink, bool iOwnIt ){
  if (_bufferInputEvents) {
    cerr << "arInputNode error: an input node that buffers events may not pass "
         << "data on to sinks.\n";
    return;
  }
  
  if (!theSink) {
    cerr << "arInputNode warning: ignoring NULL sink.\n";
    return;
  }

  _inputSinkList.push_back(theSink);
  _iOwnSinks.push_back( iOwnIt );
}

int arInputNode::getButton(int i) {
  return _inputState.getButton((unsigned int) i);
}

float arInputNode::getAxis(int i){
  return _inputState.getAxis((unsigned int) i);
}

arMatrix4 arInputNode::getMatrix(int i){
  return _inputState.getMatrix((unsigned int) i);
}

int arInputNode::getNumberButtons() const {
  return (int)_inputState.getNumberButtons();
}

int arInputNode::getNumberAxes() const {
  return (int)_inputState.getNumberAxes();
}

int arInputNode::getNumberMatrices() const {
  return (int)_inputState.getNumberMatrices();
}

//void arInputNode::_logEvent( arInputEvent& inputEvent ) {
//
//  switch (inputEvent.getType()) {
//    case AR_EVENT_BUTTON:
//      if (_buttonCallback)
//	      _buttonCallback( inputEvent );
//      break;
//    case AR_EVENT_AXIS:
//      if (_axisCallback)
//        _axisCallback( inputEvent );
//      break;
//    case AR_EVENT_MATRIX:
//      if (_matrixCallback)
//        _matrixCallback( inputEvent );
//      break;
//    default:
//      cerr << "ar_ClientDataThread warning:"
//           << " ignoring invalid event type "
//	   << inputEvent.getType() << ".\n";
//  }
//}

void arInputNode::_lock() {
  ar_mutex_lock( &_dataSerializationLock );
}

void arInputNode::_unlock() {
  ar_mutex_unlock( &_dataSerializationLock );
}

void arInputNode::_setSignature(int numButtons, int numAxes, int numMatrices){
  // delete old storage if necessary!
  if (numButtons < 0) {
    cerr << "arInputNode warning: attempt to set button signature < 0." << endl
         << "   Setting to 0.\n";
    numButtons = 0;
  }
  if (numAxes < 0) {
    cerr << "arInputNode warning: attempt to set axis signature < 0." << endl
         << "   Setting to 0.\n";
    numAxes = 0;
  }
  if (numMatrices < 0) {
    cerr << "arInputNode warning: attempt to set matrix signature < 0." << endl
         << "   Setting to 0.\n";
    numMatrices = 0;
  }
  _inputState.setSignature( (unsigned int) numButtons,
                            (unsigned int) numAxes,
                            (unsigned int) numMatrices );
}


void arInputNode::_remapData( unsigned int channelNumber, arStructuredData* data ) {
  // first, we massage the data a little bit... this will become MUCH
  // more elaborate over time
  const int sig[3] = { _inputState.getNumberButtons(),
                       _inputState.getNumberAxes(),
                       _inputState.getNumberMatrices() };
  if (!data->dataIn(_inp._SIGNATURE,sig,AR_INT,3)) {
    cerr << "arInputNode warning: problem in receiveData.\n";
  }

  // loop through events.  For each event, change its index to 
  // index + sum for i=0 to channelNumber-1 of e.g. _deviceButton[i] so we
  // don't get collisions.
  unsigned int i, buttonOffset=0, axisOffset=0, matrixOffset=0;
  if (!_inputState.getButtonOffset( channelNumber, buttonOffset ))
    cerr << "arInputNode warning: failed to get button offset for device"
         << channelNumber << " from arInputState.\n";
  if (!_inputState.getAxisOffset( channelNumber, axisOffset ))
    cerr << "arInputNode warning: failed to get axis offset for device"
         << channelNumber << " from arInputState.\n";
  if (!_inputState.getMatrixOffset( channelNumber, matrixOffset ))
    cerr << "arInputNode warning: failed to get matrix offset for device"
         << channelNumber << " from arInputState.\n";
  
  int numEvents = data->getDataDimension(_inp._TYPES);
  for (i=0; (int)i<numEvents; ++i) {
    int eventIndex = ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i];
    int eventType = ((int*)data->getDataPtr(_inp._TYPES,AR_INT))[i];
    if (eventType==AR_EVENT_BUTTON){
      ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i] 
        = eventIndex +buttonOffset;
    }
    else if (eventType==AR_EVENT_AXIS){
      ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i] 
        = eventIndex +axisOffset;
    }
    else if (eventType==AR_EVENT_MATRIX){
      ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i] 
        = eventIndex +matrixOffset;
    }
    else
      cerr << "arInputNode warning: ignoring unexpected eventType\n"
           << "  (not button, axis, or matrix).\n";
  }
}

void arInputNode::_filterEventQueue( arInputEventQueue& queue ) {
  unsigned int filterNumber = 0;
  arFilterIterator f;
  std::vector< arInputState >::iterator stateIter = _filterStates.begin();
  for (f = _inputFilterList.begin(); f != _inputFilterList.end(); ++f) {
     arInputState* statePtr;
     if (stateIter == _filterStates.end()) {
      cerr << "arInputNode error: reading past end of filterStates array.\n";
      statePtr = &_inputState; // do something that may not be too bad? or return?
    } else
      statePtr = (arInputState*)&(*stateIter);
    if (!((*f)->filter( &queue, statePtr )))
      cerr << "arInputNode warning: filter # " << filterNumber << " failed.\n";
    filterNumber++;
    stateIter++;
  }
}

void arInputNode::_updateState( arInputEventQueue& queue ) {
  while (!queue.empty()) {
    arInputEvent thisEvent = queue.popNextEvent();
    if (thisEvent){
      _inputState.update( thisEvent );
      if (_eventCallback)
        _eventCallback( thisEvent );
    }
  }
}
    

