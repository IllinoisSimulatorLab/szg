//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSyncDataServer.h"
#include "arLogStream.h"

void ar_syncDataServerConnectionTask(void* pv) {
  arSyncDataServer* server = (arSyncDataServer*)pv;
  while (!server->_exitProgram) {
    if (!server->_dataServer.acceptConnectionNoSend()) {
      ar_usleep(1000); // CPU throttle, if errors occur.
    }
  }
}

void ar_syncDataServerSendTask(void* pv) {
  ((arSyncDataServer*)pv)->_sendTask();
}

void arSyncDataServer::_sendTask() {
  _sendThreadRunning = true;
  if (_locallyConnected)
    _sendTaskLocal();
  else
    _sendTaskRemote();
  _sendThreadRunning = false;
}

void arSyncDataServer::_sendTaskLocal() {
  // This thread double-buffers the data queue.
  // Method receiveMessage() stuffs the back buffer
  // while the front buffer is being sent.
  //
  // For a local connection (i.e., arSyncDataClient is in
  // our process), just wait for arSyncDataClient and then
  // signal arSyncDataClient to swap buffers.

  // Hack. If locally connected to an
  // arSyncDataClient, we assume that there's no need to dump state. The
  // two objects have been connected since the beginning. Also, we just
  // take a different code path...

  while (!_exitProgram) {
    // Don't swap buffers until the consumer is ready.
    _localConsumerReadyLock.lock();
    // _localConsumerReady takes one of 3 values:
    // 0: not ready for new data
    // 1: ready for new data
    // 2: stop program
    while (!_localConsumerReady) {
      _localConsumerReadyVar.wait(_localConsumerReadyLock);
    }
    if (_localConsumerReady == 2) {
      _localConsumerReadyLock.unlock();
      break;
    }

    _localConsumerReady = 0;
    _localConsumerReadyLock.unlock();
    if (_mode != AR_SYNC_AUTO_SERVER) {
      // Sync mode is manual, so wait for a buffer swap command.
      _signalObject.receiveSignal();
    }

    // Lock the queue and swap it. Release receiveMessage() if it's locked.
    // Copypasted from below.
    _queueLock.lock();
      _dataQueue->swapBuffers();
      _messageBufferFull = false;
      _messageBufferVar.signal();
    _queueLock.unlock();

    // Tell the locally connected arSyncDataClient.
    _localProducerReadyLock.lock();
    // _localProducerReady is one of:
    //   0: not ready for consumer to take new data
    //   1: ready for consumer to take new data
    //   2: stop program
    if (_localProducerReady == 2) {
      _localProducerReadyLock.unlock();
      break;
    }

    _localProducerReady = 1;
    _localProducerReadyVar.signal();
    _localProducerReadyLock.unlock();
    // If in manual buffer swap mode, release bufferSwap().
    if (_mode != AR_SYNC_AUTO_SERVER) {
      _signalObjectRelease.sendSignal();
    }
  }

  // Lock, so consumer won't deadlock on exit.
  arGuard dummy(_localProducerReadyLock);
  _localProducerReady = 2;
  _localProducerReadyVar.signal();
  ar_log_debug() << "arSyncDataServer: send thread done.\n";
}

void arSyncDataServer::_sendTaskRemote() {
  // Shutdown is indeterministic.  Just because the send call has
  // terminated on this end DOES NOT mean that it has been received on the
  // other side.  Major redesign needed?

  while (!_exitProgram) {
    // wait for a send action to be requested
    _signalObject.receiveSignal();
    // _signalObject may have been released because we should be exiting.
    if (_exitProgram)
      break;

    // Lock and swap the queue, releasing receiveMessage() if blocked
    _queueLock.lock();
    _dataQueue->swapBuffers(); // This can crash if app is dkill'ed.
    _messageBufferFull = false;
    _messageBufferVar.signal();
    if (_barrierServer.checkWaitingSockets()) {
      // this is what occurs upon connection
      _barrierServer.lockActivationQueue();
      list<arSocket*>* newSocketList =
        _barrierServer.getWaitingBondedSockets(&_dataServer);
      list<arSocket*>* activeSocketList = _dataServer.getActiveSockets();
      _barrierServer.activatePassiveSockets(&_dataServer);
      _dataServer.sendDataQueue(_dataQueue, activeSocketList);
      _connectionCallback(_bondedObject, _dataQueue, newSocketList);
      delete newSocketList;
      delete activeSocketList;
      _queueLock.unlock();
    }
    else{
      _queueLock.unlock();
      //ar_timeval time1, time2;
      //time1 = ar_time();
      _dataServer.sendDataQueue(_dataQueue);
      //time2 = ar_time();
      //      cout << "send time = " << ar_difftime(time2, time1) << "\n";
      // Set tuning data.
      _barrierServer.setServerSendSize(
        _dataQueue->getFrontBufferSize() * _dataServer.getNumberConnected());
    }
//     cout << "draw = " << _barrierServer.getDrawTime() << " "
// 	 << "proc = " << _barrierServer.getProcTime() << " "
// 	 << "recv = " << _barrierServer.getRcvTime() << "\n";

    // This local loop needs to be in the synchronization group.
    // we do not do this if we are in nosync mode!
    // The following call will not block forever!
    if (_mode != AR_NOSYNC_MANUAL_SERVER) {
      _barrierServer.localSync();
    }
  }
}

