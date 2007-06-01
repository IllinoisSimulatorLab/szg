//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDatabase.h"
#include "arStructuredDataParser.h"
#include "arLogStream.h"

arDatabase::arDatabase() :
  _lang(NULL),
  _typeCode(AR_GENERIC_DATABASE),
  _typeString("generic"),
  _server(false),
  _bundlePathName("NULL"),
  _bundleName("NULL"),
  // We start at ID 1 for subsequent nodes since the root node has ID = 0
  // (the default)
  _nextAssignedID(1),
  _dataParser(NULL){

  // The root node is already initialized properly by the default 
  // arDatabaseNode constructor.
  _nodeIDContainer.insert(map<int,arDatabaseNode*,less<int> >::
			  value_type (0,&_rootNode));
  // Root node must know its owner. Some node
  // insertion commands use the parent's internally stored database owner.
  _rootNode._setOwner(this);

  // Reset the function pointers.
  memset(_databaseReceive, 0, sizeof(_databaseReceive));
  memset(_parsingData, 0, sizeof(_parsingData));
  memset(_routingField, 0, sizeof(_routingField));
}

arDatabase::~arDatabase(){
  // Unreference our nodes in the database;
  // the nodes self-delete when their ref count falls to 0.
  // Don't use an arDatabase method here that passes a message to alter().
  _eraseNode(&_rootNode);
  delete eraseData;
  delete makeNodeData;
  if (_dataParser)
    delete _dataParser;
}

// Sometimes, we want to store supporting objects like textures or
// sound clips in an application directory,
// much like standard application data files. In many ways, this is
// superior to storing them all in the same directory (which is
// really unmaintainable). Conceptually, each application has its
// own directory where it can store supporting objects, as installed on one
// of the Syzygy system directory paths (like SZG_DATA or SZG_PYTHON).
// When the arDatabase subclass (like arGraphicsDatabase or
// arSoundDatabase) is initialized, its owning
// program creates a "bundle path map" (by talking to the Phleet or by 
// reading a local config file, depending upon mode of operation).
// This associates a bundlePathName (SZG_DATA, SZG_PYTHON) with a
// file system path like my_directory_1;my_directory_2;my_directory3.
// If bundlePathName maps to a path like so, is not "NULL", and bundleSubDirectory
// is not "NULL", then addTexture (for arGraphicsDatabase) or
// addFile (arSoundDatabase)  will look for the supporting object
// (texture or sound file) on the path
// my_directory_1/bundleSubDirectory;my_directory_2/bundleSubDirectory;
// my_directory_3/bundleSubDirectory, in addition to the texture path.
void arDatabase::setDataBundlePath(const string& bundlePathName,
			           const string& bundleSubDirectory){
  _bundlePathName = bundlePathName;
  _bundleName = bundleSubDirectory;
}

// Associate a bundle path name (SZG_DATA, SZG_PYTHON) with
// a file system path my_directory_1;my_directory_2;my_directory_3.
void arDatabase::addDataBundlePathMap(const string& bundlePathName,
                                      const string& bundlePath){
  if (bundlePath == "NULL") {
    ar_log_remark() << "addDataBundlePathMap: undefined " << bundlePathName << "/path.\n";
    return;
  }

  map<string,string,less<string> >::iterator i =
    _bundlePathMap.find(bundlePathName);
  if (i != _bundlePathMap.end()){
    // Erase pre-existing entry before replacing it.
    _bundlePathMap.erase(i);
  }
  _bundlePathMap.insert(map<string,string,less<string> >::value_type
                        (bundlePathName, bundlePath));
}

// Find a node by name,
// returning the ID of the first such node found (else -1).
int arDatabase::getNodeID(const string& name, bool fWarn) {
  const arDatabaseNode* pNode = getNode(name,fWarn);
  return pNode ? pNode->getID() : -1;
}

arDatabaseNode* arDatabase::_ref(arDatabaseNode* node, const bool fRef) {
  if (node && fRef)
    node->ref();
  return node;
}

// Note how this call is thread-safe using the global arDatabase lock.
// Consequently, it cannot be called internally in any of the arDatabase
// (or subclass) code for message processing in order to avoid deadlocks.
arDatabaseNode* arDatabase::getNode(int ID, bool fWarn, bool refNode){
  arGuard dummy(_l);
  return _ref(_getNodeNoLock(ID, fWarn), refNode);
}

arDatabaseNode* arDatabase::getNodeRef(int ID, bool fWarn) {
  return getNode(ID, fWarn, true);
}


// Node names are not assumed to be unique within the arDatabase.
// Consequently, this function is really just a restatement of findNode().
// We do a breadth-first search for the first node with the given name and
// return that.
arDatabaseNode* arDatabase::getNode(const string& name, bool fWarn, 
                                    bool refNode){
  // Search breadth-first for the node with the given name.
  arDatabaseNode* result = NULL;
  bool success = false;
  arGuard dummy(_l);
  _rootNode._findNode(result, name, success, NULL, true);
  if (!success && fWarn){
    cerr << "arDatabase warning: no node '" << name << "'.\n";
  }
  return _ref(result, refNode);
}

arDatabaseNode* arDatabase::getNodeRef(const string& name, bool fWarn) {
  return getNode(name, fWarn, true);
}

// Search depth-first for a node with the given name, from the arDatabase's root.
arDatabaseNode* arDatabase::findNode(const string& name, bool refNode) {
  arDatabaseNode* result = NULL;
  bool success = false;
  arGuard dummy(_l);
  _rootNode._findNode(result, name, success, NULL, true);
  return _ref(result, refNode);
}

arDatabaseNode* arDatabase::findNodeRef(const string& name) {
  return findNode(name, true);
}

arDatabaseNode* arDatabase::findNode(arDatabaseNode* node, const string& name,
			             bool refNode) {
  if (!node || !node->active() || node->getOwner() != this){
    ar_log_warning() << "arDatabaseNode::findNode from non-owned node.\n";
    return NULL;
  }

  arGuard dummy(_l);
  // Whether or not the caller has requested the ptr be
  // ref'ed, we CANNOT request this of the arDatabaseNode, since that will
  // result in a call to arDatabase::findNode again and an infinite recursion.
  return _ref(node->findNode(name, false), refNode);
}

arDatabaseNode* arDatabase::findNodeRef(arDatabaseNode* node, const string& name) {
  return findNode(node, name, true);
}

arDatabaseNode* arDatabase::findNodeByType(arDatabaseNode* node, 
                                           const string& nodeType,
			                   bool refNode){
  if (!node || !node->active() || node->getOwner() != this){
    ar_log_warning() << "arDatabase can't find from null, inactive, or unowned node.\n";
    return NULL;
  }

  arGuard dummy(_l);
  // Whether or not the caller has requested the ptr be
  // ref'ed, we CANNOT request this of the arDatabaseNode, since that will
  // result in a call to arDatabase::findNodeByType again and an infinite recursion.
  return _ref(node->findNodeByType(nodeType, false), refNode);
}

arDatabaseNode* arDatabase::findNodeByTypeRef(arDatabaseNode* node, 
                                              const string& nodeType) {
  return findNodeByType(node, nodeType, true);
}

// Just a way to make getting a node's parent thread-safe with respect to
// the other database operations. The idea is that the alter method
// (within which all database operations occur) will be locked.
// This is called when an owned node's getParentRef() method is invoked.
arDatabaseNode* arDatabase::getParentRef(arDatabaseNode* node){
  arGuard dummy(_l);

  // Make sure this is a *active* node owned by this database.
  // Here, active means that the node is owned AND has a parent (or is the
  // root node).
  if (!node || !node->active() || node->getOwner() != this)
    return NULL;

  // We CANNOT call arDatabaseNode::getParentRef here, since that
  // will simply lead to another call to arDatabase::getParentRef (an
  // infinite recursion).
  return _ref(node->getParent(), true);
}

