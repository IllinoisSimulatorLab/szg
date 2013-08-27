//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_SERVER
#define AR_DATA_SERVER

#include "arDataPoint.h"
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arStructuredDataParser.h"
#include "arDataUtilities.h"
#include "arQueuedData.h"
#include "arLanguageCalling.h"
#include <list>
#include <map>

// Send data to arDataClient objects.

class SZG_CALL arDataServer : public arDataPoint {
 // Needs assignment operator and copy constructor, for pointer members.
 friend void ar_readDataThread(void*);
 public:
   arDataServer(int dataBufferSize);
   virtual ~arDataServer();

   // If incoming packets are processed by a consumption callback,
   // packets consumed in different threads (coming from different sockets)
   // are consumed atomically by default.
   // Call atomicReceive(false) to let consumptions overlap.
   void atomicReceive(bool);

   // set IP:port on which server listens;  default is INADDR_ANY.
   bool setInterface(const string&);
   bool setPort(int);
   bool beginListening(arTemplateDictionary*);

   int getPort() const
     { return _portNumber; }
   const string& getInterface() const
     { return _interfaceIP; }

   // delete a connection (whatever that means)
   bool removeConnection(int id);
   // accept a single new connection
   arSocket* acceptConnection()
     { return _acceptConnection(true); };
   // accept a single new connection... but do not add to the sending list yet
   arSocket* acceptConnectionNoSend()
     { return _acceptConnection(false); };
   // add the passive sockets to the send queue
   void activatePassiveSockets();
   void activatePassiveSocket(int);
   // are there any passive sockets?
   bool checkPassiveSockets();
   list<arSocket*>* getActiveSockets();

   // Send data to all connected clients.
   bool sendData(arStructuredData*);
   bool sendDataQueue(arQueuedData*);

   // Send data to someone in particular.
   bool sendData(arStructuredData*, arSocket*);
   bool sendDataNoLock(arStructuredData*, arSocket*);
   bool sendDataQueue(arQueuedData*, arSocket*);

   // Send data to a group of someone's in particular.
   bool sendDataQueue(arQueuedData*, list<arSocket*>*);

   // NOTE: setConsumerCallback calls setConsume(true)
   // subclasses that override onConsume() will need to
   // explicitly call this.
   void setConsume( bool onoff ) { _consumeData = onoff; }
   virtual void onConsumeData( arStructuredData* data, arSocket* socket );
   void setConsumerCallback
     (void (*consumerCallback)(arStructuredData*, void*, arSocket*));
   void setConsumerObject(void*);

   int getNumberConnected() const       // passive and active connections
     { return _numberConnected; }
   int getNumberConnectedActive() const // exclude passive connections
     { return _numberConnectedActive; }

   virtual void onDisconnect( arSocket* theSocket );
   void setDisconnectCallback
     (void (*disconnectCallback)(void*, arSocket*));
   void setDisconnectObject(void*);

   // For the database of labels, and choosing a connection based on a given ID
   string dumpConnectionLabels();
   arSocket* getConnectedSocket(int theSocketID);
   arSocket* getConnectedSocketNoLock(int theSocketID);
   void setSocketLabel(arSocket* theSocket, const string& theLabel);
   string getSocketLabel(int theSocketID);
   int getFirstIDWithLabel(const string& theSocketLabel);

   // Accept connections.
   int dialUpFallThrough(const string& s, int port);

   // Restrict connections to certain IPs (see arSocket and arSocketAddress).
   void setAcceptMask(list<string>& mask) { _acceptMask = mask; }

 private:
   string _interfaceIP;
   int _portNumber;
   arSocket* _listeningSocket;      // only need to listen on one socket
   // todo: _connectionSockets is redundant with _connectionIDs.  Use only _connectionIDs.
   list<arSocket*> _connectionSockets; // Active connections

   // Database of connected sockets.
   int _numberConnected;
   int _numberConnectedActive;
   int _nextID;  // The next socket will get this ID.
   map<int, string, less<int> >         _connectionLabels;  // all communications points have
                                                          // a text label.
   map<int, arSocket*, less<int> >      _connectionIDs;     // map from ID to comm point
   map<int, arStreamConfig, less<int> > _connectionConfigs; // remote stream's binary data format

   // Helpers for _connectionLabels and _connectionIDs
   void _addSocketLabel(arSocket*, const string&);
   bool _delSocketLabel(arSocket*);
   void _addSocketID(arSocket*);
   bool _delSocketID(arSocket*);

   void _addSocketToDatabase(arSocket*);
   void _deleteSocketFromDatabase(arSocket*);
   void _setSocketRemoteConfig(arSocket*, const arStreamConfig&);
   arSocket* _acceptConnection(bool);

   arLock _lockTransfer; // Serializes socket lists and _dataBuffer.

   arTemplateDictionary*   _theDictionary;
   arStructuredDataParser* _dataParser;

   arSocket* _nextConsumer;
   list<arSocket*> _pendingConsumers;
   arLock _lockConsume; // Serialize data consumption.

   bool _consumeData;
   void (*_consumerCallback)(arStructuredData*, void*, arSocket*);
   void* _consumerObject;
   void (*_disconnectCallback)(void*, arSocket*);
   void* _disconnectObject;

   bool _atomicReceive;

   list<arSocket*> _passiveSockets;

   arSignalObject  _threadLaunchSignal;

   list<string>    _acceptMask;

   void _readDataTask();
   // To a specific socket.
   bool _sendDataCore(const ARchar* theBuffer, const int theSize, arSocket* fd);
   // To all active sockets.
   bool _sendDataCore(const ARchar* theBuffer, const int theSize);
};

#endif
