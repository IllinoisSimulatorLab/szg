//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BARRIER_SERVER_H
#define AR_BARRIER_SERVER_H

#include "arThread.h"
#include "arDataUtilities.h"
#include "arDataServer.h"
#include "arSZGClient.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arBarrierCalling.h"
#include <iostream>
using namespace std;

/// Server for arBarrierClient objects.
class SZG_CALL arBarrierServer{
  // Needs assignment operator and copy constructor, for pointer members.

  friend void ar_releaseFunction(void*);
  friend void ar_connectionFunction(void*);
  friend void ar_barrierDataFunction(arStructuredData*,void*,arSocket*);
  friend void ar_barrierDisconnectFunction(void*,arSocket*);
 public:
  arBarrierServer();
  ~arBarrierServer();

  void setServiceName(string serviceName);
  bool init(arSZGClient& client);
  void setChannel(const string& channel)
    { _channel = channel; } ///< Set route for network traffic.
  bool start();
  void stop();
  int getDrawTime() const
    { return _drawTime; }
  int getRcvTime() const
    { return _rcvTime; }
  int getProcTime() const
    { return _procTime; }
  bool setServerSendSize(int);

  void setSignalObject(arSignalObject* signalObject);
  void setSignalObjectRelease(arSignalObject*);

  bool activatePassiveSockets(arDataServer*);
    ///< Releases activationQueue lock, if lockActivationQueue() set it.
  bool checkWaitingSockets();
  void lockActivationQueue();
  void unlockActivationQueue();
  list<arSocket*>* getWaitingBondedSockets(arDataServer*);

  void registerLocal(); ///< Register local connection.
  void localSync();

  int getNumberConnectedActive();
  int getNumberConnected();

 private: 
  arSZGClient*   _client;
  string         _serviceName;
  
  int            _totalWaiting;
  arConditionVar _waitingCondVar;
  arMutex        _waitingLock; // Guards _totalWaiting and _waitingCondVar (?)

  arThread _releaseThread;
  arThread _receiveThread;

  bool _started;     // Was start() called?
  bool _runThreads;  // When true, signal the threads to terminate.

  arSignalObject* _signalObject;
  arSignalObject* _signalObjectRelease;

  // internal storage for the tuning information
  int _drawTime;        // time it took to execute the actual draw command
  int _rcvTime;         // time it took to receive the data
  int _procTime;        // time it took to process the data
  int _frameNum;        // the ID of the frame last processed
  int _serverSendSize;  // the amount of data the server sent

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
  list< pair<int,int> > _activationSocketIDs;
  arMutex _activationLock;
  arMutex _queueActivationLock;
  bool _activationQueueLockedExternally;
  bool _activationResponse;
  arConditionVar _activationVar;

  bool _pumpPrimingFlag;

  //local connection stuff
  arSignalObject _localSignal;
  bool _localConnection;

  bool _exitProgram;

  string _channel;

  void _barrierDataFunction(arStructuredData*,arSocket*);
  void _releaseFunction();
  void _barrierDisconnectFunction();
};

#endif
