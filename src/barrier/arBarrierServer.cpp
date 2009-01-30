//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBarrierServer.h"

void ar_barrierDataFunction(arStructuredData* data, void* server,
                               arSocket* theSocket) {
  ((arBarrierServer*)server)->_barrierDataFunction(data, theSocket);
}

void arBarrierServer::_barrierDataFunction(arStructuredData* data,
                                           arSocket* theSocket) {
  const int id = data->getID();
  if (id == _handshakeData->getID()) {
    arGuard _(_queueActivationLock, "arBarrierServer::_barrierDataFunction _handshakeData");
    const int bondedSocketID = data->getDataInt(BONDED_ID);
    _activationSocketIDs.push_back(pair<int, int>(theSocket->getID(), bondedSocketID));
    // If the signal object has been set and no one is connected yet,
    // send a signal here as well.
    if (getNumberConnectedActive()==0 && _pumpPrimingFlag) {
      if (_signalObject) {
        _signalObject->sendSignal();
      }
      _pumpPrimingFlag = false;
    }
  }
  else if (id == _responseData->getID()) {
    arGuard _(_activationLock, "arBarrierServer::_barrierDataFunction _responseData");
    _activationResponse = true;
    _activationVar.signal();
  }
  else if (id == _clientTuningData->getID()) {
    int theData[4];
    data->dataOut(CLIENT_TUNING_DATA, theData, AR_INT, 4);
    _drawTime = theData[0];
    _rcvTime = theData[1];
    _procTime = theData[2];
    _frameNum = theData[3];
    arGuard _(_waitingLock, "arBarrierServer::_barrierDataFunction _clientTuningData");
    _totalWaiting++;
    _waitingCondVar.signal();
  }
  else {
    cerr << "arBarrierServer warning: ignoring unknown record.\n";
  }
}

void ar_connectionFunction(void* barrierServer) {
  arBarrierServer* s = (arBarrierServer*) barrierServer;
  while (s->_runThreads) {
    // Accept a connection.
    if (!s->_dataServer.acceptConnectionNoSend()) {
      s->_runThreads = false;
      break; // something bad happened.  Don't keep trying infinitely.
      }
    // Connected.
  }
}

void ar_releaseFunction(void* server) {
  ((arBarrierServer*)server)->_releaseFunction();
}

