//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arMasterSlaveDataRouter.h"
#include "arLogStream.h"

arMasterSlaveDataRouter::arMasterSlaveDataRouter() :
  _parser(NULL),
  _bufferPosition(0),
  _started(false),
  _nextID(0) {
}

arMasterSlaveDataRouter::~arMasterSlaveDataRouter(){
  if (_parser) {
    delete [] _parser;
  }
}

// Draws all the objects connected to the arMasterSlaveDataRouter. Very
// simple function, just goes through the draw list, drawing each one in turn.
void arMasterSlaveDataRouter::draw(){
  map<int,arFrameworkObject*,less<int> >::const_iterator i;
  for (i = _objectTable.begin(); i != _objectTable.end(); i++){
    i->second->draw();
  }
}

// Start really doesn't do much. It simply indicates that we are finished
// registering new framework objects and should create the language.
bool arMasterSlaveDataRouter::start(){
  // RENUMBER THE TEMPLATES IN THE DICTIONARY
  // BEFORE CREATING THE PARSER. THE TEMPLATES HAVE BEEN ALTERED BY
  // BEING MERGED INTO THE GLOBAL LANGUAGE.
  _dictionary.renumber();
  _parser = new arStructuredDataParser(&_dictionary);
  _started = true;
  // start needs to occur recursively
  map<int,arFrameworkObject*,less<int> >::const_iterator i;
  for (i = _objectTable.begin(); i != _objectTable.end(); i++){ 
    i->second->start();
  }
  return true;
}

// We will relay messages from the registered framework object on the master
// to it's corresponding framework object on the slaves. Messages are routed
// via object ID... and object IDs are assigned sequentially. Consequently,
// the master and the slaves must register the same set of objects in the
// same order for this to work (which is a reasonable assumption).
// We also get the arDataTemplates used by the arFrameworkObject and integrate
// them into the language used.
// NOTE: THE CURRENT MODEL IS THAT POINTERS TO THE arDataTemplates ARE 
// PASSED UP THE CHAIN. WHY? THINK ABOUT NESTED LANGUAGES (FROM NESTED
// arMasterSlaveDataRouters). IN THIS CASE, THE IDs SET BY A DICTIONARY
// NEED TO TRICKLE-DOWN.
bool arMasterSlaveDataRouter::registerFrameworkObject
                                (arFrameworkObject* object){
  if (_started){
    ar_log_warning() << "arMasterSlaveDataRouter cannot register object after start.\n";
  }
  if (!object){
    ar_log_warning() << "arMasterSlaveDataRouter: registerFrameworkObject(NULL).\n";
    return false;
  }

  // Retrieve the templates from the framework object.
  arTemplateDictionary* templateDictionary = object->getDictionary();
  // Must first check that they all have the magic "szg_router_id" field
  arTemplateType::const_iterator i;
  for (i = templateDictionary->begin(); 
       i != templateDictionary->end(); i++){
    if (i->second->getAttributeID("szg_router_id") == -1){
      ar_log_warning() << "arMasterSlaveDataRouter: template lacks ID field.\n";
      return false;
    }
  }
  // Copy the templates and add them to the language.
  for (i = templateDictionary->begin(); 
       i != templateDictionary->end(); i++){
    // first, let's see if we can find the template name
    arDataTemplate* temp = _dictionary.find(i->second->getName());
    // NOTE: it is NOT necessarily an error to have duplicate templates.
    // For instance, we could have 2 objects of the same type.
    // TO-DO: If the templates have the same name... WE SHOULD CHECK IF 
    // THEY ARE EQUAL!!!
    if (!temp){
	  // NOTE: THIS WILL RENUMBER THE RECORDS FROM SUBORDINATE OBJECTS!
	  // THIS IS CORRECTED IN START!
      _dictionary.add(i->second);
    }
    else{
      // Give the owned arFrameworkObject's template,
      // which is presumed to be equal, the same ID.
      i->second->setID(temp->getID());
    }
  }
  // The framework object gets an ID and is added to the container
  _objectTable.insert(map<int, arFrameworkObject*, less<int> >::value_type
                      (_nextID, object));
  _nextID++;
  return true;
}

