//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_PEER_H
#define AR_GRAPHICS_PEER_H

#include "arSZGClient.h"
#include "arDataServer.h"
#include "arGraphicsDatabase.h"
#include "arQueuedData.h"

class SZG_CALL arGraphicsPeerConnection{
 public:
  arGraphicsPeerConnection(){ remoteName = "NULL"; 
                              connectionID = -1;
                              receiving = false;
                              sending = false; 
                              inMap = NULL; }
  ~arGraphicsPeerConnection(){};

  string    remoteName;
  // Critical that we use the connection ID... this is one thing that is
  // actually unique about the connection. The remoteName is not unique
  // (though it is at any particular time) since we could have open, then
  // closed, then opened again, a connection between two peers. 
  int       connectionID;
  arSocket* socket;
  bool      receiving;
  bool      sending;
  list<int> nodesLockedLocal;
  list<int> nodesLockedRemote;

  // If the remote peer *mapped* into us, then there needs to be a
  // basic ID-mapping process that happens before the more general
  // filter. This allows IDs of the incoming records to be *mapped*
  // to the appropriate IDs locally. This must happen *before*
  // the records hit the inFilter, since that operates on *local* ID.
  map<int, int, less<int> >* inMap;

  // Each connection includes information about how stuff on it should
  // be filtered going out.
  map<int, int, less<int> > outFilter;

  // Each connection includes information on how stuff on it should be
  // filtered coming in. NOTE: Locks will eventually be implemented
  // this way, I think.
  map<int, int, less<int> > inFilter;

  string print();
};

class SZG_CALL arGraphicsPeer: public arGraphicsDatabase{
  friend void ar_graphicsPeerConsumptionFunction(arStructuredData*,
					         void*,
					         arSocket*);
  friend void ar_graphicsPeerDisconnectFunction(void*, arSocket*);
  friend void ar_graphicsPeerConnectionTask(void*);
 public:
  arGraphicsPeer();
  ~arGraphicsPeer();

  void setName(const string&);

  bool init(arSZGClient& client);
  bool init(int& argc, char** argv);
  bool start();
  void stop();

  arDatabaseNode* alter(arStructuredData*);

  // These functions are virtual so that they, by default, can use a path.
  // (as set through the arSZGClient).
  virtual bool readDatabase(const string& fileName, const string& path="");
  virtual bool readDatabaseXML(const string& fileName, const string& path="");
  virtual bool attachXML(arDatabaseNode* parent, const string& fileName,
			 const string& path="");
  virtual bool mapXML(arDatabaseNode* parent, const string& fileName,
		      const string& path="");
  virtual bool writeDatabase(const string& fileName, const string& path="");
  virtual bool writeDatabaseXML(const string& fileName, 
                                const string& path="");
  virtual bool writeRootedXML(arDatabaseNode* parent,
                              const string& fileName,
                              const string& path="");

  // These functions are essentially set-up, not day-to-day usage.
  void useLocalDatabase(bool);
  void queueData(bool);
  bool consume();
  
  // These form the most important part of the API.
  int connectToPeer(const string& name);
  bool closeConnection(const string& name);
  bool receiving(const string& name, bool state);
  bool sending(const string& name, bool state);
  bool pullSerial(const string& name, bool receiveOn);
  bool pushSerial(const string& name, bool sendOn);
  bool closeAllAndReset();
  bool lockRemoteNode(const string& name, int nodeID);
  bool unlockRemoteNode(const string& name, int nodeID);
  bool lockLocalNode(const string& name, int nodeID);
  bool unlockLocalNode(int nodeID);
  bool filterDataBelowRemote(const string& peer,
                             int remoteNodeID, int on);
  bool filterDataBelowLocal(const string& peer,
                            int localNodeID, int on);
  int  getNodeIDRemote(const string& peer, const string& nodeName);

  // Not quite so important.
  //list<arGraphicsPeerConnection> getConnections();
  string printConnections();
  string print();

 protected:
  string          _name;
  arQueuedData*   _incomingQueue;
  arDataServer*   _dataServer;
  // The IDs of the sockets that are sending information.
  // We will just rely on the list of connections in general for now.
  //list<arSocket*> _outgoingSockets;
  arSZGClient*    _client;
  arThread        _connectionThread;

  arMutex _socketsLock;
  arMutex _alterLock;
  arMutex _queueLock;
  
  bool    _queueingData;
  bool    _localDatabase;

  // It is possible to lock a node (preventing futher changes to said
  // node). (map of nodes to connection IDs)
  map<int, int, less<int> > _lockContainer;
  // Need to keep a list of connections with their properties.
  // (map of connectionIDs to nodes is contained herein)
  map<int, arGraphicsPeerConnection*, less<int> > _connectionContainer;

  // Some calls involve a round trip to a remote peer, like finding the
  // ID of a node if we haven't transfered the stuff locally.
  arMutex        _IDResponseLock;
  arConditionVar _IDResponseVar;
  int            _requestedNodeID;

  // Also, involving a round trip to the remote peer is a request to dump
  arMutex        _dumpLock;
  arConditionVar _dumpVar;
  bool           _dumped;

  // A path for reading and writing info might be specified.
  string _readWritePath;

  bool _setRemoteLabel(arSocket* sock, const string& name);
  bool _serializeAndSend(arSocket* socket, bool sendOn);
  void _serializeDoneNotify(arSocket* socket);

  void _activateSocket(arSocket*);
  void _deactivateSocket(arSocket*);
  void _closeConnection(arSocket*);
  void _lockNode(int nodeID, arSocket* socket);
  void _unlockNode(int nodeID);
  int  _unlockNodeNoNotification(int nodeID);
  void _filterDataBelow(int nodeID,
                        arSocket* socket,
                        int on);
  void _recSerialize(arDatabaseNode* pNode, arStructuredData& nodeData,
                     arSocket* socket, bool& success);
  void _recDataOnOff(arDatabaseNode* pNode,
                     int value,
                     map<int, int, less<int> >& filterMap);
};

#endif
