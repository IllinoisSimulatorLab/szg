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
  _lockInfo("DBASENODE_INFO"),
  _lockName("DBASENODE_NAME"),
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

arDatabaseNode::~arDatabaseNode() {
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
bool arDatabaseNode::active() {
  return getOwner() && (getParent() || isroot());
}

void arDatabaseNode::ref() {
  ++_refs;
}

void arDatabaseNode::unref() {
  if (--_refs == 0)
    delete this;
}

int arDatabaseNode::getRef() const {
  return _refs;
}

// If this node has an owning database, use that as a node factory.
arDatabaseNode* arDatabaseNode::newNode(const string& type,
                                        const string& name,
                                        bool refNode) {
  return active() ? getOwner()->newNode(this, type, name, refNode) : NULL;
}

// Wrapper for newNode.  Return a ref'ed node pointer.
arDatabaseNode* arDatabaseNode::newNodeRef(const string& type,
                                           const string& name) {
  return newNode(type, name, true);
}


// Make an existing node a child of this node.
// Neither node can be owned by an arDatabase.
// Detecting cycles is the caller's responsibility.
bool arDatabaseNode::addChild(arDatabaseNode* child) {
  // Bug: should test that the new child has the right type,
  // e.g. arGraphicsNode doesn't match arSoundNode.
  return child && !getOwner() && !child->getOwner() && _addChild(child);
}

// Remove an existing node from the child list of this node.
// Return false on failure, e.g. if one of the nodes
// is owned by an arDatabase.
bool arDatabaseNode::removeChild(arDatabaseNode* child) {
  return child && !getOwner() && !child->getOwner() && _removeChild(child);
}

string arDatabaseNode::getName() {
  arGuard _(_lockName, "arDatabaseNode::getName");
  return _name;
}

void arDatabaseNode::setName(const string& name) {
  if (!active()) {
    _setName(name);
    return;
  }

  arStructuredData* r = getOwner()->getDataParser()->getStorage(_dLang->AR_NAME);
  const int ID = getID();
  r->dataIn(_dLang->AR_NAME_ID, &ID, AR_INT, 1);
  r->dataInString(_dLang->AR_NAME_NAME, name);
  _lockInfo.lock("arDatabaseNode::setName");
    r->dataInString(_dLang->AR_NAME_INFO, _info);
  _lockInfo.unlock();
  getOwner()->alter(r);
  recycle(r); // avoid memory leak
}

bool arDatabaseNode::hasInfo() {
  arGuard _(_lockInfo, "arDatabaseNode::hasInfo");
  return !_info.empty();
}

string arDatabaseNode::getInfo() {
  arGuard _(_lockInfo, "arDatabaseNode::getInfo");
  return _info;
}

void arDatabaseNode::setInfo(const string& info) {
  if (isroot()) {
    cout << "arDatabaseNode warning: can't set info of root.\n";
    return;
  }

  if (active()) {
    arStructuredData* r = getOwner()->getDataParser()->getStorage(_dLang->AR_NAME);
    int ID = getID();
    r->dataIn(_dLang->AR_NAME_ID, &ID, AR_INT, 1);
    _lockName.lock("arDatabaseNode::setInfo active");
      r->dataInString(_dLang->AR_NAME_NAME, _name);
    _lockName.unlock();
    r->dataInString(_dLang->AR_NAME_INFO, info);
    getOwner()->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_lockInfo, "arDatabaseNode::setInfo inactive");
    _info = info;
  }
}

string arDatabaseNode::dumpOneline() {
  string s(ar_intToString(_ID) + ", name " + getName());
  const string info(getInfo());
  if (!info.empty())
    s += ", info " + info;
  return s + ".\n";
}

// We do not worry about thread-safety if the caller does not request that
// the returned node ptr be ref'ed. If, on the other hand, the caller does
// so request, thread safety gets handled by forwarding the request to
// the owning database (if one exists) which then calls back to us (but from
// within a _lock()).
arDatabaseNode* arDatabaseNode::findNode(const string& name, bool refNode) {
  if (refNode && getOwner()) {
    // Will return a ptr with an extra ref.
    return getOwner()->findNode(this, name, true);
  }

  arDatabaseNode* r = NULL;
  bool ok = false;
  _findNode(r, name, ok, NULL, true);
  if (r && refNode) {
    r->ref();
  }
  return r;
}

