//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBarrierServer.h"

void ar_barrierDataFunction(arStructuredData* data, void* server,
                               arSocket* theSocket){
  ((arBarrierServer*)server)->_barrierDataFunction(data, theSocket);
}

void arBarrierServer::_barrierDataFunction(arStructuredData* data,
                                           arSocket* theSocket){
  const int id = data->getID();
  if (id == _handshakeData->getID()) {
    ar_mutex_lock(&_queueActivationLock);
    const int bondedSocketID = data->getDataInt(BONDED_ID);
    _activationSocketIDs.push_back(
                           pair<int,int>(theSocket->getID(),bondedSocketID));
    // If the signal object has been set and no one is connected yet,
    // send a signal here as well.
    if (getNumberConnectedActive()==0 && _pumpPrimingFlag){
      if (_signalObject){
        _signalObject->sendSignal();
      }
      _pumpPrimingFlag = false;
    }
    ar_mutex_unlock(&_queueActivationLock);
  }
  else if (id == _responseData->getID()) {
    ar_mutex_lock(&_activationLock);
    _activationResponse = true;
    _activationVar.signal();
    ar_mutex_unlock(&_activationLock);
  }
  else if (id == _clientTuningData->getID()) {
    int theData[4];
    data->dataOut(CLIENT_TUNING_DATA,theData,AR_INT,4);
    _drawTime = theData[0];
    _rcvTime = theData[1];
    _procTime = theData[2];
    _frameNum = theData[3];
    ar_mutex_lock(&_waitingLock);
    _totalWaiting++;
    _waitingCondVar.signal();
    ar_mutex_unlock(&_waitingLock);
  }
  else {
    cerr << "arBarrierServer warning: ignoring unknown record.\n";
  }
}

void ar_connectionFunction(void* barrierServer){
  arBarrierServer* s = (arBarrierServer*) barrierServer;
  while (s->_runThreads) {
    // cout << "arBarrierServer remark: about to accept a connection!\n";
    if (!s->_dataServer.acceptConnectionNoSend()) {
      s->_runThreads = false;
      break; // something bad happened.  Don't keep trying infinitely.
      }
    // cout << "arBarrierServer remark: connected.\n";
  }
}

void ar_releaseFunction(void* server){
  ((arBarrierServer*)server)->_releaseFunction();
}
  
void arBarrierServer::_releaseFunction(){
  while (_runThreads){
    ar_mutex_lock(&_waitingLock);
    while (true) {
      int total = _dataServer.getNumberConnectedActive();
      if (_localConnection)
        ++total;
      if (_totalWaiting >= total && total >0) {
        _totalWaiting = 0;
	break;
      }
      else{
        _waitingCondVar.wait(&_waitingLock);
      }
    }
    // send release packet
    const int tuningData = _serverSendSize;
    if (!_serverTuningData->dataIn(
           SERVER_TUNING_DATA, &tuningData,AR_INT,1) ||
        !_dataServer.sendData(_serverTuningData)) {
      // cerr << "arBarrierServer warning: problem in ar_releaseFunction.\n";
      // Don't complain, probably a client just disconnected from this master.
    }
    ar_mutex_unlock(&_waitingLock);
   
    // If the _signalObjectRelease is activated, use that to send the
    // external signal that the buffer swap has been activated.
    // Otherwise, use _signalObject.
    // The only other place _signalObject is signalled
    // is in the pump priming on connection, i.e. to recover from the
    // deadlock of "the barrier won't fire until the first buffer has been
    // sent but the first buffer won't be sent until the barrier has fired."
    if (_signalObjectRelease)
      _signalObjectRelease->sendSignal();
    else if (_signalObject)
      _signalObject->sendSignal();
    // Local connection too!
    _localSignal.sendSignal();
  }
}

void ar_barrierDisconnectFunction(void* server, arSocket*){
  ((arBarrierServer*)server)->_barrierDisconnectFunction();
}

void arBarrierServer::_barrierDisconnectFunction(){
  //NOISY cout << "arBarrierServer remark: disconnected.\n";
  ar_mutex_lock(&_waitingLock);
  _waitingCondVar.signal();
  ar_mutex_unlock(&_waitingLock);

  //********************************************************
  // there really is a bit of a race condition in the below
  //********************************************************
  ar_mutex_lock(&_queueActivationLock);
  if (getNumberConnected() <= 0)
    _pumpPrimingFlag = true;
  ar_mutex_unlock(&_queueActivationLock);
}

