//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSyncDataClient.h"
#include "arDataUtilities.h"
#include "arLogStream.h"

#include <math.h>

void ar_syncDataClientConnectionTask(void* client){
  ((arSyncDataClient*)client)->_connectionTask();
}

void arSyncDataClient::_connectionTask(){
  // DOH! Will start slowly working towards deterministic shutdown.
  _connectionThreadRunning = true;
  while (!_exitProgram){
    // make sure barrier connects first
    while (!_barrierClient.checkConnection() && !_exitProgram)
      ar_usleep(10000);
    if (_exitProgram)
      break;

    // connection brokering

    // the following is THE ONLY blocking call in this thread. 
    // It would be a good idea to figure out a way to get the szgserver to shut this
    // down. Maybe the client could send an "I'm planning on shutting down message?"
    // And the server could terminate all blocking calls???
    // In any case, the quick and dirty HACK is to claim the connection thread
    // is NOT running while this is going, to allow a kill while it is blocking.
    _connectionThreadRunning = false;
    arPhleetAddress result = _client->discoverService(_serviceName, _networks, true);
    // we will not allow this thread to be arbitrarily killed now
    _connectionThreadRunning = true;
    // the race condition of: stop() was called between discoverServuce and
    // _connectionThreadRunning = true; is handled by checking _exitProgram here.
    if (_exitProgram)
      break;

    // we can now go on about our business
    if (!result.valid){
      ar_log_error() << getLabel() << " error: no valid address on server discovery.\n";
      continue;
    }
    if (!_dataClient.dialUpFallThrough(result.address, result.portIDs[0])){
      ar_log_error() << getLabel() << " error: failed to connect to brokered "
	             << result.address << ":" << result.portIDs[0] << ".\n";
      continue;
    }
    if (_connectionCallback){
      _connectionCallback(_bondedObject, _dataClient.getDictionary());
    }
    else {
      ar_log_error() << getLabel() << " error: undefined connection callback for "
	             << result.address << ":" << result.portIDs[0] << ".\n";
      _connectionThreadRunning = false;
      return;
    }
    // Bond the data channel to this sync channel.
    _barrierClient.setBondedSocketID(_dataClient.getSocketIDRemote());
    _stateClientConnected = true;
    ar_log_remark() << getLabel() << " remark: connected to "
	            << result.address << ":" << result.portIDs[0] << ".\n";

    // ugly polling! make sure we cannot block here if stop() was called.
    while (_stateClientConnected && !_exitProgram)
      ar_usleep(30000);
    if (_exitProgram)
      break;

    // ar_syncClientDataReadTask() sets _stateClientConnected to false.
    _dataClient.closeConnection();
    // pop out of the beginning of the consumption loop
    skipConsumption();
    // the disconnection function is called after the NULL callback!

    // bug: some copypaste with below
    // Guarantee that one _nullCallback is called
    // before a new connection is made, especially for
    // wildcat graphics cards.
    ar_mutex_lock(&_nullHandshakeLock);
    // We might be in stop mode.
    if (_nullHandshakeState != 2){
      _nullHandshakeState = 1; // Request that a disconnect callback be issued
      while (_nullHandshakeState != 2){
        _nullHandshakeVar.wait(&_nullHandshakeLock);
      }
      _nullHandshakeState = 0;
    }
    ar_mutex_unlock(&_nullHandshakeLock);

    ar_log_remark() << getLabel() << " remark: disconnected.\n";
  }
  _connectionThreadRunning = false;
}

void ar_syncDataClientReadTask(void* client){
  ((arSyncDataClient*)client)->_readTask();
}

