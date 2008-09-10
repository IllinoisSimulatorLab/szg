//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATABASE_H
#define AR_DATABASE_H

#include "arDatabaseNode.h"
#include "arDatabaseLanguage.h"
#include "arNameNode.h"
#include "arStructuredDataParser.h"
#include "arLanguageCalling.h"
#include <iostream>
#include <stack>
#include <vector>
using namespace std;

typedef map<int, arDatabaseNode*, less<int> >::iterator arNodeIDIterator;
typedef map<int, arDatabaseNode*, less<int> >::const_iterator const_arNodeIDIterator;

typedef arDatabaseNode*
    (arDatabase::*arDatabaseProcessingCallback)(arStructuredData*);

// Codes for different database types.
enum{
  AR_GENERIC_DATABASE = 0,
  AR_GRAPHICS_DATABASE = 1,
  AR_SOUND_DATABASE = 2
};

// Generic database distributed across a cluster.
class SZG_CALL arDatabase{
 public:
  arDatabase();
  virtual ~arDatabase();

  int getTypeCode() const { return _typeCode; }
  string getTypeString() const { return _typeString; }

  void setDataBundlePath(const string& bundlePathName,
                         const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName,
                            const string& bundlePath);

  // getNode(), findNode(), etc., aren't const because they ref().
  int getNodeID(const string& name, bool fWarn=true);
  arDatabaseNode* getNode(int ID, bool fWarn=true, bool refNode=false);
  arDatabaseNode* getNodeRef(int ID, bool fWarn=true);
  arDatabaseNode* getNode(const string& name, bool fWarn=true,
                          bool refNode=false);
  arDatabaseNode* getNodeRef(const string& name, bool fWarn=true);
  arDatabaseNode* findNode(const string& name, bool refNode=false);
  arDatabaseNode* findNodeRef(const string& name);
  arDatabaseNode* findNode(arDatabaseNode* node, const string& name,
                           bool refNode = false);
  arDatabaseNode* findNodeRef(arDatabaseNode* node, const string& name);
  arDatabaseNode* findNodeByType(arDatabaseNode* node, const string& nodeType,
                                 bool refNode = false);
  arDatabaseNode* findNodeByTypeRef(arDatabaseNode* node,
                                    const string& nodeType);
  arDatabaseNode* getRoot() { return &_rootNode; }

  arDatabaseNode* getParentRef(arDatabaseNode*);
  list<arDatabaseNode*> getChildrenRef(arDatabaseNode*);

  // These functions manipulate the tree structure of the database.
  // Some of them are mirrored as methods of arDatabase nodes.
  arDatabaseNode* newNode(arDatabaseNode* parent, const string& type,
                          const string& name = "", bool refNode = false);
  arDatabaseNode* newNodeRef(arDatabaseNode* parent, const string& type,
                             const string& name = "");
  arDatabaseNode* insertNode(arDatabaseNode* parent,
                             arDatabaseNode* child,
                             const string& type,
                             const string& name = "",
                             bool refNode = false);
  arDatabaseNode* insertNodeRef(arDatabaseNode* parent,
                             arDatabaseNode* child,
                             const string& type,
                             const string& name = "");
  bool cutNode(arDatabaseNode* node);
  bool cutNode(int ID);
  bool eraseNode(arDatabaseNode* node);
  bool eraseNode(int ID);
  void permuteChildren(arDatabaseNode* parent,
                       list<arDatabaseNode*>& children);
  void permuteChildren(arDatabaseNode* parent,
                       int number, int* children);

  bool fillNodeData(arStructuredData* data, arDatabaseNode* node);

  virtual arDatabaseNode* alter(arStructuredData* data, bool refNode = false);
  arDatabaseNode* alterRaw(ARchar*);
  bool handleDataQueue(ARchar*);

  virtual bool readDatabase(const string& fileName, const string& path="");
  virtual bool readDatabaseXML(const string& fileName, const string& path="");
  virtual bool attach(arDatabaseNode* parent,
                      const string& fileName,
                      const string& path="");
  virtual bool attachXML(arDatabaseNode* parent,
                         const string& fileName,
                         const string& path="");
  virtual bool merge(arDatabaseNode* parent,
                     const string& fileName,
                     const string& path="");
  virtual bool mergeXML(arDatabaseNode* parent,
                        const string& fileName,
                        const string& path="");
  virtual bool writeDatabase(const string& fileName, const string& path="");
  virtual bool writeDatabaseXML(const string& fileName,
                                const string& path="");
  virtual bool writeRooted(arDatabaseNode* parent,
                           const string& fileName,
                           const string& path="");
  virtual bool writeRootedXML(arDatabaseNode* parent,
                              const string& fileName,
                              const string& path="");

