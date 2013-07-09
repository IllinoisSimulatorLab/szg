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
  _interfaceIP(string("INADDR_ANY")),
  _portNumber(-1),
  _listeningSocket(NULL),
  _numberConnected(0),
  _numberConnectedActive(0),
  _nextID(0),
  _lockTransfer("DSERVE_TRANSFER"),
  _dataParser(NULL),
  _lockConsume("DSERVE_CONSUME"),
  _consumeData(false),
  _consumerCallback(NULL),
  _consumerObject(NULL),
  _disconnectCallback(NULL),
  _disconnectObject(NULL),
  _atomicReceive(true)
{
}

arDataServer::~arDataServer() {
  // Close all connections.
  if (_numberConnected > 0) {
    ar_log_remark() << "arDataServer destructor closing sockets.\n";
    for (list<arSocket*>::iterator i(_connectionSockets.begin());
         i != _connectionSockets.end();
         ++i) {
      (*i)->ar_close();
      delete *i;
      --_numberConnected;
    }
  }
  if (_numberConnected > 0)
    ar_log_error() << "arDataServer destructor confused.\n";
  delete _listeningSocket;
}

void ar_readDataThread(void* dataServer) {
  ((arDataServer*)dataServer)->_readDataTask();
}

void arDataServer::_readDataTask() {
  // Bug: chaos ensues if _atomicReceive changes during this thread.  Cache a local copy thereof.
  arSocket* newFD = _nextConsumer;

  // Determine the formatting used by this client's remote thread.
  map<int, arStreamConfig, less<int> >::const_iterator iter =
    _connectionConfigs.find(newFD->getID());
  if (iter == _connectionConfigs.end()) {
    ar_log_error() <<
      "arDataServer: read thread launched without stream config, socket ID = " <<
      newFD->getID() << ".\n";
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
        ar_log_error() << "arDataServer: no dictionary.\n";
        break;
      }
      const ARint recordID = ar_translateInt(transBuffer+AR_INT_SIZE, remoteConfig);
      // Bug? if !_atomicReceive, this still needs to be locked.
      arGuard _(_lockConsume, "arDataServer::_readDataTask");
      arDataTemplate* t = _theDictionary->find(recordID);
      if (!t || t->translate(dest, transBuffer, remoteConfig) <= 0) {
        ar_log_error() << "arDataServer failed to translate record.\n";
        break;
      }
    }

    // data is OK
    if (_atomicReceive) {
      _lockConsume.lock("arDataServer::_readDataTask A");
    }
    arStructuredData* inData = _dataParser->parse(dest, theSize);
    if (inData) {
      onConsumeData( inData, newFD );
      _dataParser->recycle(inData);
    }
    if (_atomicReceive) {
      _lockConsume.unlock();
    }
    if (!inData) {
      ar_log_error() << "arDataServer failed to parse record.\n";
      break;
    }
  }

  delete [] dest;
  delete [] transBuffer;

  // If _atomicReceive, also invoke that lock here, because the delete
  // socket callback might want to stuff (as in szgserver) that expects to
  // be atomic w.r.t. the consumer function invoked above.
  if (_atomicReceive) {
    _lockConsume.lock("arDataServer::_readDataTask B");
  }
  _lockTransfer.lock("arDataServer::_readDataTask C");
  _deleteSocketFromDatabase(newFD);
  _lockTransfer.unlock();
  if (_atomicReceive) {
    _lockConsume.unlock();
  }
}

void arDataServer::atomicReceive(bool atomicReceive) {
  _atomicReceive = atomicReceive;
}

bool arDataServer::setPort(int thePort) {
  if (thePort < 1024 || thePort > 50000) {
    ar_log_error() << "arDataServer ignoring out-of-range (1024-50000) port value " <<
      thePort << ".\n";
    return false;
  }

  _portNumber = thePort;
  return true;
}

bool arDataServer::setInterface(const string& theInterface) {
  if (theInterface == "NULL") {
    ar_log_error() << "arDataServer ignoring NULL setInterface.\n";
    return false;
  }
  _interfaceIP = theInterface;
  return true;
}

