//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataServer.h"

// DO NOT ALLOCATE THE LISTENING SOCKET IN THE CONSTRUCTOR. WE WANT TO
// BE ABLE TO DECLARE arDataServer AS A GLOBAL ON WINDOWS COMPUTERS.
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
  _atomicReceive(true),
  _acceptMask("NULL")
{
  ar_mutex_init(&_dataTransferMutex);
  ar_mutex_init(&_consumptionLock);
}

arDataServer::~arDataServer(){
  // close all current connections
  if (_numberConnected > 0){
    cerr << "arDataServer warning: destructor trying to close sockets.\n";
    for (list<arSocket*>::iterator i(_connectionSockets.begin());
         i != (_connectionSockets.end());
         ++i){
      arSocket* theSocket = *i;
      theSocket->ar_close();
      delete theSocket;
      --_numberConnected;
    }
  }
  if (_numberConnected > 0)
    cerr << "arDataServer error: destructor internal logic error.\n";
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

  // Allocate buffer space for this thread.
  int availableSize = _dataBufferSize;
  ARchar* dest = new ARchar[availableSize];
  // note that we also need a local translation buffer... neither of these
  // buffers can be shared with the other read threads
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

  // it seems like throttling the rate at which connections can
  // be accepted prevents szgserver crashes on dual processor
  // linux machines... not sure why! this is something it would
  // be nice to remove if stability could be maintained.
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
    return NULL;
  }

  // based on the connection-acceptance state, we either add the new
  // socket to the active list or the passive list
  (addToActive ? _connectionSockets : _passiveSockets).push_back(newSocketFD);

  // Send the connection's configuration information.
  // first, the stream config, next the ID of the socket according to
  // this arDataServer, and finally the dictionary that defines the
  // language we'll use to communicate
  if (!_theDictionary){
    cerr << "arDataServer error: no dictionary.\n";
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }

  const int theSize = _theDictionary->size()+2*AR_INT_SIZE;
  if (theSize<=0){
    ar_mutex_unlock(&_dataTransferMutex);
    cerr << "arDataServer error 1: garbled dictionary.\n";
    return NULL;
  }
  ARchar* buffer = new ARchar[theSize];
  // send the config info
  buffer[0] = AR_ENDIAN_MODE;
  buffer[1] = 0;
  buffer[2] = 0;
  buffer[3] = 0;
  ARint socketID = newSocketFD->getID();
  ar_packData(buffer+AR_INT_SIZE,&socketID,AR_INT,1);
  _theDictionary->pack(buffer+2*AR_INT_SIZE);
  if (!newSocketFD->ar_safeWrite(buffer,theSize)){
    cerr << "arDataServer error 2: failed to send dictionary.\n";
    _deleteSocketFromDatabase(newSocketFD);
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }
  // now, we need the other round of the handshake... since the remote
  // socket can also send *us* information

  if (!newSocketFD->ar_safeRead(buffer,AR_INT_SIZE)){
    cerr << "arDataServer error 3: failed to receive remote stream config.\n";
    _deleteSocketFromDatabase(newSocketFD);
    ar_mutex_unlock(&_dataTransferMutex);
    return NULL;
  }

  /// The remote stream info is, in fact, used. For instance, if we are
  /// reading data from external sources.
  arStreamConfig remoteStreamConfig;
  remoteStreamConfig.endian = buffer[0];
  _setSocketRemoteConfig(newSocketFD,remoteStreamConfig);
  delete [] buffer;

  // based on the connection-acceptance state, we might or might
  // not need to increment the number of active connections
  _numberConnected++;
  if (addToActive)
    _numberConnectedActive++;

  if (_consumerFunction){
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = newSocketFD;
    arThread* dummy = new arThread; /// \bug memory leak?
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

  // delete the socket from the internal databases, if it, in fact, is still there
  if (!_delSocketID(theSocket))
    // Socket wasn't in the ID table.  Must have been deleted already.
    return;

  // delete the socket from the label table
  if (!_delSocketLabel(theSocket))
    cerr << "arDataServer warning: internal socket databases are inconsistent.\n";

  // finally, go ahead and remove the socket*
  for (list<arSocket*>::iterator removalIterator(_connectionSockets.begin());
       removalIterator != _connectionSockets.end();
       ++removalIterator){
    if (theSocket->getID() == (*removalIterator)->getID()){
      _connectionSockets.erase(removalIterator);
      theSocket->ar_close(); // good idea
      --_numberConnected;
      --_numberConnectedActive;
      break; // don't look any more
    }
  }
  // if the user has supplied a disconnect function, go ahead and call it here
  if (_disconnectFunction){
    _disconnectFunction(_disconnectObject, theSocket);
  }
  // don't forget to delete the socket. obviously important to do this *after*
  // calling the disconnect function... because the disconnect function uses the
  // socket pointer
  // NOTE: I've eliminated the delete of the socket for right now. As it stands, given
  // the comments above, IT IS UNSAFE, i.e. multiple threads can be working on the 
  // pointer at once. Note that this is a very small resource leak... about 20 bytes or
  // so per connection, so an szgserver would have to handle *millions* of connections
  // before we'd need to care. Obviously important to plug eventually, but not as
  // critical as stability. Note, that it IS critical we close the socket (which is
  // a very large scale system resource), which is, indeed, done. 
  // MEMORY LEAK
  //delete theSocket;
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

/// -1 is returned on error. Otherwise the ID of the new socket.
/// THIS IS VERY BAD COPY-PASTE FROM arDataClient. PLEASE LEAVE IT
/// UNTIL IT GETS CLEANED-UP.
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
    return -1;
  }

  // Now, set-up communications.
  ARchar sizeBuffer[AR_INT_SIZE];
  // Get stream configuration (only endianness, more could happen later).
  if (!socket->ar_safeRead(sizeBuffer,AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to get stream config.\n";
    socket->ar_close();
    return -1;
  }
  arStreamConfig remoteStreamConfig;
  remoteStreamConfig.endian = sizeBuffer[0];

  // Get ID of socket managed by the remote arDataServer object.
  if (!socket->ar_safeRead(sizeBuffer,AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to get socket ID.\n";
    socket->ar_close();
    return -1;
  }
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
    cerr << "arDataServer error: dialUp got a garbled dictionary.\n";
    socket->ar_close();
    return -1;
  }

  ARchar* dataBuffer = new ARchar[totalSize];
  memcpy(dataBuffer, sizeBuffer, AR_INT_SIZE);
  if (!socket->ar_safeRead(dataBuffer+AR_INT_SIZE, totalSize-AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to get dictionary.\n";
    goto LAbort;
  }
  // ONLY THE arDataClient INITIALIZES THE DICTIONARY!
  // Handshake, since the other arDataServer will potentially receive 
  // data from us.
  sizeBuffer[0] = AR_ENDIAN_MODE;
  sizeBuffer[1] = 0;
  sizeBuffer[2] = 0;
  sizeBuffer[3] = 0;
  if (!socket->ar_safeWrite(sizeBuffer,AR_INT_SIZE)){
    cerr << "arDataServer error: dialUp failed to send local stream config.\n";
LAbort:
    delete [] dataBuffer;
    socket->ar_close();
    return -1;
  }

  // Success!
  delete [] dataBuffer;
  ar_mutex_lock(&_dataTransferMutex);
  // IMPORTANT TO ADD THIS TO THE ACTIVE CONNECTIONS!
  _connectionSockets.push_back(socket);
  _addSocketToDatabase(socket);
  _setSocketRemoteConfig(socket, remoteStreamConfig);
  // THIS IS CUT_AND_PASTE FROM ACCEPT CONNECTION IN THIS OBJECT.
  _numberConnected++;
  // THIS IS ONLY MEANINGFUL IN THE SPECIAL CASE OF THE SYNC SOCKETS
  //if (addToActive)
  //  _numberConnectedActive++;

  if (_consumerFunction){
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = socket;
    arThread* dummy = new arThread; /// \bug memory leak?
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
