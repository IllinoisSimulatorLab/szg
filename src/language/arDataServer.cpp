//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataServer.h"
#include "arLogStream.h"

// Allocating the listening socket in the constructor may prevent
// arDataServer from being declared as a global in win32.  Sigh.
arDataServer::arDataServer(int dataBufferSize) :
  arDataPoint(dataBufferSize),
  _interfaceIP(string("INADDR_ANY")), // the default
  _portNumber(-1),
  _listeningSocket(NULL),
  _numberConnected(0),
  _numberConnectedActive(0),
  _nextID(0),
  _dataParser(NULL),
  _consumerFunction(NULL),
  _consumerObject(NULL),
  _disconnectFunction(NULL),
  _disconnectObject(NULL),
  _atomicReceive(true)
{
}

arDataServer::~arDataServer(){
  // Close all connections.
  if (_numberConnected > 0){
    ar_log_remark() << "arDataServer destructor closing sockets.\n";
    for (list<arSocket*>::iterator i(_connectionSockets.begin());
         i != (_connectionSockets.end());
         i++){
      arSocket* theSocket = *i;
      theSocket->ar_close();
      delete theSocket;
      --_numberConnected;
    }
  }
  if (_numberConnected > 0)
    ar_log_warning() << "arDataServer destructor confused.\n";
  delete _listeningSocket;
}

void ar_readDataThread(void* dataServer){
  ((arDataServer*)dataServer)->_readDataTask();
}

void arDataServer::_readDataTask(){
  // Bug: chaos ensues if _atomicReceive changes during this thread.  Cache a local copy thereof.
  arSocket* newFD = _nextConsumer;

  // Determine the formatting used by this client's remote thread.
  map<int,arStreamConfig,less<int> >::iterator iter = 
    _connectionConfigs.find(newFD->getID());
  if (iter == _connectionConfigs.end()) {
    cout << "arDataServer error: read thread launched without stream "
	 << "config (socket ID = " << newFD->getID() << ".\n";
    _threadLaunchSignal.sendSignal();
    return;
  }
  arStreamConfig remoteConfig = iter->second;
  _threadLaunchSignal.sendSignal();

  int availableSize = _dataBufferSize;
  ARchar* dest = new ARchar[availableSize];

  // Translation buffer.
  int transSize = availableSize;
  ARchar* transBuffer = new ARchar[transSize];

  while (true) {
    bool fEndianMode = false;
    ARint theSize = -1;
    if (!getDataCore(dest, availableSize, transBuffer, transSize,
                     theSize, fEndianMode, newFD, remoteConfig))
	break;
    if (!fEndianMode) {
      if (!_theDictionary) {
	cerr << "arDataServer error: no dictionary.\n";
	break;
      }
      const ARint recordID = ar_translateInt(transBuffer+AR_INT_SIZE, remoteConfig);
      // Bug? if !_atomicReceive, this still needs to be locked.
      arGuard dummy(_lockConsume);
      arDataTemplate* t = _theDictionary->find(recordID);
      if (!t || t->translate(dest,transBuffer,remoteConfig) <= 0) {
	cerr << "arDataServer warning: failed to translate record.\n";
        break;
      }
    }

    // data is OK
    if (_atomicReceive){
      _lockConsume.lock();
    }
    arStructuredData* inData = _dataParser->parse(dest, theSize);
    if (inData) {
      _consumerFunction(inData,_consumerObject,newFD);
      _dataParser->recycle(inData);
    }
    if (_atomicReceive){
      _lockConsume.unlock();
    }
    if (!inData) {
      cerr << "arDataServer warning: failed to parse record.\n";
      break;
    }
  }

  delete [] dest;
  delete [] transBuffer;
  // NOTE: if we are in "atomic receive" mode, we must also invoke
  // that lock here. Why? The delete socket callback might very well
  // want to to stuff (as in szgserver) that expects to be atomic
  // vis-a-vis the consumer function invoked above
  if (_atomicReceive){
    _lockConsume.lock();
  }
  _lockTransfer.lock();
  _deleteSocketFromDatabase(newFD);
  _lockTransfer.unlock();
  if (_atomicReceive){
    _lockConsume.unlock();
  }
}