bool arDataServer::beginListening(arTemplateDictionary* theDictionary) {
  if (_portNumber == -1) {
    ar_log_error() << "arDataServer failed to listen on undefined port.\n";
    return false;
  }

  if (!theDictionary) {
    cerr << "arDataServer error: no dictionary.\n";
    return false;
  }

  _theDictionary = theDictionary;
  if (!_dataParser) {
    _dataParser = new arStructuredDataParser(_theDictionary);
  }
  if (!_listeningSocket) {
    _listeningSocket = new arSocket(AR_LISTENING_SOCKET);
  }
  if (_listeningSocket->ar_create() < 0 ||
      !setReceiveBufferSize(_listeningSocket) ||
      !_listeningSocket->reuseAddress(true)) {
    ar_log_error() << "arDataServer failed to begin listening.\n";
    _listeningSocket->ar_close(); // avoid memory leak
    return false;
  }

  // Pass down the accept mask that implements
  // TCP-wrappers style filtering on clients that try to connect
  _listeningSocket->setAcceptMask(_acceptMask);

  // ar_bind needs C string, not std::string
  char addressBuffer[256];
  ar_stringToBuffer(_interfaceIP, addressBuffer, sizeof(addressBuffer));
  if (_listeningSocket->
      ar_bind(_interfaceIP == "INADDR_ANY" ? NULL : addressBuffer, _portNumber) < 0) {
    ar_log_error() << "arDataServer failed to bind to "
         << _interfaceIP << ":" << _portNumber
         << "\n\t(not this host's IP address?  port in use?).\n";
    _listeningSocket->ar_close(); // avoid memory leak
    return false;
  }

  _listeningSocket->ar_listen(256);
  return true;
}

bool arDataServer::removeConnection(int id) {
  arGuard _(_lockTransfer, "arDataServer::removeConnection");
  const map<int, arSocket*, less<int> >::iterator i(_connectionIDs.find(id));
  const bool found = (i != _connectionIDs.end());
  if (found)
    _deleteSocketFromDatabase(i->second);
  return found;
}

arSocket* arDataServer::_acceptConnection(bool addToActive) {
  if (!_listeningSocket) {
    ar_log_error() << "arDataServer can't acceptConnection before beginListening.\n";
    return NULL;
  }
  ar_usleep(30000); // Might improve stability.  Probably unnecessary.

  // Accept connections in a different thread from the one sending data.
  arSocket* sockNew = new arSocket(AR_STANDARD_SOCKET);
  if (!sockNew) {
    ar_log_error() << "arDataServer: no socket in _acceptConnection.\n";
    return NULL;
  }
  arSocketAddress addr;
  if (_listeningSocket->ar_accept(sockNew, &addr) < 0) {
    ar_log_error() << "arDataServer failed to _acceptConnection.\n";
    return NULL;
  }
  // Possible bug, e.g. server is masterslave app, client is SoundRender:
  // if there was only 1 client, which disconnected and then reconnected from a different IP
  // (on a different subnet), then we should ar_log_error().
  // But how can we distinguish this case from the general multiple-client case?
  // Caching a single addr as a member variable is too simplistic.

  ar_log_debug() << "arDataServer got connection from " << addr.getRepresentation() << ".\n";

  arGuard _(_lockTransfer, "arDataServer::_acceptConnection");
  _addSocketToDatabase(sockNew);
  if (!sockNew->smallPacketOptimize(_smallPacketOptimize)) {
    ar_log_error() << "arDataServer failed to smallPacketOptimize.\n";
LAbort:
    return NULL;
  }

  // Add the new socket to either the active or the passive list.
  (addToActive ? _connectionSockets : _passiveSockets).push_back(sockNew);

  if (!_theDictionary) {
    // We expected to SEND the dictionary to the connected data point.
    ar_log_error() << "arDataServer: no dictionary.\n";
    _deleteSocketFromDatabase(sockNew);
    goto LAbort;
  }

  // Configuration handshake.
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE; // todo: do this line in arStreamConfig's constructor.
  localConfig.ID = sockNew->getID();
  arStreamConfig remoteStreamConfig = handshakeConnectTo(sockNew, localConfig);
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
    ar_log_error() << "arDataServer rejected connection from " <<
      addr.getRepresentation() << ": " << sSymptom << ".\n";
    _deleteSocketFromDatabase(sockNew);
    goto LAbort;
  }
  // We need to know the remote stream config for this socket, since we
  // might send data (i.e. szgserver or arBarrierServer).
  _setSocketRemoteConfig(sockNew, remoteStreamConfig);

  // Send the dictionary.
  const int theSize = _theDictionary->size();
  if (theSize<=0) {
    ar_log_error() << "arDataServer failed to pack dictionary.\n";
    _deleteSocketFromDatabase(sockNew);
    goto LAbort;
  }

  ARchar* buffer = new ARchar[theSize]; // Storage for the dictionary.
  _theDictionary->pack(buffer);
  if (!sockNew->ar_safeWrite(buffer, theSize)) {
    ar_log_error() << "arDataServer failed to send dictionary.\n";
    _deleteSocketFromDatabase(sockNew);
    goto LAbort;
  }
  delete [] buffer;

  _numberConnected++;
  if (addToActive)
    _numberConnectedActive++;

  if (_consumeData) {
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = sockNew;
    arThread* dummy = new arThread; // memory leak?
    if (!dummy->beginThread(ar_readDataThread, this)) {
      ar_log_error() << "arDataServer failed to start read thread.\n";
      return NULL;
    }
    // Wait until ar_readDataThread reads _nextConsumer into local storage.
    _threadLaunchSignal.receiveSignal();
  }

  return sockNew;
}

