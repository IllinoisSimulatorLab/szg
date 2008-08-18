//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arStructuredDataParser.h"
#include "arXMLUtilities.h"
#include "arLogStream.h"

arStructuredDataParser::arStructuredDataParser(arTemplateDictionary* dictionary) :
  _dictionary(dictionary),
  _globalLock("DataParserGlobal"),
  _recycleLock("DataParserRecycle"),
  _translationBufferListLock("DataParserTranslate"),
  _activationLock("DataParserActive"),
  _activated(true)
  {
  // Each template in the dictionary has a queue lock and a condition variable.
  for (arTemplateType::const_iterator i = _dictionary->begin();
      i != _dictionary->end(); ++i) {
    _messageQueue.insert(SZGmessageQueue::value_type(
      i->second->getID(), new arMessageQueueByID()));
  }
}

typedef SZGdatalist::const_iterator iData;
typedef SZGmessageQueue::const_iterator iQueue;
typedef SZGtaggedMessageQueue::const_iterator iTagQueue;
typedef SZGtaggedMessageSync::const_iterator iSync;

void arStructuredDataParser::_deletelist(const SZGdatalist& p) {
  iData pEnd = p.end();
  for (iData i = p.begin(); i != pEnd; ++i)
    if (*i)
      delete *i;
}

void arStructuredDataParser::_recyclelist(const SZGdatalist& p) {
  iData pEnd = p.end();
  for (iData i = p.begin(); i != pEnd; ++i)
    recycle(*i);
}

arStructuredDataParser::~arStructuredDataParser() {
  // Delete owned storage.
  for (SZGrecycler::const_iterator i(recycling.begin()); i != recycling.end(); ++i) {
    SZGdatalist* p = i->second;
    if (p) {
      _deletelist(*p);
      delete p;
    }
  }

  // Delete messages received but not yet delivered, one queue at a time.
  // (these are the messages queued by ID, not the "tagged" messages.
  for (iQueue k(_messageQueue.begin()); k != _messageQueue.end(); ++k) {
    arMessageQueueByID* p = k->second;
    _deletelist(p->messages);
    // Delete the elements of the map as well.
    delete p;
  }

  // Delete tagged messages.
  for (iTagQueue ii = _taggedMessages.begin(); ii != _taggedMessages.end(); ii++) {
    // Unclaimed arStructuredData records.
    _deletelist(ii->second);
  }

  // Delete synchronization primitives.
  for (iSync jj = _messageSync.begin(); jj != _messageSync.end(); jj++) {
    delete jj->second;
  }

  // Delete to-be-used synchronization primitives.
  for (SZGunusedMessageSync::const_iterator kk = _recycledSync.begin();
       kk != _recycledSync.end(); kk++) {
    delete *kk;
  }

  // Delete translation buffers.
  for (list<arBuffer<char>*>::const_iterator n = _translationBuffers.begin();
       n != _translationBuffers.end(); n++) {
    delete *n;
  }
}

// Grab a data record of type ID from SZGrecycler,
// creating a new one if none such exists.

arStructuredData* arStructuredDataParser::getStorage(int ID) {
  arStructuredData* result = NULL;
  arDataTemplate* theTemplate = _dictionary->find(ID);
  if (theTemplate) {
    _recycleLock.lock();
    const SZGrecycler::const_iterator i = recycling.find(ID);
    if (i == recycling.end()) {
      // Create recycling list.
      SZGdatalist* tmp = new SZGdatalist;
      recycling.insert(SZGrecycler::value_type(ID, tmp));
    }
    else {
      SZGdatalist* j = i->second;
      if (!j->empty()) {
	// Found recycling for this ID.
	result = *(j->begin());
	j->pop_front();
      }
    }
    _recycleLock.unlock();
    if (!result)
      result = new arStructuredData(theTemplate);
  }
  return result;
}

// Get a translation buffer, creating one if needed.
arBuffer<char>* arStructuredDataParser::getTranslationBuffer() {
  arBuffer<char>* result = NULL;
  arGuard dummy(_translationBufferListLock);
  if (_translationBuffers.empty()) {
    result = new arBuffer<char>(1024);
  }
  else{
    result = _translationBuffers.front();
    _translationBuffers.pop_front();
  }
  return result;
}

