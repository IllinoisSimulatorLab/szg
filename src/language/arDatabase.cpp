//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDatabase.h"
#include "arStructuredDataParser.h"
#include <sstream>

arDatabase::arDatabase() :
  _lang(NULL),
  _server(false),
  _bundlePathName("NULL"),
  _bundleName("NULL"),
  // We start at ID 1 for subsequent nodes since the root node has ID = 0
  // (the default)
  _nextAssignedID(1){
  ar_mutex_init(&_bufferLock);
  ar_mutex_init(&_eraseLock);
  ar_mutex_init(&_deletionLock);

  // The root node is already initialized properly by the default 
  // arDatabaseNode constructor.
  _nodeIDContainer.insert(map<int,arDatabaseNode*,less<int> >::
			  value_type (0,&_rootNode));
  // Important that the root node know who its owner is. Some node
  // insertion commands use the parent's internally stored database owner.
  _rootNode._databaseOwner = this;

  // Reset the function pointers.
  memset(_databaseReceive, 0, sizeof(_databaseReceive));
  memset(_parsingData, 0, sizeof(_parsingData));
  memset(_routingField, 0, sizeof(_routingField));
}

arDatabase::~arDatabase(){
  delete eraseData;
  delete makeNodeData;
}

/// Sometimes, we want to store supporting objects like textures or
/// sound clips in an application directory,
/// much like standard application data files. In many ways, this is
/// superior to storing them all in the same directory (which is
/// really unmaintainable). Conceptually, each application has its
/// own directory where it can store supporting objects, as installed on one
/// of the Syzygy system directory paths (like SZG_DATA or SZG_PYTHON).
/// When the arDatabase subclass (like arGraphicsDatabase or
/// arSoundDatabase) is initialized, its owning
/// program creates a "bundle path map" (by talking to the Phleet or by 
/// reading a local config file, depending upon mode of operation).
/// This associates a bundlePathName (SZG_DATA, SZG_PYTHON) with a
/// file system path like my_directory_1;my_directory_2;my_directory3.
/// If bundlePathName maps to a path like so, is not "NULL", and bundleName
/// is not "NULL", then addTexture (for arGraphicsDatabase) or
/// addFile (arSoundDatabase)  will look for the supporting object
/// (texture or sound file) on the path
/// my_directory_1/bundleName;my_directory_2/bundleName;
/// my_directory_3/bundleName, in addition to the texture path.
void arDatabase::setBundlePtr(const string& bundlePathName,
			      const string& bundleName){
  _bundlePathName = bundlePathName;
  _bundleName = bundleName;
}

/// Used to associated a bundle path name (SZG_DATA, SZG_PYTHON) with
/// a file system path my_directory_1;my_directory_2;my_directory_3.
void arDatabase::addBundleMap(const string& bundlePathName,
                              const string& bundlePath){
  map<string,string,less<string> >::iterator i
    = _bundlePathMap.find(bundlePathName);
  if (i != _bundlePathMap.end()){
    // If an entry is already present, must erase so that we can insert
    // something new.
    _bundlePathMap.erase(i);
  }
  _bundlePathMap.insert(map<string,string,less<string> >::value_type
                        (bundlePathName, bundlePath));
}

// This function is simply an ABOMINATION and will go away.
int arDatabase::getNodeID(const string& name, bool fWarn) {
  const arDatabaseNode* pNode = getNode(name,fWarn);
  return pNode ? pNode->getID() : -1;
}

arDatabaseNode* arDatabase::getNode(int theID, bool fWarn) {
  arNodeIDIterator i(_nodeIDContainer.find(theID));
  if (i == _nodeIDContainer.end()){
    if (fWarn){
      cerr << "arDatabase warning: no node with ID " << theID <<
	   (empty() ? " in empty database.\n" : ".\n");
    }
    return NULL;
  }
  return i->second;
}