void arDataServer::activatePassiveSockets() {
  arGuard _(_lockTransfer, "arDataServer::activatePassiveSockets");
  for (list<arSocket*>::const_iterator i(_passiveSockets.begin());
       i != _passiveSockets.end(); ++i) {
    _connectionSockets.push_back(*i);
    _numberConnectedActive++;
  }
  _passiveSockets.clear();
}

bool arDataServer::checkPassiveSockets() {
  arGuard _(_lockTransfer, "arDataServer::checkPassiveSockets");
  return _passiveSockets.begin() != _passiveSockets.end();
}

list<arSocket*>* arDataServer::getActiveSockets() {
  list<arSocket*>* result = new list<arSocket*>;
  arGuard _(_lockTransfer, "arDataServer::getActiveSockets");
  for (list<arSocket*>::const_iterator i=_connectionSockets.begin();
       i != _connectionSockets.end(); ++i) {
    result->push_back(*i);
  }
  return result;
}

void arDataServer::activatePassiveSocket(int socketID) {
  arGuard _(_lockTransfer, "arDataServer::activatePassiveSocket socketID");
  for (list<arSocket*>::iterator i(_passiveSockets.begin());
       i != _passiveSockets.end(); ++i) {
    if ((*i)->getID() == socketID) {
      _connectionSockets.push_back(*i);
      _passiveSockets.erase(i);
      ++_numberConnectedActive;
      break;
    }
  }
}

//#define DEBUG

bool arDataServer::sendData(arStructuredData* pData) {
  ar_timeval t0 = ar_time();
  const int theSize = pData->size();
  arGuard _(_lockTransfer, "arDataServer::sendData");
  bool stat = ar_growBuffer(_dataBuffer, _dataBufferSize, theSize);
  if (!stat) {
#ifdef DEBUG
    ar_log_error() << "arDataServer::sendData(): ar_growBuffer() failed.\n";
#endif
    return false;
  }
  ar_timeval t1 = ar_time();
  stat = pData->pack(_dataBuffer); 
  if (!stat) {
#ifdef DEBUG
    ar_log_error() << "arDataServer::sendData(): pack() failed.\n";
#endif
    return false;
  }
  ar_timeval t2 = ar_time();
  stat = _sendDataCore( _dataBuffer, theSize );
  if (!stat) {
#ifdef DEBUG
    ar_log_error() << "arDataServer::sendData(): _sendDataCore() failed.\n";
#endif
    return false;
  }
  ar_timeval t3 = ar_time();
#ifdef DEBUG
  ar_log_remark() << "sendData() time: " << ar_difftime( t3, t0 ) << ar_endl;
#endif
  return true;
}

