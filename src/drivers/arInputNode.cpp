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
  _label = szgClient.getLabel();

  // Initialize the input sources registered to this node.
  for (arSourceIterator i = _inputSourceList.begin();
       i != _inputSourceList.end();
       ++i){
    // If one fails, the whole thing fails.
    if (!(*i)->init(szgClient)) {
      // todo: add arInputSource::_label (dll's name), to print it here.
      ar_log_warning() << _label << " failed to init input source.\n";
      return false;
    }

    _inputState.addInputDevice(
      (*i)->getNumberButtons(), (*i)->getNumberAxes(), (*i)->getNumberMatrices());
  }

  // Initialize the input sinks registered to this node.
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
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
  for (arSourceIterator i = _inputSourceList.begin();
       i != _inputSourceList.end();
       ++i){
    ok &= (*i)->start();
  }
  for (arSinkIterator j = _inputSinkList.begin();
       j != _inputSinkList.end();
       ++j){
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
    // init() already complained.
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

  if (channelNumber < 0) {
    ar_log_warning() << _label << ": negative channel number.\n";
    return;
  }
  
  _lock();
  _remapData( (unsigned int) channelNumber, data );

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
    ar_log_warning() << _label << " arInputNode ignoring out-of-range channel "
                   << whichChannel << ".\n";
    _unlock();
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
    ar_log_warning() << "arInputNode ignoring NULL source.\n";
    return;
  }

  _inputSourceList.push_back( theSource );
  theSource->_setInputSink( _currentChannel++, this );
  _iOwnSources.push_back( iOwnIt );
}

int arInputNode::addFilter( arIOFilter* theFilter, bool iOwnIt ){
  if (!theFilter) {
    ar_log_warning() << "arInputNode ignoring NULL filter.\n";
    return -1;
  }

  const int newID = _findUnusedFilterID();
  theFilter->setID( newID );
  _inputFilterList.push_back( theFilter );
  _iOwnFilters.push_back( iOwnIt );
  _filterStates.push_back(
    _filterStates.empty() ? _inputState : _filterStates.back());
  ar_log_remark() << "arInputNode installed event filter with new ID " << newID << ".\n";
  return newID;
}

bool arInputNode::removeFilter( int filterID ) {
  ar_log_remark() << "arInputNode removing event filter with ID " << filterID << ".\n";
  _lock();
  unsigned int filterNumber = 0;
  arFilterIterator f;
  for (f = _inputFilterList.begin(); f != _inputFilterList.end(); ++f) {
    if ((*f)->getID() == filterID) {
      ar_log_remark() << _label << " arInputNode found filter with ID " << filterID << ".\n";
      _inputFilterList.erase(f);
      _iOwnFilters.erase( _iOwnFilters.begin() + filterNumber );
      _filterStates.erase( _filterStates.begin() + filterNumber );
      _unlock();
      return true;
    }
    ++filterNumber;
  }
  _unlock();
  ar_log_warning() << _label << " arInputNode: no filter ID " << filterID << ".\n";
  return false;
}


bool arInputNode::replaceFilter( int filterID, arIOFilter* newFilter, bool iOwnIt ) {
  ar_log_remark() << "arInputNode replacing event filter with ID " << filterID << ".\n";
  _lock();
  unsigned int filterNumber = 0;
  arFilterIterator f;
  for (f = _inputFilterList.begin(); f != _inputFilterList.end(); ++f) {
    if ((*f)->getID() == filterID) {
      ar_log_remark() << "arInputNode found filter with ID " << filterID << ".\n";
      newFilter->setID( filterID );
      *f = newFilter;
      _iOwnFilters[filterNumber] = iOwnIt;
      _filterStates[filterNumber] = arInputState();
      _unlock();
      return true;
    }
    ++filterNumber;
  }
  _unlock();
  ar_log_warning() << "arInputNode: no filter ID " << filterID << ".\n";
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

void arInputNode::_lock() {
  ar_mutex_lock( &_dataSerializationLock );
}

void arInputNode::_unlock() {
  ar_mutex_unlock( &_dataSerializationLock );
}

void arInputNode::_setSignature(int numButtons, int numAxes, int numMatrices){
  // delete old storage if necessary!
  if (numButtons < 0) {
    ar_log_warning() << "arInputNode overriding negative button signature.\n";
    numButtons = 0;
  }
  if (numAxes < 0) {
    ar_log_warning() << "arInputNode overriding negative axis signature.\n";
    numAxes = 0;
  }
  if (numMatrices < 0) {
    ar_log_warning() << "arInputNode overriding negative matrix signature.\n";
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
  if (!data->dataIn(_inp._SIGNATURE,sig,AR_INT,3))
    ar_log_warning() << "arInputNode problem in receiveData.\n";

  // loop through events.  For each event, change its index to 
  // index + sum for i=0 to channelNumber-1 of e.g. _deviceButton[i] so we
  // don't get collisions.
  unsigned int i, buttonOffset=0, axisOffset=0, matrixOffset=0;
  if (!_inputState.getButtonOffset( channelNumber, buttonOffset ))
    ar_log_warning() << "arInputNode failed to get button offset for device"
                     << channelNumber << " from arInputState.\n";
  if (!_inputState.getAxisOffset( channelNumber, axisOffset ))
    ar_log_warning() << "arInputNode failed to get axis offset for device"
                     << channelNumber << " from arInputState.\n";
  if (!_inputState.getMatrixOffset( channelNumber, matrixOffset ))
    ar_log_warning() << "arInputNode failed to get matrix offset for device"
                     << channelNumber << " from arInputState.\n";
  
  const int numEvents = data->getDataDimension(_inp._TYPES);
  for (i=0; (int)i<numEvents; ++i) {
    int eventIndex = ((int*)data->getDataPtr(_inp._INDICES,AR_INT))[i];
    int eventType = ((int*)data->getDataPtr(_inp._TYPES,AR_INT))[i];
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
  unsigned int filterNumber = 0;
  arFilterIterator f;
  std::vector< arInputState >::iterator stateIter = _filterStates.begin();
  for (f = _inputFilterList.begin(); f != _inputFilterList.end(); ++f) {
     arInputState* statePtr = NULL;
     if (stateIter == _filterStates.end()) {
      ar_log_warning() << "arInputNode reading past end of filterStates array.\n";
      statePtr = &_inputState; // do something that may not be too bad? or return?
    } else {
      statePtr = (arInputState*)&*stateIter;
    }
    if (!(*f)->filter( &queue, statePtr ))
      ar_log_warning() << " arInputNode filter # " << filterNumber << " failed.\n";
    ++filterNumber;
    ++stateIter;
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
  std::list<arIOFilter*>::iterator iter;
  int id = 1;
  bool done = false;
  while (!done) {
    done = true;
    for (iter = _inputFilterList.begin(); iter != _inputFilterList.end(); ++iter) {
      if ((*iter)->getID() == id) {
        ++id;
        done = false;
        break;
      }
    }
  }
  return id;
}
