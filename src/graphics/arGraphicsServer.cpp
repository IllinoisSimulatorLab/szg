//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This file is massively duplicated with arSoundServer.cpp.

#include "arPrecompiled.h"
#include "arGraphicsServer.h"
#include "arLogStream.h"

bool ar_graphicsServerConnectionCallback(void* server,
                                         arQueuedData*,
                                         list<arSocket*>* socketList) {
  return ((arGraphicsServer*)server)->_connectionCallback(socketList);
}

// Clone the database and send it to arGraphicsClient objects.
// This is where a node's dumpData() method gets (conventionally) called,
// when a database is created.  dumpData() is not called every time a node's
// contents change.  That's done entirely through alter() and receiveMessage().
bool arGraphicsServer::_connectionCallback(list<arSocket*>* socketList) {
  if (!_connectionQueue) {
    return false;
  }
  // If we've set up a "remote" texture path, this needs to be
  // copied over to the szgrender on the other side.
  if (_bundlePathName != "NULL" && _bundleName != "NULL") {
    arStructuredData pathData(_gfx.find("graphics admin"));
    pathData.dataInString("action", "remote_path");
    pathData.dataInString("name", _bundlePathName+"/"+_bundleName);
    _connectionQueue->forceQueueData(&pathData);
  }
  arStructuredData nodeData(_gfx.find("make node"));
  _recSerialize(&_rootNode, nodeData);
  _connectionQueue->swapBuffers();

  bool ok = true;
  arDataServer* dataServer = _syncServer.dataServer();
  for (list<arSocket*>::iterator iSocket = socketList->begin();
       iSocket != socketList->end();
       ++iSocket) {
    if (!dataServer->sendDataQueue(_connectionQueue, *iSocket)) {
      ar_log_error() << "arGraphicsServer failed to send connection.\n";
      ok = false;
    }
  }
  return ok;
}

arDatabaseNode* ar_graphicsServerMessageCallback(void* server,
                                                 arStructuredData* data) {
  // NOTE: we do NOT allow node creation ref'ing here.
  // Such is dealt with inside the receiveMessage call of the arSyncDataServer.
  return ((arGraphicsServer*)server)->arGraphicsDatabase::alter(data);
}

arGraphicsServer::arGraphicsServer() :
  _connectionQueue(new arQueuedData())
{
  _server = true; // arDatabase::_server
  (void)_syncServer.setDictionary(_gfx.getDictionary());
  _syncServer.setBondedObject(this);
  _syncServer.setMessageCallback(ar_graphicsServerMessageCallback);
  _syncServer.setConnectionCallback(ar_graphicsServerConnectionCallback);
}

arGraphicsServer::~arGraphicsServer() {
  if (_connectionQueue)
    delete _connectionQueue;
}

// Initialize.  But don't start threads yet.
bool arGraphicsServer::init(arSZGClient& client) {
  _syncServer.setServiceName("SZG_GEOMETRY");
  _syncServer.setChannel("graphics");
  return _syncServer.init(client);
}

// Start threads.
bool arGraphicsServer::start() {
  return _syncServer.start();
}

// Stop more-or-less deterministically.
void arGraphicsServer::stop() {
  _syncServer.stop();
}

arDatabaseNode* arGraphicsServer::alter(arStructuredData* theData,
                                        bool refNode) {
  // Serialization must occur at this level AND must use the thread-safety lock
  // for the arDatabase.
  _lock("arGraphicsServer::alter");
  arDatabaseNode* result = _syncServer.receiveMessage(theData);
  if (result && refNode) {
    result->ref();
  }
  _unlock();
  return result;
}

void arGraphicsServer::_recSerialize(arDatabaseNode* pNode,
                                     arStructuredData& nodeData) {
  // This will fail for the root node
  if (fillNodeData(&nodeData, pNode)) {
    _connectionQueue->forceQueueData(&nodeData);
    arStructuredData* theData = pNode->dumpData();
    _connectionQueue->forceQueueData(theData);
    delete theData;
  }
  // Thread-safety does NOT require using getChildrenRef instead of
  // getChildren. That would result in frequent deadlocks on connection
  // attempts. This is called from the connection callback (which is
  // called from within a locked _queueLock in arSyncDataServer). By examining
  // arSyncDataServer::receiveMessage one easily sees that any alteration to
  // the local database occurs protected by _queueLock. Thus, we are OK!
  const list<arDatabaseNode*> children = pNode->getChildren();
  for (list<arDatabaseNode*>::const_iterator i=children.begin();
       i!=children.end(); i++) {
    _recSerialize(*i, nodeData);
  }
}

arDatabaseNode* arGraphicsServer::_makeNode(const string& type) {
  if (type == "graphics plugin") {
    return (arDatabaseNode*) new arGraphicsPluginNode(true);
  }
  return arGraphicsDatabase::_makeNode(type);
}
