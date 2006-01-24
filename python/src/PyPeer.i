// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

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

/// Codes for different database types.
enum{ 
  AR_GENERIC_DATABASE = 0,
  AR_GRAPHICS_DATABASE = 1,
  AR_SOUND_DATABASE = 2
};

enum arNodeLevel{ AR_IGNORE_NODE = -1,
                  AR_STRUCTURE_NODE = 0, 
                  AR_STABLE_NODE = 1,
                  AR_OPTIONAL_NODE = 2,
                  AR_TRANSIENT_NODE = 3 };

/// Generic database distributed across a cluster.
class arDatabase{
 public:
  arDatabase();
  virtual ~arDatabase();

  int getTypeCode(){ return _typeCode; }
  string getTypeString(){ return _typeString; }

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
  bool cutNode(arDatabaseNode* node);
  bool cutNode(int ID);
  bool eraseNode(arDatabaseNode* node);
  bool eraseNode(int ID);
  void permuteChildren(arDatabaseNode* parent,
                       int number, int* children);
                       

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
%pythoncode{
  def permute(self, parent, children):
      c = []
      for i in children:
          c.append(i.getID())
      if len(c) > 0:
          self.permuteChildren(parent, len(c), c)
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
arViewerNode* castToViewer(arDatabaseNode* n);
arBlendNode* castToBlend(arDatabaseNode* n);
arBillboardNode* castToBillboard(arDatabaseNode* n);
arVisibilityNode* castToVisibility(arDatabaseNode* n);
arPointsNode* castToPoints(arDatabaseNode* n);
arNormal3Node* castToNormal3(arDatabaseNode* n);
arColor4Node* castToColor4(arDatabaseNode* n);
arTex2Node* castToTex2(arDatabaseNode* n);
arIndexNode* castToIndex(arDatabaseNode* n);
arDrawableNode* castToDrawable(arDatabaseNode* n);
arGraphicsStateNode* castToGraphicsState(arDatabaseNode* n);

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
    elif label == 'viewer':
        return castToViewer(n)
    elif label == 'blend':
        return castToBlend(n)
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
    elif label == 'graphics state':
        return castToGraphicsState(n)
    # DO NOT RAISE AN EXCEPTION HERE!
    # NOT ALL VALID NODES IN A GRAPHICS DATABASE ARE GRAPHICS-Y, such as
    # name nodes and root nodes...
    else:
        return n

def nodecast(n):
    if n == None:
        return None
    elif not n.getOwner():
        return n
    elif n.getOwner().getTypeCode() == AR_GRAPHICS_DATABASE:
        return gcast(n)
    else:
        return n

}

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

  void permuteChildren(int number, int* children);

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
      return nodecast(self.newNodeRef(type, name))

  def parent(self):
      return nodecast(self.getParentRef()) 

  def children(self):
      wl = NodeListWrapper(self.getChildrenRef())
      l = []
      while wl.get() != None:
          l.append(nodecast(wl.get()))
          wl.next()
      return l

  def permute(self, children):
      c = []
      for i in children:
          c.append(i.getID())
      if len(c) > 0:
          self.permuteChildren(len(c), c)

  def find(self, name):
      return nodecast(self.findNodeRef(name))

  def findByType(self, type):
      return nodecast(self.findNodeByTypeRef(type))
}
};

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
  arRay(const arVector3& o, const arVector3& d);
  arRay(const arRay&);
  ~arRay();

  void transform(const arMatrix4&);
  // Intersection with sphere.
  float intersect(float radius, const arVector3& position);
  float intersect(const arBoundingSphere& b);
  const arVector3& getOrigin() const;
  const arVector3& getDirection() const;

  arVector3 origin;
  arVector3 direction;

%extend{
  string __repr__( void ){
    ostringstream s;
    s << "arRay\n";
    s << "origin: arVector3" << self->origin << "\n";
    s << "direction: arVector3" << self->direction << "\n";
    return s.str();
  }
}
};

class arBoundingSphere{
 public:
  arBoundingSphere();
  arBoundingSphere(const arVector3& position, float radius);
  arBoundingSphere(const arBoundingSphere&);
  ~arBoundingSphere();
  void transform(const arMatrix4& m);
  float intersect(const arBoundingSphere& b);
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
  arMaterial(const arMaterial&);
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
  arLight(const arLight&);
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

class arBoundingSphereNode: public arGraphicsNode {
 public:
  arBoundingSphereNode();
  virtual ~arBoundingSphereNode();

  arBoundingSphere getBoundingSphere();
  void setBoundingSphere(const arBoundingSphere& b);

%pythoncode{
  def get(self):
      return self.getBoundingSphere()

  def set(self, b):
      self.setBoundingSphere(b)
}
};

class arViewerNode: public arGraphicsNode{
 public:
  arViewerNode();
  virtual ~arViewerNode();

  arHead* getHead();
  void setHead(const arHead& head);
%pythoncode{
  def get(self):
      return self.getHead()
  
  def set(self, h):
      self.setHead(h) 
}
};

class arBlendNode:public arGraphicsNode{
 public:
  arBlendNode();
  virtual ~arBlendNode();