bool arDataServer::sendDataQueue(arQueuedData* pData) {
  arGuard _(_lockTransfer, "arDataServer::sendDataQueue");
  return _sendDataCore(pData->getFrontBufferRaw(), pData->getFrontBufferSize());
}

// Send data in a different thread from where we accept connections.
// Call this only inside _lockTransfer.
// Return true if any connections.
bool arDataServer::_sendDataCore(const ARchar* theBuffer, const int theSize) {
  bool ok = false;
  list<arSocket*> removalList;
  list<arSocket*>::iterator iter;
  for (iter = _connectionSockets.begin(); iter != _connectionSockets.end(); ++iter) {
    arSocket* fd = *iter;
    if (fd->ar_safeWrite(theBuffer, theSize)) {
      ok = true;
    } else {
      // Failed to broadcast data.
      removalList.push_back(fd);
#ifdef DEBUG
      ar_log_error() << "arSocket::ar_safeWrite() failed.\n";
#endif
    }
  }
  for (iter = removalList.begin(); iter != removalList.end(); ++iter)
    _deleteSocketFromDatabase(*iter);
  return ok;
}

bool arDataServer::sendData(arStructuredData* pData, arSocket* fd) {
  arGuard _(_lockTransfer, "arDataServer::sendData fd");
  return sendDataNoLock(pData, fd);
}

bool arDataServer::sendDataNoLock(arStructuredData* pData, arSocket* fd) {
  if (!fd) {
    ar_log_error() << "arDataServer ignoring send to NULL socket.\n";
    return false;
  }
  const int theSize = pData->size();
  if (!ar_growBuffer(_dataBuffer, _dataBufferSize, theSize)) {
    ar_log_error() << "arDataServer failed to grow buffer.\n";
    return false;
  }
  pData->pack(_dataBuffer);
  return _sendDataCore(_dataBuffer, theSize, fd);
}

bool arDataServer::sendDataQueue(arQueuedData* p, arSocket* fd) {
  if (!fd) {
    ar_log_error() << "arDataServer ignoring queue-send to NULL socket.\n";
    return false;
  }
  arGuard _(_lockTransfer, "arDataServer::sendDataQueue fd");
  return _sendDataCore(p->getFrontBufferRaw(), p->getFrontBufferSize(), fd);
}

// Call this only inside _lockTransfer.
bool arDataServer::_sendDataCore(const ARchar* theBuffer, const int theSize, arSocket* fd) {
  // Caller ensures that fd != NULL.
  if (fd->ar_safeWrite(theBuffer, theSize))
    return true;
  ar_log_error() << "arDataServer failed to send data to specific socket.\n";
  _deleteSocketFromDatabase(fd);
  return false;
}

bool arDataServer::sendDataQueue(arQueuedData* theData, list<arSocket*>* socketList) {
  const int theSize = theData->getFrontBufferSize();
  list<arSocket*> removalList;
  bool ok = false;
  ARchar* theBuffer = theData->getFrontBufferRaw();
  list<arSocket*>::iterator i;
  arGuard _(_lockTransfer, "arDataServer::sendDataQueue socketList");
  for (i = socketList->begin(); i != socketList->end(); ++i) {
    if ((*i)->ar_safeWrite(theBuffer, theSize)) {
      ok = true;
    }
    else{
      ar_log_error() << "arDataServer failed to send data.\n";
      removalList.push_back(*i);
    }
  }
  for (i = removalList.begin(); i != removalList.end(); i++) {
    _deleteSocketFromDatabase(*i);
  }
  return ok;
}

void arDataServer::onConsumeData( arStructuredData* data, arSocket* socket ) {
  if (_consumerCallback == NULL) {
    return;
  }
  _consumerCallback( data, _consumerObject, socket );
}

void arDataServer::setConsumerCallback(void (*consumerCallback)
                                       (arStructuredData*, void*, arSocket*)) {
  _consumerCallback = consumerCallback;
  if (_consumerCallback == NULL) {
    setConsume(false);
  } else {
    setConsume(true);
  }
}

void arDataServer::setConsumerObject(void* consumerObject) {
  _consumerObject = consumerObject;
}

void arDataServer::onDisconnect( arSocket* theSocket ) {
  if (_disconnectCallback) {
    // Call the user-supplied disconnect function.
    _disconnectCallback(_disconnectObject, theSocket);
  }
}