// Makes the framework dump the state of all the registered objects that
// have been updated since the last dump.
void arMasterSlaveDataRouter::internalDumpState(){
  // We start at the beginning of the buffer.
  _bufferPosition = 0;
  map<int, arFrameworkObject*, less<int> >::iterator i;
  for (i = _objectTable.begin(); i != _objectTable.end(); i++){
    if (i->second->changed()){
      _addStructuredDataToBuffer(i->first, i->second->dumpData());
    }
  } 
}

// Propagates the remote stream config down the tree. NOTE: THIS IS
// NO DOUBT VERY INEFFICIENT FOR LARGE NUMBERS OF OBJECTS (SINCE THE 
// MASTER-SLAVE FRAMEWORK DOES IT ONCE PER FRAME)
void arMasterSlaveDataRouter::setRemoteStreamConfig(const arStreamConfig& c){
  _remoteStreamConfig = c;
  map<int,arFrameworkObject*,less<int> >::iterator i;
  for (i = _objectTable.begin(); i != _objectTable.end(); i++){
    i->second->setRemoteStreamConfig(c);
  }
}

// Takes messages from the given buffer and routes them to the objects on
// this end. NOTE: data format translation (i.e. big-endian to little-endian)
// must happen here, since the buffer was transfered from computer to
// computer without reformating the raw data.
bool arMasterSlaveDataRouter::routeMessages(char* inBuffer, int bufferSize){
  for (int inBufferPosition = 0; inBufferPosition < bufferSize; ){
    int delta = -1;
    arStructuredData* message = _parser->parse(inBuffer+inBufferPosition, 
                                               delta,
                                               _remoteStreamConfig);
    if (!message){
      ar_log_warning() << "arMasterSlaveDataRouter failed to parse data.\n";
      return false;
    }

    // advance position to that of the next record
    inBufferPosition += delta;
    // before going on, remember to send record to framework object
    int objectID = -1;
    message->dataOut("szg_router_id",&objectID,AR_INT,1);
    map<int, arFrameworkObject*, less<int> >::iterator i = _objectTable.find(objectID);
    if (i != _objectTable.end()){
      if (!i->second->receiveData(message)){
        ar_log_warning() << "arMasterSlaveDataRouter: receiveData failed.\n";
      }
    }
    else{
      ar_log_warning() << "arMasterSlaveDataRouter: no such object in table.\n";
    }
    _parser->recycle(message);
  }
  return true;
}

// Once the internalDumpState() has occured, the state information needs to
// be accessible for transfer out via a socket.
char* arMasterSlaveDataRouter::getTransferBuffer(int& bufferSize){
  bufferSize = _bufferPosition;
  return _buffer.data;
}

// Pushes the packed version of the arStructuredData record into the buffer,
// expanding the buffer's size if necessary.
void arMasterSlaveDataRouter::_addStructuredDataToBuffer
  (int objectID, arStructuredData* data){
  if (!data){
    // perhaps arFrameworkObject's dumpData behavior is still the default.
    return;
  }

  // Add the object ID for routing. Bug: verbal field tag is inefficient.
  // This alters record size, so do this before calculating that.
  data->dataIn("szg_router_id",&objectID,AR_INT,1);
  const int recordSize = data->size();
  const int newBufferPosition = recordSize + _bufferPosition;
  if (newBufferPosition > _buffer.size()){
    // _buffer will be updated. NOTE: we rely on the fact that existing
    // data will be copied! (so ar_growBuffer is not suitable!)
    _buffer.grow(2*newBufferPosition);
    ar_log_remark() << "arMasterSlaveDataRouter growing buffer to size = "
	 << 2*newBufferPosition << "\n";
  }
  data->pack((_buffer.data)+_bufferPosition);
  _bufferPosition = newBufferPosition;
  // NOTE: we must recycle this data.
  // Caller deletes anything returned from dumpData().
  // Eventually, this should be recycled to the data parser.
  delete data;
}