// The meaning of the below function is changing. Names are no longer
// assumed to be unique. Indeed, this is a very important FEATURE now.
// It simplifies many of the name management issues that existed before...
// AND it allows us to use the names to present SEMANTIC information for
// mapping one tree to another. ONE PROBLEM: getNode is now HORRENDOUSLY slow
// in comparison to its old version (where one could rely on uniqueness of
// names via the mapping container). 
arDatabaseNode* arDatabase::getNode(const string& name, bool fWarn) {
  // conducts a breadth-first search for the node with the given name.
  arDatabaseNode* result = NULL;
  bool success = false;
  _rootNode._findNode(result, name, success);
  if (!success && fWarn){
    cerr << "arDatabase warning: no node \"" << name << ".\n";
  }
  return result;
}

/// What getNode(nodeName) really is...
arDatabaseNode* arDatabase::findNode(const string& name){
  arDatabaseNode* result = NULL;
  bool success = false;
  _rootNode._findNode(result, name, success);
  return result;
}

// NOT THREAD-SAFE!!!!!!! (because of the naming methodlogy)
// (also because we use the global data structure... but... then again...
// the dgMakeNode isn't thread safe for the same reason)
arDatabaseNode* arDatabase::newNode(arDatabaseNode* parent,
                                    const string& type,
                                    const string& name){
  string nodeName = name;
  if (nodeName == ""){
    // we must choose a default name
    stringstream def;
    def << "szg_default_" << _nextAssignedID;
    nodeName = def.str();
  }
  // BUG: NOT TESTING TO SEE IF THIS DATABASE NODE IS, INDEED, OWNED BY
  // THIS DATABASE!
  int parentID = parent->getID();
  makeNodeData->dataIn(_lang->AR_MAKE_NODE_PARENT_ID, &parentID, AR_INT, 1);
  int temp = -1;
  // we are requesting an ID be assigned.
  makeNodeData->dataIn(_lang->AR_MAKE_NODE_ID, &temp, AR_INT, 1);
  makeNodeData->dataInString(_lang->AR_MAKE_NODE_NAME, nodeName);
  makeNodeData->dataInString(_lang->AR_MAKE_NODE_TYPE, type);
  // The alter(...) call will return the new node.
  return alter(makeNodeData);
}

// NOT THREAD_SAFE EITHER!
bool arDatabase::eraseNode(int ID){
  arDatabaseNode* node = getNode(ID, false);
  if (!node){
    return false;
  }
  eraseData->dataIn(_lang->AR_ERASE_ID, &ID, AR_INT, 1);
  alter(eraseData);
  return true;
}

/// When transfering the database state, we often want to dump the structure
/// before dumping the data of the individual nodes.
bool arDatabase::fillNodeData(arStructuredData* data, arDatabaseNode* node){
  if (!data || !node){
    return false;
  }
  if (node->getID() <= 0){
    // This is the root node... it does not get mirrored.
    return false;
  }
  const int nodeID = node->getID();
  // guaranteed to have a parent since we are not the root node.
  const int parentID = node->getParent()->getID();
  return
    data->dataIn(_lang->AR_MAKE_NODE_PARENT_ID,&parentID,AR_INT,1) &&
    data->dataIn(_lang->AR_MAKE_NODE_ID,&nodeID,AR_INT,1) &&
    data->dataInString(_lang->AR_MAKE_NODE_NAME, node->getName()) &&
    data->dataInString(_lang->AR_MAKE_NODE_TYPE, node->getTypeString());
}

// In the case of node creation, we actually want to be able to return
// an arDatabaseNode* (i.e. the created node). This makes code easier to
// manage. In other cases, we return the only node we are guaranteed
// to have... the root node! NOTE: this really is necessary. Consider, for
// instance, the newNode(...) method of arDatabase. It actually needs to get
// the node back from alter since we are no longer counting on names being
// unique!
arDatabaseNode* arDatabase::alter(arStructuredData* inData){
  const ARint dataID = inData->getID();
  if (_databaseReceive[dataID]){
    return (this->*(_databaseReceive[dataID]))(inData);
    // Call one of _handleQueuedData _eraseNode _makeDatabaseNode.
    // If it fails, it prints its own error message,
    // so we don't need to print another one here.
  }
  arDatabaseNode* pNode = getNode(inData->getDataInt(_routingField[dataID]));
  if (!pNode)
    return NULL;
  if (!pNode->receiveData(inData)){
    cerr << "arDatabase warning: receiveData() of child \""
         << pNode->_name << "\" failed.\n";
    return NULL;
  }
  return &_rootNode;
}