list<arDatabaseNode*> arDatabase::getChildrenRef(arDatabaseNode* node){
  list<arDatabaseNode*> l;
  arGuard dummy(_l);
  // Make sure this is a *active* node owned by this database.
  // Here, active means that the node is owned AND has a parent (or is the
  // root node).
  if (!node || !node->active() || node->getOwner() != this)
    return l;

  // NOTE: We CANNOT call arDatabaseNode::getChildrenRef here, since that
  // will simply lead to another call to arDatabase::getChildrenRef (an
  // infinite recursion).
  l = node->getChildren();
  ar_refNodeList(l);
  return l;
}

arDatabaseNode* arDatabase::newNode(arDatabaseNode* parent,
                                    const string& type,
                                    const string& name,
                                    bool refNode){
  if (!parent || !parent->active() || parent->getOwner() != this){
    ar_log_warning() << "arDatabaseNode::newNode got non-owned parent.\n";
    return NULL;
  }
  int parentID = parent->getID();
  arStructuredData* data = _dataParser->getStorage(_lang->AR_MAKE_NODE);
  data->dataIn(_lang->AR_MAKE_NODE_PARENT_ID, &parentID, AR_INT, 1);
  int temp = -1;
  // We are requesting an ID be assigned.
  data->dataIn(_lang->AR_MAKE_NODE_ID, &temp, AR_INT, 1);
  const string nodeName = name!="" ? name : _getDefaultName();
  data->dataInString(_lang->AR_MAKE_NODE_NAME, nodeName);
  data->dataInString(_lang->AR_MAKE_NODE_TYPE, type);
  // alter() returns the new node.
  // In subclasses, alter() is atomic. If refNode is
  // true, then the node pointer returned will have an extra reference added.
  arDatabaseNode* result = alter(data, refNode);
  _dataParser->recycle(data);
  return result;
}

arDatabaseNode* arDatabase::newNodeRef(arDatabaseNode* parent,
				       const string& type,
				       const string& name){
  return newNode(parent, type, name);
}

// If no child node is given (i.e. the 
// parameter is NULL) then the new node goes between the parent and
// its current children. If a child node is given, put the new node
// between the two specified nodes (assuming they are truly parent and 
// child). Return a pointer to the new node (or NULL on error).
// Fail if either node is not owned by this database.

arDatabaseNode* arDatabase::insertNode(arDatabaseNode* parent,
			               arDatabaseNode* child,
			               const string& type,
			               const string& name,
                                       bool refNode){

  // The parent node can't be NULL and must be owned by this database.
  // If the child node is not NULL, it must be owned by this database as well.
  if (!parent || !parent->active() || parent->getOwner() != this ||
      (child && (!child->active() || child->getOwner() != this))){
    ar_log_warning() << "arDatabaseNode: can't insert with non-owned nodes.\n";
    return NULL;
  }

  string nodeName = name;
  if (nodeName == ""){
    nodeName = _getDefaultName();
  }
  
  int parentID = parent->getID();
  // If child is NULL, then we want to use the "invalid" ID of -1.
  int childID = -1;
  if (child){
    childID = child->getID();
  }
  arStructuredData* data = _dataParser->getStorage(_lang->AR_INSERT);
  data->dataIn(_lang->AR_INSERT_PARENT_ID, &parentID, AR_INT, 1);
  data->dataIn(_lang->AR_INSERT_CHILD_ID, &childID, AR_INT, 1);
  int temp = -1;
  data->dataIn(_lang->AR_INSERT_ID, &temp, AR_INT, 1);
  data->dataInString(_lang->AR_INSERT_NAME, nodeName);
  data->dataInString(_lang->AR_INSERT_TYPE, type);
  // Subclasses may guarantee that alter(...) occurs atomically. Consequently,
  // this call is thread-safe. If refNode is true, then the node pointer will
  // be returned with an extra reference.
  arDatabaseNode* result = alter(data, refNode);
  _dataParser->recycle(data); // Avoid memory leak.
  return result;
}

arDatabaseNode* arDatabase::insertNodeRef(arDatabaseNode* parent,
			                  arDatabaseNode* child,
			                  const string& type,
			                  const string& name){
  return insertNode(parent, child, type, name, true);
}

bool arDatabase::cutNode(arDatabaseNode* node){
  if (!node || !node->active() || node->getOwner() != this){
    ar_log_warning() << "arDatabaseNode: can't cut non-owned node.\n";
    return false;
  }
  return cutNode(node->getID());
}

bool arDatabase::cutNode(int ID){
  // There is no need to check (here) whether or not there is a node in the
  // arDatabase with this ID. This will occur inside the alter(...).
  arStructuredData* data = _dataParser->getStorage(_lang->AR_CUT);
  data->dataIn(_lang->AR_CUT_ID, &ID, AR_INT, 1);
  arDatabaseNode* result = alter(data);
  _dataParser->recycle(data); // Avoid memory leak.
  return result ? true : false;
}

bool arDatabase::eraseNode(arDatabaseNode* node){
  if (!node || !node->active() || node->getOwner() != this){
    ar_log_warning() << "arDatabaseNode: can't erase non-owned node.\n";
    return false;
  }
  return eraseNode(node->getID());
}

bool arDatabase::eraseNode(int ID){
  // There is no need to check (here) whether or not there is a node in the
  // arDatabase with this ID. This will occur inside the alter(...).
  arStructuredData* data = _dataParser->getStorage(_lang->AR_ERASE);
  data->dataIn(_lang->AR_ERASE_ID, &ID, AR_INT, 1);
  alter(data);
  _dataParser->recycle(data); // Avoid memory leak.
  return true;
}

void arDatabase::permuteChildren(arDatabaseNode* parent,
		                 list<arDatabaseNode*>& children){
  int size = children.size();
  if (size == 0){
    // There's nothing to do. Just return.
    return;
  }
  arStructuredData* data = _dataParser->getStorage(_lang->AR_PERMUTE);
  int parentID = parent->getID();
  data->dataIn(_lang->AR_PERMUTE_PARENT_ID, &parentID, AR_INT, 1);
  int* IDs = new int[size];
  int count = 0;
  for (list<arDatabaseNode*>::iterator i = children.begin(); 
       i != children.end(); i++){
    if (*i && (*i)->active() && (*i)->getOwner() == this){
      IDs[count] = (*i)->getID();
      count++;
    }
  }
  data->dataIn(_lang->AR_PERMUTE_CHILD_IDS, IDs, AR_INT, count);
  delete [] IDs;
  if (count > 0){
    // Only do this if, in fact, some of the pointers make sense to use.
    alter(data);
  }
  _dataParser->recycle(data); // Avoid memory leak.
}

// An adapter for the Python wrapping. 
void arDatabase::permuteChildren(arDatabaseNode* parent,
				 int number,
				 int* children){
  list<arDatabaseNode*> l;
  for (int i=0; i<number; i++){
    // For thread-safety, must own a reference to the node.
    arDatabaseNode* n = getNodeRef(children[i]);
    if (n){
      l.push_back(n);
    }
  }
  permuteChildren(parent, l);
  // Must unref to prevent a memory leak.
  ar_unrefNodeList(l);
}

