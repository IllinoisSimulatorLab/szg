

typedef map<int,arDatabaseNode*,less<int> >::iterator arNodeIDIterator;
typedef map<string,arDatabaseNode*,less<string> >::iterator arNodeNameIterator;

class arInputSource{
 public:
  arInputSource();
  virtual ~arInputSource();

  void setInputNode(arInputSink*);

  virtual bool init(arSZGClient&);
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
};

class arInputSink{
 public:
  arInputSink();
  virtual ~arInputSink();

  virtual bool init(arSZGClient&);
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
};

class arNetInputSource: public arInputSource{
 public:
  arNetInputSource();
  ~arNetInputSource();

  void setSlot(int slot);

  virtual bool init(arSZGClient&);
  virtual bool start();
  virtual bool stop();
  virtual bool restart();
};

class arInputNode: public arInputSink {
  // Needs assignment operator and copy constructor, for pointer members.
  public:
    arInputNode( bool bufferEvents = false );
    // if anyone ever derives from this class, make the following virtual:
    // destructor init start stop restart receiveData sourceReconfig.
    ~arInputNode();
  
    bool init(arSZGClient&);
    bool start();
    bool stop();
    bool restart();
  
    void addInputSource( arInputSource* theSource, bool iOwnIt );
  
    int getButton(int);
    float getAxis(int);
    arMatrix4 getMatrix(int);
};

%{
class NodeListWrapper{
 public:
  NodeListWrapper(list<arDatabaseNode*> nodeList){
    _list = nodeList;
    _iter = _list.begin();
  }
  ~NodeListWrapper(){}

  arDatabaseNode* get(){
    if (_iter != _list.end()){
      return *_iter;
    }
    return NULL;
  }

  void next(){ _iter++; }

  list<arDatabaseNode*> _list;
  list<arDatabaseNode*>::iterator _iter;
};
%}

class NodeListWrapper{
 public:
  NodeListWrapper(list<arDatabaseNode*>);
  ~NodeListWrapper();
  arDatabaseNode* get();
  void next();
};

class arDatabaseNode{
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();

  int getID() const;
  string getName() const;
  void setName(const string&);
  int getTypeCode() const;
  string getTypeString() const;
  
  arDatabaseNode* getParent();
  list<arDatabaseNode*> getChildren();

  arDatabaseNode* findNode(const string& name);
  arDatabaseNode* findNodeByType(const string& nodeType);
  void printStructure();
  void printStructure(int maxLevel);

%pythoncode{
  # simply present to provide a uniform interface to the nodes
  # vis-a-vis arGraphicsNode
  def parent(self):
      return self.getParent() 

  def children(self):
      wl = NodeListWrapper(self.getChildren())
      l = []
      while wl.get() != None:
          l.append(wl.get())
          wl.next()
      return l
}
};

class arGraphicsNode;
class arLightNode;
class arMaterialNode;
class arTransformNode;
class arTextureNode;
class arBoundingSphereNode;
class arBillboardNode;
class arVisibilityNode;
class arPointsNode;
class arNormal3Node;
class arColor4Node;
class arTex2Node;
class arIndexNode;
class arDrawableNode;

arGraphicsNode* castToGraphics(arDatabaseNode* n);
arLightNode* castToLight(arDatabaseNode* n);
arMaterialNode* castToMaterial(arDatabaseNode* n);
arTransformNode* castToTransform(arDatabaseNode* n);
arTextureNode* castToTexture(arDatabaseNode* n);
arBoundingSphereNode* castToBoundingSphere(arDatabaseNode* n);
arBillboardNode* castToBillboard(arDatabaseNode* n);
arVisibilityNode* castToVisibility(arDatabaseNode* n);
arPointsNode* castToPoints(arDatabaseNode* n);
arNormal3Node* castToNormal3(arDatabaseNode* n);
arColor4Node* castToColor4(arDatabaseNode* n);
arTex2Node* castToTex2(arDatabaseNode* n);
arIndexNode* castToIndex(arDatabaseNode* n);
arDrawableNode* castToDrawable(arDatabaseNode* n);