arDatabaseNode* arDatabase::alterRaw(ARchar* theData){
  const ARint dataID = ar_rawDataGetID(theData);
  // parse() and alter() print their own warnings, in all cases of failure.
  _parsingData[dataID]->parse(theData);
  return alter(_parsingData[dataID]);
}

// Sure, we are loosing some info by returning bool here.... BUT by sending
// in a buffer packed with various calls, we're pretty much guaranteeing that
// we don't (or can't) care about specific outcomes.
bool arDatabase::handleDataQueue(ARchar* theData){
  ARint bufferSize, numberRecords;
  ar_unpackData(theData,&bufferSize,AR_INT,1);
  ar_unpackData(theData+AR_INT_SIZE,&numberRecords,AR_INT,1);
  ARint position = 2*AR_INT_SIZE;
  for (int i=0; i<numberRecords; i++){
    const int theSize = ar_rawDataGetSize(theData+position);
    if (!alterRaw(theData+position)){
      cerr << "arDatabase::handleDataQueue error: failure in record "
           << i+1 << " of " << numberRecords << ".\n";
      // We could process the remaining records, but better to tell caller
      // that an error happened.
      return false;
    }
    position += theSize;
    if (position > bufferSize){
      cerr << "arDatabase::handleDataQueue error: buffer overflow.\n";
      return false;
    }
  }
  return true;
}

/// Reads in the database in binary format.
bool arDatabase::readDatabase(const string& fileName, 
                              const string& path){
  FILE* sourceFile = sourceFile = ar_fileOpen(fileName,path,"rb");
  if (!sourceFile){
    cerr << "arDatabase warning: failed to read file \""
         << fileName << "\".\n";
    return false;
  }

  //**********AARGH!!! Do I really need to list this number explicitly?
  int bufferSize = 1000;
  ARchar* buffer = new ARchar[bufferSize];
  while (fread(buffer,AR_INT_SIZE,1,sourceFile) > 0){
    ARint recordSize;
    ar_unpackData(buffer,&recordSize,AR_INT,1);
    if (recordSize>bufferSize){
      // resize buffer
      delete [] buffer;
      bufferSize = 2 * recordSize;
      buffer = new ARchar[bufferSize];
      ar_packData(buffer,&recordSize,AR_INT,1);
    }
    const int result = fread(buffer+AR_INT_SIZE,1,
                             recordSize-AR_INT_SIZE,sourceFile);
    if (result < recordSize - AR_INT_SIZE){
      cerr << "arDatabase error: failed reading record: got only " 
           << result+AR_INT_SIZE
	   << " of " << recordSize << " expected bytes.\n";
      break;
    }
    alterRaw(buffer);
  }  
  delete [] buffer;

  fclose(sourceFile);
  return true;
}

bool arDatabase::readDatabaseXML(const string& fileName, 
                                 const string& path){

  FILE* sourceFile = ar_fileOpen(fileName,path,"r");
  if (!sourceFile){
    cerr << "arDatabase warning: failed to read file \""
         << fileName << "\".\n";
    return false;
  }

  arStructuredDataParser* parser 
    = new arStructuredDataParser(_lang->getDictionary());
  arStructuredData* record;
  arFileTextStream fileStream;
  fileStream.setSource(sourceFile);
  bool done = false;
  while (!done){
    record = parser->parse(&fileStream);
    if (record){
      alter(record);
      parser->recycle(record);
    }
    else{
      done = true;
    }
  }
  delete parser;

  fclose(sourceFile);
  return true;
}

