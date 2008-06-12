//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SYNC_DATA_CLIENT
#define AR_SYNC_DATA_CLIENT

#include "arDataClient.h"
#include "arBarrierClient.h"
#include "arDataUtilities.h"
#include "arSZGClient.h"
#include "arSyncDataServer.h"
#include "arBarrierCalling.h"

#include <list>
#include <string>
using namespace std;

// arSyncDataClient/arSyncDataServer pairs may be unsynchronized for slow networks.
// These constants are public, for user apps.
SZG_CALL enum { AR_SYNC_CLIENT = 0, AR_NOSYNC_CLIENT };

// Client for arSyncDataServer.
class SZG_CALL arSyncDataClient{
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_syncDataClientReadTask(void*);
  friend void ar_syncDataClientConnectionTask(void*);
 public:
  arSyncDataClient();
  ~arSyncDataClient();

  void registerLocalConnection(arSyncDataServer*);

  void setBondedObject(void*);
  bool setMode(int);

  // Callbacks.

  // Called after connected to server.
  void setConnectionCallback
    (bool (*connectionCallback)(void*, arTemplateDictionary*));
  // Called after disconnected from server.
  void setDisconnectCallback
    (bool (*disconnectCallback)(void*));
  // Called when a buffer needs consuming.
  void setConsumptionCallback
    (bool (*consumptionCallback)(void*, ARchar*));
  // Called when action needs doing (while connected).
  void setActionCallback
    (bool (*actionCallback)(void*));
  // Called after sync at the barrier (after all clients have ActionCallback'ed).
  void setPostSyncCallback
    (bool (*postSyncCallback)(void*));
  // Called while disconnected.
  void setNullCallback
    (bool (*nullCallback)(void*));

  void setServiceName(string serviceName);
  void setNetworks(string networks);
  bool init(arSZGClient&);
  bool start();
  void stop();
  void consume();
  void skipConsumption();
  const string& getLabel() const;

  int getServerSendSize() const;
  int getFrameTime() const;
  int getProcTime() const;
  int getActionTime() const;
  int getRecvTime() const;
  int getRecvSize() const;
  bool syncClient() const { return _mode == AR_SYNC_CLIENT; }

 protected:
  void _connectionTask();

 private:
  arSZGClient* _client;
  string       _serviceName;
  string       _serviceNameBarrier;
  string       _networks;
  int _mode; // sync buffer consumption with server, or not.
  int  _dataAvailable;
  bool _bufferSwapReady;
  list<pair<char*, int> > _receiveStack;
  list<pair<char*, int> > _storageStack;
  list<pair<char*, int> > _consumeStack;
  arLock                 _stackLock; // For all three of these stacks.
  ARchar* _data[2];
  ARint _dataSize[2];
  int _backBuffer;

  bool _stateClientConnected;
  bool _exitProgram;
  bool _readThreadRunning;
  bool _connectionThreadRunning;

  void* _bondedObject;

  bool (*_connectionCallback)(void*, arTemplateDictionary*);
  bool (*_disconnectCallback)(void*);
  bool (*_consumptionCallback)(void*, ARchar*);
  bool (*_actionCallback)(void*);
  bool (*_nullCallback)(void*);
  bool (*_postSyncCallback)(void*);

  arLock _swapLock; // with _bufferSwapCondVar, _dataWaitCondVar
  arConditionVar _bufferSwapCondVar;
  arConditionVar _dataWaitCondVar;
  arDataClient _dataClient;
  arBarrierClient _barrierClient;

  arThread _connectionThread;
  arThread _consumptionThread;
  arThread _readThread;

  bool _firstConsumption;

  // performance tuning parameters
  float _frameTime;
  float _recvTime;
  float _recvSize;
  float _oldRecvTime;
  float _oldRecvSize;
  float _drawTime;
  float _procTime;
  float _serverSendSize;

  // we guarantee that at least one _nullCallback is executed upon
  // disconnection. This is necessary if, say, something needs to
  // be executed on disconnect in the consume thread (and the most
  // convenient way to do this is in the _nullCallback).
  arLock         _nullHandshakeLock; // with _nullHandshakeVar
  arConditionVar _nullHandshakeVar;
  int            _nullHandshakeState;

  // only used for the local connection case
  arSyncDataServer* _syncServer;

  // IIR filter for smoothing data.
  inline void _update(float& value, float newValue, float filter);

  void _readTask();
};

#endif
