//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BARRIER_CLIENT_H
#define AR_BARRIER_CLIENT_H

#include "arDataUtilities.h"
#include "arDataClient.h"
#include "arSZGClient.h"
#include "arBarrierCalling.h"

using namespace std;

// Client for arBarrierServer.
class SZG_CALL arBarrierClient{
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_barrierClientConnection(void*);
  friend void ar_barrierClientData(void*);
 public:
  arBarrierClient();
  ~arBarrierClient();

  bool requestActivation(); // Call this to join the server's barrier group.
  bool checkActivation();
  bool setBondedSocketID(int);

  void setServiceName(const string& serviceName);
  void setNetworks(const string& networks);

  bool init(arSZGClient& client);
  bool start();
  void stop();
  void setTuningData(int, int, int, int);
  int  getServerSendSize() const
    { return _serverSendSize; }
  bool sync();
  bool checkConnection() const
    { return _connected; }
  const string& getLabel() const;

 private:
  arSZGClient* _client;
  string       _serviceName;
  string       _networks;

  // the following variables hold the tuning data that is
  // sent back to the barrier server on every synch.
  // this data enables adaptive optimization of volume on our net connections
  int _drawTime;       // time it took to execute the actual draw command
  int _rcvTime;        // time it took to receive the data
  int _procTime;       // time it took to process the data
  int _frameNum;       // the ID of the frame last processed
  int _serverSendSize; // how much data was sent to all connected clients

  // TCP connection stuff... this is used to monitor the connection
  // and to do the initial 3-way handshake that activates a passive connection
  arStructuredData* _handshakeData;
  arStructuredData* _responseData;
  arStructuredData* _clientTuningData;
  arStructuredData* _serverTuningData;
  arDataClient _dataClient;
  ARchar* _dataBuffer;
  ARint   _bufferSize;
  bool _activationResponse;
  arLock _activationLock; // with _activationVar
  arConditionVar _activationVar;
  int _bondedSocketID;
  int BONDED_ID;
  int CLIENT_TUNING_DATA;
  int SERVER_TUNING_DATA;

  bool _activated;        // has the connection been activated?

  bool _connected;
  bool _keepRunningThread;

  arThread _connectionThread;
  arThread _dataThread;
  arSignalObject _releaseSignal;

  bool _exitProgram;
  bool _connectionThreadRunning;
  bool _dataThreadRunning;
  arLock _sendLock;
  bool _finalSyncSent;

  void _connectionTask();
  void _dataTask();
};

#endif
