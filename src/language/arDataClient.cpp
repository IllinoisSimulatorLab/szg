//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataClient.h"

// DO NOT ALLOCATE THE SOCKET IN THE CONSTRUCTOR. WE WANT TO
// BE ABLE TO DECLARE arDataServer AS A GLOBAL ON WINDOWS COMPUTERS.
arDataClient::arDataClient(const string& exeName) :
  arDataPoint(256),
  _theDictionary(NULL),
  _socket(NULL),
  _activeConnection(false), // disable closeConnection if there is no active connection
  _numComplaints(0)
{
  setLabel(exeName);
  ar_mutex_init(&_sendLock);
}

arDataClient::~arDataClient(){
  if (_theDictionary){
    delete _theDictionary;
  }
  if (_socket){
    delete _socket;
  }
}

void arDataClient::setLabel(const string& exeName){
  if (exeName.length() > 0)
    _exeName = exeName;
  else
    _exeName = string("syzygy arDataClient");
}

arTemplateDictionary* arDataClient::getDictionary(){
  return _theDictionary; // not a const pointer:  pointee WILL change.
}

bool arDataClient::_translateID(ARchar* buf, ARchar* dest, int& size) {
  if (!_theDictionary) {
    cerr << _exeName << " error: no dictionary.\n";
    return false;
  }
  const ARint recordID =
    ar_translateInt(buf+AR_INT_SIZE, _remoteStreamConfig);
  arDataTemplate* theTemplate = _theDictionary->find(recordID);
  if (!theTemplate ||
      (size = theTemplate->translate(dest, buf, _remoteStreamConfig)) < 0) {
    cerr << _exeName << " error: failed to translate data record.\n";
    return false;
  }
  return recordID >= 0;
}

bool arDataClient::getData(ARchar*& dest,int& availableSize){
  bool fEndianMode = false;
  ARint size = -1;
  if (!getDataCore(dest, availableSize, size, fEndianMode, 
                   _socket, _remoteStreamConfig)){
    return false;
  }
  // if fEndianMode is true, then no translation is necessary
  // else, translation is needed
  return fEndianMode || _translateID(_translationBuffer, dest, size);
}

/// This function loads a data queue from the server, translating between
/// machine formats if necessary. A data queue is two ints (total size,
/// number of records) followed by a sequence of serialized data records
/// @param dest Where the data will go, a buffer that grows if needed
/// @param availableSize Size of "dest"
bool arDataClient::getDataQueue(ARchar*& dest,int& availableSize){
  bool fEndianMode = false;
  if (!getDataCore(dest, availableSize, fEndianMode, 
                   _socket, _remoteStreamConfig))
    return false;
  if (fEndianMode)
    return true;

  ARint destPos = 0;
  ARint srcPos = 0;
  (void)ar_translateInt(dest,destPos,_translationBuffer,
		  srcPos,_remoteStreamConfig);
  const ARint numberRecords = ar_translateInt(dest,destPos,
					      _translationBuffer,srcPos,
					      _remoteStreamConfig);
  for (int i=0; i<numberRecords; ++i) {	
    const ARint recordSize =
      ar_translateInt(_translationBuffer+srcPos, _remoteStreamConfig);
    int transSize;
    if (!_translateID(_translationBuffer+srcPos, dest+destPos, transSize))
      return false;

    destPos += transSize;
    srcPos += recordSize;
  }
  return true; 
}

/// When connection occurs between two peers, they must exchange configuration
/// information (so that each can translate the other's binary data format)
bool arDataClient::_dialUpActivate(){
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE;
  // We have only one socket in this data point.
  localConfig.ID = 0;
  // Now, the handshaking looks like so:
  //   a. server sends config, waits for remote config.
  //   b. client waits for remote config, sends config.
  // In the future, it might be desirable for the server and client to
  // simultaneously send configs. And then simultaneously receive. 
  // (to minimize connection start-up latency). Fortunately, these future
  // animals will be able to interoperate with the current ones, with their
  // rigid handshakes (at least to the extent that outmoded clients will not
  // hang). This is due to the buffering and async built into TCP sockets.
  _remoteStreamConfig = handshakeReceiveConnection(_socket, localConfig);
  if (!_remoteStreamConfig.valid){
    cout << _exeName << " error: remote data point has wrong szg protocol "
	 << "version = " << _remoteStreamConfig.version << ".\n";
    return false;
  }
  // Must set the remote socket ID like so:
  _socketIDRemote = _remoteStreamConfig.ID;

  ARchar sizeBuffer[AR_INT_SIZE];
  // Read in the dictionary from the server.
  if (!_socket->ar_safeRead(sizeBuffer,AR_INT_SIZE)){
    cerr << _exeName << " error: dialUp failed to read dictionary size.\n";
    return false;
  }
  const ARint totalSize = ar_translateInt(sizeBuffer,_remoteStreamConfig);
  if (totalSize<AR_INT_SIZE){
    cerr << _exeName << " error: dialUp failed to translate dictionary "
	 << "size.\n";
    return false;
  }
  ARchar* dataBuffer = new ARchar[totalSize];
  memcpy(dataBuffer, sizeBuffer, AR_INT_SIZE);
  if (!_socket->ar_safeRead(dataBuffer+AR_INT_SIZE, totalSize-AR_INT_SIZE)){
    cerr << _exeName << " error: dialUp failed to get dictionary.\n";
    delete [] dataBuffer;
    return false;
  }
  // Initialize the dictionary.
  _theDictionary = new arTemplateDictionary;
  if (!_theDictionary->unpack(dataBuffer,_remoteStreamConfig)){
    cerr << _exeName << " error: dialUp failed to unpack dictionary.\n";
    delete [] dataBuffer;
    return false;
  }

  // Success!
  delete [] dataBuffer;
  _activeConnection = true;
  return true;
}

