//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsPeer.h"

void arGraphicsPeerCullObject::clear(){ 
  cullOnOff.clear(); 
}

void arGraphicsPeerCullObject::frame(){ 
  cullChangeOn.clear(); 
  cullChangeOff.clear(); 
}

void arGraphicsPeerCullObject::insert(int ID, int state){
  map<int,int,less<int> >::iterator i = cullOnOff.find(ID);
  if (i == cullOnOff.end()){
    cullOnOff.insert(map<int,int,less<int> >::value_type(ID, state));
  }
  else{
    if (i->second != state){
      i->second = state;
      if (state){
	// Now on.
	cullChangeOn.push_back(ID);
      }
      else{
	cullChangeOff.push_back(ID);
      }
    }
  }
}

arGraphicsPeerConnection::arGraphicsPeerConnection(){ 
  remoteName = "NULL"; 
  connectionID = -1;
  socket = NULL;
  rootMapNode = NULL;
  // By default, send all updates.
  sendLevel = AR_TRANSIENT_NODE;
  remoteFrameTime = 0;
  // Without doing this, the children on the root node will never 
  // get automatically mapped to a feedback peer! Choose the most permissive
  // send level. (because we send updates if the node level is <= the
  // filter level)
  outFilter.insert(map<int,int,less<int> >::value_type(0,AR_TRANSIENT_NODE));
}

string arGraphicsPeerConnection::print(){ 
  stringstream result;
  result << "connection = " << remoteName << "/"
         << connectionID << ":";
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
  arGraphicsPeerSerializeInfo() : peer(NULL), socket(NULL) {}
  ~arGraphicsPeerSerializeInfo(){}

  arGraphicsPeer* peer;
  arSocket* socket;
  int localRootID;
  int remoteRootID;
  arNodeLevel sendLevel;
  arNodeLevel localSendLevel;
  arNodeLevel remoteSendLevel;
};