arSyncDataServer::arSyncDataServer() :
  _client(NULL),
  _serviceName("NULL"),
  _serviceNameBarrier("NULL"),
  _dataServer(200000),
  _mode(AR_SYNC_AUTO_SERVER),
  _dataQueue(NULL),
  _messageBufferFull(false),
  _exitProgram(false),
  _sendThreadRunning(false),
  _channel("NULL"),
  _locallyConnected(false),
  _localConsumerReady(0),
  _localProducerReady(0) {
}

arSyncDataServer::~arSyncDataServer() {
  if (_dataQueue)
    delete _dataQueue;
}

bool arSyncDataServer::setMode(int theMode) {
  if (theMode != AR_SYNC_AUTO_SERVER &&
      theMode != AR_SYNC_MANUAL_SERVER &&
      theMode != AR_NOSYNC_MANUAL_SERVER) {
    ar_log_error() << "arDataSyncServer: invalid operating mode " << theMode << ".\n";
    return false;
  }
  _mode = theMode;
  return true;
}

// In the case of arGraphicsServer and arSoundServer, we want to pass in
// the appropriate language (i.e. a graphics dictionary or a sound dictionary
// respectively).
bool arSyncDataServer::setDictionary(arTemplateDictionary* dictionary) {
  if (!dictionary) {
    ar_log_error() << "arSyncDataServer: NULL dictionary.\n";
    return false;
  }
  _dictionary = dictionary;
  if (!_dataQueue) {
    _dataQueue = new arQueuedData();
  }
  return true;
}

void arSyncDataServer::setBondedObject(void* bondedObject) {
  _bondedObject = bondedObject;
}

void arSyncDataServer::setConnectionCallback
  (bool (*connectionCallback)(void*, arQueuedData*, list<arSocket*>*)) {
  _connectionCallback = connectionCallback;
}

void arSyncDataServer::setMessageCallback
  (arDatabaseNode* (*messageCallback)(void*, arStructuredData*)) {
  _messageCallback = messageCallback;
}

void arSyncDataServer::setServiceName(const string& serviceName) {
  _serviceName = serviceName;
}

void arSyncDataServer::setChannel(const string& channel) {
  _channel = channel;
}

// Setup, but do not start, various threads
bool arSyncDataServer::init(arSZGClient& client) {
  if (_locallyConnected) {
    ar_log_error() << "arSyncDataServer ignoring locally connected init().\n";
    return false;
  }

  // There is not much consistency between the
  // way calls are broken-up in my various init's and start's.
  // Should they be combined into one????
  if (_channel == "NULL") {
    ar_log_error() << "arSyncDataServer: no channel before init().\n";
    return false;
  }

  _client = &client;

  // Use the "complex" service name
  _serviceNameBarrier = _client->createComplexServiceName(
    _serviceName+"_BARRIER");
  _serviceName = _client->createComplexServiceName(_serviceName);
  // connection brokering goes here
  _dataServer.smallPacketOptimize(true);
  int port = -1;
  if (!_client->registerService(_serviceName, _channel, 1, &port)) {
    ar_log_error() << "arSyncDataServer failed to register service.\n";
    return false;
  }

  // todo: factor out copypaste with barrier/arSyncDataServer.cpp:214
  _dataServer.setPort(port);
  _dataServer.setInterface("INADDR_ANY");
  bool success = false;
  for (int tries = 0; tries < 10; ++tries) {
    if (_dataServer.beginListening(_dictionary)) {
      success = true;
      break;
    }
    ar_log_warning() << "arSyncDataServer retrying to listen on brokered port.\n";
    _client->requestNewPorts(_serviceName, _channel, 1, &port);
    _dataServer.setPort(port);
  }
  if (!success) {
    // failed to bind to ports
    ar_log_error() << "arSyncDataServer failed to listen on brokered port.\n";
    return false;
  }
  if (!_client->confirmPorts(_serviceName, _channel, 1, &port)) {
    ar_log_error() << "arSyncDataServer error: failed to confirm ports.\n";
    return false;
  }
  // end of copypaste

  if (!_barrierServer.init(_serviceNameBarrier, _channel, client)) {
    ar_log_error() << "arSyncDataServer: barrier server failed to init.\n";
    return false;
  }
  ar_log_debug() << "arSyncDataServer initialized.\n";
  return true;
}