bool arDataClient::_dialUpInit(const char* address, int port){
  if (!strcmp(address, "NULL")){
    cerr << _exeName 
         << " error: arDataClient::dialUp got NULL address for port "
         << port << ".\n";
    return false;
  }
  if (strlen(address) < 7 || port < 1000 || port > 65535){
    cerr << _exeName << " error: arDataClient::dialUp(" 
         << address << ":" << port
         << ") got an invalid IP address and/or invalid port.\n";
    return false;
  }
  if (!_socket){
    _socket = new arSocket(AR_STANDARD_SOCKET);
  }
  if (_socket->ar_create() < 0) {
    cerr << _exeName << " error: dialUp(" << address << ":" << port
         << ") failed to create socket.\n";
    return false;
  }
  if (!setReceiveBufferSize(_socket))
    return false;
  if (!_socket->smallPacketOptimize(_smallPacketOptimize)){
    cerr << _exeName << " error: dialUp(" << address << ":" << port
         << ") failed to smallPacketOptimize.\n";
    return false;
  }
  return true;
}

bool arDataClient::_dialUpConnect(const char* address, int port) {
  if (_socket->ar_connect(address, port) >= 0)
    return true;
  if (++_numComplaints <= 2){
    // Is the below message really a good idea? It is NORMAL for many data
    // sinks (like szgrender or SoundRender) to be running when there is
    // no associated data server. Consequently this error message pops up,
    // causing confusion for new users. In the near future, when we move
    // to connection brokering, this will be obsolete anyway.

    //cout << _exeName << " remark: no data server at "
    //	 << address << ":" << port << ".\n";
  }
  _socket->ar_close();
  return false;
}

bool arDataClient::dialUpFallThrough(const char* address, int port){
  return _dialUpInit(address, port) &&
         _dialUpConnect(address, port) &&
	 _dialUpActivate();
}

bool arDataClient::dialUp(const char* address, int port){
  _numComplaints = 0;
  int usecDelay = 100000;
  while (true) {
    if (!_dialUpInit(address, port))
      return false;
    if (_dialUpConnect(address, port))
      return _dialUpActivate();

    // Don't DDOS!  Back off slowly, up to a fixed maximum.
    // (1.05 ^ 22 = 3, so after about 2.5 seconds it's every 300 msec.)
    ar_usleep(usecDelay);
    usecDelay = int(usecDelay * 1.05);
    if (usecDelay > 300000)
      usecDelay = 300000;
  }
}

void arDataClient::closeConnection(){
  if (_activeConnection){
    _socket->ar_close();
    _activeConnection = false;
  }
}

/// Sends data on the data client's socket to the connect data server.
/// NOTE: it is very important to lock this so that different threads
/// in a program can issue send data commands (as happens in some
/// program's usage of arSZGClient and its embedded arDataClient).
bool arDataClient::sendData(arStructuredData* theData){
  if (!theData) {
    cerr << _exeName << " error: sendData(NULL)\n";
    return false;
  }

  ar_mutex_lock(&_sendLock);
  const int theSize = theData->size();
  if (!ar_growBuffer(_dataBuffer, _dataBufferSize, theSize)){
    ar_mutex_unlock(&_sendLock);
    return false;
  }
  theData->pack(_dataBuffer);
  bool ok = _socket->ar_safeWrite(_dataBuffer, theSize);
  ar_mutex_unlock(&_sendLock);
  return ok;
}