arDatabaseNode* arDatabaseNode::findNodeRef(const string& name) {
  return findNode(name, true);
}

// We do not worry about thread-safety if the caller does not request that
// the returned node ptr be ref'ed. If, on the other hand, the caller does
// so request, thread safety gets handled by forwarding the request to
// the owning database (if one exists) which then calls back to us (but from
// within a _lock()).
arDatabaseNode* arDatabaseNode::findNodeByType(const string& nodeType,
                                               bool refNode) {
  if (refNode && getOwner()) {
    // Will return a ptr with an extra ref.
    return getOwner()->findNodeByType(this, nodeType, true);
  }

  arDatabaseNode* r = NULL;
  bool success = false;
  _findNodeByType(r, nodeType, success, NULL, true);
  if (r && refNode) {
    r->ref();
  }
  return r;
}

arDatabaseNode* arDatabaseNode::findNodeByTypeRef(const string& nodeType) {
  return findNodeByType(nodeType, true);
}

void arDatabaseNode::printStructure(int maxLevel, ostream& s) {
  _printStructureOneLine(0, maxLevel, s);
}

bool arDatabaseNode::receiveData(arStructuredData* data) {
  if (data->getID() != _dLang->AR_NAME) {
    // Called by a subclass, which already printed a diagnostic.
    return false;
  }
  if (isroot()) {
    ar_log_error() << "arDatabaseNode::receiveData cannot rename root node.\n";
  }
  else{
    arGuard _(_lockName, "arDatabaseNode::receiveData name");
    _name = data->getDataString(_dLang->AR_NAME_NAME);
  }
  arGuard _(_lockInfo, "arDatabaseNode::receiveData info");
  _info = data->getDataString(_dLang->AR_NAME_INFO);
  return true;
}

arStructuredData* arDatabaseNode::dumpData() {
  arStructuredData* result = _dLang->makeDataRecord(_dLang->AR_NAME);
  _dumpGenericNode(result, _dLang->AR_NAME_ID);
  _lockName.lock("arDatabaseNode::dumpData name");
    result->dataInString(_dLang->AR_NAME_NAME, _name);
  _lockName.unlock();
  _lockInfo.lock("arDatabaseNode::dumpData info");
    result->dataInString(_dLang->AR_NAME_INFO, _info);
  _lockInfo.unlock();
  return result;
}

void arDatabaseNode::initialize(arDatabase* d) {
  _setOwner(d);
  _dLang = d->_lang;
}

// So far only implemented for OWNED nodes.
void arDatabaseNode::permuteChildren(list<arDatabaseNode*>& children) {
  if (active()) {
    getOwner()->permuteChildren(this, children);
  }
}

// So far only implemented for OWNED nodes.
void arDatabaseNode::permuteChildren(int number, int* children) {
  if (active()) {
    getOwner()->permuteChildren(this, number, children);
  }
}

// Thread-safe w.r.t. the owning database.
arDatabaseNode* arDatabaseNode::getParentRef() {
  if (getOwner()) {
    // Owning database exists.  It executes the function.
    return getOwner()->getParentRef(this);
  }

  // Just ref and return the parent.
  if (_parent) {
    _parent->ref();
  }
  return _parent;
}

// Warning: copies the whole list!
list<arDatabaseNode*> arDatabaseNode::getChildren() const {
  return _children;
}

// For thread-safety with respect to the owning database,
// the latter should execute the function.
// If there is no owner, just return the ref'd list.
list<arDatabaseNode*> arDatabaseNode::getChildrenRef() {
  if (getOwner()) {
    return getOwner()->getChildrenRef(this);
  }

  ar_refNodeList(_children);
  return _children;
}