void arSyncDataClient::_readTask(){
  _readThreadRunning = true;
  while (!_exitProgram){
    if (!_stateClientConnected){
      ar_usleep(30000);
      // Fall down to the test of _exitProgram:
      // an exit request might be issued while we wait to connect.
      continue;
    }
    // Activate our connection to the barrier server.
    if (!_barrierClient.checkActivation())
      _barrierClient.requestActivation();
    // read data into back buffer 
    ar_timeval time1 = ar_time();
    ar_mutex_lock(&_stackLock);
    if (_storageStack.empty()){
      // need some more storage for the next network buffer!
      const int tempSize = 10000;
      char* temp = new char[tempSize];
      _storageStack.push_back(pair<char*,int>(temp,tempSize));
    }
    pair<char*,int> dataStorage = _storageStack.front();
    _storageStack.pop_front();
    ar_mutex_unlock(&_stackLock);
    //bool ok = _dataClient.getDataQueue(_data[_backBuffer],
    //					 _dataSize[_backBuffer]);
    const bool ok = _dataClient.getDataQueue(dataStorage.first, dataStorage.second);
    const float temp = ar_difftime(ar_time(), time1);
    _oldRecvTime = temp>0. ? temp : _oldRecvTime;
    if (ok && _firstConsumption){
      // request another buffer right away so our double-buffering will work
      // of course, only do this if we are in synchronized read mode
      if (syncClient() && !_barrierClient.sync()){
	ar_log_error() << getLabel() << " error: sync failed.\n";
      }
      _firstConsumption = false;
    }
    if (ok) {
      // we got some data! before swapping buffers, wait for
      // the consumer to finish (and do a little performance analysis).
      ARint bufferSize = -1;
      ar_unpackData(_data[_backBuffer],&bufferSize,AR_INT,1);
      _oldRecvSize = bufferSize>0 ? float(bufferSize) : 0.; // ignore garbage

      // Wait to swap buffers. Only do this if we are in synchronized read mode
      if (syncClient()) {
        ar_mutex_lock(&_swapLock);
        while (!_bufferSwapReady){
	  _bufferSwapCondVar.wait(&_swapLock);
        }
        // Buffer swap can occur now.
        _bufferSwapReady = false;
        // put the stored data on the receive stack
        ar_mutex_lock(&_stackLock);
        _receiveStack.push_back(dataStorage);
        ar_mutex_unlock(&_stackLock);
        //_backBuffer = 1 - _backBuffer;
        _dataAvailable = 1;
        _dataWaitCondVar.signal();
        ar_mutex_unlock(&_swapLock);
      }
      else{
	// still need to put data onto the receiveStack, even in nosync mode
        // put the stored data on the receive stack
        ar_mutex_lock(&_stackLock);
        _receiveStack.push_back(dataStorage);
        ar_mutex_unlock(&_stackLock);
        //_backBuffer = 1 - _backBuffer;
      }
    }
    else {
      // The connection must have closed.
      
      // For double-buffering, we need to be idle on the first
      // consumption of a connection.
      _firstConsumption = true;
      // Reset the rest of the state, to avoid deadlocks.
      _bufferSwapReady = true;
      // recycle any pending buffers on the receive stack
      ar_mutex_lock(&_stackLock);
      for (list<pair<char*,int> >::iterator iter = _receiveStack.begin();
           iter != _receiveStack.end();
	   iter++) {
        _storageStack.push_back(*iter);
      }
      _receiveStack.clear();
      ar_mutex_unlock(&_stackLock);
      //_backBuffer = 0;
      _dataAvailable = 0;
      // signal that the connection has been closed *last*
      _stateClientConnected = false;
    }
  }
  _readThreadRunning = false;
}

arSyncDataClient::arSyncDataClient():
  _client(NULL),
  _serviceName("NULL"),
  _serviceNameBarrier("NULL"),
  _networks("NULL"),
  _syncServer(NULL){

  /// \todo turn these assignments into initializers.
  _mode = AR_SYNC_CLIENT;
  _dataAvailable = 0;
  _bufferSwapReady = true;
  _data[0] = new ARchar[10000];
  _dataSize[0] = 10000;
  _data[1] = new ARchar[10000];
  _dataSize[1] = 10000;
  _backBuffer = 0;

  ar_mutex_init(&_stackLock);

  _stateClientConnected = false;
  _exitProgram = false;
  _readThreadRunning = false;
  _connectionThreadRunning = false;
  
  _bondedObject = NULL;

  _connectionCallback = NULL;
  _disconnectCallback = NULL;
  _consumptionCallback = NULL;
  _actionCallback = NULL;
  _postSyncCallback = NULL;

  ar_mutex_init(&_swapLock);

  _firstConsumption = true;

  // performance analysis data
  _frameTime = 100000;
  _drawTime = 50000;
  _procTime = 50000;
  _recvTime = 100000;
  _oldRecvTime = 100000;
  _oldRecvSize = 10000;
  _recvSize = 10000;
  _serverSendSize = 10000;

  ar_mutex_init(&_nullHandshakeLock);
  _nullHandshakeState = 0;

}

