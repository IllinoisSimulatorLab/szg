//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATABASE_H
#define AR_DATABASE_H

#include "arDatabaseNode.h"
#include "arDatabaseLanguage.h"
#include "arThread.h"
#include "arNameNode.h"
#include "arStructuredDataParser.h"
#include "arLanguageCalling.h"
#include <iostream>
#include <stack>
#include <vector>
using namespace std;

typedef map<int,arDatabaseNode*,less<int> >::iterator arNodeIDIterator;

typedef arDatabaseNode*
    (arDatabase::*arDatabaseProcessingCallback)(arStructuredData*);

/// Generic database distributed across a cluster.
class SZG_CALL arDatabase{
 public:
  arDatabase();
  virtual ~arDatabase();

  void setDataBundlePath(const string& bundlePathName, 
                         const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName, 
                            const string& bundlePath);

  int getNodeID(const string& name, bool fWarn=true);
  arDatabaseNode* getNode(int, bool fWarn=true);
  arDatabaseNode* getNode(const string&, bool fWarn=true);
  arDatabaseNode* findNode(const string& name);
  arDatabaseNode* getRoot(){ return &_rootNode; }

  // These functions manipulate the tree structure of the database.
  // Some of them are mirrored as methods of arDatabase nodes.
  arDatabaseNode* newNode(arDatabaseNode* parent, const string& type,
                          const string& name = "");
  arDatabaseNode* insertNode(arDatabaseNode* parent,
			     arDatabaseNode* child,
			     const string& type,
			     const string& name = "");
  bool cutNode(int ID);
  bool eraseNode(int ID);
  void permuteChildren(arDatabaseNode* parent,
                       list<int>& childIDs);
  
  bool fillNodeData(arStructuredData* data, arDatabaseNode* node);

  virtual arDatabaseNode* alter(arStructuredData*);
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

  bool createNodeMap(int externalNodeID, arDatabase* externalDatabase,
		     map<int, int, less<int> >& nodeMap);
  bool filterData(arStructuredData* record,
		  map<int, int, less<int> >& nodeMap);
  int  filterIncoming(arDatabaseNode* mappingRoot,
                      arStructuredData* record, 
	              map<int, int, less<int> >& nodeMap,
		      int* mappedIDs,
		      map<int, int, less<int> >* outFilter,
		      bool outFilterOn,
                      bool allNew);

  bool empty(); // not const, because it uses a lock
  virtual void reset();
  
  void printStructure(int maxLevel);
  void printStructure(int maxLevel, ostream& s);
  void printStructure() { printStructure(10000); }
  // Abbreviations for the above.
  void ps(int maxLevel){ printStructure(maxLevel); }
  void ps(int maxLevel, ostream& s){ printStructure(maxLevel, s); }
  void ps(){ printStructure(); }

  // Sometimes a database only needs to store information, not manage it
  // for some other activity (as in the case of the arGraphicsDatabase and
  // loading textures). If isServer() returns true (in the arGraphicsDatabase
  // case), it will not load textures.
  bool isServer() const
    { return _server; }

  //available for external data input use  (public data members are dangerous!)
  arStructuredData* eraseData;
  arStructuredData* makeNodeData;

  arDatabaseLanguage* _lang;

 protected:
  // The server flag enables a object that doesn't need to
  // display (i.e. arGraphicsServer in the arDistSceneGraphFramework) do
  // LESS unnecessary work (like load textures into memory, which might fail
  // anyway since there is no OpenGL context).
  bool _server;
  // Stuff pertaining to finding the data bundles.
  string                               _bundlePathName;
  string                               _bundleName;
  map<string,string,less<string> >     _bundlePathMap;

  //arMutex _bufferLock;
  //arMutex _eraseLock;
  //arMutex _deletionLock;
  arMutex _databaseLock;

  map<int,arDatabaseNode*,less<int> > _nodeIDContainer;
  int _nextAssignedID;
  arDatabaseNode _rootNode;
  
  // Here is the machinery that assists in the data processing
  arDatabaseProcessingCallback _databaseReceive[256];
  arStructuredData*            _parsingData[256];
  int                          _routingField[256];
  arStructuredDataParser*      _dataParser;

  bool _initDatabaseLanguage();
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
		      map<int, int, less<int> >& nodeMap,
		      bool& failure);
  void _insertOutFilter(map<int,int,less<int> >& outFilter,
			int nodeID,
			bool sendOn);
  arDatabaseNode* _mapNodeBelow(arDatabaseNode* parent,
				const string& nodeType,
				const string& nodeName,
				map<int,int,less<int> >& nodeMap);
  int _filterIncomingMakeNode(arDatabaseNode* mappingRoot,
                      arStructuredData* record, 
	              map<int, int, less<int> >& nodeMap,
		      int* mappedIDs,
		      map<int, int, less<int> >* outFilter,
		      bool outFilterOn,
                      bool allNew);
  int _filterIncomingInsert(arDatabaseNode* mappingRoot,
                      arStructuredData* data, 
	              map<int, int, less<int> >& nodeMap,
		      int* mappedIDs);
  int _filterIncomingErase(arDatabaseNode* mappingRoot,
                           arStructuredData* data,
			   map<int, int, less<int> >& nodeMap);
  int _filterIncomingCut(arStructuredData* data,
			 map<int, int, less<int> >& nodeMap);
  int _filterIncomingPermute(arStructuredData* data,
			     map<int, int, less<int> >& nodeMap);
  
  // The factory function that is redefined by subclasses.
  virtual arDatabaseNode* _makeNode(const string& type);
};

#endif
