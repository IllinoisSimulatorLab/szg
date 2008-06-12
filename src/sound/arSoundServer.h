//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_SERVER_H
#define AR_SOUND_SERVER_H

#include "arSoundDatabase.h"
#include "arSyncDataServer.h"
#include "arSoundCalling.h"

// Provides data on SZG_SOUND/{IP, port} for arSoundClients to render.

class SZG_CALL arSoundServer: public arSoundDatabase {
  // Needs assignment operator and copy constructor, for pointer members.
  friend bool
    ar_soundServerConnectionCallback(void*, arQueuedData*, list<arSocket*>*);
  friend arDatabaseNode*
    ar_soundServerMessageCallback(void*, arStructuredData*);
 public:
  arSoundServer();
  ~arSoundServer();
  bool init(arSZGClient& client);
  bool start();
  void stop();
  arDatabaseNode* alter(arStructuredData*, bool refNode=false);
  arSyncDataServer _syncServer;
 protected:
  arQueuedData* _connectionQueue;
 private:
  bool _connectionCallback(list<arSocket*>*);
  void _recSerialize(arDatabaseNode* data, arStructuredData& nodeData);
};

#endif
