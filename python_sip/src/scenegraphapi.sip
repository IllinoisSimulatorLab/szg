// Defines mapping from python to C++ strings.
%Include typemaps.sip

%Include database.sip
%Include view.sip

// node type IDs (as distinct from record IDs in the graphics language)
enum {
  AR_G_TRANSFORM_NODE = 0,
  AR_G_TEXTURE_NODE = 1,
  AR_G_BOUNDING_SPHERE_NODE = 2,
  AR_G_BILLBOARD_NODE = 3,
  AR_G_VISIBILITY_NODE = 4,
  AR_G_VIEWER_NODE = 5,
  AR_G_BLEND_NODE = 6,
  AR_G_LIGHT_NODE = 7,
  AR_G_MATERIAL_NODE = 8,
  AR_G_PERSP_CAMERA_NODE = 9,
  AR_G_POINTS_NODE = 10,
  AR_G_NORMAL3_NODE = 11,
  AR_G_COLOR4_NODE = 12,
  AR_G_TEX2_NODE = 13,
  AR_G_INDEX_NODE = 14,
  AR_G_DRAWABLE_NODE = 15,
  AR_G_BUMP_MAP_NODE = 16,
  AR_G_GRAPHICS_STATE_NODE = 17,
  AR_G_GRAPHICS_PLUGIN_NODE = 18
};

enum arGraphicsStateID {
  AR_G_GARBAGE_STATE = 0,
  AR_G_POINT_SIZE = 1,
  AR_G_LINE_WIDTH = 2,
  AR_G_SHADE_MODEL = 3,
  AR_G_LIGHTING = 4,
  AR_G_BLEND = 5,
  AR_G_DEPTH_TEST = 6,
  AR_G_BLEND_FUNC = 7
};

enum arGraphicsStateValue {
  AR_G_FALSE= 0,
  AR_G_TRUE = 1,
  AR_G_SMOOTH = 2,
  AR_G_FLAT = 3,
  AR_G_ZERO = 4,
  AR_G_ONE = 5,
  AR_G_DST_COLOR = 6,
  AR_G_SRC_COLOR = 7,
  AR_G_ONE_MINUS_DST_COLOR = 8,
  AR_G_ONE_MINUS_SRC_COLOR = 9,
  AR_G_SRC_ALPHA = 10,
  AR_G_ONE_MINUS_SRC_ALPHA = 11,
  AR_G_DST_ALPHA = 12,
  AR_G_ONE_MINUS_DST_ALPHA = 13,
  AR_G_SRC_ALPHA_SATURATE = 14
};



class arGraphicsDatabase: arDatabase {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arGraphicsDatabase();
  virtual ~arGraphicsDatabase();

  // Default should be false (i.e. we do not need an extra reference
  // tacked on to the indicated node in the case of node creation).
//  virtual arDatabaseNode* alter(arStructuredData*, bool refNode=false);
  virtual void reset();

  void loadAlphabet(const string& path);
  arTexFont* getTexFont();
  void setTexturePath(const string& thePath);
  arTexture* addTexture(const string& name, int* theAlpha /Out/);
  arBumpMap* addBumpMap(const string& name, 
                        vector<arVector3>& points,
                        vector<int>& indices,
                        vector<arVector2>& texCoords,
                        float height, arTexture* decalTexture /TransferBack/);

  arMatrix4 accumulateTransform(int nodeID);
  arMatrix4 accumulateTransform(int startNodeID, int endNodeID);

  void setVRCameraID(int cameraID);

  void draw( arGraphicsWindow& win, arViewport& view );
  void draw(arMatrix4* projectionMatrix = NULL);
  int intersect(const arRay&);
  arDatabaseNodeList intersect(const arBoundingSphere& b, bool addRef=false);
  arDatabaseNodeList intersectRef(const arBoundingSphere& b);
//  list<int> intersectList(const arRay&);
  arGraphicsNode* intersectGeometry(const arRay& theRay, int excludeBelow = -1);
%MethodCode
    sipRes = sipCpp->intersectGeometry( *a0, a1 );
    sipRes->ref();  // ??
%End

  bool registerLight(arGraphicsNode* node, arLight* theLight);
  bool removeLight(arGraphicsNode* node);
  void activateLights();

  arHead* getHead();
  bool registerCamera(arGraphicsNode* node, arPerspectiveCamera* theCamera);
  bool removeCamera(arGraphicsNode* node);
  // Normally we use the default "VR camera"... however, the database can
  // also be drawn from the perspective of one of the attached camera
  arPerspectiveCamera* getCamera( unsigned int cameraID );
};


