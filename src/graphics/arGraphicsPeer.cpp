//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsPeer.h"

string arGraphicsPeerConnection::print(){ 
  stringstream result;
  result << "connection = " << remoteName << "/"
         << connectionID << ":";
  if (receiving){
    result << "1";
  }
  else{
    result << "0";
  }
  result << ":";
  if (sending){
    result << "1";
  }
  else{
    result << "0";
  }
  result << ":";
  for (list<int>::iterator i = nodesLockedLocal.begin();
       i != nodesLockedLocal.end();
       i++){
    if (i != nodesLockedLocal.begin()){
      result << "/";
    }
    result << *i;
  }
  result << "\n";
  return result.str();
}

class arGraphicsPeerSerializeInfo{ 
 public:
  arGraphicsPeerSerializeInfo(){}
  ~arGraphicsPeerSerializeInfo(){}

  arGraphicsPeer* peer;
  arSocket* socket;
  int rootID;
  bool relay;
};

void ar_graphicsPeerSerializeFunction(void* info){
  arGraphicsPeerSerializeInfo* i = (arGraphicsPeerSerializeInfo*) info;
  i->peer->_serializeAndSend(i->socket, i->rootID, i->relay);
  i->peer->_serializeDoneNotify(i->socket);
  // It is our responsibility to delete this.
  delete i;
}

void ar_graphicsPeerConsumptionFunction(arStructuredData* data,
                                        void* graphicsPeer,
                                        arSocket* socket){
  // Need to go ahead and translate the void* into something useful.
  arGraphicsPeer* gp = (arGraphicsPeer*)graphicsPeer;
  arGraphicsLanguage* l = &gp->_gfx;
  // Messages from other peers are binned into the internal buffer,
  // to be read in as a whole before drawing the scene.
  if (data->getID() == l->AR_GRAPHICS_ADMIN){
    // Various things can happen here. We can be told to relay data to the
    // requester. We can be told to dump the database.
    string action = data->getDataString(l->AR_GRAPHICS_ADMIN_ACTION);
    int nodeID;
    if (action == "map"){
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      gp->_resetConnectionMap(socket->getID(), nodeID);
    }
    else if (action == "node_map"){
      int* mapPtr = (int*) data->getDataPtr(l->AR_GRAPHICS_ADMIN_NODE_ID, 
                                            AR_INT);
      int mapDim = data->getDataDimension(l->AR_GRAPHICS_ADMIN_NODE_ID);
      ar_mutex_lock(&gp->_socketsLock);
      // Important that remember which sockets are receiving info.
      // When we get this message, the remote peer informs us that
      // it will be sending scene graph updates.
      map<int, arGraphicsPeerConnection*, less<int> >::iterator i
        = gp->_connectionContainer.find(socket->getID());
      if (i == gp->_connectionContainer.end()){
        cout << "arGraphicsPeer internal error: could not find connection "
	     << "object.\n";
      }
      else{
        i->second->inMap.insert(map<int,int,less<int> >::value_type(mapPtr[1],
								    mapPtr[0]));
        if (mapDim == 4){
          i->second->inMap.insert
            (map<int,int,less<int> >::value_type(mapPtr[3], mapPtr[2]));
	}
      }
      ar_mutex_unlock(&gp->_socketsLock);
    }
    else if (action == "frame_time"){
      // These locks must be CONSISTENTLY nested everywhere!
      ar_mutex_lock(&gp->_alterLock); 
      ar_mutex_lock(&gp->_socketsLock);
      int frameTime = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      map<int, arGraphicsPeerConnection*, less<int> >::iterator i
        = gp->_connectionContainer.find(socket->getID());
      if (i == gp->_connectionContainer.end()){
        cout << "arGraphicsPeer internal error: could not find connection "
	     << "object.\n";
      }
      else{
        i->second->remoteFrameTime = frameTime;
      }
      ar_mutex_unlock(&gp->_socketsLock);
      ar_mutex_unlock(&gp->_alterLock);  
    }
    else if (action == "dump"){
      // Since every node creation message results in a message sent back
      // to us, to avoid a deadlock (when there are too many messages
      // in the return queue and none have been processed) we need to go
      // ahead and launch a new thread for this!
      arThread dumpThread;
      arGraphicsPeerSerializeInfo* serializeInfo 
        = new arGraphicsPeerSerializeInfo();
      serializeInfo->peer = gp;
      serializeInfo->socket = socket;
      serializeInfo->rootID = 0;
      serializeInfo->relay = false;
      dumpThread.beginThread(ar_graphicsPeerSerializeFunction, serializeInfo);
    }
    else if (action == "dump-relay"){
      // Since every node creation message results in a message sent back
      // to us, to avoid a deadlock (when there are too many messages
      // in the return queue and none have been processed) we need to go
      // ahead and launch a new thread for this!
      arThread dumpThread;
      arGraphicsPeerSerializeInfo* serializeInfo 
        = new arGraphicsPeerSerializeInfo();
      serializeInfo->peer = gp;
      serializeInfo->socket = socket;
      serializeInfo->rootID = 0;
      serializeInfo->relay = true;
      dumpThread.beginThread(ar_graphicsPeerSerializeFunction, serializeInfo);
    }
    else if (action == "dump-done"){
      ar_mutex_lock(&gp->_dumpLock);
      gp->_dumped = true;
      gp->_dumpVar.signal();
      ar_mutex_unlock(&gp->_dumpLock);
    }
    else if (action == "receive-on"){
      ar_mutex_lock(&gp->_socketsLock);
      // Important that remember which sockets are receiving info.
      // When we get this message, the remote peer informs us that
      // it will be sending scene graph updates.
      map<int, arGraphicsPeerConnection*, less<int> >::iterator i
        = gp->_connectionContainer.find(socket->getID());
      if (i == gp->_connectionContainer.end()){
        cout << "arGraphicsPeer internal error: could not find connection "
	     << "object.\n";
      }
      else{
        i->second->receiving = true;
      }
      ar_mutex_unlock(&gp->_socketsLock);
    }
    else if (action == "receive-off"){
      ar_mutex_lock(&gp->_socketsLock);
      // Important that remember which sockets are receiving info.
      // When we get this message, the remote peer informs us that
      // it will be sending scene graph updates.
      map<int, arGraphicsPeerConnection*, less<int> >::iterator i
        = gp->_connectionContainer.find(socket->getID());
      if (i == gp->_connectionContainer.end()){
        cout << "arGraphicsPeer internal error: could not find connection "
	     << "object.\n";
      }
      else{
        i->second->receiving = false;
      }
      ar_mutex_unlock(&gp->_socketsLock);
    }
    else if (action == "relay-on"){
      gp->_activateSocket(socket);
    }
    else if (action == "relay-off"){
      gp->_deactivateSocket(socket);
    }
    else if (action == "close"){
      // Probably a good idea to handshake back with the following.
      // We want an *active* close on the other end. This avoids
      // situations where we are waiting for the other end to
      // passively go down... and closeAllAndReset can deadlock as
      // a result.
      arStructuredData adminData(l->find("graphics admin"));
      adminData.dataInString(l->AR_GRAPHICS_ADMIN_ACTION, "close");
      gp->_dataServer->sendData(&adminData, socket);
      gp->_closeConnection(socket);
    }
    else if (action == "lock"){
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      gp->_lockNode(nodeID, socket);
    }
    else if (action == "unlock"){
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      gp->_unlockNode(nodeID);
    }
    else if (action == "filter_data"){
      int dataFilterInfo[2];
      data->dataOut(l->AR_GRAPHICS_ADMIN_NODE_ID, dataFilterInfo, AR_INT, 2);
      gp->_filterDataBelow(dataFilterInfo[0], socket, dataFilterInfo[1]);
    }
    else if (action =="set-name"){
      string socketLabel = data->getDataString(l->AR_GRAPHICS_ADMIN_NAME);
      gp->_dataServer->setSocketLabel(socket, socketLabel);
      // also need to set the name for this connection in the
      // container.
      ar_mutex_lock(&gp->_socketsLock);
      map<int, arGraphicsPeerConnection*, less<int> >::iterator i
	= gp->_connectionContainer.find(socket->getID());
      if (i == gp->_connectionContainer.end()){
	cout << "arGraphicsPeer internal error: could not get requested "
	     << "object.\n";
      }
      else{
        i->second->remoteName = socketLabel;
      }
      ar_mutex_unlock(&gp->_socketsLock);
    }
    else if (action == "ID-request"){
      // A little bit cheesy. We're only allowing a single round trip
      // at a time.
      string nodeName = data->getDataString(l->AR_GRAPHICS_ADMIN_NAME);
      nodeID = gp->getNodeID(nodeName);
      data->dataIn(l->AR_GRAPHICS_ADMIN_NODE_ID, &nodeID, AR_INT, 1);
      data->dataInString(l->AR_GRAPHICS_ADMIN_ACTION, "ID-response");
      gp->_dataServer->sendData(data, socket);
    }
    else if (action == "ID-response"){
      // A little bit cheesy. We're only allowing a single round trip at a
      // time.
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      ar_mutex_lock(&gp->_IDResponseLock);
      gp->_requestedNodeID = nodeID;
      gp->_IDResponseVar.signal();
      ar_mutex_unlock(&gp->_IDResponseLock);
    }
  }
  else{
    // This is just a standard record. We either queue this OR send it
    // along to the alter, based on the configuration of the peer.
    // First, however, we tag it according to its origin.
    int* IDs;
    int dataID = data->getID();
    if (!gp->_databaseReceive[dataID]){
      // NOT a message directed to the database itself.
      data->setDataDimension(gp->_routingField[dataID], 2);
      IDs = (int*)data->getDataPtr(gp->_routingField[dataID],AR_INT);
      IDs[1] = socket->getID();
    }
    else{
      // We must take care to deal with "make node" commands as well.
      // (and erase node commands)
      if (dataID == l->AR_MAKE_NODE){
        data->setDataDimension(l->AR_MAKE_NODE_ID, 2);
        IDs = (int*)data->getDataPtr(l->AR_MAKE_NODE_ID, AR_INT);
        IDs[1] = socket->getID();
      }
      if (dataID == l->AR_ERASE){
        data->setDataDimension(l->AR_ERASE_ID, 2);
        IDs = (int*)data->getDataPtr(l->AR_ERASE_ID, AR_INT);
        IDs[1] = socket->getID();
      }
    }
    if (gp->_queueingData){
      ar_mutex_lock(&gp->_queueLock);
      gp->_incomingQueue->forceQueueData(data);
      ar_mutex_unlock(&gp->_queueLock);
    }
    else{
      if (!gp->alter(data)){
        cout << "arGraphicsPeer error: alter failed.\n";
      }
    }
  }
}

