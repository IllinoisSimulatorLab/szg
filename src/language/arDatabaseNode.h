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
using namespace std;

// THIS IS AN OBNOXIOUS HACK!!!!!
// REALLY SHOULD TRY TO REMOVE THESE FORWARD DECLARATIONS!!!!
class arDatabase;
class arGraphicsDatabase;
class arSoundDatabase;

/// Node in an arDatabase.

class arDatabaseNode{
  friend class arDatabase;
  friend class arGraphicsDatabase;
  friend class arSoundDatabase;
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();

  int getID() const { return _ID; }
  string getName() const { return _name; }
  // NOTE: THE FOLLOWING IS SOMEWHAT UNSAFE SINCE CHANGES ARE NOT PROPOGATED
  // to connected databases.
  void setName(const string& name);
  int getTypeCode() const { return _typeCode; }
  string getTypeString() const { return _typeString; }

  arDatabase* getOwningDatabase(){ return _databaseOwner; }

  arDatabaseNode* getParent(){ return _parent; }
  list<arDatabaseNode*> getChildren(){ return _children; }

  arDatabaseNode* findNode(const string& name);
  arDatabaseNode* findNodeByType(const string& nodeType);
  void printStructure(){ printStructure(10000); }
  void printStructure(int maxLevel){ printStructure(maxLevel, cout); }
  void printStructure(int maxLevel, ostream& s);

  virtual arStructuredData* dumpData();
  virtual bool receiveData(arStructuredData*);
  virtual void initialize(arDatabase* d);
 protected:
  ARint  _ID;
  string _name;
  int    _typeCode;
  string _typeString;

  // THIS IS A LITTLE OBNOXIOUS... the arGraphicsDatabase and arSoundDatabase
  // both have a referenced owing database and language. Which is duplicated
  // here.... This will be solved once all the database operations are
  // made better unified.
  arDatabase*         _databaseOwner;
  arDatabaseLanguage* _dLang;

  arDatabaseNode* _parent;
  list<arDatabaseNode*> _children;

  void _dumpGenericNode(arStructuredData*, int);
  void _findNode(arDatabaseNode*& result, const string& name, bool& success);
  void _findNodeByType(arDatabaseNode*& result, const string& nodeType,
                       bool& success);
  void _printStructureOneLine(int level, int maxLevel, ostream& s);
};

#endif
