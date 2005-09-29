//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDatabaseNode.h"
#include "arDatabase.h"

// DO NOT CHANGE THE BELOW DEFAULTS! THE ROOT NODE IS ASSUMED TO BE 
// INITIALIZED WITH THESE! For instance, sometimes we detect that it
// is the root node by testing one of them (i.e. the ID or name).)
arDatabaseNode::arDatabaseNode():
  _ID(0),
  _name("root"),
  _typeCode(-1),
  _typeString("root"),
  _parent(NULL),
  _refs(1),
  _transient(false),
  _info(""){
  
  _databaseOwner = NULL;
  _dLang = NULL;

  ar_mutex_init(&_nodeLock);
  ar_mutex_init(&_dataLock);
}

arDatabaseNode::~arDatabaseNode(){
}

void arDatabaseNode::ref(){
  ar_mutex_lock(&_nodeLock);
  _refs++;
  ar_mutex_unlock(&_nodeLock);
}

void arDatabaseNode::unref(){
  ar_mutex_lock(&_nodeLock);
  _refs--;
  bool state = _refs == 0 ? true : false;
  ar_mutex_unlock(&_nodeLock);
  // If the reference count has gone down to zero, delete the object.
  if (state){
    delete this;
  }
}

// If this node has an owning database, go ahead and use that as a node
// factory. Otherwise, return NULL.
arDatabaseNode* arDatabaseNode::newNode(const string& type,
					const string& name){
  if (!_databaseOwner){
    return NULL;
  }
  return _databaseOwner->newNode(this, type, name);
}


// Take an existing child (and all of its children recursively) and make
// them children of this node. NOTE: If this node is already associated with
// an arDatabase, then we need to make it generate messages to modify attached
// databases (like in arGraphicsServer/arGraphicsClient, for instance).
void arDatabaseNode::attach(arDatabaseNode* child){
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // Not checking to see if this new child node is of the right *type*
  // (i.e. arGraphicsNode instead of arSoundNode).
  if (!_databaseOwner){
    _addChild(child);
  }
  else{
    // There is an owning database. Consequently, the owning database needs
    // to generate a message (which does things like update connected, mirrored
    // databases).
    _databaseOwner->attach(this, child);
  }
}

string arDatabaseNode::getName() const{
  return _name;
}

void arDatabaseNode::setName(const string& name){
  if (_name == "root"){
    cout << "arDatabaseNode warning: cannot set root name.\n";
    return;
  }
  if (_databaseOwner){
    arStructuredData* r = _dLang->makeDataRecord(_dLang->AR_NAME);
    r->dataIn(_dLang->AR_NAME_ID, &_ID, AR_INT, 1);
    r->dataInString(_dLang->AR_NAME_NAME, name);
    r->dataInString(_dLang->AR_NAME_INFO, _info);
    _databaseOwner->alter(r);
    delete r;
  }
  else{
    _name = name;
  }
}

string arDatabaseNode::getInfo() const{
  return _info;
}

void arDatabaseNode::setInfo(const string& info){
  if (_name == "root"){
    cout << "arDatabaseNode warning: cannot set root info.\n";
    return;
  }
  if (_databaseOwner){
    arStructuredData* r = _dLang->makeDataRecord(_dLang->AR_NAME);
    r->dataIn(_dLang->AR_NAME_ID, &_ID, AR_INT, 1);
    r->dataInString(_dLang->AR_NAME_NAME, _name);
    r->dataInString(_dLang->AR_NAME_INFO, info);
    _databaseOwner->alter(r);
    delete r;
  }
  else{
    _info = info;
  }
}

arDatabaseNode* arDatabaseNode::findNode(const string& name){
  arDatabaseNode* result = NULL;
  bool success = false;
  _findNode(result, name, success, NULL, true);
  return result;
}

arDatabaseNode* arDatabaseNode::findNodeByType(const string& nodeType){
  arDatabaseNode* result = NULL;
  bool success = false;
  _findNodeByType(result, nodeType, success, NULL, true);
  return result;
}

void arDatabaseNode::printStructure(int maxLevel, ostream& s){
  _printStructureOneLine(0, maxLevel, s);
}