void ar_graphicsPeerDisconnectFunction(void* graphicsPeer,
				       arSocket* socket){
  // The arGraphicsPeer must maintain a list of sockets that have 
  // requested updates. When the consumption function receives
  // a special message, it adds the socket to the list. When a socket
  // disconnects, we must remove from the list.
  // THESE SOCKETS SEND INFORMATION
  arGraphicsPeer* gp = (arGraphicsPeer*) graphicsPeer;
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // THere is a deadlock here! Since sendData is called within a socketsLock
  // and can result in an immediate call to this callback!
  ar_mutex_lock(&gp->_socketsLock);
  // NOTE: This has now been folded into the connection list.
  /*for (list<arSocket*>::iterator i = gp->_outgoingSockets.begin();
       i != gp->_outgoingSockets.end();
       i++){
    if (socket->getID() == (*i)->getID()){
      gp->_outgoingSockets.erase(i);
      // We are done.
      break;
    }
    }*/
  // The arGraphicsPeer maintains a list of connections. The affected
  // connection must be removed from the list and any nodes it is currently
  // locking must be unlocked.
  map<int, arGraphicsPeerConnection*, less<int> >::iterator j
    = gp->_connectionContainer.find(socket->getID());
  if (j != gp->_connectionContainer.end()){
    for (list<int>::iterator k = j->second->nodesLockedLocal.begin();
	 k != j->second->nodesLockedLocal.end(); k++){
      ar_mutex_lock(&gp->_alterLock);
      gp->_unlockNodeNoNotification(*k);
      ar_mutex_unlock(&gp->_alterLock);
    }
    delete j->second;
    gp->_connectionContainer.erase(j);
  }
  else{
    cout << "arGraphicsPeer internal error: deleted connection "
	 << "without object.\n";
  }
  ar_mutex_unlock(&gp->_socketsLock);
}

