class arObject {
 public:
  arObject();

  /// returns the number of triangles, or -1 if undefined
  virtual inline int numberOfTriangles() {return -1;}
  /// returns the number of normals, or -1 if undefined
  virtual inline int numberOfNormals() {return -1;}
  /// returns the number of vertices, or -1 if undefined
  virtual inline int numberOfVertices() {return -1;}
  /// returns the number of materials, or -1 if undefined
  virtual inline int numberOfMaterials() {return -1;}
  /// returns the number of texture coordinates, or -1 if undefined
  virtual inline int numberOfTexCoords() {return -1;}
  /// returns the number of smoothing groups in the (OBJ)arObject, or -1 if undefined
  virtual inline int numberOfSmoothingGroups() {return -1;}
  /// returns the number of groups, or -1 if undefined
  virtual inline int numberOfGroups() {return -1;}

  /// returns the name of the arObject (as specified by read/attach function)
  const string& name() { return _name; }
  /// sets and returns the new name
  /// @param newName the new name
  const string & setName(const string& newName) { return (_name = newName); }

  /// @param objectName The name of the object
  /// @param parent Where to attach the object in the scenegraph
  /// This takes a valid arObject file and attaches it to the scenegraph
  virtual void attachMesh(const string& objectName, const string& parent) = 0;
  /// automagically inserts the object name as baseName
  virtual inline void attachMesh(const string& parent) { attachMesh(parent, _name); }
  
  /// returns what kind of Object this is
  virtual inline string type(void) = 0;

  /// does this object support the basic animation commands?
  virtual bool supportsAnimation(void) = 0;
  /// number of frames in animation
  virtual inline int	numberOfFrames() { return -1; }
  /// returns current frame number, or -1 if not implemented
  virtual inline int	currentFrame() { return -1; }
  /// go to this frame in the animation
  /// \param newFrame frame to jump to
  /// if newFrame is out-of-bounds, return false and do nothing
  virtual bool	setFrame(int) {return false;}
  /// go to the next frame, or if already at end, return false
  virtual bool	nextFrame() { return false; }
  /// go to previous frame, or if already at start, return false
  virtual bool	prevFrame() { return false; }

  /// Resizes the model to fit in a sphere of radius 1
  /// Note: MUST be called before attachMesh()!
  virtual void normalizeModelSize() = 0;

  /// Returns arDatabaseNode ID where object's vertices exist, or -1 if undefined
  int vertexNodeID () { return _vertexNodeID; } 
};

/// Wrapper for OpenGL material.
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
class arOBJTriangle {
 public:
  int smoothingGroup;
  int material;
  int namedGroup;
  int vertices[3];
  int normals[3];
  int texCoords[3];
};

/// Representation of a .OBJ file.
class arOBJ : public arObject{
 public:

  arOBJ();
  ~arOBJ() {}

  // NOTE: this CANNOT be one call since the second argument should be path
  // a little annoying, but it's this way for historical reasons
  bool readOBJ(const char* fileName, const string& path="");
  // bool readOBJ(const char* fileName, string subdirectory, string path);
  // bool readOBJ(FILE* inputFile);
  // int readMaterialsFromFile(arOBJMaterial* materialArray, char* theFilename);

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
  const string& name()		{return _name;}
  const string& setName(const string& newName) {return (_name = newName);}
  bool supportsAnimation()	{return false;}

  void setTransform(const arMatrix4& transform);
  void attachMesh(const string& baseName, const string& where);
  void attachPoints(const string& baseName, const string& where);
  void attachGroup(int group, const string& base, const string& where);
  arBoundingSphere getGroupBoundingSphere(int groupID);
  arAxisAlignedBoundingBox getAxisAlignedBoundingBox(int groupID);
  float intersectGroup(int groupID, const arRay& theRay);

  void normalizeModelSize();

  /// automagically inserts the OBJ's object name as baseName
  inline void attachPoints(const string& p)      {attachPoints(_name, p);}
  inline void attachGroup(int g, const string& p){attachGroup(g, _name, p);}

};

class ar3DS : public arObject{
  public:

    ar3DS();
    ~ar3DS();

    bool read3DS(char* fileName);
    void normalizeModelSize();
    bool normalizationMatrix(arMatrix4 &theMatrix);

    void attachMesh(const string& baseName, const string& where);
    void attachMesh(const string& baseName, arGraphicsNode* parent);
};


