//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsAPI.h"
#include "arHead.h"

// global variables
static arGraphicsDatabase* __database = NULL;
static arGraphicsLanguage  __gfx;

void dgSetGraphicsDatabase(arGraphicsDatabase* database) {
  __database = database;
}

string dgGetNodeName(int nodeID) {
  arDatabaseNode* theNode = __database->getNode(nodeID);
  if (!theNode) {
    cerr << "dgGetNodeName error: no node with that ID.\n";
    return string("NULL");
  }
  return theNode->getName();
}

arGraphicsNode* dgGetNode(const string& nodeName) {
  return (arGraphicsNode*) __database->getNode(nodeName);
}

arDatabaseNode* dgMakeNode(const string& name,
                           const string& parent,
                           const string& type) {
  if (!__database) {
    cerr << "syzygy error: dgSetGraphicsDatabase not yet called.\n";
    return NULL;
  }

  arDatabaseNode* parentNode = __database->getNode(parent);
  if (!parentNode)
    return NULL;

  // -1 means accept an ID from the database.
  int ID = -1;
  int parentID = parentNode->getID();
  arStructuredData* data = __database->makeNodeData;
  if (!data->dataIn(__gfx.AR_MAKE_NODE_PARENT_ID, &parentID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_MAKE_NODE_ID, &ID, AR_INT, 1) ||
      !data->dataInString(__gfx.AR_MAKE_NODE_NAME, name) ||
      !data->dataInString(__gfx.AR_MAKE_NODE_TYPE, type)) {
    cerr << "dgMakeNode error: dataIn failed.\n";
    return NULL;
  }
  // Use alter not arGraphicsDatabase::alter, so the remote node will be created.
  return __database->alter(data);
}

int dgViewer( const string& parent, const arHead& head) {
  arDatabaseNode* node = dgMakeNode("szg_viewer", parent, "viewer");
  return node && dgViewer(node->getID(), head) ? node->getID() : -1;
}