void ar_graphicsPeerConnectionTask(void* graphicsPeer){
  arGraphicsPeer* gp = (arGraphicsPeer*) graphicsPeer;
  while (true){
    arSocket* socket = gp->_dataServer->acceptConnection();
    if (!socket){
      break;
    }
    // must insert new connection object into the table
    // NOTE: we are guaranteed that this key is unique since
    // IDs are not reused.
    ar_mutex_lock(&gp->_socketsLock);
    arGraphicsPeerConnection* newConnection = new arGraphicsPeerConnection();
    // we do not know the remote name yet. That will be forwarded to
    // us.
    newConnection->connectionID = socket->getID();
    newConnection->socket = socket;
    newConnection->rootMapNode = &gp->_rootNode;
    gp->_connectionContainer.insert(
      map<int, arGraphicsPeerConnection*, less<int> >::value_type(
	socket->getID(),
	newConnection));
    ar_mutex_unlock(&gp->_socketsLock);
  }
  cout << "arGraphicsPeer error: connection accept failed.\n";
}

arGraphicsPeer::arGraphicsPeer(){
  // set a few defaults and initialize the mutexes.
  ar_mutex_init(&_alterLock);
  ar_mutex_init(&_socketsLock);
  ar_mutex_init(&_queueLock);
  ar_mutex_init(&_IDResponseLock);
  ar_mutex_init(&_dumpLock);
  _localDatabase = true;
  _queueingData = false;
  _name = string("default");
  _client = NULL;
  _incomingQueue = new arQueuedData();
  _dataServer = new arDataServer(1000);
  _dataServer->smallPacketOptimize(true);

  _requestedNodeID = -1;

  _dumped = false;

  _readWritePath = "";

  _bridgeDatabase = NULL;
  _bridgeRootMapNode = NULL;
}

arGraphicsPeer::~arGraphicsPeer(){

}

/// The graphics peer uses the "phleet" to offer its services to the
/// world, based on the "name" key.
void arGraphicsPeer::setName(const string& name){
  _name = name;
}

/// We are initialized as part of a larger entity.
bool arGraphicsPeer::init(arSZGClient& client){
  // All this does is set the arSZGClient...
  _client = &client;
  string result = _client->getAttribute("SZG_PEER","path");
  if (result != "NULL"){
    _readWritePath = result;
  }
  return true;
}

/// We have our own arSZGClient
bool arGraphicsPeer::init(int& argc, char** argv){
  _client = new arSZGClient();
  _client->simpleHandshaking(true);
  _client->init(argc, argv);
  if (!(*_client)){
    return false;
  }
  string result = _client->getAttribute("SZG_PEER","path");
  if (result != "NULL"){
    _readWritePath = result;
  }
  return true;
}

bool arGraphicsPeer::start(){
  if (!_client){
    cerr << "arGraphicsPeer error: start() called before init().\n";
    return false;
  }
  string serviceName = _client->createComplexServiceName("szg-rp-"+_name);
  // Register the service. AARGH! THIS IS PRETTY-MUCH COPY-PASTED EVERYWHERE!
  int port;
  if (!_client->registerService(serviceName,"default",1,&port)){
    cerr << "arGraphicsPeer error: failed to register service.\n";
    return false;
  }

  _dataServer->setConsumerFunction(ar_graphicsPeerConsumptionFunction);
  _dataServer->setConsumerObject(this);
  _dataServer->setDisconnectFunction(ar_graphicsPeerDisconnectFunction);
  _dataServer->setDisconnectObject(this);
  // Definitely want have finer-grained locks then over the processing
  // of each admin message. This makes message operations that take
  // a long time to complete practical.
  _dataServer->atomicReceive(false);
  _dataServer->setPort(port);
  _dataServer->setInterface("INADDR_ANY");
  bool success = false;
  for (int tries = 0; tries < 10; ++tries) {
    if (_dataServer->beginListening(_gfx.getDictionary())) {
      success = true;
      break;
    }
    cerr << "arGraphicsPeer warning: failed to listen on "
	 << "brokered port, retrying.\n";
    _client->requestNewPorts(serviceName,"default",1,&port);
    _dataServer->setPort(port);
  }
  if (!success){
    // failed to bind to ports
    cerr << "arGraphicsPeer error: failed to listen on "
         << "brokered port.\n";
    return false;
  }
  if (!_client->confirmPorts(serviceName,"default",1,&port)){
    cerr << "arGraphicsPeer error: failed to confirm ports.\n";
    return false;
  }
  // end of copy-paste
  // Launch the connection thread that accepts connections for that
  // service.
  _connectionThread.beginThread(ar_graphicsPeerConnectionTask, this);
  return true;
}
  
void arGraphicsPeer::stop(){
  // DOESN'T DO ANYTHING YET... DOH!
  closeAllAndReset();
  _client->closeConnection();  
}