void arDataServer::setDisconnectCallback
  (void (*disconnectCallback)(void*, arSocket*)) {
  _disconnectCallback = disconnectCallback;
}

void arDataServer::setDisconnectObject(void* disconnectObject) {
  _disconnectObject = disconnectObject;
}

string arDataServer::dumpConnectionLabels() {
  string s;
  arGuard _(_lockTransfer, "arDataServer::dumpConnectionLabels");
  for (map<int, string, less<int> >::const_iterator iLabel(_connectionLabels.begin());
       iLabel != _connectionLabels.end(); ++iLabel) {
    s += iLabel->second + "/" + ar_intToString(iLabel->first) + ":";
  }
  return s;
}

arSocket* arDataServer::getConnectedSocket(int id) {
  arGuard _(_lockTransfer, "arDataServer::getConnectedSocket");
  return getConnectedSocketNoLock(id);
}

arSocket* arDataServer::getConnectedSocketNoLock(int id) {
  map<int, arSocket*, less<int> >::iterator i(_connectionIDs.find(id));
  return i==_connectionIDs.end() ? NULL : i->second;
}

void arDataServer::setSocketLabel(arSocket* theSocket, const string& theLabel) {
  arGuard _(_lockTransfer, "arDataServer::setSocketLabel");
  _addSocketLabel(theSocket, theLabel);
}

string arDataServer::getSocketLabel(int theSocketID) {
  arGuard _(_lockTransfer, "arDataServer::getSocketLabel");
  map<int, string, less<int> >::const_iterator i(_connectionLabels.find(theSocketID));
  return i==_connectionLabels.end() ? string("NULL") : i->second;
}

int arDataServer::getFirstIDWithLabel(const string& theSocketLabel) {
  arGuard _(_lockTransfer, "arDataServer::getFirstIDWithLabel");
  for (map<int, string, less<int> >::const_iterator i = _connectionLabels.begin();
      i != _connectionLabels.end(); ++i) {
    if (theSocketLabel == i->second) {
      return i->first;
    }
  }
  return -1;
}

// _lockTransfer serializes this, so 2 sockets don't get the same ID.
void arDataServer::_addSocketToDatabase(arSocket* theSocket) {
  theSocket->setID(_nextID++);         // give the socket an ID
  _addSocketLabel(theSocket, "NULL");  // give the socket a default label
  _addSocketID(theSocket);             // insert in the ID table
}

bool arDataServer::_delSocketID(arSocket* s) {
  map<int, arSocket*, less<int> >::iterator i(_connectionIDs.find(s->getID()));
  if (i == _connectionIDs.end())
    return false;
  _connectionIDs.erase(i); // Socket already existed.
  return true;
}

bool arDataServer::_delSocketLabel(arSocket* s) {
  map<int, string, less<int> >::iterator i(_connectionLabels.find(s->getID()));
  if (i == _connectionLabels.end())
    return false;
  ar_log_debug() << "Deleted socket label " << s->getID() << ", " << i->second << ".\n";
  _connectionLabels.erase(i); // Socket already existed.
  return true;
}

void arDataServer::_addSocketID(arSocket* s) {
  (void)_delSocketID(s);
  _connectionIDs.insert(
    map<int, arSocket*, less<int> >::value_type(s->getID(), s));
}

void arDataServer::_addSocketLabel(arSocket* s, const string& label) {
  (void)_delSocketLabel(s);
  _connectionLabels.insert(
    map<int, string, less<int> >::value_type(s->getID(), label));
  ar_log_debug() << "Added socket label " << s->getID() << ", " << label << ".\n";
}

void arDataServer::_deleteSocketFromDatabase(arSocket* theSocket) {
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
    ar_log_error() << "arDataServer: inconsistent internal socket databases.\n";

  // Remove the socket*.
  for (list<arSocket*>::iterator removalIterator(_connectionSockets.begin());
       removalIterator != _connectionSockets.end();
       ++removalIterator) {
    if (theSocket->getID() == (*removalIterator)->getID()) {
      _connectionSockets.erase(removalIterator);
      theSocket->ar_close(); // good idea
      --_numberConnected;
      --_numberConnectedActive;
      break; // Stop looking.
    }
  }
  onDisconnect( theSocket );

  // Memory leak, about 20 bytes per connection:
  // theSocket isn't deleted, because multiple threads might
  // be using it at once.  If we could delete it, we'd do so here
  // after onDisconnect() has used it, of course.
}