  float getBlend();
  void setBlend(float blendFactor);

%pythoncode{
  def get(self):
      return self.getBlend()

  def set(self, b):
      return self.setBlend(b)
}
};

class arGraphicsStateNode: public arGraphicsNode{
 public:
  arGraphicsStateNode();
  virtual ~arGraphicsStateNode();

  string getStateName();
  arGraphicsStateID getStateID();
  bool isFloatState();
  bool isFloatState(const string& stateName);
  arGraphicsStateValue getStateValueInt(int i);
  string getStateValueString(int i);
  float getStateValueFloat();
  bool setGraphicsStateInt(const string& stateName,
                           arGraphicsStateValue value1, 
                           arGraphicsStateValue value2 = AR_G_FALSE);
  bool setGraphicsStateString(const string& stateName,
                              const string& value1,
			      const string& value2 = "false");
  bool setGraphicsStateFloat(const string& stateName,
                             float stateValueFloat);
%pythoncode{
  def get(self):
    n = self.getStateName()
    if self.isFloatState():
        return (n,self.getStateValueFloat())
    v1 = self.getStateValueString(0)
    v2 = self.getStateValueString(1)
    return (n,v1,v2)

  def set(self, s):
    if len(s) < 2:
        print('Error: must have at least 2 elements.')
        return 0
    if self.isFloatState(s[0]) and len(s) >= 2:
        return self.setGraphicsStateFloat(s[0],s[1])
    if self.isFloatState(s[0]) == 0 and len(s) == 2:
        return self.setGraphicsStateString(s[0],s[1])
    if self.isFloatState(s[0]) == 0 and len(s) == 3:
        return self.setGraphicsStateString(s[0],s[1],s[2])
    return 0
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
  void setFileName(const string& fileName, int alpha=-1);

%pythoncode{
  def get(self):
      return self.getFileName()

  def set(self, m, alpha=-1):
      self.setFileName(m, alpha);
}
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
  string getTypeAsString();
  int getNumber();
  void setDrawable(arDrawableType type, int number);
  void setDrawableViaString(const string& type, int number);

%pythoncode{
  def get(self):
    return (self.getTypeAsString(), self.getNumber())

  def set(self, d):
    self.setDrawableViaString(d[0],d[1])
}
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

arViewerNode* castToViewer(arDatabaseNode* n){
  n->ref();
  return (arViewerNode*) n;
}

arBlendNode* castToBlend(arDatabaseNode* n){
  n->ref();
  return (arBlendNode*) n;
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

arGraphicsStateNode* castToGraphicsState(arDatabaseNode* n){
  n->ref();
  return (arGraphicsStateNode*) n;
}

%}

class arGraphicsDatabase: public arDatabase{
 public:
  arGraphicsDatabase();
  virtual ~arGraphicsDatabase();
  virtual void reset();
  arTexFont* getTexFont();
  void setTexturePath(const string& thePath);
  arMatrix4 accumulateTransform(int nodeID);
  void draw();
  int intersect(const arRay&);
  list<arDatabaseNode*> arGraphicsDatabase::intersectRef(const arBoundingSphere& b);
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
		
    def intersectSphere(self, b):
      wl = NodeListWrapper(self.intersectRef(b))
      l = []
      while wl.get() != None:
          l.append(nodecast(wl.get()))
          wl.next()
      return l
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

// ********************** based on arMesh.h ****************************

class arMesh {
 public:
  arMesh(const arMatrix4& transform = ar_identityMatrix());
  virtual ~arMesh();
  void setTransform(const arMatrix4& matrix);
  arMatrix4 getTransform();
  virtual bool attachMesh(const string& name, const string& parentName);
  virtual bool attachMesh(arGraphicsNode* node, const string& name)=0;
  virtual bool attachMesh(arGraphicsNode* node)=0;

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arMesh, 'this',
            getSwigModuleDll().new_arMesh(t))
        _swig_setattr(self, arMesh, 'thisown', 1)
}
};

/// Cube, made of 12 triangles.

class arCubeMesh : public arMesh {
 public:
  arCubeMesh();
  arCubeMesh(const arMatrix4& transform);
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arCubeMesh, 'this',
            getSwigModuleDll().new_arCubeMesh(t))
        _swig_setattr(self, arCubeMesh, 'thisown', 1)
}
};

/// Rectangle (to apply a texture to).

class arRectangleMesh : public arMesh {
 public:
  arRectangleMesh();
  arRectangleMesh(const arMatrix4& transform);
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arRectangleMesh, 'this',
            getSwigModuleDll().new_arRectangleMesh(t))
        _swig_setattr(self, arRectangleMesh, 'thisown', 1)
}
};

/// Cylinder (technically a prism).

class arCylinderMesh : public arMesh {
 public:
  arCylinderMesh();
  arCylinderMesh(const arMatrix4&);