/// Attaching means to create a copy of the file in the database,
/// with the file's root node mapped to parent. All other nodes in the
/// file will be associated with new nodes in the database.
// DOH! CUT_AND_PASTE IS REARING ITS UGLY HEAD!
bool arDatabase::attachXML(arDatabaseNode* parent,
			   const string& fileName,
			   const string& path){
  FILE* source = ar_fileOpen(fileName, path, "r");
  if (!source){
    cerr << "arDatabase warning: failed to read file \""
         << fileName << "\".\n";
    return false;
  }
  arStructuredDataParser* parser 
    = new arStructuredDataParser(_lang->getDictionary());
  arStructuredData* record;
  arFileTextStream fileStream;
  fileStream.setSource(source);
  bool done = false;
  map<int, int, less<int> > nodeMap;
  while (!done){
    record = parser->parse(&fileStream);
    if (record){
      bool success = _filterIncoming(parent, record, nodeMap, true);
      parser->recycle(record);
      if (!success){
        // There was an unrecoverable error. _filterIncoming already
	// complained so do not do so here.
        break;
      }
    }
    else{
      done = true;
    }
  }
  delete parser;
  // best to close this way...
  fileStream.ar_close();
  return true;
}

/// Mapping means attempting to merge the file into the existing database
/// in a reasonable way, starting with the root node of the file being
/// mapped to parent. When a new node is defined in the file, the
/// function looks at the database node to which its file-parent maps.
/// It then searches below this database node to find a node with the
/// same name and type. If such can be found, it creates an association 
/// between that database node and the file node. If none such can be found,
/// it creates a new node in the database and associates that with the file
/// node.
bool arDatabase::mapXML(arDatabaseNode* parent,
                        const string& fileName,
                        const string& path){
  FILE* source = ar_fileOpen(fileName, path, "r");
  if (!source){
    cerr << "arDatabase warning: failed to read file \""
         << fileName << "\".\n";
    return false;
  }
  arStructuredDataParser* parser 
    = new arStructuredDataParser(_lang->getDictionary());
  arStructuredData* record;
  arFileTextStream* fileStream = new arFileTextStream();
  fileStream->setSource(source);
  bool done = false;
  map<int, int, less<int> > nodeMap;
  while (!done){
    record = parser->parse(fileStream);
    if (record){
      bool success = _filterIncoming(parent, record, nodeMap, false);
      parser->recycle(record);
      if (!success){
        // There was an unrecoverable error. _filterIncoming already
	// complained so do not do so here.
        break;
      }
    }
    else{
      done = true;
    }
  }
  delete parser;
  // best to close this way...
  fileStream->ar_close();
  return true;
}

/// Writes the database to a file using a binary format.
bool arDatabase::writeDatabase(const string& fileName,
                               const string& path){
  FILE* destFile = ar_fileOpen(fileName, path, "wb");
  if (!destFile){
    cerr << "arDatabase warning: failed to write file \""
         << fileName << "\".\n";
    return false;
  }
  size_t bufferSize = 1000;
  ARchar* buffer = new ARchar[bufferSize];
  arStructuredData nodeData(_lang->find("make node"));
  _writeDatabase(&_rootNode, nodeData, buffer, bufferSize, destFile); 
  delete [] buffer;
  fclose(destFile);
  return true;
}

/// Writes the database to a file using an XML format.
bool arDatabase::writeDatabaseXML(const string& fileName,
                                  const string& path){
  return writeRootedXML(&_rootNode, fileName, path); 
}

bool arDatabase::writeRootedXML(arDatabaseNode* parent,
                                const string& fileName,
                                const string& path){
  FILE* destFile = ar_fileOpen(fileName, path, "w");
  if (!destFile){
    cerr << "arDatabase warning: failed to write file \""
         << fileName << "\".\n";
    return false;
  }
  arStructuredData nodeData(_lang->find("make node"));
  _writeDatabaseXML(parent, nodeData, destFile);
  fclose(destFile);
  return true;
}

// NOTE: this is another attempt at the algorithm found in _filterIncoming
bool arDatabase::createNodeMap(int externalNodeID, 
                               arDatabase* externalDatabase,
                               map<int, int, less<int> >& nodeMap){
  bool failure = false;
  _createNodeMap(&_rootNode, externalNodeID, externalDatabase, nodeMap,
		 failure);
  return !failure;
}