arBarrierServer::arBarrierServer():
  _client(NULL),
  _serviceName("NULL"),
  _totalWaiting(0),
  _started(false),
  _runThreads(false),
  _signalObject(NULL),
  _signalObjectRelease(NULL),
  _dataServer(10000),
  _handshakeTemplate("handshake"),
  _responseTemplate("response"),
  _clientTuningTemplate("client tuning"),
  _serverTuningTemplate("server tuning"),
  _activationQueueLockedExternally(false),
  _activationResponse(false),
  _pumpPrimingFlag(true),
  _localConnection(false),
  _exitProgram(false),
  _channel("NULL"){

  _dataServer.setConsumerFunction(ar_barrierDataFunction);
  _dataServer.setConsumerObject(this);
  _dataServer.setDisconnectFunction(ar_barrierDisconnectFunction);
  _dataServer.setDisconnectObject(this);
  _dataServer.smallPacketOptimize(true);

  ar_mutex_init(&_waitingLock);
  ar_mutex_init(&_activationLock);
  ar_mutex_init(&_queueActivationLock);

  // Set up the language.
  BONDED_ID = _handshakeTemplate.add("bonded ID",AR_INT);
  CLIENT_TUNING_DATA = _clientTuningTemplate.add("client tuning data",AR_INT);
  SERVER_TUNING_DATA = _serverTuningTemplate.add("server tuning data",AR_INT);

  _theDictionary.add(&_handshakeTemplate);
  _theDictionary.add(&_responseTemplate);
  _theDictionary.add(&_clientTuningTemplate);
  _theDictionary.add(&_serverTuningTemplate);

  _handshakeData = new arStructuredData(&_handshakeTemplate);
  _responseData = new arStructuredData(&_responseTemplate);
  _clientTuningData = new arStructuredData(&_clientTuningTemplate);
  _serverTuningData = new arStructuredData(&_serverTuningTemplate);
}

arBarrierServer::~arBarrierServer(){
  _runThreads = false;
  /// \bug memory leaks (but we need a shutdown procedure first)
}

void arBarrierServer::setServiceName(string serviceName){
  _serviceName = serviceName;
}

/// Does not do much. Just stores a pointer to the arSZGClient
bool arBarrierServer::init(arSZGClient& client){
  _client = &client;
  return true;
}

bool arBarrierServer::start(){
  if (!_client){
    cerr << "arBarrierServer error: init not called.\n";
    return false;
  }
  if (_channel == "NULL"){
    cerr << "arBarrierServer error: no channel.\n";
    return false;
  }
  // register the service and get some ports
  int port = -1;
  if (!_client->registerService(_serviceName,_channel,1,&port)){
    cerr << "arBarrierServer error: failed to register service \""
         << _serviceName << "\".\n";
    return false;
  }

  /// \todo factor out copy-paste with barrier/arSyncDataServer.cpp:189
  _dataServer.setPort(port);
  _dataServer.setInterface("INADDR_ANY");
  bool success = false;
  for (int tries = 0; tries < 10; ++tries) {
    if (_dataServer.beginListening(&_theDictionary)) {
      success = true;
      break;
    }
    cerr << "arBarrierServer warning: failed to listen on brokered port, retrying.\n";
    _client->requestNewPorts(_serviceName,_channel,1,&port);
    _dataServer.setPort(port);
  }
  if (!success) {
    // failed to bind to ports
    cerr << "arBarrierServer error: failed to listen on brokered port.\n";
    return false;
  }
  if (!_client->confirmPorts(_serviceName,_channel,1,&port)){
    cerr << "arBarrierServer error: failed to confirm ports.\n";
    return false;
  }
  // end of copy-paste
  
  _dataServer.atomicReceive(false);
  _started = true;
  _runThreads = true;
  if (!_releaseThread.beginThread(ar_releaseFunction, this)) {
    cerr << "arBarrierServer error: failed to start release thread.\n";
    goto LAbort;
  }
  if (!_connectionThread.beginThread(ar_connectionFunction, this)) {
    cerr << "arBarrierServer error: failed to start connection thread.\n";
LAbort:
    _runThreads = false;
    return false;
    }

  return true;
}

// really all this does (SO FAR) is disable the localSync call so that it
// automatically falls through. It would be a BAD IDEA, at this stage,
// to make the barrier server stop sending replies to its clients, since
// those clients rely on receiving messages to stop their data reading
// threads 
void arBarrierServer::stop(){
  _exitProgram = true;
  _localSignal.sendSignal();
}