arDatabaseNode* arGraphicsPeer::alter(arStructuredData* data){ 
  map<int, arGraphicsPeerConnection*, less<int> >::iterator connectionIter;
  int potentialNewNodeID = -1;
  arDatabaseNode* result = &_rootNode;
  int filterIDs[4];
  // Needed for filtering on "transient" nodes.
  ar_timeval currentTime;
  // Important to do this so we don't mistakenly send a map record to the
  // other side.
  filterIDs[0] = -1;
  ar_mutex_lock(&_alterLock);
  ar_mutex_lock(&_socketsLock);
  // NOTE: we exploit the fact that the ID field of the record is an ARRAY
  // of ints. As data comes in, classify it as follows:
  //  1. Does the ID have data dimension 1? If so, this record was LOCALLY
  //     generated.
  //  2. If the ID has data dimension > 1, then we must have processed it
  //     through the external connections. The second int in the ID field
  //     will give the ID of the external connection.
  //  3. In either case, the behavior varies based on the origin.
  //     a. Do not send records back to their origins
  //     b. Check the "lock" on that node. If one exists, and is different
  //        than the origin of this message, return, doing nothing.  
  int dataID = data->getID();
  int originID = -1;
  int* IDPtr = NULL;
  // We must first determine the origin of the data. If it has come across
  // a communications link then it will be subect to "mapping" (i.e. having the
  // ID of the target node changed) 
  if (!_databaseReceive[dataID]){
    // We are not talking directly to the database, but instead have a
    // message for a particular node. NOTE: there are two data paths that
    // the peer can take to get to here. Either local (in which case the
    // data dimension will be 1 or remote (in which case the data dimension
    // will be 2). In the local case, go ahead and leave the originID at
    // its default of -1.
    IDPtr = (int*) data->getDataPtr(_routingField[dataID], AR_INT);
    if (data->getDataDimension(_routingField[dataID]) == 2){
      originID = IDPtr[1];
    }
  }
  else{
    // Even if we are talking to the database, the database messages
    // should not be casually relayed. For instance, consider the message
    // to create a node.
    if (dataID == _gfx.AR_MAKE_NODE){
      IDPtr = (int*)data->getDataPtr(_gfx.AR_MAKE_NODE_ID, AR_INT);
      if (data->getDataDimension(_gfx.AR_MAKE_NODE_ID) == 2){
        originID = IDPtr[1];
        // NO NEED TO DEAL WITH LOCKING IN NODE CREATION
      }
    }
    if (dataID == _gfx.AR_ERASE){
      IDPtr = (int*)data->getDataPtr(_gfx.AR_ERASE_ID, AR_INT);
      if (data->getDataDimension(_gfx.AR_ERASE_ID) == 2){
        originID = IDPtr[1];
        // NO NEED TO DEAL WITH LOCKING IN NODE ERASE??
      }
    }
  }
  // Do the mapping. 
  if (originID != -1){
    // This data has not come locally. Get the connection that spawned it
    // and run it through the node map.
    connectionIter = _connectionContainer.find(originID);
    if (connectionIter == _connectionContainer.end()){
      // There is no connection with that ID currently used. This is
      // really an error.
      ar_mutex_unlock(&_socketsLock);
      ar_mutex_unlock(&_alterLock);
      // Return a pointer to the root node.
      return result; 
    }
    // Such a connection does exist. NOTE: we are using the *mapping*
    // mode here (as indicated by the final false) of updating the
    // node map.
    potentialNewNodeID = _filterIncoming(connectionIter->second->rootMapNode,
                                         data,
					 connectionIter->second->inMap,
					 filterIDs,
                                         false);
  }

  // Check to see if the node is locked by a source other than the origin of
  // this communication. If so, go ahead and return. Note that the
  // _filterIncoming might have changed this ID. 
  if (IDPtr){
    map<int, int, less<int> >::iterator j = _lockContainer.find(IDPtr[0]);
    if (j != _lockContainer.end()){
      if (j->second != originID){
        // If the node has been locked, go ahead and return.
        // (we return the root node for default success).
        ar_mutex_unlock(&_socketsLock);
        ar_mutex_unlock(&_alterLock);
        return result;
      }
    }
  }
  // We are not necessarily altering the local database. 
  // IT IS CRITICALLY IMPORTANT THAT THE ALTERATION OF THE LOCAL DATABASE
  // OCCURE BEFORE PASSING THE DATA ON TO OTHER PEERS. THE LOCAL PEER
  // IS ALLOWED TO MODIFY THE RECORD "IN PLACE" (AS IN BEING ABLE TO
  // ATTACH A NODE TO THE DATABASE THAT WAS CREATED SOME OTHER WAY).
  // Node creation alters the record in place! See how arDatabase::_makeNode
  // works...
  if (_localDatabase){
    // If we are actually going to the local database, go ahead and get the
    // result from that...
    // BUG BUG BUG BUG BUG BUG BUG BUG BUG
    // Sometimes it is not appropriate to send filtered messages to the
    // local database.... but here we ALWAYS do so... (for instance,
    // the return value from _filterIncoming might be 0.
    result = arGraphicsDatabase::alter(data);
    if (_bridgeDatabase){
      _sendDataToBridge(data);
    } 
    // If a new node has been created, we need to augment the node
    // map for this connection.
    if (potentialNewNodeID > 0 && result){
      connectionIter->second->inMap.insert
        (map<int, int, less<int> >::value_type
      	  (potentialNewNodeID, result->getID()));
      // Don't forget to put this in the filterIDs.
      // Sometimes there can actually be TWO pairs in added to the node map,
      // so we need to test for this.
      if (filterIDs[1] == -1){
        filterIDs[1] = result->getID();
      }
      else{
        filterIDs[3] = result->getID();
      }
    }
  }
  // Go ahead and send to the connected peers who desire
  // updates. NOTE: The peers will either alter their
  // internal databases immediately (if they are NOT drawers)
  // or will queue the alterations (if they are drawers)
  
  map<int, int, less<int> >::iterator outIter;
  for (connectionIter = _connectionContainer.begin();
       connectionIter != _connectionContainer.end(); 
       connectionIter++){
    // NOTE: we do not want a feedback loop. So, we should not send back
    // to the origin, unless the origin made us update our node map,
    // in which case, we need to send back the results.
    if (connectionIter->second->connectionID != originID){
      // Better check whether or not this is a "transient" node. If so,
      // (and the update time has NOT passed), we might do nothing.
      bool updateNodeEvenIfTransient = true;
      if (result && result->_transient){
        currentTime = ar_time();
        if (result->_invalidUpdateTime
            || ar_difftime(currentTime,result->_lastUpdate)
	    > connectionIter->second->remoteFrameTime){
          result->_invalidUpdateTime = false;
          result->_lastUpdate = currentTime;
	}
        else{
	  // Filter out this update on this connection.
	  updateNodeEvenIfTransient = false;
        }
      }
      // Still need to check the connection's filter.
      IDPtr = (int*) data->getDataPtr(_routingField[dataID], AR_INT);
      outIter = connectionIter->second->outFilter.find(IDPtr[0]);
      if (updateNodeEvenIfTransient && connectionIter->second->sending && 
           (outIter == connectionIter->second->outFilter.end()
            || outIter->second == 1)){
        _dataServer->sendData(data, connectionIter->second->socket);
      }
    }
    else{
      // The sender's node map needs to be augmented exactly when the
      // filterIDs array has been modified.
      if (filterIDs[0] != -1){
        arStructuredData adminData(_gfx.find("graphics admin"));
        adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "node_map");
        adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, filterIDs, AR_INT, 
                         filterIDs[2] == -1 ? 2 : 4);
        _dataServer->sendData(&adminData, connectionIter->second->socket);
      }
    }
  }
  ar_mutex_unlock(&_socketsLock);
  ar_mutex_unlock(&_alterLock);
  // No matter what happened just above, we return the state from just before.
  return result;
}

/// The read/write methods are redefined so that we IMPLICITLY use a path
/// as might be specified by the arSZGClient through init unless one is
/// explicitly specified.
bool arGraphicsPeer::readDatabase(const string& fileName, 
                                  const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::readDatabase(fileName, _readWritePath);
  }  
  return arGraphicsDatabase::readDatabase(fileName, path);
}
  