// NOTE: this code is, essentially, also found in _filterIncoming!
bool arDatabase::filterData(arStructuredData* record, 
			    map<int, int, less<int> >& nodeMap){
  int nodeID = record->getDataInt(_routingField[record->getID()]);
  map<int, int, less<int> >::iterator i = nodeMap.find(nodeID);
  if (i == nodeMap.end()){
    cout << "arDatabase error: record filter failed.\n";
    return false;
  }
  nodeID = i->second;
  record->dataIn(_routingField[record->getID()], &nodeID, AR_INT, 1);
  return true;
}

bool arDatabase::empty(){
  ar_mutex_lock( &_deletionLock );
  bool isEmpty = _rootNode._children.begin() == _rootNode._children.end();
  ar_mutex_unlock( &_deletionLock );
  return isEmpty;
}

/// \todo inefficient: this locking section is too big.
void arDatabase::reset(){
  ar_mutex_lock( &_deletionLock );
  ar_mutex_lock(&_eraseLock);
  for (arNodeIDIterator i(_nodeIDContainer.begin());
       i != _nodeIDContainer.end();
       ++i){
    arDatabaseNode* theNode = i->second;
    if (theNode->getID() != 0){
      // not the root node, delete
      delete theNode;
    }
  }

  _nodeIDContainer.clear();
  _nodeIDContainer.insert(map<int,arDatabaseNode*,less<int> >::
			  value_type (0,&_rootNode));
  _rootNode._children.clear();
  _nextAssignedID = 1;
  ar_mutex_unlock(&_eraseLock);
  ar_mutex_unlock( &_deletionLock );
}

void arDatabase::printStructure(int maxLevel){
  printStructure(maxLevel, cout);
}

/// Prints the structure of the database tree, including node IDs, node names,
/// and node types. The information needed to reflect on the database.
/// @param maxLevel (optional) how far down the tree hierarchy we will go
/// (default is 10,000)
void arDatabase::printStructure(int maxLevel, ostream& s){
  s << "Database structure: \n";
  _rootNode._printStructureOneLine(0, maxLevel,s);
}

// The node creation/deletion functions require arStructuredData storage.
// However, this cannot be created (under the current goofy model) until
// the language is set (like graphics language or sound language) by one
// of the subclasses. Consequently, this function needs to exist to be
// called from arGraphicsDatabase, etc. constructors. NOTE: this model
// will SOON BE IMPROVED... AND THIS WILL BECOME UNECESSARY!
bool arDatabase::_initDatabaseLanguage(){
  // Allocate storage for external parsing.
  arTemplateDictionary* d = _lang->getDictionary();
  eraseData = new arStructuredData(d, "erase");
  makeNodeData = new arStructuredData(d, "make node");
  if (!eraseData || !makeNodeData ||
      !*eraseData || !*makeNodeData) {
    if (eraseData)
      delete eraseData;
    if (makeNodeData)
      delete makeNodeData;
    return false;
  }
  
  // Create the parsing helpers. NOTE: THIS WILL GO AWAY ONCE THE
  // arStructuredDataParser is integrated!
  for (arTemplateType::iterator iter = d->begin();
       iter != d->end(); ++iter){
    const int ID = iter->second->getID();
    _parsingData[ID] = new arStructuredData(iter->second);
    _routingField[ID] = iter->second->getAttributeID("ID");
  }

  // Go ahead and register the callbacks.
  arDataTemplate* t = _lang->find("erase");
  _databaseReceive[t->getID()] = &arDatabase::_eraseNode;
  t = _lang->find("make node");
  _databaseReceive[t->getID()] = &arDatabase::_makeDatabaseNode;
  return true;
}

/// When the database receives a message demanding erasure of a node,
/// it is handled here. On failure, return NULL. On success, return a
/// pointer to the root node. NOTE: this works will the old semantics 
/// when this returned bool.
arDatabaseNode* arDatabase::_eraseNode(arStructuredData* inData){
  int ID = inData->getDataInt(_lang->AR_ERASE_ID);
  arDatabaseNode* startNode = getNode(ID);
  if (!startNode){
    cerr << "arDatabase::_eraseNode failed: no such node.\n";
    return NULL;
  }
  ar_mutex_lock(&_eraseLock);
  // Delete the startNode from the child list of its parent
  arDatabaseNode* parent = startNode->getParent();
  if (parent){
    for (list<arDatabaseNode*>::iterator i = parent->_children.begin();
         i != parent->_children.end(); i++){
      if ((*i)->getID() == startNode->getID()){
	// This means we've found the one instance of the node in the
	// parent's list, since IDs are unique. No need to look further.
        parent->_children.erase(i);
	break;
      }
    }
  }
  _eraseNode(startNode);
  ar_mutex_unlock(&_eraseLock);
  return &_rootNode;
}