void arDataServer::atomicReceive(bool atomicReceive){
  _atomicReceive = atomicReceive;
}

bool arDataServer::setPort(int thePort){
  if (thePort < 1024 || thePort > 50000){
    cerr << "arDataServer warning: ignoring port value " << thePort
	<< ": out of range 1024 to 50000.\n";
    return false;
  }
  _portNumber = thePort;
  return true;
}

bool arDataServer::setInterface(const string& theInterface){
  if (theInterface == "NULL"){
    cerr << "arDataServer warning: ignoring NULL setInterface.\n";
    return false;
  }
  _interfaceIP = theInterface;
  return true;
}

bool arDataServer::beginListening(arTemplateDictionary* theDictionary){
  if (_portNumber == -1){
    cerr << "arDataServer warning: failed to listen on undefined port.\n";
    return false;
  }
  if (!theDictionary){
    cerr << "arDataServer error: no dictionary.\n";
    return false;
  }
  _theDictionary = theDictionary;
  if (!_dataParser){
    _dataParser = new arStructuredDataParser(_theDictionary);
  }
  if (!_listeningSocket){
    _listeningSocket = new arSocket(AR_LISTENING_SOCKET);
  }
  if (_listeningSocket->ar_create() < 0 ||
      !setReceiveBufferSize(_listeningSocket) ||
      !_listeningSocket->reuseAddress(true)) {
    cerr << "arDataServer error: failed to begin listening.\n";
    _listeningSocket->ar_close(); // avoid memory leak
    return false;
  }

  // Pass down the accept mask that implements
  // TCP-wrappers style filtering on clients that try to connect
  _listeningSocket->setAcceptMask(_acceptMask);

  // ar_bind needs C string, not std::string
  char addressBuffer[256];
  ar_stringToBuffer(_interfaceIP, addressBuffer, sizeof(addressBuffer));
  if (_listeningSocket->ar_bind(_interfaceIP == "INADDR_ANY" 
                                   ? NULL : addressBuffer,
                                _portNumber) < 0){
    cerr << "arDataServer warning: failed to bind to "
         << _interfaceIP << ":" << _portNumber
         << "\n\t(maybe " << _interfaceIP
         << " is not this host's address,\n\tor port "
         << _portNumber << " is already in use.)\n";
    _listeningSocket->ar_close(); // avoid memory leak
    return false;
  }

  _listeningSocket->ar_listen(256); 
  return true;
}

bool arDataServer::removeConnection(int id){
  arGuard dummy(_lockTransfer);
  const map<int,arSocket*,less<int> >::iterator i(_connectionIDs.find(id));
  const bool found = (i != _connectionIDs.end());
  if (found)
    _deleteSocketFromDatabase(i->second);
  return found;
}