// Return a translation buffer, and put it at the
// front of the list so buffers are reused often.
void arStructuredDataParser::recycleTranslationBuffer(arBuffer<char>* buffer) {
  arGuard dummy(_translationBufferListLock);
  _translationBuffers.push_front(buffer);
}

// Parse a binary stream into an arStructuredData record
// (using internal recycling if possible).
arStructuredData* arStructuredDataParser::parse(ARchar* buffer, int& end) {
  end = ar_rawDataGetSize(buffer);
  arStructuredData* result = getStorage(ar_rawDataGetID(buffer));
  if (!result) {
    ar_log_error() << "arStructuredDataParser failed to parse binary data.\n";
    return NULL;
  }
  result->unpack(buffer);
  return result;
}

// NOTE: historically, the arStructuredDataParser has been used *after*
// the arDataClient (or whatever else) has grabbed a buffer of data (either
// a single record or a queue of records) and translated them from remote
// into local format. So, there was no need to include a "translation"
// feature. However, it seems that a translation feature is, indeed,
// important. Eventually, we'll fold the arStructuredDataParser into
// the lower-level infrastructure. (i.e. it'll no longer be possible to
// use the arDataClient to get a raw buffer of data). At that time, this'll
// be useful.
// Also it might be better to fold translation
// into arStructuredData, and not need to
// double the storage for buffers in the case of translation.
arStructuredData* arStructuredDataParser::parse
                                    (ARchar* buffer, int& end,
				     const arStreamConfig& remoteStreamConfig) {
  if (remoteStreamConfig.endian == AR_ENDIAN_MODE) {
    return parse(buffer, end);
  }

  // Translate.
  arBuffer<char>* transBuffer = getTranslationBuffer();
  // Buffer grows if necessary.
  // Translated data should NOT grow.
  int size = ar_translateInt(buffer, remoteStreamConfig);
  // pass this back
  end = size;
  transBuffer->grow(size);
  int recordID = ar_translateInt(buffer+AR_INT_SIZE, remoteStreamConfig);
  arDataTemplate* theTemplate = _dictionary->find(recordID);
  if (!theTemplate ||
      theTemplate->translate(transBuffer->data, buffer, remoteStreamConfig) < 0) {
    cerr << "arStructuredDataParser error: failed to translate data record.\n";
    recycleTranslationBuffer(transBuffer);
    return NULL;
  }

  // "end=size" already handled the data's size.
  int temp = -1;
  arStructuredData* result = parse(transBuffer->data, temp);
  recycleTranslationBuffer(transBuffer);
  return result;
}

// Parse an XML text stream (using internal recycling if possible).
arStructuredData* arStructuredDataParser::parse(arTextStream* textStream) {
  const string recordBegin(ar_getTagText(textStream));
  return (recordBegin == "NULL" || ar_isEndTag(recordBegin)) ?
    NULL : parse(textStream, recordBegin);
}

#ifdef AR_USE_WIN_64

std::istream& operator>>(std::istream& is, ARint64 &i ) {
  while (isspace(is.peek()))
    (void)is.get();
  bool neg = false;
  int c;
  if ((c = is.get()) == '-' && isdigit(is.peek()))
    neg = true;
  else if (!isdigit(c)) {
    is.setf(ios::failbit);
    is.putback(c);
    return is;
  }
  is.putback(c);

  i = 0;
  for (int k = 0; k < 19; k++) {
    if (!isdigit((c = is.get()))) {
      is.putback(c);
      break;
    }
    if (unsigned __int64(i*10 + c-'0') < 0x7fffffffffffffff) {
      i = i*10 + c-'0';
    }
  }
  if (neg)
    i *= -1;
  return is;
}

#endif

