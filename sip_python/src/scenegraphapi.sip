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

//int  dgPoints(const string& name, const string& parent, 
//              int num, int* IDs, float* coords){
int  dgPoints(const string& name, const string& parent, 
              arIntArray, arFloatArray );
%MethodCode
  if (3*(a2->size) != a3->size) {
    PyErr_SetString( PyExc_ValueError, "dgPoints() coords list must have 3 items for each item in IDs" );
    sipIsErr = 1;
  } else {
    sipRes = dgPoints( *a0, *a1, a2->size, a2->data, a3->data );
  }
%End

//bool dgPoints(int, int, int*, float*);
int  dgPoints( int nodeID, arIntArray, arFloatArray );
%MethodCode
  if (3*(a1->size) != a2->size) {
    PyErr_SetString( PyExc_ValueError, "dgPoints() coords list must have 3 items for each item in IDs" );
    sipIsErr = 1;
  } else {
    sipRes = dgPoints( a0, a1->size, a1->data, a2->data );
  }
%End

//int dgPoints(const string& name, const string& parent, int numPoints, float* positions);
int dgPoints(const string& name, const string& parent, arFloatArray positions );
%MethodCode
  sipRes = dgPoints( *a0, *a1, (a2->size)/3, a2->data );
%End

//bool dgPoints(int ID, int numPoints, float* positions);
bool dgPoints(int ID, arFloatArray positions);
%MethodCode
  sipRes = dgPoints( a0, (a1->size)/3, a1->data );
%End

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

//int dgNormal3(const string& name, const string& parent, 
//                       int numNormals, int* IDs, float* normals);
int dgNormal3(const string& name, const string& parent, 
                       arIntArray IDs, arFloatArray coords );
%MethodCode
  if (3*(a2->size) != a3->size) {
    PyErr_SetString( PyExc_ValueError, "dgNormal3() coords list must have 3 items for each item in IDs" );
    sipIsErr = 1;
  } else {
    sipRes = dgNormal3( *a0, *a1, a2->size, a2->data, a3->data );
  }
%End

//bool dgNormal3(int ID, int numNormals, int* IDs, float* normals);
bool dgNormal3(int ID, arIntArray IDs, arFloatArray coords);
%MethodCode
  if (3*(a1->size) != a2->size) {
    PyErr_SetString( PyExc_ValueError, "dgNormal3() coords list must have 3 items for each item in IDs" );
    sipIsErr = 1;
  } else {
    sipRes = dgNormal3( a0, a1->size, a1->data, a2->data );
  }
%End


//int dgNormal3(const string& name, const string& parent, int numNormals, float* normals);
int dgNormal3(const string& name, const string& parent, arFloatArray normals);
%MethodCode
  sipRes = dgNormal3( *a0, *a1, (a2->size)/3, a2->data );
%End

//bool dgNormal3(int ID, int numNormals, float* normals);
bool dgNormal3(int ID, arFloatArray normals);
%MethodCode
  sipRes = dgNormal3( a0, (a1->size)/3, a1->data );
%End

int dgColor4(const string& name, const string& parent, int numColors,
	              int* IDs, float* colors);
bool dgColor4(int ID, int numColors, int* IDs, float* colors);
int dgColor4(const string& name, const string& parent, int numColors,
	              float* colors);
bool dgColor4(int ID, int numColors, float* colors);

int dgTex2(const string& name, const string& parent, int numTexcoords,
	            int* IDs, float* coords);
bool dgTex2(int ID, int numTexcoords, int* IDs, float* coords);
int dgTex2(const string& name, const string& parent, int numTexcoords,
	            float* coords);
bool dgTex2(int ID, int numTexcoords, float* coords);

int dgIndex(const string& name, const string& parent, int numIndices,
	             int* IDs, int* indices);
bool dgIndex(int ID, int numIndices, int* IDs, int* indices);
int dgIndex(const string& name, const string& parent, int numIndices,
	             int* indices);
bool dgIndex(int ID, int numIndices, int* indices);

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


int dgPlugin(const string& name, const string& parent, const string& fileName,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );
    

bool dgPlugin(int ID, const string& fileName,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

int dgPlugin(const string& name,
              const string& parent,
              const string& fileName,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );

bool dgPlugin( int ID, const string& fileName, 
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );


int dgPython(const string& name,
              const string& parent,
              const string& moduleName,
              const string& factoryName,
              bool reloadModule,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

bool dgPython( int ID, const string& moduleName,
               const string& factoryName,
               bool reloadModule,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

int dgPython(const string& name,
              const string& parent,
              const string& moduleName,
              const string& factoryName,
              bool reloadModule,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );

bool dgPython( int ID, const string& moduleName, 
               const string& factoryName,
               bool reloadModule,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );


