//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_PEER_H
#define AR_GRAPHICS_PEER_H

#include "arSZGClient.h"
#include "arDataServer.h"
#include "arGraphicsDatabase.h"
#include "arQueuedData.h"
#include "arGraphicsCalling.h"

// Hack.
class SZG_CALL arGraphicsPeerCullObject{
 public:
  arGraphicsPeerCullObject() {}
  ~arGraphicsPeerCullObject() {}

  void clear();
  void frame();
  void insert(int ID, int state);

  arNodeMap cullOnOff;
  list<int> cullChangeOn;
  list<int> cullChangeOff;
};

class SZG_CALL arGraphicsPeerUpdateInfo{
 public:
  arGraphicsPeerUpdateInfo() { invalidUpdateTime = true; }
  ~arGraphicsPeerUpdateInfo() {}

  bool invalidUpdateTime;
  ar_timeval lastUpdate;
};

class SZG_CALL arGraphicsPeerConnection{
 public:
  arGraphicsPeerConnection();
  ~arGraphicsPeerConnection() {};

  string    remoteName;
  // Critical that we use the connection ID... this is one thing that is
  // actually unique about the connection. The remoteName is not unique
  // (though it is at any particular time) since we could have open, then
  // closed, then opened again, a connection between two peers.
  int       connectionID;
  arSocket* socket;
  list<int> nodesLockedLocal;
  list<int> nodesLockedRemote;

  // If the remote peer *mapped* into us, then there needs to be a
  // basic ID-mapping process that happens before the more general
  // filter. This allows IDs of the incoming records to be *mapped*
  // to the appropriate IDs locally. This must happen *before*
  // the records hit the inFilter, since that operates on *local* ID.
  arNodeMap inMap;
  // The in map starts somewhere, by default at the root node, but it
  // could be elsewhere.
  arDatabaseNode* rootMapNode;
  // Controls whether mapped nodes should send messages back to the mapping
  // peer or not.
  arNodeLevel sendLevel;

  // To be able to filter "transient" nodes on this side, we need to
  // know the remote frame time.
  int remoteFrameTime;

  // Each connection includes information about how stuff on it should
  // be filtered going out.
  arNodeMap outFilter;

  // Need to hold info on when a node is updated (for filtering messages
  // to transient nodes).
  map<int, arGraphicsPeerUpdateInfo, less<int> > transientMap;

  string print();
};

class SZG_CALL arGraphicsPeer: public arGraphicsDatabase{
  friend void ar_graphicsPeerSerializeFunction(void*);
  friend void ar_graphicsPeerConnectionDeletionFunction(void*);
  friend void ar_graphicsPeerConsumptionFunction(arStructuredData*,
                                                 void*,
                                                 arSocket*);
  friend void ar_graphicsPeerDisconnectFunction(void*, arSocket*);
  friend void ar_graphicsPeerConnectionTask(void*);
 public:
  arGraphicsPeer();
  ~arGraphicsPeer();

  string getName() { return _name; }
  void setName(const string&);

  bool init(arSZGClient& client);
  // Included solely to handle a disambiguation bug in SWIG.
  bool initSZG(arSZGClient& client) { return init(client); }
  bool init(int& argc, char** argv);
  bool start();
  void stop();

  void setBridge(arGraphicsDatabase* database) {
    _bridgeDatabase = database;
    _bridgeRootMapNode = database->getRoot();
  }
  void setBridgeRootMapNode(arDatabaseNode* node) {
    _bridgeRootMapNode = node;
  }

  // Default should be false (i.e. we do not need an extra reference
  // tacked on to the indicated node in the case of node creation).
  arDatabaseNode* alter(arStructuredData*, bool refNode=false);

  // These functions are virtual so that they, by default, can use a path.
  // (as set through the arSZGClient).
  virtual bool readDatabase(const string& fileName, const string& path="");
  virtual bool readDatabaseXML(const string& fileName, const string& path="");
  virtual bool attach(arDatabaseNode* parent, const string& fileName,
                      const string& path="");
  virtual bool attachXML(arDatabaseNode* parent, const string& fileName,
                         const string& path="");
  virtual bool merge(arDatabaseNode* parent, const string& fileName,
                     const string& path="");
  virtual bool mergeXML(arDatabaseNode* parent, const string& fileName,
                        const string& path="");
  virtual bool writeDatabase(const string& fileName, const string& path="");
  virtual bool writeDatabaseXML(const string& fileName,
                                const string& path="");
  virtual bool writeRooted(arDatabaseNode* parent,
                           const string& fileName,
                           const string& path="");
  virtual bool writeRootedXML(arDatabaseNode* parent,
                              const string& fileName,
                              const string& path="");

  // These functions are essentially set-up, not day-to-day usage.
  void useLocalDatabase(bool);
  void queueData(bool);
  int consume();

