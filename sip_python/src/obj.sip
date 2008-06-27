// Defines mapping from python to C++ strings.
%Include typemaps.sip

%ModuleHeaderCode
#include <string>
using namespace std;
#include "arObjectUtilities.h"
%End


bool ar_mergeOBJandHTR(arOBJ* theOBJ, arHTR* theHTR, const string& where);
//bool ar_mergeOBJandHTR(arGraphicsNode* parent, arOBJ* theOBJ, arHTR *theHTR, const string& objectName="");
arObject* ar_readObjectFromFile(const string& fileName, const string& path);


class arObject /Abstract/ {
%TypeHeaderCode
#include "arObject.h"
%End
 public:
  arObject();
  virtual int numberOfTriangles();
  virtual int numberOfNormals();
  virtual int numberOfVertices();
  virtual int numberOfMaterials();
  virtual int numberOfTexCoords();
  virtual int numberOfSmoothingGroups();
  virtual int numberOfGroups();
  string name();
  string setName(const string& name);
  //virtual bool attachMesh(const string& objectName, const string& parent);
  //virtual bool attachMesh(arGraphicsNode* parent, const string& objectName="")=0;
  virtual string type()=0;
  virtual bool supportsAnimation()=0;
  virtual int numberOfFrames();
  virtual int currentFrame();
  virtual bool  setFrame(int);
  virtual bool  nextFrame();
  virtual bool  prevFrame();
  virtual void normalizeModelSize()=0;
  int vertexNodeID();
 private:
  arObject( const arObject& );
};

class arOBJMaterial {
%TypeHeaderCode
#include "arOBJ.h"
%End
 public:
  arOBJMaterial();
  float     illum;
  float     Ns;  
  arVector3 Kd; 
  arVector3 Ks;  
  arVector3 Ka;  
  string    map_Kd;
  string    map_Bump;
  char*     name; // 32-char array
%GetCode
  if ((sipPy = PyString_FromString(sipCpp->name)) == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "unable to convert arOBJMaterial.name");
  }
%End
%SetCode
  if (!PyString_Check(sipPy)) {
    PyErr_SetString(PyExc_TypeError, "arOBJMaterial.name must be a string");
    sipErr = 1;
  } else if (PyString_Size(sipPy) > 31) {
    PyErr_SetString(PyExc_TypeError, "arOBJMaterial.name must be < 32 characters");
    sipErr = 1;
  } else {
    memcpy( sipCpp->name, PyString_AsString(sipPy), PyString_Size(sipPy) );
  } 
%End
};


%ModuleCode
int _setIntArrayFromPyObject( PyObject* seq, int* ptr, int num ) {
  if (!PyList_Check(seq)) {
    PyErr_SetString(PyExc_TypeError, "list of Ints required");
    return 1;
  }
  if (PyList_Size(seq) > num) {
    PyErr_SetString(PyExc_TypeError, "incorrect list size");
    return 1;
  }
  for (int i=0; i<num; ++i) {
    PyObject* item = PyList_GetItem( seq, i );
    if (!PyInt_Check(item)) {
      PyErr_SetString( PyExc_TypeError, "attribute (list) items must be Ints" );
      return 1;
    }
    ptr[i] = (int)PyInt_AsLong( item );
  } 
  return 0;
}
void _setPyListFromIntArray( PyObject** seq, int* ptr, int num ) {
  *seq = PyList_New( num );
  if (*seq == NULL) {
    PyErr_SetString(PyExc_MemoryError, "unable to allocate list");
    return;
  } 
  for (int i=0; i<num; ++i) {
    PyList_SetItem( *seq, i, PyInt_FromLong( (long)ptr[i] ) );
  } 
}

%End
class arOBJTriangle {
%TypeHeaderCode
#include "arOBJ.h"
%End
 public:
  int smoothingGroup;
  int material;
  int namedGroup;
  int *vertices;
%GetCode
  _setPyListFromIntArray( &sipPy, sipCpp->vertices, 3 );
%End
%SetCode
  sipErr = _setIntArrayFromPyObject( sipPy, sipCpp->vertices, 3 );
%End

  int *normals;
%GetCode
  _setPyListFromIntArray( &sipPy, sipCpp->normals, 3 );
%End
%SetCode
  sipErr = _setIntArrayFromPyObject( sipPy, sipCpp->normals, 3 );
%End

  int *texCoords;
%GetCode
  _setPyListFromIntArray( &sipPy, sipCpp->texCoords, 3 );
%End
%SetCode
  sipErr = _setIntArrayFromPyObject( sipPy, sipCpp->texCoords, 3 );
%End
};

// Ray, for intersection testing.
class arRay {
%TypeHeaderCode
#include "arRay.h"
%End
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

  string __str__();
%MethodCode
    ostringstream s;
    s << "arRay( ";
    s << sipCpp->origin << " -> ";
    s << sipCpp->direction << " )";
    sipRes = new string(s.str());
%End
};


// Bounding sphere.
class arBoundingSphere {
%TypeHeaderCode
#include "arRay.h"
%End
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

  arVector3 getPosition() /TransferBack/;
%MethodCode
    sipRes = new arVector3(sipCpp->position);
%End

  float getRadius();
%MethodCode
    sipRes = sipCpp->radius;
%End