void arBarrierServer::_releaseFunction() {
  while (_runThreads) {
    _waitingLock.lock("arBarrierServer::_releaseFunction");
    while (true) {
      int total = getNumberConnectedActive();
      if (_localConnection)
        ++total;
      if (_totalWaiting >= total && total > 0) {
        _totalWaiting = 0;
        break;
      }
      _waitingCondVar.wait(_waitingLock);
    }
    // send release packet
    const int tuningData = _serverSendSize;
    if (!_serverTuningData->dataIn(
           SERVER_TUNING_DATA, &tuningData, AR_INT, 1) ||
        !_dataServer.sendData(_serverTuningData)) {
      // cerr << "arBarrierServer warning: problem in ar_releaseFunction.\n";
      // Don't complain, probably a client just disconnected from this master.
    }
    _waitingLock.unlock();

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

void ar_barrierDisconnectFunction(void* server, arSocket*) {
  ((arBarrierServer*)server)->_barrierDisconnectFunction();
}

void arBarrierServer::_barrierDisconnectFunction() {
  _waitingLock.lock("arBarrierServer::_barrierDisconnectFunction wait");
  _waitingCondVar.signal();
  _waitingLock.unlock();

  // Race condition?
  arGuard _(_queueActivationLock, "arBarrierServer::_barrierDisconnectFunction queueActivate");
  if (getNumberConnected() <= 0)
    _pumpPrimingFlag = true;
}

arBarrierServer::arBarrierServer():
  _client(NULL),
  _serviceName("NULL"),
  _totalWaiting(0),
  _waitingCondVar("arBarrierServer-wait"),
  _started(false),
  _runThreads(false),
  _signalObject(NULL),
  _signalObjectRelease(NULL),
  _dataServer(10000),
  _handshakeTemplate("handshake"),
  _responseTemplate("response"),
  _clientTuningTemplate("client tuning"),
  _serverTuningTemplate("server tuning"),
  _activationVar("arBarrierServer-activate"),
  _activationQueueLockedExternally(false),
  _activationResponse(false),
  _pumpPrimingFlag(true),
  _localConnection(false),
  _exitProgram(false),
  _channel("NULL") {

  _dataServer.setConsumerFunction(ar_barrierDataFunction);
  _dataServer.setConsumerObject(this);
  _dataServer.setDisconnectFunction(ar_barrierDisconnectFunction);
  _dataServer.setDisconnectObject(this);
  _dataServer.smallPacketOptimize(true);

  // Set up the language.
  BONDED_ID = _handshakeTemplate.add("bonded ID", AR_INT);
  CLIENT_TUNING_DATA = _clientTuningTemplate.add("client tuning data", AR_INT);
  SERVER_TUNING_DATA = _serverTuningTemplate.add("server tuning data", AR_INT);

  _theDictionary.add(&_handshakeTemplate);
  _theDictionary.add(&_responseTemplate);
  _theDictionary.add(&_clientTuningTemplate);
  _theDictionary.add(&_serverTuningTemplate);

  _handshakeData = new arStructuredData(&_handshakeTemplate);
  _responseData = new arStructuredData(&_responseTemplate);
  _clientTuningData = new arStructuredData(&_clientTuningTemplate);
  _serverTuningData = new arStructuredData(&_serverTuningTemplate);
}

arBarrierServer::~arBarrierServer() {
  _runThreads = false;
  // bug: memory leaks (but we need a shutdown procedure first)
}

void arBarrierServer::setServiceName(string serviceName) {
  _serviceName = serviceName;
}

// Store a pointer to the arSZGClient.
bool arBarrierServer::init(const string& serviceName, const string& channel, arSZGClient& client) {
  _serviceName = serviceName;
  _channel = channel;
  _client = &client;
  return true;
}

bool arBarrierServer::start() {
  if (!_client) {
    cerr << "arBarrierServer error: init not called.\n";
    return false;
  }
  if (_channel == "NULL") {
    cerr << "arBarrierServer error: no channel.\n";
    return false;
  }
  // register the service and get some ports
  int port = -1;
  if (!_client->registerService(_serviceName, _channel, 1, &port)) {
    cerr << "arBarrierServer error: failed to register service \""
         << _serviceName << "\".\n";
    return false;
  }

  // todo: factor out copy-paste with barrier/arSyncDataServer.cpp:189
  _dataServer.setPort(port);
  _dataServer.setInterface("INADDR_ANY");
  bool success = false;
  for (int tries = 0; tries < 10; ++tries) {
    if (_dataServer.beginListening(&_theDictionary)) {
      success = true;
      break;
    }
    cerr << "arBarrierServer warning: failed to listen on brokered port, retrying.\n";
    _client->requestNewPorts(_serviceName, _channel, 1, &port);
    _dataServer.setPort(port);
  }
  if (!success) {
    // failed to bind to ports
    cerr << "arBarrierServer error: failed to listen on brokered port.\n";
    return false;
  }
  if (!_client->confirmPorts(_serviceName, _channel, 1, &port)) {
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

// Disable the localSync call so that it
// automatically falls through. It would be a BAD IDEA, at this stage,
// to make the barrier server stop sending replies to its clients, since
// those clients rely on receiving messages to stop their data reading
// threads
void arBarrierServer::stop() {
  _exitProgram = true;
  _localSignal.sendSignal();
}

// todo: needs error handling
bool arBarrierServer::setServerSendSize(int serverSize) {
  _serverSendSize = serverSize;
  return true;
}

// todo: needs error handling
void arBarrierServer::setSignalObject(arSignalObject* signalObject) {
  _signalObject = signalObject;
}

// todo: needs error handling
void arBarrierServer::setSignalObjectRelease(arSignalObject* signalObject) {
  _signalObjectRelease = signalObject;
}

bool arBarrierServer::activatePassiveSockets(arDataServer* bondedServer) {
  // Only issue a locking command if this hasn't been issued externally.
  // Locking dance has a subtle deadlock danger:
  // if we held _queueActivationLock
  // through the end of the function, we could block in
  // ar_barrierDataFunction on the first round of a handshake receive
  // (which wants _queueActivationLock) but then never be able to
  // get the third handshake round, which also must pass through that
  // API point, thus deadlocking in _activationVar.wait().
  if (!_activationQueueLockedExternally) {
    _queueActivationLock.lock("arBarrierServer::activatePassiveSockets A");
  }
  list< pair<int, int> >::iterator iter;
  list< pair<int, int> > tmp;
  for (iter = _activationSocketIDs.begin(); iter != _activationSocketIDs.end(); ++iter) {
    tmp.push_back(*iter);
  }
  _activationSocketIDs.clear();
  _queueActivationLock.unlock(); // Even if _activationQueueLockedExternally was true.
  _activationQueueLockedExternally = false;

  // Update the number of active connections atomically w.r.t. the barrier release.
  // Also atomically update global "heartbeat", which lets remote arBarrierClients
  // discard unwanted broadcast release packets.

  arGuard _(_waitingLock, "arBarrierServer::activatePassiveSockets B");

  for (iter = tmp.begin(); iter != tmp.end(); ++iter) {
    arSocket* theSocket = _dataServer.getConnectedSocket(iter->first);
    _activationLock.lock("arBarrierServer::activatePassiveSockets C");
      _activationResponse = false;
    _activationLock.unlock();

    // Send 2nd round of handshake to the client.
    _dataServer.sendData(_handshakeData, theSocket);

    _activationLock.lock("arBarrierServer::activatePassiveSockets D");
    while (!_activationResponse) {
      _activationVar.wait(_activationLock);
    }
    // Got the response.
    _activationLock.unlock();

    _dataServer.activatePassiveSocket(iter->first);
    if (bondedServer) {
      bondedServer->activatePassiveSocket(iter->second);
    }
  }

  return true;
}

bool arBarrierServer::checkWaitingSockets() {
  return !_activationSocketIDs.empty();
}

void arBarrierServer::lockActivationQueue() {
  _queueActivationLock.lock("arBarrierServer::lockActivationQueue");
  _activationQueueLockedExternally = true;
}

void arBarrierServer::unlockActivationQueue() {
  _queueActivationLock.unlock();
  _activationQueueLockedExternally = false;
}

list<arSocket*>* arBarrierServer::getWaitingBondedSockets
                                       (arDataServer* bondedServer) {
  list<arSocket*>* result = new list<arSocket*>;

  if (!_activationQueueLockedExternally)
    _queueActivationLock.lock("arBarrierServer::getWaitingBondedSockets");

  for (list< pair<int, int> >::iterator iter = _activationSocketIDs.begin();
       iter != _activationSocketIDs.end(); ++iter) {
    result->push_back(bondedServer->getConnectedSocket(iter->second));
  }

  if (!_activationQueueLockedExternally)
    _queueActivationLock.unlock();

  return result;
}

void arBarrierServer::registerLocal() {
  _localConnection = true;
}

void arBarrierServer::localSync() {
  if (_exitProgram)
    return;

  if (getNumberConnectedActive() == 0)
    ar_usleep(10000);
  _waitingLock.lock("arBarrierServer::localSync");
    _totalWaiting++;
    _waitingCondVar.signal();
  _waitingLock.unlock();
  _localSignal.receiveSignal();
}

int arBarrierServer::getNumberConnected() const {
  return _dataServer.getNumberConnected();
}

int arBarrierServer::getNumberConnectedActive() const {
  return _dataServer.getNumberConnectedActive();
}
