//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arStructuredDataParser.h"
#include "arXMLUtilities.h"

arStructuredDataParser::arStructuredDataParser(arTemplateDictionary* dictionary) :
  _dictionary(dictionary){
  ar_mutex_init(&_globalLock);
  ar_mutex_init(&_translationBufferListLock);
  // Each template in the dictionary has a queue lock and a condition variable.
  for (arTemplateType::iterator i = dictionary->begin();
       i != dictionary->end();
       ++i){
    arMutex* theMutex = new arMutex;
    ar_mutex_init(theMutex);
    int templateID = i->second->getID();
    _messageQueueLock.insert(SZGmessageQueueLock::value_type(templateID, 
                                                             theMutex));
    arConditionVar* theConditionVar = new arConditionVar;
    _messageQueueVar.insert(SZGmessageQueueVar::value_type(templateID, 
                                                           theConditionVar));
    list<arStructuredData*> theList;
    _messageQueue.insert(SZGmessageQueue::value_type(templateID, theList));
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
  for (SZGmessageQueue::iterator k(_messageQueue.begin());
       k != _messageQueue.end();
       ++k){
    for (list<arStructuredData*>::iterator l(k->second.begin());
	 l != k->second.end();
	 l++){
      if (*l)
	delete *l;
    }
  }
  // Delete the mutexes.
  for (SZGmessageQueueLock::iterator m(_messageQueueLock.begin());
       m != _messageQueueLock.end();
       ++m){
    if (m->second)
      delete m->second;
  }
  // Delete the condition vars.
  for (SZGmessageQueueVar::iterator n(_messageQueueVar.begin());
       n != _messageQueueVar.end();
       ++n){
    if (n->second)
      delete n->second;
  }
  // delete the tagged messages
  for (SZGtaggedMessageQueue::iterator ii = _taggedMessages.begin();
       ii != _taggedMessages.end();
       ii++){
    // we have a list of arStructuredData records that are unclaimed
    list<arStructuredData*>::iterator dataIter;
    for (dataIter = ii->second.begin();
	 dataIter != ii->second.end(); dataIter++){
      if (*dataIter){
        delete *dataIter;
      }
    }
  }
  // delete the currently used synchronization primitives
  for (SZGtaggedMessageSync::iterator jj = _messageSync.begin();
       jj != _messageSync.end(); jj++){
    delete jj->second.lock;
    delete jj->second.var;
    delete jj->second.tag;
  }
  // delete the to-be-used synchronization primitives
  for (SZGunusedMessageSync::iterator kk = _recycledSync.begin();
       kk != _recycledSync.end(); kk++){
    delete (*kk).lock;
    delete (*kk).var;
    delete (*kk).tag;
  }
}

/// Try to liberate a data record from the internal recycling list,
/// creating a new one if none-such exists.
/// @param ID type of record we want
arStructuredData* arStructuredDataParser::getStorage(int ID){
  arStructuredData* result = NULL;
  ar_mutex_lock(&_globalLock);
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
  ar_mutex_unlock(&_globalLock);
  return result;
}

/// Get one of the translation buffers. If none yet exist, create one and
/// return that.
arBuffer<char>* arStructuredDataParser::getTranslationBuffer(){
  arBuffer<char>* result;
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
    int temp;
    // we use "temp" because we don't care about the size of the data,
    // that was already taken care of above via "end=size"
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
  while (whichAttribute < totalAttributes){
    string fieldBegin = ar_getTagText(textStream, workingBuffer);
    if (fieldBegin == "NULL" || ar_isEndTag(fieldBegin)){
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
    int size;
    ARchar* charData = NULL;
    ARint* intData = NULL;
    ARfloat* floatData = NULL;
    ARlong* longData = NULL;
    ARdouble* doubleData = NULL;
    // Get the data from the record. 
    if (!ar_getTextBeforeTag(textStream, workingBuffer)){
      delete theData;
      recycleTranslationBuffer(workingBuffer);
      return NULL;
    }
    switch (fieldType) {
    case AR_CHAR:
      // THIS MUST SKIP WHITESPACE! NONE OF THE OTHERS SHOULD DO SO!
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
  recycleTranslationBuffer(workingBuffer);
  return theData;
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
  //ar_mutex_lock(&_globalLock);
  //if (_taggedMessages.find(tag) != _taggedMessages.end()){
  //  cout << "arStructuredDataParser error: attempted to use previously "
  //	 << "used tag.\n";
  //  ar_mutex_unlock(&_globalLock);
  //  return false;
  //}
  //ar_mutex_unlock(&_globalLock);
  if (!data){
    // we were passed bogus data
    return false;
  }
  // we have valid data
  _pushOntoTaggedQueue(tag, data);
  return true;
}

/// Either returns the top piece of arStructuredData on the appropriate queue 
/// or blocks until such is available
arStructuredData* arStructuredDataParser::getNextInternal(int ID){
  SZGmessageQueue::iterator i(_messageQueue.find(ID));
  if (i == _messageQueue.end()){
    cerr << "arStructuredDataParser error: getNextInternal passed invalid "
	 << "ID.\n";
    return NULL;
  }
  // these next 2 finds are guaranteed to succeed given that the above 
  // one succeeded
  SZGmessageQueueLock::iterator j(_messageQueueLock.find(ID));
  SZGmessageQueueVar::iterator k(_messageQueueVar.find(ID));
  ar_mutex_lock(j->second);
  while (i->second.empty()){
    k->second->wait(j->second);
  }
  arStructuredData* result = i->second.front();
  i->second.pop_front();
  ar_mutex_unlock(j->second);
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
  list<int>::iterator j;
  SZGtaggedMessageQueue::iterator i;
  arStructuredData* potentialData;
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
  // the data does not yet exist. we must get a list of synchronizers.
  arStructuredDataSynchronizer syn;
  for (j = tags.begin(); j != tags.end(); j++){
    if (_recycledSync.empty()){
      // must create 
      syn.lock = new arMutex();
      syn.var = new arConditionVar;
      syn.tag = new int;
      // These need to be pointers so copies will point to the same value.
      *(syn.tag) = -1;
      ar_mutex_init(syn.lock);
    }
    else{
      // simply pop the front, making sure that tag is set to -1.
      syn = _recycledSync.front();
      *(syn.tag) = -1;
      _recycledSync.pop_front();
    }
    // in either case, push the synchronizer on the list of active ones
    _messageSync.insert(SZGtaggedMessageSync::value_type(*j, syn));
  }
  ar_mutex_unlock(&_globalLock);

  // phase 2 begins... we wait.
  bool normalExit = false;
  ar_mutex_lock(syn.lock);
  while (*(syn.tag) < 0){
    // The wait call returns false exactly when we've got a time-out.
    // It is important to distinguish this case, since we won't actually
    // have any data.
    normalExit = syn.var->wait(syn.lock, timeout);
    if (!normalExit){
      break;
    }
  }
  // Get the tag associated with the signal. This is the tag of the
  // message we've been awaiting.
  int tag = *(syn.tag);
  ar_mutex_unlock(syn.lock);

  // We don't need the synchronizers anymore. Go ahead and put them on the
  // recycling list
  _cleanupSynchronizers(tags);

  if (!normalExit){
    // we must have gotten a time-out instead of a signal indicating
    // that data has arrived.
    // DO NOT PRINT ANYTHING. THIS IS A NORAML OCCURENCE, UNLIKE
    // GETTING A TAGGED ITEM WITH THE WRONG DATATYPE.
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
  ar_mutex_lock(&_globalLock);
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
  ar_mutex_unlock(&_globalLock);
}

/// here, we go ahead and push the received message onto one of the queues and
/// signal a potentially blocked getNextInternal(...) that it can proceed
void arStructuredDataParser::_pushOntoQueue(arStructuredData* theData){
  // we assume the next 3 iterators will find something, because we have a
  // valid piece of arStructuredData passed-in
  SZGmessageQueueLock::iterator i(_messageQueueLock.find(theData->getID()));
  SZGmessageQueueVar::iterator j(_messageQueueVar.find(theData->getID()));
  SZGmessageQueue::iterator k(_messageQueue.find(theData->getID()));
  ar_mutex_lock(i->second);
  (k->second).push_back(theData);
  (j->second)->signal();
  ar_mutex_unlock(i->second);
}

/// go ahead and pushed the received message onto the tagged queue (a single
/// queue for all message types, but indexed via ID). Signal a potentially
/// blocked getNextTaggedMessage(...) that it can proceed
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
    ar_mutex_lock(i->second.lock);
    // must set the tag, this tells the other side that the
    // condition variable firing was not accidental (as can happen under
    // POSIX)
    *(i->second.tag) = tag;
    i->second.var->signal();
    ar_mutex_unlock(i->second.lock);
  }
  ar_mutex_unlock(&_globalLock);
}

void arStructuredDataParser::_cleanupSynchronizers(list<int> tags){
  list<int>::iterator i;
  SZGtaggedMessageSync::iterator j;
  ar_mutex_lock(&_globalLock);
  for (i = tags.begin(); i != tags.end(); i++){
    j = _messageSync.find(*i);
    if (j != _messageSync.end()){
      _recycledSync.push_front(j->second);
      _messageSync.erase(j);
    }
  }
  ar_mutex_unlock(&_globalLock);
}
