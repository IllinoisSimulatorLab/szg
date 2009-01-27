//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSyncDataClient.h"
#include "arDataUtilities.h"
#include "arLogStream.h"

#include <math.h>

void ar_syncDataClientConnectionTask(void* client) {
  ((arSyncDataClient*)client)->_connectionTask();
}

void arSyncDataClient::_connectionTask() {
  // todo: deterministic shutdown.
  _connectionThreadRunning = true;
  while (!_exitProgram) {
    // Barrier must connect first.
    arSleepBackoff a(7, 50, 1.2);
    while (!_barrierClient.checkConnection() && !_exitProgram)
      a.sleep();
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

    const string IPport =
      result.address + ":" + ar_intToString(result.portIDs[0]) + ".\n";
    if (!result.valid) {
      ar_log_error() << "no valid address on server discovery.\n";
      continue;
    }
    if (!_dataClient.dialUpFallThrough(result.address, result.portIDs[0])) {
      ar_log_error() << "failed to connect to brokered " << IPport;
      continue;
    }
    if (!_connectionCallback) {
      ar_log_error() << "no connection callback for " << IPport;
      _connectionThreadRunning = false;
      return;
    }

    _connectionCallback(_bondedObject, _dataClient.getDictionary());
    // Bond the data channel to this sync channel.
    _barrierClient.setBondedSocketID(_dataClient.getSocketIDRemote());
    _stateClientConnected = true;
    ar_log_remark() << "connected to " << IPport;

    // ugly polling! make sure we cannot block here if stop() was called.
    while (_stateClientConnected && !_exitProgram)
      ar_usleep(20000);
    if (_exitProgram)
      break;

    // ar_syncClientDataReadTask() sets _stateClientConnected to false.
    _dataClient.closeConnection();
    // pop out of the beginning of the consumption loop
    skipConsumption();
    // the disconnection function is called after the NULL callback!

    // bug: some copypaste with below
    // Guarantee that one _nullCallback is called
    // before a new connection is made, especially for wildcat graphics cards.
    _nullHandshakeLock.lock("arSyncDataClient::_connectionTask");
    // We might be in stop mode.
    if (_nullHandshakeState != 2) {
      _nullHandshakeState = 1; // Request that a disconnect callback be issued
      while (_nullHandshakeState != 2) {
        _nullHandshakeVar.wait(_nullHandshakeLock);
      }
      _nullHandshakeState = 0;
      _nullHandshakeLock.unlock();
    }
    ar_log_remark() << "disconnected.\n";
  }
  _connectionThreadRunning = false;
}

void ar_syncDataClientReadTask(void* client) {
  ((arSyncDataClient*)client)->_readTask();
}

// Avoid race condition: _stackLock happens inside _swapLock.
// So _swapLock should never happen inside _stackLock.  Indeed it is not.  Phew.