arSyncDataClient::~arSyncDataClient(){
  // lame, ain't doing anything yet!

}

void arSyncDataClient::registerLocalConnection(arSyncDataServer* server){
  _syncServer = server;
  _syncServer->_locallyConnected = true;
}

void arSyncDataClient::setBondedObject(void* bondedObject){
  _bondedObject = bondedObject;
}

bool arSyncDataClient::setMode(int mode){
  if (mode != AR_SYNC_CLIENT && mode != AR_NOSYNC_CLIENT){
    ar_log_error() << getLabel() << " error: ignoring unrecognized mode "
                   << mode << ".\n";
    return false;
  } 
  _mode = mode;
  return true;
}

void arSyncDataClient::setConnectionCallback
(bool (*connectionCallback)(void*, arTemplateDictionary*)){
  _connectionCallback = connectionCallback;
}

void arSyncDataClient::setDisconnectCallback
(bool (*disconnectCallback)(void*)){
  _disconnectCallback = disconnectCallback;
}

void arSyncDataClient::setConsumptionCallback
(bool (*consumptionCallback)(void*, ARchar*)){
  _consumptionCallback = consumptionCallback;
}

void arSyncDataClient::setActionCallback
(bool (*actionCallback)(void*)){
  _actionCallback = actionCallback;
}

void arSyncDataClient::setNullCallback
(bool (*nullCallback)(void*)){
  _nullCallback = nullCallback;
}

void arSyncDataClient::setPostSyncCallback
(bool (*postSyncCallback)(void*)){
  _postSyncCallback = postSyncCallback;
}

void arSyncDataClient::setServiceName(string serviceName){
  _serviceName = serviceName;
}

void arSyncDataClient::setNetworks(string networks){
  _networks = networks;
}

bool arSyncDataClient::init(arSZGClient& client){
  if (_syncServer){
    ar_log_error() << "arSyncDataClient error: can't init() when locally connected.\n";
    return false;
  }

  _client = &client;
  _serviceNameBarrier =
    client.createComplexServiceName(_serviceName+"_BARRIER");
  _serviceName = client.createComplexServiceName(_serviceName);
  ar_log_remark() << getLabel() << " remark: initialized with service name "
                  << _serviceName << ".\n";
  return true;
}

bool arSyncDataClient::start(){
  if (_syncServer){
    ar_log_error() << getLabel() << " can't start() when locally connected.\n";
    return false;
  }

  if (!_client){
    ar_log_error() << getLabel() << " can't start() before init().\n";
    return false;
  }

  // connection brokering goes here
  _barrierClient.setServiceName(_serviceNameBarrier);
  _barrierClient.setNetworks(_networks);
  if (!_barrierClient.init(*_client) || !_barrierClient.start()){
    ar_log_error() << getLabel() << " failed to start barrier client.\n";
    return false;
  }
  
  // do some configuration of the data client
  _dataClient.smallPacketOptimize(true);

  if (!_connectionThread.beginThread(ar_syncDataClientConnectionTask, this)) {
    ar_log_error() << getLabel() << " failed to start connection thread.\n";
    return false;
  }

  if (!_readThread.beginThread(ar_syncDataClientReadTask, this)) {
    ar_log_error() << getLabel() << " failed to start read thread.\n";
    return false;
  }

  ar_log_remark() << getLabel() << " started.\n";
  return true;
}

void arSyncDataClient::stop(){
  if (_syncServer){
    ar_log_error() << getLabel() << " can't stop() when locally connected.\n";
    return;
  }

  // set _exitProgram to true *before* starting the read thread
  _exitProgram = true;
  // stop the barrier client. so far, this consists of making sure
  // we are not blocking in requestActivation()
  _barrierClient.stop();
  // make sure the read thread is not hung waiting to swap buffers
  ar_mutex_lock(&_swapLock);
  _bufferSwapReady = true;
  _bufferSwapCondVar.signal();
  ar_mutex_unlock(&_swapLock);
  // make sure that we are not blocked at the end of the connection thread
  ar_mutex_lock(&_nullHandshakeLock);
  _nullHandshakeState = 2;
  _nullHandshakeVar.signal();
  ar_mutex_unlock(&_nullHandshakeLock);
  // wait for the read thread and the connection thread to finish
  while (_readThreadRunning || _connectionThreadRunning){
    ar_usleep(30000);
  }
}