arSocket* arDataServer::_acceptConnection(bool addToActive){
  if (!_listeningSocket) {
    ar_log_warning() << "arDataServer can't acceptConnection before beginListening.\n";
    return NULL;
  }
  ar_usleep(30000); // Might improve stability.  Probably unnecessary.

  // Accept connections in a different thread from the one sending data.
  arSocket* newSocketFD = new arSocket(AR_STANDARD_SOCKET);
  if (!newSocketFD){
    ar_log_warning() << "arDataServer: no socket in _acceptConnection.\n";
    return NULL;
  }
  arSocketAddress addr;
  if (_listeningSocket->ar_accept(newSocketFD, &addr) < 0) {
    ar_log_warning() << "arDataServer failed to _acceptConnection.\n";
    return NULL;
  }
  ar_log_remark() << "arDataServer connected from "
                  << addr.getRepresentation() << ar_endl;
 
  arGuard dummy(_lockTransfer);
  _addSocketToDatabase(newSocketFD);
  if (!newSocketFD->smallPacketOptimize(_smallPacketOptimize)) {
    ar_log_warning() << "arDataServer failed to smallPacketOptimize.\n";
LAbort:
    return NULL;
  }

  // Add the new socket to either the active or the passive list.
  (addToActive ? _connectionSockets : _passiveSockets).push_back(newSocketFD);

  if (!_theDictionary){
    // We expected to SEND the dictionary to the connected data point.
    ar_log_warning() << "arDataServer: no dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    goto LAbort;
  }

  // Configuration handshake.
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE; // todo: do this line in arStreamConfig's constructor.
  localConfig.ID = newSocketFD->getID();
  arStreamConfig remoteStreamConfig = handshakeConnectTo(newSocketFD, localConfig);
  if (!remoteStreamConfig.valid) {
    string sSymptom;
    const int ver = remoteStreamConfig.version;
    switch (ver) {
    // todo: unify magic numbers -X with arDataPoint::_fillConfig
    case -1:
      sSymptom = "not Syzygy";
      break;
    case -2:
      sSymptom = "no Syzygy version key";
      break;
    case -3:
      sSymptom = "unparseable Syzygy version key";
      break;
    default:
      sSymptom = "wrong Syzygy version " + ar_intToString(ver);
      break;
    }
    ar_log_warning() << "arDataServer rejected connection from " <<
      addr.getRepresentation() << ": " << sSymptom << ".\n";
    _deleteSocketFromDatabase(newSocketFD);
    goto LAbort;
  }
  // We need to know the remote stream config for this socket, since we
  // might send data (i.e. szgserver or arBarrierServer).
  _setSocketRemoteConfig(newSocketFD,remoteStreamConfig);

  // Send the dictionary.
  const int theSize = _theDictionary->size();
  if (theSize<=0){
    cout << "arDataServer error: failed to pack dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    goto LAbort;
  }

  ARchar* buffer = new ARchar[theSize]; // Storage for the dictionary.
  _theDictionary->pack(buffer);
  if (!newSocketFD->ar_safeWrite(buffer,theSize)){
    cerr << "arDataServer error: failed to send dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    goto LAbort;
  }
  delete [] buffer;

  _numberConnected++;
  if (addToActive)
    _numberConnectedActive++;

  if (_consumerFunction){
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = newSocketFD;
    arThread* dummy = new arThread; // memory leak?
    if (!dummy->beginThread(ar_readDataThread, this)){
      cerr << "arDataServer error: failed to start read thread.\n";
      return NULL;
    } 
    // Wait until ar_readDataThread reads _nextConsumer into local storage.
    _threadLaunchSignal.receiveSignal();
  }
 
  return newSocketFD;
}

void arDataServer::activatePassiveSockets(){
  arGuard dummy(_lockTransfer);
  for (list<arSocket*>::const_iterator i(_passiveSockets.begin());
       i != _passiveSockets.end(); ++i){
    _connectionSockets.push_back(*i);
    _numberConnectedActive++;
  }
  _passiveSockets.clear();
}

bool arDataServer::checkPassiveSockets(){
  arGuard dummy(_lockTransfer);
  return _passiveSockets.begin() != _passiveSockets.end();
}

list<arSocket*>* arDataServer::getActiveSockets(){
  list<arSocket*>* result = new list<arSocket*>;
  arGuard dummy(_lockTransfer);
  for (list<arSocket*>::const_iterator i=_connectionSockets.begin();
       i != _connectionSockets.end();
       ++i){
    result->push_back(*i);
  }
  return result;
}

void arDataServer::activatePassiveSocket(int socketID){
  arGuard dummy(_lockTransfer);
  for (list<arSocket*>::iterator i(_passiveSockets.begin());
       i != _passiveSockets.end(); ++i){
    if ((*i)->getID() == socketID){
      _connectionSockets.push_back(*i);
      _passiveSockets.erase(i);
      _numberConnectedActive++;
      break;
    }
  }
}

bool arDataServer::sendData(arStructuredData* pData){
  const int theSize = pData->size();
  arGuard dummy(_lockTransfer);
  return ar_growBuffer(_dataBuffer, _dataBufferSize, theSize) &&
    pData->pack(_dataBuffer) &&
    _sendDataCore(_dataBuffer, theSize);
}

bool arDataServer::sendDataQueue(arQueuedData* pData){
  arGuard dummy(_lockTransfer);
  return _sendDataCore(pData->getFrontBufferRaw(), pData->getFrontBufferSize());
}

