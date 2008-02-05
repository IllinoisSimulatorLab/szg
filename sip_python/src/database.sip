// Defines mapping from python to C++ strings.
%Include typemaps.sip

enum arNodeLevel {
  AR_IGNORE_NODE = -1,
  AR_STRUCTURE_NODE,
  AR_STABLE_NODE,
  AR_OPTIONAL_NODE,
  AR_TRANSIENT_NODE
};


class arDatabaseNode {
%TypeHeaderCode
#include "arDatabaseNode.h"
%End
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();
  bool active();
  void ref();
  void unref();
  int getRef();
  string getName();
  void setName(const string& name);
  bool hasInfo();
  string getInfo();
  void setInfo(const string& info);
  int getTypeCode() const;
  string getTypeString() const;

  // If the node has an owner, it can act as a node factory as well.
  // This method will NOT worked with an "unowned" node.
  arDatabaseNode* newNode( const string& type, const string& name = "",
                          bool refNode = false ) /TransferBack/;
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

  void printStructure();
  void printStructure(int maxLevel);
  //void printStructure(int maxLevel, ostream& s);

  //virtual arStructuredData* dumpData();
  //virtual bool receiveData(arStructuredData*);
  //virtual void initialize(arDatabase* d);
  // Called by arDatabase upon removing the node.
  virtual void deactivate();

  //void permuteChildren(list<arDatabaseNode*>& children);
  //void permuteChildren(int number, int* children);

// These accessors (with the next block of functions)
// should be the only way that external code touches _databaseOwner,
// _ID, _parent, and _children. Although short, leave them out of the
// header file for future flexibility.

  bool isroot() const;
  int getID() const;
  //arDatabase* getOwner() const;
  arDatabaseNode* getParent() const;
  //arStructuredDataParser* getParser() const;
  //arStructuredData* getStorage(int) const;
  //inline void recycle(arStructuredData* r) const {
  // A version of getParent() that is thread-safe with respect to database
  // manipulations. The arDatabaseNode ptr returned has an extra reference
  // added to it (will not be deleted out from under us, for instance).
  // Only really useful when this node is OWNED by a database.
  arDatabaseNode* getParentRef();
  //list<arDatabaseNode*> getChildren() const;
  // A version of getChildren() that is thread-safe with respect to database
  // manipulations. Each arDatabaseNode ptr returned has am extra reference
  // added.
  //list<arDatabaseNode*> getChildrenRef();
  bool hasChildren() const;

  arNodeLevel getNodeLevel() const;
  void setNodeLevel(arNodeLevel nodeLevel);

  string dumpOneline();

 private:
  arDatabaseNode( const arDatabaseNode& );
};