%pythoncode {

def gcast(n):
    if n == None:
        return None
    label=n.getTypeString()
    if label == 'light':
        return castToLight(n)
    elif label == 'material':
        return castToMaterial(n)
    elif label == 'transform':
        return castToTransform(n)
    elif label == 'texture':
        return castToTexture(n)
    elif label == 'bounding sphere':
        return castToBoundingSphere(n)
    elif label == 'billboard':
        return castToBillboard(n)
    elif label == 'visibility':
        return castToVisibility(n)
    elif label == 'points':
        return castToPoints(n)
    elif label == 'normal3':
        return castToNormal3(n)
    elif label == 'color4':
        return castToColor4(n)
    elif label == 'tex2':
        return castToTex2(n)
    elif label == 'index':
        return castToIndex(n)
    elif label == 'drawable':
        return castToDrawable(n)
    # DO NOT RAISE AN EXCEPTION HERE!
    # NOT ALL VALID NODES IN A GRAPHICS DATABASE ARE GRAPHICS-Y, such as
    # name nodes and root nodes...
    else:
        return n

}

/// Node in an arGraphicsDatabase.
class arGraphicsNode: public arDatabaseNode{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arGraphicsNode();
  virtual ~arGraphicsNode();

  virtual void initialize(arDatabase*);
  virtual bool receiveData(arStructuredData*);
  virtual arStructuredData* dumpData();

  virtual void draw();

%pythoncode{
  def find(self, name):
      return gcast(self.findNode(name))

  def parent(self):
      return gcast(self.getParent()) 

  def children(self):
      wl = NodeListWrapper(self.getChildren())
      l = []
      while wl.get() != None:
          l.append(gcast(wl.get()))
          wl.next()
      return l
}
};

class arGraphicsArrayNode:public arGraphicsNode{
 public:
  arGraphicsArrayNode(){}
  ~arGraphicsArrayNode(){}

  void draw();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

class arMaterial{
 public:
  arMaterial();
  ~arMaterial(){}

  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  arVector3 emissive;
  float exponent;     // i.e. shininess
  float alpha;        // transparency of the material
};

class arLight{
 public:
  arLight();
  ~arLight(){};

  int lightID;      
  arVector4 position;
  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  float     constantAttenuation;
  float     linearAttenuation;
  float     quadraticAttenuation;
  arVector3 spotDirection;
  float     spotCutoff;
  float     spotExponent;
};

class arLightNode:public arGraphicsNode{
 public:
  arLightNode(){}
  ~arLightNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arLight getLight(){ return _nodeLight; }
  void setLight(arLight& light);
};

class arMaterialNode:public arGraphicsNode{
 public:
  arMaterialNode(){}
  ~arMaterialNode(){}

  void draw(){} 
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arMaterial getMaterial(){ return _lMaterial; }
  void setMaterial(const arMaterial& material);
};

class arPointsNode:public arGraphicsArrayNode{
/// Set of (OpenGL) points.
 public:
  arPointsNode(arGraphicsDatabase*);
  ~arPointsNode(){}

  float* getPoints(int& number);
  void   setPoints(int number, float* points, int* IDs = NULL);
};

class arIndexNode:public arGraphicsArrayNode{
 public:
  arIndexNode(arGraphicsDatabase*);
  ~arIndexNode(){}

  int* getIndices(int& number);
  void setIndices(int number, int* indices, int* IDs = NULL);
};

class arNormal3Node: public arGraphicsArrayNode{
 public:
  arNormal3Node(arGraphicsDatabase*);
  ~arNormal3Node(){}

  float* getNormal3(int& number);
  void   setNormal3(int number, float* normal3, int* IDs = NULL);
};

class arTex2Node:public arGraphicsArrayNode{
 public:
  arTex2Node(arGraphicsDatabase*);
  ~arTex2Node(){}

