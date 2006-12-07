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

// Ray, for intersection testing.
class arRay {
 public:
  arRay();
  arRay(const arVector3& origin, const arVector3& direction);
  arRay(const arRay&);
  ~arRay();

  void transform(const arMatrix4&);
  // Compute intersection with sphere.
  // Return -1 if no intersection, otherwise return distance
  float intersect(float radius, const arVector3& position);
  float intersect(const arBoundingSphere& b);
  const arVector3& getOrigin() const;
  const arVector3& getDirection() const;
  arVector3 origin;
  arVector3 direction;

%extend{
  string __str__( void ){
    ostringstream s;
    s << "arRay\n";
    s << "origin: arVector3" << self->origin << "\n";
    s << "direction: arVector3" << self->direction << "\n";
    return s.str();
  }
}
};


// Bounding sphere.
class arBoundingSphere {
 public:
  arBoundingSphere();
  arBoundingSphere(const arVector3& position, float radius);
  arBoundingSphere(const arBoundingSphere&);
  ~arBoundingSphere();

  // Somewhat inaccurate. Only works if the matrix maps spheres to spheres.
  void transform(const arMatrix4& m);
  // There are three possibilities when trying to intersect two spheres.
  // 1. They do not intersect.
  // 2. They intersect but do not contain one another.
  // 3. One sphere contains the other.
  // The return value are correspondingly:
  // -1: do not intersect, do not contain one another.
  // >= 0: intersect or one sphere contains the other. Distance between centers.
  float intersect(const arBoundingSphere&) const;
  bool intersectViewFrustum(const arMatrix4&) const;

  arVector3 position;
  float     radius;
  bool      visibility;

%extend{
  arVector3 getPosition(void) {
    return self->position;
  }
  float getRadiu(void) {
    return self->radius;
  }
  string __str__( void ){
    ostringstream s;
    s << "arBoundingSphere\n";
    s << "position: arVector3" << self->position << "\n";
    s << "radius: " << self->radius << "\n";
    s << "visibility: " << self->visibility << "\n";
    return s.str();
  }
}
};


// Class for rendering in master/slave apps.
class arOBJGroupRenderer {
  public:
    arOBJGroupRenderer();
    virtual ~arOBJGroupRenderer();
/*    bool build( arOBJRenderer* renderer,*/
/*              const string& groupName,*/
/*              vector<int>& thisGroup,*/
/*              vector<arVector3>& texCoords,*/
/*              vector<arVector3>& normals,*/
/*              vector<arOBJTriangle>& triangles );*/
    string getName() const { return _name; }
    void draw();
    void clear();
    arBoundingSphere getBoundingSphere();
    arAxisAlignedBoundingBox getAxisAlignedBoundingBox();
    float getIntersection( const arRay& theRay );
};

class arOBJRenderer {
  public:
    arOBJRenderer();
    virtual ~arOBJRenderer();
    bool readOBJ(const string& fileName, const string& path="");
    bool readOBJ(const string& fileName, const string& subdirectory, const string& path);
    bool readOBJ(FILE* inputFile);
    string getName() const;
    int getNumberGroups() const;
    arOBJGroupRenderer* getGroup( unsigned int i );
    arOBJGroupRenderer* getGroup( const string& name );
    void draw();
    void clear();
    void normalizeModelSize();
    arBoundingSphere getBoundingSphere();
    arAxisAlignedBoundingBox getAxisAlignedBoundingBox();
    float getIntersection( const arRay& theRay );
};


class arOBJ : public arObject {
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

class ar3DS : public arObject {
  public:

    ar3DS();
    ~ar3DS();

    bool read3DS(const string& fileName);
    void normalizeModelSize();
    bool normalizationMatrix(arMatrix4 &theMatrix);

    bool attachMesh(const string& baseName, const string& where);
    bool attachMesh(arGraphicsNode* parent, const string& baseName="");
};

%{
#include "arHTR.h"
%}

class arHTR : public arObject {
  public:
    arHTR();
    ~arHTR();
    bool readHTR(const string& fileName, const string& path="");
    bool readHTR(const string& fileName, const string& subdirectory, const string& path);
    bool readHTR(FILE* htrFileHandle);
    bool writeToFile(const string& fileName);
    bool attachMesh(const string& objectName,
		    const string& parent);
    bool attachMesh(arGraphicsNode* parent, 
                    const string& objectName="");
    bool attachMesh(const string& baseName, 
                    const string& where, 
                    bool withLines);
    string type(void);
    void normalizeModelSize(void);
    void basicDataSmoothing();
    bool supportsAnimation(void);
    bool setFrame(int newFrame);
    bool nextFrame();
    bool prevFrame();
    bool setBasePosition();
    int	numberOfFrames();
    int currentFrame();
    int	numberOfSegments();
    int	version();
    string nameOfSegment(int i);
    arTransformNode* transformForSegment(int i);
    arTransformNode* preTransformForSegment(int i);
    arTransformNode* postTransformForSegment(int i);
    arTransformNode* localTransformForSegment(int i);
    arTransformNode* inverseForSegment(int i);
    arBoundingSphereNode* boundingSphereForSegment(int i);
    int transformIDForSegment(int i);
    int preTransformIDForSegment(int i);
    int postTransformIDForSegment(int i);
    int localTransformIDForSegment(int i);
    int inverseIDForSegment(int i);
    int boundingSphereIDForSegment(int i);
    arMatrix4 segmentBaseTransformRelative(int segmentID);
    int	numberOfSegment(const string& segmentName);
    arMatrix4 inverseTransformForSegment(int i);
};

%{
#include "arObjectUtilities.h"
%}

bool ar_mergeOBJandHTR(arOBJ* theOBJ, arHTR* theHTR, const string& where);
bool ar_mergeOBJandHTR(arGraphicsNode* parent, arOBJ* theOBJ, arHTR *theHTR, const string& objectName="");
arObject* ar_readObjectFromFile(const string& fileName, const string& path);
