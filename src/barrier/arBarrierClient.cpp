//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBarrierClient.h"

void ar_barrierClientConnection(void* barrierClient){
  ((arBarrierClient*)barrierClient)->_connectionTask();
}

void arBarrierClient::_connectionTask(){
  _connectionThreadRunning = true;
  if (!_client){
    cerr << "arBarrierClient error: object has not been initialized.\n";
    _keepRunningThread = false;
    return;
  }
  arSleepBackoff a(7, 50, 1.2);
  while (_keepRunningThread && !_exitProgram){
    a.sleep();
    if (_connected)
      continue;
    a.reset();

    _dataClient.closeConnection();

    // This is the one blocking call. Pretend the connection thread isn't running.
    _connectionThreadRunning = false;
    const arPhleetAddress result = _client->discoverService(_serviceName, _networks, true);
    if (_exitProgram)
      break;
    _connectionThreadRunning = true;

    if (!result.valid){
      cerr << getLabel() << " warning: no service '"
           << _serviceName << "' on network '" << _networks << "'.\n";
      continue;
    } 

    _connected = _dataClient.dialUpFallThrough(result.address, result.portIDs[0]);
    if (!_connected){
      cerr << getLabel() << " warning: failed to connect to brokered address '"
	   << result.address << "' for service '"
	   << _serviceName << "' on network '" << _networks << "'.\n";
    }

    if (_connected && !_handshakeData){
      arTemplateDictionary* d = _dataClient.getDictionary();
      _responseData = new arStructuredData(d, "response");

      // bug: should test that d->find() != NULL in the lines below.
      _handshakeData = new arStructuredData(d, "handshake");
      BONDED_ID = d->find("handshake")->getAttributeID("bonded ID");

      _clientTuningData = new arStructuredData(d, "client tuning");
      CLIENT_TUNING_DATA = 
	d->find("client tuning")->getAttributeID("client tuning data");

      _serverTuningData = new arStructuredData(d, "server tuning");
      SERVER_TUNING_DATA = 
	d->find("server tuning")->getAttributeID("server tuning data");
    }
  }
  _connectionThreadRunning = false;
}

void ar_barrierClientData(void* barrierClient){
  ((arBarrierClient*)barrierClient)->_dataTask();
}

void arBarrierClient::_dataTask(){
  _dataThreadRunning = true;
  arSleepBackoff a(1, 10, 1.2);
  while (_keepRunningThread && !_exitProgram){
    if (!_connected){
      a.sleep();
      continue;
    }
    a.reset();

    if (!_dataClient.getData(_dataBuffer, _bufferSize)){
      _connected = false;
      _activated = false;
      // must send release (if someone is waiting at a sync barrier,
      // that barrier needs to be released
      _releaseSignal.sendSignal();
      continue;
    }
    if (ar_rawDataGetID(_dataBuffer) == _handshakeData->getID()){
      // this must be round 2 of the handshake
      ar_mutex_lock(&_activationLock);
      _activationResponse = true;
      _activationVar.signal();
      ar_mutex_unlock(&_activationLock);
    }
    else if (ar_rawDataGetID(_dataBuffer) == _serverTuningData->getID()){
      // the server has sent a release packet
      _serverTuningData->unpack(_dataBuffer);
      _serverSendSize = _serverTuningData->getDataInt(SERVER_TUNING_DATA);
      if (_serverSendSize < 0)
	_serverSendSize = 0; // ignore garbage data
      _releaseSignal.sendSignal();
    }
    else{
      cerr << getLabel() << " warning: got unknown packet.\n";
    }
  }
  _dataThreadRunning = false;
}

arBarrierClient::arBarrierClient(){
  // ;; all these should be initializers, not assignments...
  _serviceName = string("NULL");
  _networks = string("NULL");
  _client = NULL;

  // zero-out the tuning data storage
  _drawTime = 0;
  _rcvTime = 0;
  _procTime = 0;
  _frameNum = 0;
  _serverSendSize = 0;
  
  // set up the TCP sockets who monitor connectivity and
  // provide a way to transmit the 3-way handshake
  // in passive connection mode
  _connected = false;
  _bufferSize = 200;
  _dataBuffer = new ARchar[_bufferSize];
  ar_mutex_init(&_activationLock);
  _activationResponse = false;

  // connection mode stuff
  _activated = false;
  _handshakeData = NULL;
  _responseData = NULL;
  _clientTuningData = NULL;
  _serverTuningData = NULL;
  _exitProgram = false;
  _connectionThreadRunning = false;
  _dataThreadRunning = false;
  _finalSyncSent = false;
}

arBarrierClient::~arBarrierClient(){
  //********************************************************************
  // better release the sync call.... this is a bit of a kludge, I know
  //********************************************************************
  _releaseSignal.sendSignal();
  _keepRunningThread = false;
  delete [] _dataBuffer;
}

