// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

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
      // MUST ref here for compatibility with the python memory management.
      (*_iter)->ref();
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

enum arNodeLevel{ AR_IGNORE_NODE = -1,
                  AR_STRUCTURE_NODE = 0, 
                  AR_STABLE_NODE = 1,
                  AR_OPTIONAL_NODE = 2,
                  AR_TRANSIENT_NODE = 3 };

class arDatabaseNode{
 public:
  arDatabaseNode();
  virtual ~arDatabaseNode();

  void ref();
  void unref();
  int  getRef();

  int getID() const;
  string getName() const;
  void setName(const string&);
  string getInfo() const;
  void setInfo(const string&);
  int getTypeCode() const;
  string getTypeString() const;
  
  arDatabaseNode* newNodeRef(const string& type, const string& name="");
  arDatabase* getOwner();
  arDatabaseNode* getParentRef();
  list<arDatabaseNode*> getChildrenRef();
  arDatabaseNode* findNodeRef(const string& name);
  arDatabaseNode* findNodeByTypeRef(const string& nodeType);

  void printStructure();
  void printStructure(int maxLevel);
  void ps();
  void ps(int maxLevel);

  arNodeLevel getNodeLevel();
  void setNodeLevel(arNodeLevel nodeLevel);

%pythoncode{
  def __del__(self):
      try:
          if self.thisown == 0 or self.thisown == 1:
             if self.getOwner() == None or self.getName() != "root":
                 self.unref()
      except: pass

  # simply present to provide a uniform interface to the nodes
  # vis-a-vis arGraphicsNode
  def new(self, type, name=""):
      return self.newNodeRef(type, name)

  def parent(self):
      return self.getParentRef() 

  def children(self):
      wl = NodeListWrapper(self.getChildrenRef())
      l = []
      while wl.get() != None:
          l.append(wl.get())
          wl.next()
      return l

  def find(self, name):
      return self.findNodeRef(name)

  def findByType(self, type):
      return self.findNodeByTypeRef(type)
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

%pythoncode{
  def __del__(self):
      try:
          if self.thisown == 0 or self.thisown == 1:
              if self.getOwner() == None or self.getName() != "root":
                  self.unref()
      except: pass
  
  def new(self, type, name=""):
      return gcast(self.newNodeRef(type, name))

  def parent(self):
      return gcast(self.getParentRef()) 

  def children(self):
      wl = NodeListWrapper(self.getChildrenRef())
      l = []
      while wl.get() != None:
          l.append(gcast(wl.get()))
          wl.next()
      return l
 
  def find(self, name):
      return gcast(self.findNodeRef(name))

  def findByType(self, type):
      return gcast(self.findNodeByTypeRef(type))

}
};

class arRay{
 public:
  arRay();
  arRay(const arVector3& origin, const arVector3& direction);
  ~arRay();

  void transform(const arMatrix4&);
  // Intersection with sphere.
  float intersect(float radius, const arVector3& position);
  const arVector3& getOrigin() const;
  const arVector3& getDirection() const;

%extend{
  string __repr__( void ){
    ostringstream s;
    s << "arRay\n";
    s << "origin: arVector3" << self->getOrigin() << "\n";
    s << "direction: arVector3" << self->getDirection() << "\n";
    return s.str();
  }
}
};

class arBoundingSphere{
 public:
  arBoundingSphere();
  arBoundingSphere(const arVector3& position, float radius);
  ~arBoundingSphere();

  bool intersectViewFrustum(arMatrix4& m);

