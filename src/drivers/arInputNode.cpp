//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInputNode.h"
#include "arEventUtilities.h"
#include "arLogStream.h"

arInputNode::arInputNode( bool bufferEvents ) :
  _eventCallback(NULL),
  _currentChannel(0),
  _bufferInputEvents(bufferEvents),
  _complained(false),
  _initOK(false),
  _label("arInputNode")
{
}

arInputNode::~arInputNode() {

#if 0
// Bug: bad things happen.
  iterSrc sourceIter;
  iterSink sinkIter;
  iterFlt filterIter;
  std::vector<bool>::iterator iter;
  
  for (sourceIter = _sources.begin(), iter = _iOwnSources.begin();
         sourceIter != _sources.end() && iter != _iOwnSources.end();
         sourceIter++, iter++) {
    if (*iter)
      delete *sourceIter;
  }
  for (sinkIter = _sinks.begin(), iter = _iOwnSinks.begin();
         sinkIter != _sinks.end() && iter != _iOwnSinks.end();
         sinkIter++, iter++) {
    if (*iter)
      delete *sinkIter;
  }
  for (filterIter = _filters.begin(), iter = _iOwnFilters.begin();
         filterIter != _filters.end() && iter != _iOwnFilters.end();
         filterIter++, iter++) {
    if (*iter)
      delete *filterIter;
  }
#endif

  _filters.clear();
  _sinks.clear();
  _sources.clear();
  _iOwnSources.clear();
  _iOwnFilters.clear();
  _iOwnSinks.clear();
  _filterStates.clear();
  _eventBuffer.clear();
}

bool arInputNode::init(arSZGClient& szgClient){
  _initOK = false;
  _label = szgClient.getLabel();

  // Initialize the registered input sources.
  for (iterSrc i = _sources.begin(); i != _sources.end(); ++i){
    // If one fails, the whole thing fails.
    if (!(*i)->init(szgClient)) {
      ar_log_warning() << _label << " failed to init input source.\n";
      return false;
    }
    _inputState.addInputDevice(
      (*i)->getNumberButtons(), (*i)->getNumberAxes(), (*i)->getNumberMatrices());
  }

  // Initialize the registered input sinks.
  for (iterSink j = _sinks.begin(); j != _sinks.end(); ++j){
    // If one fails, the whole thing fails.
    if (!(*j)->init(szgClient)){
      ar_log_warning() << _label << " failed to init input sink.\n";
      return false;
    }
  }
  _initOK = true;
  return true;
}

bool arInputNode::start(){
  if (!_initOK)
    // init() already complained.
    return false;

  _complained = false;
  bool ok = true;
  for (iterSrc i = _sources.begin(); i != _sources.end(); ++i){
    ok &= (*i)->start();
  }
  for (iterSink j = _sinks.begin(); j != _sinks.end(); ++j){
    if ( (*j)->_autoActivate ){
      // Skip the file logging input sink, which auto-activates in DeviceServer.
      ok &= (*j)->start();
    }
  }
  return ok;
}

bool arInputNode::stop(){
  if (!_initOK)
    // init() already complained.
    return false;

  bool ok = true;
  for (iterSrc i = _sources.begin(); i != _sources.end(); ++i){
    ok &= (*i)->stop();
  }
  for (iterSink j = _sinks.begin(); j != _sinks.end(); ++j){
    ok &= (*j)->stop();
  }
  return ok;
}

bool arInputNode::restart(){
  if (!_initOK)
    // init() already complained.
    return false;

  bool ok = true;
  for (iterSrc i = _sources.begin(); i != _sources.end(); ++i){
    ok &= (*i)->restart();
  }
  for (iterSink j = _sinks.begin(); j != _sinks.end(); ++j){
    ok &= (*j)->restart();
  }
  return ok;
}

