//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arStructuredDataParser.h"
#include "arXMLUtilities.h"

arStructuredDataParser::arStructuredDataParser(arTemplateDictionary* dictionary) :
  _dictionary(dictionary){
  ar_mutex_init(&_globalLock);
  ar_mutex_init(&_recycleLock);
  ar_mutex_init(&_activationLock);
  _activated = true;
  ar_mutex_init(&_translationBufferListLock);
  // Each template in the dictionary has a queue lock and a condition variable.
  for (arTemplateType::iterator i = dictionary->begin();
       i != dictionary->end();
       ++i){
    arMessageQueueByID* theQueue = new arMessageQueueByID();
    ar_mutex_init(&theQueue->lock);
    theQueue->exitFlag = false;
    int templateID = i->second->getID();
    _messageQueue.insert(SZGmessageQueue::value_type(templateID, 
                                                     theQueue));
  }
}

arStructuredDataParser::~arStructuredDataParser(){
  for (SZGrecycler::iterator i(recycling.begin());
       i != recycling.end();
       ++i){
    // Delete the owned storage.
    if (i->second){
      for (list<arStructuredData*>::iterator j(i->second->begin());
	   j != i->second->end();
	   ++j){
        if (*j)
          delete *j;
      }
      delete i->second;
    }
  }
  // Delete messages received but not yet delivered, one queue at a time.
  // (these are the messages queued by ID, not the "tagged" messages.
  for (SZGmessageQueue::iterator k(_messageQueue.begin());
       k != _messageQueue.end();
       ++k){
    for (list<arStructuredData*>::iterator l = k->second->messages.begin();
	 l != k->second->messages.end();
	 l++){
      if (*l)
	delete *l;
    }
    // Delete the elements of the map as well.
    delete k->second;
  }
  // delete the tagged messages
  for (SZGtaggedMessageQueue::iterator ii = _taggedMessages.begin();
       ii != _taggedMessages.end();
       ii++){
    // we have a list of arStructuredData records that are unclaimed
    for (list<arStructuredData*>::iterator dataIter = ii->second.begin();
	 dataIter != ii->second.end(); ++dataIter){
      if (*dataIter)
        delete *dataIter;
    }
  }
  // delete the currently used synchronization primitives
  for (SZGtaggedMessageSync::iterator jj = _messageSync.begin();
       jj != _messageSync.end(); jj++){
    delete jj->second;
  }
  // delete the to-be-used synchronization primitives
  for (SZGunusedMessageSync::iterator kk = _recycledSync.begin();
       kk != _recycledSync.end(); kk++){
    delete *kk;
  }
  // delete the translation buffers.
  for (list<arBuffer<char>*>::iterator n = _translationBuffers.begin();
       n != _translationBuffers.end(); n++){
    delete *n;
  }
}

/// Try to liberate a data record from the internal recycling list,
/// creating a new one if none-such exists.
/// @param ID type of record we want
arStructuredData* arStructuredDataParser::getStorage(int ID){
  arStructuredData* result = NULL;
  ar_mutex_lock(&_recycleLock);
  // Is there a data record in the dictionary with this ID?
  arDataTemplate* theTemplate = _dictionary->find(ID);
  if (!theTemplate)
    goto LDone;

  {
    // Is there any recycling available for this ID?
    SZGrecycler::iterator i = recycling.find(ID);
    if (i != recycling.end()){
      list<arStructuredData*>* j = i->second;
      if (!j->empty()){
	// Yes!
	result = *(j->begin());
	j->pop_front();
      }
      else{
	// No.  Make some.
	result = new arStructuredData(theTemplate);
      }
    }
    else{
      // Recycling list.
      list<arStructuredData*>* tmp = new list<arStructuredData*>;
      recycling.insert(SZGrecycler::value_type(ID,tmp));
      // Storage.
      result = new arStructuredData(theTemplate);
    }
  }

LDone:
  ar_mutex_unlock(&_recycleLock);
  return result;
}

/// Get one of the translation buffers. If none yet exist, create one and
/// return that.
arBuffer<char>* arStructuredDataParser::getTranslationBuffer(){
  arBuffer<char>* result = NULL;
  ar_mutex_lock(&_translationBufferListLock);
  if (_translationBuffers.empty()){
    result = new arBuffer<char>(1024);
  }
  else{
    result = _translationBuffers.front();
    _translationBuffers.pop_front();
  }
  ar_mutex_unlock(&_translationBufferListLock);
  return result;
}

