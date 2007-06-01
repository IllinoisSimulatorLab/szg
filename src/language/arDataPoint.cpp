//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataPoint.h"

#ifndef AR_USE_WIN_32
  #include <signal.h>
  #include <netinet/tcp.h>
#endif

// The includes for the handshake.
#include "arStructuredDataParser.h"
#include "arSocketTextStream.h"
#include <sstream>
using namespace std;

arDataPoint::arDataPoint(int dataBufferSize) :
  _bufferSize(-1), // Set socket's TCP receive buffer to its default.
  _smallPacketOptimize(false)
{
  if (dataBufferSize < 256)
    dataBufferSize = 256;
  _dataBufferSize = dataBufferSize;
  _translationBufferSize = _dataBufferSize;

  _dataBuffer = new char[_dataBufferSize];
  _translationBuffer = new ARchar[_translationBufferSize];

#ifndef AR_USE_WIN_32
  // crashing clients shouldn't mess with us
  // this seems mainly helpful when a remote server terminates
  // during a client's write
  signal(SIGPIPE,SIG_IGN);
#endif
}

arDataPoint::~arDataPoint() {
  delete [] _dataBuffer;
  delete [] _translationBuffer;
}

// Grow the buffers used to receive data, both
// the "local binary format buffer" and the "translation buffer",
// and read data into the local binary format buffer. Don't use
// built-in buffers (for instance the built-in 
// translation buffer), because objects like the arDataServer have multiple
// simultaneous connections in seperate threads. 
// @param dest a pointer reference to the "local binary format buffer"
// @param availableSize the allocated size of the "local ... buffer"
// @param trans a pointer reference to the "translation buffer"
// @param transSize the allocated size of the "translation buffer"
// @param theSize the size of the data read from the connection
// @param fEndianMode a flag showing whether machine-specific translation
// is required
// @param fd the socket from which we are reading data
// @param remoteConfig the binary format of the remote peer
bool arDataPoint::getDataCore(ARchar*& dest, int& availableSize, 
                              ARchar*& trans, int& transSize,
                              ARint& theSize, bool& fEndianMode, arSocket* fd, 
                              const arStreamConfig& remoteConfig){
  char temp[4];
  if (!fd->ar_safeRead(temp,AR_INT_SIZE))
    return false;
  
  theSize = ar_translateInt(temp,remoteConfig);
  fEndianMode = (remoteConfig.endian == AR_ENDIAN_MODE);
  ARchar* pch = NULL;
  if (fEndianMode) {
    if (!ar_growBuffer(dest, availableSize, theSize))
      return false;
    pch = dest;
  }
  else {
    if (!ar_growBuffer(trans, transSize, theSize)
        || !ar_growBuffer(dest, availableSize, theSize))
      return false;
    pch = trans;
  }
  pch[0] = temp[0];
  pch[1] = temp[1];
  pch[2] = temp[2];
  pch[3] = temp[3];
  return fd->ar_safeRead(pch + AR_INT_SIZE, theSize - AR_INT_SIZE);
}

// Simplified getDataCore for the single internal _translationBuffer.
// Not thread-safe.
bool arDataPoint::getDataCore(ARchar*& dest, int& availableSize,
                              ARint& theSize, bool& fEndianMode, arSocket* fd, 
                              const arStreamConfig& remoteConfig){
  return getDataCore(dest, availableSize, _translationBuffer, 
		     _translationBufferSize, theSize, fEndianMode, fd,
                     remoteConfig);
}

// This function is a simplified version of getDataCore for when one is
// unconcerned about the size of the downloaded data (and also when one
// is using the single internal _translationBuffer). NOTE: THIS IS NOT
// THREAD-SAFE!
bool arDataPoint::getDataCore(ARchar*& dest, int& availableSize, 
                              bool& fEndianMode, arSocket* fd, 
                              const arStreamConfig& remoteConfig){
  ARint size = -1;
  return getDataCore(dest, availableSize, 
                     _translationBuffer, _translationBufferSize,
                     size, fEndianMode, fd, remoteConfig);
}

void arDataPoint::setBufferSize(int numBytes){
  if (numBytes < 256) {
    cout << "syzygy remark: rounding buffer size " << numBytes << " up to 256.\n";
    numBytes = 300;
  }
  else if (numBytes > 1000000) {
    cout << "syzygy remark: rounding buffer size "
         << numBytes << " down to 1000000.\n";
    numBytes = 1000000;
  }
  _bufferSize = numBytes;
}

bool arDataPoint::setReceiveBufferSize(arSocket* socket) {
  // If we haven't explicitly set the buffer size, use the OS defaults.
  if (_bufferSize > 0 && 
      (!socket->setSendBufferSize(_bufferSize)
       || !socket->setReceiveBufferSize(_bufferSize))) {
    cerr << "syzygy error: failed to set buffer size to " << _bufferSize << ".\n";
    return false;
  }
  return true;
}

