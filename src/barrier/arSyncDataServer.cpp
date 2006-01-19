//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSyncDataServer.h"
#include "arLogStream.h"

void ar_syncDataServerConnectionTask(void* pv){
  arSyncDataServer* server = (arSyncDataServer*)pv;
  while (!server->_exitProgram){
    if (!server->_dataServer.acceptConnectionNoSend()){
      ar_usleep(1000); // CPU throttle, if errors occur.
    }
  }
}

void ar_syncDataServerSendTask(void* pv){
  ((arSyncDataServer*)pv)->_sendTask();
}

void arSyncDataServer::_sendTask(){
  // This thread manages the double-buffering of the data queue.
  // The receiveMessage(...) method is stuffing data into the back buffer
  // of the queue, while the front buffer is being sent out on the network.
  // In the case of a purely local connection (i.e. the arSyncDataClient
  // is in the same process as us), all this really does is wait to swap
  // buffers until the arSyncDataClient is ready and signals the 
  // arSyncDataClient when we are ready to swap data buffers.

  // This is a bit of a hack. If we are locally connected to an 
  // arSyncDataClient, we assume that there's no need to dump state. The
  // two objects have been connected since the beginning. Also, we just
  // take a different code path...
  if (_locallyConnected){
    _sendThreadRunning = true;
    while (!_exitProgram){
      // We cannot swap buffers until the consumer is ready
      ar_mutex_lock(&_localConsumerReadyLock);
      // _localConsumerReady takes one of 3 values:
      // 0: not ready for new data
      // 1: ready for new data
      // 2: stop program
      while (!_localConsumerReady){
        _localConsumerReadyVar.wait(&_localConsumerReadyLock);
      }
      if (_localConsumerReady == 2){
	ar_mutex_unlock(&_localConsumerReadyLock);
        break;
      }
      _localConsumerReady = 0;
      ar_mutex_unlock(&_localConsumerReadyLock);
      // If the sync mode is manual, then we must also wait for a
      // buffer swap command to be issued.
      if (_mode != AR_SYNC_AUTO_SERVER){
        _signalObject.receiveSignal();
      }

      // go ahead and lock the queue and swap it, releasing the
      // receiveMessage method if it is locked. THIS IS COPY/PASTE
      // from below!
      ar_mutex_lock(&_queueLock);
      _dataQueue->swapBuffers();
      _messageBufferFull = false;
      _messageBufferVar.signal();
      ar_mutex_unlock(&_queueLock);
      // need to let the locally connected arSyncDataClient know
      ar_mutex_lock(&_localProducerReadyLock);
      // _localProducerReady takes one of 3 values
      // 0: not ready for consumer to take new data
      // 1: ready for consumer to take new data
      // 2: stop program
      // Might be the case that we should be stopping anyway
      if (_localProducerReady == 2){
        ar_mutex_unlock(&_localProducerReadyLock);
	break;
      }
      _localProducerReady = 1;
      _localProducerReadyVar.signal();
      ar_mutex_unlock(&_localProducerReadyLock);
      // If we are in manual buffer swap mode, the bufferSwap(...)
      // command must be released.
      if (_mode != AR_SYNC_AUTO_SERVER){
        _signalObjectRelease.sendSignal();
      }
    }
    // Make sure that the consumer does not deadlock on exit
    ar_mutex_lock(&_localProducerReadyLock);
    _localProducerReady = 2;
    _localProducerReadyVar.signal();
    ar_mutex_unlock(&_localProducerReadyLock);
    ar_log_remark() << "arSyncDataServer remark: send thread done.\n";
    _sendThreadRunning = false;
    return;
  }

  // This is the traditional code path... Network connections.

  // OUCH!!! There is a somewhat thorny problem in here with respect
  // to deterministic shut-down. Specifically, just because the send
  // call has terminated on this end DOES NOT mean that it has been
  // received on the other side!
  // It seems to me that a major redesign might be called for to
  // redress this issue!
  _sendThreadRunning = true;
  while (!_exitProgram){
    // to start out, we wait for a send action to be requested
    _signalObject.receiveSignal();
    // The signal object could have been released because we are supposed
    // to be exiting.
    if (_exitProgram){
      // pop out of the while loop
      break;
    }
    // next, go ahead and lock the queue and swap it, releasing the
    // receiveMessage method if it is blocked
    ar_mutex_lock(&_queueLock);
    _dataQueue->swapBuffers(); // This can crash if app is dkill'ed.
    _messageBufferFull = false;
    _messageBufferVar.signal();
    if (_barrierServer.checkWaitingSockets()){
      // this is what occurs upon connection
      _barrierServer.lockActivationQueue();
      list<arSocket*>* newSocketList =
        _barrierServer.getWaitingBondedSockets(&_dataServer);
      list<arSocket*>* activeSocketList = _dataServer.getActiveSockets();
      _barrierServer.activatePassiveSockets(&_dataServer);
      _dataServer.sendDataQueue(_dataQueue,activeSocketList);
      _connectionCallback(_bondedObject, _dataQueue, newSocketList); 
      delete newSocketList;
      delete activeSocketList;
      ar_mutex_unlock(&_queueLock); 
    }
    else{
      ar_mutex_unlock(&_queueLock);
      ar_timeval time1, time2;
      time1 = ar_time();
      _dataServer.sendDataQueue(_dataQueue);
      time2 = ar_time();
      //      cout << "send time = " << ar_difftime(time2,time1) << "\n";
      // Set tuning data.
      _barrierServer.setServerSendSize(
        _dataQueue->getFrontBufferSize() * _dataServer.getNumberConnected());
    }
//     cout << "draw = " << _barrierServer.getDrawTime() << " "
// 	 << "proc = " << _barrierServer.getProcTime() << " "
// 	 << "recv = " << _barrierServer.getRcvTime() << "\n";

    // This local loop needs to be in the synchronization group.
    // we do not do this if we are in nosync mode!
    // NOTE: the following call will not block forever!
    if (_mode != AR_NOSYNC_MANUAL_SERVER){
      _barrierServer.localSync();
    }
  }
  ar_log_remark() << "arSyncDataServer remark: send thread done.\n";
  _sendThreadRunning = false;
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
  _localProducerReady(0){

  ar_mutex_init(&_queueLock);
  ar_mutex_init(&_localConsumerReadyLock);
  ar_mutex_init(&_localProducerReadyLock);
}

arSyncDataServer::~arSyncDataServer(){
  if (_dataQueue)
    delete _dataQueue;
}

bool arSyncDataServer::setMode(int theMode){
  if (theMode != AR_SYNC_AUTO_SERVER
      && theMode != AR_SYNC_MANUAL_SERVER
      && theMode != AR_NOSYNC_MANUAL_SERVER){
    ar_log_error() << "arDataSyncServer error: invalid operating mode "
                   << theMode << ".\n";
    return false;
  }
  _mode = theMode;
  return true;
}

// In the case of arGraphicsServer and arSoundServer, we want to pass in
// the appropriate language (i.e. a graphics dictionary or a sound dictionary
// respectively). 
bool arSyncDataServer::setDictionary(arTemplateDictionary* dictionary){
  if (!dictionary) {
    ar_log_error() << "arSyncDataServer error: NULL dictionary.\n";
    return false;
  }
  _dictionary = dictionary;
  if (!_dataQueue){
    _dataQueue = new arQueuedData();
  }
  return true;
}

void arSyncDataServer::setBondedObject(void* bondedObject){
  _bondedObject = bondedObject;
}

void arSyncDataServer::setConnectionCallback
  (bool (*connectionCallback)(void*,arQueuedData*,list<arSocket*>*)){
  _connectionCallback = connectionCallback;
}

void arSyncDataServer::setMessageCallback
  (arDatabaseNode* (*messageCallback)(void*,arStructuredData*)){
  _messageCallback = messageCallback;
}

void arSyncDataServer::setServiceName(string serviceName){
  _serviceName = serviceName;
}

void arSyncDataServer::setChannel(string channel){
  _channel = channel;
}

/// Sets things up, but does not start the various threads
bool arSyncDataServer::init(arSZGClient& client){
  if (_locallyConnected){
    ar_log_error() << "arSyncDataServer error: init should not be called if "
	           << "locally connected.\n";
    return false;
  }
  // HMMMM! This is a little weird... There is not much consistency between the
  // way calls are broken-up in my various init's and start's. 
  // Should they be combined
  // into one????
  if (_channel == "NULL"){
    ar_log_error() << "arSyncDataServer error: "
	           << "channel not set before init(...).\n";
    return false;
  }
  _client = &client;
  // we really want to use the "complex" service name
  _serviceNameBarrier = _client->createComplexServiceName(
    _serviceName+"_BARRIER");
  _serviceName = _client->createComplexServiceName(_serviceName);
  // connection brokering goes here
  _dataServer.smallPacketOptimize(true);
  int port = -1;
  if (!_client->registerService(_serviceName,_channel,1,&port)){
    ar_log_error() << "arSyncDataServer error: failed to register service.\n";
    return false;
  }

  /// \todo factor out copy-paste with barrier/arSyncDataServer.cpp:214
  _dataServer.setPort(port);
  _dataServer.setInterface("INADDR_ANY");
  bool success = false;
  for (int tries = 0; tries < 10; ++tries) {
    if (_dataServer.beginListening(_dictionary)) {
      success = true;
      break;
    }
    ar_log_warning() << "arSyncDataServer warning: failed to listen on "
	             << "brokered port, retrying.\n";
    _client->requestNewPorts(_serviceName,_channel,1,&port);
    _dataServer.setPort(port);
  }
  if (!success){
    // failed to bind to ports
    ar_log_error() << "arSyncDataServer error: failed to listen on "
                   << "brokered port.\n";
    return false;
  }
  if (!_client->confirmPorts(_serviceName,_channel,1,&port)){
    ar_log_error() << "arSyncDataServer error: failed to confirm ports.\n";
    return false;
  }
  // end of copy-paste

  _barrierServer.setServiceName(_serviceNameBarrier);
  _barrierServer.setChannel(_channel);
  if (!_barrierServer.init(client)) {
    ar_log_error() << "arSyncDataServer error: barrier server failed to init.\n";
    return false;
  }
  ar_log_remark() << "arSyncDataServer remark: initialized.\n";
  return true;
}

bool arSyncDataServer::start(){
  if (_locallyConnected){
    // Note that we don't do much if only locally connected
    // COPY/PASTE from below!
    if (!_sendThread.beginThread(ar_syncDataServerSendTask,this)) {
      ar_log_error() << "arSyncDataServer error: failed to start send thread.\n";
      return false;
    }
    return true;
  }

  // This is what we do in the case of network connections (i.e. traditional)
  if (!_client){
    ar_log_error() << "arSyncDataServer error: init was not called before start.\n";
    return false;
  }

  // note that it is very important that we synchronize the local production
  // loop with the loops that are occuring on other machines.
  // if we just synchronize the loops on other machines (i.e. the remote
  // renderers) then things can easily get out of phase during the
  // connection process. For instance, one machine might get a frame ahead of
  // everybody else. The registerLocal() method of barrier server puts the
  // local production loop in the synchronization group.
  _barrierServer.registerLocal();
  // This is a little bit confusing. In the case of AR_SYNC_AUTO_SERVER,
  // we are letting the synchronization control the buffer swapping. 
  // This means that the barrier server will signal _signalObject upon
  // release (which is being waited upon at the top of the send thread).
  // In other cases, the buffer swapping occurs because of the bufferSwap
  // command and then we use the _signalObjectRelease, which is caught in
  // swapBuffers(). Note that if we've call setSignalObjectRelease then
  // setSignalObject(...) is moot (look in the arBarrierServer code).
  if (_mode != AR_SYNC_AUTO_SERVER){
    _barrierServer.setSignalObjectRelease(&_signalObjectRelease);
  }
  _barrierServer.setSignalObject(&_signalObject);
  // Start the various services.
  if (!_barrierServer.start()) {
    ar_log_error() << "arSyncDataServer error: "
		   << "failed to start barrier server.\n";
    return false;
  }
  if (!_connectionThread.beginThread(ar_syncDataServerConnectionTask,this)) {
    ar_log_error() << "arSyncDataServer error: "
	           << "failed to start connection thread.\n";
    return false;
  }
  if (!_sendThread.beginThread(ar_syncDataServerSendTask,this)) {
    ar_log_error() << "arSyncDataServer error: failed to start send thread.\n";
    return false;
  }
  ar_log_remark() << "arSyncDataServer remark: started.\n";
  return true;
}

void arSyncDataServer::stop(){
  _exitProgram = true;
  // better make sure we are not blocked in the send data thread
  _signalObject.sendSignal();
  // In local connection mode, it is also necessary to set the queue variables
  // to finish state.
  if (_locallyConnected){
    ar_mutex_lock(&_localConsumerReadyLock);
    _localConsumerReady = 2;
    _localConsumerReadyVar.signal();
    ar_mutex_unlock(&_localConsumerReadyLock);
    ar_mutex_lock(&_localProducerReadyLock);
    _localProducerReady = 2;
    _localProducerReadyVar.signal();
    ar_mutex_unlock(&_localProducerReadyLock);
  }
  // wait for that thread to finish
  while (_sendThreadRunning){
    ar_usleep(10000);
  }
}

void arSyncDataServer::swapBuffers(){
  // this call should not even be made in the following case!
  if (_mode == AR_SYNC_AUTO_SERVER){
    ar_log_remark() << "arSyncDataServer remark: ignoring swapBuffers() in sync mode.\n";
    return;
  }
  _signalObject.sendSignal();
  // Wait for the buffer consumption on the other side
  // (but not if in no-sync mode).
  if (_mode != AR_NOSYNC_MANUAL_SERVER){
    _signalObjectRelease.receiveSignal();
  }
}

arDatabaseNode* arSyncDataServer::receiveMessage(arStructuredData* data){
  // Caller is responsible for ensuring atomicity.
  ar_mutex_lock(&_queueLock);
  if (_dataQueue->getBackBufferSize() > _sendLimit &&
      _barrierServer.getNumberConnectedActive() > 0 &&
      _mode == AR_SYNC_AUTO_SERVER){
    _messageBufferFull = true;
    while (_messageBufferFull){
      _messageBufferVar.wait(&_queueLock);
    }
  }

  // Very important that this occurs after the wait on a full buffer
  // but before putting the data on the queue!
  // If it comes before the wait on full before, a connection
  // happening on a blocked buffer could, in fact, send an initial
  // dump that's one record ahead of the buffer sent to the already 
  // connected clients.
  // If it comes after putting data on the queue, then we can send the
  // record to a connected client without crucial info added (like the ID
  // of a new node, as in newNode or insert), which would be bad!
  arDatabaseNode* node = _messageCallback(_bondedObject,data); 

  // Only queue the data if there are currently connected processes...
  // OR if we are purely connected locally (i.e. a arSyncDataClient is
  // in the same process as us).
  if (_barrierServer.getNumberConnectedActive() > 0
      || _locallyConnected){
    _dataQueue->forceQueueData(data);
  }
  
  ar_mutex_unlock(&_queueLock);
  return node;  
}