bool arGraphicsPeer::readDatabaseXML(const string& fileName, 
                                     const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::readDatabaseXML(fileName, _readWritePath);
  }  
  return arGraphicsDatabase::readDatabaseXML(fileName, path);
}

bool arGraphicsPeer::attachXML(arDatabaseNode* parent,
			       const string& fileName,
			       const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::attachXML(parent, fileName, _readWritePath);
  }
  return arGraphicsDatabase::attachXML(parent, fileName, path);
}

bool arGraphicsPeer::mapXML(arDatabaseNode* parent,
			    const string& fileName,
			    const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::mapXML(parent, fileName, _readWritePath);
  }
  return arGraphicsDatabase::mapXML(parent, fileName, path);
}

bool arGraphicsPeer::writeDatabase(const string& fileName, 
                                   const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::writeDatabase(fileName, _readWritePath);
  }  
  return arGraphicsDatabase::writeDatabase(fileName, path);
}

bool arGraphicsPeer::writeDatabaseXML(const string& fileName, 
                                      const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::writeDatabaseXML(fileName, _readWritePath);
  }  
  return arGraphicsDatabase::writeDatabaseXML(fileName, path);
}

bool arGraphicsPeer::writeRootedXML(arDatabaseNode* parent,
                                      const string& fileName, 
                                      const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::writeRootedXML(parent,
                                              fileName, 
                                              _readWritePath);
  }  
  return arGraphicsDatabase::writeRootedXML(parent, fileName, path);
}

/// It is not always desirable to have a local database. For instance,
/// what if we have a simple, throwaway program that just wants to send
/// messages to nodes with known IDs. The default is to use a local
/// database.
void arGraphicsPeer::useLocalDatabase(bool state){
  _localDatabase = state;
}

/// By default, the peer alters its local database immediately upon
/// receiving messages. However, this is not desirable if it is also
/// being drawn, for instance. In that case, we manually choose when to
/// draw the peer. The default is false.
void arGraphicsPeer::queueData(bool state){
  _queueingData = state;
}

/// If our peer is displaying graphics, it is desirable to only alter
/// its database between draws, not during draws. (HMMMM... or is it
/// REALLY desirable if we aren't trying to maintain consistency?)
int arGraphicsPeer::consume(){
  ar_mutex_lock(&_queueLock);
  _incomingQueue->swapBuffers();
  ar_mutex_unlock(&_queueLock);
  //int bufferSize;
  //ar_unpackData(_incomingQueue->getFrontBufferRaw(),&bufferSize,AR_INT,1);
  //cout << "buffer size = " << bufferSize << "\n";
  int bufferSize = _incomingQueue->getFrontBufferSize();
  handleDataQueue(_incomingQueue->getFrontBufferRaw());
  return bufferSize;
}

/// Attempt to connect to the named graphics peer (with service name
/// szg-rp-<given name>. Upon failure, return -1. For instance, no service
/// might exist with that name. On success, return the ID of the socket
/// which has the connection. Locally, we can just refer to the connection
/// by name. BUT some of the RPC text wrappers actually need to be able to
/// return the ID for the proxy objects.
/// THIS IS A NON-BLOCKING CALL.
int arGraphicsPeer::connectToPeer(const string& name){
  // First, make sure that we haven't already connected to a peer of the same
  // name.
  int ID = _dataServer->getFirstIDWithLabel(name);
  if (ID != -1){
    // The above returns -1 if it can't find a connection... so otherwise
    // we COULD find a connection.
    cerr << "arGraphicsPeer remark: cannot make duplicate connection.\n";
    return -1;
  }
  arPhleetAddress result 
    = _client->discoverService(
         _client->createComplexServiceName("szg-rp-"+name),
         _client->getNetworks("default"), false);
  if (!result.valid){
    cerr << "arGraphicsPeer remark: could not connect to named peer.\n";
    return -1;
  }
  int socketID = _dataServer->dialUpFallThrough(result.address, 
					        result.portIDs[0]);
  if (socketID < 0){
    cerr << "arGraphicsPeer remark: failed to connect to brokered ports.\n";
    return -1;
  }
  // WE WANT THIS LABEL SET!
  arSocket* socket = _dataServer->getConnectedSocket(socketID);
  _dataServer->setSocketLabel(socket, name);
  // must insert new connection object into the table
  // NOTE: we are guaranteed that this key is unique since
  // IDs are not reused.
  ar_mutex_lock(&_socketsLock);
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // There is a race condition whereby a new connection that
  // disappeared *immediately* might not be correctly handled.
  // ANOTHER REASON TO HANDLE DISCONNECT EVENTS DIFFERENTLY
  arGraphicsPeerConnection* newConnection = new arGraphicsPeerConnection();
  newConnection->remoteName = name;
  newConnection->connectionID = socket->getID();
  newConnection->socket = socket;
  newConnection->rootMapNode = &_rootNode;
  _connectionContainer.insert(
    map<int, arGraphicsPeerConnection*, less<int> >::value_type(
      socket->getID(),
      newConnection));
  ar_mutex_unlock(&_socketsLock);
  // set the remote socket with our name
  _setRemoteLabel(socket, _name);
  return socket->getID();
}

/// Close the connection to the named graphics peer. This will have had
/// service name szg-rp-<name>. NOTE: THIS CALL IS BADLY DESIGNED. We send
/// a close message and then return before the connection has even
/// necessarily been closed. THIS REFLECTS OVERALL PROBLEMS WITH THE
/// WAY THE SYZYGY NETWORKING HAS BEEN IMPLEMENTED. Really, we need a way
/// to handle opening/closing connections that is much less automatic and
/// a bit more MANUAL.
bool arGraphicsPeer::closeConnection(const string& name){
  int socketID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(socketID);
  if (!socket){
    return false;
  }
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "close");
  return _dataServer->sendData(&adminData, socket);
}