void ar_graphicsPeerSerializeFunction(void* info){
  arGraphicsPeerSerializeInfo* i = (arGraphicsPeerSerializeInfo*) info;
  i->peer->_serializeAndSend(i->socket, 
                             i->remoteRootID,
                             i->localRootID, 
                             i->sendLevel, 
                             i->remoteSendLevel,
			     i->localSendLevel);
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
    string action(data->getDataString(l->AR_GRAPHICS_ADMIN_ACTION));
    int nodeID = -1;
    if (action == "map"){
      int nodeIDs[2] = {0,0};
      data->dataOut(l->AR_GRAPHICS_ADMIN_NODE_ID, nodeIDs, AR_INT, 2);
      gp->_resetConnectionMap(socket->getID(), nodeIDs[0], 
                              ar_convertToNodeLevel(nodeIDs[1]));
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
    else if (action == "pull_serial"){
      // Since every node creation message results in a message sent back
      // to us, to avoid a deadlock (when there are too many messages
      // in the return queue and none have been processed) we need to go
      // ahead and launch a new thread for this!
      arThread dumpThread;
      arGraphicsPeerSerializeInfo* serializeInfo 
        = new arGraphicsPeerSerializeInfo();
      int nodeIDs[5];
      data->dataOut(l->AR_GRAPHICS_ADMIN_NODE_ID, nodeIDs, AR_INT, 5);
      serializeInfo->peer = gp;
      serializeInfo->socket = socket;
      serializeInfo->remoteRootID = nodeIDs[1];
      serializeInfo->localRootID = nodeIDs[0];
      serializeInfo->sendLevel = ar_convertToNodeLevel(nodeIDs[2]);
      serializeInfo->localSendLevel = ar_convertToNodeLevel(nodeIDs[3]);
      serializeInfo->remoteSendLevel = ar_convertToNodeLevel(nodeIDs[4]);
      // The parameter is deleted inside the thread.
      dumpThread.beginThread(ar_graphicsPeerSerializeFunction, serializeInfo);
    }
    else if (action == "dump-done"){
      ar_mutex_lock(&gp->_dumpLock);
      gp->_dumped = true;
      gp->_dumpVar.signal();
      ar_mutex_unlock(&gp->_dumpLock);
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
    else if (action == "lock_below"){
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      gp->_lockNodeBelow(nodeID, socket);
    }
    else if (action == "unlock"){
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      gp->_unlockNode(nodeID);
    }
    else if (action == "unlock_below"){
      nodeID = data->getDataInt(l->AR_GRAPHICS_ADMIN_NODE_ID);
      gp->_unlockNodeBelow(nodeID);
    }
    else if (action == "filter_data"){
      int dataFilterInfo[2];
      data->dataOut(l->AR_GRAPHICS_ADMIN_NODE_ID, dataFilterInfo, AR_INT, 2);
      gp->_filterDataBelow(dataFilterInfo[0], socket, 
                           ar_convertToNodeLevel(dataFilterInfo[1]));
    }
    else if (action == "mapped_filter_data"){
      int mappedNodeID = -1;
      int dataFilterInfo[2];
      data->dataOut(l->AR_GRAPHICS_ADMIN_NODE_ID, dataFilterInfo, AR_INT, 2);
      ar_mutex_lock(&gp->_socketsLock);
      map<int, arGraphicsPeerConnection*, less<int> >::iterator i
        = gp->_connectionContainer.find(socket->getID());
      if (i == gp->_connectionContainer.end()){
        cout << "arGraphicsPeer internal error: could not find connection "
	     << "object.\n";
      }
      else{
        map<int, int, less<int> >::iterator mapIter 
          = i->second->inMap.find(dataFilterInfo[0]);
        if (mapIter != i->second->inMap.end()){
          mappedNodeID = mapIter->second;
	}
      }
      ar_mutex_unlock(&gp->_socketsLock);
      if (mappedNodeID != -1){
        gp->_filterDataBelow(mappedNodeID, socket, 
                             ar_convertToNodeLevel(dataFilterInfo[1]));
      }
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
    else if (action == "camera_node"){
      // Just go ahead and discard.
    }
    else{
      cout << "arGraphicsPeer error: received illegal admin message ("
	   << action << ").\n";
    }
  }
  else{
    // This is just a standard message. We either queue this OR send it
    // along to the alter, based on the configuration of the peer.
    // First, however, we examine a distinguished field (the ID field).
    // This field (in its first place) contains information about this
    // message's destination. Subsequent places contain a history of where
    // the message has been. The final place contains a socket ID upon which
    // the *previous* peer received this message (this is irrelevant to us
    // here and will be over-written by our current component ID). 
    // We check to make sure that our component ID does no appear on the
    // previously visited list. If so, the message is discarded.
    // If not, we expand the field by one place. The next-to-last place 
    // gets our component ID. The last place gets the incoming socket ID.
    
    int dataID = data->getID();
    int fieldID = gp->_getRoutingFieldID(dataID);
    int fieldSize = data->getDataDimension(fieldID);
    int* IDs = (int*)data->getDataPtr(fieldID, AR_INT);
    // Checking for component repeat. The first and last item are not
    // relevant. If we exceed a certain hop limit, also go ahead and 
    // discard. (here the hop limit is 10)
    bool cycleFound = fieldSize > 12 ? true : false;
    for (int i=1; i<fieldSize-1 && !cycleFound; i++){
      if (IDs[i] == gp->_componentID){
        cycleFound = true;
      }
    }
    if (!cycleFound){
      if (fieldSize == 1){
	fieldSize += 2;
        data->setDataDimension(fieldID, fieldSize);
      }
      else{
        fieldSize++;
        data->setDataDimension(fieldID, fieldSize);
      }
      // Must reget the ptr since it might have changed.
      IDs = (int*)data->getDataPtr(fieldID, AR_INT);
      // Go ahead and alter the message's history.
      IDs[fieldSize-2] = gp->_componentID;
      IDs[fieldSize-1] = socket->getID();
      // Process the message as appropriate for our application's style
      // (either queued or not).
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
    // Otherwise, just return.
  }
}

class arGraphicsPeerConnectionDeletionInfo{
 public:
  arGraphicsPeerConnectionDeletionInfo() : peer(NULL), socket(NULL) {}
  ~arGraphicsPeerConnectionDeletionInfo(){}

  arGraphicsPeer* peer;
  arSocket*       socket;
};

void ar_graphicsPeerConnectionDeletionFunction(void* deletionInfo){
  arGraphicsPeerConnectionDeletionInfo* d =
    (arGraphicsPeerConnectionDeletionInfo*) deletionInfo;
  arGraphicsPeer* gp = d->peer;
  arSocket* socket = d->socket;
  ar_mutex_lock(&gp->_socketsLock);
  // The arGraphicsPeer maintains a list of connections. The affected
  // connection must be removed from the list and any nodes it is currently
  // locking must be unlocked.
  map<int, arGraphicsPeerConnection*, less<int> >::iterator j
    = gp->_connectionContainer.find(socket->getID());
  if (j != gp->_connectionContainer.end()){
    for (list<int>::iterator k = j->second->nodesLockedLocal.begin();
	 k != j->second->nodesLockedLocal.end(); k++){
      // Not necessary to further lock this statement since every
      // instance of _alterLock is inside _socketsLock. Indeed, using
      // _alterLock here would allow a deadlock! (order of locks reversed)
      gp->_unlockNodeNoNotification(*k);
    }
    delete j->second;
    gp->_connectionContainer.erase(j);
  }
  else{
    cout << "arGraphicsPeer internal error: deleted connection "
	 << "without object.\n";
  }
  ar_mutex_unlock(&gp->_socketsLock);
  // Don't forget to delete the local storage.
  delete d;
}

void ar_graphicsPeerDisconnectFunction(void* graphicsPeer,
				       arSocket* socket){
  // We want to avoid a deadlock with the remote stuff.
  // (this can occur since sendData(...) can cause this function to be called,
  // it is within a socketsLock block, and the connection removal must
  // come within a socketsLock block also). Consequently, for removal from
  // the internal connection queue, we launch a thread that eliminates the
  // connection when it can obtain the lock.

  arThread deletionThread;
  arGraphicsPeerConnectionDeletionInfo* deletionInfo = 
    new arGraphicsPeerConnectionDeletionInfo();
  deletionInfo->peer = (arGraphicsPeer*) graphicsPeer;
  deletionInfo->socket = socket;
  deletionThread.beginThread(ar_graphicsPeerConnectionDeletionFunction,
			     deletionInfo);
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
  // WEIRD... THIS SEEMS TO MAKE THINGS WORK BETTER! AT LEAST WITH LEGION!
  // BUT... THIS IS DIFFERENT THAN THE BEHAVIOR ON WINDOWS, WHERE SETTING
  // THIS FLAG = TRUE IS NECESSARY FOR THE NORMAL OPERATION OF szgrender
  // and DIST SCENE GRAPH STUFF! 
  // Also, it only seems to be a problem when windows sends to windows...
  //
  // Originally, this was TRUE... and this created a problem (many jerky
  // instances of non-sending) on windows->windows...
  _dataServer->smallPacketOptimize(false);

  _requestedNodeID = -1;

  _dumped = false;

  _readWritePath = "";

  _bridgeDatabase = NULL;
  _bridgeRootMapNode = NULL;
  
  _componentID = -1;
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
  _componentID = _client->getProcessID();
  return true;
}

bool arGraphicsPeer::start(){
  if (!_client){
    cerr << "arGraphicsPeer error: start() called before init().\n";
    return false;
  }
  // NOTE: We go against pretty much normal convention in szg and
  // just use the peer's service name STRAIGHT instead of creating a
  // "complex service name". The idea is that people will want to share peers
  // and that applications like "peerBridge" want to peer able to take
  // peers and run them under the auspices of a virtual computer.
  // The old way...
  //string serviceName = _client->createComplexServiceName("szg-rp-"+_name);
  // The new way...
  string serviceName = "szg-rp-"+_name;
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
  closeAllAndReset();
  _client->closeConnection();  
}

arDatabaseNode* arGraphicsPeer::alter(arStructuredData* data){ 
  // NOTE: no "graphics admin" message should make it into this function.
  // The model is that the arGraphicsPeer sends these directly to connected
  // peers, instead of automatically sending them through alter(...).
  if (data->getID() == _gfx.AR_GRAPHICS_ADMIN){
    return &_rootNode;
  }
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
  int routingFieldID = _getRoutingFieldID(dataID);
  int workingNodeFieldID = _getWorkingFieldID(dataID);
  int* IDPtr = (int*)data->getDataPtr(routingFieldID, AR_INT);
  // Determine the socket ID upon which the message originated (this will
  // be -1 if it was generated locally). If it has come across
  // a communications link then it will be subect to "mapping" (i.e. having the
  // ID of the target node changed) 
  int originID = _getOriginSocketID(data, routingFieldID);

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
    potentialNewNodeID = filterIncoming(connectionIter->second->rootMapNode,
                                        data,
					connectionIter->second->inMap,
					filterIDs,
					&connectionIter->second->outFilter,
					connectionIter->second->sendLevel,
                                        false);
  }

  // Check to see if the node is locked by a source other than the origin of
  // this communication. If so, go ahead and return. Note that the
  // filterIncoming might have changed this ID. 
  // How do locks influence the "structure messages"... namely 
  // make node, insert, erase, cut, permute? The identity of their routing
  // field determines this.
  // make node: the parent node ID.
  // insert: the parent node ID.
  // erase: the ID of the node to be erased.
  // cut: the ID of the node to be cut.
  // permute: the parent ID of the sibling nodes to be permuted.
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
  // OCCUR BEFORE PASSING THE DATA ON TO OTHER PEERS. THE LOCAL PEER
  // IS ALLOWED TO MODIFY THE RECORD "IN PLACE" (AS IN BEING ABLE TO
  // ATTACH A NODE TO THE DATABASE THAT WAS CREATED SOME OTHER WAY).
  // Node creation alters the record in place! See how arDatabase::_makeNode
  // works... (specifically, an ID is assigned to the node, whereby the
  // original message might just have asked for an arbitrary ID).
  if (_localDatabase){
    // If we are actually going to the local database, go ahead and get the
    // result from that...
    // If the message is NOT from a connection, potentialNewNodeID = -1
    // If the message is from a connection and is unmapped, 
    // potentialNewNodeID = 0
    // If the message is from a connection and creates a new node, 
    // potentialNewNodeID > 0
    if (potentialNewNodeID){ 
      result = arGraphicsDatabase::alter(data);
      // The "bridge database" is a way to integrate an arGraphicsPeer into
      // a distributed scene graph application.
      if (_bridgeDatabase){
        _sendDataToBridge(data);
      } 
    }
    // If a new node has been created, we need to augment the node
    // map for this connection. Note that we'll only enter this condition
    // if the message came from elsewhere (and consequently connectionIter
    // will actually point to something).
    if (potentialNewNodeID > 0 && result){
      connectionIter->second->inMap.insert
        (map<int, int, less<int> >::value_type
      	  (potentialNewNodeID, result->getID()));
      // Don't forget to put this in the filterIDs.
      // Sometimes there can actually be TWO pairs added to the node map,
      // so we need to test for this.
      if (filterIDs[1] == -1){
        filterIDs[1] = result->getID();
      }
      else{
        filterIDs[3] = result->getID();
      }
      // Finally, don't forget to activate the outFilter on this node!
      _insertOutFilter(connectionIter->second->outFilter, result->getID(),
		       connectionIter->second->sendLevel);
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
    // in which case, we need to send back the *inverse* node map update.
    if (connectionIter->second->connectionID != originID){
      // Better check whether or not this is a "transient" node. If so,
      // (and the update time has NOT passed), we might do nothing.
      bool updateNodeEvenIfTransient = true;
      if (result && result->getNodeLevel() == AR_TRANSIENT_NODE){
        currentTime = ar_time();
        updateNodeEvenIfTransient 
          = _updateTransientMap(result->getID(),
                                connectionIter->second->transientMap,
				connectionIter->second->remoteFrameTime);
      }
     
      // Still need to check the connection's filter. And augment it
      // for node creation. We've already augmented the node maps for the
      // incoming connection... but now we need to alter the node maps for
      // the outgoing connection(s) as well!
      if (dataID == _gfx.AR_MAKE_NODE || dataID == _gfx.AR_INSERT){
	// The parentID is stored in the routing field of both message types.
        int parentID = data->getDataInt(routingFieldID);
        outIter = connectionIter->second->outFilter.find(parentID);
	// Note how we only update if our parent is currently mapped.
	// (or our parent is the root node, which is never mapped... see how
	// we always put an outFilter entry for the root node in the
	// connection constructor).
	// This makes sense. We should not start mapping updates from
	// "unmapped" parts of the scene graph.

	// The ID of the created node is stored in the working field of both
	// message types.
	if (outIter != connectionIter->second->outFilter.end()){
          _insertOutFilter(connectionIter->second->outFilter, 
                           data->getDataInt(workingNodeFieldID),
		           connectionIter->second->sendLevel);
	}
      }
      // We now determine whether the outbound filtering allows
      // us to relay our message. The working node field corresponds to the
      // new node ID (for make node or insert)
      IDPtr = (int*) data->getDataPtr(workingNodeFieldID, AR_INT);
      outIter = connectionIter->second->outFilter.find(IDPtr[0]);
      // PLEASE NOTE: When a node gets created, it should have a node
      // level of AR_STRUCTURE_NODE (which means that the "make node"
      // message will be transmitted through any connection send level
      // EXCEPT for AR_IGNORE_NODE). 
      // Similarly, successful erase and cut messages have an implicit
      // level of AR_STRUCTURE_NODE (which may be different from the
      // node level of their formerly existing nodes). This is because
      // the database's root node (which is returned on success) has
      // a node level of AR_STRUCTURE_NODE.
      // Also, note how we do not send the message on if
      // arGraphicsDatabase::alter failed locally (i.e. result == NULL).
      if (updateNodeEvenIfTransient && 
          outIter != connectionIter->second->outFilter.end() &&
          result && result->getNodeLevel() <= outIter->second){
        _dataServer->sendData(data, connectionIter->second->socket);
      }
    }
    else{
      // The sender's node map needs to be augmented exactly when the
      // filterIDs array has been modified. (see filterIncoming above)
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

bool arGraphicsPeer::attach(arDatabaseNode* parent,
			    const string& fileName,
			    const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::attach(parent, fileName, _readWritePath);
  }
  return arGraphicsDatabase::attach(parent, fileName, path);
}

bool arGraphicsPeer::attachXML(arDatabaseNode* parent,
			       const string& fileName,
			       const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::attachXML(parent, fileName, _readWritePath);
  }
  return arGraphicsDatabase::attachXML(parent, fileName, path);
}

bool arGraphicsPeer::merge(arDatabaseNode* parent,
			   const string& fileName,
			   const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::merge(parent, fileName, _readWritePath);
  }
  return arGraphicsDatabase::merge(parent, fileName, path);
}

bool arGraphicsPeer::mergeXML(arDatabaseNode* parent,
			      const string& fileName,
			      const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::mergeXML(parent, fileName, _readWritePath);
  }
  return arGraphicsDatabase::mergeXML(parent, fileName, path);
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

bool arGraphicsPeer::writeRooted(arDatabaseNode* parent,
                                 const string& fileName, 
                                 const string& path){
  if (_readWritePath != "" && path == ""){
    return arGraphicsDatabase::writeRooted(parent,
                                           fileName, 
                                           _readWritePath);
  }  
  return arGraphicsDatabase::writeRooted(parent, fileName, path);
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
  // PLEASE NOTE: Previously, we were using arSZGClient's 
  // creatComplexServiceName method, but no longer!
  arPhleetAddress result 
    = _client->discoverService(
         "szg-rp-"+name,
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
/// The graphics peers must request actions.
bool arGraphicsPeer::pullSerial(const string& name,
                                int remoteRootID,
				int localRootID,
				arNodeLevel sendLevel,
				arNodeLevel remoteSendLevel,
                                arNodeLevel localSendLevel){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "pull_serial");
  int nodeIDs[5];
  nodeIDs[0] = remoteRootID;
  nodeIDs[1] = localRootID;
  nodeIDs[2] = sendLevel;
  nodeIDs[3] = remoteSendLevel;
  nodeIDs[4] = localSendLevel;
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, nodeIDs, AR_INT, 5);

  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }

  // We have to wait for the serialization to be completed
  ar_mutex_lock(&_dumpLock);
  _dumped = false;
  _dataServer->sendData(&adminData, socket);
  // Block until we are done receiving data (the remote peer signals us that
  // it is finished.)
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
                                int localRootID,
				arNodeLevel sendLevel,
                                arNodeLevel remoteSendLevel,
                                arNodeLevel localSendLevel){
  int ID = _dataServer->getFirstIDWithLabel(name);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  
  // NOTE: this call does not send a serialization-done notification.
  return _serializeAndSend(socket, remoteRootID, localRootID, sendLevel,
                           remoteSendLevel, localSendLevel);
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
bool arGraphicsPeer::remoteLockNode(const string& name, int nodeID){
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

/// Sometimes, we want a node on a remote peer to only change according to
/// our actions. (this is useful if several peers are connected to a single
/// remote peer and are changing the same things). This differs from 
/// remoteLockNode in that every node (at and including the given node)
/// is locked to us on the remote peer.
bool arGraphicsPeer::remoteLockNodeBelow(const string& name, int nodeID){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "lock_below");
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
bool arGraphicsPeer::remoteUnlockNode(const string& name, int nodeID){
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

/// Of course, we want to be able to unlock the node as well.
/// Why is the connection name present? Because we are unlocking the node
/// on THAT peer (i.e. the one at the other end of the named connection).
/// This differs from remoteUnlockNode in working recursively downwards.
bool arGraphicsPeer::remoteUnlockNodeBelow(const string& name, int nodeID){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "unlock_below");
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
bool arGraphicsPeer::localLockNode(const string& name, int nodeID){
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
bool arGraphicsPeer::localUnlockNode(int nodeID){
  _unlockNode(nodeID);
  // REALLY LAME THAT THIS RETURNS TRUE NO MATTER WHAT!
  return true;
}

/// We want to be able to prevent a remote peer from sending us data (or
/// turn sending back on once we have turned it off).
bool arGraphicsPeer::remoteFilterDataBelow(const string& peer,
                                           int remoteNodeID,
                                           arNodeLevel level){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "filter_data");
  int data[2];
  data[0] = remoteNodeID;
  data[1] = level;
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, data, AR_INT, 2);
  int ID = _dataServer->getFirstIDWithLabel(peer);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return false;
  }
  _dataServer->sendData(&adminData, socket);
  return true;
}

/// We want to change how people are sending data to us on a particular
/// subtree.
bool arGraphicsPeer::mappedFilterDataBelow(int localNodeID,
                                           arNodeLevel level){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "mapped_filter_data");
  int data[2];
  data[0] = localNodeID;
  data[1] = level;
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, data, AR_INT, 2);
  // Gets sent to everybody!
  _dataServer->sendData(&adminData);
  return true;
}

/// Keep data from being relayed to a remote peer (or start relaying the
/// data again).
bool arGraphicsPeer::localFilterDataBelow(const string& peer,
                                          int localNodeID,
                                          arNodeLevel level){
  int ID = _dataServer->getFirstIDWithLabel(peer);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    cout << "arGraphicsPeer error: no such connection.\n";
    return false;
  }
  // Lame that this doesn't complain if no such node.
  _filterDataBelow(localNodeID, socket, level);
  return true;
}