  // These form the most important part of the API.
  int connectToPeer(const string& name);
  bool closeConnection(const string& name);
  bool pullSerial(const string& name, int remoteRootID, int localRootID,
                  arNodeLevel sendLevel,
                  arNodeLevel remoteSendLevel, arNodeLevel localSendLevel);
  bool pushSerial(const string& name, int remoteRootID, int localRootID,
                  arNodeLevel sendLevel,
                  arNodeLevel remoteSendLevel, arNodeLevel localSendLevel);
  bool closeAllAndReset();
  bool pingPeer(const string& name);
  bool broadcastFrameTime(int frameTime);
  bool remoteLockNode(const string& name, int nodeID);
  bool remoteLockNodeBelow(const string& name, int nodeID);
  bool remoteUnlockNode(const string& name, int nodeID);
  bool remoteUnlockNodeBelow(const string& name, int nodeID);
  bool localLockNode(const string& name, int nodeID);
  //bool localLockNodeBelow(const string& name, int nodeID);
  bool localUnlockNode(int nodeID);
  //bool localUnlockNodeBelow(int nodeID);
  bool remoteFilterDataBelow(const string& peer,
                             int remoteNodeID, arNodeLevel level);
  bool mappedFilterDataBelow(int localNodeID, arNodeLevel level);
  bool localFilterDataBelow(const string& peer,
                            int localNodeID, arNodeLevel level);
  int  remoteNodeID(const string& peer, const string& nodeName);

  // Not quite so important.
  //list<arGraphicsPeerConnection> getConnections();
  string printConnections();
  string printPeer();

  // a hack to enable us to do motion culling independently of drawing.
  void motionCull(arGraphicsPeerCullObject*, arCamera*);

 protected:
  string          _name;
  arQueuedData*   _incomingQueue;
  arDataServer*   _dataServer;
  arSZGClient*    _client;
  arThread        _connectionThread;
  arLock _queueLock;
  bool    _queueingData;
  bool    _localDatabase;

  // It is possible to lock a node (preventing futher changes to said
  // node). (map of nodes to connection IDs)
  arNodeMap _lockContainer;
  // Need to keep a list of connections with their properties.
  // (map of connectionIDs to nodes is contained herein)
  map<int, arGraphicsPeerConnection*, less<int> > _connectionContainer;

  // Some calls involve a round trip to a remote peer, like finding the
  // ID of a node if we haven't transfered the stuff locally.
  arLock         _IDResponseLock; // with _IDResponseVar
  arConditionVar _IDResponseVar;
  int            _requestedNodeID;

  // Also, involving a round trip to the remote peer is a request to serialize
  arLock         _dumpLock; // with _dumpVar
  arConditionVar _dumpVar;
  bool           _dumped;

  // Finally, we can "ping" a connected peer. This allows us to be sure that
  // all previous messages we've sent have been processed.
  arLock _pingLock; // with _pingVar
  arConditionVar _pingVar;
  bool           _pinged;

  // We need to delay sending a ping reply until consumption of queued messages
  // has occured.
  arLock         _queueConsumeLock; // with _queueConsumeVar
  arConditionVar _queueConsumeVar;
  bool           _queueConsumeQuery;
  arLock         _queueQueryUniquenessLock;

  // A path for reading and writing info might be specified.
  string _readWritePath;

  // As a preliminary HACK for drawing a peer in a CLUSTER display environment,
  // we might have a "bridge" to a local database. The bridge database might,
  // for instance, be an arGraphicsServer embedded in an
  // arDistSceneGraphFramework.
  arGraphicsDatabase* _bridgeDatabase;
  arNodeMap _bridgeInMap;
  arDatabaseNode*           _bridgeRootMapNode;

  // To prevent loops, we keep track of the component ID (this is the Phleet
  // component ID and is determined in the init(...) method;
  int _componentID;

  // Some utility functions for dealing with the message path recording/reading
  // in the graphics peer messages.
  int _getOriginSocketID(arStructuredData* data, int fieldID);
  int _getRoutingFieldID(int dataID);
  int _getWorkingFieldID(int dataID);

  void _motionCull(arGraphicsNode*, stack<arMatrix4>&,
                   arGraphicsPeerCullObject*, arMatrix4&);
  bool _setRemoteLabel(arSocket* sock, const string& name);
  bool _serializeAndSend(arSocket* socket, int remoteRootID,
                         int localRootID,
                         arNodeLevel sendLevel,
                         arNodeLevel remoteSendLevel,
                         arNodeLevel localSendLevel);
  void _serializeDoneNotify(arSocket* socket);

  void _closeConnection(arSocket*);
  void _resetConnectionMap(int connectionID, int nodeID, arNodeLevel level);
  void _lockNode(int nodeID, arSocket* socket);
  void _lockNodeBelow(int nodeID, arSocket* socket);
  void _unlockNode(int nodeID);
  void _unlockNodeBelow(int nodeID);
  int  _unlockNodeNoNotification(int nodeID);
  void _filterDataBelow(int nodeID,
                        arSocket* socket,
                        arNodeLevel level);
  void _recSerialize(arDatabaseNode* pNode, arStructuredData& nodeData,
                     arSocket* socket, arNodeMap& outFilter,
                     arNodeLevel localSendLevel,
                     arNodeLevel sendLevel,
                     int& dataSent,
                     bool& success);
  void _recDataOnOff(arDatabaseNode* pNode,
                     int value,
                     arNodeMap& filterMap); // Call only when _lock()'ed.
  void _sendDataToBridge(arStructuredData*);
  bool _updateTransientMap(int nodeID,
                  map<int, arGraphicsPeerUpdateInfo, less<int> >& transientMap,
                  int remoteFrameTime);
};

#endif