/// \todo needs error handling
bool arBarrierServer::setServerSendSize(int serverSize){
  _serverSendSize = serverSize;
  return true;
}

/// \todo needs error handling
void arBarrierServer::setSignalObject(arSignalObject* signalObject){
  _signalObject = signalObject;
}

/// \todo needs error handling
void arBarrierServer::setSignalObjectRelease(arSignalObject* signalObject){
  _signalObjectRelease = signalObject;
}

bool arBarrierServer::activatePassiveSockets(arDataServer* bondedServer){
  // we only want to issue a locking command if this hasn't been issued
  // externally
  // note the funny locking dance. There's a pretty subtle deadlock
  // potentiality... namely, if we held the _queueActivationLock
  // through the end of the function, we could block in
  // ar_barrierDataFunction on the first round of a handshake receive
  // (which wants the _queueActivationLock) but then never be able to
  // get the third handshake round, which also must pass through that
  // API point, consequently blocking in _activationVar.wait() forever
  if (!_activationQueueLockedExternally){
    ar_mutex_lock(&_queueActivationLock);
  }
  list< pair<int,int> >::iterator iter;
  list< pair<int,int> > tmp;
  for (iter = _activationSocketIDs.begin();
       iter != _activationSocketIDs.end();
       ++iter){
    tmp.push_back(*iter);
  }
  _activationSocketIDs.clear();
  // release the lock no matter what... also we set the shadow variable
  // saying we are holding the lock to false
  ar_mutex_unlock(&_queueActivationLock);
  _activationQueueLockedExternally = false;

  // we want to change the number of active connections atomically
  // with respect to the barrier release. also, we have a global
  // "heartbeat" value that lets remote arBarrierClient objects
  // discard unwanted broadcast release packets. This should be updated
  // atomically as well.

  ar_mutex_lock(&_waitingLock);
  
  for (iter = tmp.begin(); iter != tmp.end(); ++iter){
    arSocket* theSocket = _dataServer.getConnectedSocket(iter->first);
    ar_mutex_lock(&_activationLock);
    _activationResponse = false;
    ar_mutex_unlock(&_activationLock);

    // Send 2nd round of handshake to the client.
    _dataServer.sendData(_handshakeData, theSocket);

    // Wait for the response.
    ar_mutex_lock(&_activationLock);
    while (!_activationResponse){
      _activationVar.wait(&_activationLock);
    }
    ar_mutex_unlock(&_activationLock); 

    // Activate the sockets.
    _dataServer.activatePassiveSocket(iter->first);
    if (bondedServer){
      bondedServer->activatePassiveSocket(iter->second);
    }
  }
  
  ar_mutex_unlock(&_waitingLock);
  return true;
}

bool arBarrierServer::checkWaitingSockets(){
  return !(_activationSocketIDs.empty());
}

void arBarrierServer::lockActivationQueue(){
  ar_mutex_lock(&_queueActivationLock);
  _activationQueueLockedExternally = true;
}

void arBarrierServer::unlockActivationQueue(){
  ar_mutex_unlock(&_queueActivationLock);
  _activationQueueLockedExternally = false;
}

list<arSocket*>* arBarrierServer::getWaitingBondedSockets
                                       (arDataServer* bondedServer){
  list<arSocket*>* result = new list<arSocket*>;
  // must lock and unlock if the external locks have not been set
  if (!_activationQueueLockedExternally)
    ar_mutex_lock(&_queueActivationLock);

  for (list< pair<int,int> >::iterator iter = _activationSocketIDs.begin();
       iter != _activationSocketIDs.end();
       ++iter){
    result->push_back(bondedServer->getConnectedSocket(iter->second));
  }

  if (!_activationQueueLockedExternally)
    ar_mutex_unlock(&_queueActivationLock);
  return result;
}

void arBarrierServer::registerLocal(){
  _localConnection = true;
}

void arBarrierServer::localSync(){
  if (_exitProgram)
    return;
  if (_dataServer.getNumberConnectedActive() == 0)
    ar_usleep(10000);
  ar_mutex_lock(&_waitingLock);
  _totalWaiting++;
  _waitingCondVar.signal();
  ar_mutex_unlock(&_waitingLock);
  _localSignal.receiveSignal();
}

int arBarrierServer::getNumberConnected(){
  return _dataServer.getNumberConnected();
}

int arBarrierServer::getNumberConnectedActive(){
  return _dataServer.getNumberConnectedActive();
}
