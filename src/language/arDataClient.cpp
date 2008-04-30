//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataClient.h"
#include "arLogStream.h"

// DO NOT ALLOCATE THE SOCKET IN THE CONSTRUCTOR. WE WANT TO
// BE ABLE TO DECLARE arDataServer AS A GLOBAL ON WINDOWS COMPUTERS.
arDataClient::arDataClient(const string& exeName) :
  arDataPoint(256),
  _theDictionary(NULL),
  _socket(NULL),
  _activeConnection(false) // disable closeConnection if there is no active connection
{
  setLabel(exeName);
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
  _exeName = (exeName.length() > 0) ? exeName : string("Syzygy arDataClient");
}

arTemplateDictionary* arDataClient::getDictionary(){
  return _theDictionary; // not a const pointer:  pointee WILL change.
}

bool arDataClient::_translateID(ARchar* buf, ARchar* dest, int& size) {
  if (!_theDictionary) {
    ar_log_error() << _exeName << ": no dictionary.\n";
    return false;
  }

  const ARint recordID =
    ar_translateInt(buf+AR_INT_SIZE, _remoteStreamConfig);
  arDataTemplate* theTemplate = _theDictionary->find(recordID);
  if (!theTemplate ||
      (size = theTemplate->translate(dest, buf, _remoteStreamConfig)) < 0) {
    ar_log_error() << _exeName << " failed to translate data record.\n";
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

  // Iff fEndianMode is true, then no translation is necessary.
  return fEndianMode || _translateID(_translationBuffer, dest, size);
}

// This function loads a data queue from the server, translating between
// machine formats if necessary. A data queue is two ints (total size,
// number of records) followed by a sequence of serialized data records
// @param dest Where the data will go, a buffer that grows if needed
// @param availableSize Size of "dest"
bool arDataClient::getDataQueue(ARchar*& dest,int& availableSize){
  bool fEndianMode = false;
  if (!getDataCore(dest, availableSize, fEndianMode, 
                   _socket, _remoteStreamConfig)) {
    return false;
  }

  if (fEndianMode)
    return true;

  ARint destPos = 0;
  ARint srcPos = 0;
  (void)ar_translateInt(
    dest, destPos, _translationBuffer, srcPos, _remoteStreamConfig);
  const ARint numberRecords = ar_translateInt(
    dest, destPos, _translationBuffer, srcPos, _remoteStreamConfig);
  for (int i=0; i<numberRecords; ++i) {
    const ARint recordSize =
      ar_translateInt(_translationBuffer+srcPos, _remoteStreamConfig);
    int transSize = -1;
    if (!_translateID(_translationBuffer+srcPos, dest+destPos, transSize))
      return false;

    destPos += transSize;
    srcPos += recordSize;
  }
  return true; 
}

// When two peers connect, they exchange configuration information,
// so that each can translate the other's binary data format.
bool arDataClient::_dialUpActivate(){
  arStreamConfig localConfig;
  localConfig.endian = AR_ENDIAN_MODE;

  // What is a "data point"?  Is there a friendlier name we can use?

  // Only one socket in this data point.
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
    if (_remoteStreamConfig.refused){
      ar_log_remark() << _exeName << ": connection got closed.\n"
	   << "  (Maybe this IP address isn't on the szgserver's whitelist.)\n";
      return false;
    }

    ar_log_error() << _exeName << ": remote data point has wrong szg protocol version, "
      << _remoteStreamConfig.version << ".\n";
    return false;

  }
  // Set the remote socket ID.
  _socketIDRemote = _remoteStreamConfig.ID;

  ARchar sizeBuffer[AR_INT_SIZE];
  // Read in the dictionary from the server.
  if (!_socket->ar_safeRead(sizeBuffer,AR_INT_SIZE)){
    ar_log_error() << _exeName << ": dialUp failed to read dictionary size.\n";
    return false;
  }

  const ARint totalSize = ar_translateInt(sizeBuffer,_remoteStreamConfig);
  if (totalSize<AR_INT_SIZE){
    ar_log_error() << _exeName << ": dialUp failed to translate dictionary "
	 << "size.\n";
    return false;
  }

  ARchar* dataBuffer = new ARchar[totalSize];
  memcpy(dataBuffer, sizeBuffer, AR_INT_SIZE);
  if (!_socket->ar_safeRead(dataBuffer+AR_INT_SIZE, totalSize-AR_INT_SIZE)){
    ar_log_error() << _exeName << ": dialUp failed to get dictionary.\n";
    delete [] dataBuffer;
    return false;
  }

  // Initialize the dictionary.
  _theDictionary = new arTemplateDictionary;
  if (!_theDictionary->unpack(dataBuffer,_remoteStreamConfig)){
    ar_log_error() << _exeName << ": dialUp failed to unpack dictionary.\n";
    delete [] dataBuffer;
    return false;
  }

  delete [] dataBuffer;
  _activeConnection = true;
  return true;
}