// A typical stream of XML data contains "extra" tags for
// flow control. For instance, consider the below:
//
// <szg_vtk_world>
//   <world_record>
//    ...
//   </world_record>
//
//   <world_record>
//    ...
//   </world_record>
// </szg_vtk_world>
//
// The app reading this stream first reads a tag.
// If that is control-related, such as <szg_vtk_world>,
// then the app changes behavior somehow.
// Otherwise, read in the rest of the record, based on the tag's name.
arStructuredData* arStructuredDataParser::parse(arTextStream* textStream,
                                                const string& tagText) {
  // Get a resizable buffer for reading in text fields.
  arBuffer<char>* workingBuffer = getTranslationBuffer();
  arDataTemplate* theTemplate = _dictionary->find(tagText);
  if (!theTemplate) {
    cerr << "arStructuredDataParser error: dictionary has no type '" << tagText << "'.\n";
    return NULL;
  }
  // Get some storage.
  const int ID = theTemplate->getID();
  arStructuredData* theData = getStorage(ID);

  // Zero all fields' data dimensions, so unset fields have no data.
  const int cAttribute = theTemplate->getNumberAttributes();
  for (int i = 0; i < cAttribute; i++)
    theData->setDataDimension(i, 0);

  string fieldBegin;
  bool foundEndTag = false;
  int iAttribute=0;
  for (iAttribute=0; iAttribute < cAttribute; iAttribute++) {
    fieldBegin = ar_getTagText(textStream, workingBuffer);
    if (ar_isEndTag(fieldBegin)) {
      // "premature" end tag - some fields were missing (that's ok).
      foundEndTag = true;
      break;
    }
    if (fieldBegin == "NULL") {
      cerr << "arStructuredDataParser error: expected start tag at start of field name.\n";
LAbort:
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
    arDataType fieldType = theTemplate->getAttributeType(fieldBegin);
    if (fieldType == AR_GARBAGE) {
      cerr << "arStructuredDataParser error: type " << tagText
	   << " has no field named " << fieldBegin << ".\n";
      goto LAbort;
    }
    int size = -1;
    ARchar* charData = NULL;
    ARint* intData = NULL;
    ARfloat* floatData = NULL;
    ARlong* longData = NULL;
    ARint64* int64Data = NULL;
    ARdouble* doubleData = NULL;
    // Get the data from the record.
    if (!ar_getTextBeforeTag(textStream, workingBuffer)) {
      goto LAbort;
    }
    switch (fieldType) {
    case AR_CHAR:
      // Don't skip whitespace.  All other cases should, though.
      size = ar_parseXMLData<ARchar>(workingBuffer, charData, false);
      if (!charData) {
        cerr << "arStructuredDataParser error: expected XML char.\n";
	goto LAbort;
      }
      (void)theData->dataIn(fieldBegin, charData, AR_CHAR, size);
      delete [] charData;
      break;
    case AR_INT:
      size = ar_parseXMLData<ARint>(workingBuffer, intData);
      if (!intData) {
        cerr << "arStructuredDataParser error: expected XML int.\n";
	goto LAbort;
      }
      (void)theData->dataIn(fieldBegin, intData, AR_INT, size);
      delete [] intData;
      break;
    case AR_FLOAT:
      size = ar_parseXMLData<ARfloat>(workingBuffer, floatData);
      if (!floatData) {
        cerr << "arStructuredDataParser error: expected XML float.\n";
	goto LAbort;
      }
      (void)theData->dataIn(fieldBegin, floatData, AR_FLOAT, size);
      delete [] floatData;
      break;
    case AR_LONG:
      size = ar_parseXMLData<ARlong>(workingBuffer, longData);
      if (!longData) {
        cerr << "arStructuredDataParser error: expected XML long.\n";
	goto LAbort;
      }
      (void)theData->dataIn(fieldBegin, longData, AR_LONG, size);
      delete [] longData;
      break;
    case AR_INT64:
      size = ar_parseXMLData<ARint64>(workingBuffer, int64Data);
      if (!int64Data) {
        cerr << "arStructuredDataParser error: expected XML 64-bit int.\n";
	goto LAbort;
      }
      (void)theData->dataIn(fieldBegin, int64Data, AR_INT64, size);
      delete [] int64Data;
      break;
    case AR_DOUBLE:
      size = ar_parseXMLData<ARdouble>(workingBuffer, doubleData);
      if (!doubleData) {
        cerr << "arStructuredDataParser error: expected XML double.\n";
	goto LAbort;
      }
      (void)theData->dataIn(fieldBegin, doubleData, AR_DOUBLE, size);
      delete [] doubleData;
      break;
    case AR_GARBAGE:
      cerr << "arStructuredDataParser error: got AR_GARBAGE field.\n";
      goto LAbort;
    }
    string fieldEnd = ar_getTagText(textStream, workingBuffer);
    if (fieldEnd == "NULL" ||
        !ar_isEndTag(fieldEnd) ||
        fieldBegin != ar_getTagType(fieldEnd)) {
      cerr << "arStructuredDataParser error: data field not terminated with "
	   << "the correct closing tag. Field began with " << fieldBegin
	   << " and closed with " << ar_getTagType(fieldEnd) << ".\n";
      goto LAbort;
    }
  }
  if (foundEndTag) {
    // Found an end tag before reading all the fields.
    if (tagText != ar_getTagType(fieldBegin)) {
      cerr << "arStructuredDataParser error: expected closing tag for " << tagText <<
        ", not " << ar_getTagType(fieldBegin) << ".\n";
      goto LAbort;
    }
  }
  else{
    const string recordEnd = ar_getTagText(textStream, workingBuffer);
    if (recordEnd == "NULL" || !ar_isEndTag(recordEnd) ||
        tagText != ar_getTagType(recordEnd)) {
      cerr << "arStructuredDataParser error: expected closing tag for " << tagText <<
        ", not " << ar_getTagType(recordEnd) << ".\n";
      goto LAbort;
    }
  }
  // Success.
  recycleTranslationBuffer(workingBuffer);
  return theData;
}

arStructuredData* arStructuredDataParser::parseBinary(FILE* inputFile) {
  arBuffer<char>* buffer = getTranslationBuffer();
  if (fread(buffer->data, AR_INT_SIZE, 1, inputFile) == 0) {
    // Failed to read file.
    return NULL;
  }
  // buffer is at least 1024 bytes, so one int will fit.
  ARint recordSize = -1;
  ar_unpackData(buffer->data, &recordSize, AR_INT, 1);
  if (recordSize > buffer->size()) {
    buffer->resize(max(recordSize, 2*buffer->size()));
  }
  const int result = fread(buffer->data+AR_INT_SIZE, 1, recordSize-AR_INT_SIZE, inputFile);
  if (result < recordSize - AR_INT_SIZE) {
    cerr << "arStructuredDataParser error: failed to read record: read only "
         << result+AR_INT_SIZE
	 << " of " << recordSize << " expected bytes.\n";
    recycleTranslationBuffer(buffer);
    return NULL;
  }

  arStructuredData* data = getStorage(ar_rawDataGetID(buffer->data));
  if (data)
    data->unpack(buffer->data);
  recycleTranslationBuffer(buffer);
  return data;
}

// Sometimes it is advantageous to read incoming data in a thread separate
// from where it is actually used. In addition to holding raw "recycled"
// storage, the arStructuredDataParser can hold received message lists. There
// is one received message list per record type in the language. New
// messages, as parsed by "parseIntoInternal", are appended to
// the list. This allows demultiplexing via message type, for
// arSZGClient objects etc. This function
// parses data from a char buffer and stores it in one of the received
// message list. If a getNextInternal() method is waiting on such data, it
// is woken up.
bool arStructuredDataParser::parseIntoInternal(ARchar* buffer, int& end) {
  arStructuredData* theData = parse(buffer, end);
  if (!theData) {
    // the parse failed for some reason
    return false;
  }
  // we have some valid data
  _pushOntoQueue(theData);
  return true;
}

// Same as parseIntoInternal(ARchar*, int) except that here the source is a
// text file, with XML structure
bool arStructuredDataParser::parseIntoInternal(arTextStream* textStream) {
  arStructuredData* theData = parse(textStream);
  if (!theData) {
    return false;
  }
  _pushOntoQueue(theData);
  return true;
}

// The arStructuredDataParser can hold received messages in two ways.
// The first, as typified by parseIntoInternal() is useful for storing
// and later retrieving message based on message type. This method, in
// contrast, stores the message internally based on a user-defined tag.
// Records are retrieved (possibly in a different thread) via a blocking
// getNextTaggedMessage(), which uses the tag as a retrieval key. The
// idea is that this can be used to implement async rpc.
// NOTE: multiple records can be stored with a particular tag. This is
// useful when, for instance, a syzygy message can get several responses.
bool arStructuredDataParser::pushIntoInternalTagged(arStructuredData* data,
                                                    int tag) {
  if (!data) {
    return false;
  }
  _pushOntoTaggedQueue(tag, data);
  return true;
}

// Either returns the top piece of arStructuredData on the appropriate queue
// or blocks until such is available.
// NOTE: This can be interrupted, both by the time-out OR by the clearQueues()
// method, which flushes all queues, releases any outstanding waits inside
// getNextInternal or getNextTaggedMessage, etc.
arStructuredData* arStructuredDataParser::getNextInternal(int ID) {
  iQueue i(_messageQueue.find(ID));
  if (i == _messageQueue.end()) {
    ar_log_error() << "arStructuredDataParser: getNextInternal got invalid ID.\n";
    return NULL;
  }

  arMessageQueueByID* p = i->second;
  p->lock();

  // NOTE: after calling clearQueues(), we should ALWAYS fall through here
  // with an error UNTIL activateQueues() has been called.
  // (NO MATTER HOW calls to getNextInternal() were "IN FLIGHT" when the
  // original clearQueues() was issued. This allows classes that depend
  // on this to be GLOBALLY turned on/off, no matter where they live in the
  // threading universe.
  _activationLock.lock();
    if (!_activated) {
      _activationLock.unlock();
      p->unlock();
      return NULL;
    }
  _activationLock.unlock();

  while (p->messages.empty() && !p->exitFlag) {
    p->wait();
  }
  arStructuredData* result = NULL;
  if (p->exitFlag) {
    // clearQueues() woke us.
    p->exitFlag = false;
  }
  else{
    result = p->messages.front();
    p->messages.pop_front();
  }

  p->unlock();
  return result;
}

// If a piece of data is in internal storage with one of the list of passed
// tags and matches the requested dataID, stuff it into the first parameter.
// Else, time out (in msec; the default is no timeout, given by the value -1).
// Return the tag of the message actually retrieved (or -1 on
// timeout or other failure). If dataID (optional, default -1) is nonnegative,
// also check that the retrieved message has the proper ID.
int arStructuredDataParser::getNextTaggedMessage(arStructuredData*& message,
                                                 list<int> tags,
                                                 int dataID,
                                                 int timeout) {
  _globalLock.lock();

  // As in getNextInternal(), allow falling through this call,
  // to let arSZGClient be effectively STOPPED and RESTARTED
  // arbitrarily from that object's data thread!
  _activationLock.lock();
  if (!_activated) {
    _activationLock.unlock();
    _globalLock.unlock();
    return -1;
  }
  _activationLock.unlock();

  list<int>::const_iterator j;
  SZGtaggedMessageQueue::iterator i; // not const
  for (j = tags.begin(); j != tags.end(); ++j) {
    i = _taggedMessages.find(*j);
    if (i == _taggedMessages.end())
      continue;
    SZGdatalist& l = i->second;
    arStructuredData* potentialData = l.front();
    if (dataID >= 0 && potentialData->getID() != dataID) {
      ar_log_error() << "arStructuredDataParser expected id " << dataID <<
	", not " << potentialData->getID() <<".\n";
      _globalLock.unlock();
      return -1;
    }
    message = potentialData;
    l.pop_front();
    if (l.empty()) {
      // No more data, so ok to erase this entry.
      _taggedMessages.erase(i);
    }
    _globalLock.unlock();
    return *j;
  }

  // tags was empty: no data yet.
  // Associate a synchronizer with the list of tags,
  // the same synchronizer for all these tags.
  arStructuredDataSynchronizer* syn = NULL;
  if (_recycledSync.empty()) {
    syn = new arStructuredDataSynchronizer();
  }
  else{
    syn = _recycledSync.front();
    syn->reset();
    _recycledSync.pop_front();
  }

  // Associate it with the given tags.
  list<int> actualUsedTags;
  for (j = tags.begin(); j != tags.end(); j++) {
    // Ignore duplicate and illegel (value<0) tags.
    if (_messageSync.find(*j) == _messageSync.end() && *j >= 0) {
      // Add the synchronizer to the list.
      actualUsedTags.push_back(*j);
      syn->refCount++;
      _messageSync.insert(SZGtaggedMessageSync::value_type(*j, syn));
    }
  }
  _globalLock.unlock();

  // Phase 2: wait.
  // NOTE: there was originally a subtle bug here. Specifically, the
  // tag on the synchronizer may ALREADY be set. Consequently, a normal
  // exit might never get into the body of the loop. Thus the default for
  // normalExit must be TRUE and NOT false (as it was originally). If
  // we do, in fact, make it into the body of the loop, then the final value
  // of normalExit is determined by the wait.
  bool normalExit = true;
  syn->lock();

    // If syn->exitFlag, the tag will be -1, so exit normally from the wait.
    while (syn->tag < 0 && !syn->exitFlag && normalExit) {
      // Wait returns false on timeout, i.e. no data.
      normalExit = syn->wait(timeout);
    }
    // Get the tag associated with the signal,
    // i.e. the tag of the awaited message.
    const int tag = syn->tag;

  syn->unlock();

  // Synchronizers are no longer needed. Put them on the
  // recycling list. Use the ERROR-CHECKED list of tags,
  // which prevents putting tags on the list twice.
  _cleanupSynchronizers(actualUsedTags);

  if (!normalExit || tag == -1) {
    // Either timed-out (!normalExit), or
    // clearQueues() released the wait() above with -1.
    // No data to get, so fail.  Normal, so don't warn.
    return -1;
  }

  // Phase 3: got the message.
  arGuard dummy(_globalLock);
  i = _taggedMessages.find(tag);
  if ( i == _taggedMessages.end() ) {
    ar_log_error() << "getNextTaggedMessage got no message.\n";
  } else {
    SZGdatalist& l = i->second;
    arStructuredData* potentialData = l.front();
    if ( dataID >= 0 && potentialData->getID() != dataID ) {
      ar_log_error() << "getTaggedMessage expected id " << dataID << ", not " <<
	potentialData->getID() <<".\n";
      return -1;
    }
    // Either we don't care about the data type, or we've got the proper
    // data type. Return success.
    message = potentialData;
    l.pop_front();
    // only erase the entry if the list is empty
    if (l.empty()) {
      _taggedMessages.erase(i);
    }
  }
  return tag;
}

// Avoid memory leaks: reclaim data storage.
void arStructuredDataParser::recycle(arStructuredData* trash) {
  const int ID = trash->getID();
  arGuard dummy(_recycleLock);
  const SZGrecycler::const_iterator i = recycling.find(ID);
  if (i != recycling.end()) {
    // Found recycling for this ID.
    i->second->push_back(trash);
  }
  else{
    // Create recycling list.
    SZGdatalist* l = new SZGdatalist(1, trash);
    recycling.insert(SZGrecycler::value_type(ID, l));
  }
}

// "Release" every pending request for getNextTaggedMessage() or getNextInternal().
// Remove all already parsed messages from the internal queues for
// particular message IDs AND from all the tagged queues, to let e.g. an
// an arSZGClient disconnect from an szgserver, let current calls fall through,
// and later be able to reconnect to a different szgserver.

void arStructuredDataParser::clearQueues() {
  arGuard dummy(_globalLock);

  // Deactivate: getNextInternal() and getNextTagged() will fail
  // until ativateQueues() is called.
  _activationLock.lock();
    _activated = false;
  _activationLock.unlock();

  // Find any outstanding message sync requests (where
  // threads are waiting on messages). Send the release signal.
  // Since a synchronizer may appear in the list multiple times,
  // we may send the release multiple times as well.
  for (iSync i(_messageSync.begin()); i != _messageSync.end(); ++i) {
    arStructuredDataSynchronizer* p = i->second;
    p->lock();

      // No tag to pass.
      p->tag = -1;
      // Tell any woken getNextTagged() to fail.
      p->exitFlag = true;
      p->signal();

    p->unlock();
  }
  // DO NOT remove from the _messageQueue. That is the job of
  // _cleanupSynchronizers() below. It will be executed by the various
  // released getNextTaggedMessage() functions.

  // Recycle all received tagged messages, removing them from their respective queues.
  for (iTagQueue k(_taggedMessages.begin()); k != _taggedMessages.end(); ++k) {
    _recyclelist(k->second);
  }
  _taggedMessages.clear();

  // Release blocked getNextInternal() calls.
  // Incidentally, recycle all messages as waiting by ID.
  for (iQueue j(_messageQueue.begin()); j != _messageQueue.end(); ++j) {
    arMessageQueueByID* p = j->second;
    p->lock();

      // Recycle everything on the message list and then signal.
      // Set the variable indicating that data has been cleared.
      // Clear but don't delete the list.
      _recyclelist(p->messages);
      p->messages.clear();
      // Assume that at most *one* getNextInternal() is waiting for this ID.
      // Thus, set the exit flag, to be reset by the woken waiter.
      p->exitFlag = true;
      p->signal();

    p->unlock();
  }
}

// Resume data flowing from getNextInternal() and getNextTagged().

void arStructuredDataParser::activateQueues() {
  arGuard dummy(_activationLock);
  _activated = true;
  for (iQueue i(_messageQueue.begin()); i != _messageQueue.end(); ++i) {
    i->second->exitFlag = false;
  }
  // Tagged synchronizers don't need this since they start out already reset.
}

// Push the received message onto one of the queues.
// Signal a potentially blocked getNextInternal() that it can proceed.

void arStructuredDataParser::_pushOntoQueue(arStructuredData* theData) {
  // we assume the next 3 iterators will find something, because theData is valid
  iQueue i(_messageQueue.find(theData->getID()));
  arMessageQueueByID* p = i->second;
  p->lock();
    p->messages.push_back(theData);
    p->signal();
  p->unlock();
}

// Push the received message onto the tagged queue (a single
// queue for all message types, but indexed via ID). Signal a potentially
// blocked getNextTaggedMessage() that it can proceed.
// Because of "message continuations",
// multiple messages may be received for a particular tag.
// The only uniqueness is that at most one command at a time
// may wait on a particular tag.

void arStructuredDataParser::_pushOntoTaggedQueue(int tag, arStructuredData* theData) {
  arGuard dummy(_globalLock);
  SZGtaggedMessageQueue::iterator j = _taggedMessages.find(tag);
  if (j == _taggedMessages.end()) {
    // This tag has no data yet.
    _taggedMessages.insert(
      SZGtaggedMessageQueue::value_type(tag, SZGdatalist(1, theData)));
  }
  else{
    j->second.push_back(theData);
  }
  // if a condition var exists, signal
  iSync i = _messageSync.find(tag);
  if (i != _messageSync.end()) {
    // a getNextTaggedMessage is waiting on us
    // set the tag, this tells the other side that the
    // condition variable firing was not accidental (as can happen under
    // POSIX) AND it also tells the other side that THIS PARTICULAR TAG
    // is the one responsible for the firing!
    // NOTE: MULTIPLE tags can correspond to ONE synchronizer!
    arStructuredDataSynchronizer* p = i->second;
    p->lock();
      p->tag = tag;
      p->signal();
    p->unlock();
  }
}

// Clean up the synchronizer associated with a list of tags.
// All tags in the list associate with the same synchronizer.
void arStructuredDataParser::_cleanupSynchronizers(list<int> tags) {
  arGuard dummy(_globalLock);
  for (list<int>::const_iterator i(tags.begin()); i != tags.end(); ++i) {
    SZGtaggedMessageSync::iterator j = _messageSync.find(*i);
    if (j != _messageSync.end()) {
      if (--(j->second->refCount) == 0) {
        _recycledSync.push_front(j->second);
      }
      _messageSync.erase(j);
    }
  }
}
