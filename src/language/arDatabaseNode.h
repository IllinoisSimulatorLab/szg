//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATABASE_NODE_H
#define AR_DATABASE_NODE_H

#include <list>
#include <string>
#include "arStructuredDataParser.h"
#include "arDatabaseLanguage.h"
#include "arLanguageCalling.h"
using namespace std;

// Forward declaration, since an arDatabase can own an arDatabaseNode.
class arDatabase;

// Common way to manage nodes.
typedef map< int, int, less<int> > arNodeMap;
inline arNodeMap::value_type arNodePair(const int a, const int b) { return arNodeMap::value_type(a, b); }

// Types of nodes.
enum arNodeLevel{
  AR_IGNORE_NODE = -1,
  AR_STRUCTURE_NODE,
  AR_STABLE_NODE,
  AR_OPTIONAL_NODE,
  AR_TRANSIENT_NODE };

// Node in an arDatabase.

class SZG_CALL arDatabaseNode{
  friend class arDatabase;
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();

  bool active();

  void ref();
  void unref();
  int getRef() const;

  string getName();
  void setName(const string& name);
  bool hasInfo();
  string getInfo();
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

  void printStructure() { printStructure(10000); }
  void printStructure(int maxLevel) { printStructure(maxLevel, cout); }
  void printStructure(int maxLevel, ostream& s);

  virtual arStructuredData* dumpData();
  virtual bool receiveData(arStructuredData*);
  virtual void initialize(arDatabase* d);
  // Called by arDatabase upon removing the node.
  virtual void deactivate() {}

  void permuteChildren(list<arDatabaseNode*>& children);
  void permuteChildren(int number, int* children);

// These accessors (with the next block of functions)
// should be the only way that external code touches _databaseOwner,
// _ID, _parent, and _children. Although short, leave them out of the
// header file for future flexibility.

  bool isroot() const;
  int getID() const;
  arDatabase* getOwner() const;
  arDatabaseNode* getParent() const;
  arStructuredDataParser* getParser() const;
  arStructuredData* getStorage(int) const;
  inline void recycle(arStructuredData* r) const {
    getParser()->recycle(r);
  }
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
  bool empty() const;

  arNodeLevel getNodeLevel() const;
  void setNodeLevel(arNodeLevel nodeLevel);

  string dumpOneline();

 private:
  // Lock _info and _name (data i/o).
  // Not thread-safe for changes to the database owner, children, and parent.
  string _info;
  arLock _lockInfo;
  arLock _lockName;
  ARint  _ID;

 protected:
  string _name;

 protected:
  int    _typeCode;
  string _typeString;

  // _databaseOwner _parent _children aren't guarded by a specific arLock.
  // THIS IS A LITTLE OBNOXIOUS... the arGraphicsDatabase and arSoundDatabase
  // both have a referenced owning database and language. Which is duplicated
  // here.... This will be solved once all the database operations are
  // made better unified.
  arDatabase*         _databaseOwner; // Who owns us.
  arDatabaseLanguage* _dLang; // Language used to encode messages.

  arDatabaseNode* _parent;
  list<arDatabaseNode*> _children;

  // For derived classes.  Mutable because some const methods use it.
  mutable arLock _nodeLock;

  // Reference count.
  arIntAtom _refs;

  // Might be the case that we want to filter messages into the node based,
  // somehow, on certain known properties it has. For instance, the node
  // might just hold "transient data".
  arNodeLevel  _nodeLevel;

  // Generic structure manipulation.
  void _setName(const string& name);
  void _setOwner(arDatabase* database);
  void _setID(int ID);
  bool _setParentNotAddingToParentsChildren(arDatabaseNode* parent);
  void _removeParentLeavingInParentsChildren();
  bool _addChild(arDatabaseNode* node);
  bool _removeChild(arDatabaseNode* node);
  void _removeAllChildren();
  void _stealChildren(arDatabaseNode* node);
  void _permuteChildren(list<arDatabaseNode*> childList);
  // Helper for serializing a generic node.
  void _dumpGenericNode(arStructuredData*, int);
  // Recursive helper functions.
  void _findNode(arDatabaseNode*& result, const string& name, bool& success,
                 const arNodeMap* nodeMap, const bool checkTop);
  void _findNodeByType(arDatabaseNode*& result, const string& nodeType,
                       bool& success, const arNodeMap* nodeMap, const bool checkTop);
  void _printStructureOneLine(int level, int maxLevel, ostream& s);
};

#endif
