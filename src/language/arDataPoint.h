//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************
 
#ifndef AR_DATA_POINT
#define AR_DATA_POINT

#include "arDataUtilities.h"
using namespace std;

/// Infrastructure for arDataClient and arDataServer.

class arDataPoint {
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
};

#endif
