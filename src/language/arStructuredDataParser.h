//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STRUCTURED_DATA_PARSER_H
#define AR_STRUCTURED_DATA_PARSER_H

#include "arStructuredData.h"
#include "arTemplateDictionary.h"
#include "arDataUtilities.h"
#include "arBuffer.h"
#include "arTextStream.h"
#include "arFileTextStream.h"
#include "arLanguageCalling.h"
#include <list>
#include <map>
using namespace std;

class SZG_CALL arLockable {
  arLock l;
 protected:
  arConditionVar var; //("arLockable");
 public:
  // arLockable() : var("arLockable") {}
  arLockable(const string& s) : var(s) {}
  void lock(const char* name)
    { l.lock(name); }
  void unlock()
    { l.unlock(); }
  bool wait(int msecTimeout = -1)
    { return var.wait(l, msecTimeout); }
  void signal()
    { var.signal(); }
};

class SZG_CALL arStructuredDataSynchronizer: public arLockable {
  // lock() guards tag and exitFlag, but not refCount.
 public:
  bool exitFlag;
  int tag;
  int refCount;
  void reset()
    { exitFlag = false; tag = -1; refCount = 0; }
  arStructuredDataSynchronizer() : arLockable("arStructuredDataSynchronizer")
    { reset(); }
};

class SZG_CALL arMessageQueueByID: public arLockable {
  // lock() guards exitFlag and messages.
 public:
  bool exitFlag;
  list<arStructuredData*> messages;
  arMessageQueueByID(const string& s) : arLockable("arMessageQueueByID " + s), exitFlag(false) {}
};

typedef list<arStructuredData*> SZGdatalist;
typedef map<int, SZGdatalist*, less<int> > SZGrecycler;
typedef map<int, arMessageQueueByID*, less<int> > SZGmessageQueue;
typedef map<int, list<arStructuredData*>, less<int> > SZGtaggedMessageQueue;
typedef map<int, arStructuredDataSynchronizer*, less<int> > SZGtaggedMessageSync;
typedef list<arStructuredDataSynchronizer*> SZGunusedMessageSync;

// This class converts a byte-stream or text-stream into a sequence of
// arStructuredData objects. It encapsulates some commonly used parsing
// functions. For instance, one can read data from a source in a
// dedicated thread, which goes into internal storage. A response handler in
// another thread can then block until a record of a certain type is read
// (and receive that record). Furthermore, records can be stored internally
// via an application-defined tag (instead of via record type), e.g. for
// building asynchronous rpc type calls.
// Memory management lets arStructuredData records be reused.

class SZG_CALL arStructuredDataParser{
 // Needs assignment operator and copy constructor,
 // for pointer member _dictionary (unless that's fine, in which case
 // make that explicit).

 public:
  arStructuredDataParser(arTemplateDictionary*);
  ~arStructuredDataParser();

  arStructuredData* getStorage(int ID);
  arBuffer<char>*   getTranslationBuffer();
  void recycleTranslationBuffer(arBuffer<char>*);

  // AARGH! THIS IS ALL A MESS! The parse command needs a better model...
  // To be provided eventually...
  arStructuredData* parse(ARchar*, int&);
  arStructuredData* parse(ARchar*, int&, const arStreamConfig&);
  arStructuredData* parse(arTextStream*);
  arStructuredData* parse(arTextStream*, const string&);
  arStructuredData* parseBinary(FILE*);
  bool parseIntoInternal(ARchar*, int&);
  bool parseIntoInternal(arTextStream*);
  bool pushIntoInternalTagged(arStructuredData*, int);
  arStructuredData* getNextInternal(int ID);
  int getNextTaggedMessage(arStructuredData*&, list<int>, int dataID=-1, int timeout=-1);
  void recycle(arStructuredData*);
  void clearQueues();
  void activateQueues();

 private:
  SZGrecycler recycling;
  SZGmessageQueue       _messageQueue;
  SZGtaggedMessageQueue _taggedMessages;
  SZGtaggedMessageSync  _messageSync;
  SZGunusedMessageSync  _recycledSync;
  arTemplateDictionary* _dictionary;
  list<arBuffer<char>*> _translationBuffers;

  arLock _globalLock; // Guard complex message storage
  arLock _recycleLock; // Guard unused message storage
  arLock _translationBufferListLock; // Guard the list of translation buffers
  // For clearQueues/activateQueues
  arLock _activationLock;
  // true iff "clients" may grab data from queues
  bool _activated;

  void _pushOntoQueue(arStructuredData* theData);
  void _pushOntoTaggedQueue(int tag, arStructuredData* theData);
  void _cleanupSynchronizers(list<int> tags);
  void _deletelist(const SZGdatalist& p);
  void _recyclelist(const SZGdatalist& p);
};

#endif
