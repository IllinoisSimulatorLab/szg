//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDatabaseNode.h"
#include "arDatabase.h"
#include "arLogStream.h"

// DO NOT CHANGE THE BELOW DEFAULTS! THE ROOT NODE IS ASSUMED TO BE 
// INITIALIZED WITH THESE! For instance, sometimes we detect that it
// is the root node by testing one of them (i.e. the ID or name).)
arDatabaseNode::arDatabaseNode():
  _info(""),
  _ID(0),
  _name("root"),
  _typeCode(-1),
  _typeString("root"),
  _parent(NULL),
  _refs(1),
  _nodeLevel(AR_STRUCTURE_NODE) // default; see arGraphicsPeer::alter.
{
  _databaseOwner = NULL;
  _dLang = NULL;
}

arDatabaseNode::~arDatabaseNode(){
  // If we are being deleted AND the conventions for using arDatabaseNodes
  // have been followed, our parent no longer holds us in its child list,
  // so the following call is safe.
  _removeParentLeavingInParentsChildren();
  _removeAllChildren();
}

// The node is owned by a database and has a parent (consequently it has not
// been deleted from the node container) OR it is owned by a database and
// is the root node. This call is thread-safe (indeed, it is how we determine
// whether or not to route our function calls through the arDatabase message
// stream OR through the local memory).
bool arDatabaseNode::active(){
  // The root node is the unique node with ID = 0
  return getOwner() && (getParent() || getID() == 0);
}

void arDatabaseNode::ref(){
  ++_refs;
}

void arDatabaseNode::unref(){
  if (--_refs == 0)
    delete this;
}

int arDatabaseNode::getRef(){
  return _refs;
}

// If this node has an owning database, use that as a node factory.
arDatabaseNode* arDatabaseNode::newNode(const string& type,
					const string& name,
                                        bool refNode){
  return active() ? getOwner()->newNode(this, type, name, refNode) : NULL;
}

// Wrapper for newNode.  Return a ref'ed node pointer always.
arDatabaseNode* arDatabaseNode::newNodeRef(const string& type,
					   const string& name){
  return newNode(type, name, true);
}


// Make an existing node a child of this node.
// Neither node can be owned by an arDatabase. 
// Detecting cycles is the caller's responsibility.
bool arDatabaseNode::addChild(arDatabaseNode* child){
  // Bug: should test that the new child has the right type,
  // e.g. arGraphicsNode doesn't match arSoundNode.
  if (!child || getOwner() || child->getOwner())
    return false;

  return _addChild(child);
}

// Remove an existing node from the child list of this node.
// Return false on failure, e.g. if one of the nodes
// is owned by an arDatabase.
bool arDatabaseNode::removeChild(arDatabaseNode* child){
  if (!child || getOwner() || child->getOwner())
    return false;

  return _removeChild(child);
}

string arDatabaseNode::getName() {
  _lockName.lock();
    const string result(_name);
  _lockName.unlock();
  return result;
}

void arDatabaseNode::setName(const string& name){
  if (active()){
    arStructuredData* r = getOwner()->getDataParser()->getStorage(_dLang->AR_NAME);
    int ID = getID();
    r->dataIn(_dLang->AR_NAME_ID, &ID, AR_INT, 1);
    r->dataInString(_dLang->AR_NAME_NAME, name);
    _lockInfo.lock();
      r->dataInString(_dLang->AR_NAME_INFO, _info);
    _lockInfo.unlock();
    getOwner()->alter(r);
    // Must recycle or there will be a memory leak.
    getOwner()->getDataParser()->recycle(r);
  }
  else{
    _setName(name);
  }
}

string arDatabaseNode::getInfo() {
  _lockInfo.lock();
    const string result = _info;
  _lockInfo.unlock();
  return result;
}

void arDatabaseNode::setInfo(const string& info){
  if (getName() == "root"){
    cout << "arDatabaseNode warning: can't set info of root.\n";
    return;
  }

  if (active()){
    arStructuredData* r = getOwner()->getDataParser()->getStorage(_dLang->AR_NAME);
    int ID = getID();
    r->dataIn(_dLang->AR_NAME_ID, &ID, AR_INT, 1);
    _lockName.lock();
      r->dataInString(_dLang->AR_NAME_NAME, _name);
    _lockName.unlock();
    r->dataInString(_dLang->AR_NAME_INFO, info);
    getOwner()->alter(r);
    getOwner()->getDataParser()->recycle(r);
  }
  else{
    _lockInfo.lock();
      _info = info;
    _lockInfo.unlock();
  }
}