bool arDatabaseNode::receiveData(arStructuredData* data){
  if (data->getID() != _dLang->AR_NAME){
    // DO NOT PRINT ANYTHING OUT HERE. THE ASSUMPTION IS THAT THIS IS
    // BEING CALLED FROM ELSEWHERE (i.e. FROM A SUBCLASS) WHERE ANY
    // NECESSARY MESSAGE WILL, IN FACT, BE PRINTED. 
    return false;
  } 
  _name = data->getDataString(_dLang->AR_NAME_NAME);
  _info = data->getDataString(_dLang->AR_NAME_INFO);
  return true;
}

arStructuredData* arDatabaseNode::dumpData(){
  arStructuredData* result = _dLang->makeDataRecord(_dLang->AR_NAME);
  _dumpGenericNode(result,_dLang->AR_NAME_ID);
  result->dataInString(_dLang->AR_NAME_NAME, _name);
  result->dataInString(_dLang->AR_NAME_INFO, _info);
  return result;
}

void arDatabaseNode::initialize(arDatabase* d){
  // We keep track of who owns us and the language use to encode messages.
  _databaseOwner = d;
  _dLang = d->_lang;
}

void arDatabaseNode::_dumpGenericNode(arStructuredData* theData,int IDField){
  (void)theData->dataIn(IDField,&_ID,AR_INT,1);
}

void arDatabaseNode::_addChild(arDatabaseNode* node){
  // This is only called when there is no "owning database".
  node->_databaseOwner = NULL;
  node->_dLang = NULL;
  // Book-keeping for the tree structure.
  node->_parent = this;
  _children.push_back(node);
}

void arDatabaseNode::_findNode(arDatabaseNode*& result,
                               const string& name,
                               bool& success,
                               map<int,int,less<int> >* nodeMap,
                               bool checkTop){
  // First, check self.
  if (checkTop && getName() == name){
    success = true;
    result = this;
    return;
  }
  list<arDatabaseNode*> children = getChildren();
  // we are doing a breadth-first search (maybe..), check the children first
  // then recurse
  list<arDatabaseNode*>::iterator i;
  // If a node map has been passed, make sure that we do NOT find an already
  // mapped node.
  for (i = children.begin(); i != children.end(); i++){
    if ( (*i)->getName() == name && 
         (!nodeMap || nodeMap->find((*i)->getID()) == nodeMap->end())){
      success = true;
      result = *i;
      return;
    }
  }
  // now, recurse...
  for (i = children.begin(); i != children.end(); i++){
    if (success){
      // we're already done.
      return;
    }
    (*i)->_findNode(result, name, success, nodeMap, true);
  }
}

void arDatabaseNode::_findNodeByType(arDatabaseNode*& result,
                                     const string& nodeType,
                                     bool& success,
                                     map<int,int,less<int> >* nodeMap,
                                     bool checkTop){
  // First, check self.
  if (checkTop && getTypeString() == nodeType){
    success = true;
    result = this;
    return;
  }
  list<arDatabaseNode*> children = getChildren();
  // We are doing a breadth-first search (maybe..), check the children first
  // then recurse
  list<arDatabaseNode*>::iterator i;
  // If a node map has been passed, make sure we do NOT find an already mapped
  // node.
  for (i = children.begin(); i != children.end(); i++){
    if ( (*i)->getTypeString() == nodeType &&
         (!nodeMap || nodeMap->find((*i)->getID()) == nodeMap->end())){
      success = true;
      result = *i;
      return;
    }
  }
  // now, recurse...
  for (i = children.begin(); i != children.end(); i++){
    if (success){
      // we're already done.
      return;
    }
    (*i)->_findNodeByType(result, nodeType, success, nodeMap, true);
  }
}

/// helps printStructure() recurse through database
/// @param currentNodeID ID of node to print out and recurse down through
/// @param level How far down the tree hierarchy we are
/// @param maxLevel how far down the tree hierarchy we will go
void arDatabaseNode::_printStructureOneLine(int level, int maxLevel, 
                                            ostream& s) {

  for (int l=0; l<level; l++){
    if (maxLevel > l){
      s << " ";
    }
    else{
      s << " *****\n";
      return;
    }
  }

  // Is it really efficient to COPY the list like this?
  list<arDatabaseNode*> childList = getChildren();
 
  s << '(' << getID() << ", " 
    << "\"" << getName() << "\", " 
    << "\"" << getTypeString() << "\")\n";
  
  for (list<arDatabaseNode*>::iterator i(childList.begin()); 
       i != childList.end(); ++i){
    (*i)->_printStructureOneLine(level+1, maxLevel, s);
  }
}