arStreamConfig& arDataPoint::_handshakeReceive(arStreamConfig& config, arSocket* fd) {
  // Receive stuff from the other side.
  const string keys = _remoteConfigString(fd);
  if (keys == "NULL"){
    config.valid = false;
    config.refused = true; // The connection attempt was refused.
    return config;
  }
  // Got a parseable header for the stream.
  // _fillConfig may invalidate config.
  _fillConfig(config, keys);
  return config;
}

arStreamConfig arDataPoint::handshakeConnectTo(
    arSocket* fd, arStreamConfig localConfig){
  // Caller is responsible for printing warnings.
  arStreamConfig config;

  // Defaults.
  config.endian = AR_LITTLE_ENDIAN;
  config.version = -1;
  config.ID = -1;

  // As the connector-to, send first.
  const string configString = _constructConfigString(localConfig);
  if (!fd->ar_safeWrite(configString.c_str(), configString.length())){
    config.valid = false;
    return config;
  }

  return _handshakeReceive(config, fd);
}

arStreamConfig arDataPoint::handshakeReceiveConnection(arSocket* fd,
						   arStreamConfig localConfig){
  arStreamConfig config;
  // Bug: why does config get default values in handshakeConnectTo but not here?
  (void)_handshakeReceive(config, fd);

  // As the one receiving the connection, send second.
  string configString = _constructConfigString(localConfig);
  if (!fd->ar_safeWrite(configString.c_str(),configString.length())){
    config.valid = false;
  }
  return config;
}

string arDataPoint::_remoteConfigString(arSocket* fd){
  arTemplateDictionary configDictionary;
  arDataTemplate config("config");
  config.add("keys",AR_CHAR);
  configDictionary.add(&config);

  // Receive stuff from the other side first.
  arStructuredDataParser parser(&configDictionary);
  arSocketTextStream socketStream;
  socketStream.setSource(fd);
  arStructuredData* remoteConfig = parser.parse(&socketStream);
  if (!remoteConfig){
    // This happens mainly when the other connection end rejects
    // our connection attempt outright (for instance when our IP address
    // does not meet the filtering requirements)

    // It might also happen if the other end does not speak the szg connection
    // protocol.
    
    // Do not complain here.
    return "NULL";
  }
  string result = remoteConfig->getDataString("keys");
  parser.recycle(remoteConfig);
  return result;
}

string arDataPoint::_constructConfigString(const arStreamConfig& config){
  stringstream s;
  // Manually build up the string to send out,
  // instead of relying on the arStructuredData infrastructure.
  // Note the MAGIC version number.
  s << "<config> <keys> "
    << "endian=" 
    << (config.endian == AR_LITTLE_ENDIAN ? "little" : "big")
    << " version=" << SZG_VERSION_NUMBER
    << " ID=" << config.ID
    << "</keys> </config>";
  return s.str();
}

map<string, string, less<string> > arDataPoint::_parseKeyValueBlock(
						   const string& text){
  map<string, string, less<string> > tokens;
  // Tokenize the string.
  stringstream s;
  s.str(text);
  string myToken;
  while (true) {
    s >> myToken;
    if (!s.fail()){
      const unsigned position = myToken.find("=");
      if (position == string::npos){
	cerr << "arDataPoint error: invalid token.\n";
	continue;
      }
      const string key(myToken.substr(0, position));
      const string value(myToken.substr(position+1, myToken.length()-position-1));
      tokens.insert(map<string,string,less<string> >::value_type(key, value));
    }
    if (s.eof())
      break;
  }
  return tokens;
}

void arDataPoint::_fillConfig(arStreamConfig& config, const string& text){
  config.refused = false;

  map<string,string,less<string> > table = _parseKeyValueBlock(text);
  map<string,string,less<string> >::iterator iter;

  iter = table.find("version");
  if (iter == table.end()){
    // No version key.
    config.version = -2;
    config.valid = false;
    return;
  }
  stringstream versionParser;
  versionParser.str(iter->second);
  versionParser >> config.version;
  if (versionParser.fail()){
    // Unparseable version key.
    config.version = -3;
    config.valid = false;
    return;
  }
  if (config.version != SZG_VERSION_NUMBER){
    // Version mismatch.
    config.valid = false;
    return;
  }
  // Next, extract the endian-ness of the remote connection.
  iter = table.find("endian");
  if (iter == table.end()){
    // No endian key.
    config.endian = AR_UNDEFINED_ENDIAN;
    config.valid = false;
    return;
  }
  if (iter->second == "little"){
    config.endian = AR_LITTLE_ENDIAN;
  }
  else if (iter->second == "big"){
    config.endian = AR_BIG_ENDIAN;
  }
  else{
    config.endian = AR_GARBAGE_ENDIAN;
    config.valid = false;
    return;
  }

  // Extract the ID of the remote connection.
  iter = table.find("ID");
  if (iter == table.end()){
    // There is no ID tag.
    config.valid = false;
    return;
  }

  stringstream IDparser;
  IDparser.str(iter->second);
  IDparser >> config.ID;
  if (IDparser.fail()){
    config.valid = false;
    return;
  }
  config.valid = true;
}
