//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_OBJ_TRANSLATOR
#define AR_OBJ_TRANSLATOR

#include <stdio.h>
#include <iostream>
#include "arMath.h"
#include "arDataUtilities.h"
#include "arGraphicsDatabase.h"
#include <string>
#include <vector>
#include "arObject.h"
#include "arOBJSmoothingGroup.h"
#include "arRay.h"
#include "arAxisAlignedBoundingBox.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arObjCalling.h"

/// Wrapper for OpenGL material.
class SZG_CALL arOBJMaterial {
 public:
  arOBJMaterial() :
    Ns(60),
    Kd(arVector3(1,1,1)),
    Ks(arVector3(0,0,0)),
    Ka(arVector3(.2,.2,.2)),
    map_Kd("none"),
    map_Bump("none")
  { }
  float     illum;	///< 0: no lighting || 1: ambient&diffuse || 2: all on
  float     Ns;		///< specular power
  arVector3 Kd;		///< diffuse color
  arVector3 Ks;		///< specular color
  arVector3 Ka;		///< ambient color
  char	    name[32];	///< name
  string    map_Kd;	///< texture map for base color
  string    map_Bump;	///< texture map for base color
};

/// Wrapper for a single triangle.
class SZG_CALL arOBJTriangle {
 public:
  int smoothingGroup;
  int material;
  int namedGroup;
  int vertices[3];
  int normals[3];
  int texCoords[3];
};

/// Representation of a .OBJ file.
class SZG_CALL arOBJ : public arObject{
 public:

  arOBJ();
  ~arOBJ() {}

  // NOTE: this CANNOT be one call since the second argument should be path
  // a little annoying, but it's this way for historical reasons
  bool readOBJ(const string& fileName, const string& path="");
  bool readOBJ(const string& fileName, const string& subdirectory, 
               const string& path);
  bool readOBJ(FILE* inputFile);
  int readMaterialsFromFile(arOBJMaterial* materialArray, char* theFilename);

  string type()			{return "OBJ";}
  int numberOfTriangles()	{return _triangle.size();}
  int numberOfNormals()		{return _normal.size();}
  int numberOfVertices()	{return _vertex.size();}
  int numberOfMaterials()	{return _material.size();}
  int numberOfTexCoords()	{return _texCoord.size();}
  int numberOfSmoothingGroups()	{return _smoothingGroup.size();}
  int numberOfGroups()		{return _group.size();}
  int numberInGroup(int i)	{return _group[i].size();}
  string nameOfGroup(int i)	{return string(_groupName[i]);} // unsafe to return a ref
  int groupID(const string& name);
  string name()		{return _name;}
  string setName(const string& newName) {return (_name = newName);}
  bool supportsAnimation()	{return false;}

  void setTransform(const arMatrix4& transform);
  bool attachMesh(const string& objectName, const string& parentName);
  bool attachMesh(arGraphicsNode* where, const string& baseName="");
  arGraphicsNode* attachPoints(arGraphicsNode* where, const string& nodeName);
  bool attachGroup(arGraphicsNode* where, int group, const string& base);
  arBoundingSphere getGroupBoundingSphere(int groupID);
  arAxisAlignedBoundingBox getAxisAlignedBoundingBox(int groupID);
  float intersectGroup(int groupID, const arRay& theRay);

  void normalizeModelSize();

 protected:
  bool _readMaterialsFromFile(FILE* matFile);
  void _parseFace(int numTokens, char *token[]);
  bool _parseOneLine(FILE* inputFile);
  void _generateNormals();

 private:
  // status/condition variables
  int  _thisMaterial;  //< the material being used
  int  _thisSG;        //< the smoothing group in use now
  int  _thisGroup;     //< current group
  
  // data
  vector<arVector3>	_vertex;	//< Vertices
  vector<arVector3>	_normal;	//< Normals
  vector<arVector3>	_texCoord;	//< Texture Coordinates
  vector<arOBJTriangle>	_triangle;	//< Triangles (polys)
  vector<arOBJMaterial> _material;	//< Materials
  vector<arOBJSmoothingGroup> _smoothingGroup;	//< Smoothing Groups
  vector<vector<int> >	_group;		//< Groups (face indices)
  vector<string>	_groupName;	//< names of groups
  arMatrix4		_transform;

  // search path and subdirectory  needed for finding .mtl files
  string _searchPath;
  string _subdirectory;
  // the file name is also needed...
  string _fileName;
  

};

#endif