// Send data in a different thread from where we accept connections.
// Call this only inside _lockTransfer.
// Return true if any connections.
bool arDataServer::_sendDataCore(ARchar* theBuffer, const int theSize){
  bool ok = false;
  list<arSocket*> removalList;
  list<arSocket*>::iterator iter;
  for (iter = _connectionSockets.begin(); iter != _connectionSockets.end(); ++iter){
    arSocket* fd = *iter;
    if (fd->ar_safeWrite(theBuffer,theSize)){
      ok = true;
    }
    else{
      // Failed to broadcast data.
      removalList.push_back(fd);
    }
  }
  for (iter = removalList.begin(); iter != removalList.end(); ++iter)
    _deleteSocketFromDatabase(*iter);
  return ok;
}

bool arDataServer::sendData(arStructuredData* pData, arSocket* fd){
  arGuard dummy(_lockTransfer);
  return sendDataNoLock(pData, fd);
}

bool arDataServer::sendDataNoLock(arStructuredData* pData, arSocket* fd){
  if (!fd){
    cerr << "arDataServer warning: ignoring data-send to NULL socket.\n";
    return false;
  }
  const int theSize = pData->size();
  if (!ar_growBuffer(_dataBuffer, _dataBufferSize, theSize)) {
    cerr << "arDataServer warning: failed to grow buffer.\n";
    return false;
  }
  pData->pack(_dataBuffer);
  return _sendDataCore(_dataBuffer, theSize, fd);
}

bool arDataServer::sendDataQueue(arQueuedData* p, arSocket* fd){
  if (!fd){
    cerr << "arDataServer warning: ignoring data-queue-send to NULL socket.\n";
    return false;
  }
  arGuard dummy(_lockTransfer);
  return _sendDataCore(p->getFrontBufferRaw(), p->getFrontBufferSize(), fd);
}

// Call this only inside _lockTransfer.
bool arDataServer::_sendDataCore(ARchar* theBuffer, const int theSize, arSocket* fd){
  // Caller ensures that fd != NULL.
  if (fd->ar_safeWrite(theBuffer,theSize))
    return true;
  cerr << "arDataServer warning: failed to send data to specific socket.\n";
  _deleteSocketFromDatabase(fd);
  return false;
}

bool arDataServer::sendDataQueue(arQueuedData* theData, list<arSocket*>* socketList){
  const int theSize = theData->getFrontBufferSize();
  list<arSocket*> removalList;
  bool ok = false;
  ARchar* theBuffer = theData->getFrontBufferRaw();
  list<arSocket*>::iterator i;
  arGuard dummy(_lockTransfer);
  for (i = socketList->begin(); i != socketList->end(); ++i){
    if ((*i)->ar_safeWrite(theBuffer, theSize)){
      ok = true;
    }
    else{
      cerr << "arDataServer warning: failed to send data.\n";
      removalList.push_back(*i);
    }
  }
  for (i = removalList.begin(); i != removalList.end(); i++){
    _deleteSocketFromDatabase(*i);
  }
  return ok;
}

void arDataServer::setConsumerFunction(void (*consumerFunction)
			               (arStructuredData*,void*,arSocket*)){
  _consumerFunction = consumerFunction;
}

void arDataServer::setConsumerObject(void* consumerObject){
  _consumerObject = consumerObject;
}

void arDataServer::setDisconnectFunction
  (void (*disconnectFunction)(void*,arSocket*)){
  _disconnectFunction = disconnectFunction;
}

void arDataServer::setDisconnectObject(void* disconnectObject){
  _disconnectObject = disconnectObject;
}

string arDataServer::dumpConnectionLabels(){
  string s;
  arGuard dummy(_lockTransfer);
  for (map<int,string,less<int> >::const_iterator iLabel(_connectionLabels.begin());
       iLabel != _connectionLabels.end(); ++iLabel){
    s += iLabel->second + "/" + ar_intToString(iLabel->first) + ":";
  }
  return s;
}

arSocket* arDataServer::getConnectedSocket(int id){
  arGuard dummy(_lockTransfer);
  return getConnectedSocketNoLock(id);
}

arSocket* arDataServer::getConnectedSocketNoLock(int id){
  map<int,arSocket*,less<int> >::iterator i(_connectionIDs.find(id));
  return i==_connectionIDs.end() ? NULL : i->second;
}

void arDataServer::setSocketLabel(arSocket* theSocket, const string& theLabel){
  arGuard dummy(_lockTransfer);
  _addSocketLabel(theSocket, theLabel);
}

