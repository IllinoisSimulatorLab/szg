//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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
#include <list>
#include <map>
using namespace std;

class arStructuredDataSynchronizer{
 public:
  arStructuredDataSynchronizer(){}
  ~arStructuredDataSynchronizer(){}

  arMutex* lock;
  arConditionVar* var;
  int* tag;
};

typedef map<int,list<arStructuredData*>*,less<int> > SZGrecycler;
typedef map<int,list<arStructuredData*>,less<int> > SZGmessageQueue;
typedef map<int,arMutex*,less<int> > SZGmessageQueueLock;
typedef map<int,arConditionVar*,less<int> > SZGmessageQueueVar;
typedef map<int,list<arStructuredData*>,less<int> > SZGtaggedMessageQueue;
typedef map<int,arStructuredDataSynchronizer,less<int> > SZGtaggedMessageSync;
typedef list<arStructuredDataSynchronizer> SZGunusedMessageSync;

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

class arStructuredDataParser{
 // Needs assignment operator and copy constructor,
 // for pointer member _dictionary (unless that's fine, in which case
 // make that explicit).
 public:
  arStructuredDataParser(arTemplateDictionary*);
  ~arStructuredDataParser();

  arStructuredData* getStorage(int ID);
  arBuffer<char>*   getTranslationBuffer();
  void              recycleTranslationBuffer(arBuffer<char>*);
  arStructuredData* parse(ARchar*,int&);
  arStructuredData* parse(ARchar*,int&,const arStreamConfig&);
  arStructuredData* parse(arTextStream*);
  arStructuredData* parse(arTextStream*, const string&);
  bool parseIntoInternal(ARchar*, int&);
  bool parseIntoInternal(arTextStream*);
  bool pushIntoInternalTagged(arStructuredData*, int);
  arStructuredData* getNextInternal(int ID);
  int getNextTaggedMessage(arStructuredData*& message,
                           list<int> tags,
                           int dataID = -1,
                           int timeout = -1);
  
  void recycle(arStructuredData*);
 private:
  SZGrecycler recycling;
  SZGmessageQueue       _messageQueue;
  SZGmessageQueueLock   _messageQueueLock;
  SZGmessageQueueVar    _messageQueueVar;
  SZGtaggedMessageQueue _taggedMessages;
  SZGtaggedMessageSync  _messageSync;
  SZGunusedMessageSync  _recycledSync;
  arTemplateDictionary* _dictionary;
  list<arBuffer<char>*> _translationBuffers;

  ///< Serialize access to the data storage pool.
  arMutex _globalLock; 
  /// Serialize access to the list of translation buffers
  arMutex _translationBufferListLock; 

  void _pushOntoQueue(arStructuredData* theData);
  void _pushOntoTaggedQueue(int tag, arStructuredData* theData);
  void _cleanupSynchronizers(list<int> tags);
};

#endif
