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
  ar_mutex_init(&_dataTransferMutex);
  ar_mutex_init(&_consumptionLock);
}

arDataServer::~arDataServer(){
  // close all current connections
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

  // Allocate a buffer for this thread.
  int availableSize = _dataBufferSize;
  ARchar* dest = new ARchar[availableSize];

  // Local translation buffer.  Neither buffer can be shared with other read threads.
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
      const ARint recordID =
	ar_translateInt(transBuffer+AR_INT_SIZE, remoteConfig);
      // HMMMM!!! THIS SEEMS A LITTLE SUSPICIOUS! WHAT IF WE ARE NOT IN
      // _atomicReceive mode?? This would still need to be locked
      // vis-a-vis other threads!
      ar_mutex_lock(&_consumptionLock);
      arDataTemplate* theTemplate = _theDictionary->find(recordID);
      const bool ok = theTemplate &&
        (theTemplate->translate(dest,transBuffer,remoteConfig) > 0);
      ar_mutex_unlock(&_consumptionLock);
      if (!ok) {
	cerr << "arDataServer warning: failed to translate record.\n";
        break;
      }
    }

    // data is OK
    if (_atomicReceive){
      ar_mutex_lock(&_consumptionLock);
    }
    arStructuredData* inData = _dataParser->parse(dest, theSize);
    if (inData) {
      _consumerFunction(inData,_consumerObject,newFD);
      _dataParser->recycle(inData);
    }
    if (_atomicReceive){
      ar_mutex_unlock(&_consumptionLock);
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
    ar_mutex_lock(&_consumptionLock);
  }
  ar_mutex_lock(&_dataTransferMutex);
  _deleteSocketFromDatabase(newFD);
  ar_mutex_unlock(&_dataTransferMutex);
  if (_atomicReceive){
    ar_mutex_unlock(&_consumptionLock);
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
  // it is possible that beginListening will be called again and again
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

  // don't forget to pass down the accept mask that let's us do 
  // TCP-wrappers style filtering on clients that try to connect
  _listeningSocket->setAcceptMask(_acceptMask);

  // ar_bind needs null-terminated C-style string instead of C++ string
  char addressBuffer[256];
  ar_stringToBuffer(_interfaceIP, addressBuffer, sizeof(addressBuffer));
  if (_listeningSocket->ar_bind(_interfaceIP == "INADDR_ANY" 
                                   ? NULL : addressBuffer,
                                _portNumber) < 0){
    cerr << "arDataServer error: failed to bind to "
         << _interfaceIP << ":" << _portNumber
         << "\n\t(maybe " << _interfaceIP
         << " is not this host's address,\n\tor port "
         << _portNumber << " is already in use.)\n";
    _listeningSocket->ar_close(); // avoid memory leak
    return false;
  }

  // Silence means it worked:  all "return false" above print a diagnostic.
  //if (_interfaceIP != "NULL" /* less noisy */ ){
  //  cout << "arDataServer remark: bound to " 
  //	 << _interfaceIP
  //	 << ":" << _portNumber << ".\n";
  //  }
  _listeningSocket->ar_listen(256); 
  return true;
}

bool arDataServer::removeConnection(int id){
  ar_mutex_lock(&_dataTransferMutex);
  const map<int,arSocket*,less<int> >::iterator i(_connectionIDs.find(id));
  const bool found = (i != _connectionIDs.end());
  if (found)
    _deleteSocketFromDatabase(i->second);
  ar_mutex_unlock(&_dataTransferMutex);
  return found;
}