class arGraphicsNode: arDatabaseNode {
%TypeHeaderCode
#include "arGraphicsNode.h"
%End
 public:
  arGraphicsNode();
  virtual ~arGraphicsNode();

  // From arDatabaseNode
  //virtual void initialize(arDatabase*);
  //virtual bool receiveData(arStructuredData*){ return false; };
  //virtual arStructuredData* dumpData(){ return NULL; };
    // Caller deletes returned value.
    // Subclasses' _dumpData is usually not thread-safe.

  // unique to arGraphicsNode
  arMatrix4 accumulateTransform() /TransferBack/;
  //virtual void draw(arGraphicsContext*){};
  //inline ARfloat* getBuffer(){ return _commandBuffer.v; }
  int getBufferSize() const;
 private:
  arGraphicsNode( const arGraphicsNode& );
};


//void dgSetGraphicsDatabase(arGraphicsDatabase*);

string dgGetNodeName(int);

arGraphicsNode* dgGetNode(const string& nodeName);

arDatabaseNode* dgMakeNode(const string&, const string&, 
                                    const string&);

int  dgViewer( const string& parent, const arHead& head);
bool dgViewer( int ID, const arHead& head );

int dgTransform(const string&, const string&, const arMatrix4&);
bool dgTransform(int, const arMatrix4&);

int dgTexture(const string& name, const string& parent,
                       const string& filename, int alphaValue=-1);
bool dgTexture(int, const string& filename, int alphaValue=-1);

//int dgTexture(const string& name, const string& parent,
//                       bool alpha, int w, int h, const char* pixels);
//bool dgTexture(int ID, bool alpha, int w, int h, const char* pixels);

int  dgBoundingSphere(const string&, const string&, int, 
                      float, const arVector3&);
bool dgBoundingSphere(int, int, float, const arVector3&);

bool dgErase(const string&);

int dgBillboard(const string&, const string&, int, const string&);
bool dgBillboard(int, int, const string&);

int dgVisibility(const string&, const string&, int);
bool dgVisibility(int, int);

int dgBlend(const string&, const string&, float);
bool dgBlend(int, float);

int dgStateInt(const string& nodeName, const string& parentName,
                        const string& stateName,
                        arGraphicsStateValue val1,
                        arGraphicsStateValue val2 = AR_G_FALSE );
bool dgStateInt(int nodeID, const string& stateName,
    arGraphicsStateValue val1, arGraphicsStateValue val2 = AR_G_FALSE);

int dgStateFloat(const string& nodeName, const string& parentName,
                          const string& stateName, float value );
bool dgStateFloat(int nodeID, const string& stateName, float value);

int dgPoints(const string& name, const string& parent, vector<int>& ids, vector<arVector3>& positions);
bool dgPoints(int ID, vector<int>& IDs, vector<arVector3>& positions);
int dgPoints(const string& name, const string& parent, vector<arVector3>&);
bool dgPoints(int ID, vector<arVector3>&);

int dgNormal3(const string& name, const string& parent, vector<int>& IDs, vector<arVector3>& normals);
bool dgNormal3(int ID, vector<int>& IDs, vector<arVector3>& normals);
int dgNormal3(const string& name, const string& parent, vector<arVector3>& normals);
bool dgNormal3(int ID, vector<arVector3>& normals);

int dgColor4(const string& name, const string& parent, vector<int>& IDs, vector<arVector4>& colors);
bool dgColor4(int ID, vector<int>& IDs, vector<arVector4>& colors);
int dgColor4(const string& name, const string& parent, vector<arVector4>& colors);
bool dgColor4(int ID, vector<arVector4>& colors);

int dgTex2(const string& name, const string& parent, vector<int>& IDs, vector<arVector2>& coords);
bool dgTex2(int ID, vector<int>& IDs, vector<arVector2>& coords);
int dgTex2(const string& name, const string& parent, vector<arVector2>& coords);
bool dgTex2(int ID, vector<arVector2>& coords);

int dgIndex(const string& name, const string& parent, vector<int>& IDs, vector<int>& indices);
bool dgIndex(int ID, vector<int>& IDs, vector<int>& indices);
int dgIndex(const string& name, const string& parent, vector<int>& indices);
bool dgIndex(int ID, vector<int>& indices);


int dgDrawable(const string& name, const string& parent,
	                int drawableType, int numPrimitives);
bool dgDrawable(int ID, int drawableType, int numPrimitives);


int dgMaterial(const string& name, const string& parent, 
                  const arVector3& diffuse,
	                const arVector3& ambient = arVector3(0.2,0.2,0.2), 
                  const arVector3& specular = arVector3(0,0,0), 
                  const arVector3& emissive = arVector3(0,0,0),
	                float exponent = 0., 
                  float alpha = 1.);