  float* getTex2(int& number);
  void   setTex2(int number, float* tex2, int* IDs = NULL);
};

/// Texture map loaded from a file.
class arTextureNode: public arGraphicsNode{
 public:
  arTextureNode();
  ~arTextureNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  string getFileName(){ return _fileName; }
  void setFileName(const string& fileName);
};

class arColor4Node:public arGraphicsArrayNode{
 public:
  arColor4Node(arGraphicsDatabase*);
  ~arColor4Node(){}

  float* getColor4(int& number);
  void   setColor4(int number, float* color4, int* IDs = NULL);
};

/// 4x4 matrix transformation (an articulation in the scene graph).
class arTransformNode: public arGraphicsNode{
 public:
  arTransformNode(){}
  ~arTransformNode(){}

  void draw() { glMultMatrixf(_transform.v); }

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  /// Data access functions specific to arTransformNode
  arMatrix4 getTransform();
  void setTransform(const arMatrix4& transform);
};


/// Toggle visibility of subtree under this node.
class arVisibilityNode: public arGraphicsNode{
 public:
  arVisibilityNode();
  ~arVisibilityNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  bool getVisibility();
  void setVisibility(bool visibility);
};

/// Billboard display of text.
class arBillboardNode: public arGraphicsNode{
 public:
  arBillboardNode();
  ~arBillboardNode(){}

  void draw();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  string getText();
  void   setText(const string& text);
};

/// Bump map loaded from a file.

class arBumpMapNode: public arGraphicsNode{
 public:
  arBumpMapNode(){}
  ~arBumpMapNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
};

class arDrawableNode:public arGraphicsNode{
 public:
  arDrawableNode();
  ~arDrawableNode(){}

  void draw();
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  bool _01DPreDraw();
  void _01DPostDraw();
  bool _2DPreDraw();
  void _2DPostDraw();

  int getType();
  int getNumber();
  void setDrawable(arDrawableType type, int number);
};

class arCamera{
 public:
  arCamera();
  virtual ~arCamera(){}
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();
};

class arPerspectiveCamera: public arCamera{
 public:
  arPerspectiveCamera();
  virtual ~arPerspectiveCamera();

  void setSides(float left, float right, float bottom, float top);
  void setNearFar(float near, float far);
  void setPosition(float x, float y, float z);
  void setTarget(float x, float y, float z);
  void setUp(float x, float y, float z);

  float getFrustumData(int i);
  float getLookatData(int i);
};

%{

arGraphicsNode* castToGraphics(arDatabaseNode* n){
  return (arGraphicsNode*) n;
}

arLightNode* castToLight(arDatabaseNode* n){
  return (arLightNode*) n;
}

arMaterialNode* castToMaterial(arDatabaseNode* n){
  return (arMaterialNode*) n;
}

arTransformNode* castToTransform(arDatabaseNode* n){
  return (arTransformNode*) n;
}

arTextureNode* castToTexture(arDatabaseNode* n){
  return (arTextureNode*) n;
}  

arBoundingSphereNode* castToBoundingSphere(arDatabaseNode* n){
  return (arBoundingSphereNode*) n;
}

arBillboardNode* castToBillboard(arDatabaseNode* n){
  return (arBillboardNode*) n;
}

arVisibilityNode* castToVisibility(arDatabaseNode* n){
  return (arVisibilityNode*) n;
}

arPointsNode* castToPoints(arDatabaseNode* n){
  return (arPointsNode*) n;
}

arNormal3Node* castToNormal3(arDatabaseNode* n){
  return (arNormal3Node*) n;
}

arColor4Node* castToColor4(arDatabaseNode* n){
  return (arColor4Node*) n;
}

arTex2Node* castToTex2(arDatabaseNode* n){
  return (arTex2Node*) n;
}

arIndexNode* castToIndex(arDatabaseNode* n){
  return (arIndexNode*) n;
}

arDrawableNode* castToDrawable(arDatabaseNode* n){
  return (arDrawableNode*) n;
}

%}

/// Generic database distributed across a cluster.
class arDatabase{
 public:
  arDatabase();
  virtual ~arDatabase();

