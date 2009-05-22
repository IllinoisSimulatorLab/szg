//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NET_INPUT_SOURCE_H
#define AR_NET_INPUT_SOURCE_H

#include "arInputSource.h"
#include "arDriversCalling.h"

#include <string>
using namespace std;

// Send input-device messages.

class SZG_CALL arNetInputSource: public arInputSource{
  friend void ar_netInputSourceConnectionTask(void*);
 public:
  arNetInputSource();
  virtual ~arNetInputSource() {}

  // Only use ONE OR THE OTHER of these two!
  bool setSlot(unsigned);
  bool setServiceName( const string& name );

  bool connected() const
    { return _connected; }

  virtual bool init(arSZGClient&);
  virtual bool start();

 private:
  arDataClient  _dataClient;
  arSZGClient*  _szgClient;
  ARchar*       _dataBuffer;
  int           _dataBufferSize;
  unsigned    _slot;
  string      _serviceName;
  string _IP;
  int    _port;
  bool   _connected;
  bool   _sigOK;

  void _closeConnection();
  void _dataTask();
  void _connectionTask();
  const string getLabel() const
    { return (_szgClient ? _szgClient->getLabel() : "uninited") + " arNetInputSource"; }
};

#endif