arSocket* arDataServer::_acceptConnection(bool addToActive){
  // At one time, I thought that stability would be improved if the
  // rate at which a server could accept connections was throttled.
  // HOWEVER, the instability I was battling at the time was NOT due to
  // syzygy but instead to a non-thread-safe standard C++ lib.
  // It would be a good idea to experiment with removing this at some point.
  ar_usleep(30000);

  // we need to be able to accept connections in a different thread
  // than where we send data
  arSocket* newSocketFD = new arSocket(AR_STANDARD_SOCKET);
  if (!newSocketFD){
    cerr << "arDataServer error: no socket in _acceptConnection.\n";
    return NULL;
  }
  if (_listeningSocket->ar_accept(newSocketFD) < 0) {
    cerr << "arDataServer error: failed to _acceptConnection.\n";
    return NULL;
  }
  
  ar_mutex_lock(&_dataTransferMutex);
  _addSocketToDatabase(newSocketFD);
  if (!newSocketFD->smallPacketOptimize(_smallPacketOptimize)) {
    cerr << "arDataServer error: failed to smallPacketOptimize.\n";
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }

  // Based on the connection-acceptance state, we either add the new
  // socket to the active list or the passive list
  (addToActive ? _connectionSockets : _passiveSockets).push_back(newSocketFD);

  // If there is no dictionary, abort! (we expect to SEND the dictionary to
  // the connected data point)
  if (!_theDictionary){
    cout << "arDataServer error: no dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }

  // Configuration handshake exchange.
  arStreamConfig remoteStreamConfig;
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE;
  localConfig.ID = newSocketFD->getID();
  remoteStreamConfig = handshakeConnectTo(newSocketFD, localConfig);
  if (!remoteStreamConfig.valid){
    cout << "arDataServer error: remote data point has wrong szg protocol "
	 << "version = " << remoteStreamConfig.version << ".\n";
    _deleteSocketFromDatabase(newSocketFD);
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }
  // We need to know the remote stream config for this socket, since we
  // might be sending data (i.e. szgserver or arBarrierServer).
  _setSocketRemoteConfig(newSocketFD,remoteStreamConfig);

  // Send the dictionary.
  const int theSize = _theDictionary->size();
  if (theSize<=0){
    cout << "arDataServer error: could not pack dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }
  ARchar* buffer = new ARchar[theSize];
  _theDictionary->pack(buffer);
  if (!newSocketFD->ar_safeWrite(buffer,theSize)){
    cerr << "arDataServer error: failed to send dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }
  // We can delete the storage for the dictionary.
  delete [] buffer;

  // Based on the connection-acceptance state, we might or might
  // not need to increment the number of active connections.
  _numberConnected++;
  if (addToActive)
    _numberConnectedActive++;

  if (_consumerFunction){
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = newSocketFD;
    arThread* dummy = new arThread; // \bug memory leak?
    if (!dummy->beginThread(ar_readDataThread, this)){
      cerr << "arDataServer error: failed to start read thread.\n";
      return NULL;
    } 
    // Wait until the new thread reads _nextConsumer into local storage.
    _threadLaunchSignal.receiveSignal();
  }
 
  ar_mutex_unlock(&_dataTransferMutex);
  return newSocketFD;
}

void arDataServer::activatePassiveSockets(){
  ar_mutex_lock(&_dataTransferMutex);
  for (list<arSocket*>::iterator i(_passiveSockets.begin());
       i != _passiveSockets.end();
       ++i){
    _connectionSockets.push_back(*i);
    _numberConnectedActive++;
  }
  _passiveSockets.clear();
  ar_mutex_unlock(&_dataTransferMutex);
}

bool arDataServer::checkPassiveSockets(){
  ar_mutex_lock(&_dataTransferMutex);
    const bool ok = _passiveSockets.begin() != _passiveSockets.end();
  ar_mutex_unlock(&_dataTransferMutex);
  return ok;
}

list<arSocket*>* arDataServer::getActiveSockets(){
  list<arSocket*>* result = new list<arSocket*>;
  ar_mutex_lock(&_dataTransferMutex);
    for (list<arSocket*>::iterator i=_connectionSockets.begin();
	 i != _connectionSockets.end();
	 ++i){
      result->push_back(*i);
    }
  ar_mutex_unlock(&_dataTransferMutex);
  return result;
}

void arDataServer::activatePassiveSocket(int socketID){
  ar_mutex_lock(&_dataTransferMutex);
    for (list<arSocket*>::iterator i(_passiveSockets.begin());
	 i != _passiveSockets.end();
	 ++i){
      if ((*i)->getID() == socketID){
	_connectionSockets.push_back(*i);
	_passiveSockets.erase(i);
	_numberConnectedActive++;
	break; // Found it.
      }
    }
  ar_mutex_unlock(&_dataTransferMutex);
}

bool arDataServer::sendData(arStructuredData* pData){
  ar_mutex_lock(&_dataTransferMutex);
    bool anyConnections = false;
    const int theSize = pData->size();
    if (ar_growBuffer(_dataBuffer, _dataBufferSize, theSize)) {
      pData->pack(_dataBuffer);
      anyConnections = _sendDataCore(_dataBuffer, theSize);
    }
    else {
      cerr << "arDataServer warning: failed to grow buffer.\n";
    }
  ar_mutex_unlock(&_dataTransferMutex);
  return anyConnections;
}

bool arDataServer::sendDataQueue(arQueuedData* pData){
  ar_mutex_lock(&_dataTransferMutex);
    const bool anyConnections =
      _sendDataCore(pData->getFrontBufferRaw(), pData->getFrontBufferSize());
  ar_mutex_unlock(&_dataTransferMutex);
  return anyConnections;
}

// We send data in a different thread from where we accept connections.
// Call this only inside _dataTransferMutex.
bool arDataServer::_sendDataCore(ARchar* theBuffer, const int theSize){
  list<arSocket*> removalList;
  list<arSocket*>::iterator iter;
  bool anyConnections = false;
  for (iter = _connectionSockets.begin();
       iter != _connectionSockets.end();
       ++iter){
    arSocket* fd = *iter;
    if (fd->ar_safeWrite(theBuffer,theSize)){
      anyConnections = true;
    }
    else{
      //NOISY cerr << "arDataServer warning: failed to broadcast data.\n";
      removalList.push_back(fd);
    }
  }
  for (iter = removalList.begin();
       iter != removalList.end();
       ++iter)
    _deleteSocketFromDatabase(*iter);
  return anyConnections;
}

bool arDataServer::sendData(arStructuredData* pData, arSocket* fd){
  ar_mutex_lock(&_dataTransferMutex);
  bool status = sendDataNoLock(pData, fd);
  ar_mutex_unlock(&_dataTransferMutex);
  return status;
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

bool arDataServer::sendDataQueue(arQueuedData* pData, arSocket* fd){
  // we need to check that the arSocket pointer is not NULL
  if (!fd){
    cerr << "arDataServer warning: ignoring data-queue-send to NULL socket.\n";
    return false;
  }
  ar_mutex_lock(&_dataTransferMutex);
  const bool ok =
    _sendDataCore(pData->getFrontBufferRaw(), pData->getFrontBufferSize(), fd);
  ar_mutex_unlock(&_dataTransferMutex);
  return ok;
}

// Call this only inside _dataTransferMutex.
bool arDataServer::_sendDataCore(ARchar* theBuffer, const int theSize, arSocket* fd){
  // Checks in caller already ensure that fd is not NULL.
  if (fd->ar_safeWrite(theBuffer,theSize))
    return true;
  cerr << "arDataServer warning: failed to send data to specific socket.\n";
  _deleteSocketFromDatabase(fd);
  return false;
}

bool arDataServer::sendDataQueue(arQueuedData* theData,
				 list<arSocket*>* socketList){
  const int theSize = theData->getFrontBufferSize();
  ar_mutex_lock(&_dataTransferMutex);
  list<arSocket*> removalList;
  bool anyConnections = false;
  ARchar* theBuffer = theData->getFrontBufferRaw();
  list<arSocket*>::iterator i;
  for (i = socketList->begin(); i != socketList->end(); ++i){
    if ((*i)->ar_safeWrite(theBuffer, theSize)){
      anyConnections = true;
    }
    else{
      cerr << "arDataServer warning: failed to send data.\n";
      removalList.push_back(*i);
    }
  }
  for (i = removalList.begin(); i != removalList.end(); i++){
    _deleteSocketFromDatabase(*i);
  }
  ar_mutex_unlock(&_dataTransferMutex);
  return anyConnections;
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
  string result;
  char buffer[16];
  ar_mutex_lock(&_dataTransferMutex);
    for (map<int,string,less<int> >::iterator iLabel(_connectionLabels.begin());
	 iLabel != _connectionLabels.end();
	 ++iLabel){
      sprintf(buffer,"%i",iLabel->first);
      result += iLabel->second + "/" + buffer + ":";
    }
  ar_mutex_unlock(&_dataTransferMutex);
  return result;
}

arSocket* arDataServer::getConnectedSocket(int theSocketID){
  ar_mutex_lock(&_dataTransferMutex);
    arSocket* result = getConnectedSocketNoLock(theSocketID);
  ar_mutex_unlock(&_dataTransferMutex);
  return result;
}

arSocket* arDataServer::getConnectedSocketNoLock(int theSocketID){
  map<int,arSocket*,less<int> >::iterator i(_connectionIDs.find(theSocketID));
  return i==_connectionIDs.end() ? NULL : i->second;
}

void arDataServer::setSocketLabel(arSocket* theSocket, const string& theLabel){
  ar_mutex_lock(&_dataTransferMutex);
    _addSocketLabel(theSocket, theLabel);
  ar_mutex_unlock(&_dataTransferMutex);
}

string arDataServer::getSocketLabel(int theSocketID){
  ar_mutex_lock(&_dataTransferMutex);
  map<int,string,less<int> >::iterator i(_connectionLabels.find(theSocketID));
  const string s(i==_connectionLabels.end() ? string("NULL") : i->second);
  ar_mutex_unlock(&_dataTransferMutex);
  return s;
}

int arDataServer::getFirstIDWithLabel(const string& theSocketLabel){
  int result = -1;
  ar_mutex_lock(&_dataTransferMutex);
  for (map<int,string,less<int> >::iterator i = _connectionLabels.begin();
      i != _connectionLabels.end();
      ++i){
    if (theSocketLabel == i->second){
      // Found a match!
      result = i->first;
      break;
    }
  }
  ar_mutex_unlock(&_dataTransferMutex);
  return result;
}

// _dataTransferMutex serializes this, so 2 sockets don't get the same ID.
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
    cerr << "arDataServer error: dialUp(" << s << ":" << port
         << ") failed to create socket.\n";
    return -1;
  }
  if (!setReceiveBufferSize(socket)){
    return -1;
  }
  if (!socket->smallPacketOptimize(_smallPacketOptimize)){
    cerr << "arDataServer error: dialUp(" << s << ":" << port
         << ") failed to smallPacketOptimize.\n";
    return -1;
  }
  if (socket->ar_connect(s.c_str(), port) < 0){
    cerr << "arDataServer error: dialUp failed.\n";
    socket->ar_close();
    // delete socket?
    return -1;
  }

  // Set up communications.
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE;

  // BUG: This is NOT the socket ID... but the arDataServer doesn't do
  // anything with it anyway (yet). This badly breaks symmetry. Oh well.
  // No code depends on it yet.
  localConfig.ID = 0;
  arStreamConfig remoteStreamConfig;
  remoteStreamConfig = handshakeReceiveConnection(socket, localConfig);
  if (!remoteStreamConfig.valid){
    if (remoteStreamConfig.refused){
      cout << "arDataServer remark: remote data point closed connection.\n"
	   << "  (Maybe this IP address isn't on the szgserver's whitelist.)\n";
      return false;
    }
    cerr << "arDataServer error: remote data point has wrong szg protocol "
	 << "version = " << remoteStreamConfig.version << ".\n";
    return false;
  }
  
  ARchar sizeBuffer[AR_INT_SIZE];
  // NOTE: We DO NOT actually use that in the arDataServer (as opposed
  // to the arDataClient.
  // in arDataClient, a statement would go hear storing the remote socket
  // ID.
  if (!socket->ar_safeRead(sizeBuffer,AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to get dictionary size.\n";
    socket->ar_close();
    return -1;
  }
  const ARint totalSize = ar_translateInt(sizeBuffer,remoteStreamConfig);
  if (totalSize<AR_INT_SIZE){
    cerr << "arDataServer error: dialUp failed to translate dictionary.\n";
    socket->ar_close();
    return -1;
  }

  ARchar* dataBuffer = new ARchar[totalSize];
  memcpy(dataBuffer, sizeBuffer, AR_INT_SIZE);
  if (!socket->ar_safeRead(dataBuffer+AR_INT_SIZE, totalSize-AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to get dictionary.\n";
    delete [] dataBuffer;
    return -1;
  }

  // Success!
  delete [] dataBuffer;
  ar_mutex_lock(&_dataTransferMutex);
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
      ar_mutex_unlock(&_dataTransferMutex);
      return -1;
    } 
    // Wait until the new thread reads _nextConsumer into local storage.
    _threadLaunchSignal.receiveSignal();
  }
  ar_mutex_unlock(&_dataTransferMutex);
  return socket->getID();
}