// When transfering the database state, we often want to dump the structure
// before dumping the data of the individual nodes.
// Thread-safe, assuming that the node is
// ref'ed outside the arDatabase (or is the root). This means the parent
// is also ref'ed since the node refs its parent.
bool arDatabase::fillNodeData(arStructuredData* data, arDatabaseNode* node){
  if (!data || !node){
    return false;
  }
  if (node->getID() <= 0){
    // This is the root node.  It does not get mirrored.
    return false;
  }
  const int nodeID = node->getID();
  // Guaranteed to have a parent since not the root node.
  const int parentID = node->getParent()->getID();
  return
    data->dataIn(_lang->AR_MAKE_NODE_PARENT_ID,&parentID,AR_INT,1) &&
    data->dataIn(_lang->AR_MAKE_NODE_ID,&nodeID,AR_INT,1) &&
    data->dataInString(_lang->AR_MAKE_NODE_NAME, node->getName()) &&
    data->dataInString(_lang->AR_MAKE_NODE_TYPE, node->getTypeString());
}

// For node creation, return an arDatabaseNode* (i.e. the created node).
// In other cases, return a pointer to the altered node.
// Thread-safe subclasses must execute this atomically (via _lock()).
arDatabaseNode* arDatabase::alter(arStructuredData* inData, bool refNode){
  const ARint dataID = inData->getID();
  arDatabaseNode* pNode = NULL;
  if (_databaseReceive[dataID]){
    // Call one of _handleQueuedData _eraseNode _makeDatabaseNode.
    // If it fails, it prints its own diagnostic.
    pNode = (this->*(_databaseReceive[dataID]))(inData); 
    if (!pNode) {
      ar_log_remark() << "arDatabase::alter() failed.\n";
    }

    // Only requests for new nodes (i.e. make node and insert) actually
    // make refNode true. Thus, e.g., the root node's ref count won't increase.
    return _ref(pNode, refNode);
  }

  // Use _getNodeNoLock instead of getNode, since
  // subclasses may call this within _lock()/_unlock().
  // pNode needs no extra ref(), unlike the make node
  // or insert cases, where new nodes can be created.

  const int id = inData->getDataInt(_routingField[dataID]);
  pNode = _getNodeNoLock(id);
  if (!pNode) {
    ar_log_warning() << "arDatabaseNode::alter() _getNodeNoLock() failed:\n";
    // Re-fail, but with an error message this time.
    (void)_getNodeNoLock(id, true);
    return NULL;
  }

  if (!pNode->receiveData(inData)){
    cerr << "arDatabase warning: receiveData() of child \""
         << pNode->_name << "\" failed.\n";
    return NULL;
  }

  // Return it so the caller can manipulate it,
  // e.g. update its timestamp if it's transient.
  return pNode;
}

arDatabaseNode* arDatabase::alterRaw(ARchar* theData){
  const ARint dataID = ar_rawDataGetID(theData);
  // parse() and alter() print their own warnings, in all cases of failure.
  _parsingData[dataID]->parse(theData);
  return alter(_parsingData[dataID]);
}

// Sure, we are losing info by returning bool. But sending
// in a buffer packed with various calls, pretty much guarantees that
// we can't care about specific outcomes.
bool arDatabase::handleDataQueue(ARchar* theData){
  ARint bufferSize = -1;
  ARint numberRecords = -1;
  ar_unpackData(theData,&bufferSize,AR_INT,1);
  ar_unpackData(theData+AR_INT_SIZE,&numberRecords,AR_INT,1);
  ARint position = 2*AR_INT_SIZE;
  for (int i=0; i<numberRecords; i++){
    const int theSize = ar_rawDataGetSize(theData+position);
    if (!alterRaw(theData+position)) {
      ar_log_warning() << "arDatabase::handleDataQueue failure in record "
           << i+1 << " of " << numberRecords << ".\n";
      // Keep processing the remaining records.
    }
    position += theSize;
    if (position > bufferSize){
      ar_log_warning() << "arDatabase::handleDataQueue buffer overflow.\n";
      return false;
    }
  }
  return true;
}

