//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This file is massively duplicated with arGraphicsServer.cpp.

#include "arPrecompiled.h"
#include "arSoundServer.h"
#include "arLogStream.h"

bool ar_soundServerConnectionCallback(void* server,
                                         arQueuedData*,
                                         list<arSocket*>* socketList) {
  return ((arSoundServer*)server)->_connectionCallback(socketList);
}
 
bool arSoundServer::_connectionCallback(list<arSocket*>* socketList) {
  if (!_connectionQueue){
    return false;
  }
  // If we've set up a "remote" texture path, this needs to be
  // copied over to the szgrender on the other side.
  if (_bundlePathName != "NULL" && _bundleName != "NULL"){
    arStructuredData pathData(_langSound.find("sound_admin"));
    pathData.dataInString("action","remote_path");
    pathData.dataInString("name",_bundlePathName+"/"+_bundleName);
    _connectionQueue->forceQueueData(&pathData);
  }
  arStructuredData nodeData(_langSound.find("make node"));
  _recSerialize(&_rootNode, nodeData);
  _connectionQueue->swapBuffers();
 
  bool ok = true;
  arDataServer* dataServer = _syncServer.dataServer();
  for (list<arSocket*>::iterator iSocket = socketList->begin();
       iSocket != socketList->end();
       ++iSocket){
    if (!dataServer->sendDataQueue(_connectionQueue,*iSocket)){
      ar_log_warning() << "arSoundServer warning: connection send failed.\n";
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
 
arDatabaseNode* arSoundServer::alter(arStructuredData* theData,
                                     bool refNode){
  // Serialization must occur at this level AND must use the thread-safety lock
  // for arDatabase.
  ar_mutex_lock(&_databaseLock);
  arDatabaseNode* result = _syncServer.receiveMessage(theData);
  if (result && refNode){
    result->ref();
  }
  ar_mutex_unlock(&_databaseLock);
  return result;
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
  // Thread-safety does NOT require using getChildrenRef instead of 
  // getChildren. That would result in frequent deadlocks on connection
  // attempts. This is called from the connection callback (which is
  // called from within a locked _queueLock in arSyncDataServer). By examining
  // arSyncDataServer::receiveMessage one easily sees that any alteration to 
  // the local database occurs protected by _queueLock. Thus, we are OK!
  list<arDatabaseNode*> children = pNode->getChildren();
  for (list<arDatabaseNode*>::iterator i=children.begin(); 
       i!=children.end(); i++){
    _recSerialize(*i, nodeData);
  }
}