/// By default, nothing happens when we connect a graphics peer to another.
/// The graphics peers must request actions. For instance, we might want the
/// remote peer to relay all messages it receives to us. For this effect,
/// set "state" to true. To stop relaying once it has started, set "state"
/// to false.
bool arGraphicsPeer::receiving(const string& name, bool state){
  arStructuredData adminData(_gfx.find("graphics admin"));
  if (state){
    adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "relay-on");
  }
  else{
    adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "relay-off"); 
  }
  // NOTE: labels are FORCED to be unique since they are connected with
  // peer names.
  int socketID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(socketID);
  if (!socket){
    return false;
  }
  _dataServer->sendData(&adminData, socket);
  ar_mutex_lock(&_socketsLock);
  // Important that remember which sockets have requested updates.
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i
    = _connectionContainer.find(socket->getID());
  if (i == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: could not find connection "
	 << "object.\n";
  }
  else{
    if (state){
      i->second->receiving = true;
    }
    else{
      // relaying is off
      i->second->receiving = false;
    }
  }
  ar_mutex_unlock(&_socketsLock);
  return true;
}

/// Proactively start sending the connected peer data, without being asked.
bool arGraphicsPeer::sending(const string& name, bool state){
  // NOTE: labels are forced to be unique since they are connected with 
  // peer names.
  int socketID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(socketID);
  if (!socket){
    return false;
  }
  // NOTE: the remote peer needs to be able to keep track of whether it
  // is receiving data. COnsequently, we must send a message saying we
  // are doing so...
  arStructuredData adminData(_gfx.find("graphics admin"));
  if (state){
    adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "receive-on");
  }
  else{
    adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "receive-off"); 
  }
  _dataServer->sendData(&adminData, socket);

  if (state){
    _activateSocket(socket);
  }
  else{
    _deactivateSocket(socket);
  }
  return true;
}

/// By default, nothing happens when we connect a graphics peer to another.
/// The graphics peers must request actions.
bool arGraphicsPeer::pullSerial(const string& name, bool receiveOn){
  arStructuredData adminData(_gfx.find("graphics admin"));
  if (receiveOn){
    adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "dump-relay");
  }
  else{
    adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "dump");
  }
  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  if (receiveOn){
    // Important to remember who has requested updates.
    ar_mutex_lock(&_socketsLock);
    map<int, arGraphicsPeerConnection*, less<int> >::iterator i
      = _connectionContainer.find(socket->getID());
    if (i == _connectionContainer.end()){
      cout << "arGraphicsPeer internal error: could not find connection "
	   << "object.\n";
    }
    else{
      i->second->receiving = true;
    }
    ar_mutex_unlock(&_socketsLock);
  }
  // We have to wait for the dump to be completed
  ar_mutex_lock(&_dumpLock);
  _dumped = false;
  _dataServer->sendData(&adminData, socket);
  while (!_dumped){
    _dumpVar.wait(&_dumpLock);
  }
  ar_mutex_unlock(&_dumpLock);
  return true;
}

/// By default, nothing happens when we connect one graphics peer to another.
/// The following call allows us to push our serialization to a connected peer.
bool arGraphicsPeer::pushSerial(const string& name, 
                                int remoteRootID,
                                bool sendOn){
  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  // NOTE: this call does not send a serialization-done notification.
  return _serializeAndSend(socket, remoteRootID, sendOn);
}

/// Closes all connected sockets and resets the database. This is 
/// accomplished via sending all connected peers a "close" message.
/// They close the sockets on their side, which causes all our
/// reading threads to go away and close the relevant sockets on
/// our side. 
bool arGraphicsPeer::closeAllAndReset(){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "close");
  _dataServer->sendData(&adminData);
  // wait for everybody to close... THIS IS BAD SINCE IT RELIES ON TRUSTING
  // THE CONNECTED PEERS!
  while (_dataServer->getNumberConnected() > 0){
    ar_usleep(100000);
  }
  // Delete the stuff in the database. It is important to do this
  // AFTER disconnecting from everybody else. Why? Because we don't
  // want to erase THEIR databases as well!
  reset();
  return true;
}

bool arGraphicsPeer::broadcastFrameTime(int frameTime){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "frame_time");
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, &frameTime, AR_INT, 1);
  _dataServer->sendData(&adminData);
  return true;
}

/// Sometimes, we want a node on a remote peer to only change according to
/// our actions. (this is useful if several peers are connected to a single
/// remote peer and are changing the same things).
bool arGraphicsPeer::lockRemoteNode(const string& name, int nodeID){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "lock");
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, &nodeID, AR_INT, 1);
  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  _dataServer->sendData(&adminData, socket);
  return true;
}

/// Of course, we want to be able to unlock the node as well.
/// Why is the connection name present? Because we are unlocking the node
/// on THAT peer (i.e. the one at the other end of the named connection)
bool arGraphicsPeer::unlockRemoteNode(const string& name, int nodeID){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "unlock");
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, &nodeID, AR_INT, 1);
  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  _dataServer->sendData(&adminData, socket);
  return true;
}

/// To preserve symmetry of operations, it would be nice to be able to
/// lock a node to the updates from a particular connection.
bool arGraphicsPeer::lockLocalNode(const string& name, int nodeID){
  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    cout << "arGraphicsPeer error: no such connection.\n";
    return false;
  }
  _lockNode(nodeID, socket);
  return true;
}

/// We also want to be able to unlock one of our nodes (instead of relying
/// on another peer to do it for us). Note that it isn't necessary to name
/// a connection since the model is that we are removing ALL locking on the
/// node.
bool arGraphicsPeer::unlockLocalNode(int nodeID){
  _unlockNode(nodeID);
  // REALLY LAME THAT THIS RETURNS TRUE NO MATTER WHAT!
  return true;
}

/// We want to be able to prevent a remote peer from sending us data (or
/// turn sending back on once we have turned it off).
bool arGraphicsPeer::filterDataBelowRemote(const string& peer,
                                           int remoteNodeID,
                                           int on){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "filter_data");
  int data[2];
  data[0] = remoteNodeID;
  data[1] = on;
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, data, AR_INT, 2);
  int ID = _dataServer->getFirstIDWithLabel(peer);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  _dataServer->sendData(&adminData, socket);
  return true;
}

/// Keep data from being relayed to a remote peer (or start relaying the
/// data again).
bool arGraphicsPeer::filterDataBelowLocal(const string& peer,
                                          int localNodeID,
                                          int on){
  int ID = _dataServer->getFirstIDWithLabel(peer);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    cout << "arGraphicsPeer error: no such connection.\n";
    return false;
  }
  // Lame that this doesn't complain if no such node.
  _filterDataBelow(localNodeID, socket, on);
  return true;
}