  int getNodeID(const string& name, bool fWarn=true);
  arDatabaseNode* getNode(int, bool fWarn=true);
  arDatabaseNode* getNode(const string&, bool fWarn=true);
  arDatabaseNode* findNode(const string& name);
  arDatabaseNode* getRoot(){ return &_rootNode; }
  arDatabaseNode* newNode(arDatabaseNode* parent, const string& type,
                          const string& name="");
  bool eraseNode(int ID);

  virtual arDatabaseNode* alter(arStructuredData*);
  arDatabaseNode* alterRaw(ARchar*);
  bool handleDataQueue(ARchar*);

  virtual bool readDatabase(const string& fileName, 
                            const string& path = "");
  virtual bool readDatabaseXML(const string& fileName, 
                               const string& path = "");
  virtual bool attach(arDatabaseNode* parent,
                      const string& fileName,
                      const string& path = "");
  virtual bool attachXML(arDatabaseNode* parent,
                         const string& fileName,
                         const string& path = "");
  virtual bool merge(arDatabaseNode* parent,
                     const string& fileName,
                     const string& path = "");
  virtual bool mergeXML(arDatabaseNode* parent,
                        const string& fileName,
                        const string& path = "");
  virtual bool writeDatabase(const string& fileName, 
                             const string& path = "");
  virtual bool writeDatabaseXML(const string& fileName, 
                                const string& path = "");
  virtual bool writeRooted(arDatabaseNode* parent,
                           const string& fileName,
                           const string& path="");
  virtual bool writeRootedXML(arDatabaseNode* parent,
                              const string& fileName,
                              const string& path="");
  

  bool empty();
  virtual void reset();
  void printStructure(int maxLevel);
  void printStructure();
};

class arGraphicsDatabase: public arDatabase{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arGraphicsDatabase();
  virtual ~arGraphicsDatabase();

  virtual arDatabaseNode* alter(arStructuredData*);
  virtual void reset();

  void loadAlphabet(const char*);
  arTexture** getAlphabet();
  void setTexturePath(const string& thePath);
  arTexture* addTexture(const string&, int*);
  arTexture* addTexture(int w, int h, bool alpha, const char* pixels);
  arBumpMap* addBumpMap(const string& name, int numPts, int numInd,
		  	float* points, int* indices, float* tex2,
			float height, arTexture* decalTexture);

  arMatrix4 accumulateTransform(int nodeID);

  void draw();
  int intersect(const arRay&);

  bool registerLight(int owningNodeID, arLight* theLight);
  void activateLights();

  bool registerCamera(int owningNodeID, arPerspectiveCamera* theCamera);

%pythoncode{
    def new(self, node, type, name):
        theNode = gcast(self.newNode(node,type,name))
        return theNode
    
    def get(self, name):
        return gcast(self.getNode(name))

    def find(self, name):
        return gcast(self.findNode(name))
}
};

class arGraphicsPeerConnection{
 public:
  arGraphicsPeerConnection();
  ~arGraphicsPeerConnection();

  string    remoteName;
  // Critical that we use the connection ID... this is one thing that is
  // actually unique about the connection. The remoteName is not unique
  // (though it is at any particular time) since we could have open, then
  // closed, then opened again, a connection between two peers. 
  int       connectionID;
  bool      receiving;
  bool      sending;
  // list<int> nodesLockedLocal;
  // list<int> nodesLockedRemote;

%extend{
    string __repr__() {
        return self->print();
    }
}
};

// %rename(PeerList) list<arGraphicsPeerConnection>;

class arGraphicsPeer: public arGraphicsDatabase{
 public:
  arGraphicsPeer();
  ~arGraphicsPeer();
  
  string getName();
  void setName(const string&);

  bool init(int&,char**);
  bool start();
  void stop();

  bool alter(arStructuredData*);

