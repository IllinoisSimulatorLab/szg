//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_POINT
#define AR_DATA_POINT

#include "arDataUtilities.h"
#include "arLanguageCalling.h"
#include <map>
using namespace std;

// Infrastructure for arDataClient and arDataServer.

// Block connections to incompatible protocols.
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
    { _smallPacketOptimize = flag; } // faster for small packets
  bool getDataCore(ARchar*& dest, int& availableSize, bool& fEndianMode,
    arSocket* fd, const arStreamConfig& remoteConfig);
  bool getDataCore(ARchar*& dest, int& availableSize, ARint& theSize,
                   bool& fEndianMode, arSocket* fd,
                   const arStreamConfig& remoteConfig);
  bool getDataCore(ARchar*& dest, int& availableSize,
                   ARchar*& trans, int& transSize,
                   ARint& theSize, bool& fEndianMode, arSocket* fd,
                   const arStreamConfig& remoteConfig);
  void setBufferSize(int numBytes); // Set size of socket buffer.

  // Initial socket handshake.
  arStreamConfig handshakeConnectTo(arSocket*, arStreamConfig);
  arStreamConfig handshakeReceiveConnection(arSocket*, arStreamConfig);
  string _remoteConfigString(arSocket* fd);
  string _constructConfigString(const arStreamConfig&);
  map<string, string, less<string> > _parseKeyValueBlock(const string& text);
  arStreamConfig& _handshakeReceive(arStreamConfig&, arSocket*);
  void _fillConfig(arStreamConfig&, const string&);
};

#endif