// Friend of arHead, to directly access its data.
bool dgViewer( int ID, const arHead& head ) {
#if 0
  arDatabaseNode* node = __database->getNode("szg_viewer", false);
  if (!node) {
    node = dgMakeNode("szg_viewer", "root", "viewer");
  }
  const ARint ID = node->getID();
#endif
  if (ID < 0)
    return false;
  arStructuredData* data = __database->viewerData;
  if (!data->dataIn(__gfx.AR_VIEWER_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_MATRIX, head._matrix.v , AR_FLOAT, AR_FLOATS_PER_MATRIX) ||
      !data->dataIn(__gfx.AR_VIEWER_MID_EYE_OFFSET, head._midEyeOffset.v , AR_FLOAT, AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_DIRECTION, head._eyeDirection.v , AR_FLOAT, AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_SPACING, &head._eyeSpacing , AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_NEAR_CLIP, &head._nearClip , AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_FAR_CLIP, &head._farClip , AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_FIXED_HEAD_MODE, &head._fixedHeadMode , AR_INT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_UNIT_CONVERSION, &head._unitConversion , AR_FLOAT, 1)) {
    cerr << "dgViewer error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

#if 0
bool dgViewer(const arMatrix4& headMatrix, const arVector3& midEyeOffset,
	     const arVector3& eyeDirection, float eyeSpacing,
	     float nearClip, float farClip, float unitConversion,
             bool fixedHeadMode ) {
  arDatabaseNode* node = __database->getNode("szg_viewer", false);
  if (!node) {
    node = dgMakeNode("szg_viewer", "root", "viewer");
  }
  int fixedHeadInt = (int)fixedHeadMode;
  const ARint ID = node->getID();
  arStructuredData* data = __database->viewerData;
  if (!data->dataIn(__gfx.AR_VIEWER_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_MATRIX, headMatrix.v, AR_FLOAT, AR_FLOATS_PER_MATRIX) ||
      !data->dataIn(__gfx.AR_VIEWER_MID_EYE_OFFSET, midEyeOffset.v, AR_FLOAT, AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_DIRECTION, eyeDirection.v, AR_FLOAT, AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_SPACING, &eyeSpacing, AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_NEAR_CLIP, &nearClip, AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_FAR_CLIP, &farClip, AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_FIXED_HEAD_MODE, &fixedHeadInt, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_VIEWER_UNIT_CONVERSION, &unitConversion, AR_FLOAT, 1)) {
    cerr << "dgViewer error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
  }
#endif

int dgTransform(const string& name, const string& parent,
                const arMatrix4& matrix) {
  arDatabaseNode* node = dgMakeNode(name, parent, "transform");
  return node && dgTransform(node->getID(), matrix) ? node->getID() : -1;
}

bool dgTransform(int ID, const arMatrix4& matrix) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->transformData;
  if (!data->dataIn(__gfx.AR_TRANSFORM_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_TRANSFORM_MATRIX, matrix.v, AR_FLOAT, AR_FLOATS_PER_MATRIX)) {
    cerr << "dgTransform error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int  dgPoints(const string& name, const string& parent,
              int num, int* IDs, float* coords) {
  arDatabaseNode* node = dgMakeNode(name, parent, "points");
  return node && dgPoints(node->getID(), num, IDs, coords) ?
    node->getID() : -1;
}

bool dgPoints(int ID, int num, int* IDs, float* coords) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->pointsData;
  if (!data->dataIn(__gfx.AR_POINTS_ID, &ID, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_POINTS_POINT_IDS, IDs, num) ||
      !data->ptrIn(__gfx.AR_POINTS_POSITIONS, coords, AR_FLOATS_PER_POINT*num)) {
    cerr << "dgPoints error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgPoints(const string& name, const string& parent, int numPoints,
             float* positions) {
  arDatabaseNode* node = dgMakeNode(name, parent, "points");
  return node && dgPoints(node->getID(), numPoints, positions) ?
    node->getID() : -1;
}

bool dgPoints(int ID, int numPoints, float* positions) {
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __database->pointsData;
  if (!data->dataIn(__gfx.AR_POINTS_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_POINTS_POINT_IDS, IDs, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_POINTS_POSITIONS, positions, AR_FLOATS_PER_POINT*numPoints)) {
    cerr << "dgPoints error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgTexture(const string& name, const string& parent,
              const string& filename, int alphaValue) {
  arDatabaseNode* node = dgMakeNode(name, parent, "texture");
  return node && dgTexture(node->getID(), filename, alphaValue) ?
    node->getID() : -1;
}

bool dgTexture(int ID, const string& filename, int alphaValue) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->textureData;

  // Zero width tells the implementation that this is
  // a file, not a bitmap. (This is a little confused.)
  data->setDataDimension(__gfx.AR_TEXTURE_WIDTH, 0);

  if (!data->dataIn(__gfx.AR_TEXTURE_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_TEXTURE_ALPHA, &alphaValue, AR_INT, 1) ||
      !data->dataInString(__gfx.AR_TEXTURE_FILE, filename)) {
    cerr << "dgTexture error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgTexture(const string& name, const string& parent,
              bool alpha, int w, int h, const char* pixels) {
  arDatabaseNode* node = dgMakeNode(name, parent, "texture");
  return node && dgTexture(node->getID(), alpha, w, h, pixels) ?
    node->getID() : -1;
}

bool dgTexture(int ID, bool alpha, int w, int h, const char* pixels) {
  if (ID < 0)
    return false;
  const int bytesPerPixel = alpha ? 4 : 3;
  const int cPixels = w * h * bytesPerPixel;
  arStructuredData* data = __database->textureData;
  if (!data->dataIn(__gfx.AR_TEXTURE_ID, &ID, AR_INT, 1) ||
      !data->dataInString(__gfx.AR_TEXTURE_FILE, "") ||
      !data->dataIn(__gfx.AR_TEXTURE_ALPHA, &alpha, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_TEXTURE_WIDTH, &w, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_TEXTURE_HEIGHT, &h, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_TEXTURE_PIXELS, pixels, AR_CHAR, cPixels)) {
    cerr << "dgTexture error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgBoundingSphere(const string& name, const string& parent,
                     int visibility, float radius, const arVector3& position) {
  arDatabaseNode* node = dgMakeNode(name, parent, "bounding sphere");
  return node && dgBoundingSphere(node->getID(), visibility, radius, position)
    ? node->getID() : -1;
}

bool dgBoundingSphere(int ID, int visibility, float radius,
                      const arVector3& position) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->boundingSphereData;
  if (!data->dataIn(__gfx.AR_BOUNDING_SPHERE_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_BOUNDING_SPHERE_VISIBILITY, &visibility, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_BOUNDING_SPHERE_RADIUS, &radius, AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_BOUNDING_SPHERE_POSITION, position.v, AR_FLOAT, AR_FLOATS_PER_POINT)) {
    cerr << "dgBoundingSphere error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

bool dgErase(const string& name) {
  arStructuredData* data = __database->eraseData;
  arDatabaseNode* node = __database->getNode(name);
  if (!node) {
    // error message was already printed in the above.
    return false;
  }
  int ID = node->getID();
  if (!data->dataIn(__gfx.AR_ERASE_ID, &ID, AR_INT, 1)) {
    cerr << "dgErase error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgBillboard(const string& name, const string& parent,
                 int visibility, const string& text) {
  arDatabaseNode* node = dgMakeNode(name, parent, "billboard");
  return node && dgBillboard(node->getID(), visibility, text) ?
    node->getID() : -1;
}

bool dgBillboard(int ID, int visibility, const string& text) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->billboardData;
  if (!data->dataIn(__gfx.AR_BILLBOARD_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_BILLBOARD_VISIBILITY, &visibility, AR_INT, 1) ||
      !data->dataInString(__gfx.AR_BILLBOARD_TEXT, text)) {
    cerr << "dgBillboard error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgVisibility(const string& name, const string& parent, int visibility) {
  arDatabaseNode* node = dgMakeNode(name, parent, "visibility");
  return node && dgVisibility(node->getID(), visibility) ?
    node->getID() : -1;
}

bool dgVisibility(int ID, int visibility) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->visibilityData;
  if (!data->dataIn(__gfx.AR_VISIBILITY_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_VISIBILITY_VISIBILITY, &visibility, AR_INT, 1)) {
    cerr << "dgVisibility error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgBlend(const string& name, const string& parent, float factor) {
  arDatabaseNode* node = dgMakeNode(name, parent, "blend");
  return node && dgBlend(node->getID(), factor) ?
    node->getID() : -1;
}

bool dgBlend(int ID, float factor) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->blendData;
  if (!data->dataIn(__gfx.AR_BLEND_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_BLEND_FACTOR, &factor, AR_FLOAT, 1)) {
    cerr << "dgBlend error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgStateInt(const string& nodeName, const string& parentName,
               const string& stateName,
               arGraphicsStateValue val1,
               arGraphicsStateValue val2 ) {
  arDatabaseNode* node = dgMakeNode(nodeName, parentName, "state");
  return (node && dgStateInt( node->getID(), stateName, val1, val2 )) ?
    (node->getID()) : (-1);
}
bool dgStateInt( int nodeID, const string& stateName,
    arGraphicsStateValue val1, arGraphicsStateValue val2 ) {
  if (nodeID < 0)
    return false;
  arGraphicsStateValue tmp[2];
  tmp[0] = val1;
  tmp[1] = val2;
  float ftmp(0.);
  arStructuredData* data = __database->graphicsStateData;
  if (!data->dataIn(__gfx.AR_GRAPHICS_STATE_ID, &nodeID, AR_INT, 1) ||
      !data->dataInString( __gfx.AR_GRAPHICS_STATE_STRING, stateName ) ||
      !data->dataIn( __gfx.AR_GRAPHICS_STATE_INT, tmp, AR_INT, 2 ) ||
      !data->dataIn( __gfx.AR_GRAPHICS_STATE_FLOAT, &ftmp, AR_FLOAT, 1)) {
    cerr << "dgStateInt error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgStateFloat( const string& nodeName, const string& parentName,
    const string& stateName, float value ) {
  arDatabaseNode* node = dgMakeNode(nodeName, parentName, "state");
  return (node && dgStateFloat( node->getID(), stateName, value )) ?
    (node->getID()) : (-1);
}
bool dgStateFloat( int nodeID, const string& stateName, float value ) {
  if (nodeID < 0)
    return false;
  arGraphicsStateValue tmp[2];
  tmp[0] = AR_G_FALSE;
  tmp[1] = AR_G_FALSE;
  arStructuredData* data = __database->graphicsStateData;
  if (!data->dataIn(__gfx.AR_GRAPHICS_STATE_ID, &nodeID, AR_INT, 1) ||
      !data->dataInString( __gfx.AR_GRAPHICS_STATE_STRING, stateName ) ||
      !data->dataIn( __gfx.AR_GRAPHICS_STATE_INT, tmp, AR_INT, 2 ) ||
      !data->dataIn( __gfx.AR_GRAPHICS_STATE_FLOAT, &value, AR_FLOAT, 1 )) {
    cerr << "dgStateFloat error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgNormal3(const string& name, const string& parent, int numNormals,
	      int* IDs, float* normals) {
  arDatabaseNode* node = dgMakeNode(name, parent, "normal3");
  return node && dgNormal3(node->getID(), numNormals, IDs, normals) ?
    node->getID() : -1;
}

bool dgNormal3(int ID, int numNormals, int* IDs, float* normals) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->normal3Data;
  if (!data->dataIn(__gfx.AR_NORMAL3_ID, &ID, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_NORMAL3_NORMAL_IDS, IDs, numNormals) ||
      !data->ptrIn(__gfx.AR_NORMAL3_NORMALS, normals, AR_FLOATS_PER_NORMAL*numNormals)) {
    cerr << "dgNormal3 error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgNormal3(const string& name, const string& parent, int numNormals,
	      float* normals) {
  arDatabaseNode* node = dgMakeNode(name, parent, "normal3");
  return node && dgNormal3(node->getID(), numNormals, normals) ?
    node->getID() : -1;
}

bool dgNormal3(int ID, int numNormals, float* normals) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->normal3Data;
  int IDs = -1;
  if (!data->dataIn(__gfx.AR_NORMAL3_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_NORMAL3_NORMAL_IDS, &IDs, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_NORMAL3_NORMALS, normals, AR_FLOATS_PER_NORMAL*numNormals)) {
    cerr << "dgNormal3 error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

// there sure is alot of cut-and-pasting with the API commands associated
// with the arGraphicsArray descended nodes.... sigh!
// maybe there's something one can do about this eventually!

int dgColor4(const string& name, const string& parent, int numColors,
	     int* IDs, float* colors) {
  arDatabaseNode* node = dgMakeNode(name, parent, "color4");
  return node && dgColor4(node->getID(), numColors, IDs, colors) ?
    node->getID() : -1;
}

bool dgColor4(int ID, int numColors, int* IDs, float* colors) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->color4Data;
  if (!data->dataIn(__gfx.AR_COLOR4_ID, &ID, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_COLOR4_COLOR_IDS, IDs, numColors) ||
      !data->ptrIn(__gfx.AR_COLOR4_COLORS, colors, AR_FLOATS_PER_COLOR*numColors)) {
    cerr << "dgColor4 error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgColor4(const string& name, const string& parent, int numColors,
	     float* colors) {
  arDatabaseNode* node = dgMakeNode(name, parent, "color4");
  return node && dgColor4(node->getID(), numColors, colors) ?
    node->getID() : -1;
}

bool dgColor4(int ID, int numColors, float* colors) {
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __database->color4Data;
  if (!data->dataIn(__gfx.AR_COLOR4_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_COLOR4_COLOR_IDS, IDs, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_COLOR4_COLORS, colors, AR_FLOATS_PER_COLOR*numColors)) {
    cerr << "dgColor4 error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgTex2(const string& name, const string& parent, int numTexcoords,
	   int* IDs, float* coords) {
  arDatabaseNode* node = dgMakeNode(name, parent, "tex2");
  return node && dgTex2(node->getID(), numTexcoords, IDs, coords) ?
    node->getID() : -1;
}

bool dgTex2(int ID, int numTexcoords, int* IDs, float* coords) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->tex2Data;
  if (!data->dataIn(__gfx.AR_TEX2_ID, &ID, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_TEX2_TEX_IDS, IDs, numTexcoords) ||
      !data->ptrIn(__gfx.AR_TEX2_COORDS, coords, AR_FLOATS_PER_TEXCOORD*numTexcoords)) {
    cerr << "dgTex2 error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgTex2(const string& name, const string& parent, int numTexcoords,
	   float* coords) {
  arDatabaseNode* node = dgMakeNode(name, parent, "tex2");
  return node && dgTex2(node->getID(), numTexcoords, coords) ?
    node->getID() : -1;
}

bool dgTex2(int ID, int numTexcoords, float* coords) {
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __database->tex2Data;
  if (!data->dataIn(__gfx.AR_TEX2_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_TEX2_TEX_IDS, IDs, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_TEX2_COORDS, coords, AR_FLOATS_PER_TEXCOORD*numTexcoords)) {
    cerr << "dgTex2 error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgIndex(const string& name, const string& parent, int numIndices,
	    int* IDs, int* indices) {
  arDatabaseNode* node = dgMakeNode(name, parent, "index");
  return node && dgIndex(node->getID(), numIndices, IDs, indices) ?
    node->getID() : -1;
}

bool dgIndex(int ID, int numIndices, int* IDs, int* indices) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->indexData;
  if (!data->dataIn(__gfx.AR_INDEX_ID, &ID, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_INDEX_INDEX_IDS, IDs, numIndices) ||
      !data->ptrIn(__gfx.AR_INDEX_INDICES, indices, numIndices)) {
    cerr << "dgIndex error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgIndex(const string& name, const string& parent, int numIndices,
	    int* indices) {
  arDatabaseNode* node = dgMakeNode(name, parent, "index");
  return node && dgIndex(node->getID(), numIndices, indices) ?
    node->getID() : -1;
}

bool dgIndex(int ID, int numIndices, int* indices) {
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __database->indexData;
  if (!data->dataIn(__gfx.AR_INDEX_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_INDEX_INDEX_IDS, IDs, AR_INT, 1) ||
      !data->ptrIn(__gfx.AR_INDEX_INDICES, indices, numIndices)) {
    cerr << "dgIndex error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgDrawable(const string& name, const string& parent,
	       int drawableType, int numPrimitives) {
  arDatabaseNode* node = dgMakeNode(name, parent, "drawable");
  return node && dgDrawable(node->getID(), drawableType, numPrimitives) ?
    node->getID() : -1;
}

bool dgDrawable(int ID, int drawableType, int numPrimitives) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->drawableData;
  if (!data->dataIn(__gfx.AR_DRAWABLE_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_DRAWABLE_TYPE, &drawableType, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_DRAWABLE_NUMBER, &numPrimitives, AR_INT, 1)) {
    cerr << "dgDrawable error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgMaterial(const string& name,
               const string& parent,
               const arVector3& diffuse,
	       const arVector3& ambient,
               const arVector3& specular,
               const arVector3& emissive,
	       float exponent,
               float alpha) {
  arDatabaseNode* node = dgMakeNode(name, parent, "material");
  return node && dgMaterial(node->getID(), diffuse, ambient,
		            specular, emissive, exponent, alpha) ?
    node->getID() : -1;
}

bool dgMaterial(int ID,
                const arVector3& diffuse,
	        const arVector3& ambient,
                const arVector3& specular,
                const arVector3& emissive,
	        float exponent,
                float alpha) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->materialData;
  if (!data->dataIn(__gfx.AR_MATERIAL_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_MATERIAL_DIFFUSE, diffuse.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_MATERIAL_AMBIENT, ambient.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_MATERIAL_SPECULAR, specular.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_MATERIAL_EMISSIVE, emissive.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_MATERIAL_EXPONENT, &exponent, AR_FLOAT, 1) ||
      !data->dataIn(__gfx.AR_MATERIAL_ALPHA, &alpha, AR_FLOAT, 1)) {
    cerr << "dgMaterial error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

// OpenGL interprets light positions weirdly.  If the fourth value
// is nonzero, it is a positional light. Otherwise it's directional,
// and the "position" really gives the light direction.  These two
// styles transform differently.  (See arLight::activateLight().)

int dgLight(const string& name,
            const string& parent,
	    int lightID,
            arVector4 position,
            const arVector3& diffuse,
	    const arVector3& ambient,
            const arVector3& specular,
            const arVector3& attenuate,
            const arVector3& spotDirection,
            float spotCutoff,
            float spotExponent) {
  arDatabaseNode* node = dgMakeNode(name, parent, "light");
  return node && dgLight(node->getID(), lightID, position,
                         diffuse, ambient, specular, attenuate, spotDirection,
                         spotCutoff, spotExponent) ?
    node->getID() : -1;
}

bool dgLight(int ID,
	     int lightID,
             arVector4 position,
             const arVector3& diffuse,
	     const arVector3& ambient,
             const arVector3& specular,
             const arVector3& attenuate,
             const arVector3& spotDirection,
             float spotCutoff,
             float spotExponent) {
  if (ID < 0)
    return false;
  const float temp[5] = {spotDirection.v[0],
		   spotDirection.v[1],
		   spotDirection.v[2],
		   spotCutoff,
		   spotExponent };
  arStructuredData* data = __database->lightData;
  if (!data->dataIn(__gfx.AR_LIGHT_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_LIGHT_LIGHT_ID, &lightID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_LIGHT_POSITION, position.v, AR_FLOAT, 4) ||
      !data->dataIn(__gfx.AR_LIGHT_DIFFUSE, diffuse.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_LIGHT_AMBIENT, ambient.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_LIGHT_SPECULAR, specular.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_LIGHT_ATTENUATE, attenuate.v, AR_FLOAT, 3) ||
      !data->dataIn(__gfx.AR_LIGHT_SPOT, temp, AR_FLOAT, 5)) {
    cerr << "dgLight error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgCamera(const string& name, const string& parent,
	     int cameraID, float leftClip, float rightClip,
	     float bottomClip, float topClip, float nearClip, float farClip,
             const arVector3& eyePosition,
             const arVector3& centerPosition,
             const arVector3& upDirection) {
  arDatabaseNode* node = dgMakeNode(name, parent, "persp camera");
  return node && dgCamera(node->getID(), cameraID, leftClip,
		          rightClip, bottomClip, topClip, nearClip, farClip,
		          eyePosition, centerPosition, upDirection) ?
    node->getID() : -1;
}

bool dgCamera(int ID,
	      int cameraID, float leftClip, float rightClip,
	      float bottomClip, float topClip, float nearClip, float farClip,
              const arVector3& eyePosition,
              const arVector3& centerPosition,
              const arVector3& upDirection) {
  if (ID < 0)
    return false;
  float temp1[6] = {leftClip, rightClip, bottomClip, topClip, nearClip,
		    farClip};
  float temp2[9] = {eyePosition.v[0], eyePosition.v[1], eyePosition.v[2],
		    centerPosition.v[0], centerPosition.v[1],
                    centerPosition.v[2],
                    upDirection.v[0], upDirection.v[1], upDirection.v[2]};
  arStructuredData* data = __database->perspCameraData;
  if (!data->dataIn(__gfx.AR_PERSP_CAMERA_ID, &ID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_PERSP_CAMERA_CAMERA_ID, &cameraID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_PERSP_CAMERA_FRUSTUM, temp1, AR_FLOAT, 6) ||
      !data->dataIn(__gfx.AR_PERSP_CAMERA_LOOKAT, temp2, AR_FLOAT, 9)) {
    cerr << "dgCamera error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgBumpMap(const string& name, const string& parent,
	      const string& filename, float height) {
  arDatabaseNode* node = dgMakeNode(name, parent, "bump map");
  return node && dgBumpMap(node->getID(), filename, height) ?
    node->getID() : -1;
}

bool dgBumpMap(int ID, const string& filename, float height) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->bumpMapData;
  if (!data->dataIn(__gfx.AR_BUMPMAP_ID, &ID, AR_INT, 1) ||
      !data->dataInString(__gfx.AR_BUMPMAP_FILE, filename) ||
      !data->dataIn(__gfx.AR_BUMPMAP_HEIGHT, &height, AR_FLOAT, 1)) {
    cerr << "dgBumpMap error: dataIn failed.\n";
    return false;
  }
  return __database->alter(data);
}

int dgPlugin(const string& name,
              const string& parent,
              const string& fileName,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData ) {
  arDatabaseNode* node = dgMakeNode(name, parent, "graphics plugin");
  if (!node) {
    return -1;
  }
  if (!dgPlugin( node->getID(), fileName, intData, numInts, floatData, numFloats,
        longData, numLongs, doubleData, numDoubles, stringData )) {
    return -1;
  }
  return node->getID();
}

bool dgPlugin( int ID, const string& fileName,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData ) {
  if (ID < 0)
    return false;
  arStructuredData* data = __database->graphicsPluginData;
  if (!data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_ID, &ID, AR_INT, 1 )) {
    ar_log_error() << "dgPlugin failed to set ID.\n";
    return false;
  }
  if (!data->dataInString( __gfx.AR_GRAPHICS_PLUGIN_NAME, fileName )) {
    ar_log_error() << "dgPlugin failed to set fileName.\n";
    return false;
  }

  bool ok = true;
  ok &= (intData) ?
    data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_INT, (const void*)intData, AR_INT, numInts ) :
    data->setDataDimension( __gfx.AR_GRAPHICS_PLUGIN_INT, 0 );
  ok &= (longData) ?
    data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_LONG, (const void*)longData, AR_LONG, numLongs ) :
    data->setDataDimension( __gfx.AR_GRAPHICS_PLUGIN_LONG, 0 );
  ok &= (floatData) ?
    data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_FLOAT, (const void*)floatData, AR_FLOAT, numFloats ) :
    data->setDataDimension( __gfx.AR_GRAPHICS_PLUGIN_FLOAT, 0 );
  ok &= (doubleData) ?
    data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_DOUBLE, (const void*)doubleData, AR_DOUBLE, numDoubles ) :
    data->setDataDimension( __gfx.AR_GRAPHICS_PLUGIN_DOUBLE, 0 );
  if (!ok) {
    ar_log_error() << "dgPlugin() failed to pack numerical data.\n";
    return false;
  }

  int numStrings = 0;
  if (stringData) {
    numStrings = (int)stringData->size();
    unsigned stringSize;
    char* stringPtr = ar_packStringVector( *stringData, stringSize );
    if (!stringPtr) {
      ar_log_error() << "dgPlugin() failed to allocate string buffer.\n";
      return false;
    }

    const bool ok = data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_STRING, (const void*)stringPtr, AR_CHAR, stringSize );
    delete[] stringPtr;
    if (!ok) {
      ar_log_error() << "dgPlugin() failed to pack string data.\n";
      return false;
    }
  }
  if (!data->dataIn( __gfx.AR_GRAPHICS_PLUGIN_NUMSTRINGS, (const void*)&numStrings, AR_INT, 1 )) {
    ar_log_error() << "dgPlugin() failed to pack number of strings.\n";
    return false;
  }

  return __database->alter(data);
}

int dgPlugin(const string& name,
              const string& parent,
              const string& fileName,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData ) {
  arDatabaseNode* node = dgMakeNode(name, parent, "graphics plugin");
  return (node && dgPlugin( node->getID(), fileName, intData, floatData, longData,
	                    doubleData, stringData )) ?
    node->getID() : -1;
}

bool dgPlugin( int ID, const string& fileName,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData ) {
  int* intPtr = new int[intData.size()];
  float* floatPtr = new float[floatData.size()];
  long* longPtr = new long[longData.size()];
  double* doublePtr = new double[doubleData.size()];
  if (!intPtr || !longPtr || !floatPtr || !doublePtr) {
    ar_log_error() << "dgPlugin() out of memory.\n";
    return false;
  }

  std::copy( intData.begin(), intData.end(), intPtr );
  std::copy( floatData.begin(), floatData.end(), floatPtr );
  std::copy( longData.begin(), longData.end(), longPtr );
  std::copy( doubleData.begin(), doubleData.end(), doublePtr );
  bool stat = dgPlugin( ID, fileName,
                        intPtr, (int)intData.size(),
                        floatPtr, (int)floatData.size(),
                        longPtr, (int)longData.size(),
                        doublePtr, (int)doubleData.size(),
                        &stringData );
  delete[] intPtr;
  delete[] floatPtr;
  delete[] longPtr;
  delete[] doublePtr;
  return stat;
}

int dgPython(const string& name,
              const string& parent,
              const string& moduleName,
              const string& factoryName,
              bool reloadModule,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData ) {
  arDatabaseNode* node = dgMakeNode(name, parent, "graphics plugin");
  return (node && dgPython( node->getID(), moduleName, factoryName, reloadModule,
        intData, numInts, floatData, numFloats,
        longData, numLongs, doubleData, numDoubles, stringData )) ?
    node->getID() : -1;
}

bool dgPython( int ID, const string& moduleName,
               const string& factoryName,
               bool reloadModule,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData ) {
  int* iData = new int[numInts+1];
  if (!iData) {
    ar_log_error() << "dgPython() out of memory.\n";
    return false;
  }

  std::vector< std::string > sData;
  if (stringData) {
    sData = *stringData;
  }
  if (intData) {
    memcpy( iData, intData, numInts*sizeof(int) );
  }
  iData[numInts] = (int)reloadModule;
  sData.push_back( moduleName );
  sData.push_back( factoryName );
  const bool ok = dgPlugin( ID, "arPythonGraphicsPlugin", iData, numInts+1,
    floatData, numFloats, longData, numLongs, doubleData, numDoubles, &sData );
  delete[] iData;
  return ok;
}

int dgPython(const string& name,
              const string& parent,
              const string& moduleName,
              const string& factoryName,
              bool reloadModule,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData ) {
  std::vector<int> iData = intData;
  std::vector< std::string > sData = stringData;
  iData.push_back((int)reloadModule);
  sData.push_back( moduleName );
  sData.push_back( factoryName );
  return dgPlugin( name, parent, "arPythonGraphicsPlugin",
    intData, floatData, longData, doubleData, stringData );
}

bool dgPython( int ID, const string& moduleName,
               const string& factoryName,
               bool reloadModule,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData ) {
  std::vector<int> iData = intData;
  std::vector< std::string > sData = stringData;
  iData.push_back((int)reloadModule);
  sData.push_back( moduleName );
  sData.push_back( factoryName );
  return dgPlugin( ID, "arPythonGraphicsPlugin",
    intData, floatData, longData, doubleData, stringData );
}
