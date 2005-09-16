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

// THIS IS AN OBNOXIOUS HACK!!!!!
// REALLY SHOULD TRY TO REMOVE THESE FORWARD DECLARATIONS!!!!
class arDatabase;
class arGraphicsDatabase;
class arSoundDatabase;

/// Node in an arDatabase.

class SZG_CALL arDatabaseNode{
  friend class arDatabase;
  friend class arGraphicsDatabase;
  friend class arGraphicsPeer;
  friend class arSoundDatabase;
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();

  void ref();
  void unref();

  int getID() const { return _ID; }
  string getName() const { return _name; }
  void setName(const string& name);
  int getTypeCode() const { return _typeCode; }
  string getTypeString() const { return _typeString; }

  arDatabase* getOwningDatabase(){ return _databaseOwner; }
  // If the node has an owner, it can act as a node factory as well.
  arDatabaseNode* newNode(const string& type, const string& name = ""); 
  // Sometimes we want to be able to take an existing node and add it to
  // the database.
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // There should be some sort of check to make sure that the node is of the
  // right type (i.e. arGraphicsNode or arSoundNode).
  void attach(arDatabaseNode* child);

  arDatabaseNode* getParent(){ return _parent; }
  list<arDatabaseNode*> getChildren(){ return _children; }

  arDatabaseNode* findNode(const string& name);
  arDatabaseNode* findNodeByType(const string& nodeType);

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

  void setTransient(bool state){ _transient = state; }
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

  // This lock guards all changes to node structure. Such as changing the
  // database owner, removing children, adding children.
  arMutex _nodeLock;
  // This lock guards all changes to node data. Used on a case by case basis
  // by the code in individual nodes.
  arMutex _dataLock;

  // Must keep a reference count.
  int _refs;

  // Might be the case that we want to filter messages into the node based,
  // somehow, on certain known properties it has. For instance, the node
  // might just hold "transient data".
  bool       _transient;

  void _dumpGenericNode(arStructuredData*, int);
  void _addChild(arDatabaseNode* node);
  void _findNode(arDatabaseNode*& result, const string& name, bool& success,
		 bool checkTop);
  void _findNodeByType(arDatabaseNode*& result, const string& nodeType,
                       bool& success, bool checkTop);
  void _printStructureOneLine(int level, int maxLevel, ostream& s);
};

#endif