bool arSyncDataServer::start() {
  if (_locallyConnected) {
    // Not much happens if locally connected.
    // Copypaste from below.
    if (!_sendThread.beginThread(ar_syncDataServerSendTask, this)) {
      ar_log_error() << "arSyncDataServer failed to start send thread.\n";
      return false;
    }
    return true;
  }

  // This is what we do in the case of network connections (i.e. traditional)
  if (!_client) {
    ar_log_error() << "arSyncDataServer: init was not called before start.\n";
    return false;
  }

  // Synchronize the local production
  // loop with the loops that are occuring on other machines.
  // If we just synchronize the loops on other machines (i.e. the remote
  // renderers) then things can easily get out of phase during
  // connection. For instance, one machine might get a frame ahead.
  // BarrierServer::registerLocal() adds the
  // local production loop to the synchronization group.
  _barrierServer.registerLocal();

  // This is convoluted. In the case of AR_SYNC_AUTO_SERVER,
  // we let the synchronization control the buffer swapping,
  // so the barrier server signals _signalObject upon
  // release (which is being waited on at the top of the send thread).
  // In other cases, the buffer swapping occurs because of the bufferSwap
  // command;  then we use _signalObjectRelease, which is caught in
  // swapBuffers().  If we've called setSignalObjectRelease then
  // setSignalObject() is moot (see arBarrierServer).
  if (_mode != AR_SYNC_AUTO_SERVER)
    _barrierServer.setSignalObjectRelease(&_signalObjectRelease);
  _barrierServer.setSignalObject(&_signalObject);

  // Start the various services.
  if (!_barrierServer.start()) {
    ar_log_error() << "arSyncDataServer failed to start barrier server.\n";
    return false;
  }
  if (!_connectionThread.beginThread(ar_syncDataServerConnectionTask, this)) {
    ar_log_error() << "arSyncDataServer failed to start connection thread.\n";
    return false;
  }
  if (!_sendThread.beginThread(ar_syncDataServerSendTask, this)) {
    ar_log_error() << "arSyncDataServer failed to start send thread.\n";
    return false;
  }
  ar_log_debug() << "arSyncDataServer started.\n";
  return true;
}

void arSyncDataServer::stop() {
  _exitProgram = true;
  // Ensure the send data thread isn't blocked.
  _signalObject.sendSignal();

  if (_locallyConnected) {
    // Set the queue variables to "finished."

    _localConsumerReadyLock.lock();
      _localConsumerReady = 2;
      _localConsumerReadyVar.signal();
    _localConsumerReadyLock.unlock();

    _localProducerReadyLock.lock();
      _localProducerReady = 2;
      _localProducerReadyVar.signal();
    _localProducerReadyLock.unlock();
  }

  arSleepBackoff a(8, 20, 1.15);
  while (_sendThreadRunning) {
    a.sleep();
  }
}

void arSyncDataServer::swapBuffers() {
  if (_mode == AR_SYNC_AUTO_SERVER) {
    ar_log_remark() << "arSyncDataServer ignoring swapBuffers() in sync mode.\n";
    return;
  }

  _signalObject.sendSignal();
  if (_mode != AR_NOSYNC_MANUAL_SERVER) {
    // Wait for the other side to consume the buffer.
    _signalObjectRelease.receiveSignal();
  }
}

arDatabaseNode* arSyncDataServer::receiveMessage(arStructuredData* data) {
  // Caller ensures atomicity.
  arGuard dummy(_queueLock);
  if (_dataQueue->getBackBufferSize() > _sendLimit &&
      _barrierServer.getNumberConnectedActive() > 0 &&
      _mode == AR_SYNC_AUTO_SERVER) {
    _messageBufferFull = true;
    while (_messageBufferFull) {
      _messageBufferVar.wait(_queueLock);
    }
  }

  // Do this after the wait on a full buffer, but before queueing the data.
  //
  // If before the wait on full buffer, a connection
  // happening on a blocked buffer could send an initial
  // dump that's one record ahead of the buffer sent to the already
  // connected clients.
  //
  // After queueing the data, send the
  // record to a connected client without crucial info added (like the ID
  // of a new node, as in newNode or insert).

  arDatabaseNode* node = _messageCallback(_bondedObject, data);

  if (_barrierServer.getNumberConnectedActive() > 0 || _locallyConnected) {
    // Something, local or remote, wants our data.  Queue it.
    _dataQueue->forceQueueData(data);
  }

  return node;
}