/// Sometimes, we want to be able to find the ID of the node on a remotely
/// connected peer.
int arGraphicsPeer::remoteNodeID(const string& peer,
				 const string& nodeName){
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "ID-request");
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_NAME, nodeName);
  int ID = _dataServer->getFirstIDWithLabel(peer);
  arSocket* socket = _dataServer->getConnectedSocket(ID);
  if (!socket){
    return -1;
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

string arGraphicsPeer::printPeer(){
  stringstream result;
  ar_mutex_lock(&_alterLock);
  printStructure(100, result);
  ar_mutex_unlock(&_alterLock);
  return result.str();
}

/// THIS IS A TEMPORARY HACK... eventually will do something more general!
void arGraphicsPeer::motionCull(arGraphicsPeerCullObject* cull,
				arCamera* camera){
  stack<arMatrix4> transformStack;
  transformStack.push(camera->getModelviewMatrix());
  // AARGH! look at the coarse-grained locking!
  //ar_mutex_lock(&_eraseLock);
  arMatrix4 temp = camera->getProjectionMatrix();
  _motionCull((arGraphicsNode*)&_rootNode, 
              transformStack, 
	      cull,
              temp);
  //ar_mutex_unlock(&_eraseLock);
}

// Returns the ID of the socket upon which this message originated
// (if it wasn't local) and -1 (if it was local).
int arGraphicsPeer::_getOriginSocketID(arStructuredData* data, int fieldID){
  int dim = data->getDataDimension(fieldID);
  if (dim < 2){
    // local
    return -1;
  }
  return ((int*)data->getDataPtr(fieldID, AR_INT))[dim-1];
}

int arGraphicsPeer::_getRoutingFieldID(int dataID){
  if (!_databaseReceive[dataID]){
    return _routingField[dataID];
  }
  else if (dataID == _gfx.AR_MAKE_NODE){
    return _gfx.AR_MAKE_NODE_PARENT_ID;
  }
  else if (dataID == _gfx.AR_ERASE){
    return _gfx.AR_ERASE_ID;
  }
  else if (dataID == _gfx.AR_INSERT){
    return _gfx.AR_INSERT_PARENT_ID;
  }
  else if (dataID == _gfx.AR_CUT){
    return _gfx.AR_CUT_ID;
  }
  else if (dataID == _gfx.AR_PERMUTE){
    return _gfx.AR_PERMUTE_PARENT_ID;
  }
  cout << "arGraphicsPeer error: invalid data record ID "
       << "(_getRoutingFieldID).\n";
  return -1;
}

int arGraphicsPeer::_getWorkingFieldID(int dataID){
  if (!_databaseReceive[dataID]){
    return _routingField[dataID];
  }
  else if (dataID == _gfx.AR_MAKE_NODE){
    return _gfx.AR_MAKE_NODE_ID;
  }
  else if (dataID == _gfx.AR_ERASE){
    return _gfx.AR_ERASE_ID;
  }
  else if (dataID == _gfx.AR_INSERT){
    return _gfx.AR_INSERT_ID;
  }
  else if (dataID == _gfx.AR_CUT){
    return _gfx.AR_CUT_ID;
  }
  else if (dataID == _gfx.AR_PERMUTE){
    return _gfx.AR_PERMUTE_PARENT_ID;
  }
  cout << "arGraphicsPeer error: invalid data record ID "
       << "(_getWorkingFieldID).\n";
  return -1;
}

void arGraphicsPeer::_motionCull(arGraphicsNode* node, 
			         stack<arMatrix4>& transformStack,
				 arGraphicsPeerCullObject* cull,
			         arMatrix4& projectionCullMatrix){
  arMatrix4 tempMatrix;
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    // Push current onto the matrix stack.
    glGetFloatv(GL_MODELVIEW_MATRIX, tempMatrix.v);
    transformStack.push(transformStack.top()
                        *((arTransformNode*)node)->getTransform());
  }
  // Deal with view frustum culling.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE){
    arBoundingSphere b = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arMatrix4 temp = projectionCullMatrix*transformStack.top();
    if (!b.intersectViewFrustum(temp)){
      cull->insert(node->getID(),0);
    }
    else{
      cull->insert(node->getID(),1);
    }
    // DO NOT DRAW CHILDREN EITHER WAY! THIS IS DEFINITELY A HACK!
    // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
    return;
  }
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _motionCull((arGraphicsNode*)(*i), transformStack, 
		cull, projectionCullMatrix);
  }

  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    // Pop from stack.
    transformStack.pop();
  }
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
/// THIS IS REALLY THE *MAP* FUNCTION!
bool arGraphicsPeer::_serializeAndSend(arSocket* socket, 
                                       int remoteRootID,
                                       int localRootID,
                                       arNodeLevel sendLevel,
                                       arNodeLevel remoteSendLevel,
                                       arNodeLevel localSendLevel){
  // First thing to do is to send the "map" command. This tells the
  // remote peer how to map our stuff AND whether or not we should be 
  // receiving data back from the mapped nodes.
  arStructuredData adminData(_gfx.find("graphics admin"));
  adminData.dataInString(_gfx.AR_GRAPHICS_ADMIN_ACTION, "map");
  int flags[2];
  flags[0] = remoteRootID;
  flags[1] = remoteSendLevel;
  adminData.dataIn(_gfx.AR_GRAPHICS_ADMIN_NODE_ID, flags, AR_INT, 2);
  if (!_dataServer->sendData(&adminData, socket)){
    return false;
  }
  // EXPERIMENTING WITH BACKGROUND SERIALIZATION!
  //ar_mutex_lock(&_alterLock);
  arStructuredData nodeData(_gfx.find("make node"));
  if (!nodeData){
    cerr << "arGraphicsPeer error: failed to create node record.\n";
    ar_mutex_unlock(&_alterLock);
    return false;
  }
  bool success = true;
  // The local root node.
  arDatabaseNode* pNode = getNode(localRootID);
  // The connection information... since we need to augment the
  // outgoing filter.
  map<int, arGraphicsPeerConnection*, less<int> >::iterator
    connectionIter = _connectionContainer.find(socket->getID());
  if (!pNode || connectionIter == _connectionContainer.end()){
    cout << "arGraphicsPeer error: failed to get local node or connection.\n";
  }
  else{
    int dataSent = 0;
    // Important that we set the local send level (which will be the default 
    // way updates are subsequently sent). 
    connectionIter->second->sendLevel = localSendLevel;
    // This actually does the work of sending.
    _recSerialize(pNode, nodeData, socket, connectionIter->second->outFilter,
                  localSendLevel, sendLevel, dataSent, success);
  }
  // EXPERIMENTING WITH BACKGROUND SERIALIZATION.
  //ar_mutex_unlock(&_alterLock);

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

void arGraphicsPeer::_closeConnection(arSocket* socket){
  // Disconnect funtion will be called (implicitly) by remove connection.
  // All the lock removal, _connectionContainer updating, etc. is handled
  // there. 
  _dataServer->removeConnection(socket->getID());
}

void arGraphicsPeer::_resetConnectionMap(int connectionID, int nodeID,
                                         arNodeLevel sendLevel){
  ar_mutex_lock(&_socketsLock);
  map<int, arGraphicsPeerConnection*, less<int> >::iterator i
    = _connectionContainer.find(connectionID);
  if ( i == _connectionContainer.end()){
    cout << "arGraphicsPeer internal error: missed connection when trying "
	 << "to reset map.\n";
  }
  else{
    // DO NOT RESET THE outFilter HERE! THIS ALLOWS US TO MAP MULTIPLE TIMES
    // ON A SINGLE CONNECTION!
    // DO NOT RESET THE inMap HERE! FOR THE SAME REASON
    //i->second->inMap.clear();
    arDatabaseNode* newRootNode = getNode(nodeID);
    if (!newRootNode){
      cout << "arGraphicsPeer error: connection map root node not present. "
	   << "Using default.(ID = " << nodeID << ")\n";
      i->second->rootMapNode = &_rootNode;
    } 
    else{
      i->second->rootMapNode = newRootNode;
    }
    i->second->sendLevel = sendLevel;
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

void arGraphicsPeer::_lockNodeBelow(int nodeID, arSocket* socket){
  arDatabaseNode* node = getNode(nodeID);
  if (!node){
    return;
  }
  _lockNode(nodeID, socket);
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _lockNodeBelow((*i)->getID(), socket);
  }
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

void arGraphicsPeer::_unlockNodeBelow(int nodeID){
  arDatabaseNode* node = getNode(nodeID);
  if (!node){
    return;
  }
  _unlockNode(nodeID);
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _unlockNodeBelow((*i)->getID());
  }
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
                                      arNodeLevel level){
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
    _recDataOnOff(pNode, level, i->second->outFilter);
  }
  ar_mutex_unlock(&_socketsLock);
}

void arGraphicsPeer::_recSerialize(arDatabaseNode* pNode,
                                   arStructuredData& nodeData,
                                   arSocket* socket,
				   map<int,int,less<int> >& outFilter,
				   arNodeLevel localSendLevel,
                                   arNodeLevel sendLevel,
				   int& dataSent,
                                   bool& success){
  // EXPERIMENTING WITH DUMPING IN THE BACKGROUND!
  ar_mutex_lock(&_alterLock);
  // This will fail for the root node
  if (fillNodeData(&nodeData, pNode)){
    dataSent += nodeData.size();
    if (!_dataServer->sendData(&nodeData, socket)){
      success = false;
    }
    if (sendLevel >= pNode->getNodeLevel()){
      arStructuredData* theData = pNode->dumpData();
      dataSent += theData->size();
      if (!_dataServer->sendData(theData, socket)){
        success = false;
      }
      delete theData;
    }
  }
  _insertOutFilter(outFilter, pNode->getID(), localSendLevel);
  list<arDatabaseNode*> children = pNode->getChildren();
  ar_mutex_unlock(&_alterLock);
  // NOTE: We are CAPPING the serialize rate at about:
  // 100 x (10000x8) = 8 Mbps here. Hopefully, this allows for
  // reasonable connect behavior.
  if (dataSent > 10000){
    ar_usleep(10000);
    dataSent = 0;
  }
  list<arDatabaseNode*>::iterator i;
  for (i=children.begin(); i!=children.end(); i++){
    if (success){
      _recSerialize(*i, nodeData, socket, outFilter, localSendLevel,
                    sendLevel, dataSent, success);
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
  // NOTE: The AR_IGNORE_NODE parameter is ignored since there is no outFilter
  // parameter.
  int potentialNewNodeID 
    = _bridgeDatabase->filterIncoming(_bridgeRootMapNode,
                                      data,
				      _bridgeInMap,
				      NULL,
				      NULL,
				      AR_IGNORE_NODE,
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

bool arGraphicsPeer::_updateTransientMap(int nodeID,
		  map<int, arGraphicsPeerUpdateInfo,less<int> >& transientMap,
                  int remoteFrameTime){
  ar_timeval currentTime = ar_time();
  map<int, arGraphicsPeerUpdateInfo,less<int> >::iterator i 
    = transientMap.find(nodeID);
  if (i == transientMap.end()){
    arGraphicsPeerUpdateInfo temp;
    transientMap.insert
      (map<int, arGraphicsPeerUpdateInfo,less<int> >::value_type
        (nodeID, temp));
    i = transientMap.find(nodeID);
  }  
  if (i->second.invalidUpdateTime
      || ar_difftime(currentTime,i->second.lastUpdate) > remoteFrameTime){
    i->second.invalidUpdateTime = false;
    i->second.lastUpdate = currentTime;
    return true;
  }
  return false;
}