void arInputNode::receiveData(int channelNumber, arStructuredData* data) {

  if (channelNumber < 0) {
    ar_log_warning() << _label << ": negative channel number.\n";
    return;
  }
  
  _lock();
  _remapData( unsigned(channelNumber), data );

  _eventQueue.clear();
  if (!ar_setEventQueueFromStructuredData( &_eventQueue, data )) {
    ar_log_warning() << _label << " arInputNode failed to convert received data to event queue.\n";
LAbort:
    _unlock();
    return;
  }
  
  if (_bufferInputEvents) {
    _eventBuffer.appendQueue( _eventQueue );
    _eventQueue.clear();
    goto LAbort;
  }
  
  _filterEventQueue( _eventQueue );
  if (!ar_saveEventQueueToStructuredData( &_eventQueue, data )){
    ar_log_warning() << _label << " arInputNode failed to convert event queue to arStructuredData.\n";
  }

  // Update node's arInputState, and empty queue.
  _updateState( _eventQueue );
  
  // Forward this to the input sinks.
  for (iterSink j = _sinks.begin(); j != _sinks.end(); ++j){
    (*j)->receiveData(channelNumber, data);
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

// Called when a connected devices has changed its signature.
// Bug: called too many times, resetting the signature n times instead of just once.

bool arInputNode::sourceReconfig(int whichChannel){
  _lock();

  if (whichChannel < 0 || whichChannel >= (int)_sources.size()) {
    ar_log_warning() << _label << " arInputNode ignoring out-of-range channel "
                   << whichChannel << ".\n";
    _unlock();
    return false;
  }

  unsigned j=0;
  for (iterSrc i = _sources.begin(); i != _sources.end(); ++i, ++j) {
    if ((int)j != whichChannel)
      continue;

    // Found the whichChannel'th member of _sources (std::list is slow, oh well).
    _inputState.remapInputDevice(whichChannel,
      (*i)->getNumberButtons(), (*i)->getNumberAxes(), (*i)->getNumberMatrices());
    _unlock();
    return true;
  }    

  ar_log_warning() << _label << " arInputNode internally missing a channel.\n";
  _unlock();
  return false;
}

void arInputNode::addInputSource( arInputSource* src, bool iOwnIt ){
  if (!src) {
    ar_log_warning() << "arInputNode ignoring NULL source.\n";
    return;
  }

  _sources.push_back( src );
  src->_setInputSink( _currentChannel++, this );
  _iOwnSources.push_back( iOwnIt );
}

int arInputNode::addFilter( arIOFilter* theFilter, bool iOwnIt ){
  if (!theFilter) {
    ar_log_warning() << "arInputNode ignoring NULL filter.\n";
    return -1;
  }

  const int ID = _findUnusedFilterID();
  theFilter->setID( ID );
  _filters.push_back( theFilter );
  _iOwnFilters.push_back( iOwnIt );
  _filterStates.push_back(
    _filterStates.empty() ? _inputState : _filterStates.back());
  ar_log_debug() << "arInputNode: new filter with ID " << ID << "\n";
  return ID;
}

bool arInputNode::removeFilter( int ID ) {
  _lock();
  unsigned filterNumber = 0;
  for (iterFlt f = _filters.begin(); f != _filters.end(); ++f,++filterNumber) {
    if ((*f)->getID() != ID)
      continue;

    ar_log_remark() << _label << " arInputNode removing filter with ID " << ID << ".\n";
    _filters.erase(f);
    _iOwnFilters .erase( _iOwnFilters .begin() + filterNumber );
    _filterStates.erase( _filterStates.begin() + filterNumber );
    _unlock();
    return true;
  }

  _unlock();
  ar_log_warning() << _label << " arInputNode: no filter with ID " << ID << " to remove.\n";
  return false;
}


bool arInputNode::replaceFilter( int ID, arIOFilter* newFilter, bool iOwnIt ) {
  _lock();
  unsigned filterNumber = 0;
  for (iterFlt f = _filters.begin(); f != _filters.end(); ++f,++filterNumber) {
    if ((*f)->getID() != ID)
      continue;

    ar_log_remark() << _label << " arInputNode replacing filter with ID " << ID << ".\n";
    newFilter->setID( ID );
    *f = newFilter;
    _iOwnFilters[filterNumber] = iOwnIt;
    _filterStates[filterNumber] = arInputState();
    _unlock();
    return true;
  }

  _unlock();
  ar_log_warning() << "arInputNode: no filter with ID " << ID << " to replace.\n";
  return false;
}


void arInputNode::addInputSink( arInputSink* theSink, bool iOwnIt ){
  if (_bufferInputEvents) {
    ar_log_warning() << "arInputNode event buffer ignoring attempted sink.\n";
    return;
  }
  
  if (!theSink) {
    ar_log_warning() << "arInputNode ignoring NULL sink.\n";
    return;
  }

  _sinks.push_back(theSink);
  _iOwnSinks.push_back( iOwnIt );
}

int arInputNode::getButton(int i) {
  return _inputState.getButton(unsigned(i));
}

float arInputNode::getAxis(int i){
  return _inputState.getAxis(unsigned(i));
}

arMatrix4 arInputNode::getMatrix(int i){
  return _inputState.getMatrix(unsigned(i));
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

void arInputNode::_lock() {
  _dataSerializationLock.lock();
}

void arInputNode::_unlock() {
  _dataSerializationLock.unlock();
}

void arInputNode::_setSignature(int numButtons, int numAxes, int numMatrices){
  if (numButtons < 0) {
    numButtons = 0;
    ar_log_warning() << "arInputNode overriding negative button signature, to 0.\n";
  }
  if (numAxes < 0) {
    numAxes = 0;
    ar_log_warning() << "arInputNode overriding negative axis signature, to 0.\n";
  }
  if (numMatrices < 0) {
    numMatrices = 0;
    ar_log_warning() << "arInputNode overriding negative matrix signature, to 0.\n";
  }
  _inputState.setSignature( unsigned(numButtons), unsigned(numAxes), unsigned(numMatrices) );
  // todo: delete old storage if necessary!
}


void arInputNode::_remapData( unsigned channelNumber, arStructuredData* data ) {
  // massage the data... this will become MUCH more elaborate
  const int sig[3] = { _inputState.getNumberButtons(),
                       _inputState.getNumberAxes(),
                       _inputState.getNumberMatrices() };
  if (!data->dataIn(_inp._SIGNATURE,sig,AR_INT,3))
    ar_log_warning() << "arInputNode problem in receiveData.\n";

  // loop through events.  For each event, change its index to 
  // index + sum for i=0 to channelNumber-1 of e.g. _deviceButton[i] so we
  // don't get collisions.
  unsigned i, buttonOffset=0, axisOffset=0, matrixOffset=0;
  if (!_inputState.getButtonOffset( channelNumber, buttonOffset ))
    ar_log_warning() << "arInputNode got no button offset for device"
                     << channelNumber << " from arInputState.\n";
  if (!_inputState.getAxisOffset( channelNumber, axisOffset ))
    ar_log_warning() << "arInputNode got no axis offset for device"
                     << channelNumber << " from arInputState.\n";
  if (!_inputState.getMatrixOffset( channelNumber, matrixOffset ))
    ar_log_warning() << "arInputNode got no matrix offset for device"
                     << channelNumber << " from arInputState.\n";
  
  const int numEvents = data->getDataDimension(_inp._TYPES);
  for (i=0; (int)i<numEvents; ++i) {
    const int eventIndex = ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i];
    const int eventType = ((int*)data->getDataPtr(_inp._TYPES,AR_INT))[i];
    if (eventType==AR_EVENT_BUTTON){
      ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i] = eventIndex + buttonOffset;
    }
    else if (eventType==AR_EVENT_AXIS){
      ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i] = eventIndex + axisOffset;
    }
    else if (eventType==AR_EVENT_MATRIX){
      ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i] = eventIndex + matrixOffset;
    }
    else
      ar_log_warning() << "arInputNode ignoring unexpected eventType (not button, axis, or matrix).\n";
  }
}