  virtual bool readDatabase(const string& fileName, 
                            const string& path = "");
  virtual bool readDatabaseXML(const string& fileName, 
                               const string& path = "");
  virtual bool attach(arDatabaseNode* parent,
                      const string& fileName,
                      const string& path = "");
  virtual bool attachXML(arDatabaseNode* parent,
                         const string& fileName,
                         const string& path = "");
  virtual bool merge(arDatabaseNode* parent,
                     const string& fileName,
                     const string& path = "");
  virtual bool mergeXML(arDatabaseNode* parent,
                        const string& fileName,
                        const string& path = "");
  virtual bool writeDatabase(const string& fileName, 
                             const string& path = "");
  virtual bool writeDatabaseXML(const string& fileName, 
                                const string& path = "");
  virtual bool writeRooted(arDatabaseNode* parent,
                           const string& fileName,
                           const string& path="");
  virtual bool writeRootedXML(arDatabaseNode* parent,
                              const string& fileName,
                              const string& path="");

  void useLocalDatabase(bool);
  void queueData(bool);
  int consume();
  
  int connectToPeer(const string& name);
  bool closeConnection(const string& name);
  bool receiving(const string& name, bool state);
  bool sending(const string& name, bool state);
  bool pullSerial(const string& name, int remoteRootID, int localRootID,
                  int sendLevel, bool remoteSendOn, bool localSendOn);
  bool pushSerial(const string& name, int remoteRootID, int localRootID,
                  int sendLevel, bool remoteSendOn, bool localSendOn);
  bool closeAllAndReset();
  bool broadcastFrameTime(int frameTime);
  bool remoteLockNode(const string& name, int nodeID);
  bool remoteUnlockNode(const string& name, int nodeID);
  bool localLockNode(const string& name, int nodeID);
  bool localUnlockNode(int nodeID);
  bool remoteFilterDataBelow(const string& peer,
                             int remoteNodeID, int on);
  bool localFilterDataBelow(const string& peer,
                            int localNodeID, int on);
  int  remoteNodeID(const string& peer, const string& nodeName);
  