  void setAttributes(int numberDivisions, float bottomRadius, float topRadius);
  int getNumberDivisions();
  float getBottomRadius();
  float getTopRadius();
  void toggleEnds(bool);
  bool getUseEnds();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self.getTransform(),self.getNumberDivisions(),
                self.getBottomRadius(),self.getTopRadius(),
                self.getUseEnds()]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,L):
        _swig_setattr(self, arCylinderMesh, 'this',
            getSwigModuleDll().new_arCylinderMesh(L[0]))
        _swig_setattr(self, arCylinderMesh, 'thisown', 1)
        self.setAttributes(L[1],L[2],L[3])
        self.toggleEnds(L[4])
}
};

/// Pyramid.

class arPyramidMesh : public arMesh {
 public:
  arPyramidMesh();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return self.getTransform()

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,t):
        _swig_setattr(self, arPyramidMesh, 'this',
            getSwigModuleDll().new_arPyramidMesh())
        _swig_setattr(self, arPyramidMesh, 'thisown', 1)
        self.setTransform(t)
}
};

/// Sphere.

class arSphereMesh : public arMesh {
 public:
  arSphereMesh(int numberDivisions=10);
  arSphereMesh(const arMatrix4&, int numberDivisions=10);

  void setAttributes(int numberDivisions);
  int getNumberDivisions();
  void setSectionSkip(int skip);
  int getSectionSkip();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self.getTransform(),self.getNumberDivisions(),
                self.getSectionSkip()]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,L):
        _swig_setattr(self, arSphereMesh, 'this',
            getSwigModuleDll().new_arSphereMesh(L[0],L[1]))
        _swig_setattr(self, arSphereMesh, 'thisown', 1)
        self.setSectionSkip(L[2])
}
};

/// Torus (donut).

class arTorusMesh : public arMesh {
 public:
  arTorusMesh(int,int,float,float);
  ~arTorusMesh();
 
  void reset(int,int,float,float);
  int getNumberBigAroundQuads();
  int getNumberSmallAroundQuads();
  float getBigRadius();
  float getSmallRadius();
  bool attachMesh(const string& name, const string& parentName);
  bool attachMesh(arGraphicsNode* parent, const string& name);
  bool attachMesh(arGraphicsNode* parent);

%pythoncode {
    # __getstate__ returns the contents of self in a format that can be
    # pickled, a list of floats in this case
    def __getstate__(self):
        return [self.getTransform(),self.getNumberBigAroundQuads(),
                self.getNumberSmallAroundQuads(),self.getBigRadius(),
                self.getSmallRadius()]

    # __setstate__ recreates an object from its pickled state
    def __setstate__(self,L):
        _swig_setattr(self, arTorusMesh, 'this',
            getSwigModuleDll().new_arTorusMesh(L[1],L[2],L[3],L[4]))
        _swig_setattr(self, arTorusMesh, 'thisown', 1)
        self.setTransform(L[0])
}
};

/////////////////////////////////////////////////////////////////////////
// Some small utility classes follow ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

%{
class arEditorRenderCallback: public arGUIRenderCallback{
 public:
  arEditorRenderCallback(){ database = NULL; camera = NULL; }
  ~arEditorRenderCallback(){}

  virtual void operator()( arGraphicsWindow&, arViewport& ){
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (camera){
      camera->loadViewMatrices();
    }
    if (database){
      database->activateLights();
      database->draw();
    }
  }

  virtual void operator()( arGUIWindowInfo* windowInfo,
                           arGraphicsWindow* graphicsWindow ){
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (camera){
      camera->loadViewMatrices();
    }
    if (database){
      database->activateLights();
      database->draw();
    }
  }

  virtual void operator()( arGUIWindowInfo* windowInfo ){
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (camera){
      camera->loadViewMatrices();
    }
    if (database){
      database->activateLights();
      database->draw();
    }
  }

  arGraphicsDatabase* database;
  arCamera* camera;
};
%}

class arEditorRenderCallback: public arGUIRenderCallback{
 public:
  arEditorRenderCallback();
  ~arEditorRenderCallback();
  virtual void operator()( arGraphicsWindow&, arViewport& );
  virtual void operator()( arGUIWindowInfo* windowInfo,
                           arGraphicsWindow* graphicsWindow );
  virtual void operator()( arGUIWindowInfo* windowInfo );

  arGraphicsDatabase* database;
  arCamera* camera;
};

%pythoncode{
class ezview:
    def __init__(self):
        self.m = arGUIWindowManager(None, None, None, None, 1)
        self.c = arGUIWindowConfig()
        self.c.setSize(400, 400)
        self.id = self.m.addWindow(self.c)
        self.e = arEditorRenderCallback()
        self.e.database = None
        self.e.camera = None
        self.m.registerDrawCallback(self.id, self.e)
        self.m.swapAllWindowBuffers(1)

    def __del__(self):
        self.m.deleteWindow(self.id)

    def draw(self, database, camera):
        self.m.processWindowEvents()
        if self.m.hasActiveWindows() == 0:
            self.id = self.m.addWindow(self.c)
        self.e.database = database
        self.e.camera = camera
        self.m.drawAllWindows(1)
        self.m.swapAllWindowBuffers(1)
        self.e.database = None
        self.e.camera = None
  
}