void arDataServer::_setSocketRemoteConfig(arSocket* theSocket,
                                          const arStreamConfig& config) {
  map<int, arStreamConfig, less<int> >::iterator
    iter(_connectionConfigs.find(theSocket->getID()));
  if (iter != _connectionConfigs.end()) {
    ar_log_error() << "arDataServer erasing duplicate socket ID.\n";
    _connectionConfigs.erase(iter);
  }
  _connectionConfigs.insert
    (map<int, arStreamConfig, less<int> >::value_type
      (theSocket->getID(), config));
}

// -1 is returned on error. Otherwise the ID of the new socket.
// Unlike arDataClient, which returns bool.  Beware!
int arDataServer::dialUpFallThrough(const string& s, int port) {
  arSocket* socket = new arSocket(AR_STANDARD_SOCKET);
  if (socket->ar_create() < 0) {
    ar_log_error() << "arDataServer: dialUp(" << s << ":" << port
         << ") failed to create socket.\n";
    return -1;
  }

  if (!setReceiveBufferSize(socket)) {
    return -1;
  }

  if (!socket->smallPacketOptimize(_smallPacketOptimize)) {
    ar_log_error() << "arDataServer: dialUp(" << s << ":" << port
         << ") failed to smallPacketOptimize.\n";
    return -1;
  }

  arSocketAddress addr;
  if (socket->ar_connect(s.c_str(), port) < 0) {
    ar_log_error() << "arDataServer failed to dialUp.\n";
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
  if (!remoteStreamConfig.valid) {
    if (remoteStreamConfig.refused) {
      ar_log_remark() << "arDataServer disconnected (host not on szgserver's whitelist?).\n";
      return -1;
    }
    ar_log_error() << "arDataServer got wrong Syzygy protocol version "
         << remoteStreamConfig.version << ".\n";
    return -1;
  }

  ARchar sizeBuffer[AR_INT_SIZE];
  // Unlike arDataClient, don't *use* sizeBuffer.
  // arDataClient would store the remote socket ID here.
  if (!socket->ar_safeRead(sizeBuffer, AR_INT_SIZE)) {
    ar_log_error() << "arDataServer: dialUp got no dictionary size.\n";
    socket->ar_close();
    return -1;
  }

  const ARint totalSize = ar_translateInt(sizeBuffer, remoteStreamConfig);
  if (totalSize < AR_INT_SIZE) {
    ar_log_error() << "arDataServer: dialUp failed to translate dictionary.\n";
    socket->ar_close();
    return -1;
  }

  ARchar* dataBuffer = new ARchar[totalSize];
  memcpy(dataBuffer, sizeBuffer, AR_INT_SIZE);
  if (!socket->ar_safeRead(dataBuffer+AR_INT_SIZE, totalSize-AR_INT_SIZE)) {
    ar_log_error() << "arDataServer: dialUp got no dictionary.\n";
    delete [] dataBuffer;
    return -1;
  }

  delete [] dataBuffer;

  arGuard _(_lockTransfer, "arDataServer::dialUpFallThrough");
  // Add this to the list of active connections.
  _connectionSockets.push_back(socket);
  _addSocketToDatabase(socket);
  _setSocketRemoteConfig(socket, remoteStreamConfig);
  // Copypaste from accept connection in this object.
  _numberConnected++;

  // Only meaningful for sync sockets
  //if (addToActive)
  //  _numberConnectedActive++;

  if (_consumeData) {
    // A consumer is registered, so start a read thread for the new connection.
    _nextConsumer = socket;
    arThread* dummy = new arThread; // \bug memory leak?
    if (!dummy->beginThread(ar_readDataThread, this)) {
      ar_log_error() << "arDataServer failed to start read thread.\n";
      return -1;
    }
    // Wait until the new thread reads _nextConsumer into local storage.
    _threadLaunchSignal.receiveSignal();
  }
  return socket->getID();
}