bool arDatabaseNode::empty() const {
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

void arDatabaseNode::_setName(const string& name) {
  arGuard _(_lockName, "arDatabaseNode::_setName");
  _name = name;
}

// A node either belongs to an arDatabase or it doesn't. This will only be
// changed around the time of node creation. If the node is created by the
// user directly, then it will never be owned by an arDatabase. If it is
// created by an arDatabase, then this function will be called soon after
// creation (from inside arDatabaseNode::initialize()).
void arDatabaseNode::_setOwner(arDatabase* owner) {
  _databaseOwner = owner;
}

// The node's ID we only be set away from the default (0) if it is part of
// an arDatabase. If so, it will be set once upon creation and never
// subsequently changed.
void arDatabaseNode::_setID(int ID) {
  _ID = ID;
}

// This function is given an awkward name to discourage its use (outside of
// this block of functions). Add a ref to the
// parent and set the pointer BUT do not add it to the parent's children.
bool arDatabaseNode::_setParentNotAddingToParentsChildren
  (arDatabaseNode* parent) {
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
void arDatabaseNode::_removeParentLeavingInParentsChildren() {
  if (_parent) {
    // Each node holds a reference to its parent.
    _parent->unref();
    _parent = NULL;
  }
}

// Tree bookkeeping.
bool arDatabaseNode::_addChild(arDatabaseNode* node) {
  if (node->_parent)
    return false; // Node already had a parent.

  node->_setParentNotAddingToParentsChildren(this);
  // Add and ref the child.
  _children.push_back(node);
  node->ref();
  return true;
}

// Tree bookkeeping.
bool arDatabaseNode::_removeChild(arDatabaseNode* node) {
  for (list<arDatabaseNode*>::iterator i = _children.begin();
       i != _children.end(); i++) {
    if (*i == node) {
      (*i)->unref();
      node->_removeParentLeavingInParentsChildren();
      _children.erase(i);
      return true;
    }
  }
  return false; // Child not found.
}

// Tree bookkeeping.
void arDatabaseNode::_removeAllChildren() {
  for (list<arDatabaseNode*>::iterator i = _children.begin();
       i != _children.end(); i++) {
    // Release the node's reference to the parent.
    (*i)->_removeParentLeavingInParentsChildren();
    // Release OUR reference to the node.
    (*i)->unref();
  }
  // Empty the list.
  _children.clear();
}

// Steal all the children from a node.
void arDatabaseNode::_stealChildren(arDatabaseNode* node) {
  for (list<arDatabaseNode*>::iterator i = node->_children.begin();
       i != node->_children.end(); ++i) {
    _children.push_back(*i);
    // Remove the child node's reference to its old parent.
    // Since the node's child list still contains *i, remove that last.
    (*i)->_removeParentLeavingInParentsChildren();
    // Set the node's parent to the current node.
    (*i)->_parent = this;
    // Ref the new parent.
    ref();
    // Don't ref the node, because the old parent already incremented the ref count.
  }
  // The old node has no children left.
  node->_children.clear();
}

// Move the given children (if they are children of the node)
// to the front of the list, in the order given.
// Inefficient for large lists.
void arDatabaseNode::_permuteChildren(list<arDatabaseNode*> childList) {
  childList.reverse();
  for (list<arDatabaseNode*>::iterator i = childList.begin();
       i != childList.end(); i++) {
    // Given the current node. Attempt to find it in the list and move it
    // to the front. Note how we need to iterate over this list in reverse.
    for (list<arDatabaseNode*>::iterator j = _children.begin();
         j != _children.end(); j++) {
      if (*i == *j) {
        // No ref or unref; just move this node to the front of the list.
        arDatabaseNode* node = *j;
        _children.erase(j);
        _children.push_front(node);
        // Search is done.
        break;
      }
    }
  }
}

//***************************************************************************
// Helper functions, mostly for recursion.
//***************************************************************************

void arDatabaseNode::_dumpGenericNode(arStructuredData* r, int IDField) {
  if (!r)
    return;
  int ID = getID();
  (void)r->dataIn(IDField, &ID, AR_INT, 1);
}

// Not thread-safe (uses getChildren, actually just _children to avoid
// copying a list, instead of getChildrenRef).
// Since not thread-safe, use _name instead of getName().
void arDatabaseNode::_findNode(arDatabaseNode*& result,
                               const string& name,
                               bool& success,
                               const arNodeMap* nodeMap,
                               const bool checkTop) {
  // Check self.
  if (checkTop && _name == name) {
    success = true;
    result = this;
    return;
  }

  // Breadth-first.  Search children.
  const list<arDatabaseNode*>::const_iterator iFirst = _children.begin();
  const list<arDatabaseNode*>::const_iterator iLast = _children.end();
  list<arDatabaseNode*>::const_iterator i;
  for (i = iFirst; i != iLast; ++i) {
    // If we have a node map, skip already mapped nodes.
    if ( (*i)->_name == name &&
         (!nodeMap || nodeMap->find((*i)->getID()) == nodeMap->end())) {
      success = true;
      result = *i;
      return;
    }
  }

  // Recurse.
  for (i = iFirst; !success && (i != iLast); ++i) {
    (*i)->_findNode(result, name, success, nodeMap, true);
  }
}

// Not thread-safe (uses getChildren, actually just _children to avoid
// copying a list, instead of getChildrenRef).
// Since not thread-safe, use _name instead of getName().
void arDatabaseNode::_findNodeByType(arDatabaseNode*& result,
                                     const string& nodeType,
                                     bool& success,
                                     const arNodeMap* nodeMap,
                                     const bool checkTop) {
  // Check self.
  if (checkTop && getTypeString() == nodeType) {
    success = true;
    result = this;
    return;
  }

  // Breadth-first.  Search children.
  const list<arDatabaseNode*>::const_iterator iFirst = _children.begin();
  const list<arDatabaseNode*>::const_iterator iLast = _children.end();
  list<arDatabaseNode*>::const_iterator i;
  for (i = _children.begin(); i != _children.end(); ++i) {
    // If we have a node map, skip already mapped nodes.
    if ( (*i)->getTypeString() == nodeType &&
         (!nodeMap || nodeMap->find((*i)->getID()) == nodeMap->end())) {
      success = true;
      result = *i;
      return;
    }
  }

  // Recurse.
  for (i = iFirst; !success && (i != iLast); ++i) {
    (*i)->_findNodeByType(result, nodeType, success, nodeMap, true);
  }
}

// helps printStructure() recurse through database
// @param currentNodeID ID of node to print out and recurse down through
// @param level How far down the tree hierarchy we are
// @param maxLevel how far down the tree hierarchy we will go
void arDatabaseNode::_printStructureOneLine(int level, int maxLevel, ostream& s) {

  for (int l=0; l<level; l++) {
    s << " ";
    if (l >= maxLevel) {
      s << "*****\n";
      return;
    }
  }

  // Inefficient list-copying.  Oh well.
  // Ref-ing ensures thread safety, but forbids constness.
  list<arDatabaseNode*> childList = getChildrenRef();

  s << '(' << getID() << ", "
    << "\"" << getName() << "\", "
    << "\"" << getTypeString() << "\")\n";

  for (list<arDatabaseNode*>::const_iterator i(childList.begin());
       i != childList.end(); ++i) {
    (*i)->_printStructureOneLine(level+1, maxLevel, s);
  }
  // Since we ref'ed the list of nodes, unref.
  ar_unrefNodeList(childList);
}

bool arDatabaseNode::isroot() const {
  // Faster than, but otherwise the same as, getName() == "root".
  return _ID == 0;
}

int arDatabaseNode::getID() const {
  return _ID;
}

arDatabase* arDatabaseNode::getOwner() const {
  return _databaseOwner;
}

arDatabaseNode* arDatabaseNode::getParent() const {
  return _parent;
}

arStructuredDataParser* arDatabaseNode::getParser() const {
  return getOwner()->getDataParser();
}

arStructuredData* arDatabaseNode::getStorage(int id) const {
  return getParser()->getStorage(id);
}
