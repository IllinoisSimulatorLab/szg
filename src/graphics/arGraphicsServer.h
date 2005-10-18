//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_SERVER_H
#define AR_GRAPHICS_SERVER_H

#include "arGraphicsDatabase.h"
#include "arSyncDataServer.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// Creates a scene graph for rendering by arGraphicsClient objects.

class SZG_CALL arGraphicsServer: public arGraphicsDatabase {
  // Needs assignment operator and copy constructor, for pointer member.
  friend bool
    ar_graphicsServerConnectionCallback(void*,arQueuedData*,list<arSocket*>*);
  friend arDatabaseNode*
    ar_graphicsServerMessageCallback(void*,arStructuredData*);
 public:
  arGraphicsServer();
  ~arGraphicsServer();

  bool init(arSZGClient& client);
  bool start();
  void stop();

  // Default should be false (i.e. we do not need an extra reference
  // tacked on to the indicated node in the case of node creation).
  arDatabaseNode* alter(arStructuredData*, bool refNode=false);

  arSyncDataServer _syncServer; 

 protected:
  arQueuedData* _connectionQueue;

 private:
  bool _connectionCallback(list<arSocket*>*);
  void _recSerialize(arDatabaseNode* data, arStructuredData& nodeData);
};

#endif
