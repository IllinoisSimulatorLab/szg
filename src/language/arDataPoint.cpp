//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataPoint.h"
#ifdef AR_USE_WIN_32
#include <iostream>
#else
#include <signal.h>
#include <netinet/tcp.h>
#endif

// The includes for the handshake.
#include "arStructuredDataParser.h"
#include "arSocketTextStream.h"
#include <sstream>
using namespace std;

// Used to make sure that incompatible communications protocols do not
// attempt to speak to one another.
const int SZG_VERSION_NUMBER = 1;

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
  // in the midst of a client's write
  signal(SIGPIPE,SIG_IGN);
#endif
}

arDataPoint::~arDataPoint() {
  delete [] _dataBuffer;
  delete [] _translationBuffer;
}

/// getDataCore is used to grow the buffers used to receive data (both
/// the "local binary format buffer" and the "translation buffer" and
/// read data into the local binary format buffer. Note that we cannot
/// necessarily use built in buffers (for instance the built-in 
/// translation buffer) since objects like the arDataServer have multiple
/// simultaneous connections in seperate threads. 
/// @param dest a pointer reference to the "local binary format buffer"
/// @param availableSize the allocated size of the "local ... buffer"
/// @param trans a pointer reference to the "translation buffer"
/// @param transSize the allocated size of the "translation buffer"
/// @param theSize the size of the data read from the connection
/// @param fEndianMode a flag showing whether machine-specific translation
/// is required
/// @param fd the socket from which we are reading data
/// @param remoteConfig the binary format of the remote peer
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

/// This function is a simplified version of getDataCore for when one is
/// using the single internal _translationBuffer. NOTE: THIS IS NOT
/// THREAD-SAFE!
bool arDataPoint::getDataCore(ARchar*& dest, int& availableSize,
                              ARint& theSize, bool& fEndianMode, arSocket* fd, 
                              const arStreamConfig& remoteConfig){
  return getDataCore(dest, availableSize, _translationBuffer, 
		     _translationBufferSize, theSize, fEndianMode, fd,
                     remoteConfig);
}

/// This function is a simplified version of getDataCore for when one is
/// unconcerned about the size of the downloaded data (and also when one
/// is using the single internal _translationBuffer). NOTE: THIS IS NOT
/// THREAD-SAFE!
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
    cout << "syzygy remark: rounding buffer size "
         << numBytes << " up to 256.\n";
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
    cerr << "syzygy error: failed to set buffer size to " 
         << _bufferSize << ".\n";
    return false;
  }
  return true;
}

arStreamConfig arDataPoint::handshakeConnectTo(arSocket* fd, 
                                               arStreamConfig localConfig){
  arStreamConfig result;
  // Give some sensible values.
  result.endian = AR_LITTLE_ENDIAN;
  result.version = -1;
  result.ID = -1;
  string configString = _constructConfigString(localConfig);
  // As the connector-to, we send first!
  if (!fd->ar_safeWrite(configString.c_str(),configString.length())){
    // Do not complain here.
    result.valid = false;
    return result;
  }
  // Now, receive the stuff from the other side.
  string keys = _remoteConfigString(fd);
  if (keys == "NULL"){
    // Do not complain here.
    result.valid = false;
    return result;
  }
  _fillConfig(result, keys);
  return result;
}

arStreamConfig arDataPoint::handshakeReceiveConnection(arSocket* fd,
						   arStreamConfig localConfig){
  arStreamConfig result;
  // Receive the stuff from the other side.
  string keys = _remoteConfigString(fd);
  if (keys == "NULL"){
    // Do not complain here.
    result.valid = false;
    return result;
  }
  _fillConfig(result, keys);
  string configString = _constructConfigString(localConfig);
  // As the one receiving the connection, we send second!
  if (!fd->ar_safeWrite(configString.c_str(),configString.length())){
    // Do not complain here.
    result.valid = false;
    return result;
  }
  return result;
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
    // Do not complain here.
    return "NULL";
  }
  string result = remoteConfig->getDataString("keys");
  parser.recycle(remoteConfig);
  return result;
}

string arDataPoint::_constructConfigString(arStreamConfig config){
  stringstream sendStream;
  // We manually build up the string to send out, instead of relying on
  // the arStructuredData infrastructure.
  // Note the use of the MAGIC version number!
  sendStream << "<config>"
	     << " "
	     << "<keys>"
	     << " " 
             << "endian=" 
             << (config.endian == AR_LITTLE_ENDIAN ? "little" : "big")
	     << " "
	     << "version=" << SZG_VERSION_NUMBER
	     << " "
	     << "ID=" << config.ID
	     << "</keys>"
	     << " "
	     << "</config>";
  return sendStream.str();
}

map<string, string, less<string> > arDataPoint::_parseKeyValueBlock(
						   const string& text){
  map<string, string, less<string> > result;
  // Break the string up into tokens.
  stringstream s;
  s.str(text);
  string myToken;
  while (true) {
    s >> myToken;
    if (!s.fail()){
      // Got something;
      unsigned int position = myToken.find("=");
      if (position == string::npos){
        // Token is invalid;
	cout << "arDataPoint error: received invalid token.\n";
	continue;
      }
      string key = myToken.substr(0, position);
      string value = myToken.substr(position+1, myToken.length()-position-1);
      result.insert(map<string,string,less<string> >::value_type(key,
								 value));
    }
    if (s.eof()){
      break;
    }
  }
  // Go ahead and return the constructed table.
  return result;
}

void arDataPoint::_fillConfig(arStreamConfig& config, const string& text){
  map<string,string,less<string> > table = _parseKeyValueBlock(text);
  map<string,string,less<string> >::iterator iter;
  // First, check to see that version numbers match.
  iter = table.find("version");
  if (iter == table.end()){
    // There is no version key.
    config.valid = false;
    return;
  }
  stringstream versionParser;
  versionParser.str(iter->second);
  versionParser >> config.version;
  if (versionParser.fail()){
    config.valid = false;
    return;
  }
  if (config.version != SZG_VERSION_NUMBER){
    config.valid = false;
    return;
  }
  // Next, extract the endian-ness of the remote connection.
  iter = table.find("endian");
  if (iter == table.end()){
    // There is no endian key.
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
    // Not one of the allowed values.
    config.valid = false;
    return;
  }
  // Finally, extract the ID of the remote connection.
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