// We do not worry about thread-safety if the caller does not request that
// the returned node ptr be ref'ed. If, on the other hand, the caller does
// so request, thread safety gets handled by forwarding the request to
// the owning database (if one exists) which then calls back to us (but from 
// within a _lock()).
arDatabaseNode* arDatabaseNode::findNode(const string& name, bool refNode){
  if (refNode && getOwner()){
    // Will return a ptr with an extra ref.
    return getOwner()->findNode(this, name, true); 
  }
  arDatabaseNode* result = NULL;
  bool ok = false;
  _findNode(result, name, ok, NULL, true);
  if (result && refNode){
    result->ref();
  }
  return result;
}

arDatabaseNode* arDatabaseNode::findNodeRef(const string& name){
  return findNode(name, true);
}

// We do not worry about thread-safety if the caller does not request that
// the returned node ptr be ref'ed. If, on the other hand, the caller does
// so request, thread safety gets handled by forwarding the request to
// the owning database (if one exists) which then calls back to us (but from 
// within a _lock()).
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

void arDatabaseNode::printStructure(int maxLevel, ostream& s) {
  _printStructureOneLine(0, maxLevel, s);
}

bool arDatabaseNode::receiveData(arStructuredData* data){
  if (data->getID() != _dLang->AR_NAME){
    // Called by a subclass, which already printed a diagnostic.
    return false;
  } 
  if (getName() == "root"){
    cout << "arDatabaseNode warning: failed to set root name.\n";
  }
  else{
    _lockName.lock();
      _name = data->getDataString(_dLang->AR_NAME_NAME);
    _lockName.unlock();
  }
  _lockInfo.lock();
    _info = data->getDataString(_dLang->AR_NAME_INFO);
  _lockInfo.unlock();
  return true;
}

arStructuredData* arDatabaseNode::dumpData(){
  arStructuredData* result = _dLang->makeDataRecord(_dLang->AR_NAME);
  _dumpGenericNode(result,_dLang->AR_NAME_ID);
  _lockName.lock();
    result->dataInString(_dLang->AR_NAME_NAME, _name);
  _lockName.unlock();
  _lockInfo.lock();
    result->dataInString(_dLang->AR_NAME_INFO, _info);
  _lockInfo.unlock();
  return result;
}

void arDatabaseNode::initialize(arDatabase* d){
  _setOwner(d);
  _dLang = d->_lang;
}

// So far only implemented for OWNED nodes.
void arDatabaseNode::permuteChildren(list<arDatabaseNode*>& children){
  if (active()){
    getOwner()->permuteChildren(this, children);
  }
}

// So far only implemented for OWNED nodes.
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

// A little bit unusual. To achieve thread-safety with respect to the
// owning database, we must have the owning database (if such exists)
// execute the function. If there is no owning database, just ref the
// parent and return it. 
arDatabaseNode* arDatabaseNode::getParentRef(){
  if (getOwner()){
    return getOwner()->getParentRef(this);
  }
  if (_parent){
    _parent->ref();
  }
  return _parent;
}
  
list<arDatabaseNode*> arDatabaseNode::getChildren() const {
  return _children; 
}

// For thread-safety with respect to the owning database,
// the latter should execute the function.
// If there is no owner, just return the ref'd list.
list<arDatabaseNode*> arDatabaseNode::getChildrenRef(){
  if (getOwner()){
    return getOwner()->getChildrenRef(this);
  }
  ar_refNodeList(_children);
  return _children;
}

bool arDatabaseNode::hasChildren() const {
  return _children.begin() == _children.end();
}

arNodeLevel arDatabaseNode::getNodeLevel() const {
  return _nodeLevel;
}

void arDatabaseNode::setNodeLevel(arNodeLevel nodeLevel) {
  _nodeLevel = nodeLevel;
}

//**********************************************************************
// The following functions are for tree structure manipulation. These
// are the ONLY functions in arDatabaseNode and arDatabase that should
// modify _databaseOwner, _ID, _parent, and _children.
// None of these check _databaseOwner.
// The caller should check that all nodes in 
// question are owned by the same arDatabase (in which case these are
// being called from arDatabase methods), or are owned by none (in which
// case they are being called by arDatabaseNode methods.
//**********************************************************************