void arSyncDataClient::consume(){
  // A TERRIBLE LITTLE HACK! We have two cases. The first is when this
  // object is connected locally and directly to an arSyncDataServer.
  // The second is when we are trying to connect over the network.

  // This is the local connection case.
  if (_syncServer){
    // The arSyncDataServer must be informed that we are ready to get some
    // data
    ar_mutex_lock(&_syncServer->_localConsumerReadyLock);
    // It could be the case that everything is stopping...
    if (_syncServer->_localConsumerReady == 2){
      ar_mutex_unlock(&_syncServer->_localConsumerReadyLock);
      return;
    }
    _syncServer->_localConsumerReady = 1;
    _syncServer->_localConsumerReadyVar.signal();
    ar_mutex_unlock(&_syncServer->_localConsumerReadyLock);
    // We must wait for the data to become ready.
    ar_mutex_lock(&_syncServer->_localProducerReadyLock);
    while (!_syncServer->_localProducerReady){
      _syncServer->_localProducerReadyVar.wait
        (&_syncServer->_localProducerReadyLock);
    } 
    // Could be the case that everything is stopping.
    if (_syncServer->_localProducerReady == 2){
      ar_mutex_unlock(&_syncServer->_localProducerReadyLock);
      return;
    }
    _syncServer->_localProducerReady = 0;
    ar_mutex_unlock(&_syncServer->_localProducerReadyLock);    

    _consumptionCallback(_bondedObject,
                         _syncServer->_dataQueue->getFrontBufferRaw());
    _actionCallback(_bondedObject);
    _postSyncCallback(_bondedObject);
    return;
  }

  // This is the MUCH more complicated network connection case.

  // performance measurement
  ar_timeval time1, time2, time3, time4;

  time1 = ar_time();
  if (!_stateClientConnected){
    _nullCallback(_bondedObject);
    // bug: some copypaste with above
    // Guarantee that one _nullCallback is called
    // before a new connection is made, especially for
    // wildcat graphics cards.
    ar_mutex_lock(&_nullHandshakeLock);
    if (_nullHandshakeState == 1){
      // The connection thread registers disconnected and is waiting for
      // us to say we've cleared, so it can then accept a new connection.
      // We call the disconnect callback HERE, because
      // we are guaranteed to hit here on disconnect (unless we are
      // stopping anyway). AND this is in the same thread as the consumption
      // which lets us clear the database on the disconnect callback.
      if (_disconnectCallback) {
        _disconnectCallback(_bondedObject);
      }
      else {
        ar_log_error() << "arSyncDataClient warning: undefined disconnection "
	               << "callback.\n";
      }
      _nullHandshakeState = 2;
      _nullHandshakeVar.signal();
    }
    ar_mutex_unlock(&_nullHandshakeLock);
  }
  else{
    ar_mutex_lock(&_swapLock);
    // only wait if we are in synchronized read mode
    while (!_dataAvailable && _mode == AR_SYNC_CLIENT){
      _dataWaitCondVar.wait(&_swapLock);
    }
    if (_dataAvailable != 2){
      _dataAvailable = 0;
      ar_mutex_unlock(&_swapLock);
      time2 = ar_time();
      // Copy the current receive stack into the consume stack
      // (if we are synchronized, this will be exactly 1 buffer).
      // Then consume everything.
      _consumeStack.clear();
      ar_mutex_lock(&_stackLock);
      list<pair<char*,int> >::iterator iter;
      if (_mode == AR_NOSYNC_CLIENT){
	// consume everything
        for (iter = _receiveStack.begin(); iter != _receiveStack.end();
	     iter++){
          _consumeStack.push_back(*iter);
        }
        _receiveStack.clear();
      }
      else{
	// Consume exactly one thing, as we are in synchronized
	// read mode, AR_SYNC_CLIENT.
	// PLEASE NOTE: If the arSyncDataServer (that is providing us data)
	// was killed by SIGNINT instead of dkill then we might get here
	// during connection shutdown with nothing in the receive stack!
	// Consequently, test for this.
        if (!_receiveStack.empty()){
          _consumeStack.push_back(_receiveStack.front());
	  _receiveStack.pop_front();
	}
      }
      ar_mutex_unlock(&_stackLock);
      for (iter = _consumeStack.begin(); iter != _consumeStack.end(); iter++){
        _consumptionCallback(_bondedObject, (*iter).first);
      }
      // move storage from consume stack to storage stack
      ar_mutex_lock(&_stackLock);
      for (iter = _consumeStack.begin(); iter != _consumeStack.end(); iter++){
        _storageStack.push_back(*iter);
      }
      _consumeStack.clear();
      ar_mutex_unlock(&_stackLock);
      //_consumptionCallback(_bondedObject, _data[1 - _backBuffer]);
      time3 = ar_time();
      _actionCallback(_bondedObject);
      time4 = ar_time();
      // eliminate sync if we are not in the right mode
      if (_stateClientConnected && _mode == AR_SYNC_CLIENT){
        if (!_barrierClient.sync())
	  ar_log_error() << getLabel() << " error: sync failed.\n";
      }
      _postSyncCallback(_bondedObject);
      // Signal that we are ready to swap buffers.
      ar_mutex_lock(&_swapLock); 
	_bufferSwapReady = true;
	_bufferSwapCondVar.signal();
      ar_mutex_unlock(&_swapLock);
    }
    else{
      _dataAvailable = 0;
      ar_mutex_unlock(&_swapLock);
      _nullCallback(_bondedObject);
    }
  }

  float temp = ar_difftime(ar_time(), time1);
  temp = temp>0 ? temp : _frameTime;
  float drawTime = ar_difftime(time4, time3);
  drawTime = drawTime>0 ? drawTime : _drawTime;
  float procTime = ar_difftime(time3, time2);
  procTime = procTime>0 ? procTime : _procTime;
  const float usecFilter =
    temp>100000 ? .5 : temp>50000 ? .08 : temp>25000 ? .04 : .02;
  _update(_frameTime, temp, usecFilter);
  _update(_serverSendSize, _barrierClient.getServerSendSize(), usecFilter);
  _update(_recvTime, _oldRecvTime, usecFilter);
  _update(_recvSize, _oldRecvSize, usecFilter);
  _update(_drawTime, drawTime, usecFilter);
  _update(_procTime, procTime, usecFilter);
  _barrierClient.setTuningData(int(_drawTime),int(_recvTime),int(_procTime),0);
}

