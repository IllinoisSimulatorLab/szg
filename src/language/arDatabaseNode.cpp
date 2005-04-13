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
// is the root node by testing one of them (all of which are unique
// for all nodes in the database)
arDatabaseNode::arDatabaseNode():
  _ID(0),
  _name("root"),
  _typeCode(-1),
  _typeString("root"),
  _parent(NULL),
  _transient(false){
  
  _databaseOwner = NULL;
  _dLang = NULL;
}

arDatabaseNode::~arDatabaseNode(){
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
    _databaseOwner->alter(r);
    delete r;
  }
  else{
    _name = name;
  }
}

arDatabaseNode* arDatabaseNode::findNode(const string& name){
  arDatabaseNode* result = NULL;
  bool success = false;
  _findNode(result, name, success, true);
  return result;
}

arDatabaseNode* arDatabaseNode::findNodeByType(const string& nodeType){
  arDatabaseNode* result = NULL;
  bool success = false;
  _findNodeByType(result, nodeType, success, true);
  return result;
}

void arDatabaseNode::printStructure(int maxLevel, ostream& s){
  _printStructureOneLine(0, maxLevel, s);
}

bool arDatabaseNode::receiveData(arStructuredData* data){
  if (data->getID() != _dLang->AR_NAME){
    // DO NOT PRINT ANYTHING OUT HERE. THE ASSUMPTION IS THAT THIS IS
    // BEING CALLED FROM ELSEWHERE (i.e. FROM A SUBCLASS) WHERE THE
    // MESSAGE WILL, IN FACT, BE PRINTED. 
    return false;
  } 
  _name = data->getDataString(_dLang->AR_NAME_NAME);
  return true;
}

arStructuredData* arDatabaseNode::dumpData(){
  arStructuredData* result = _dLang->makeDataRecord(_dLang->AR_NAME);
  _dumpGenericNode(result,_dLang->AR_NAME_ID);
  result->dataInString(_dLang->AR_NAME_NAME, _name);
  return result;
}

void arDatabaseNode::initialize(arDatabase* d){
  // AARGH! THIS IS JUST BAD DESIGN!
  _databaseOwner = d;
  _dLang = d->_lang;
}

void arDatabaseNode::_dumpGenericNode(arStructuredData* theData,int IDField){
  (void)theData->dataIn(IDField,&_ID,AR_INT,1);
}

void arDatabaseNode::_findNode(arDatabaseNode*& result,
                               const string& name,
                               bool& success,
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
  for (i = children.begin(); i != children.end(); i++){
    if ( (*i)->getName() == name ){
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
    (*i)->_findNode(result, name, success, true);
  }
}

void arDatabaseNode::_findNodeByType(arDatabaseNode*& result,
                                     const string& nodeType,
                                     bool& success,
                                     bool checkTop){
  // First, check self.
  if (checkTop && getTypeString() == nodeType){
    success = true;
    result = this;
    return;
  }
  list<arDatabaseNode*> children = getChildren();
  // we are doing a breadth-first search (maybe..), check the children first
  // then recurse
  list<arDatabaseNode*>::iterator i;
  for (i = children.begin(); i != children.end(); i++){
    if ( (*i)->getTypeString() == nodeType ){
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
    (*i)->_findNodeByType(result, nodeType, success, true);
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