  bool createNodeMap(int externalNodeID,
                     arDatabase* externalDatabase,
                     arNodeMap& nodeMap);
  bool filterData(arStructuredData* record,
                  arNodeMap& nodeMap);
  int  filterIncoming(arDatabaseNode* mappingRoot,
                      arStructuredData* record,
                      arNodeMap& nodeMap,
                      int* mappedIDs,
                      arNodeMap* outFilter,
                      arNodeLevel outFilterLevel,
                      bool allNew);

  bool empty(); // not const, because it uses a lock
  virtual void reset();

  void printStructure(int maxLevel);
  void printStructure(int maxLevel, ostream& s);
  void printStructure() { printStructure(10000); }

  // Databases like arGraphicsDatabase store info (texture maps) but don't manage it
  // for some other activity. If _server, as with arGraphicsDatabase, it will not load textures.
  bool isServer() const
    { return _server; }

  inline arStructuredDataParser* getDataParser() { return _dataParser; }

  //available for external data input use  (public data members are dangerous!)
  arStructuredData* eraseData;
  arStructuredData* makeNodeData;

  arDatabaseLanguage* _lang;

 private:
  arLock _dbLock;
  arDatabaseNode* _ref(arDatabaseNode*, const bool);

 protected:
  // Used by arGraphicsPeer, arGraphicsDatabase, etc.
  void _lock(const char* name = NULL) { _dbLock.lock(name); }
  void _unlock() { _dbLock.unlock(); }

  bool _check(arDatabaseNode* n) const
    { return n && n->active() && n->getOwner()==this; }

  int    _typeCode; // what "kind" of database
  string _typeString;

  // If true, work less (don't load textures since there's no OpenGL context).
  bool _server;

  // Finding data bundles.
  string                               _bundlePathName;
  string                               _bundleName;
  map<string, string, less<string> >     _bundlePathMap;

  map<int, arDatabaseNode*, less<int> > _nodeIDContainer;
  int _nextAssignedID;
  arDatabaseNode _rootNode;

  // Here is the machinery that assists in the data processing
  arDatabaseProcessingCallback _databaseReceive[256];
  arStructuredData*            _parsingData[256];
  int                          _routingField[256];
  arStructuredDataParser*      _dataParser;

  bool _initDatabaseLanguage();
  arDatabaseNode* _getNodeNoLock(int ID, bool fWarn=false); // Call only while lock()'d.
  string          _getDefaultName();
  arDatabaseNode* _makeDatabaseNode(arStructuredData*);
  arDatabaseNode* _insertDatabaseNode(arStructuredData*);
  arDatabaseNode* _cutDatabaseNode(arStructuredData*);
  arDatabaseNode* _eraseNode(arStructuredData*);
  arDatabaseNode* _permuteDatabaseNodes(arStructuredData*);

  arDatabaseNode* _createChildNode(arDatabaseNode* parentNode,
                                   const string& typeString,
                                   int nodeID,
                                   const string& nodeName,
                                   bool moveChildren);
  void _cutNode(arDatabaseNode*);
  void _eraseNode(arDatabaseNode*);

  // Helper functions for recursive operations.
  void _writeDatabase(arDatabaseNode* pNode, arStructuredData& nodeData,
                      ARchar*& buffer, size_t& bufferSize, FILE* destFile);
  void _writeDatabaseXML(arDatabaseNode* pNode, arStructuredData& nodeData,
                         FILE* destFile);
  void _createNodeMap(arDatabaseNode* localNode, int externalNodeID,
                      arDatabase* externalDatabase,
                      arNodeMap& nodeMap,
                      bool& failure);
  void _insertOutFilter(arNodeMap& outFilter,
                        int nodeID,
                        arNodeLevel outFilterLevel); // Call only while lock()'d.
  arDatabaseNode* _mapNodeBelow(arDatabaseNode* parent,
                                const string& nodeType,
                                const string& nodeName,
                                arNodeMap& nodeMap);
  int _filterIncomingMakeNode(arDatabaseNode* mappingRoot,
                      arStructuredData* record,
                      arNodeMap& nodeMap,
                      int* mappedIDs,
                      arNodeMap* outFilter,
                      arNodeLevel outFilterLevel,
                      bool allNew);
  int _filterIncomingInsert(arDatabaseNode* mappingRoot,
                      arStructuredData* data,
                      arNodeMap& nodeMap,
                      int* mappedIDs);
  int _filterIncomingErase(arDatabaseNode* mappingRoot,
                           arStructuredData* data,
                           arNodeMap& nodeMap);
  int _filterIncomingCut(arStructuredData* data,
                         arNodeMap& nodeMap);
  int _filterIncomingPermute(arStructuredData* data,
                             arNodeMap& nodeMap);

  // The factory function that is redefined by subclasses.
  virtual arDatabaseNode* _makeNode(const string& type);
};

#endif
