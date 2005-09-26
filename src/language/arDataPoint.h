//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************
 
#ifndef AR_DATA_POINT
#define AR_DATA_POINT

#include "arDataUtilities.h"
#include "arLanguageCalling.h"
#include <map>
using namespace std;

/// Infrastructure for arDataClient and arDataServer.

// Used to make sure that incompatible communications protocols do not
// attempt to speak to one another.
enum {SZG_VERSION_NUMBER = 2};

class SZG_CALL arDataPoint {
 private:
  int _bufferSize;
 protected:
  ARchar* _dataBuffer;
  int _dataBufferSize;
  ARchar* _translationBuffer;
  ARint _translationBufferSize;
  bool _smallPacketOptimize;
  bool setReceiveBufferSize(arSocket* socket);

 public:
  arDataPoint(int dataBufferSize);
  ~arDataPoint();
  void smallPacketOptimize(bool flag)
    { _smallPacketOptimize = flag; } ///< faster for small packets
  bool getDataCore(ARchar*& dest, int& availableSize, bool& fEndianMode,
    arSocket* fd, const arStreamConfig& remoteConfig);
  bool getDataCore(ARchar*& dest, int& availableSize, ARint& theSize, 
                   bool& fEndianMode, arSocket* fd, 
                   const arStreamConfig& remoteConfig);
  bool getDataCore(ARchar*& dest, int& availableSize,
		   ARchar*& trans, int& transSize,
		   ARint& theSize, bool& fEndianMode, arSocket* fd,
		   const arStreamConfig& remoteConfig);
  void setBufferSize(int numBytes); ///< Set size of socket buffer.
  // The functions we need to do the initial socket handshake.
  arStreamConfig handshakeConnectTo(arSocket* fd, arStreamConfig localConfig);
  arStreamConfig handshakeReceiveConnection(arSocket* fd,
                                            arStreamConfig localConfig);
  string _remoteConfigString(arSocket* fd);
  string _constructConfigString(arStreamConfig config);
  map<string, string, less<string> > _parseKeyValueBlock(const string& text);
  void _fillConfig(arStreamConfig& config, const string& text);
};

#endif