bool arBarrierClient::requestActivation(){
  // could use some internal error checking here...
  // what if the server goes away during the connection handshake?
  if (!_connected)
    return false;

  // communicate that we are ready to activate
  ar_mutex_lock(&_activationLock);
    _activationResponse = false;
  ar_mutex_unlock(&_activationLock);

  _handshakeData->dataIn(BONDED_ID,&_bondedSocketID,AR_INT,1);
  _sendLock.lock();
  bool ok = _dataClient.sendData(_handshakeData);
  _sendLock.unlock();
  if (!ok){
    cerr << getLabel() << " error: requestActivation failed to send data.\n";
    return false;
  }

  // wait on response (this is handled in the data receive thread)
  // NOTE: there is a very, very subtle deadlock possibility whereby we
  // might execute the stop() command *before* hitting here... thus
  // leaving the arSyncDataClient read thread unable to exit. (note how
  // _activationResponse is set to false above and see the race condition
  // w/ stop(). Consequently, we also need to test for _exitProgram below
  ar_mutex_lock(&_activationLock);
    while (!_activationResponse && !_exitProgram){
      _activationVar.wait(&_activationLock);
    }
  ar_mutex_unlock(&_activationLock);
  // if we pushed through the following wait because of stop()... DO NOT
  // send a response
  if (!_exitProgram){
    // send 3-way handshake completion
    _sendLock.lock();
    _dataClient.sendData(_responseData);
    _sendLock.unlock();
  }
  // even if we got here because of stop()... we must pretend we are
  // activated... otherwise, arSyncDataCLient's read thread might block
  // on checkActivation()
  _activated = true;
  // finally, it would be a good idea to *reset* the release signal.
  // while it is necessary to send a release signal on disconnect
  // (what if we are waiting in the sync method), we could also *not*
  // be in the sync call on disconnect, in which case there is a 
  // spurious release tagged on the next connect (unless we reset the
  // signal as here)
  _releaseSignal.reset();
  return true;
}

bool arBarrierClient::checkActivation(){
  return _activated;
}

bool arBarrierClient::setBondedSocketID(int theID){
  // needs error handling
  _bondedSocketID = theID;
  return true;
}

void arBarrierClient::setServiceName(const string& serviceName){
  _serviceName = serviceName;
}

// A slash-delimited string containing the networks, in order of descending preference,
// that the object will use to connect to a service
void arBarrierClient::setNetworks(const string& networks){
  _networks = networks;
}

// The arSZGClient object is needed later in the connection thread.
bool arBarrierClient::init(arSZGClient& client){
  _client = &client;
  return true;
}

bool arBarrierClient::start(){
  _keepRunningThread = true;
  _dataClient.setLabel("syzygy barrier_thread");
  _dataClient.smallPacketOptimize(true);
  return _connectionThread.beginThread(ar_barrierClientConnection, this) &&
         _dataThread.beginThread(ar_barrierClientData, this); 
}

void arBarrierClient::stop(){
  // make sure we are not blocking on any calls (like requestActivation(...))
  // this really is somewhat cheesy SO FAR
  ar_mutex_lock(&_activationLock);

  // Set _exitProgram both within the _activationLock and the _sendLock.
  // In the case of _sendLock,
  // after _exitProgram is set, there must be exactly one final sync send
  // so that readDataThread will not block.
  // _sendLock here avoids a race condition in sync().
  _sendLock.lock();
  _exitProgram = true; 
  _sendLock.unlock();

  _activationResponse = true;
  _activationVar.signal();
  ar_mutex_unlock(&_activationLock);
  // Ping the server to avoid getting stuck in arDataClient's readData call.
  const int tuningData[4] = { _drawTime, _rcvTime, _procTime, _frameNum };
  _sendLock.lock();
  // If we have never connected,
  // _clientTuningData is uninitialized and doesn't need to be sent anyway.
  if (_clientTuningData){
    _clientTuningData->dataIn(CLIENT_TUNING_DATA,tuningData,AR_INT,4);
    // Send only one final sync packet.
    if (!_finalSyncSent){
      _dataClient.sendData(_clientTuningData);
      _finalSyncSent = true;
    }
  }
  else{
    _finalSyncSent = true;
  }
  _sendLock.unlock();
  // Paranoid: make sure we are not blocking in the sync
  // call (it's likely though that sending to the server caused
  // this signal to be called anyway!)
  _releaseSignal.sendSignal();

  // Wait for the threads to finish
  arSleepBackoff a(8, 20, 1.08);
  while (_dataThreadRunning || _connectionThreadRunning){
    a.sleep();
  }
}

void arBarrierClient::setTuningData(int drawTime, int rcvTime,
				    int procTime, int frameNum){
  _drawTime = drawTime;
  _rcvTime = rcvTime;
  _procTime = procTime;
  _frameNum = frameNum;
}

bool arBarrierClient::sync(){
  // if the object is being told to stop, this call should become NULL

  if (_exitProgram || !_connected || !_activated){
    // does it really make sense to return true here????
    return true;
  }

  // there are definitely race conditions here...
  // Pack the buffer with the tuning data.
  _sendLock.lock();
  const int tuningData[4] = { _drawTime, _rcvTime, _procTime, _frameNum };
  _clientTuningData->dataIn(CLIENT_TUNING_DATA,tuningData,AR_INT,4);
  bool ok = false;
  if (!_finalSyncSent){
    ok = _dataClient.sendData(_clientTuningData);
    // NOTE: _exitProgram could have, very easily, been set sometime
    // between the check at the beginning of the function
    // and here. The key thing is: after _exitProgram has been set to true
    // exactly one send (here and in stop) can occur. Otherwise,
    // multiple release signals to the server will confuse it (in the
    // case of clients going away and reconnecting)
    if (_exitProgram){
      _finalSyncSent = true;
    }
  }
  _sendLock.unlock();
  if (!ok){
    cerr << getLabel() << " error: sync failed.\n";
    return false;
  }
  _releaseSignal.receiveSignal();
  return true;
}

const string& arBarrierClient::getLabel() const {
  static const string noname("arBarrierClient");
  return _client ? _client->getLabel() : noname;
}