/// Return one of the translation buffers. Note that the returned buffer
/// goes to the front of the list. This is to insure that buffers are
/// reused as much as possible.
void arStructuredDataParser::recycleTranslationBuffer(arBuffer<char>* buffer){
  ar_mutex_lock(&_translationBufferListLock);
  _translationBuffers.push_front(buffer);
  ar_mutex_unlock(&_translationBufferListLock);
}

/// Parse the a binary stream into an arStructuredData record
/// (using internal recycling if possible).
arStructuredData* arStructuredDataParser::parse(ARchar* buffer, int& end){
  const int ID = ar_rawDataGetID(buffer);
  end = ar_rawDataGetSize(buffer);
  arStructuredData* result = getStorage(ID);
  if (!result){
    cerr << "arStructuredDataParser error: failed to parse binary data.\n";
    return NULL;
  }
  result->unpack(buffer);
  return result;
}

/// NOTE: historically, the arStructuredDataParser has been used *after*
/// the arDataClient (or whatever else) has grabbed a buffer of data (either
/// a single record or a queue of records) and translated them from remote
/// into local format. SO... there was no need to include a "translation"
/// feature. However, it seems that a translation feature is, indeed,
/// important. Eventually, we'll fold the arStructuredDataParser into
/// the lower-level infrastructure. (i.e. it'll no longer be possible to
/// use the arDataClient to get a raw buffer of data). At that time, this'll
/// be useful.
/// ALSO, JUST AS A THOUGHT, it might be better to fold the translation
/// capabilities into the arStructuredData object... and not need to
/// double the storage for buffers in the case of translation. Something
/// for the future.
arStructuredData* arStructuredDataParser::parse
                                    (ARchar* buffer, int& end,
				     const arStreamConfig& remoteStreamConfig){
  if (remoteStreamConfig.endian == AR_ENDIAN_MODE){
    return parse(buffer, end);
  }
  else{
    // we actually need to do translation
    arBuffer<char>* transBuffer = getTranslationBuffer();
    // buffer gets bigger if necessary. WE ARE ASSUMING HERE THAT
    // TRANSLATED DATA DOES NOT GET BIGGER!!!!
    int size = ar_translateInt(buffer, remoteStreamConfig); 
    // important to pass this back
    end = size;
    transBuffer->grow(size);
    int recordID = ar_translateInt(buffer+AR_INT_SIZE, remoteStreamConfig);
    arDataTemplate* theTemplate = _dictionary->find(recordID);
    if (!theTemplate ||
        theTemplate->translate(transBuffer->data, 
                               buffer, 
                               remoteStreamConfig) < 0) {
      cerr << "arStructuredDataParser error: failed to translate data "
	   << "record.\n";
      recycleTranslationBuffer(transBuffer);
      return NULL;
    }
    // we use "temp" because we don't care about the size of the data,
    // that was already taken care of above via "end=size"
    int temp = -1;
    arStructuredData* result = parse(transBuffer->data, temp);
    recycleTranslationBuffer(transBuffer);
    return result;
  }
}

/// Parse an XML text stream (using internal recycling if possible).
arStructuredData* arStructuredDataParser::parse(arTextStream* textStream){
  string recordBegin = ar_getTagText(textStream);
  if (recordBegin == "NULL" || ar_isEndTag(recordBegin)){
    return NULL;
  }
  return parse(textStream, recordBegin);
}