bool arDataClient::_dialUpInit(const char* address, int port){
  if (!strcmp(address, "NULL")){
    if (port == 0)
      ar_log_error() << _exeName << ": dialUp: no IP address or port.\n";
    else
      ar_log_error() << _exeName << ": dialUp: NULL IP address for port " << port << ".\n";
    return false;
  }

  if (strlen(address) < 7){
    ar_log_error() << _exeName << ": invalid IP address in dialUp("
      << address << ":" << port << ").\n";
    return false;
  }

  if (port < 1000 || port > 65535){
    ar_log_error() << _exeName << ": out-of-range port in dialUp(" <<
      address << ":" << port << ").  Try 1000 to 65535.\n";
    return false;
  }

  if (!_socket){
    _socket = new arSocket(AR_STANDARD_SOCKET);
  }
  if (_socket->ar_create() < 0) {
    ar_log_error() << _exeName << ": no socket created for dialUp(" <<
      address << ":" << port << ").\n";
    return false;
  }

  if (!setReceiveBufferSize(_socket))
    return false;

  if (!_socket->smallPacketOptimize(_smallPacketOptimize)){
    ar_log_error() << _exeName << ": no smallPacketOptimize for dialUp(" <<
      address << ":" << port << ").\n";
    return false;
  }

  return true;
}

bool arDataClient::_dialUpConnect(const char* address, int port) {
  if (_socket->ar_connect(address, port) >= 0) {
    return true;
  }

  // Don't complain. Data sinks like szgrender don't need a data server.
  _socket->ar_close();
  return false;
}

bool arDataClient::dialUpFallThrough(const char* address, int port){
  // bug? dlogin won't take "-szg log=DEBUG", so dlogin can't print these ar_log_debug()'s.
  if (!_dialUpInit(address, port)) {
    ar_log_debug() << "arDataClient._dialUpInit(" << address << ":" << port << ") failed.\n";
    return false;
  }
  if (!_dialUpConnect(address, port)) {
    ar_log_debug() << "arDataClient._dialUpConnect(" << address << ":" << port << ") failed.\n";
    return false;
  }
  if (!_dialUpActivate()) {
    ar_log_debug() << "arDataClient._dialUpActivate(" << address << ":" << port << ") failed.\n";
    return false;
  }
  return true;
}

bool arDataClient::dialUp(const char* address, int port){
  arSleepBackoff a(15, 200, 1.4);
  while (true) {
    if (!_dialUpInit(address, port))
      return false;
    if (_dialUpConnect(address, port))
      return _dialUpActivate();
    a.sleep();
  }
}

void arDataClient::closeConnection(){
  if (_activeConnection){
    _socket->ar_close();
    _activeConnection = false;
  }
}

// Send data to the data server.
bool arDataClient::sendData(arStructuredData* theData){
  if (!theData) {
    ar_log_error() << _exeName << " ignoring sendData(NULL)\n";
    return false;
  }

  const int theSize = theData->size();

  // Thread-safe.
  arGuard dummy(_lockSend);

  return ar_growBuffer(_dataBuffer, _dataBufferSize, theSize) &&
    theData->pack(_dataBuffer) &&
    _socket->ar_safeWrite(_dataBuffer, theSize);
}