string arDataServer::getSocketLabel(int theSocketID){
  arGuard dummy(_lockTransfer);
  map<int,string,less<int> >::const_iterator i(_connectionLabels.find(theSocketID));
  return i==_connectionLabels.end() ? string("NULL") : i->second;
}

int arDataServer::getFirstIDWithLabel(const string& theSocketLabel){
  arGuard dummy(_lockTransfer);
  for (map<int,string,less<int> >::const_iterator i = _connectionLabels.begin();
      i != _connectionLabels.end(); ++i) {
    if (theSocketLabel == i->second){
      return i->first;
    }
  }
  return -1;
}

// _lockTransfer serializes this, so 2 sockets don't get the same ID.
void arDataServer::_addSocketToDatabase(arSocket* theSocket){
  theSocket->setID(_nextID++);         // give the socket an ID
  _addSocketLabel(theSocket, "NULL");  // give the socket a default label
  _addSocketID(theSocket);             // insert in the ID table
}

bool arDataServer::_delSocketID(arSocket* s){
  map<int,arSocket*,less<int> >::iterator i(_connectionIDs.find(s->getID()));
  if (i == _connectionIDs.end())
    return false;
  _connectionIDs.erase(i); // Socket already existed.
  return true;
}

bool arDataServer::_delSocketLabel(arSocket* s){
  map<int,string,less<int> >::iterator i(_connectionLabels.find(s->getID()));
  if (i == _connectionLabels.end())
    return false;
  _connectionLabels.erase(i); // Socket already existed.
  return true;
}

void arDataServer::_addSocketID(arSocket* s){
  (void)_delSocketID(s);
  _connectionIDs.insert(
    map<int,arSocket*,less<int> >::value_type(s->getID(), s));
}

void arDataServer::_addSocketLabel(arSocket* s, const string& label){
  (void)_delSocketLabel(s);
  _connectionLabels.insert(
    map<int,string,less<int> >::value_type(s->getID(), label));
}

void arDataServer::_deleteSocketFromDatabase(arSocket* theSocket){
  // we have the following problem with the sockets: since reading
  // and writing occur in seperate threads, we cannot arbitrairly delete the socket
  // in one or another of the threads. It might be the case that the other thread
  // is still actively using that socket. Also, we need to be careful that a socket
  // is not deleted twice. Consequently, a count, per socket, is maintained of the
  // operations pending upon it. This count is incremented and decremented 
  // internally in the socket upon entrance to and exit from
  // ar_safeRead and ar_safeWrite. The _deleteSocketFromDatabase
  // function will, the first time it has been called, delete the socket from the
  // database. Any subsequent times it is called, it will fail to do so (since it has
  // already been deleted from the database). However, the final time (associated with
  // 0 pending socket operations), it will delete the socket.

  // NOTE: haven't implemented the "delete socket if it's usage count is O" yet
  // sockets are just getting closed and deleted the first time _deleteSocketFromDatabase
  // is called on them... however, at least they won't now be deleted twice!

  // MUST FIX

  // Delete the socket from the internal databases.
  if (!_delSocketID(theSocket))
    // Socket wasn't in the ID table, so it was already deleted.
    return;

  // Delete the socket from the label table.
  if (!_delSocketLabel(theSocket))
    cerr << "arDataServer warning: internal socket databases are inconsistent.\n";

  // Remove the socket*.
  for (list<arSocket*>::iterator removalIterator(_connectionSockets.begin());
       removalIterator != _connectionSockets.end();
       ++removalIterator){
    if (theSocket->getID() == (*removalIterator)->getID()){
      _connectionSockets.erase(removalIterator);
      theSocket->ar_close(); // good idea
      --_numberConnected;
      --_numberConnectedActive;
      break; // Stop looking.
    }
  }
  if (_disconnectFunction){
    // Call the user-supplied disconnect function.
    _disconnectFunction(_disconnectObject, theSocket);
  }

  // Memory leak, about 20 bytes per connection:
  // theSocket isn't deleted, because multiple threads might
  // be using it at once.  If we could delete it, we'd do so here
  // after _disconnectFunction has used it, of course.
}