  string __str__();
%MethodCode
    ostringstream s;
    s << "arBoundingSphere\n";
    s << "  position: arVector3" << sipCpp->position << "\n";
    s << "  radius: " << sipCpp->radius << "\n";
    s << "  visibility: " << sipCpp->visibility << "\n";
    sipRes = new string( s.str() );
%End
};


// Class for rendering in master/slave apps.
class arOBJGroupRenderer {
%TypeHeaderCode
#include "arOBJ.h"
%End
  public:
    arOBJGroupRenderer();
    virtual ~arOBJGroupRenderer();
    bool build( arOBJRenderer* renderer,
              const string& groupName,
              vector<int>& thisGroup,
              vector<arVector3>& texCoords,
              vector<arVector3>& normals,
              vector<arOBJTriangle>& triangles );
    string getName() const;
    void draw();
    void clear();
    arBoundingSphere getBoundingSphere() /TransferBack/;
    arAxisAlignedBoundingBox getAxisAlignedBoundingBox() /TransferBack/;
    float getIntersection( const arRay& theRay );
  private:
    arOBJGroupRenderer( const arOBJGroupRenderer& );
};

class arOBJRenderer {
%TypeHeaderCode
#include "arOBJ.h"
%End
  public:
    arOBJRenderer();
    virtual ~arOBJRenderer();
    bool readOBJ(const string& fileName, const string& path="");
    bool readOBJ(const string& fileName, const string& subdirectory, const string& path);
    //bool readOBJ(FILE* inputFile);
    string getName() const;
    int getNumberGroups() const;
    arOBJGroupRenderer* getGroup( unsigned int i );
    arOBJGroupRenderer* getGroup( const string& name );
    int getNumberMaterials() const;
    arMaterial* getMaterial( unsigned int i );
    int getNumberTextures() const;
    arTexture* getTexture( unsigned int i );
    void draw();
    void clear();
    void transformVertices( const arMatrix4& matrix );
    void normalizeModelSize();
    arBoundingSphere getBoundingSphere() /TransferBack/;
    arAxisAlignedBoundingBox getAxisAlignedBoundingBox() /TransferBack/;
    float getIntersection( const arRay& theRay );
    void activateTextures();
    void mipmapTextures( bool onoff );
  private:
    arOBJRenderer( const arOBJRenderer& );
};


class arOBJ : arObject {
%TypeHeaderCode
#include "arOBJ.h"
%End
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
  //bool attachMesh(arGraphicsNode* parent, const string& objectName="");
  //arGraphicsNode* attachPoints(arGraphicsNode* where, const string& nodeName);
  //bool attachGroup(arGraphicsNode* where, int group, const string& base);
  arBoundingSphere getGroupBoundingSphere(int groupID) /TransferBack/;
  arAxisAlignedBoundingBox getAxisAlignedBoundingBox(int groupID) /TransferBack/;
  float intersectGroup(int groupID, const arRay& theRay);
  void normalizeModelSize();
 private:
  arOBJ( const arOBJ& );
};

class ar3DS : arObject {
%TypeHeaderCode
#include "ar3DS.h"
%End
  public:
    ar3DS();
    ~ar3DS();

    bool read3DS(const string& fileName);
    void normalizeModelSize();
    bool normalizationMatrix(arMatrix4 &theMatrix);

    bool attachMesh(const string& baseName, const string& where);
    //bool attachMesh(arGraphicsNode* parent, const string& baseName="");
  private:
    ar3DS( const ar3DS& );
};


class arHTR : arObject {
%TypeHeaderCode
#include "arHTR.h"
%End
  public:
    arHTR();
    ~arHTR();
    bool readHTR(const string& fileName, const string& path="");
    bool readHTR(const string& fileName, const string& subdirectory, const string& path);
    //bool readHTR(FILE* htrFileHandle);
    bool writeToFile(const string& fileName);
    bool attachMesh(const string& objectName, const string& parent);
    //bool attachMesh(arGraphicsNode* parent, const string& objectName="");
    bool attachMesh(const string& baseName, const string& where, bool withLines);
    string type();
    void normalizeModelSize();
    void basicDataSmoothing();
    bool supportsAnimation();
    bool setFrame(int newFrame);
    bool nextFrame();
    bool prevFrame();
    bool setBasePosition();
    int  numberOfFrames();
    int currentFrame();
    int  numberOfSegments();
    int  version();
    string nameOfSegment(int i);
    //arTransformNode* transformForSegment(int i);
    //arTransformNode* preTransformForSegment(int i);
    //arTransformNode* postTransformForSegment(int i);
    //arTransformNode* localTransformForSegment(int i);
    //arTransformNode* inverseForSegment(int i);
    //arBoundingSphereNode* boundingSphereForSegment(int i);
    int transformIDForSegment(int i);
    int preTransformIDForSegment(int i);
    int postTransformIDForSegment(int i);
    int localTransformIDForSegment(int i);
    int inverseIDForSegment(int i);
    int boundingSphereIDForSegment(int i);
    arMatrix4 segmentBaseTransformRelative(int segmentID) /TransferBack/;
    int  numberOfSegment(const string& segmentName);
    arMatrix4 inverseTransformForSegment(int i) /TransferBack/;
  private:
    arHTR( const arHTR& );
};