/// When the database receives a message demanding creation of a node,
/// it is handled here. If we cannot succeed for some reason, return
/// NULL, otherwise, return a pointer to the node in question.
arDatabaseNode* arDatabase::_makeDatabaseNode(arStructuredData* inData){
  const int parentID = inData->getDataInt(_lang->AR_MAKE_NODE_PARENT_ID);
  const int theID = inData->getDataInt(_lang->AR_MAKE_NODE_ID);
  const string name(inData->getDataString(_lang->AR_MAKE_NODE_NAME));
  const string type(inData->getDataString(_lang->AR_MAKE_NODE_TYPE));
  // Must use the virtual factory function.
  arDatabaseNode* node = _makeNode(type);
  if (_insertDatabaseNode(node, parentID, theID, name) < 0){
    cerr << "arDatabase error: "
	 << " _makeDatabaseNode failed when makeNode failed\n\tfor ID "
         << theID << ", name/parent/type \""
	 << name << "/" << parentID << "/" << type << "\"\n";
    delete node;
    return NULL;
  }
  return node;
}

// Return -1 on error.  Generate a new ID, if passed-in ID is -1.
// Also, if it turns out we have a duplicate node (based on ID), just
// return that ID.
// NOTE: the type-string is unused here... but it is used in subclasses.
int arDatabase::_insertDatabaseNode(arDatabaseNode* node,
                                    int parentID,
                                    int nodeID,
                                    const string& nodeName){
  // Make sure no node exists with this ID.
  if (nodeID != -1){
    // In this case, we're trying to insert a node with a specific ID
    // (THIS IS WHAT LET'S THE arGraphicsServer SUCCESSFULLY MANIPULATE
    // ITS REMOTE CLIENTS WITHOUT MAPPING)
    arDatabaseNode* pNode = getNode(nodeID, false);
    if (pNode){
      // Such a node already exists. Return its ID.
      cout << "arDatabase warning: using prexisting node " << pNode->getName()
	   << ".\n";
      return pNode->getID(); 
    }
  }
  // Make sure that the parent node (as given by ID) exists.
  arDatabaseNode* parentNode = getNode(parentID, false);
  if (!parentNode){
    cout << "arDatabase warning: parent (ID=" << parentID << ") "
	 << "does not exist.\n";
    return -1;
  }
  // Set parameters.
  node->_parent = parentNode;
  node->_name = nodeName;
  // Set ID appropriately.
  if (nodeID == -1){
    // assign an ID automatically
    node->_ID = _nextAssignedID++;
  }
  else{
    node->_ID = nodeID;
    if (_nextAssignedID <= nodeID){
      _nextAssignedID = nodeID+1;
    }
  }
  // Add to children of parent.
  ar_mutex_lock(&_bufferLock);
    parentNode->_children.push_back(node);
    _nodeIDContainer.insert(
      map<int,arDatabaseNode*,less<int> >::value_type(node->_ID, node));
  ar_mutex_unlock(&_bufferLock);

  // nodes can inherit attributes from nodes above them in the tree
  // BOY THIS SURE IS SUCKY DESIGN!
  // THIS REALLY NEEDS TO CHANGE! (perhaps via some kind of...
  // "rendering context", for lack of a better word)
  node->initialize(this);

  if (node->_ID < 0)
    cerr << "arDatabase error: "
	 << "insertion failed because of outNode->initialize().\n";
  return node->_ID;
}

