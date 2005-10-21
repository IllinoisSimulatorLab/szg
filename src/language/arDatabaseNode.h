//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATABASE_NODE_H
#define AR_DATABASE_NODE_H

#include <list>
#include <string>
#include "arStructuredData.h"
#include "arDatabaseLanguage.h"
#include "arLanguageCalling.h"
using namespace std;

// This forward declaration is necessary since an arDatabaseNode can be
// owned by an arDatabase.
class arDatabase;

/// The nodes are of various types.

enum arNodeLevel{ AR_IGNORE_NODE = -1,
                  AR_STRUCTURE_NODE = 0, 
                  AR_STABLE_NODE = 1,
                  AR_OPTIONAL_NODE = 2,
                  AR_TRANSIENT_NODE = 3 };

/// Node in an arDatabase.

class SZG_CALL arDatabaseNode{
  friend class arDatabase;
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();

  bool active();

  void ref();
  void unref();
  int getRef();

  void lock();
  void unlock();

  string getName() const;
  void setName(const string& name);
  string getInfo() const;
  void setInfo(const string& info);
  int getTypeCode() const { return _typeCode; }
  string getTypeString() const { return _typeString; }

  // If the node has an owner, it can act as a node factory as well.
  // This method will NOT worked with an "unowned" node.
  arDatabaseNode* newNode(const string& type, const string& name = "",
                          bool refNode = false);
  arDatabaseNode* newNodeRef(const string& type, const string& name = "");
 
  // Sometimes we want to work with node trees that are not controlled by
  // an arDatabase. This method will NOT work with an "owned" node.
  bool addChild(arDatabaseNode* child);
  // Also only for node trees that are NOT owned by an arDatabase.
  bool removeChild(arDatabaseNode* child);

  arDatabaseNode* findNode(const string& name, bool refNode = false);
  arDatabaseNode* findNodeRef(const string& name);
  arDatabaseNode* findNodeByType(const string& nodeType, bool refNode = false);
  arDatabaseNode* findNodeByTypeRef(const string& nodeType);

  void printStructure(){ printStructure(10000); }
  void printStructure(int maxLevel){ printStructure(maxLevel, cout); }
  void printStructure(int maxLevel, ostream& s);
  // Abbreviations for the above.
  void ps(){ printStructure(); }
  void ps(int maxLevel){ printStructure(maxLevel); }
  void ps(int maxLevel, ostream& s){ printStructure(maxLevel, s); }

  virtual arStructuredData* dumpData();
  virtual bool receiveData(arStructuredData*);
  virtual void initialize(arDatabase* d);
  // Called by arDatabase upon removing the node.
  virtual void deactivate(){}

  int getID() const;
  inline arDatabase* getOwner() const { return _databaseOwner; }
  arDatabaseNode* getParent() const;
  // A version of getParent() that is thread-safe with respect to database
  // manipulations. The arDatabaseNode ptr returned has an extra reference
  // added to it (will not be deleted out from under us, for instance).
  // Only really useful when this node is OWNED by a database.
  arDatabaseNode* getParentRef();
  list<arDatabaseNode*> getChildren() const;
  // A version of getChildren() that is thread-safe with respect to database
  // manipulations. Each arDatabaseNode ptr returned has am extra reference
  // added.
  list<arDatabaseNode*> getChildrenRef();
  bool hasChildren() const;

  arNodeLevel getNodeLevel();
  void setNodeLevel(arNodeLevel nodeLevel);
 protected:
  ARint  _ID;
  string _name;
  int    _typeCode;
  string _typeString;

  // THIS IS A LITTLE OBNOXIOUS... the arGraphicsDatabase and arSoundDatabase
  // both have a referenced owning database and language. Which is duplicated
  // here.... This will be solved once all the database operations are
  // made better unified.
  arDatabase*         _databaseOwner;
  arDatabaseLanguage* _dLang;

  arDatabaseNode* _parent;
  list<arDatabaseNode*> _children;

  // This lock guards changes to the reference count and lets data go into
  // and out of the node atomically. NOTE: it does not ensure total thread
  // safety since changes to the database owner, children, and parent are not
  // counted. Must be mutable since it is locked/unlocked in some const
  // methods.
  mutable arMutex _nodeLock;

  // Must keep a reference count.
  int _refs;

  // Might be the case that we want to filter messages into the node based,
  // somehow, on certain known properties it has. For instance, the node
  // might just hold "transient data".
  arNodeLevel  _nodeLevel;

  // The node is allowed to hold an "info" string.
  string _info;

  // Generic structure manipulation functions.
  void _setName(const string& name);
  void _setOwner(arDatabase* database);
  void _setID(int ID);
  bool _setParentNotAddingToParentsChildren(arDatabaseNode* parent);
  void _removeParentLeavingInParentsChildren();
  bool _addChild(arDatabaseNode* node);
  void _removeChild(arDatabaseNode* node);
  void _removeAllChildren();
  void _stealChildren(arDatabaseNode* node);
  void _permuteChildren(list<arDatabaseNode*> childList);
  // Helper function for serializing a generic node.
  void _dumpGenericNode(arStructuredData*, int);
  // Recursive helper functions.
  void _findNode(arDatabaseNode*& result, const string& name, bool& success,
		 map<int,int,less<int> >* nodeMap, bool checkTop);
  void _findNodeByType(arDatabaseNode*& result, const string& nodeType,
                       bool& success, map<int,int,less<int> >* nodeMap,  
                       bool checkTop);
  void _printStructureOneLine(int level, int maxLevel, ostream& s);
};

#endif
