//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This file is massively duplicated with arGraphicsServer.cpp.

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundServer.h"

bool ar_soundServerConnectionCallback(void* server,
                                         arQueuedData*,
                                         list<arSocket*>* socketList) {
  return ((arSoundServer*)server)->_connectionCallback(socketList);
}
 
bool arSoundServer::_connectionCallback(list<arSocket*>* socketList) {
  if (!_connectionQueue)
    return false;
  arStructuredData nodeData(_langSound.find("make node"));
  _recSerialize(&_rootNode, nodeData);
  _connectionQueue->swapBuffers();
 
  bool ok = true;
  arDataServer* dataServer = _syncServer.dataServer();
  for (list<arSocket*>::iterator iSocket = socketList->begin();
       iSocket != socketList->end();
       ++iSocket){
    if (!dataServer->sendDataQueue(_connectionQueue,*iSocket)){
      cout << "arSoundServer warning: connection send failed.\n";
      ok = false;
    }
  }
  return ok;
}
 
arDatabaseNode* ar_soundServerMessageCallback(void* server,
                                              arStructuredData* data){
  return ((arSoundServer*)server)->arSoundDatabase::alter(data);
}

arSoundServer::arSoundServer() :
  _connectionQueue(new arQueuedData())
{
  _server = true; // _server is a member of arDatabase

  (void)_syncServer.setDictionary(_langSound.getDictionary());
  _syncServer.setBondedObject(this);
  _syncServer.setMessageCallback(ar_soundServerMessageCallback);
  _syncServer.setConnectionCallback(ar_soundServerConnectionCallback);
}

arSoundServer::~arSoundServer(){
  if (_connectionQueue)
    delete _connectionQueue;
}

bool arSoundServer::init(arSZGClient& client){
  _syncServer.setServiceName("SZG_SOUND");
  _syncServer.setChannel("sound");
  return _syncServer.init(client);
}

bool arSoundServer::start(){
  return _syncServer.start();
}

void arSoundServer::stop(){
  _syncServer.stop();
}
 
arDatabaseNode* arSoundServer::alter(arStructuredData* theData){
  return _syncServer.receiveMessage(theData);
}

void arSoundServer::_recSerialize(arDatabaseNode* pNode,
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
