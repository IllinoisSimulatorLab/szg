//********************************************************
// Syzygy is licensed under the BSD license v2
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
  // Important that the node level defaults to this. See arGraphicsPeer::alter
  // for details.
  _nodeLevel(AR_STRUCTURE_NODE),
  _info(""){
  
  _databaseOwner = NULL;
  _dLang = NULL;

  ar_mutex_init(&_nodeLock);
}

arDatabaseNode::~arDatabaseNode(){
  // If we are being deleted AND the conventions for using arDatabaseNodes
  // have been followed, our parent no longer holds us in its child list,
  // so the following call is safe.
  _removeParentLeavingInParentsChildren();
  _removeAllChildren();
}

/// The node is owned by a database and has a parent (consequently it has not
/// been deleted from the node container) OR it is owned by a database and
/// is the root node. This call is thread-safe (indeed, it is how we determine
/// whether or not to route our function calls through the arDatabase message
/// stream OR through the local memory).
bool arDatabaseNode::active(){
  // The root node is the unique node with ID = 0
  if (getOwner() && (getParent() || getID() == 0)){
    return true;
  }
  return false;
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

int arDatabaseNode::getRef(){
  ar_mutex_lock(&_nodeLock);
  int r = _refs;
  ar_mutex_unlock(&_nodeLock);
  return r;
}

void arDatabaseNode::lock(){
  ar_mutex_lock(&_nodeLock);
}

void arDatabaseNode::unlock(){
  ar_mutex_unlock(&_nodeLock);
}

/// If this node has an owning database, go ahead and use that as a node
/// factory. Otherwise, return NULL.
arDatabaseNode* arDatabaseNode::newNode(const string& type,
					const string& name,
                                        bool refNode){
  if (!active()){
    return NULL;
  }
  return getOwner()->newNode(this, type, name, refNode);
}

/// A wrapper for newNode that returns a ref'ed node pointer always.
arDatabaseNode* arDatabaseNode::newNodeRef(const string& type,
					   const string& name){
  return newNode(type, name, true);
}


/// Take an existing node and make it a child of this node. In this case,
/// neither node can be owned by an arDatabase. Note that we do not bother to
/// try detecting loops here. The user is on his or her own in that regard.
bool arDatabaseNode::addChild(arDatabaseNode* child){
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // Not checking to see if this new child node is of the right *type*
  // (i.e. arGraphicsNode instead of arSoundNode).
  if (!getOwner() && !child->getOwner()){
    return _addChild(child);
  }
  else{
    return false;
  }
}

/// Take an existing node and remove it from the child list of this node.
/// Returns false if the operation cannot be completed (i.e. one of the nodes
/// is owned by an arDatabase).
bool arDatabaseNode::removeChild(arDatabaseNode* child){
  if (!getOwner() && !child->getOwner()){
    _removeChild(child);
    return true;
  }
  else{
    return false;
  }
}

string arDatabaseNode::getName() const{
  ar_mutex_lock(&_nodeLock);
  string result = _name;
  ar_mutex_unlock(&_nodeLock);
  return result;
}

void arDatabaseNode::setName(const string& name){
  if (active()){
    arStructuredData* r 
      = getOwner()->getDataParser()->getStorage(_dLang->AR_NAME);
    int ID = getID();
    r->dataIn(_dLang->AR_NAME_ID, &ID, AR_INT, 1);
    r->dataInString(_dLang->AR_NAME_NAME, name);
    ar_mutex_lock(&_nodeLock);
    r->dataInString(_dLang->AR_NAME_INFO, _info);
    ar_mutex_unlock(&_nodeLock);
    getOwner()->alter(r);
    // Must recycle or there will be a memory leak.
    getOwner()->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _setName(name);
    ar_mutex_unlock(&_nodeLock);
  }
}

string arDatabaseNode::getInfo() const{
  ar_mutex_lock(&_nodeLock);
  string result = _info;
  ar_mutex_unlock(&_nodeLock);
  return result;
  
}

void arDatabaseNode::setInfo(const string& info){
  if (_name == "root"){
    cout << "arDatabaseNode warning: cannot set root info.\n";
    return;
  }
  if (active()){
    arStructuredData* r 
      = getOwner()->getDataParser()->getStorage(_dLang->AR_NAME);
    int ID = getID();
    r->dataIn(_dLang->AR_NAME_ID, &ID, AR_INT, 1);
    ar_mutex_lock(&_nodeLock);
    r->dataInString(_dLang->AR_NAME_NAME, _name);
    ar_mutex_unlock(&_nodeLock);
    r->dataInString(_dLang->AR_NAME_INFO, info);
    getOwner()->alter(r);
    // Must do this or there will be a memory leak.
    getOwner()->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _info = info;
    ar_mutex_unlock(&_nodeLock);
  }
}

/// We do not worry about thread-safety if the caller does not request that
/// the returned node ptr be ref'ed. If, on the other hand, the caller does
/// so request, thread safety gets handled by forwarding the request to
/// the owning database (if one exists) which then calls back to us (but from 
/// within a locked _databaseLock).
arDatabaseNode* arDatabaseNode::findNode(const string& name,
                                         bool refNode){
  if (refNode && getOwner()){
    // Will return a ptr with an extra ref.
    return getOwner()->findNode(this, name, true); 
  }
  arDatabaseNode* result = NULL;
  bool success = false;
  _findNode(result, name, success, NULL, true);
  if (result && refNode){
    result->ref();
  }
  return result;
}

arDatabaseNode* arDatabaseNode::findNodeRef(const string& name){
  return findNode(name, true);
}

/// We do not worry about thread-safety if the caller does not request that
/// the returned node ptr be ref'ed. If, on the other hand, the caller does
/// so request, thread safety gets handled by forwarding the request to
/// the owning database (if one exists) which then calls back to us (but from 
/// within a locked _databaseLock).
arDatabaseNode* arDatabaseNode::findNodeByType(const string& nodeType,
                                               bool refNode){
  if (refNode && getOwner()){
    // Will return a ptr with an extra ref.
    return getOwner()->findNodeByType(this, nodeType, true);
  }
  arDatabaseNode* result = NULL;
  bool success = false;
  _findNodeByType(result, nodeType, success, NULL, true);
  if (result && refNode){
    result->ref();
  }
  return result;
}

arDatabaseNode* arDatabaseNode::findNodeByTypeRef(const string& nodeType){
  return findNodeByType(nodeType, true);
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
  if (_name == "root"){
    cout << "arDatabaseNode warning: cannot set root name.\n";
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _name = data->getDataString(_dLang->AR_NAME_NAME);
    ar_mutex_unlock(&_nodeLock);
  }
  ar_mutex_lock(&_nodeLock);
  _info = data->getDataString(_dLang->AR_NAME_INFO);
  ar_mutex_unlock(&_nodeLock);
  return true;
}

arStructuredData* arDatabaseNode::dumpData(){
  arStructuredData* result = _dLang->makeDataRecord(_dLang->AR_NAME);
  _dumpGenericNode(result,_dLang->AR_NAME_ID);
  ar_mutex_lock(&_nodeLock);
  result->dataInString(_dLang->AR_NAME_NAME, _name);
  result->dataInString(_dLang->AR_NAME_INFO, _info);
  ar_mutex_unlock(&_nodeLock);
  return result;
}

void arDatabaseNode::initialize(arDatabase* d){
  // We keep track of who owns us and the language use to encode messages.
  _setOwner(d);
  _dLang = d->_lang;
}

/// So far only implemented for OWNED nodes.
void arDatabaseNode::permuteChildren(list<arDatabaseNode*>& children){
  if (active()){
    getOwner()->permuteChildren(this, children);
  }
}

/// So far only implemented for OWNED nodes.
void arDatabaseNode::permuteChildren(int number, int* children){
  if (active()){
    getOwner()->permuteChildren(this, number, children);
  }
}

//**********************************************************************
// These accessor functions (combined with the next block of functions)
// should be the only way(s) that external code touches _databaseOwner,
// _ID, _parent, and _children. Even though these are simple functions,
// do not put them in the header file. This gives us greater flexibility
// looking towards the future.
//**********************************************************************

int arDatabaseNode::getID() const { 
  return _ID; 
}

arDatabaseNode* arDatabaseNode::getParent() const{ 
  return _parent; 
}

/// A little bit unusual. To achieve thread-safety with respect to the
/// owning database, we must have the owning database (if such exists)
/// execute the function. If there is no owning database, just ref the
/// parent and return it. 
arDatabaseNode* arDatabaseNode::getParentRef(){
  if (getOwner()){
    return getOwner()->getParentRef(this);
  }
  if (_parent){
    _parent->ref();
  }
  return _parent;
}
  
list<arDatabaseNode*> arDatabaseNode::getChildren() const{ 
  return _children; 
}

/// A little bit unusual. To achieve thread-safety with respect to the
/// owning database, we must have the owning database (if such exists)
/// execute the function. If there is no owning database, just ref the
/// list and return it.
list<arDatabaseNode*> arDatabaseNode::getChildrenRef(){
  if (getOwner()){
    return getOwner()->getChildrenRef(this);
  }
  ar_refNodeList(_children);
  return _children;
}

bool arDatabaseNode::hasChildren() const{
  return _children.begin() == _children.end();
}

arNodeLevel arDatabaseNode::getNodeLevel(){ 
  return _nodeLevel;
}

void arDatabaseNode::setNodeLevel(arNodeLevel nodeLevel){ 
  _nodeLevel = nodeLevel;
}

//**********************************************************************
// The following functions are for tree structure manipulation. These
// are the ONLY functions in arDatabaseNode and arDatabase that should
// modify _databaseOwner, _ID, _parent, and _children.
// Note that none of these check _databaseOwner. It is assumed that
// the caller has done the appropriate checks and that all nodes in 
// question are owned by the same arDatabase (in which case these are
// being called from arDatabase methods) or are owned by none (in which
// case they are being called by arDatabaseNode methods.
//**********************************************************************

// What it says. Sets the node's name.
void arDatabaseNode::_setName(const string& name){
  _name = name;
}

// A node either belongs to an arDatabase or it doesn't. This will only be
// changed around the time of node creation. If the node is created by the
// user directly, then it will never be owned by an arDatabase. If it is
// created by an arDatabase, then this function will be called soon after
// creation (from inside arDatabaseNode::initialize()).
void arDatabaseNode::_setOwner(arDatabase* owner){
  _databaseOwner = owner;
}

// The node's ID we only be set away from the default (0) if it is part of
// an arDatabase. If so, it will be set once upon creation and never 
// subsequently changed.
void arDatabaseNode::_setID(int ID){
  _ID = ID;
}

// This function is given an awkward name to discourage its use (outside of
// this block of functions). It means just what it says. We add a ref to the
// parent and set our pointer BUT we do not add ourselves to the parent's
// children.
bool arDatabaseNode::_setParentNotAddingToParentsChildren
  (arDatabaseNode* parent){
  // If the node currently has a parent, it is illegal to try to give it
  // another.
  if (_parent){
    return false;
  }
  if (parent){
    // Each node holds a reference to its parent.
    _parent = parent;
    _parent->ref();
    return true;
  }
  // It is an error to pass in a NULL pointer.
  return false;
}

// This function is given an awkward name to discourage its use (outside of
// this block of helper functions and the arDatabaseNode destructor).
// It means just what it says. We get rid of the node's parent ref without
// being concerned that our pointer might still be in the parent's child list.
void arDatabaseNode::_removeParentLeavingInParentsChildren(){
  if (_parent){
    // Each node holds a reference to its parent.
    _parent->unref();
    _parent = NULL;
  }
}

bool arDatabaseNode::_addChild(arDatabaseNode* node){
  // Book-keeping for the tree structure.
  // If there is already a parent, then this fails.
  if (node->_parent){
    return false;
  }
  node->_setParentNotAddingToParentsChildren(this);
  _children.push_back(node);
  // We've added the child. Must add a reference to it as well.
  node->ref();
  return true;
}

void arDatabaseNode::_removeChild(arDatabaseNode* node){
  // Book-keeping for the tree structure.
  for (list<arDatabaseNode*>::iterator i = _children.begin();
       i != _children.end(); i++){
    if (*i == node){
      (*i)->unref();
      node->_removeParentLeavingInParentsChildren();
      _children.erase(i);
      // We are done.
      break;
    }
  }
}

void arDatabaseNode::_removeAllChildren(){
  // Book-keeping for the tree structure. Be careful to do the reference
  // releasing correctly.
  for (list<arDatabaseNode*>::iterator i = _children.begin();
       i != _children.end(); i++){
    // Must release the node's reference to the parent. 
    (*i)->_removeParentLeavingInParentsChildren();
    // Must release OUR reference to the node.
    (*i)->unref();
  }
  // Go ahead and empty the list.
  _children.clear();
}

// Steal the children from the given node... and add them to our child list.
void arDatabaseNode::_stealChildren(arDatabaseNode* node){
  for (list<arDatabaseNode*>::iterator i = node->_children.begin();
       i != node->_children.end(); i++){
    _children.push_back(*i);
    // Here we are essentially remove the child node's reference to its old
    // parent. BUT... the node's child list still contains *i. We'll remove
    // it at the final step.
    (*i)->_removeParentLeavingInParentsChildren();
    // Set the node's parent to the current node.
    (*i)->_parent = this;
    // We need to add a reference to the new parent (i.e. us)
    ref();
    // We do not, however, need to add a reference to the node, since
    // that the old parent had already incremented the ref count.
  }
  // The old node has no children now!
  node->_children.clear();
}

// This algorithm is inefficient. We're assuming that permute will only be
// called on small node lists.
// The given children (if they are children of the node) will be moved to
// the front of the list, in the order they are given.
void arDatabaseNode::_permuteChildren(list<arDatabaseNode*> childList){
  // Must reverse the input list first.
  childList.reverse();
  for (list<arDatabaseNode*>::iterator i = childList.begin();
       i != childList.end(); i++){
    // Given the current node. Attempt to find it in the list and move it
    // to the front. Note how we need to iterate over this list in reverse.
    for (list<arDatabaseNode*>::iterator j = _children.begin();
	 j != _children.end(); j++){
      if (*i == *j){
        // NOTE: We do not need to do any ref or unref since we're just
	// moving this node to the front of the list.
	arDatabaseNode* node = *j;
        _children.erase(j);
        _children.push_front(node);
	// We're done with the search.
	break;
      }
    }
  }
}

//***************************************************************************
// Various helper functions, mostly for recursions.
//***************************************************************************

void arDatabaseNode::_dumpGenericNode(arStructuredData* theData,int IDField){
  int ID = getID();
  (void)theData->dataIn(IDField,&ID,AR_INT,1);
}

/// Have not tried to make this call thread-safe (it uses getChildren instead
/// of getChildrenRef). However, if thread-safety is desired, it will always
/// be called (for instance) from within an arDatabase's _databaseLock.
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

/// See comments re: thread-safety above findNode.
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

  // Might not be that efficient to copy the list but this operation need
  // not be incredibly optimized. Note how we use ref-ing to ensure
  // thread-safety.
  list<arDatabaseNode*> childList = getChildrenRef();
 
  s << '(' << getID() << ", " 
    << "\"" << getName() << "\", " 
    << "\"" << getTypeString() << "\")\n";
  
  for (list<arDatabaseNode*>::iterator i(childList.begin()); 
       i != childList.end(); ++i){
    (*i)->_printStructureOneLine(level+1, maxLevel, s);
  }
  // Since we ref'ed the list of nodes, must unref.
  ar_unrefNodeList(childList);
}