void arDataServer::_setSocketRemoteConfig(arSocket* theSocket, 
                                          const arStreamConfig& config){
  map<int,arStreamConfig,less<int> >::iterator
    iter(_connectionConfigs.find(theSocket->getID()));
  if (iter != _connectionConfigs.end()){
    cerr << "arDataServer warning: erasing duplicate socket ID.\n";
    _connectionConfigs.erase(iter);
  }
  _connectionConfigs.insert
    (map<int,arStreamConfig,less<int> >::value_type 
      (theSocket->getID(), config));
}

// -1 is returned on error. Otherwise the ID of the new socket.
// THIS IS VERY BAD COPY-PASTE FROM arDataClient. PLEASE LEAVE IT
// UNTIL IT GETS CLEANED-UP.
int arDataServer::dialUpFallThrough(const string& s, int port){
  arSocket* socket = new arSocket(AR_STANDARD_SOCKET);
  if (socket->ar_create() < 0) {
    ar_log_warning() << "arDataServer: dialUp(" << s << ":" << port
         << ") failed to create socket.\n";
    return -1;
  }
  if (!setReceiveBufferSize(socket)){
    return -1;
  }
  if (!socket->smallPacketOptimize(_smallPacketOptimize)){
    ar_log_warning() << "arDataServer: dialUp(" << s << ":" << port
         << ") failed to smallPacketOptimize.\n";
    return -1;
  }
  arSocketAddress addr;
  if (socket->ar_connect(s.c_str(), port) < 0){
    cerr << "arDataServer error: dialUp failed.\n";
    socket->ar_close();
    // delete socket?
    return -1;
  }

  // Set up communications.
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE;
  // BUG: localConfig.ID here is NOT the socket ID... but the arDataServer doesn't
  // use it yet.  This breaks symmetry, but no code depends on it yet.
  localConfig.ID = 0;
  arStreamConfig remoteStreamConfig = handshakeReceiveConnection(socket, localConfig);
  if (!remoteStreamConfig.valid){
    if (remoteStreamConfig.refused){
      ar_log_remark() << "arDataServer: remote data point disconnected.\n"
	   << "  (Maybe this IP address isn't on the szgserver's whitelist.)\n";
      return false;
    }
    ar_log_warning() << "arDataServer: remote data point has wrong szg protocol version "
         << remoteStreamConfig.version << ".\n";
    return false;
  }
  
  ARchar sizeBuffer[AR_INT_SIZE];
  // arDataServer doesn't actually use sizeBuffer (as opposed to arDataClient).
  // In arDataClient, a statement would go here storing the remote socket ID.
  if (!socket->ar_safeRead(sizeBuffer,AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to get dictionary size.\n";
    socket->ar_close();
    return -1;
  }
  const ARint totalSize = ar_translateInt(sizeBuffer,remoteStreamConfig);
  if (totalSize < AR_INT_SIZE){
    ar_log_warning() << "arDataServer: dialUp failed to translate dictionary.\n";
    socket->ar_close();
    return -1;
  }

  ARchar* dataBuffer = new ARchar[totalSize];
  memcpy(dataBuffer, sizeBuffer, AR_INT_SIZE);
  if (!socket->ar_safeRead(dataBuffer+AR_INT_SIZE, totalSize-AR_INT_SIZE)){
    ar_log_warning() << "arDataServer: dialUp got no dictionary.\n";
    delete [] dataBuffer;
    return -1;
  }

  // Success!
  delete [] dataBuffer;

  arGuard dummy(_lockTransfer);
  // Add this to the list of active connections.
  _connectionSockets.push_back(socket);
  _addSocketToDatabase(socket);
  _setSocketRemoteConfig(socket, remoteStreamConfig);
  // Copypaste from accept connection in this object.
  _numberConnected++;

  // Only meaningful for sync sockets
  //if (addToActive)
  //  _numberConnectedActive++;

  if (_consumerFunction){
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = socket;
    arThread* dummy = new arThread; // \bug memory leak?
    if (!dummy->beginThread(ar_readDataThread, this)){
      cerr << "arDataServer error: failed to start read thread.\n";
      return -1;
    } 
    // Wait until the new thread reads _nextConsumer into local storage.
    _threadLaunchSignal.receiveSignal();
  }
  return socket->getID();
}
