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
 friend class arGraphicsPeer;
 public:
  arDatabase();
  virtual ~arDatabase();

  void setDataBundlePath(const string& bundlePathName, const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName, const string& bundlePath);

  int getNodeID(const string& name, bool fWarn=true);
  arDatabaseNode* getNode(int, bool fWarn=true);
  arDatabaseNode* getNode(const string&, bool fWarn=true);
  arDatabaseNode* findNode(const string& name);
  arDatabaseNode* getRoot(){ return &_rootNode; }

  // These functions manipulate the tree structure of the database.
  // IT IS UNCLEAR TO ME EXTACTLY WHICH OF THESE BELONG HERE AND
  // WHICH BELONG MORE PROPERLY AS METHODS OF arDatabaseNode
  arDatabaseNode* newNode(arDatabaseNode* parent, const string& type,
                          const string& name = "");
  bool eraseNode(int ID);
  // THE FOLLOWING ARE FUNCTIONS TO BE IMPLEMENTED AT SOME POINT...
  /*arDatabaseNode* insertNode(arDatabaseNode* parent,
			     arDatabaseNode* child,
			     const string& type,
			     const string& name = "");
  void attachNode(arDatabaseNode* parent,
                  arDatabaseNode* node);
  void detachNode(arDatabaseNode* node);*/

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

  bool empty(); // not const, because it uses a lock
  virtual void reset();
  
  void printStructure(int maxLevel);
  void printStructure(int maxLevel, ostream& s);
  void printStructure() { printStructure(10000); }

  // THIS isServer stuff is REALLY OBNOXIOUS! IT HAS LED TO SEVERAL BUGS!
  // REALLY ALL IT IS DOING IS FIGURING OUT WHEN NOT TO LOAD TEXTURES!
  // (MAYBE IT SHOULD BE CALLED SOMETHING ELSE???)
  bool isServer() const
    { return _server; }

  //available for external data input use  (public data members are dangerous!)
  arStructuredData* eraseData;
  arStructuredData* queuedDataData;
  arStructuredData* makeNodeData;

  arDatabaseLanguage* _lang;

 protected:
  // GRUMBLE. I'm a little annoyed by the _server flag. BUT... it does
  // do something helpful... namely enable a object that doesn't need to
  // display (i.e. arGraphicsServer in the arDistSceneGraphFramework) do
  // LESS (unnecessary) work (like load textures into memory, which might fail
  // anyway since there is no graphics window).
  bool _server;
  // Stuff pertaining to finding the data bundles.
  string                               _bundlePathName;
  string                               _bundleName;
  map<string,string,less<string> >     _bundlePathMap;
  arMutex _bufferLock;
  arMutex _eraseLock;
  arMutex _deletionLock;
  map<int,arDatabaseNode*,less<int> > _nodeIDContainer;
  int _nextAssignedID;
  arDatabaseNode _rootNode;
  
  // here is the machinery that assists in the data processing
  arDatabaseProcessingCallback _databaseReceive[256];
  arStructuredData*            _parsingData[256];
  int                          _routingField[256];

  bool _initDatabaseLanguage();
  arDatabaseNode* _eraseNode(arStructuredData*);
  arDatabaseNode* _makeDatabaseNode(arStructuredData*);
  arDatabaseNode* _insertDatabaseNode(const string& typeString, int parentID, 
                                      int nodeID, const string& nodeName);
  
  // Helper functions for recursive operations.
  void _writeDatabase(arDatabaseNode* pNode, arStructuredData& nodeData,
                      ARchar*& buffer, size_t& bufferSize, FILE* destFile);
  void _writeDatabaseXML(arDatabaseNode* pNode, arStructuredData& nodeData,
                         FILE* destFile);
  void _eraseNode(arDatabaseNode*);
  void _createNodeMap(arDatabaseNode* localNode, int externalNodeID,
		      arDatabase* externalDatabase,
		      map<int, int, less<int> >& nodeMap,
		      bool& failure);
  void _insertOutFilter(map<int,int,less<int> >& outFilter,
			int nodeID,
			bool sendOn);
  int _filterIncoming(arDatabaseNode* mappingRoot,
                      arStructuredData* record, 
	              map<int, int, less<int> >& nodeMap,
		      int* mappedIDs,
		      map<int, int, less<int> >* outFilter,
		      bool outFilterOn,
                      bool allNew);
  
  // The factory function that is redefined by subclasses.
  virtual arDatabaseNode* _makeNode(const string& type);
};

#endif
