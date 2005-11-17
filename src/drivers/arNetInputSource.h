//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NET_INPUT_SOURCE_H
#define AR_NET_INPUT_SOURCE_H

#include "arInputSource.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"
#include <string>
using namespace std;

/// Something that sends input-device messages over ethernet.

class SZG_CALL arNetInputSource: public arInputSource{
  friend void ar_netInputSourceDataTask(void*);
  friend void ar_netInputSourceConnectionTask(void*);
 public:
  arNetInputSource();
  ~arNetInputSource() {}

  void setSlot(int slot);

  virtual bool init(arSZGClient&);
  virtual bool start();

 private:
  arDataClient  _dataClient;
  arSZGClient*  _client;
  ARchar*       _dataBuffer;
  int           _dataBufferSize;
  int    _slot;
  string _interface;
  int    _port;
  bool   _clientConnected;
  bool _clientInitialized;
  
  void _closeConnection();
  bool _checkConnection();
  void _dataTask();
};

#endif
