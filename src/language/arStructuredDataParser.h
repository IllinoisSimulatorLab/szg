//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STRUCTURED_DATA_PARSER_H
#define AR_STRUCTURED_DATA_PARSER_H

#include "arStructuredData.h"
#include "arTemplateDictionary.h"
#include "arDataUtilities.h"
#include "arThread.h"
#include "arBuffer.h"
#include "arTextStream.h"
#include "arFileTextStream.h"
#include "arLanguageCalling.h"
#include <list>
#include <map>
using namespace std;

class SZG_CALL arStructuredDataSynchronizer{
 public:
  arStructuredDataSynchronizer(){}
  ~arStructuredDataSynchronizer(){}

  arMutex lock;
  arConditionVar var;
  int tag;
  bool exitFlag;
  int refCount;
};

class SZG_CALL arMessageQueueByID{
 public:
  arMessageQueueByID(){}
  ~arMessageQueueByID(){}

  list<arStructuredData*> messages;
  arMutex lock;
  arConditionVar var;
  bool exitFlag;
};

typedef map<int,list<arStructuredData*>*,less<int> > SZGrecycler;
typedef map<int,arMessageQueueByID*,less<int> > SZGmessageQueue;
typedef map<int,list<arStructuredData*>,less<int> > SZGtaggedMessageQueue;
typedef map<int,arStructuredDataSynchronizer*,less<int> > SZGtaggedMessageSync;
typedef list<arStructuredDataSynchronizer*> SZGunusedMessageSync;

/// This class converts a byte-stream or text-stream into a sequence of 
/// arStructuredData objects. It encapsulates some of the commonly used 
/// functions in a parser. For instance, one can read data from a source in a 
/// dedicated thread, which goes into internal storage. A response handler in 
/// another thread can then block until a record of a certain type is read 
/// (and receive that record). Furthermore, records can be stored internally 
/// via an application-defined tag (instead of via record type). This 
/// facilitates building asynchronous rpc type calls, for instance. Memory 
/// management is also added, which created arStructuredData records being 
/// re-used.

class SZG_CALL arStructuredDataParser{
 // Needs assignment operator and copy constructor,
 // for pointer member _dictionary (unless that's fine, in which case
 // make that explicit).
 public:
  arStructuredDataParser(arTemplateDictionary*);
  ~arStructuredDataParser();

  arStructuredData* getStorage(int ID);
  arBuffer<char>*   getTranslationBuffer();
  void              recycleTranslationBuffer(arBuffer<char>*);
  // AARGH! THIS IS ALL A MESS! The parse command needs a better model...
  // To be provided eventually...
  arStructuredData* parse(ARchar*,int&);
  arStructuredData* parse(ARchar*,int&,const arStreamConfig&);
  arStructuredData* parse(arTextStream*);
  arStructuredData* parse(arTextStream*, const string&);
  arStructuredData* parseBinary(FILE*);
  bool parseIntoInternal(ARchar*, int&);
  bool parseIntoInternal(arTextStream*);
  bool pushIntoInternalTagged(arStructuredData*, int);
  arStructuredData* getNextInternal(int ID);
  int getNextTaggedMessage(arStructuredData*& message,
                           list<int> tags,
                           int dataID = -1,
                           int timeout = -1);
  
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

  /// Serialize access to the complex message storage structures
  arMutex _globalLock; 
  /// Serialize access to the store of unused message storage
  arMutex _recycleLock;
  /// Deals with the clearQueues/activateQueues calls
  arMutex _activationLock;
  // Should we be allowing "clients" to grab data from the various queues?
  bool _activated;
  /// Serialize access to the list of translation buffers
  arMutex _translationBufferListLock; 

  void _pushOntoQueue(arStructuredData* theData);
  void _pushOntoTaggedQueue(int tag, arStructuredData* theData);
  void _cleanupSynchronizers(list<int> tags);
};

#endif