/// Sometimes, we want to be able to find the ID of the node on a remotely
/// connected peer.
int arGraphicsPeer::getNodeIDRemote(const string& peer,
				    const string& nodeName){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "ID-request");
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_NAME, nodeName);
  int ID = _dataServer->getFirstIDWithLabel(peer);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  ar_mutex_lock(&_IDResponseLock);
  _requestedNodeID = -2;
  _dataServer->sendData(&adminData, socket);
  // On error, we'll get back -1, and otherwise nonnegative.
  while (_requestedNodeID == -2){
    _IDResponseVar.wait(&_IDResponseLock);
  }
  int result = _requestedNodeID;
  ar_mutex_unlock(&_IDResponseLock);
  return result;
}

/*list<arGraphicsPeerConnection> arGraphicsPeer::getConnections(){
  list<arGraphicsPeerConnection> result;
  ar_mutex_lock(&_socketsLock);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i;
  for (i = _connectionContainer.begin();
       i != _connectionContainer.end(); i++){
    result.push_back(i->second);
  }
  ar_mutex_unlock(&_socketsLock);
  return result;
  }*/

string arGraphicsPeer::printConnections(){
  stringstream result;
  ar_mutex_lock(&_socketsLock);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i;
  for (i = _connectionContainer.begin();
       i != _connectionContainer.end(); i++){
    result << i->second->print();
  }
  ar_mutex_unlock(&_socketsLock);
  return result.str();
}

string arGraphicsPeer::print(){
  stringstream result;
  ar_mutex_lock(&_alterLock);
  printStructure(100, result);
  ar_mutex_unlock(&_alterLock);
  return result.str();
}

/// Sets the remote name for this connection (allows the mirroring graphics
/// peer to know who connected to it.
bool arGraphicsPeer::_setRemoteLabel(arSocket* sock, const string& name){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "set-name");
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_NAME, name);
  return _dataServer->sendData(&adminData, sock);
}

/// Serialize the peer and send it out on the given socket. We might also
/// activate sending on that socket as well. This does NOT notify the
/// remote peer that the serialization is done.
/// Note that the node map is constructed from a particular node ID base.
/// This node ID is the *remote* node ID (from the perspective of the
/// remote scene graph).  
bool arGraphicsPeer::_serializeAndSend(arSocket* socket, 
                                       int remoteRootID,
                                       bool sendOn){
  // First thing to do is to send the "map" command.
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "map");
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, &remoteRootID, AR_INT, 1);
  if (!_dataServer->sendData(&adminData, socket)){
    return false;
  }
  ar_mutex_lock(&_alterLock);
  arStructuredData nodeData(_gfx.find("make node"));
  if (!nodeData){
    cerr << "arGraphicsPeer error: failed to create node record.\n";
    ar_mutex_unlock(&_alterLock);
    return false;
  }
  bool success = true;
  _recSerialize(&_rootNode, nodeData, socket, success);
  // If relaying has also been requested, go ahead and enable that.
  if (sendOn){
    _activateSocket(socket);
  }
  ar_mutex_unlock(&_alterLock);

  return success;
}

/// When a remote peer requests out serialization, we must send a "done"
/// message (because it will block in pullSerial(...) until this is received.
void arGraphicsPeer::_serializeDoneNotify(arSocket* socket){
  // Must send serialization-completion-repsonse
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "dump-done");
  if (!_dataServer->sendData(&adminData, socket)){
    cout << "szg-rp error: failed to send dump-completion response.\n";
  }
}

void arGraphicsPeer::_activateSocket(arSocket* socket){
  ar_mutex_lock(&_socketsLock);
  // We see if the socket can be found. This is a bit of a hack... since
  // we've got pointers but instead want to match the stuff via handles.

  // It is redundant to use the sockets list too. Instead, we use the
  // connection list exclusively.
  /*bool found = false;
  for (list<arSocket*>::iterator i = _outgoingSockets.begin();
       i != _outgoingSockets.end();
       i++){
    if (socket->getID() == (*i)->getID()){
      found = true;
      break;
    }
  }
  if (!found){
    _outgoingSockets.push_back(socket);
  }*/
  map<int, arGraphicsPeerConnection*, less<int> >::iterator j
    = _connectionContainer.find(socket->getID());
  if ( j == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: could not find requested "
	 << "object.\n";
  }
  else{
    j->second->sending = true;
  }
  ar_mutex_unlock(&_socketsLock);
}

void arGraphicsPeer::_deactivateSocket(arSocket* socket){
  ar_mutex_lock(&_socketsLock);
  /*for (list<arSocket*>::iterator i = _outgoingSockets.begin();
       i != _outgoingSockets.end();
       i++){
    if (socket->getID() == (*i)->getID()){
      _outgoingSockets.erase(i);
      map<int, arGraphicsPeerConnection*, less<int> >::iterator j
        = _connectionContainer.find(socket->getID());
      if ( j == _connectionContainer.end()){
        cout << "arGraphicsPeer internal error: could not find requested "
	     << "object.\n";
      }
      else{
        j->second->sending = false;
      }
      break;
    }
    }*/
  // Again, it is redundant to have a connection list and an outgoing 
  // sockets list.
  map<int, arGraphicsPeerConnection*, less<int> >::iterator j
    = _connectionContainer.find(socket->getID());
  if ( j == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: could not find requested "
	 << "object.\n";
  }
  else{
    j->second->sending = false;
  }
  ar_mutex_unlock(&_socketsLock);
}

void arGraphicsPeer::_closeConnection(arSocket* socket){
  // Disconnect funtion will be called (implicitly) by remove connection.
  // All the lock removal, _connectionContainer updating, etc. is handled
  // there. 
  _dataServer->removeConnection(socket->getID());
}

void arGraphicsPeer::_resetConnectionMap(int connectionID, int nodeID){
  ar_mutex_lock(&_socketsLock);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i
    = _connectionContainer.find(connectionID);
  if ( i == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: missed connection when trying "
	 << "to reset map.\n";
  }
  else{
    i->second->inMap.clear();
    arDatabaseNode* newRootNode = getNode(nodeID);
    if (!newRootNode){
      cout << "arGraphicsPeer error: connection map root node not present. "
	   << "Using default.\n";
      i->second->rootMapNode = &_rootNode;
    } 
    else{
      i->second->rootMapNode = newRootNode;
    }
  }
  ar_mutex_unlock(&_socketsLock);
}

