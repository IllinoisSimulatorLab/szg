//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MASTER_SLAVE_DATA_ROUTER
#define AR_MASTER_SLAVE_DATA_ROUTER

#include "arFrameworkObject.h"
#include "arBuffer.h"
#include "arStructuredDataParser.h"
#include "arFrameworkCalling.h"

#include <map>
using namespace std;

/*
Manage a collection of arFrameworkObjects:
  Create a shared language that can transfer data.
  Collect buffers of messages from the master to the slaves.
  Dump and transfer state.
  Route messages from itself to its individual arFrameworkObjects.
*/

class SZG_CALL arMasterSlaveDataRouter: public arFrameworkObject{
 public:
  arMasterSlaveDataRouter();
  ~arMasterSlaveDataRouter();

  // first, the virtual functions inherited from arFrameworkObject...
  // NOTE: only draw() is redefined.
  // DO NOT MAKE THIS CONST!
  virtual void draw();
  // To handle translations (like little-endian to big-endian), we need
  // to know the binary format of the remote data.
  virtual void setRemoteStreamConfig(const arStreamConfig& c);

  // Creates the language and the internal arStructuredDataParser.
  // Must be called after all of the registerFrameworkObject calls since
  // they add record types to the language.
  virtual bool start();

  // Adds a framework object to the collection managed by this router object
  bool registerFrameworkObject(arFrameworkObject* object);

  // GRUMBLE... THESE NEXT FUNCTIONS ARE A LITTLE ANNOYING... for instance,
  // routeMessages is very much like receiveData... and the combination of
  // internalDumpState() and getTransferBuffer(...) is very much like dumpData.
  // Think of the way arGraphicsDatabase works. It's not surprising there are
  // similarities, since I'm working off that concept.

  // Makes the managed objects dump state into the internal buffer
  // NOTE: some objects might have not changed since the last time they
  // dumped state. In which case, they will not actually make a contribution.
  void internalDumpState();

  // Routes messages from the given buffer.
  bool routeMessages(char* buffer, int bufferSize);

  // We need to be able to send the current state of the world.
  // (maybe this statement is a little misleading since sometimes we might
  //  not want to dump all objects every time... consider a collection of
  //  objects where only one gets changed per frame).
  char* getTransferBuffer(int& bufferSize);

 protected:
  arStructuredDataParser*                  _parser;
  arBuffer<char>                           _buffer;
  int                                      _bufferPosition;
  map<int, arFrameworkObject*, less<int> > _objectTable;
  bool                                     _started;
  int                                      _nextID;

  void _addStructuredDataToBuffer(int objectID, arStructuredData* data);
};

#endif
