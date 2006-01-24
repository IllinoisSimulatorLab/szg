// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).

class arObject {
 public:
  arObject();
  virtual inline int numberOfTriangles();
  virtual inline int numberOfNormals();
  virtual inline int numberOfVertices();
  virtual inline int numberOfMaterials();
  virtual inline int numberOfTexCoords();
  virtual inline int numberOfSmoothingGroups();
  virtual inline int numberOfGroups();
  string name();
  string setName(const string& name);
  virtual bool attachMesh(const string& objectName, const string& parent);
  virtual bool attachMesh(arGraphicsNode* parent, const string& objectName="")=0;
  virtual inline string type(void)=0;
  virtual bool supportsAnimation(void)=0;
  virtual inline int numberOfFrames();
  virtual inline int currentFrame();
  virtual bool	setFrame(int);
  virtual bool	nextFrame();
  virtual bool	prevFrame();
  virtual void normalizeModelSize()=0;
  int vertexNodeID();
};

class arOBJMaterial {
 public:
  arOBJMaterial() :
    Ns(60),
    Kd(arVector3(1,1,1)),
    Ks(arVector3(0,0,0)),
    Ka(arVector3(.2,.2,.2)),
    map_Kd("none"),
    map_Bump("none")
  { }
  float     illum;
  float     Ns;	
  arVector3 Kd;	
  arVector3 Ks;	
  arVector3 Ka;	
  char	    name[32];
  string    map_Kd;
  string    map_Bump;	
};

class arOBJTriangle {
 public:
  int smoothingGroup;
  int material;
  int namedGroup;
  int vertices[3];
  int normals[3];
  int texCoords[3];
};

class arOBJ : public arObject{
 public:

  arOBJ();
  ~arOBJ();
  bool readOBJ(const string& fileName, const string& path="");
  string type();
  int numberOfTriangles();
  int numberOfNormals();
  int numberOfVertices();
  int numberOfMaterials();
  int numberOfTexCoords();
  int numberOfSmoothingGroups();
  int numberOfGroups();
  int numberInGroup(int i);
  string nameOfGroup(int i);
  int groupID(const string& name);
  string name();
  string setName(const string& newName);
  bool supportsAnimation();
  void setTransform(const arMatrix4& transform);
  bool attachMesh(const string& baseName, const string& where);
  bool attachMesh(arGraphicsNode* parent, const string& objectName="");
  arGraphicsNode* attachPoints(arGraphicsNode* where, const string& nodeName);
  bool attachGroup(arGraphicsNode* where, int group, const string& base);
  arBoundingSphere getGroupBoundingSphere(int groupID);
  arAxisAlignedBoundingBox getAxisAlignedBoundingBox(int groupID);
  float intersectGroup(int groupID, const arRay& theRay);
  void normalizeModelSize();
};

class ar3DS : public arObject{
  public:

    ar3DS();
    ~ar3DS();

    bool read3DS(const string& fileName);
    void normalizeModelSize();
    bool normalizationMatrix(arMatrix4 &theMatrix);

    bool attachMesh(const string& baseName, const string& where);
    bool attachMesh(arGraphicsNode* parent, const string& baseName="");
};


