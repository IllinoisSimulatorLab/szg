//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SYNC_DATA_SERVER
#define AR_SYNC_DATA_SERVER

#include "arStructuredData.h"
#include "arQueuedData.h"
#include "arDataUtilities.h"
#include "arTemplateDictionary.h"
// VERY OBNOXIOUS THAT I HAVE TO EXPLICITLY REFERENCE arDatabaseNode HERE!
// THIS JUST SIGNALS THE BAD DESIGN THAT GOT ROLLED INTO THE arSyncDataServer!
#include "arDatabaseNode.h"
#include "arDataServer.h"
#include "arBarrierServer.h"
#include "arSZGClient.h"
#include <list>
using namespace std;

//******************************************************
// These constants need to be public! Applications need
// to be able to use them in the setMode() call.
//******************************************************
// Synchronization modes (for _mode).
  enum {
    AR_SYNC_AUTO_SERVER = 0,
    AR_SYNC_MANUAL_SERVER,
    AR_NOSYNC_MANUAL_SERVER
  };

class arSyncDataClient;

/// Server for arSyncDataClient objects.
class arSyncDataServer{
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_syncDataServerConnectionTask(void*);
  friend void ar_syncDataServerSendTask(void*);
  // We let the arSyncDataClient access some of the private members
  // in the case of a local connection
  friend class arSyncDataClient;
 public:
  arSyncDataServer();
  ~arSyncDataServer();

  bool setMode(int);
  bool setDictionary(arTemplateDictionary*);
  void setBondedObject(void*);
  void autoBufferSwap(bool);
  void setConnectionCallback
    (bool (*connectionCallback)(void*,arQueuedData*,list<arSocket*>*));
  void setMessageCallback
    (arDatabaseNode* (*messageCallback)(void*,arStructuredData*));

  void setServiceName(string serviceName);
  void setChannel(string channel);
  bool init(arSZGClient& client);
  bool start();
  void stop();

  void swapBuffers();

  arDatabaseNode* receiveMessage(arStructuredData*);

  arDataServer* const dataServer() const
    { return (arDataServer* const)&_dataServer; }

 private:
  arSZGClient* _client;
  string       _serviceName;
  string       _serviceNameBarrier;
  arDataServer _dataServer;
  int _mode;
  arBarrierServer _barrierServer;
  arQueuedData* _dataQueue;
  arSignalObject _signalObject;
  arSignalObject _signalObjectRelease;
  arMutex _queueLock;
  arMutex _messageLock;
  arConditionVar _messageBufferVar;
  bool _messageBufferFull;
  enum { _sendLimit = 100000 /* bytes */ };

  arTemplateDictionary* _dictionary;
  void* _bondedObject;
  bool _autoBufferSwap;

  arThread _connectionThread;
  arThread _sendThread;

  bool _exitProgram;
  bool _sendThreadRunning;

  bool (*_connectionCallback)(void*,arQueuedData*,list<arSocket*>*);
  arDatabaseNode* (*_messageCallback)(void*,arStructuredData*);

  string _channel;

  // Variables related to the local connection, if we are operating in 
  // that mode (i.e. on arSyncDataServer and one arSyncDataClient in a 
  // single process, as is useful for standalone mode)
  bool           _locallyConnected;
  int            _localConsumerReady;
  arMutex        _localConsumerReadyLock;
  arConditionVar _localConsumerReadyVar;
  int            _localProducerReady;
  arMutex        _localProducerReadyLock;
  arConditionVar _localProducerReadyVar;

  void _sendTask();
};

#endif