bool dgMaterial(int ID, const arVector3& diffuse,
	                 const arVector3& ambient = arVector3(0.2,0.2,0.2), 
                   const arVector3& specular = arVector3(0,0,0), 
                   const arVector3& emissive = arVector3(0,0,0),
	                 float exponent = 0., 
                   float alpha = 1.);


int dgLight(const string& name, const string& parent,
	             int lightID, arVector4 position,
	             const arVector3& diffuse,
	             const arVector3& ambient = arVector3(0,0,0),
	             const arVector3& specular = arVector3(1,1,1),
	             const arVector3& attenuate = arVector3(1,0,0),
               const arVector3& spotDiection = arVector3(0,0,-1),
	             float spotCutoff = 180.,
	             float spotExponent = 0.);

bool dgLight(int ID,
	              int lightID, arVector4 position,
	              const arVector3& diffuse,
	              const arVector3& ambient = arVector3(0,0,0),
	              const arVector3& specular = arVector3(1,1,1),
	              const arVector3& attenuate = arVector3(1,0,0),
                const arVector3& spotDiection = arVector3(0,0,-1),
	              float spotCutoff = 180.,
	              float spotExponent = 0.);


// Attach a perspective camera to the scene graph.
int dgCamera(const string& name, const string& parent,
	              int cameraID, float leftClip, float rightClip, 
	              float bottomClip, float topClip, 
                float nearClip, float farClip,
                const arVector3& eyePosition = arVector3(0,0,0),
                const arVector3& centerPosition = arVector3(0,0,-1),
                const arVector3& upDirection = arVector3(0,1,0));

bool dgCamera(int ID,
	               int cameraID, float leftClip, float rightClip, 
	               float bottomClip, float topClip, 
                 float nearClip, float farClip,
                 const arVector3& eyePosition = arVector3(0,0,0),
                 const arVector3& centerPosition = arVector3(0,0,-1),
                 const arVector3& upDirection = arVector3(0,1,0));


// Attach a bump map node to the scene graph
int dgBumpMap(const string& name, const string& parent,
	               const string& filename, float height=1.);

bool dgBumpMap(int ID, const string& filename, float height=1.);


//SZG_CALL int dgPlugin(const string& name, const string& parent, const string& fileName,
//               int* intData, int numInts,
//               float* floatData, int numFloats,
//               long* longData, int numLongs,
//               double* doubleData, int numDoubles,
//               std::vector< std::string >* stringData );

//SZG_CALL bool dgPlugin(int ID, const string& fileName,
//               int* intData, int numInts,
//               float* floatData, int numFloats,
//               long* longData, int numLongs,
//               double* doubleData, int numDoubles,
//               std::vector< std::string >* stringData );

//SZG_CALL int dgPlugin(const string& name,
//              const string& parent,
//              const string& fileName,
//               std::vector<int>& intData,
//               std::vector<float>& floatData,
//               std::vector<long>& longData,
//               std::vector<double>& doubleData,
//               std::vector< std::string >& stringData );

//SZG_CALL bool dgPlugin( int ID, const string& fileName,
//               std::vector<int>& intData,
//               std::vector<float>& floatData,
//               std::vector<long>& longData,
//               std::vector<double>& doubleData,
//               std::vector< std::string >& stringData );


//int SZG_CALL dgPython(const string& name,
//              const string& parent,
//              const string& moduleName,
//              const string& factoryName,
//              bool reloadModule,
//               int* intData, int numInts,
//               float* floatData, int numFloats,
//               long* longData, int numLongs,
//               double* doubleData, int numDoubles,
//               std::vector< std::string >* stringData );

//bool SZG_CALL dgPython( int ID, const string& moduleName,
//               const string& factoryName,
//               bool reloadModule,
//               int* intData, int numInts,
//               float* floatData, int numFloats,
//               long* longData, int numLongs,
//               double* doubleData, int numDoubles,
//               std::vector< std::string >* stringData );

//int SZG_CALL dgPython(const string& name,
//              const string& parent,
//              const string& moduleName,
//              const string& factoryName,
//              bool reloadModule,
//               std::vector<int>& intData,
//               std::vector<float>& floatData,
//               std::vector<long>& longData,
//               std::vector<double>& doubleData,
//               std::vector< std::string >& stringData );

//bool SZG_CALL dgPython( int ID, const string& moduleName,
//               const string& factoryName,
//               bool reloadModule,
//               std::vector<int>& intData,
//               std::vector<float>& floatData,
//               std::vector<long>& longData,
//               std::vector<double>& doubleData,
//               std::vector< std::string >& stringData );