void arSyncDataClient::_readTask() {
  _readThreadRunning = true;
  arSleepBackoff a(25, 40, 1.2);
  while (!_exitProgram) {
    if (!_stateClientConnected) {
      a.sleep();
      continue;
    }

    // Activate our connection to the barrier server.
    if (!_barrierClient.checkActivation())
      _barrierClient.requestActivation();
    // read data into back buffer
    ar_timeval time1 = ar_time();

    _stackLock.lock("arSyncDataClient::_readTask stack A");
    if (_storageStack.empty()) {
      // storage for the next network buffer
      const int tempSize = 10000;
      char* temp = new char[tempSize];
      _storageStack.push_back(pair<char*, int>(temp, tempSize));
    }
    pair<char*, int> dataStorage = _storageStack.front();
    _storageStack.pop_front();
    _stackLock.unlock();

    const bool ok = _dataClient.getDataQueue(dataStorage.first, dataStorage.second);
    const float temp = ar_difftime(ar_time(), time1);
    _oldRecvTime = temp>0. ? temp : _oldRecvTime;
    if (ok && _firstConsumption) {
      // if in sync read mode, request another buffer right away for double-buffering
      if (syncClient() && !_barrierClient.sync()) {
        ar_log_error() << "_readTask()'s barrier client failed to sync.\n";
      }
      _firstConsumption = false;
    }
    if (ok) {
      // we got some data! before swapping buffers, wait for
      // the consumer to finish (and do a little performance analysis).
      ARint bufferSize = -1;
      ar_unpackData(_data[_backBuffer], &bufferSize, AR_INT, 1);
      _oldRecvSize = bufferSize>0 ? float(bufferSize) : 0.; // ignore garbage

      // Put the stored data on the receive stack, sync or nosync.
      if (syncClient()) {
        _swapLock.lock("arSyncDataClient::_readTask sync");
        while (!_bufferSwapReady) {
          _bufferSwapCondVar.wait(_swapLock);
        }
        // Buffer swap can occur now.
        _bufferSwapReady = false;

        _stackLock.lock("arSyncDataClient::_readTask stack B");
          _receiveStack.push_back(dataStorage);
        _stackLock.unlock();
        //_backBuffer = 1 - _backBuffer;

        _dataAvailable = 1;
        _dataWaitCondVar.signal();
        _swapLock.unlock();
      }
      else{
        arGuard _(_stackLock, "arSyncDataClient::_readTask nosync");
        _receiveStack.push_back(dataStorage);
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
      _stackLock.lock("arSyncDataClient::_readTask stack C");
      for (list<pair<char*, int> >::iterator iter = _receiveStack.begin();
           iter != _receiveStack.end(); iter++) {
        _storageStack.push_back(*iter);
      }
      _receiveStack.clear();
      _stackLock.unlock();
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
  _stackLock("SYNCCLIENT_STACK"),
  _swapLock("SYNCCLIENT_SWAP"),
  _bufferSwapCondVar("arSyncDataClient-bufferswap"),
  _dataWaitCondVar("arSyncDataClient-datawait"),
  _nullHandshakeLock("SYNCCLIENT_SHAKE"),
  _nullHandshakeVar("arSyncDataClient-nullhandshake"),
  _syncServer(NULL) {

  // todo: turn these assignments into initializers.
  _mode = AR_SYNC_CLIENT;
  _dataAvailable = 0;
  _bufferSwapReady = true;
  _dataSize[0] = 10000;
  _dataSize[1] = 10000;
  _data[0] = new ARchar[10000];
  _data[1] = new ARchar[10000];
  _backBuffer = 0;

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

  _nullHandshakeState = 0;

}

arSyncDataClient::~arSyncDataClient() {
  // lame, ain't doing anything yet!

}

void arSyncDataClient::registerLocalConnection(arSyncDataServer* server) {
  _syncServer = server;
  _syncServer->_locallyConnected = true;
}

void arSyncDataClient::setBondedObject(void* bondedObject) {
  _bondedObject = bondedObject;
}

bool arSyncDataClient::setMode(int mode) {
  if (mode != AR_SYNC_CLIENT && mode != AR_NOSYNC_CLIENT) {
    ar_log_error() << "ignoring unrecognized mode " << mode << ".\n";
    return false;
  }
  _mode = mode;
  return true;
}

void arSyncDataClient::setConnectionCallback
(bool (*connectionCallback)(void*, arTemplateDictionary*)) {
  _connectionCallback = connectionCallback;
}

void arSyncDataClient::setDisconnectCallback
(bool (*disconnectCallback)(void*)) {
  _disconnectCallback = disconnectCallback;
}

void arSyncDataClient::setConsumptionCallback
(bool (*consumptionCallback)(void*, ARchar*)) {
  _consumptionCallback = consumptionCallback;
}

void arSyncDataClient::setActionCallback
(bool (*actionCallback)(void*)) {
  _actionCallback = actionCallback;
}

void arSyncDataClient::setNullCallback
(bool (*nullCallback)(void*)) {
  _nullCallback = nullCallback;
}

void arSyncDataClient::setPostSyncCallback
(bool (*postSyncCallback)(void*)) {
  _postSyncCallback = postSyncCallback;
}

void arSyncDataClient::setServiceName(string serviceName) {
  _serviceName = serviceName;
}

void arSyncDataClient::setNetworks(string networks) {
  _networks = networks;
}

bool arSyncDataClient::init(arSZGClient& client) {
  if (_syncServer) {
    ar_log_error() << "ignoring init() while locally connected.\n";
    return false;
  }

  _client = &client;
  _serviceNameBarrier = client.createComplexServiceName(_serviceName+"_BARRIER");
  _serviceName = client.createComplexServiceName(_serviceName);
  ar_log_debug() << "inited service " << _serviceName << ".\n";
  return true;
}

bool arSyncDataClient::start() {
  if (_syncServer) {
    ar_log_error() << "ignoring start() while locally connected.\n";
    return false;
  }

  if (!_client) {
    ar_log_error() << "ignoring start() before init().\n";
    return false;
  }

  // connection brokering goes here
  _barrierClient.setServiceName(_serviceNameBarrier);
  _barrierClient.setNetworks(_networks);
  if (!_barrierClient.init(*_client) || !_barrierClient.start()) {
    ar_log_error() << "failed to start barrier client.\n";
    return false;
  }

  // do some configuration of the data client
  _dataClient.smallPacketOptimize(true);

  if (!_connectionThread.beginThread(ar_syncDataClientConnectionTask, this)) {
    ar_log_error() << "failed to start connection thread.\n";
    return false;
  }

  if (!_readThread.beginThread(ar_syncDataClientReadTask, this)) {
    ar_log_error() << "failed to start read thread.\n";
    return false;
  }

  ar_log_remark() << "started service " << _serviceName << ".\n";
  return true;
}

void arSyncDataClient::stop() {
  if (_syncServer) {
    ar_log_error() << "ignoring stop() while locally connected.\n";
    return;
  }

  // set _exitProgram to true *before* starting the read thread
  _exitProgram = true;

  // Stop the barrier client: don't block in requestActivation().
  _barrierClient.stop();
  // make sure the read thread is not hung waiting to swap buffers
  _swapLock.lock("arSyncDataClient::stop swap");
  _bufferSwapReady = true;
  _bufferSwapCondVar.signal();
  _swapLock.unlock();

  // Ensure we're not blocked at the end of the connection thread
  _nullHandshakeLock.lock("arSyncDataClient::stop handshake");
    _nullHandshakeState = 2;
    _nullHandshakeVar.signal();
  _nullHandshakeLock.unlock();

  arSleepBackoff a(5, 30, 1.1);
  while (_readThreadRunning || _connectionThreadRunning) {
    a.sleep();
  }
}

void arSyncDataClient::consume() {
  if (_syncServer) {
    // Locally connected.
    // Tell arSyncDataServer that we're ready for data.

    _syncServer->_localConsumerReadyLock.lock("arSyncDataClient::consume A");
      if (_syncServer->_localConsumerReady == 2) {
        // Everything may be stopping.
        _syncServer->_localConsumerReadyLock.unlock();
        return;
      }
      _syncServer->_localConsumerReady = 1;
      _syncServer->_localConsumerReadyVar.signal();
    _syncServer->_localConsumerReadyLock.unlock();

    // Wait for the data to become ready.
    _syncServer->_localProducerReadyLock.lock("arSyncDataClient::consume B");
      while (!_syncServer->_localProducerReady) {
        _syncServer->_localProducerReadyVar.wait(_syncServer->_localProducerReadyLock);
      }
      if (_syncServer->_localProducerReady == 2) {
        // Everything may be stopping.
        _syncServer->_localProducerReadyLock.unlock();
        return;
      }
      _syncServer->_localProducerReady = 0;
    _syncServer->_localProducerReadyLock.unlock();

    _consumptionCallback(_bondedObject, _syncServer->_dataQueue->getFrontBufferRaw());
    _actionCallback(_bondedObject);
    _postSyncCallback(_bondedObject);
    return;
  }

  // Remotely connected.

  // performance measurement
  const ar_timeval time1 = ar_time();
  ar_timeval time2, time3, time4;

  if (!_stateClientConnected) {
    _nullCallback(_bondedObject);
    // bug: some copypaste with above
    // Guarantee that one _nullCallback is called
    // before a new connection is made, especially for
    // wildcat graphics cards.
    arGuard _(_nullHandshakeLock, "arSyncDataClient::consume disconnected");
    if (_nullHandshakeState == 1) {
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
        ar_log_error() << "no disconnection callback.\n";
      }
      _nullHandshakeState = 2;
      _nullHandshakeVar.signal();
    }
  }
  else{
    if (_exitProgram)
      return;

    _swapLock.lock("arSyncDataClient::consume C");
    // only wait if we are in synchronized read mode
    while (!_dataAvailable && _mode == AR_SYNC_CLIENT) {
      _dataWaitCondVar.wait(_swapLock);
    }
    if (_dataAvailable != 2) {
      _dataAvailable = 0;
      _swapLock.unlock();
      if (_exitProgram)
        return;
      time2 = ar_time();
      // Copy the current receive stack into the consume stack
      // (if we are synchronized, this will be exactly 1 buffer).
      // Then consume everything.
      _consumeStack.clear();
      if (_exitProgram)
        return;
      _stackLock.lock("arSyncDataClient::consume D");
      list<pair<char*, int> >::iterator iter;
      if (_mode == AR_NOSYNC_CLIENT) {
        // consume everything
        for (iter = _receiveStack.begin(); iter != _receiveStack.end(); ++iter) {
          _consumeStack.push_back(*iter);
        }
      //      MINGW crasher!
//        copy(_receiveStack.begin(), _receiveStack.end(), _consumeStack.end());
        _receiveStack.clear();
      }
      else{
        // Consume exactly one thing, since in synchronized read mode, AR_SYNC_CLIENT.
        // But if the arSyncDataServer feeding us was killed by SIGINT
        // instead of dkill, the connection would be shutting down,
        // causing _receiveStack.empty().
        if (!_exitProgram && !_receiveStack.empty()) {
          _consumeStack.push_back(_receiveStack.front());
          _receiveStack.pop_front();
        }
      }

      _stackLock.unlock();
      if (_exitProgram)
        return;

//      list<pair<char*, int> >::const_iterator iter;
      for (iter = _consumeStack.begin(); iter != _consumeStack.end() && !_exitProgram; ++iter) {
        _consumptionCallback(_bondedObject, iter->first);
      }
      if (_exitProgram)
        return;

      // move storage from consume stack to storage stack
      _stackLock.lock("arSyncDataClient::consume E");
      for (iter = _consumeStack.begin(); iter != _consumeStack.end(); ++iter) {
        _storageStack.push_back(*iter);
      }
      //      MINGW crasher!
//      copy(_consumeStack.begin(), _consumeStack.end(), _storageStack.end());
      _consumeStack.clear();
      _stackLock.unlock();
      if (_exitProgram)
        return;

      //_consumptionCallback(_bondedObject, _data[1 - _backBuffer]);
      time3 = ar_time();
      _actionCallback(_bondedObject);
      time4 = ar_time();
      if (_exitProgram)
        return;

      // eliminate sync if we are not in the right mode
      if (_stateClientConnected && _mode == AR_SYNC_CLIENT) {
        if (!_barrierClient.sync()) {
          ar_log_error() << "consume()'s barrier client failed to sync.\n";
        }
      }
      if (_exitProgram)
        return;
      _postSyncCallback(_bondedObject);
      // Signal that we are ready to swap buffers.
      if (_exitProgram)
        return;

      arGuard _(_swapLock, "arSyncDataClient::consume connected");
      _bufferSwapReady = true;
      _bufferSwapCondVar.signal();
    }
    else{
      _dataAvailable = 0;
      _swapLock.unlock();
      if (_exitProgram)
        return;
      _nullCallback(_bondedObject);
    }
  }
  if (_exitProgram)
    return;

  const float drawTime = ar_difftime(time4, time3);
  const float procTime = ar_difftime(time3, time2);
  const float t = ar_difftime(ar_time(), time1);
  const float usecFilter = t>100000 ? .5 : t>50000 ? .08 : t>25000 ? .04 : .02;
  _update(_frameTime, t, usecFilter);
  _update(_serverSendSize, _barrierClient.getServerSendSize(), usecFilter);
  _update(_recvTime, _oldRecvTime, usecFilter);
  _update(_recvSize, _oldRecvSize, usecFilter);
  _update(_drawTime, drawTime, usecFilter);
  _update(_procTime, procTime, usecFilter);
  _barrierClient.setTuningData(int(_drawTime), int(_recvTime), int(_procTime), 0);
}

inline void arSyncDataClient::_update(float& value, float newValue, float filter) {
  if (newValue <= 0.)
    newValue = value;
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

// On shutdown, make sure that consume() exits, where otherwise
// it might be blocked waiting for data from the read data thread
void arSyncDataClient::skipConsumption() {
  arGuard _(_swapLock, "arSyncDataClient::skipConsumption");
  _dataAvailable = 2;
  _dataWaitCondVar.signal();
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