void arGraphicsPeer::_lockNode(int nodeID, arSocket* socket){
  // This is a little bit cheesy. We let whoever requests the lock
  // have the lock. Maybe not too bad assuming a cooperative model.
  ar_mutex_lock(&_alterLock);
  map<int, int, less<int> >::iterator i = _lockContainer.find(nodeID);
  if (i != _lockContainer.end()){
    _lockContainer.erase(i);
  }
  _lockContainer.insert(map<int,int,less<int> >::value_type(nodeID,
							    socket->getID()));
  ar_mutex_lock(&_socketsLock);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator j
    = _connectionContainer.find(socket->getID());
  if (j == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: could not find requested "
	 << "object.\n";
  }
  else{
    bool found = false;
    for (list<int>::iterator k = j->second->nodesLockedLocal.begin();
	 k != j->second->nodesLockedLocal.end(); k++){
      if (*k == nodeID){
        found = true;
        break;
      }
    }
    if (!found){
      // go ahead and add it (this deals with the potentially weird case
      // of locking the same node multiple times)
      j->second->nodesLockedLocal.push_back(nodeID);
    }
  }
  ar_mutex_unlock(&_socketsLock);
  ar_mutex_unlock(&_alterLock);
}

// Use this when someone else is unlocking the node.
void arGraphicsPeer::_unlockNode(int nodeID){
  ar_mutex_lock(&_alterLock);
  ar_mutex_lock(&_socketsLock);
  int socketID = _unlockNodeNoNotification(nodeID);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i
    = _connectionContainer.find(socketID);
  if (i == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: could not find requested "
	 << "object.\n";
  }
  else{
    i->second->nodesLockedLocal.remove(nodeID);
  }
  ar_mutex_unlock(&_socketsLock);
  ar_mutex_unlock(&_alterLock);
}

// Use this when the fact that the connection is going away is unlocking
// the node (in this case we do the node-unlocking in the disconnect
// function). Returns the ID of the socket that was holding the lock.
int arGraphicsPeer::_unlockNodeNoNotification(int nodeID){
  int result = -1;
  map<int, int, less<int> >::iterator i = _lockContainer.find(nodeID);
  if (i != _lockContainer.end()){
    result = i->second;
    _lockContainer.erase(i);
  }
  return result;
}

void arGraphicsPeer::_filterDataBelow(int nodeID,
                                      arSocket* socket,
                                      int on){
  arDatabaseNode* pNode = getNode(nodeID);
  if (!pNode){
    return;
  }
  ar_mutex_lock(&_socketsLock);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i
    = _connectionContainer.find(socket->getID());
  if (i == _connectionContainer.end()){
    cout << "arGraphicsPeer: internal error. Could not find requested "
	 << "connection.\n";
  }
  else{
    _recDataOnOff(pNode, on, i->second->outFilter);
  }
  ar_mutex_unlock(&_socketsLock);
}

void arGraphicsPeer::_recSerialize(arDatabaseNode* pNode,
                                   arStructuredData& nodeData,
                                   arSocket* socket,
                                   bool& success){
  // This will fail for the root node
  if (fillNodeData(&nodeData, pNode)){
    if (!_dataServer->sendData(&nodeData, socket)){
      success = false;
    }
    arStructuredData* theData = pNode->dumpData();
    if (!_dataServer->sendData(theData, socket)){
      success = false;
    }
    delete theData;
  }
  list<arDatabaseNode*> children = pNode->getChildren();
  list<arDatabaseNode*>::iterator i;
  for (i=children.begin(); i!=children.end(); i++){
    if (success){
      _recSerialize(*i, nodeData, socket, success);
    }
  }
}

void arGraphicsPeer::_recDataOnOff(arDatabaseNode* pNode,
                                   int value,
                                   map<int, int, less<int> >& filterMap){
  map<int, int, less<int> >::iterator iter = filterMap.find(pNode->getID());
  if (iter == filterMap.end()){
    filterMap.insert(map<int, int, less<int> >::value_type
		     (pNode->getID(), value));
  }
  else{
    iter->second = value;
  }
  list<arDatabaseNode*> children = pNode->getChildren();
  list<arDatabaseNode*>::iterator i;
  for (i=children.begin(); i!=children.end(); i++){
    _recDataOnOff(*i, value, filterMap);
  }
}

/// There is a *hack* whereby data is transfered to the "bridge"
/// database. Note that we need to filter before putting into the bridge
/// AND restore the record (since filtering can alter the "make node"
/// message in place.
void arGraphicsPeer::_sendDataToBridge(arStructuredData* data){
  int parentID, nodeID;
  //int* parentIDPtr = (int*)data->getPtr(_lang->AR_MAKE_NODE_PARENT_ID, AR_INT);
  //int* nodeIDPtr = (int*)data->getPtr(_lang->AR_MAKE_NODE_ID, AR_INT);
  // Some fields may be altered by the maps!
  if (data->getID() == _lang->AR_MAKE_NODE){
    parentID = data->getDataInt(_lang->AR_MAKE_NODE_PARENT_ID);
    nodeID = data->getDataInt(_lang->AR_MAKE_NODE_ID);
  }
  else if (data->getID() == _lang->AR_ERASE){
    // NOT HANDLED!
    // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  }
  else{
    nodeID = data->getDataInt(_routingField[data->getID()]);
  }
  // The filterIDs parameter is NULL because there is no need to
  // construct a reverse map... nothing is coming back... the bridge is
  // one way.
  int potentialNewNodeID 
    = _bridgeDatabase->_filterIncoming(_bridgeRootMapNode,
                                       data,
				       _bridgeInMap,
				       NULL,
                                       false);
  arDatabaseNode* result = NULL;
  if (potentialNewNodeID){
    result = _bridgeDatabase->alter(data);
  }
  if (potentialNewNodeID > 0 && result){
    _bridgeInMap.insert(map<int, int, less<int> >::value_type
      (potentialNewNodeID, result->getID()));
  }
  // Must put the piece of data back the way it was, if it was altered.
  if (data->getID() == _lang->AR_MAKE_NODE){
    data->dataIn(_lang->AR_MAKE_NODE_PARENT_ID, &parentID, AR_INT, 1);
    data->dataIn(_lang->AR_MAKE_NODE_ID, &nodeID, AR_INT, 1);
  }
  else if (data->getID() == _lang->AR_ERASE){
    // NOT HANDLED!
    // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  }
  else{
    data->dataIn(_routingField[data->getID()], &nodeID, AR_INT, 1);
  }
}