void arInputNode::_filterEventQueue( arInputEventQueue& queue ) {
  // todo: assert that _lock() has been called, by setting a flag in _lock().
  unsigned filterNumber = 0;
  std::vector< arInputState >::iterator iterState = _filterStates.begin();
  for (iterFlt f = _filters.begin(); f != _filters.end(); ++f) {
     arInputState* statePtr = NULL;
     if (iterState == _filterStates.end()) {
      ar_log_warning() << "arInputNode passed end of filterStates.\n";
      statePtr = &_inputState; // do something that may not be too bad? or return?
    } else {
      statePtr = (arInputState*)&*iterState;
    }
    if (!(*f)->filter( &queue, statePtr ))
      ar_log_warning() << " arInputNode filter # " << filterNumber << " failed.\n";
    ++filterNumber;
    ++iterState;
  }
}

void arInputNode::_updateState( arInputEventQueue& queue ) {
  while (!queue.empty()) {
    arInputEvent thisEvent(queue.popNextEvent());
    if (thisEvent){
      _inputState.update( thisEvent );
      if (_eventCallback)
        _eventCallback( thisEvent );
    }
  }
}
    
int arInputNode::_findUnusedFilterID() {
  int id = 1;
  bool done = false;
  while (!done) {
    done = true;
    for (iterFlt iter = _filters.begin(); iter != _filters.end(); ++iter) {
      if ((*iter)->getID() != id)
        continue;
      ++id;
      done = false;
      break;
    }
  }
  return id;
}