inline void arSyncDataClient::_update(float& value, float newValue, float filter){
  if (newValue == 0.) {
    value = 0.;
    return;
  }
  const float ratio = fabs(value / newValue);
  if (ratio > 10.0 || ratio < .1) {
    value = newValue;
    return;
  }
  value = filter * newValue  +  (1.0 - filter) * value;
}

/// Allows us, on shutdown, to make sure that consume() exits, where otherwise
/// it might be blocked waiting for the next piece of data from the read
/// data thread
void arSyncDataClient::skipConsumption(){
  ar_mutex_lock(&_swapLock);
  _dataAvailable = 2;
  _dataWaitCondVar.signal();
  ar_mutex_unlock(&_swapLock);
}

int arSyncDataClient::getServerSendSize() const {
  return int(_serverSendSize);
}

int arSyncDataClient::getFrameTime() const {
  return int(_frameTime);
}

int arSyncDataClient::getProcTime() const {
  return int(_procTime);
}

int arSyncDataClient::getActionTime() const {
  return int(_drawTime);
}

int arSyncDataClient::getRecvTime() const {
  return int(_recvTime);
}

int arSyncDataClient::getRecvSize() const {
  return int(_recvSize);
}

const string& arSyncDataClient::getLabel() const {
    static const string noname("arSyncDataClient");
    static const string empty("");
    const string& s = _client ? _client->getLabel() : empty;
    return s=="" ? noname : s;
}