  arVector3 position;
  float     radius;
  bool      visibility;
%extend{
  string __repr__( void ){
    ostringstream s;
    s << "arBoundingSphere\n";
    s << "position: arVector3" << self->position << "\n";
    s << "radius: " << self->radius << "\n";
    s << "visibility: " << self->visibility << "\n";
    return s.str();
  }
}
};

class arMaterial{
 public:
  arMaterial();
  ~arMaterial();

  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  arVector3 emissive;
  float exponent; 
  float alpha; 
%extend{
  string __repr__(void){
    stringstream s;
    s << "arMaterial\n";
    s << "diffuse:  arVector3" << self->diffuse << "\n";
    s << "ambient:  arVector3" << self->ambient << "\n";
    s << "specular: arVector3" << self->specular << "\n";
    s << "emissive: arVector3" << self->emissive << "\n";
    s << "exponent: " << self->exponent << "\n";
    s << "alpha:    " << self->alpha << "\n";
    return s.str();
  }
}
};

class arLight{
 public:
  arLight();
  ~arLight();

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

%extend{
  string __repr__(void){
    stringstream s;
    s << "arLight\n";
    s << "position: arVector4" << self->position << "\n";
    s << "diffuse:  arVector3" << self->diffuse << "\n";
    s << "ambient:  arVector3" << self->ambient << "\n";
    s << "specular: arVector3" << self->specular << "\n";
    s << "constant attenuation:  " << self->constantAttenuation << "\n";
    s << "linear attenuation:    " << self->linearAttenuation << "\n";
    s << "quadratic attenuation: " << self->quadraticAttenuation << "\n";
    s << "spot direction: arVector3" << self->spotDirection << "\n";
    s << "spot cutoff:    " << self->spotCutoff << "\n";
    s << "spot exponent:  " << self->spotExponent << "\n";
    return s.str();
  }
}
};

class arLightNode:public arGraphicsNode{
 public:
  arLightNode();
  virtual ~arLightNode();

  arLight getLight();
  void setLight(arLight& light);

%pythoncode{
  def get(self):
      return self.getLight()

  def set(self, light):
      self.setLight(light)
}
};

class arMaterialNode:public arGraphicsNode{
 public:
  arMaterialNode();
  virtual ~arMaterialNode();

  arMaterial getMaterial();
  void setMaterial(const arMaterial& material);

%pythoncode{
  def get(self):
      return self.getMaterial()

  def set(self, material):
      self.setMaterial(material)
}
};

class arGraphicsArrayNode:public arGraphicsNode{
 public:
  arGraphicsArrayNode();
  virtual ~arGraphicsArrayNode();
};

class arPointsNode:public arGraphicsArrayNode{
 public:
  arPointsNode();
  virtual ~arPointsNode();

  vector<arVector3> getPoints();
  void setPoints(int number, float* points, int* IDs = NULL);

%pythoncode{
  def set(self, points, IDs=[]):
      pointList = []
      for v in points:
          pointList.append(v[0])
          pointList.append(v[1])
          pointList.append(v[2])
      if len(IDs) == 0:
          self.setPoints(len(pointList)/3, pointList)
      else:
          IDList = []
          for i in IDs:
              if len(IDList) <= len(points):
                  IDList.append(i)
          self.setPoints(len(IDList), pointList, IDList)

  def get(self):
      pointList = []
      f = self.getPoints()
      for i in range(len(f)):
        pointList.append(arVector3(f[i][0], f[i][1], f[i][2]))
      return pointList;
}
};

class arIndexNode:public arGraphicsArrayNode{
 public:
  arIndexNode();
  virtual ~arIndexNode();

  vector<int> getIndices();
  void setIndices(int number, int* indices, int* IDs = NULL);
  
%pythoncode{
  def set(self, indices, IDs=[]):
      indexList = []
      for v in indices:
          indexList.append(v)
      if len(IDs) == 0:
          self.setIndices(len(indexList), indexList)
      else:
          IDList = []
          for i in IDs:
              if len(IDList) <= len(indexList):
                  IDList.append(i)
          self.setIndices(len(IDList), indexList, IDList)

  def get(self):
      return self.getIndices()
}  
};

class arNormal3Node: public arGraphicsArrayNode{
 public:
  arNormal3Node();
  virtual ~arNormal3Node();

  vector<arVector3> getNormal3();
  void setNormal3(int number, float* normal3, int* IDs = NULL);

%pythoncode{
  def set(self, normals, IDs=[]):
      normalList = []
      for v in normals:
          normalList.append(v[0])
          normalList.append(v[1])
          normalList.append(v[2])
      if len(IDs) == 0:
          self.setNormal3(len(normalList)/3, normalList)
      else:
          IDList = []
          for i in IDs:
              if len(IDList) <= len(normals):
                  IDList.append(i)
          self.setNormal3(len(IDList), normalList, IDList)

  def get(self):
      normalList = []
      f = self.getNormal3()
      for i in range(len(f)):
        normalList.append(arVector3(f[i][0], f[i][1], f[i][2]))
      return normalList;
}
};

class arTex2Node:public arGraphicsArrayNode{
 public:
  arTex2Node();
  virtual ~arTex2Node();