// Reads in the database in binary format.
bool arDatabase::readDatabase(const string& fileName, 
                              const string& path){
  FILE* sourceFile = sourceFile = ar_fileOpen(fileName,path,"rb");
  if (!sourceFile){
    ar_log_warning() << "arDatabase failed to read file '" << fileName << "'.\n";
    return false;
  }

  int bufferSize = 1000;
  ARchar* buffer = new ARchar[bufferSize];
  while (fread(buffer,AR_INT_SIZE,1,sourceFile) > 0){
    ARint recordSize = -1;
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

  arStructuredDataParser* parser =
    new arStructuredDataParser(_lang->getDictionary());
  arFileTextStream fileStream;
  fileStream.setSource(sourceFile);
  for (;;){
    arStructuredData* record = parser->parse(&fileStream);
    if (!record)
      break;
    alter(record);
    parser->recycle(record);
  }
  delete parser;
  fclose(sourceFile);
  return true;
}

// Attaching means to create a copy of the file in the database,
// with the file's root node mapped to parent. All other nodes in the
// file will be associated with new nodes in the database.
// DOH! CUT_AND_PASTE IS REARING ITS UGLY HEAD!
bool arDatabase::attach(arDatabaseNode* parent,
			const string& fileName,
			const string& path){
  FILE* source = ar_fileOpen(fileName, path, "rb");
  if (!source){
    cerr << "arDatabase warning: failed to read file \""
         << fileName << "\".\n";
    return false;
  }
  arStructuredDataParser* parser 
    = new arStructuredDataParser(_lang->getDictionary());
  map<int, int, less<int> > nodeMap;
  for (;;){
    arStructuredData* record = parser->parseBinary(source);
    if (!record)
      break;
    // NOTE: the AR_IGNORE_NODE parameter is ignored (no outFilter specified).
    const int success = filterIncoming(parent, record, nodeMap, NULL, 
				       NULL, AR_IGNORE_NODE, true);
    arDatabaseNode* altered = NULL;
    if (success){
      altered = alter(record);
      if (success > 0 && altered){
	// A node was created.
	nodeMap.insert(map<int, int, less<int> >::value_type
			(success, altered->getID()));
      }
    }
    parser->recycle(record);
    if (success && !altered){
      // There was an unrecoverable error. filterIncoming already
      // complained so do not do so here.
      break;
    }
  }
  delete parser;
  // best to close this way...
  fclose(source);
  return true;
}

// Attaching means to create a copy of the file in the database,
// with the file's root node mapped to parent. All other nodes in the
// file will be associated with new nodes in the database.
// DOH! CUT_AND_PASTE IS REARING ITS UGLY HEAD!
// todo: clean up like arDatabase::attach() above was cleaned up
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
      // NOTE: the AR_IGNORE_NODE parameter is ignored (no outFilter specified)
      int success = filterIncoming(parent, record, nodeMap, NULL, 
                                   NULL, AR_IGNORE_NODE, true);
      arDatabaseNode* altered = NULL;
      if (success){
        altered = alter(record);
        if (success > 0 && altered){
          // A node was created.
          nodeMap.insert(map<int, int, less<int> >::value_type
      	                  (success, altered->getID()));
        }
      }
      parser->recycle(record);
      if (success && !altered){
        // There was an unrecoverable error. filterIncoming already
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

// Mapping tries to merge a copy of the file into the database,
// with the file's root node mapped to parent. All other nodes in the
// file will be associated with new nodes in the database.
// DOH! CUT_AND_PASTE IS REARING ITS UGLY HEAD!
// \todo clean up like arDatabase::attach() above was cleaned up
bool arDatabase::merge(arDatabaseNode* parent,
		       const string& fileName,
		       const string& path){
  FILE* source = ar_fileOpen(fileName, path, "rb");
  if (!source){
    cerr << "arDatabase warning: failed to read file \""
         << fileName << "\".\n";
    return false;
  }
  arStructuredDataParser* parser 
    = new arStructuredDataParser(_lang->getDictionary());
  arStructuredData* record;
  bool done = false;
  map<int, int, less<int> > nodeMap;
  while (!done){
    record = parser->parseBinary(source);
    if (record){
      // NOTE: The AR_IGNORE_NODE parameter is ignored (no outFilter specified)
      int success = filterIncoming(parent, record, nodeMap, NULL, 
                                   NULL, AR_IGNORE_NODE, false);
      arDatabaseNode* altered = NULL;
      if (success){
	altered = alter(record);
        if (success > 0 && altered){
          // A node was created.
          nodeMap.insert(map<int, int, less<int> >::value_type
      	                  (success, altered->getID()));
        }
      }
      parser->recycle(record);
      if (success && !altered){
        // There was an unrecoverable error. filterIncoming already
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
  fclose(source);
  return true;
}

// Mapping means attempting to merge the file into the existing database
// in a reasonable way, starting with the root node of the file being
// mapped to parent. When a new node is defined in the file, the
// function looks at the database node to which its file-parent maps.
// It then searches below this database node to find a node with the
// same name and type. If such can be found, it creates an association 
// between that database node and the file node. If none such can be found,
// it creates a new node in the database and associates that with the file
// node.
bool arDatabase::mergeXML(arDatabaseNode* parent,
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
      // NOTE: the AR_IGNORE_NODE parameter is ignored (no outFilter specifed)
      int success = filterIncoming(parent, record, nodeMap, NULL, 
                                   NULL, AR_IGNORE_NODE, false);
      arDatabaseNode* altered = NULL;
      if (success){
        altered = alter(record);
        if (success > 0 && altered){
          // A node was created.
          nodeMap.insert(map<int, int, less<int> >::value_type
      	                  (success, altered->getID()));
        }
      }
      parser->recycle(record);
      if (success && !altered){
        // There was an unrecoverable error. filterIncoming already
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

// Writes the database to a file using a binary format.
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

// Writes the database to a file using an XML format.
bool arDatabase::writeDatabaseXML(const string& fileName,
                                  const string& path){
  return writeRootedXML(&_rootNode, fileName, path); 
}

// Writes a subtree of the database to a file, in binary format. DOES
// NOT INCLUDE THE ROOT NODE!
bool arDatabase::writeRooted(arDatabaseNode* parent,
                             const string& fileName,
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
  // To make this call thread-safe, we must ref and unref the children.
  list<arDatabaseNode*> l = parent->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = l.begin(); i != l.end(); i++){
    _writeDatabase(*i, nodeData, buffer, bufferSize, destFile); 
  }
  ar_unrefNodeList(l);
  delete [] buffer;
  fclose(destFile);
  return true;
}

// Writes a subtree of the database to a file, in XML format. DOES NOT
// INCLUDE THE ROOT NODE!
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
  // To make this call thread-safe, we must ref and unref the node list.
  list<arDatabaseNode*> l = parent->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = l.begin(); i != l.end(); i++){
    _writeDatabaseXML(*i, nodeData, destFile);
  }
  ar_unrefNodeList(l);
  fclose(destFile);
  return true;
}

// NOTE: Similar to the algorithm found in filterIncoming.
// NOTE: NOT THREAD-SAFE.
bool arDatabase::createNodeMap(int externalNodeID, 
                               arDatabase* externalDatabase,
                               map<int, int, less<int> >& nodeMap){
  bool failure = false;
  _createNodeMap(&_rootNode, externalNodeID, externalDatabase, nodeMap,
		 failure);
  return !failure;
}

// NOTE: this code is, essentially, also found in filterIncoming!
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

// Helper function for filtering incoming messages (needed for mapping
// one database to another as in attachXML and mapXML). The record is
// changed in place, as is the nodeMap. allNew should be set to true
// if every node should be a new one. And set to false if we will 
// attempt to associate nodes with existing ones (as in mapXML, instead of
// attachXML.
//
// NOTE: This function has been modified so that the "alter" no longer
// occurs inside it. It is the responsibility of the caller to put the
// record into the database. Sometimes (as in the case of a node creation
// record being mapped to an already existing node), the record should
// just be discarded. This is also true if the map fails somehow.
//
// NOTE: Sometimes, we'll want to do something with the node map value
// (it is possible, in the case of a mapping to an existing node, that
// the map will be determined in here). For instance, two peer database
// may map IDs of their nodes to one another. In this case, the database
// that is the mapping target should respond to the mapped database.
//
// The return value is an integer. 
//  * -1: the record was successfully mapped. If any augmentation to the
//    node map occured, it was internally to this function. No new node
//    needs to be created and the caller should use the record.
//  * 0: the record failed to be successfully mapped. It should be discarded.
//  * > 0: the record is "half-mapped". The result of the external *alter*
//         message to the database will finish the other "half" (it produces
//         the ID of the new node).
//
// If information about the alterations to the nodeMap parameter is desired,
// a pointer to a 4 int array should be passed in via mappedIDs. If no
// information is desired, then NULL should be passed in.

//
// In here, we have to consider the fact that the mapped source might
// NOT be starting from the root, but from some other node. Consequently,
// we pass in a database node to map this parent-less node to...
int arDatabase::filterIncoming(arDatabaseNode* mappingRoot,
                               arStructuredData* record, 
	                       map<int, int, less<int> >& nodeMap,
                               int* mappedIDs,
			       map<int, int, less<int> >* outFilter,
			       arNodeLevel outFilterLevel,
                               bool allNew){
  // Give mappedIDs the right default values, if they have been passed in.
  if (mappedIDs){
    mappedIDs[0] = -1;
    mappedIDs[1] = -1;
    mappedIDs[2] = -1;
    mappedIDs[3] = -1;
  }
  if (_databaseReceive[record->getID()]){
    // This is a message to the database.
    if (record->getID() == _lang->AR_MAKE_NODE){
      // Just pass down the chain. Without breaking filterIncoming up like 
      // this, it becomes illegibly long.
      return _filterIncomingMakeNode(mappingRoot, record, nodeMap, mappedIDs,
				     outFilter, outFilterLevel, allNew);
    }
    else if (record->getID() == _lang->AR_INSERT){
      return _filterIncomingInsert(mappingRoot, record, nodeMap, mappedIDs);
    }
    else if (record->getID() == _lang->AR_ERASE){
      return _filterIncomingErase(mappingRoot, record, nodeMap);
    }
    else if (record->getID() == _lang->AR_CUT){
      return _filterIncomingCut(record, nodeMap);
    }
    else if (record->getID() == _lang->AR_PERMUTE){
      return _filterIncomingPermute(record, nodeMap);
    }
    else{
      // This is an error. We've received a funny message ID.
      cout << "arDatabase error: illegal message ID in filterIncoming.\n";
      // The message should be discarded.
      return 0;
    }
  }
  else{
    // For all other messages, we simply re-map the ID.
    int nodeID = record->getDataInt(_routingField[record->getID()]);
    map<int, int, less<int> >::iterator i = nodeMap.find(nodeID);
    if (i == nodeMap.end()){
      // DO NOT PRINT ANYTHING HERE! THIS CAN RESULT IN HUGE AMOUNTS OF
      // PRINTING UPON USER ERROR!
      // Returning false means "discard".
      return 0;
    }
    nodeID = i->second;
    // NOTE: do not use dataIn here. That sets the field data dimension,
    // which we should NOT do since a "history" can get stored in the
    // messages routing field (i.e. which peers have been visited in the
    // case of arGraphicsPeer).
    int* IDptr = (int*)record->getDataPtr(_routingField[record->getID()],
					  AR_INT);
    // This field is guaranteed to have at least dimension 1.
    IDptr[0] = nodeID;
    // Returning true means "use". The -1 means that there is an
    // incomplete mapping.
    if (nodeID){
      return -1;
    }
    else{
      // If we've mapped to the root node, this is definitely a problem.
      // Discard the message. (otherwise there will be a segfault)
      return 0;
    }
  }
}

bool arDatabase::empty(){
  arGuard dummy(_l);
  return _rootNode.hasChildren();
}

void arDatabase::reset(){
  // This causes all the nodes to be unreferenced (i.e. we start erasing at the
  // root node). All nodes without external references are deleted. The root
  // node does not get touched, though. It is special after all.
  eraseNode(0);
  // Hmmmmm. Reset should NEVER change _nextAssignedID (which is meant to be
  // unique across the life of the arDatabase. This is in contrast with the
  // way it was originally.
}

void arDatabase::printStructure(int maxLevel){
  printStructure(maxLevel, cout);
}

// Prints the structure of the database tree, including node IDs, node names,
// and node types. The information needed to reflect on the database.
// @param maxLevel (optional) how far down the tree hierarchy we will go
// (default is 10,000)
void arDatabase::printStructure(int maxLevel, ostream& s){
  s << "Database structure: \n";
  _rootNode._printStructureOneLine(0, maxLevel,s);
}

// The node creation/deletion functions require arStructuredData storage.
// However, this cannot be created until the language is set (like graphics 
// language or sound language) by one of the subclasses. Consequently, this 
// function needs to exist to be called from arGraphicsDatabase, etc. 
// constructors.
bool arDatabase::_initDatabaseLanguage(){
  // Allocate storage for external parsing.
  arTemplateDictionary* d = (/*hack*/ arTemplateDictionary*) _lang->getDictionary();
  eraseData = new arStructuredData(d, "erase");
  makeNodeData = new arStructuredData(d, "make node");
  if (!eraseData || !makeNodeData || !*eraseData || !*makeNodeData) {
    if (eraseData)
      delete eraseData;
    if (makeNodeData)
      delete makeNodeData;
    return false;
  }
  
  // Create the parsing helpers. NOTE: THIS WILL GO AWAY ONCE THE
  // arStructuredDataParser is integrated!
  for (arTemplateType::const_iterator iter = d->begin();
       iter != d->end(); ++iter){
    const int ID = iter->second->getID();
    _parsingData[ID] = new arStructuredData(iter->second);
    _routingField[ID] = iter->second->getAttributeID("ID");
  }

  // Register the callbacks.
  arDataTemplate* t = _lang->find("erase");
  _databaseReceive[t->getID()] = &arDatabase::_eraseNode;
  t = _lang->find("make node");
  _databaseReceive[t->getID()] = &arDatabase::_makeDatabaseNode;
  t = _lang->find("insert_node");
  _databaseReceive[t->getID()] = &arDatabase::_insertDatabaseNode;
  t = _lang->find("cut");
  _databaseReceive[t->getID()] = &arDatabase::_cutDatabaseNode;
  t = _lang->find("permute");
  _databaseReceive[t->getID()] = &arDatabase::_permuteDatabaseNodes;

  // Create the arStructuredDataParser for our language.
  _dataParser = new arStructuredDataParser(d);
  return true;
}

// Convert an ID into a node pointer while already _lock()'d.
arDatabaseNode* arDatabase::_getNodeNoLock(int ID, bool fWarn){
  const arNodeIDIterator i(_nodeIDContainer.find(ID));
  if (i == _nodeIDContainer.end()) {
    if (fWarn){
      ar_log_warning() << "arDatabase: no node with ID " << ID <<
	   (empty() ? " in empty database.\n" : ".\n");
      ar_log_debug() << "arDatabase: nodes are (ID, name, info):\n";
      for (arNodeIDIterator j(_nodeIDContainer.begin());
        j != _nodeIDContainer.end(); ++j) {
        ar_log_debug() << "\t" << j->first << ", " << j->second->getName() << ", " <<
	  j->second->getInfo() << "\n";
      }
    }
    return NULL;
  }
  return i->second;
}

string arDatabase::_getDefaultName(){
  stringstream s;
  arGuard dummy(_l);
  s << "szg_default_" << _nextAssignedID;
  return s.str();
}

// When the database receives a message demanding creation of a node,
// it is handled here. If we cannot succeed for some reason, return
// NULL, otherwise, return a pointer to the node in question.
// PLEASE NOTE: This function does double duty: both as a creator of new
// database nodes AND as a "mapper" of existing database nodes
// (consequently, we allow an existing node to get returned). Also,
// when we "attach" a node (really a whole subtree), the mapping feature 
// gets used.
arDatabaseNode* arDatabase::_makeDatabaseNode(arStructuredData* inData){
  // NOTE: inData is guaranteed to have the right type because alter(...)
  // distributes messages to handlers based on type.
  const int parentID = inData->getDataInt(_lang->AR_MAKE_NODE_PARENT_ID);
  int theID = inData->getDataInt(_lang->AR_MAKE_NODE_ID);
  const string name(inData->getDataString(_lang->AR_MAKE_NODE_NAME));
  const string type(inData->getDataString(_lang->AR_MAKE_NODE_TYPE));
  
  // If no parent node exists, then there is nothing to do.
  arDatabaseNode* parentNode = _getNodeNoLock(parentID);
  if (!parentNode){
    cout << "arDatabase warning: no parent (ID=" << parentID << ").\n";
    return NULL;
  }
  arDatabaseNode* result = NULL;
  // If a node exists with this ID, and has the correct type, use it.
  // ID -1 means "make a new node", since all nodes have ID >= 0.
  if (theID != -1){
    // Insert a node with a specific ID.
    // If there is already a node by this ID, just use it.
    arDatabaseNode* pNode = _getNodeNoLock(theID);
    if (pNode){
      // Determine if the existing node is suitable for "mapping".
      if (pNode->getTypeString() == type){
        // Such a node already exists and has the right type. Use it.
        // Do not print a warning since this is normal during "mapping".
        result = pNode;
      }
      else{
	// A node already exists with that ID but the wrong type.
        cout << "arDatabase error: tried to map node type " 
	     << type << " to node with ID=" 
	     << theID << " and name=" << pNode->getName() << ".\n";
	return NULL;
      }
    }
  }
  // If no suitable node has been found, make one.
  if (!result){
    result =_createChildNode(parentNode, type, theID, name, false);
  }
  // If we succeeded...
  if (result){
    // Modify the record in place with the new ID.
    // The arGraphicsServer and arGraphicsClient combo depend on there
    // being no mapping. So the client needs to always receive
    // *commands* about how to structure the IDs.
    theID = result->getID();
    inData->dataIn(_lang->AR_MAKE_NODE_ID, &theID, AR_INT, 1);
  }
  // _createChildNode already complained if a complaint was necessary.
  return result;
}

// Inserts a new node between existing nodes (parent and child).
arDatabaseNode* arDatabase::_insertDatabaseNode(arStructuredData* data){

  // NOTE: data is guaranteed to be the right type because alter(...)
  // sends messages to handlers (like this one) based on type information.
  int parentID = data->getDataInt(_lang->AR_INSERT_PARENT_ID);
  int childID = data->getDataInt(_lang->AR_INSERT_CHILD_ID);
  int nodeID = data->getDataInt(_lang->AR_INSERT_ID);
  string nodeName = data->getDataString(_lang->AR_INSERT_NAME);
  string nodeType = data->getDataString(_lang->AR_INSERT_TYPE);

  // Check that both parent and child nodes (between which we will insert)
  // exist. If not, return an error.
  // DO NOT print warnings if the node is not found (the meaning of the
  // second "false" parameter).
  arDatabaseNode* parentNode = _getNodeNoLock(parentID);
  arDatabaseNode* childNode = _getNodeNoLock(childID);

  // Only the parentNode is assumed to exist. If childID is -1, then we
  // do not deal with the child node.
  if (!parentNode || (!childNode && childID != -1)){
    cout << "arDatabase error: either parent or child does not exist.\n";
    return NULL;
  }

  // If the child is specified, check that the child is, indeed, a child 
  // of the parent.
  if (childNode && (childNode->getParent() != parentNode)){
    cout << "arDatabase error: insert database node failed. Not child of "
	 << "parent.\n";
    return NULL;
  }

  // If the node ID is specified (i.e. not -1), check that there is no node 
  // with that ID. Insert (unlike add) does not support node "mappings". If
  // a node exists with the ID, return an error.
  if (nodeID > -1){
    if (_getNodeNoLock(nodeID)){
      return NULL;
    }
  }
  // Add the node. If no child node was specified, then we make all the
  // parentNode's children into children of the new node (this is the
  // action of the final parameter). Otherwise, it is just another child!
  arDatabaseNode* result =
    _createChildNode(parentNode, nodeType, nodeID, nodeName,
                     childNode ? false : true);
  // We only need to change the tree structure if a child node was specified
  // and the node creation was a success.
  if (childNode && result){
    // Take the child node, break it off from its parent, and add it is a child
    // of the new node. Each of these calls will definitely succeed by
    // our construction, so no point in checking return values.
    parentNode->_removeChild(childNode);
    if (!result->_addChild(childNode)){
      cout << "arDatabase error: could not switch child's parent in insert.\n"
	   << "  This is a library error.\n";
    }
  }
  // NOTE: We need to fill in the ID so that it can be passed to connected
  // databases (as in the operation of subclasses arGraphicsServer or 
  // arGraphicsPeer).
  if (result){
    int theID = result->getID();
    data->dataIn(_lang->AR_INSERT_ID, &theID, AR_INT, 1);
  }
  return result;
}

// When we cut a node, we remove the node and make all its children
// become children of its parent. This stands in contrast to the erasing,
// whereby the whole subtree gets blown away.
arDatabaseNode* arDatabase::_cutDatabaseNode(arStructuredData* data){
  // NOTE: data is guaranteed to be the right type because alter(...)
  // sends messages to handlers (like this one) based on type information.
  int ID = data->getDataInt(_lang->AR_CUT_ID);
  arDatabaseNode* node = _getNodeNoLock(ID);
  if (!node){
    cout << "arDatabase remark: _cutNode failed. No such node.\n";
    return NULL;
  }
  _cutNode(node);
  return &_rootNode;
}

// When the database receives a message demanding erasure of a node,
// it is handled here. On failure, return NULL. On success, return a
// pointer to the root node. NOTE: this works will the old semantics 
// when this returned bool.
arDatabaseNode* arDatabase::_eraseNode(arStructuredData* inData){
  int ID = inData->getDataInt(_lang->AR_ERASE_ID);
  arDatabaseNode* startNode = _getNodeNoLock(ID);
  if (!startNode){
    cerr << "arDatabase::_eraseNode failed: no such node.\n";
    return NULL;
  }
  // Delete the startNode from the child list of its parent
  arDatabaseNode* parent = startNode->getParent();
  if (parent){
    parent->_removeChild(startNode);
  }
  _eraseNode(startNode);
  return &_rootNode;
}

arDatabaseNode* arDatabase::_permuteDatabaseNodes(arStructuredData* data){
  // This algorithm is pretty inefficient. We are assuming that the number
  // of permuted children is not "large".
  // NOTE: data is guaranteed to be the right type because alter(...)
  // sends messages to handlers (like this one) based on type information.
  int ID = data->getDataInt(_lang->AR_PERMUTE_PARENT_ID);
  arDatabaseNode* parent = _getNodeNoLock(ID);
  if (!parent){
    cout << "arDatabase:: _permuteDatabaseNodes failed: no such parent.\n";
    return NULL;
  }
  int* IDs = (int*) data->getDataPtr(_lang->AR_PERMUTE_CHILD_IDS, AR_INT);
  int numIDs = data->getDataDimension(_lang->AR_PERMUTE_CHILD_IDS);
  // Create the list of arDatabaseNodes.
  list<arDatabaseNode*> childList;
  for (int i=0; i<numIDs; i++){
    arDatabaseNode* node = _getNodeNoLock(IDs[i]);
    // Only the node if it is, in fact, a child of ours.
    if (node && node->getParent() == parent){
      childList.push_back(node);
    }
  }
  parent->_permuteChildren(childList);
  // Return the parent node, for the filters in arGraphicsPeer::alter.
  return &_rootNode;
}

// Make a new child for the given parent. If moveChildren is true,
// attach all the parent's current children to the new node.
arDatabaseNode* arDatabase::_createChildNode(arDatabaseNode* parentNode,
                                             const string& typeString,
                                             int nodeID,
                                             const string& nodeName,
					     bool moveChildren){
  // Create a new node.
  arDatabaseNode* node = _makeNode(typeString);
  if (!node){
    cout << "arDatabase error: failed to create node of type="
	 << typeString << ".\n";
    return NULL;
  }
  // Use protected _setName() not public setName(), which sends a message.
  node->_setName(nodeName);

  // Add to children of parent.
  if (moveChildren){
    // Attach parent's previous children to the new node.
    node->_stealChildren(parentNode);
  }
  // Either way, make the new node a child of the parent.
  parentNode->_addChild(node);

  if (nodeID == -1){
    // Assign an ID automatically.
    node->_setID(_nextAssignedID++);
    ar_log_debug() << "\t" << _typeString << " auto node " << node->dumpOneline();
  } else {
    node->_setID(nodeID);
    ar_log_debug() << "\t" << _typeString << " new node " << node->dumpOneline();
    if (_nextAssignedID <= nodeID){
      _nextAssignedID = nodeID+1;
    }
  }
  // Enter the database's registry.
  _nodeIDContainer.insert(
      map<int,arDatabaseNode*,less<int> >::value_type(node->getID(), node));
  // Do not forget to intialize.
  node->initialize(this);
  return node;
}

// Delete a node and its subtree.
void arDatabase::_eraseNode(arDatabaseNode* node){
  // Erase each child's node. Copying the node list is inefficient,
  // but hides the guts of the arDatabaseNode.

  // Don't use getChildrenRef, since this function is called while
  // already _lock()'d (when thread-safety is desired, as in
  // arGraphicsPeer and arGraphicsServer).
  list<arDatabaseNode*> childList = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = childList.begin();
       i != childList.end(); i++){
    // Recurse.
    _eraseNode(*i);
  }
  node->_removeAllChildren();
  if (node->isroot())
    return;

  // arLogStream's lock sometimes hangs on shutdown.
  // ar_log_debug() << "\t" << _typeString << " deleting node " << node->dumpOneline();

  // Unreference node, and remove it from the node ID container.
  _nodeIDContainer.erase(node->getID());
  node->deactivate();
  node->unref();
}

// Cuts a node from the database (the node's children become the children
// of the parent).
void arDatabase::_cutNode(arDatabaseNode* node){
  if (node->isroot())
    return;

  arDatabaseNode* parent = node->getParent();
  // Every node except for the root node will have a parent (since it must
  // be part of this database, given how we've been called). Still, check.
  if (!parent){
    cout << "arDatabase internal error: failed in _cutNode.\n";
    return;
  }
  parent->_removeChild(node);
  // Attach the children to their new parent.
  parent->_stealChildren(node);

  ar_log_debug() << "\t" << _typeString << " cutting node " << node->dumpOneline();
  _nodeIDContainer.erase(node->getID());
  node->deactivate();
  node->unref();
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
  // Now, recurse to the node's children. To make this call thread-safe, we
  // must ref and unref the node list.
  list<arDatabaseNode*> children = pNode->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _writeDatabase(*i, nodeData, buffer, bufferSize, destFile);
  }
  ar_unrefNodeList(children);
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
  list<arDatabaseNode*> children = pNode->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _writeDatabaseXML(*i, nodeData, destFile);
  }  
  ar_unrefNodeList(children);
}

// Recursive helper function for createNodeMap.
// NOTE: THIS FUNCTION IS NOT THREAD-SAFE.
void arDatabase::_createNodeMap(arDatabaseNode* localNode, 
                                int externalNodeID, 
                                arDatabase* externalDatabase, 
				map<int, int, less<int> >& nodeMap,
		                bool& failure){
  // Since this function is not called from the inner loop of message
  // processing (like filterIncoming, for instance), we do not risk deadlock
  // by using getNode instead of _getNodeNoLock
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
    // This helper function is used in common with filterIncoming.
    match = _mapNodeBelow(node, localNode->getTypeString(),
			  localNode->getName(), nodeMap);
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

void arDatabase::_insertOutFilter(map<int,int,less<int> >& outFilter,
				  int nodeID,
				  arNodeLevel outFilterLevel){
  map<int,int,less<int> >::iterator i = outFilter.find(nodeID);
  if (i == outFilter.end()){
    outFilter.insert(map<int,int,less<int> >::value_type(nodeID,
							 outFilterLevel));
  }
  else{
    i->second = outFilterLevel;
  }
}

arDatabaseNode* arDatabase::_mapNodeBelow(arDatabaseNode* parent,
					  const string& nodeType,
                                          const string& nodeName,
					  map<int,int,less<int> >& nodeMap){
  arDatabaseNode* target = NULL;
  bool success = false;
  // NOTE: we map things differently based on whether or not this is
  // a "name" node (in which case just map to any other name node)
  // or, otherwise, we must map to a node with the same name and type.

  // Map to nodes that have as yet been unmapped and are strict descendants
  // of the current node (i.e. do not map a child to the parent).
  // Thus, the node map will always be an *injection*.
  if (nodeType == "name"){
    parent->_findNodeByType(target, nodeType, success, &nodeMap, false);
  }
  else{
    parent->_findNode(target, nodeName, success, &nodeMap, false);
  }
  if (success){
    return target;
  }
  else{
    return NULL;
  }
}

// Alter a "make node" message, according to our mapping algorithm.
// 0:  Node mapped to an existing node (or there was some error, like a
//     stale entry in the node map). Discard the message.
// >0: Node mapping partially succeeded (we need to create a new node
//     locally). We'll need to complete the mapping in the caller upon
//     node creation.
int arDatabase::_filterIncomingMakeNode(arDatabaseNode* mappingRoot,
                                arStructuredData* record, 
	                        map<int, int, less<int> >& nodeMap,
                                int* mappedIDs,
				map<int, int, less<int> >* outFilter,
				arNodeLevel outFilterLevel,
                                bool allNew){
  int newNodeID, newParentID;
  map<int, int, less<int> >::iterator i;
  // NOTE: because we get here only through filterIncoming, this 
  // arStructuredData is guaranteed to have type AR_MAKE_NODE.
  int parentID = record->getDataInt(_lang->AR_MAKE_NODE_PARENT_ID);
  string nodeName = record->getDataString(_lang->AR_MAKE_NODE_NAME);
  string nodeType = record->getDataString(_lang->AR_MAKE_NODE_TYPE);
  int originalNodeID = record->getDataInt(_lang->AR_MAKE_NODE_ID);
  // Don't map the parent. There are no guarantees
  // that it will have a consistent node type, etc.
  i = nodeMap.find(parentID);
  arDatabaseNode* currentParent = (i == nodeMap.end()) ?
    mappingRoot : _getNodeNoLock(i->second);
  // Make sure that the parent actually exists (maybe there was a stale entry
  // in the node map). Or maybe mappingRoot was NULL as passed in from the
  // caller.
  if (!currentParent){
    return 0;
  }
  // Alter the message in place.
  newParentID = currentParent->getID();
  // Do not use dataIn, which would resize the field and mangle the
  // routing information through the network of arGraphicsPeers which is
  // stored in here.
  int* IDptr = (int*)record->getDataPtr(_lang->AR_MAKE_NODE_PARENT_ID, AR_INT);
  IDptr[0] = newParentID;
  // This helper function is used in common with _createNodeMap.
  arDatabaseNode* target = _mapNodeBelow(currentParent, nodeType, nodeName,
                                         nodeMap);
  if (!allNew && target && target->getTypeString() == nodeType){
    // Map to this node.
    nodeMap.insert(map<int, int, less<int> >::value_type
	           (originalNodeID, target->getID()));
    // Need to record any node mappings we made.
    if (mappedIDs){
      mappedIDs[0] = originalNodeID;
      mappedIDs[1] = target->getID();
    }
    // If an "outFilter" has been supplied, update its mapping.
    if (outFilter){
      _insertOutFilter(*outFilter, target->getID(), outFilterLevel);
    }     

    // In this case, DO NOT create a new node! (instead reuse an old node)
    // The record can simply be discarded by the caller. 
    // However, it is a good idea to do the message mapping *anyway* in case
    // the application code fails to discard.
    newNodeID = target->getID();
    // NOTE: do not use dataIn. This will resize the field, which is 
    // undesirable since it will wipe out routing info as stored by the
    // arGraphicsPeer.
    IDptr = (int*)record->getDataPtr(_lang->AR_MAKE_NODE_ID, AR_INT);
    IDptr[0] = newNodeID;
    // In this case, no new node was created (the node map was simply 
    // augmented). The message should be discarded.
    return 0;
  }
  else{
    // We could not find a suitable node for mapping OR we all inserting all
    // new nodes (if allNew is true).
    newNodeID = -1;
    // Setting the parameter like so (-1) indicates that we will be
    // requesting a new node. Do not use dataIn here because it can wipe out
    // routing information used by arGraphicsPeer.
    IDptr = (int*)record->getDataPtr(_lang->AR_MAKE_NODE_ID, AR_INT);
    IDptr[0] = newNodeID;
    if (mappedIDs){
      mappedIDs[0] = originalNodeID;
    }
    // Returning true means "do not discard".
    return originalNodeID;
  }
}

// The insert is mapped exactly when both the parent and the child are already
// mapped. Also, if the mapped nodes are not parent/child, then nothing 
// happend. In contrast to the "make node", the insert never tries to use
// an existing node. ("make node" does so because it does double duty as
// a mapper). Return values:
//  0: Discard message.
//  > 0: Keep message and augment node map (in caller).
int arDatabase::_filterIncomingInsert(arDatabaseNode* mappingRoot,
                                      arStructuredData* data,
				      map<int,int,less<int> >& nodeMap,
                                      int* mappedIDs){
  // If we get here, filterIncoming called us (so we know we're of the proper
  // type).
  // NOTE: child ID can legitimately be -1. This occurs when we are inserting
  // a new node between the specified parent and all of its current children.
  int childID = data->getDataInt(_lang->AR_INSERT_CHILD_ID);
  int parentID = data->getDataInt(_lang->AR_INSERT_PARENT_ID);
  map<int,int,less<int> >::iterator i = nodeMap.find(parentID);
  // See _filterIncomingMakeNode to understand that the mapping root node is
  // not actually mapped (because it is allowed to be of a different type
  // than its counterpart on the other side). Consequently, we initially assume
  // that unmapped nodes correspond to the mapping root. This allows us to
  // insert between the mapping root and one of its children. 
  // (the code checks later to make sure that childNode is actually a child
  // of parentNode)
  arDatabaseNode* parentNode = (i == nodeMap.end()) ?
    mappingRoot : _getNodeNoLock(i->second);
  // Check that the node still exists. (It may have been
  // erased on our side but not on the other side).
  if (!parentNode){
    return 0;
  }
  // childID can legitimately equal -1, when there is no corresponding child node.
  arDatabaseNode* childNode = NULL;
  if (childID != -1){
    i = nodeMap.find(childID);
    childNode = (i == nodeMap.end()) ? mappingRoot : _getNodeNoLock(i->second);
    if (!childNode){
      return 0;
    }
  }
  
  // Check to make sure that childNode is actually a child of parentNode.
  // Handle the case where childID is -1 (and thus childNode is NULL).
  
  if (childID != -1 && childNode->getParent() != parentNode)
    return 0;

  const int newChildID = childID == -1 ? -1 : childNode->getID();
  const int newParentID = parentNode->getID();
  // Do not use dataIn in the remapping to avoid resizing fields (we encoded
  // peer routing information in "extra" spaces in arGraphicsPeer)
  *(int*)data->getDataPtr(_lang->AR_INSERT_CHILD_ID, AR_INT) = newChildID;
  *(int*)data->getDataPtr(_lang->AR_INSERT_PARENT_ID, AR_INT) = newParentID;
  // Use the mapped message.
  // The node ID will NOT be -1 on any code pathway that calls
  // filterIncoming; that's true only for locally produced messages.
  const int originalNodeID = data->getDataInt(_lang->AR_INSERT_ID);
  int newNodeID = -1;
  // Do not use dataIn, because it resizes the field to 1.
  *(int*)data->getDataPtr(_lang->AR_INSERT_ID, AR_INT) = newNodeID;
  if (mappedIDs)
    *mappedIDs = originalNodeID;
  return originalNodeID;
}

// Return values:
// 0: Discard message
// -1: Use (mapped) message.
int arDatabase::_filterIncomingErase(arDatabaseNode* mappingRoot,
                                     arStructuredData* data,
				     map<int,int,less<int> >& nodeMap){
  // FilterIncoming called us (so we know we're of the proper type).
  const int nodeID = data->getDataInt(_lang->AR_ERASE_ID);
  map<int,int,less<int> >::iterator i = nodeMap.find(nodeID);
  // Note the one exception here: if the erase message is directed at the
  // (remote) root node, then it should be directed at the local mapping root.
  // (in the just mentioned case, nodeID == 0)
  if (i == nodeMap.end() && nodeID){
    // The node is unmapped. Discard this message.
    return 0;
  }

  // Check to see that the entry in the node map is not "stale".
  arDatabaseNode* node = nodeID ?  _getNodeNoLock(i->second) : mappingRoot;
  if (!node){
    // Node map entry is "stale". Discard this message.
    return 0;
  }

  int newNodeID = node->getID();
  // NOTE: do not use dataIn here. It would set the field dimension to 1 and
  // wipe out any routing info that an arGraphicsPeer might have stored.
  int* IDptr = (int*)data->getDataPtr(_lang->AR_ERASE_ID, AR_INT);
  IDptr[0] = newNodeID;
  // Use the message.
  return -1;
}

// Return values:
// 0: Discard message
// -1: Use (mapped) message.
int arDatabase::_filterIncomingCut(arStructuredData* data,
				   map<int,int,less<int> >& nodeMap){
  // If we get here, filterIncoming called us (so we know we're of the proper
  // type).
  int nodeID = data->getDataInt(_lang->AR_CUT_ID);
  map<int,int,less<int> >::iterator i = nodeMap.find(nodeID);
  if (i == nodeMap.end()){
    // The node is unmapped. Discard this message.
    return 0;
  }
  // Check to see that the entry in the node map is not "stale".
  arDatabaseNode* node = _getNodeNoLock(i->second);
  if (!node){
    // Node map entry is "stale". Discard this message.
    return 0;
  }
  int newNodeID = node->getID();
  // NOTE: do not use dataIn here. It would set the field dimension to 1 and
  // wipe out any routing info that an arGraphicsPeer might have stored.
  int* IDptr = (int*)data->getDataPtr(_lang->AR_CUT_ID, AR_INT);
  IDptr[0] = newNodeID;
  // Use this message.
  return -1;
}

// Return values:
// 0: Discard message
// -1: Use (mapped) message
int arDatabase::_filterIncomingPermute(arStructuredData* data,
				       map<int,int,less<int> >& nodeMap){
  // If we get here, filterIncoming called us (so we know we're of the
  // proper type).
  int parentID = data->getDataInt(_lang->AR_PERMUTE_PARENT_ID);
  map<int,int,less<int> >::iterator i = nodeMap.find(parentID);
  if ( i == nodeMap.end() ){
    // The parent node of the permute is unmapped. Discard this message.
    return 0;
  }
  // Check to see that the entry in the node map is not "stale".
  arDatabaseNode* parentNode = _getNodeNoLock(i->second);
  if (!parentNode){
    // Node map entry is "stale". Discard this message.
    return 0;
  }
  int newParentID = parentNode->getID();
  // NOTE: do not use dataIn here. It would set the field dimension to 1 and
  // wipe out any routing info that an arGraphicsPeer might have stored.
  int* IDptr = (int*)data->getDataPtr(_lang->AR_PERMUTE_PARENT_ID, AR_INT);
  IDptr[0] = newParentID;
  // Check the child IDs for mapping. We drop any IDs that are unmapped
  // and permute the rest.
  int numberToPermute = data->getDataDimension(_lang->AR_PERMUTE_CHILD_IDS);
  if (numberToPermute < 1){
    // Nothing to do. Discard this message.
    return 0;
  }
  int* mappedChildIDs = new int[numberToPermute];
  int* childIDs = (int*)data->getDataPtr(_lang->AR_PERMUTE_CHILD_IDS, AR_INT);
  int whichMappedID = 0;
  for (int j=0; j<numberToPermute; j++){
    // Check the specific child ID.
    i = nodeMap.find(childIDs[j]);
    if (i != nodeMap.end()){
      // Check to see that the entry in the node map is not stale.
      arDatabaseNode* childNode = _getNodeNoLock(i->second);
      if (childNode){
	// Yes, we can map now.
	mappedChildIDs[whichMappedID] = childNode->getID();
	whichMappedID++;
      }
    }
  }
  // It is OK to use dataIn here since we will NOT route based on "extra"
  // information encoded in this field.
  data->dataIn(_lang->AR_PERMUTE_CHILD_IDS, mappedChildIDs, AR_INT, 
               whichMappedID);
  delete [] mappedChildIDs;
  return -1;
}

// Construct the nodes that are unqiue to the arDatabase
// (as opposed to subclasses).
arDatabaseNode* arDatabase::_makeNode(const string& type){
  // Subclassed versions will print warnings, not this one.
  return (type == "name") ? new arNameNode() : NULL;
}
