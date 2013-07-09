//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BARRIER_SERVER_H
#define AR_BARRIER_SERVER_H

#include "arDataUtilities.h"
#include "arDataServer.h"
#include "arSZGClient.h"
#include "arBarrierCalling.h"

using namespace std;

// Server for arBarrierClient objects.
class SZG_CALL arBarrierServer {
  // Needs assignment operator and copy constructor, for pointer members.

  friend void ar_releaseFunction(void*);
  friend void ar_connectionFunction(void*);
  friend void ar_barrierDataFunction(arStructuredData*, void*, arSocket*);
  friend void ar_barrierDisconnectFunction(void*, arSocket*);
 public:
  arBarrierServer();
  ~arBarrierServer();

  void setServiceName(string serviceName);
  bool init(const string& serviceName, const string& channel, arSZGClient&);
  bool start();
  void stop();
  int getDrawTime() const { return _drawTime; }
  int getRcvTime()  const { return _rcvTime;  }
  int getProcTime() const { return _procTime; }
  bool setServerSendSize(int);

  void setSignalObject(arSignalObject* signalObject);
  void setSignalObjectRelease(arSignalObject*);

  // Release activationQueue lock, if lockActivationQueue() set it.
  bool activatePassiveSockets(arDataServer*);

  bool checkWaitingSockets();
  void lockActivationQueue();
  void unlockActivationQueue();
  list<arSocket*>* getWaitingBondedSockets(arDataServer*);

  void registerLocal(); // Register local connection.
  void localSync();

  int getNumberConnectedActive() const;
  int getNumberConnected() const;

 private:
  arSZGClient*   _client;
  string         _serviceName;

  int            _totalWaiting; // too complicated for arIntAtom
  arConditionVar _waitingCondVar;
  arLock _waitingLock; // with _waitingCondVar, guards _totalWaiting and _waitingCondVar (?)

  arThread _releaseThread;
  arThread _receiveThread;

  bool _started;     // Was start() called?
  bool _runThreads;  // When true, signal the threads to terminate.

  arSignalObject* _signalObject;
  arSignalObject* _signalObjectRelease;

  // internal storage for the tuning information
  int _drawTime;        // duration of the actual draw command
  int _rcvTime;         // time to receive the data
  int _procTime;        // time to process the data
  int _frameNum;        // ID of the frame last processed
  int _serverSendSize;  // amount of data the server sent

  // stuff that's only pertinent to the TCP connection
  // both checking the line and dealing with passive connection mode
  arDataServer _dataServer;
  arTemplateDictionary _theDictionary;
  arDataTemplate _handshakeTemplate;
  arDataTemplate _responseTemplate;
  arDataTemplate _clientTuningTemplate;
  arDataTemplate _serverTuningTemplate;
  arStructuredData* _handshakeData;
  arStructuredData* _responseData;
  arStructuredData* _clientTuningData;
  arStructuredData* _serverTuningData;
  arThread _connectionThread;
  int BONDED_ID;
  int CLIENT_TUNING_DATA;
  int SERVER_TUNING_DATA;
  list< pair<int, int> > _activationSocketIDs;
  arLock _activationLock; // with _activationVar
  arConditionVar _activationVar;
  arLock _queueActivationLock;
  bool _activationQueueLockedExternally;
  bool _activationResponse;

  bool _pumpPrimingFlag;

  arSignalObject _localSignal;
  bool _localConnection;
  bool _exitProgram;
  string _channel; // network route
  void _barrierDataFunction(arStructuredData*, arSocket*);
  void _releaseFunction();
  void _barrierDisconnectFunction();
};

#endif