  vector<arVector2> getTex2();
  void setTex2(int number, float* tex2, int* IDs = NULL);

%pythoncode{
  def set(self, tex, IDs=[]):
      texList = []
      for v in tex:
          texList.append(v[0])
          texList.append(v[1])
      if len(IDs) == 0:
          self.setTex2(len(texList)/2, texList)
      else:
          IDList = []
          for i in IDs:
              if len(IDList) <= len(tex):
                  IDList.append(i)
          self.setTex2(len(IDList), texList, IDList)

  def get(self):
      texList = []
      f = self.getTex2()
      for i in range(len(f)):
        texList.append(arVector2(f[i][0], f[i][1]))
      return texList;
}
};

class arColor4Node:public arGraphicsArrayNode{
 public:
  arColor4Node();
  virtual ~arColor4Node();

  vector<arVector4> getColor4();
  void setColor4(int number, float* color4, int* IDs = NULL);

%pythoncode{
  def set(self, color, IDs=[]):
      colorList = []
      for v in color:
          colorList.append(v[0])
          colorList.append(v[1])
          colorList.append(v[2])
          colorList.append(v[3])
      if len(IDs) == 0:
          self.setColor4(len(colorList)/4, colorList)
      else:
          IDList = []
          for i in IDs:
              if len(IDList) <= len(color):
                  IDList.append(i)
          self.setColor4(len(IDList), colorList, IDList)

  def get(self):
      colorList = []
      f = self.getColor4()
      for i in range(len(f)):
        colorList.append(arVector4(f[i][0], f[i][1], f[i][2], f[i][3]))
      return colorList;
}
};

class arTextureNode: public arGraphicsNode{
 public:
  arTextureNode();
  virtual ~arTextureNode();

  string getFileName();
  void setFileName(const string& fileName);
};

class arTransformNode: public arGraphicsNode{
 public:
  arTransformNode();
  virtual ~arTransformNode();

  arMatrix4 getTransform();
  void setTransform(const arMatrix4& transform);

%pythoncode{
  def get(self):
      return self.getTransform()

  def set(self, m):
      self.setTransform(m);
}
};

class arVisibilityNode: public arGraphicsNode{
 public:
  arVisibilityNode();
  virtual ~arVisibilityNode();

  bool getVisibility();
  void setVisibility(bool visibility);

%pythoncode{
  def get(self):
      return self.getVisibility()

  def set(self, v):
      self.setVisibility(v)
}
};

class arBillboardNode: public arGraphicsNode{
 public:
  arBillboardNode();
  virtual ~arBillboardNode();

  string getText();
  void   setText(const string& text);

%pythoncode{
  def get(self):
      return self.getText()

  def set(self, text):
      return self.setText(text)
}
};

class arDrawableNode:public arGraphicsNode{
 public:
  arDrawableNode();
  virtual ~arDrawableNode();

  int getType();
  int getNumber();
  void setDrawable(arDrawableType type, int number);
};

%{

// Each of these MUST add an extra reference to the node since Python
// doesn't know they are the same object!

arGraphicsNode* castToGraphics(arDatabaseNode* n){
  n->ref();
  return (arGraphicsNode*) n;
}

arLightNode* castToLight(arDatabaseNode* n){
  n->ref();
  return (arLightNode*) n;
}

arMaterialNode* castToMaterial(arDatabaseNode* n){
  n->ref();
  return (arMaterialNode*) n;
}

arTransformNode* castToTransform(arDatabaseNode* n){
  n->ref();
  return (arTransformNode*) n;
}

arTextureNode* castToTexture(arDatabaseNode* n){
  n->ref();
  return (arTextureNode*) n;
}  

arBoundingSphereNode* castToBoundingSphere(arDatabaseNode* n){
  n->ref();
  return (arBoundingSphereNode*) n;
}

arBillboardNode* castToBillboard(arDatabaseNode* n){
  n->ref();
  return (arBillboardNode*) n;
}

arVisibilityNode* castToVisibility(arDatabaseNode* n){
  n->ref();
  return (arVisibilityNode*) n;
}

arPointsNode* castToPoints(arDatabaseNode* n){
  n->ref();
  return (arPointsNode*) n;
}

arNormal3Node* castToNormal3(arDatabaseNode* n){
  n->ref();
  return (arNormal3Node*) n;
}

arColor4Node* castToColor4(arDatabaseNode* n){
  n->ref();
  return (arColor4Node*) n;
}

arTex2Node* castToTex2(arDatabaseNode* n){
  return (arTex2Node*) n;
}

arIndexNode* castToIndex(arDatabaseNode* n){
  n->ref();
  return (arIndexNode*) n;
}

arDrawableNode* castToDrawable(arDatabaseNode* n){
  n->ref();
  return (arDrawableNode*) n;
}

%}

/// Generic database distributed across a cluster.
class arDatabase{
 public:
  arDatabase();
  virtual ~arDatabase();