void arDatabase::_writeDatabase(arDatabaseNode* pNode,
                                arStructuredData& nodeData,
                                ARchar*& buffer,
                                size_t& bufferSize,
                                FILE* destFile){
  // The following will fail for the root node. Otherwise, we write the
  // node description to the file.
  if (fillNodeData(&nodeData, pNode)){
    size_t recordSize = nodeData.size();
    nodeData.pack(buffer);
    if (fwrite(buffer,1,recordSize,destFile) != recordSize){
      cerr << "arDatabase::writeDatabase warning: failed to write to file.\n";
    }
    arStructuredData* theRecord = pNode->dumpData();
    recordSize = theRecord->size();
    if (recordSize > bufferSize){
      delete [] buffer;
      bufferSize = 2 * recordSize;
      buffer = new ARchar[bufferSize];
    }
    theRecord->pack(buffer);
    if (fwrite(buffer,1,recordSize,destFile) != recordSize){
      cerr << "arDatabase::writeDatabase warning: failed to write to file.\n";
    }
    delete theRecord;
  }
  // Now, recurse to the node's children.
  list<arDatabaseNode*> children = pNode->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _writeDatabase(*i, nodeData, buffer, bufferSize, destFile);
  }
}

void arDatabase::_writeDatabaseXML(arDatabaseNode* pNode,
                                   arStructuredData& nodeData,
				   FILE* destFile){
  // The following will fail for the root node but succeed for other nodes.
  if (fillNodeData(&nodeData, pNode)){
    nodeData.print(destFile);
    arStructuredData* theRecord = pNode->dumpData();
    theRecord->print(destFile);
    delete theRecord;                               
  }
  // Now, recurse to the node's children.
  list<arDatabaseNode*> children = pNode->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _writeDatabaseXML(*i, nodeData, destFile);
  }  
}

void arDatabase::_eraseNode(arDatabaseNode* node){
  // Go to each child and erase that node
  for (list<arDatabaseNode*>::iterator i = node->_children.begin();
       i != node->_children.end(); i++){
    _eraseNode(*i);
  }
  // Delete this node
  const int nodeID = node->getID();
  _nodeIDContainer.erase(nodeID);
  /// \bug memory leak in texture held by the deleted node
  delete node;
}

void arDatabase::_createNodeMap(arDatabaseNode* localNode, 
                                int externalNodeID, 
                                arDatabase* externalDatabase, 
				map<int, int, less<int> >& nodeMap,
		                bool& failure){
  // If the map has failed, just return.
  if (failure){
    return;
  }
  // If the current node is the root, then we automatically map.
  // Otherwise, find at or below the current node to get a suitable mapping
  // choice.
  arDatabaseNode* match;
  if (localNode->getTypeString() == "root"){
    nodeMap.insert(map<int, int, less<int> >::value_type(localNode->getID(),
							 externalNodeID));
    match = externalDatabase->getNode(externalNodeID);
    if (!match){
      cout << "arDatabase error: could not map top level node in the "
	   << "creation of node map.\n";
      failure = true;
      return;
    }
  }
  else{
    arDatabaseNode* node = externalDatabase->getNode(externalNodeID);
    if (!node){
      cout << "arDatabase error: could not find specified node in the "
	   << "creation of node map.\n";
      failure = true;
      return;
    }
    // Need to do two different things based on whether or not this
    // is a "name" node (in which case map to any name node) or this is
    // some other kind of node (in which case only map to a node of the
    // same node and type)
    if (localNode->getTypeString() == "name"){
      match = node->findNodeByType(localNode->getTypeString());
    }
    else{
      match = node->findNode(localNode->getName());
    }
    if (match && match->getTypeString() == localNode->getTypeString()){
      nodeMap.insert(map<int, int, less<int> >::value_type(localNode->getID(),
							   match->getID()));
    }
    else{
      cout << "arDatabase error: could not find suitable match in the "
	   << "creation of node map.\n";
      failure = true;
      return;
    }
  }
  // If we're here, we succeeded in adding a mapping. Now, recurse through the
  // children
  list<arDatabaseNode*> children = localNode->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    if (!failure){
      _createNodeMap(*i, match->getID(), externalDatabase, nodeMap, failure);
    }
    else{
      return;
    }
  } 
}