void arDatabaseNode::_setName(const string& name){
  _lockName.lock();
    _name = name;
  _lockName.unlock();
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
// this block of functions). Add a ref to the
// parent and set the pointer BUT do not add it to the parent's children.
bool arDatabaseNode::_setParentNotAddingToParentsChildren
  (arDatabaseNode* parent){
  if (_parent)
    return false; // Node already had a parent.

  if (!parent)
    return false; // No new parent to add.

  // Each node holds a reference to its parent.
  _parent = parent;
  _parent->ref();
  return true;
}

// This function is given an awkward name to discourage its use (outside of
// this block of helper functions and the arDatabaseNode destructor).
// Get rid of the node's parent ref, ignoring that
// the pointer might still be in the parent's child list.
void arDatabaseNode::_removeParentLeavingInParentsChildren(){
  if (_parent){
    // Each node holds a reference to its parent.
    _parent->unref();
    _parent = NULL;
  }
}

// Tree bookkeeping.
bool arDatabaseNode::_addChild(arDatabaseNode* node){
  if (node->_parent)
    return false; // Node already had a parent.

  node->_setParentNotAddingToParentsChildren(this);
  // Add the child.
  _children.push_back(node);
  // Add a reference to the child, too.
  node->ref();
  return true;
}

// Tree bookkeeping.
bool arDatabaseNode::_removeChild(arDatabaseNode* node){
  for (list<arDatabaseNode*>::iterator i = _children.begin();
       i != _children.end(); i++){
    if (*i == node){
      (*i)->unref();
      node->_removeParentLeavingInParentsChildren();
      _children.erase(i);
      return true;
    }
  }
  return false; // Child not found.
}

// Tree bookkeeping.
void arDatabaseNode::_removeAllChildren(){
  for (list<arDatabaseNode*>::iterator i = _children.begin();
       i != _children.end(); i++){
    // Release the node's reference to the parent. 
    (*i)->_removeParentLeavingInParentsChildren();
    // Release OUR reference to the node.
    (*i)->unref();
  }
  // Empty the list.
  _children.clear();
}

// Steal all the children from a node.
void arDatabaseNode::_stealChildren(arDatabaseNode* node){
  for (list<arDatabaseNode*>::iterator i = node->_children.begin();
       i != node->_children.end(); i++){
    _children.push_back(*i);
    // Remove the child node's reference to its old
    // parent. Since the node's child list still contains *i, remove
    // that last.
    (*i)->_removeParentLeavingInParentsChildren();
    // Set the node's parent to the current node.
    (*i)->_parent = this;
    // Add a reference to the new parent.
    ref();
    // Don't add a reference to the node, since
    // the old parent had already incremented the ref count.
  }
  // The old node has no children left.
  node->_children.clear();
}

// The given children (if they are children of the node) will be moved to
// the front of the list, in the order given.
// Inefficient for large lists of nodes.
void arDatabaseNode::_permuteChildren(list<arDatabaseNode*> childList){
  childList.reverse();
  for (list<arDatabaseNode*>::iterator i = childList.begin();
       i != childList.end(); i++){
    // Given the current node. Attempt to find it in the list and move it
    // to the front. Note how we need to iterate over this list in reverse.
    for (list<arDatabaseNode*>::iterator j = _children.begin();
	 j != _children.end(); j++){
      if (*i == *j){
        // No ref or unref; just move this node to the front of the list.
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

// Not thread-safe (uses getChildren instead of getChildrenRef).
void arDatabaseNode::_findNode(arDatabaseNode*& result,
                               const string& name,
                               bool& success,
                               map<int,int,less<int> >* nodeMap,
                               bool checkTop) {
  // Check self.
  if (checkTop && getName() == name){
    success = true;
    result = this;
    return;
  }

  // Search breadth-first.
  list<arDatabaseNode*> children = getChildren();
  list<arDatabaseNode*>::const_iterator i;
  // If a node map has been passed, do NOT find an already mapped node.
  for (i = children.begin(); i != children.end(); i++){
    if ( (*i)->getName() == name && 
         (!nodeMap || nodeMap->find((*i)->getID()) == nodeMap->end())){
      success = true;
      result = *i;
      return;
    }
  }
  // Recurse.
  for (i = children.begin(); i != children.end(); i++){
    if (success){
      return;
    }
    (*i)->_findNode(result, name, success, nodeMap, true);
  }
}

// See comments re: thread-safety above findNode.
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

// helps printStructure() recurse through database
// @param currentNodeID ID of node to print out and recurse down through
// @param level How far down the tree hierarchy we are
// @param maxLevel how far down the tree hierarchy we will go
void arDatabaseNode::_printStructureOneLine(int level, int maxLevel, ostream& s) {

  for (int l=0; l<level; l++){
    if (maxLevel > l){
      s << " ";
    }
    else{
      s << " *****\n";
      return;
    }
  }

  // Inefficient list-copying.  Oh well.
  // Ref-ing ensures thread safety, but forbids constness.
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