  string printConnections();
  string printPeer();
%extend{
    string __repr__() {
        return self->printPeer();
    }
}
%pythoncode{
  def pathMap(self, node, fileName):
      return self.mapXML(node,fileName,'')

  def pathRead(self,fileName):
      return self.readDatabaseXML(fileName,'')

  def pathWrite(self,fileName):
      return self.writeDatabaseXML(fileName,'')

  def pathWriteRooted(self,node,fileName):
      return self.writeRootedXML(node,fileName,'')
}
};

%{

void transformManipTask(void*);

class TransformManip{
 friend void transformManipTask(void*);
 public:
  TransformManip(){ 
    _stopping = false;
    _running = false;
    _t = NULL;
    ar_mutex_init(&_lock);
  }
  
  ~TransformManip(){}

  void setInput(arInputNode* n){ _n = n; }
  
  void setNode(arTransformNode* t){ 
    ar_mutex_lock(&_lock);
    _t = t;
    _originalMatrix = t->getTransform();
    ar_mutex_unlock(&_lock); 
  }

  void start(){ 
    _running = true;
    _thread.beginThread(transformManipTask, this);
  }

  void stop(){
    _stopping = true;
    while (_running){
      ar_usleep(10000);
    }
    _stopping = false;
  }

 private:
  bool _stopping;
  bool _running;
  arThread _thread;
  arInputNode* _n;
  arTransformNode* _t;
  arMutex _lock;
  arMatrix4 _originalMatrix;
};

void transformManipTask(void* tm){
  arMatrix4 initialSensorRot;
  arVector3 initialSensorTrans;
  arMatrix4 initialObjectRot, initialObjectTrans;
  int lastButton[6];
  int currentButton[6];
  int i;
  for (i=0; i<6; i++){
    lastButton[i] = 0; 
    currentButton[i] = 0;
  }
  bool grabbed = false;
  float scaleFactor = 1;
  TransformManip* m = (TransformManip*) tm;
  while (!m->_stopping){
    ar_mutex_lock(&m->_lock);
    if (m->_t){
      for (i=0; i<6; i++){
        currentButton[i] = m->_n->getButton(i);
      }
      if (currentButton[3]){
        m->_t->setTransform(m->_originalMatrix);
      }
      if (currentButton[1] && !lastButton[1]){
        scaleFactor *= 0.5;
        if (scaleFactor < 0.1){
          scaleFactor = 0.1;
        }
      }
      if (currentButton[2] && !lastButton[2]){
        scaleFactor *= 2;
        if (scaleFactor > 10){
          scaleFactor = 10;
        }
      }
      float objectScaling = 1;
      if (m->_n->getAxis(1) > 0.8){
        objectScaling = 1.01;
      }
      if (m->_n->getAxis(1) < -0.8){
        objectScaling = 0.99;
      }
      // grabbing
      if (!currentButton[0] && grabbed){
        grabbed  = false;
      }
      arMatrix4 sensor = m->_n->getMatrix(0);
      arMatrix4 trans = m->_t->getTransform();
      if (currentButton[0] && !grabbed){
        grabbed = true;
        initialSensorRot = ar_extractRotationMatrix(sensor);
        initialSensorTrans = sensor*arVector3(0,0,0);
        initialObjectRot = ar_extractRotationMatrix(trans);
        initialObjectTrans = ar_extractTranslationMatrix(trans);
      }
      if (grabbed){
        arVector3 translation = sensor*arVector3(0,0,0);
        m->_t->setTransform
          (ar_translationMatrix(
            (translation[0]-initialSensorTrans[0])*scaleFactor,
            (translation[1]-initialSensorTrans[1])*scaleFactor,
            (translation[2]-initialSensorTrans[2])*scaleFactor)
           * initialObjectTrans
           * ar_extractRotationMatrix(sensor)
           * !initialSensorRot
           * initialObjectRot
           * ar_scaleMatrix(objectScaling, objectScaling, objectScaling)
           * ar_extractScaleMatrix(trans) );
      }
      for (i=0; i<6; i++){
        lastButton[i] = currentButton[i];
      }
    }
    ar_mutex_unlock(&m->_lock);
    ar_usleep(20000);
  }
  m->_running = false;
}

class AnimationMapper{
public:
  AnimationMapper();
  ~AnimationMapper();

  bool makeMap(int externalNodeID, arGraphicsDatabase* externalDatabase);
  void setFile(const string& name);
  bool start();
  bool stop();

  arThread fileTask;
  bool stopping;
  bool stopped;
  map<int, int, less<int> > nodeMap;
  arGraphicsDatabase local;
  arGraphicsDatabase* external;
  arStructuredDataParser* parser;
  arFileTextStream textStream;
  FILE* inputFile;
  string inputFileName;
  int numberSegments;
  arDatabaseNode* _externalNode;
};

void ar_animationMapperTask(void* mapper){
  AnimationMapper* am = (AnimationMapper*) mapper;
  while (!am->stopping){
    for (int i=0; i<2*am->numberSegments; i++){
      arStructuredData* record = am->parser->parse(&am->textStream);
      if (!record){
        // file must, in fact, be finished.
        am->textStream.ar_close();
        am->inputFile = fopen(am->inputFileName.c_str(),"r");
        am->textStream.setSource(am->inputFile);
        break;
      }
      am->local.filterData(record, am->nodeMap);
      am->external->alter(record);
      am->parser->recycle(record);
    }
    ar_usleep(16667);
  }  
  am->stopped = true;
}

AnimationMapper::AnimationMapper(){
  parser = NULL;
  stopping = false;
  stopped = true;
  inputFileName = string("default-motion.xml");
  // DOH! HARDCODED!
  numberSegments = 20;
  _externalNode = NULL;
  arTransformNode* t[20];
  arTransformNode* sc[20];
  arDatabaseNode* root = local.getRoot();
  t[0] = (arTransformNode*) local.newNode(root, "transform", "LowerTorso");
  t[1] = (arTransformNode*) local.newNode(t[0], "transform", "UpperTorso");
  t[2] = (arTransformNode*) local.newNode(t[1], "transform", "Neck");
  t[3] = (arTransformNode*) local.newNode(t[2], "transform", "Head");
  t[4] = (arTransformNode*) local.newNode(t[1], "transform", "RCollarBone");
  t[5] = (arTransformNode*) local.newNode(t[4], "transform", "RUpArm");
  t[6] = (arTransformNode*) local.newNode(t[5], "transform", "RLowArm");
  t[7] = (arTransformNode*) local.newNode(t[6], "transform", "RHand");
  t[8] = (arTransformNode*) local.newNode(t[1], "transform", "LCollarBone");
  t[9] = (arTransformNode*) local.newNode(t[8], "transform", "LUpArm");
  t[10] = (arTransformNode*) local.newNode(t[9], "transform", "LLowArm");
  t[11] = (arTransformNode*) local.newNode(t[10], "transform", "LHand");
  t[12] = (arTransformNode*) local.newNode(t[0], "transform", "RPelvis");
  t[13] = (arTransformNode*) local.newNode(t[12], "transform", "RThigh");
  t[14] = (arTransformNode*) local.newNode(t[13], "transform", "RLowLeg");
  t[15] = (arTransformNode*) local.newNode(t[14], "transform", "RFoot");
  t[16] = (arTransformNode*) local.newNode(t[0], "transform", "LPelvis");
  t[17] = (arTransformNode*) local.newNode(t[16], "transform", "LThigh");
  t[18] = (arTransformNode*) local.newNode(t[17], "transform", "LLowLeg");
  t[19] = (arTransformNode*) local.newNode(t[18], "transform", "LFoot");
  for (int i=0; i<20; i++){
    sc[i] = (arTransformNode*) local.newNode(t[i], "transform", 
                                             t[i]->getName() + ".scale");
  }
}

AnimationMapper::~AnimationMapper(){
  stop();
}

bool AnimationMapper::makeMap(int externalNodeID,
			      arGraphicsDatabase* externalDatabase){
  stop();
  external = externalDatabase;
  nodeMap.clear();
  _externalNode = externalDatabase->getNode(externalNodeID);
  if (!_externalNode){
    cout << "AnimationMapper error: node does not exist with that ID.\n";
    return false;
  }
  _externalNode->setName(inputFileName);
  return local.createNodeMap(externalNodeID, externalDatabase, nodeMap);
}

void AnimationMapper::setFile(const string& name){
  stop();
  inputFileName = name;
  if (_externalNode){
    _externalNode->setName(inputFileName);
  }
}

bool AnimationMapper::start(){
  if (!stopped){
    cout << "AnimationMapper error: cannot start again.\n";
    return false;
  }
  inputFile = fopen(inputFileName.c_str(), "r");
  if (!inputFile){
    cout << "AnimationMapper error: failed to open file.\n";
    return false;
  }
  textStream.setSource(inputFile);
  parser = new arStructuredDataParser(local._gfx.getDictionary());
  stopped = false;
  fileTask.beginThread(ar_animationMapperTask, this);
  return true;
}

bool AnimationMapper::stop(){
  stopping = true;
  while (!stopped){
    ar_usleep(100000);
  }
  stopping = false;
  if (parser){
    delete parser;
    parser = NULL;
    textStream.ar_close();
  }
  return true;
}

%}

class TransformManip{
 public:
  TransformManip();
  ~TransformManip();

  void setInput(arInputNode*);
  void setNode(arTransformNode*);
  void start();
  void stop();
};

class AnimationMapper{
 public:
  AnimationMapper();
  ~AnimationMapper();

  bool makeMap(int externalNodeID, arGraphicsDatabase* externalDatabase);
  void setFile(const string& name);
  bool start();
  bool stop();
};