/// Helper function for filtering incoming messages (needed for mapping
/// one database to another as in attachXML and mapXML). The record is
/// changed in place, as is the nodeMap. allNew should be set to true
/// if every node should be a new one. And set to false if we will 
/// attempt to associate nodes with existing ones (as in mapXML, instead of
/// attachXML. NOTE: an alter actually occurs in here!
//
// In here, we have to consider the fact that the mapped source might
// NOT be starting from the root, but from some other node. Consequently,
// we pass in a database node to map this parent-less node to...
bool arDatabase::_filterIncoming(arDatabaseNode* mappingRoot,
                                 arStructuredData* record, 
	                         map<int, int, less<int> >& nodeMap,
                                 bool allNew){
  map<int, int, less<int> >::iterator i;
  if (record->getID() == _lang->AR_MAKE_NODE){
    int parentID = record->getDataInt(_lang->AR_MAKE_NODE_PARENT_ID);
    string nodeName = record->getDataString(_lang->AR_MAKE_NODE_NAME);
    string nodeType = record->getDataString(_lang->AR_MAKE_NODE_TYPE);
    int originalNodeID = record->getDataInt(_lang->AR_MAKE_NODE_ID);
    i = nodeMap.find(parentID);
    if (i == nodeMap.end()){
      nodeMap.insert(map<int, int, less<int> >::value_type
                     (parentID, mappingRoot->getID()));
      i = nodeMap.find(parentID);
    }
    // Go ahead and try to map the new node.
    // First, get the current parent node. HMMM... SHOULDN'T I ALLOW
    // FOR THE FACT THAT THE FOLLOWING CALL MIGHT FAIL? 
    arDatabaseNode* currentParent = getNode(i->second);
    arDatabaseNode* target;
    bool success = false;
    // NOTE: we map things differently based on whether or not this is
    // a "name" node (in which case just map to any other name node)
    // or, otherwise, we must map to a node with the same name and type.
    if (nodeType == "name"){
      currentParent->_findNodeByType(target, nodeType, success);
    }
    else{
      currentParent->_findNode(target, nodeName, success);
    }
    if (!allNew && success && target->getTypeString() == nodeType){
      // go ahead and map to this node.
      nodeMap.insert(map<int, int, less<int> >::value_type
	             (originalNodeID, target->getID()));
      // In this case, DO NOT create a new node! The record is simply
      // discarded.
      return true;
    }
    else{
      // We could not find a suitable node for mapping.
      // (OR WE ARE JUST INSERTING ALL NEW NODES)
      int newNodeID = -1;
      // Setting the parameter like so indicates that we will be
      // requesting a new node.
      record->dataIn(_lang->AR_MAKE_NODE_ID, &newNodeID, AR_INT, 1);
      // Don't forget to remap the parent ID
      int newParentID = i->second;
      record->dataIn(_lang->AR_MAKE_NODE_PARENT_ID, &newParentID,
	             AR_INT, 1);
      arDatabaseNode* newNode = alter(record);
      if (!newNode){
	cout << "arDatabase error: mapping of XML failed in node "
	     << "creation.\n";
	return false;
      }
      // It worked. Update the map.
      nodeMap.insert(map<int, int, less<int> >::value_type
      	             (originalNodeID, newNode->getID()));
      // Nothing else to do.
      return true;
    }
  }
  else{
    // For all other records, we simply re-map the ID.
    int nodeID = record->getDataInt(_routingField[record->getID()]);
    i = nodeMap.find(nodeID);
    if (i == nodeMap.end()){
      cout << "arDatabase error: mapping of XML file failed in "
	   << "node remap.\n";
      return false;
    }
    nodeID = i->second;
    record->dataIn(_routingField[record->getID()], &nodeID, AR_INT, 1);
    alter(record);
    return true;
  }
}

// Here, we construct the nodes that are unqiue to the arDatabase (as opposed
// to subclasses).
arDatabaseNode* arDatabase::_makeNode(const string& type){
  // DO NOT COMPLAIN IN THIS FUNCTION. This will be handled in the 
  // subclassed versions.
  arDatabaseNode* outNode = NULL;
  if (type == "name"){
    outNode = new arNameNode();
  }
  return outNode;
}