  void setDataBundlePath(const string& bundlePathName, 
                         const string& bundleSubDirectory);
  void addDataBundlePathMap(const string& bundlePathName, 
                            const string& bundlePath);

  int getNodeID(const string& name, bool fWarn=true);
  arDatabaseNode* getNodeRef(int, bool fWarn=true);
  arDatabaseNode* getNodeRef(const string&, bool fWarn=true);
  arDatabaseNode* findNodeRef(const string& name);
  arDatabaseNode* getRoot();
  arDatabaseNode* newNodeRef(arDatabaseNode* parent, const string& type,
                             const string& name="");
  arDatabaseNode* insertNodeRef(arDatabaseNode* parent,
			        arDatabaseNode* child,
			        const string& type,
			        const string& name = "");
  bool cutNode(int ID);
  bool eraseNode(int ID);
  void permuteChildren(arDatabaseNode* parent,
                       list<int>& childIDs);

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
  void ps(int maxLevel);
  void ps();
};

class arGraphicsDatabase: public arDatabase{
 public:
  arGraphicsDatabase();
  virtual ~arGraphicsDatabase();

  virtual void reset();

  void setTexturePath(const string& thePath);

  arMatrix4 accumulateTransform(int nodeID);

  void draw();
  int intersect(const arRay&);

  void activateLights();

%pythoncode{
    def new(self, node, type, name=""):
        return gcast(self.newNodeRef(node,type,name))

    def insert(self, parent, child, type, name=""):
        return gcast(self.insertNodeRef(parent, child, type, name))
    
    def get(self, name):
        return gcast(self.getNodeRef(name))

    def find(self, name):
        return gcast(self.findNodeRef(name))
}
};

class arGraphicsPeerConnection{
 public:
  arGraphicsPeerConnection();
  ~arGraphicsPeerConnection();

  string    remoteName;
  int       connectionID;

%extend{
    string __repr__() {
        return self->print();
    }
}
};

class arGraphicsPeer: public arGraphicsDatabase{
 public:
  arGraphicsPeer();
  ~arGraphicsPeer();
  
  string getName();
  void setName(const string&);

  bool init(int&,char**);
  bool start();
  void stop();

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
  
  int connectToPeer(const string& name);
  bool closeConnection(const string& name);
  bool pullSerial(const string& name, int remoteRootID, int localRootID,
                  arNodeLevel sendLevel, 
                  arNodeLevel remoteSendLevel, arNodeLevel localSendLevel);
  bool pushSerial(const string& name, int remoteRootID, int localRootID,
                  arNodeLevel sendLevel, 
                  arNodeLevel remoteSendOn, arNodeLevel localSendOn);
  bool pingPeer(const string& name);
  bool closeAllAndReset();
  bool broadcastFrameTime(int frameTime);
  bool remoteLockNode(const string& name, int nodeID);
  bool remoteLockNodeBelow(const string& name, int nodeID);
  bool remoteUnlockNode(const string& name, int nodeID);
  bool remoteUnlockNodeBelow(const string& name, int nodeID);
  bool localLockNode(const string& name, int nodeID);
  bool localUnlockNode(int nodeID);
  bool remoteFilterDataBelow(const string& peer,
                             int remoteNodeID, arNodeLevel level);
  bool mappedFilterDataBelow(int localNodeID, arNodeLevel level);
  bool localFilterDataBelow(const string& peer,
                            int localNodeID, arNodeLevel level);
  int  remoteNodeID(const string& peer, const string& nodeName);
  
  string printConnections();
  string printPeer();
%extend{
    string __repr__() {
        return self->printPeer();
    }
}
};

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
