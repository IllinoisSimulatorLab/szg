//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This file is massively duplicated with arSoundServer.cpp.

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsServer.h"

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
  if (!_connectionQueue)
    return false;
  arStructuredData nodeData(_gfx.find("make node"));
  _recSerialize(&_rootNode, nodeData);
  _connectionQueue->swapBuffers();

  bool ok = true;
  arDataServer* dataServer = _syncServer.dataServer();
  for (list<arSocket*>::iterator iSocket = socketList->begin();
       iSocket != socketList->end();
       ++iSocket){
    if (!dataServer->sendDataQueue(_connectionQueue,*iSocket)){
      cerr << "arGraphicsServer warning: connection send failed.\n";
      ok = false;
    }
  }
  return ok;
}

arDatabaseNode* ar_graphicsServerMessageCallback(void* server,
				                 arStructuredData* data){
  return ((arGraphicsServer*)server)->arGraphicsDatabase::alter(data);
}

arGraphicsServer::arGraphicsServer() :
  _connectionQueue(new arQueuedData())
{
  _server = true; // _server is a member of arDatabase

  (void)_syncServer.setDictionary(_gfx.getDictionary());
  _syncServer.setBondedObject(this);
  _syncServer.setMessageCallback(ar_graphicsServerMessageCallback);
  _syncServer.setConnectionCallback(ar_graphicsServerConnectionCallback);
}

arGraphicsServer::~arGraphicsServer(){
  if (_connectionQueue)
    delete _connectionQueue;
}

/// No threads are started.
bool arGraphicsServer::init(arSZGClient& client){
  _syncServer.setServiceName("SZG_GEOMETRY");
  _syncServer.setChannel("graphics");
  return _syncServer.init(client);
}

/// This method starts the various threads going and must be made before any other
/// work is done by a calling application.
bool arGraphicsServer::start(){
  return _syncServer.start();
}

/// It is necessary to be able to make the arGraphicsServer stop in a
/// more-or-less deterministic fashion
void arGraphicsServer::stop(){
  _syncServer.stop();
}

arDatabaseNode* arGraphicsServer::alter(arStructuredData* theData){
  return _syncServer.receiveMessage(theData);
}

void arGraphicsServer::_recSerialize(arDatabaseNode* pNode,
				     arStructuredData& nodeData){
  // This will fail for the root node
  if (fillNodeData(&nodeData, pNode)){
    _connectionQueue->forceQueueData(&nodeData);
    arStructuredData* theData = pNode->dumpData();
    _connectionQueue->forceQueueData(theData);
    delete theData;
  }
  list<arDatabaseNode*> children = pNode->getChildren();
  list<arDatabaseNode*>::iterator i;
  for (i=children.begin(); i!=children.end(); i++){
    _recSerialize(*i, nodeData);
  }
}