/// A typical stream of XML data will contain some "extra" tags which act
/// as flow control. For instance, consider the below:
///
/// <szg_vtk_world>
///   <world_record>
///    ...
///   </world_record>
///
///   <world_record>
///    ...
///   </world_record>
/// </szg_vtk_world>
///
/// Note that the application reading this stream must first read a tag.
/// If the tag turns out to be control-related (such as <szg_vtk_world>),
/// then the application makes a behavior change of some sort. Otherwise,
/// we want to be able to read in the rest of the record, given the
/// tag name. 
arStructuredData* arStructuredDataParser::parse(arTextStream* textStream,
                                                const string& tagText){
  // Get a resizable buffer to use in reading-in the various text fields.
  arBuffer<char>* workingBuffer = getTranslationBuffer();
  arDataTemplate* theTemplate = _dictionary->find(tagText);
  if (!theTemplate){
    cerr << "arStructuredDataParser error: the dictionary contains no "
	 << "type with name " << tagText << ".\n";
    return NULL;
  }
  // get some storage
  int ID = theTemplate->getID();
  arStructuredData* theData = getStorage(ID);
  
  int whichAttribute = 0;
  int totalAttributes = theTemplate->getNumberAttributes();

  // NOTE: we want to be able to deal with the case where the record is
  // INCOMPLETE (i.e. some fields are missing). This is, after all, one
  // of the big advantages of XML. At one point, this was an important
  // backwards compatibility thing for szg.conf (netmask was added and
  // we wanted to be able to continue to use the old config files).

  // Zero out the data dimensions of all fields. This way, if a field doesn't
  // get set, it will have no data!
  for (int i = 0; i < totalAttributes; i++){
    theData->setDataDimension(i, 0);
  }

  string fieldBegin;
  bool foundEndTag = false;
  while (whichAttribute < totalAttributes){
    fieldBegin = ar_getTagText(textStream, workingBuffer);
    // Check to see if this is a premature ending tag (i.e. some of the
    // fields are missing. As was said before, fields are allowed to be
    // missing.
    if (ar_isEndTag(fieldBegin)){
      // This is potentially OK. 
      foundEndTag = true;
      // Exit the loop, regardless of whether or not we've found all fields.
      break;
    }
    if (fieldBegin == "NULL"){
      cerr << "arStructuredDataParser error: did not find a valid start tag "
	   << "at the beginning of the field name.\n";
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
    arDataType fieldType = theTemplate->getAttributeType(fieldBegin);
    if (fieldType == AR_GARBAGE){
      cerr << "arStructuredDataParser error: the type " << tagText
	   << " contains no field with the name " << fieldBegin << ".\n";
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
    int size = -1;
    ARchar* charData = NULL;
    ARint* intData = NULL;
    ARfloat* floatData = NULL;
    ARlong* longData = NULL;
    ARint64* int64Data = NULL;
    ARdouble* doubleData = NULL;
    // Get the data from the record. 
    if (!ar_getTextBeforeTag(textStream, workingBuffer)){
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
    switch (fieldType) {
    case AR_CHAR:
      // THIS NOT MUST SKIP WHITESPACE! ALL OF THE OTHERS SHOULD DO SO!
      size = ar_parseXMLData<ARchar>(workingBuffer, charData, false);
      if (!charData){
        cerr << "arStructuredDataParser error: XML char data expected but "
	     << "not received.\n";
	delete theData;
        recycleTranslationBuffer(workingBuffer);
	return NULL;
      }
      (void)theData->dataIn(fieldBegin, charData, AR_CHAR, size);
      delete [] charData;
      break;
    case AR_INT:
      size = ar_parseXMLData<ARint>(workingBuffer, intData);
      if (!intData){
        cerr << "arStructuredDataParser error: XML int data expected but "
	     << "not received.\n";
	delete theData;
        recycleTranslationBuffer(workingBuffer);
	return NULL;
      }
      (void)theData->dataIn(fieldBegin, intData, AR_INT, size);
      delete [] intData;
      break;
    case AR_FLOAT:
      size = ar_parseXMLData<ARfloat>(workingBuffer, floatData);
      if (!floatData){
        cerr << "arStructuredDataParser error: XML float data expected but "
	     << "not received.\n";
	delete theData;
        recycleTranslationBuffer(workingBuffer);
	return NULL;
      }
      (void)theData->dataIn(fieldBegin, floatData, AR_FLOAT, size);
      delete [] floatData;
      break;
    case AR_LONG:
      size = ar_parseXMLData<ARlong>(workingBuffer, longData);
      if (!longData){
        cerr << "arStructuredDataParser error: XML long data expected but "
	     << "not received.\n";
	delete theData;
        recycleTranslationBuffer(workingBuffer);
	return NULL;
      }
      (void)theData->dataIn(fieldBegin, longData, AR_LONG, size);
      delete [] longData;
      break;
    case AR_INT64:
      size = ar_parseXMLData<ARint64>(workingBuffer, int64Data);
      if (!int64Data){
        cerr << "arStructuredDataParser error: XML 64-bit int data expected but "
	     << "not received.\n";
	delete theData;
        recycleTranslationBuffer(workingBuffer);
	return NULL;
      }
      (void)theData->dataIn(fieldBegin, int64Data, AR_INT64, size);
      delete [] int64Data;
      break;
    case AR_DOUBLE:
      size = ar_parseXMLData<ARdouble>(workingBuffer, doubleData);
      if (!doubleData){
        cerr << "arStructuredDataParser error: XML double data expected but "
	     << "not received.\n";
	delete theData;
        recycleTranslationBuffer(workingBuffer);
	return NULL;
      }
      (void)theData->dataIn(fieldBegin, doubleData, AR_DOUBLE, size);
      delete [] doubleData;
      break;
    case AR_GARBAGE:
      cerr << "arStructuredDataParser error: field incorrectly designated as "
	   << "AR_GARBAGE.\n";
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
      break;
    }
    string fieldEnd = ar_getTagText(textStream, workingBuffer);
    if (fieldEnd == "NULL" ||
        !ar_isEndTag(fieldEnd) || 
        fieldBegin != ar_getTagType(fieldEnd)){
      cerr << "arStructuredDataParser error: data field not terminated with "
	   << "the correct closing tag. Field began with " << fieldBegin
	   << " and closed with " << ar_getTagType(fieldEnd) << ".\n";
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
    whichAttribute++;
  }
  if (foundEndTag){
    // Found an end tag before reading all the fields.
    if (tagText != ar_getTagType(fieldBegin)){
      // The end tag DOES NOT match. This is an error.
      cout << "arStructuredDataParser error: record terminated with "
	   << "incorrect closing tag = " << ar_getTagType(fieldBegin) 
	   << " when " << tagText << " was expected.\n";
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
  }
  else{
    string recordEnd = ar_getTagText(textStream, workingBuffer);
    if (recordEnd == "NULL" ||
        !ar_isEndTag(recordEnd) ||
        tagText != ar_getTagType(recordEnd)){
      cerr << "arStructuredDataParser error: record not terminated with the "
	   << "correct closing tag. Record began with " << tagText
	   << " and closed with " << ar_getTagType(recordEnd) << ".\n";
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
  }
  // Success.
  recycleTranslationBuffer(workingBuffer);
  return theData;
}

arStructuredData* arStructuredDataParser::parseBinary(FILE* inputFile){
  arBuffer<char>* buffer = getTranslationBuffer();
  ARint recordSize = -1;
  if (fread(buffer->data, AR_INT_SIZE, 1, inputFile) == 0){
    // Failure has occured in reading the file.
    return NULL;
  }
  // All buffers in the store have size at least 1024, so they can handle
  // one int.
  ar_unpackData(buffer->data, &recordSize, AR_INT, 1);
  if (recordSize>buffer->size()){
    // resize buffer
    if (recordSize > 2*buffer->size()){
      buffer->resize(recordSize);
    }
    else{
      buffer->resize(2*buffer->size());
    }
  }
  const int result = fread(buffer->data+AR_INT_SIZE,1,
                           recordSize-AR_INT_SIZE, inputFile);
  if (result < recordSize - AR_INT_SIZE){
    cerr << "arStructuredDataParser error: failed reading record: got only " 
         << result+AR_INT_SIZE
	 << " of " << recordSize << " expected bytes.\n";
    recycleTranslationBuffer(buffer);
    return NULL;
  }
  int ID = ar_rawDataGetID(buffer->data);
  arStructuredData* data = getStorage(ID);
  if (!data){
    recycleTranslationBuffer(buffer);
    return NULL;
  }
  data->unpack(buffer->data);
  recycleTranslationBuffer(buffer);
  return data;
}

/// Sometimes it is advantageous to read-in incoming data in a seperate thread 
/// than in which it is actually used. In addition to holding raw "recycled" 
/// storage, the arStructuredDataParser can hold received message lists. There 
/// is one received message list per record type in the language. New 
/// messages, as parsed by "parseIntoInternal" are placed at the end of the 
/// list. This allows demultiplexing via message type, which is important for 
/// the proper operation of arSZGClient objects, among others. This function 
/// parses data from a char buffer and stores it in one of the received 
/// message list. If a getNextInternal(...) method is waiting on such data, it 
/// is woken up.
bool arStructuredDataParser::parseIntoInternal(ARchar* buffer, int& end){
  arStructuredData* theData = parse(buffer,end);
  if (!theData){
    // the parse failed for some reason
    return false;
  }
  // we have some valid data
  _pushOntoQueue(theData);
  return true;
}

/// Same as parseIntoInternal(ARchar*, int) except that here the source is a 
/// text file, with XML structure
bool arStructuredDataParser::parseIntoInternal(arTextStream* textStream){
  arStructuredData* theData = parse(textStream);
  if (!theData){
    // the parse failed for some reason
    return false;
  }
  _pushOntoQueue(theData);
  return true;
}

/// The arStructuredDataParser can hold received messages in two ways.
/// The first, as typified by parseIntoInternal(...) is useful for storing
/// and later retrieving message based on message type. This method, in
/// contrast, stores the message internally based on a user-defined tag.
/// Records are retrieved (possibly in a different thread) via a blocking
/// getNextTaggedMessage(...), which uses the tag as a retrieval key. The
/// idea is that this can be used to implement async rpc.
/// NOTE: multiple records can be stored with a particular tag. This is
/// useful when, for instance, a syzygy message can get several responses.
bool arStructuredDataParser::pushIntoInternalTagged(arStructuredData* data,
                                                    int tag){
  if (!data){
    // we were passed bogus data
    return false;
  }
  // we have valid data
  _pushOntoTaggedQueue(tag, data);
  return true;
}

/// Either returns the top piece of arStructuredData on the appropriate queue 
/// or blocks until such is available.
/// NOTE: This can be interrupted, both by the time-out OR by the clearQueues()
/// method, which flushes all queues, releases any outstanding waits inside
/// getNextInternal or getNextTaggedMessage, etc.
arStructuredData* arStructuredDataParser::getNextInternal(int ID){
  SZGmessageQueue::iterator i(_messageQueue.find(ID));
  if (i == _messageQueue.end()){
    cerr << "arStructuredDataParser error: getNextInternal passed invalid "
	 << "ID.\n";
    return NULL;
  }
  ar_mutex_lock(&i->second->lock);
  // NOTE: after calling clearQueues(), we should ALWAYS fall through here
  // with an error UNTIL activateQueues() has been called.
  // (NO MATTER HOW calls to getNextInternal(...) were "IN FLIGHT" when the
  // original clearQueues() was issued. This allows classes that depend
  // on this to be GLOBALLY turned on/off, no matter where they live in the
  // threading universe.
  ar_mutex_lock(&_activationLock);
  if (!_activated){
    ar_mutex_unlock(&_activationLock);
    ar_mutex_unlock(&i->second->lock);
    return NULL;
  }
  ar_mutex_unlock(&_activationLock);
  while (i->second->messages.empty() && !i->second->exitFlag){
    i->second->var.wait(&i->second->lock);
  }
  arStructuredData* result = NULL;
  if (!i->second->exitFlag){
    result = i->second->messages.front();
    i->second->messages.pop_front();
  }
  else{
    // The exit flag tells us that we were woken by clearQueues().
    // Want to return this to the false state.
    i->second->exitFlag = false;
  }
  ar_mutex_unlock(&i->second->lock);
  return result;
}

/// If a piece of data is in internal storage with one of the list of passed
/// tags and matches the requested dataID, fill-in the first parameter with 
/// it. If not, wait until the specified time-out period (the default is to
/// have no time-out, as given by a default parameter for timeout of -1).
/// The function returns the tag of the message actually retrieved (or -1 on
/// timeout or other failure). The dataID is also an optional parameter,
/// given a value of -1 by default. If it is positive, we check that the
/// retrieved message has the proper ID also.
int arStructuredDataParser::getNextTaggedMessage(arStructuredData*& message,
                                                 list<int> tags,
                                                 int dataID,
                                                 int timeout){
  ar_mutex_lock(&_globalLock);
  // As in getNextInternal(...), we want to be able to fall through this
  // call. This allows arSZGClient to be effectively STOPPED and RESTARTED
  // arbitrarily from that object's data thread!
  ar_mutex_lock(&_activationLock);
  if (!_activated){
    ar_mutex_unlock(&_activationLock);
    ar_mutex_unlock(&_globalLock);
    return -1;
  }
  ar_mutex_unlock(&_activationLock);
  list<int>::iterator j;
  SZGtaggedMessageQueue::iterator i;
  arStructuredData* potentialData = NULL;
  for (j = tags.begin(); j != tags.end(); j++){
    i = _taggedMessages.find(*j);
    if (i != _taggedMessages.end()){ 
      potentialData = i->second.front();
      if (dataID < 0 || potentialData->getID() == dataID){
        message = potentialData;
        i->second.pop_front();
	// only erase the entry if there is no more data
        if (i->second.empty()){
          _taggedMessages.erase(i);
	}
        ar_mutex_unlock(&_globalLock);
        return *j;
      }
      else{
	cout << "arStructuredDataParser error: "
	     << "getTaggedMessage found a message with the wrong type.\n";
        ar_mutex_unlock(&_globalLock);
        return -1;
      }
    }
  }
  // The data does not yet exist. We must get a synchronizer and associate
  // it with the list of tags. NOTE: it is important that it be the SAME
  // synchronizer for ALL of these tags!
  arStructuredDataSynchronizer* syn = NULL;
  if (_recycledSync.empty()){
    // must create 
    syn = new arStructuredDataSynchronizer();
    syn->tag = -1;
    // We could be using this to signal "fall through" the getNextTagged(...)
    syn->exitFlag = false;
    syn->refCount = 0;
    ar_mutex_init(&syn->lock);
  }
  else{
    // simply pop the front, making sure that tag is set to -1.
    syn = _recycledSync.front();
    syn->tag = -1;
    // We could be using this to signal "fall through" the getNextTagged(...)
    syn->exitFlag = false;
    syn->refCount = 0;
    _recycledSync.pop_front();
  }
  // Now, associate it with the given tags. NOTE: It is critical that we
  // not allow DUPLICATE tags... nor should we allow tags clearly ILLEGAL
  // values (i.e. < 0)
  list<int> actualUsedTags;
  for (j = tags.begin(); j != tags.end(); j++){
    if (_messageSync.find(*j) == _messageSync.end() && *j >= 0){
      // Push the synchronizer on the list and increment its ref count by
      // one.
      actualUsedTags.push_back(*j);
      syn->refCount++;
      _messageSync.insert(SZGtaggedMessageSync::value_type(*j, syn));
    }
  }
  ar_mutex_unlock(&_globalLock);

  // phase 2 begins... we wait.
  // NOTE: there was originally a subtle bug here. Specifically, the 
  // tag on the synchronizer may ALREADY be set. Consequently, a normal
  // exit might never get into the body of the loop. Thus the default for
  // normalExit must be TRUE and NOT false (as it was originally). If
  // we do, in fact, make it into the body of the loop, then the final value
  // of normalExit is determined by the wait.
  bool normalExit = true;
  ar_mutex_lock(&syn->lock);
  // NOTE: it is important to check if the exitFlag has been set (because
  // in this case the tag will be -1 and we will exit normally from the
  // wait).
  while (syn->tag < 0 && !syn->exitFlag){
    // The wait call returns false exactly when we've got a time-out.
    // It is important to distinguish this case, since we won't actually
    // have any data.
    normalExit = syn->var.wait(&syn->lock, timeout);
    if (!normalExit){
      break;
    }
  }
  // Get the tag associated with the signal. This is the tag of the
  // message we've been awaiting.
  int tag = syn->tag;
  ar_mutex_unlock(&syn->lock);

  // We don't need the synchronizers anymore. Go ahead and put them on the
  // recycling list. NOTE: here use the ERROR-CHECKED list of tags 
  // (which prevents us from putting tags on the list twice).
  _cleanupSynchronizers(actualUsedTags);

  // Suppose we have either (a) timed-out (normalExit is false) or
  // clearQueues() has been issued (which can release the wait(...) above
  // with a value of -1). In this case, there is no data to get and we
  // should return a failure response. This is a normal occurence (really)
  // so do not go ahead and print anything.
  if (!normalExit || tag == -1){
    return -1;
  }

  // the message is now present... phase 3
  ar_mutex_lock(&_globalLock);
  i = _taggedMessages.find(tag);
  if ( i == _taggedMessages.end() ){
    cout << "arStructuredDataParser error: "
	 << "getNextTaggedMessage found no message.\n";
  }
  else{
    potentialData = i->second.front();
    if ( dataID < 0 || potentialData->getID() == dataID ){
      // Either we don't care about the data type OR we've got the proper
      // data type. Go ahead and return successfully.
      message = potentialData;
      i->second.pop_front();
      // only erase the entry if the list is empty
      if (i->second.empty()){
        _taggedMessages.erase(i);
      }
    }
    else{
      cout << "arStructuredDataParser error: "
	   << "getTaggedMessage found a message with wrong type.\n";
      // tag is the return value. Must alter it to indicate failure.
      tag = -1;
    }
  }
  ar_mutex_unlock(&_globalLock);
  return tag;
}

/// Avoid memory leaks by providing a way to return data storage into the
/// internal store
void arStructuredDataParser::recycle(arStructuredData* trash){
  ar_mutex_lock(&_recycleLock);
  const int ID = trash->getID();
  // Is there any recycling for this ID?
  SZGrecycler::iterator i = recycling.find(ID);
  if (i != recycling.end()) {
    i->second->push_back(trash);
  }
  else{
    // Create recycling list.
    list<arStructuredData*>* tmp = new list<arStructuredData*>;
    tmp->push_back(trash);
    recycling.insert(SZGrecycler::value_type(ID,tmp));
  }
  ar_mutex_unlock(&_recycleLock);
}

/// This function is intended to "release" every pending request for
/// getNextTaggedMessage(...) or getNextInternal(...). It, furthermore,
/// removes all already parsed messages from the internal queues for 
/// particular message IDs AND from all the tagged queues. Why? Well,
/// this is useful for allowing us to DISCONNECT a (for instance) arSZGClient
/// from an szgserver, have all the current calls fall through, and then
/// be able to reconnect to a DIFFERENT szgserver later, no matter what calls
/// we are blocking on where!
void arStructuredDataParser::clearQueues(){
  ar_mutex_lock(&_globalLock);
  // We should be "deactivated". This means that getNextInternal(...)
  // and getNextTagged(...) will ALWAYS return errors until ativateQueues(...)
  // is called.
  ar_mutex_lock(&_activationLock);
  _activated = false;
  ar_mutex_unlock(&_activationLock);
  // First, find any outstanding message sync requests (i.e. places where
  // people are waiting on messages). Go ahead and send the release signal.
  // NOTE: since a sunchronizer may appear in the list multiple times, we
  // may be sending the release multiple times as well. That is OK. 
  for (SZGtaggedMessageSync::iterator i = _messageSync.begin();
       i != _messageSync.end(); i++){
    ar_mutex_lock(&i->second->lock);
    // There is no tag to pass.
    i->second->tag = -1;
    // We set the flag indicating that any woken getNextTagged(...) should
    // return an error.
    i->second->exitFlag = true;
    i->second->var.signal();
    ar_mutex_unlock(&i->second->lock);
  }
  // DO NOT remove from the _messageQueue. That is the job of
  // _cleanupSynchronizers(...) below. It will be executed by the various
  // released getNextTaggedMessage(...) functions.

  // We must recycle all tagged messages, removing them from their
  // respective queues.
  for (SZGtaggedMessageQueue::iterator k = _taggedMessages.begin(); 
       k != _taggedMessages.end(); k++){
    for (list<arStructuredData*>::iterator kk = k->second.begin();
         kk != k->second.end(); kk++){
      recycle(*kk);
    }
  }
  // The map of lists of received tagged messages has been cleared. Go ahead
  // and get rid of it.
  _taggedMessages.clear();

  // Now, we want to release all of the getNextInternal(...) calls that may
  // be blocking. (and, incidentally, recycle all messages as waiting by ID)
  for (SZGmessageQueue::iterator j = _messageQueue.begin();
       j != _messageQueue.end(); j++){
    ar_mutex_lock(&j->second->lock);
    // recycle everything on the message list and then signal. NOTE:
    // we must also set the variable indicating that data has been cleared.
    for (list<arStructuredData*>::iterator jj = j->second->messages.begin();
         jj != j->second->messages.end(); jj++){
      recycle(*jj);
    }
    // Don't forget to CLEAR the list. HOWEVER, DO NOT delete the list!
    j->second->messages.clear();
    // WE ARE ASSUMING THAT THERE IS AT MOST *ONE* getNextInternal CALL
    // WAITING! (for this particular ID). Thus, we can set the exit flag
    // here... and the waiter, when woken, can UNSET it.
    j->second->exitFlag = true;
    j->second->var.signal();
    ar_mutex_unlock(&j->second->lock);
  }
  ar_mutex_unlock(&_globalLock);
}

/// Lets the flow of data from getNextInternal(...) and getNextTagged(...)
/// start again.
void arStructuredDataParser::activateQueues(){
  ar_mutex_lock(&_activationLock);
  _activated = true;
  // step through the message-queues-by-ID and go ahead and reset the exit 
  // flags to false.
  for (SZGmessageQueue::iterator i = _messageQueue.begin();
       i != _messageQueue.end(); i++){
    i->second->exitFlag = false;
  }
  // NOTE: this DOES NOT have to be done with the tagged synchronizers since
  // they always start out as reset.
  ar_mutex_unlock(&_activationLock);
}

/// here, we go ahead and push the received message onto one of the queues and
/// signal a potentially blocked getNextInternal(...) that it can proceed
void arStructuredDataParser::_pushOntoQueue(arStructuredData* theData){
  // we assume the next 3 iterators will find something, because we have a
  // valid piece of arStructuredData passed-in
  SZGmessageQueue::iterator i(_messageQueue.find(theData->getID()));
  ar_mutex_lock(&i->second->lock);
  i->second->messages.push_back(theData);
  i->second->var.signal();
  ar_mutex_unlock(&i->second->lock);
}

/// go ahead and pushed the received message onto the tagged queue (a single
/// queue for all message types, but indexed via ID). Signal a potentially
/// blocked getNextTaggedMessage(...) that it can proceed.
/// NOTE: because of "message continuations" it is INDEED possible that
/// multiple messages can be received corresponding to a particular tag.
/// The only uniqueness here is that we DO NOT allow waiting on a particular
/// tag by more than one command at a time.
void arStructuredDataParser::_pushOntoTaggedQueue(int tag,
                                                  arStructuredData* theData){
  ar_mutex_lock(&_globalLock);
  SZGtaggedMessageQueue::iterator j = _taggedMessages.find(tag);
  if (j == _taggedMessages.end()){
    // no data for this tag yet exists.
    list<arStructuredData*> newList;
    newList.push_back(theData);
    _taggedMessages.insert(SZGtaggedMessageQueue::value_type
			   (tag, newList));
  }
  else{
    j->second.push_back(theData);
  }
  // see if a condition var exists... if so signal
  SZGtaggedMessageSync::iterator i = _messageSync.find(tag);
  if ( i != _messageSync.end() ){
    // must be the case that a getNextTaggedMessage is waiting on us
    ar_mutex_lock(&i->second->lock);
    // must set the tag, this tells the other side that the
    // condition variable firing was not accidental (as can happen under
    // POSIX)... AND it also tells the other side that THIS PARTICULAR TAG
    // is the one responsible for the firing! NOTE: it is definitely 
    // possible for MULTIPLE tags to correspond to ONE synchronizer!
    i->second->tag = tag;
    i->second->var.signal();
    ar_mutex_unlock(&i->second->lock);
  }
  ar_mutex_unlock(&_globalLock);
}

/// This function is called to clean-up the synchronizer associated with a
/// list of tags. All of the tags in the list are associated with the
/// SAME synchronizer!
void arStructuredDataParser::_cleanupSynchronizers(list<int> tags){
  list<int>::iterator i;
  SZGtaggedMessageSync::iterator j;
  ar_mutex_lock(&_globalLock);
  for (i = tags.begin(); i != tags.end(); i++){
    j = _messageSync.find(*i);
    if (j != _messageSync.end()){
      j->second->refCount--;
      if (j->second->refCount == 0){
        _recycledSync.push_front(j->second);
      }
      _messageSync.erase(j);
    }
  }
  ar_mutex_unlock(&_globalLock);
}
