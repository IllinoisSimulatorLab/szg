//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsAPI.h"
#include "arHead.h"

// global variables!
static arGraphicsDatabase* __currentGraphicsDatabase = NULL;
static arGraphicsLanguage  __gfx;

void dgSetGraphicsDatabase(arGraphicsDatabase* database){
  __currentGraphicsDatabase = database;
}

string dgGetNodeName(int nodeID){
  const arDatabaseNode* theNode = __currentGraphicsDatabase->getNode(nodeID);
  if (!theNode){
    cerr << "dgGetNodeName error: no node with that ID.\n";
    return string("NULL");
  }
  return theNode->getName();
}

/** @bug If one node's gizmo count changes, other nodes' counts
 *  can't change atomically with it.  Maybe if nodes are grouped together?
 */

/** @todo Report an error if a node is created with
 * a dgDrawable node as its parent.  
 */

arGraphicsNode* dgGetNode(const string& nodeName){
  return (arGraphicsNode*) __currentGraphicsDatabase->getNode(nodeName);
}

arDatabaseNode* dgMakeNode(const string& name, 
                           const string& parent, 
                           const string& type){
  if (!__currentGraphicsDatabase) {
    cerr << "syzygy error: dgSetGraphicsDatabase not yet called.\n";
    return NULL;
  }

  arStructuredData* data = __currentGraphicsDatabase->makeNodeData;
  arDatabaseNode* parentNode 
    = __currentGraphicsDatabase->getNode(parent);
  if (!parentNode){
    // error message already printed in getNode(...)
    return NULL;
  }
  // The -1 signifies that we'll take whatever ID the database assigns us.
  int ID = -1;
  int parentID = parentNode->getID();
  if (!data->dataIn(__gfx.AR_MAKE_NODE_PARENT_ID, &parentID, AR_INT, 1) ||
      !data->dataIn(__gfx.AR_MAKE_NODE_ID, &ID, AR_INT,1) ||
      !data->dataInString(__gfx.AR_MAKE_NODE_NAME, name) ||
      !data->dataInString(__gfx.AR_MAKE_NODE_TYPE, type)) {
    cerr << "dgMakeNode error: dataIn failed.\n";
    return NULL;
  }
  // Use alter not arGraphicsDatabase::alter, to ensure that
  // the remote node will be created. NOTE: if there is an error,
  // the functions called by alter(...) will complain for us.
  return __currentGraphicsDatabase->alter(data);
}

// This function is a friend of arHead
bool dgViewer( const arHead& head ) {
  arDatabaseNode* node 
    = __currentGraphicsDatabase->getNode("szg_viewer", false);
  if (!node){
    node = dgMakeNode("szg_viewer","root","viewer");
  }
  const ARint ID = node->getID();
  arStructuredData* data = __currentGraphicsDatabase->viewerData;
  if (!data->dataIn(__gfx.AR_VIEWER_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_MATRIX, head._matrix.v ,AR_FLOAT,AR_FLOATS_PER_MATRIX) ||
      !data->dataIn(__gfx.AR_VIEWER_MID_EYE_OFFSET, head._midEyeOffset.v ,AR_FLOAT,AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_DIRECTION, head._eyeDirection.v ,AR_FLOAT,AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_SPACING, &head._eyeSpacing ,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_NEAR_CLIP, &head._nearClip ,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_FAR_CLIP, &head._farClip ,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_FIXED_HEAD_MODE, &head._fixedHeadMode ,AR_INT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_UNIT_CONVERSION, &head._unitConversion ,AR_FLOAT,1)) {
    cerr << "dgViewer error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

bool dgViewer(const arMatrix4& headMatrix, const arVector3& midEyeOffset,
	     const arVector3& eyeDirection, float eyeSpacing,
	     float nearClip, float farClip, float unitConversion,
             bool fixedHeadMode ){
  // AARGH! THIS IS NOT GOOD! getNode(nodeName) has become a very, very slow
  // command!
  arDatabaseNode* node 
    = __currentGraphicsDatabase->getNode("szg_viewer", false);
  if (!node){
    node = dgMakeNode("szg_viewer","root","viewer");
  }
  int fixedHeadInt = (int)fixedHeadMode;
  const ARint ID = node->getID();
  arStructuredData* data = __currentGraphicsDatabase->viewerData;
  if (!data->dataIn(__gfx.AR_VIEWER_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_MATRIX,headMatrix.v,AR_FLOAT,AR_FLOATS_PER_MATRIX) ||
      !data->dataIn(__gfx.AR_VIEWER_MID_EYE_OFFSET,midEyeOffset.v,AR_FLOAT,AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_DIRECTION,eyeDirection.v,AR_FLOAT,AR_FLOATS_PER_POINT) ||
      !data->dataIn(__gfx.AR_VIEWER_EYE_SPACING,&eyeSpacing,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_NEAR_CLIP,&nearClip,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_FAR_CLIP,&farClip,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_FIXED_HEAD_MODE,&fixedHeadInt,AR_INT,1) ||
      !data->dataIn(__gfx.AR_VIEWER_UNIT_CONVERSION,&unitConversion,AR_FLOAT,1)) {
    cerr << "dgViewer error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgTransform(const string& name, const string& parent,
                const arMatrix4& matrix){
  arDatabaseNode* node = dgMakeNode(name,parent,"transform");
  return node && dgTransform(node->getID(), matrix) ? node->getID() : -1;
}

bool dgTransform(int ID, const arMatrix4& matrix){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->transformData;
  if (!data->dataIn(__gfx.AR_TRANSFORM_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_TRANSFORM_MATRIX,matrix.v,AR_FLOAT,AR_FLOATS_PER_MATRIX)) {
    cerr << "dgTransform error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int  dgPoints(const string& name, const string& parent, 
              int num, int* IDs, float* coords){
  arDatabaseNode* node = dgMakeNode(name,parent,"points");
  return node && dgPoints(node->getID(), num, IDs, coords) ?
    node->getID() : -1;
}

bool dgPoints(int ID, int num, int* IDs, float* coords){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->pointsData;
  if (!data->dataIn(__gfx.AR_POINTS_ID,&ID,AR_INT,1) ||
      !data->ptrIn(__gfx.AR_POINTS_POINT_IDS,IDs,num) ||
      !data->ptrIn(__gfx.AR_POINTS_POSITIONS,coords,AR_FLOATS_PER_POINT*num)) {
    cerr << "dgPoints error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgPoints(const string& name, const string& parent, int numPoints, 
             float* positions){
  arDatabaseNode* node = dgMakeNode(name,parent,"points");
  return node && dgPoints(node->getID(), numPoints, positions) ?
    node->getID() : -1;
}

bool dgPoints(int ID, int numPoints, float* positions){
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __currentGraphicsDatabase->pointsData;
  if (!data->dataIn(__gfx.AR_POINTS_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_POINTS_POINT_IDS,IDs,AR_INT,1) || 
      !data->ptrIn(__gfx.AR_POINTS_POSITIONS,positions,AR_FLOATS_PER_POINT*numPoints)){
    cerr << "dgPoints error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgTexture(const string& name, const string& parent, 
              const string& filename, int alphaValue){
  arDatabaseNode* node = dgMakeNode(name,parent,"texture");
  return node && dgTexture(node->getID(), filename, alphaValue) ?
    node->getID() : -1;
} 

bool dgTexture(int ID, const string& filename, int alphaValue){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->textureData;
  // Important to set the data dimension of the width field to 0.
  // This is the flag we use to tell the implementation that this is
  // a file, not a bitmap. (Of course, this is a little confused...)
  data->setDataDimension(__gfx.AR_TEXTURE_WIDTH, 0);
  if (!data->dataIn(__gfx.AR_TEXTURE_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_TEXTURE_ALPHA,&alphaValue,AR_INT,1) ||
      !data->dataInString(__gfx.AR_TEXTURE_FILE, filename)) {
    cerr << "dgTexture error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
} 

int dgTexture(const string& name, const string& parent,
              bool alpha, int w, int h, const char* pixels){
  arDatabaseNode* node = dgMakeNode(name,parent,"texture");
  return node && dgTexture(node->getID(), alpha, w, h, pixels) ?
    node->getID() : -1;
} 

bool dgTexture(int ID, bool alpha, int w, int h, const char* pixels){
  if (ID < 0)
    return false;
  const int bytesPerPixel = alpha ? 4 : 3;
  const int cPixels = w * h * bytesPerPixel;
  arStructuredData* data = __currentGraphicsDatabase->textureData;
  if (!data->dataIn(__gfx.AR_TEXTURE_ID,&ID,AR_INT,1) ||
      !data->dataInString(__gfx.AR_TEXTURE_FILE, "") ||
      !data->dataIn(__gfx.AR_TEXTURE_ALPHA,&alpha,AR_INT,1) ||
      !data->dataIn(__gfx.AR_TEXTURE_WIDTH,&w,AR_INT,1) ||
      !data->dataIn(__gfx.AR_TEXTURE_HEIGHT,&h,AR_INT,1) ||
      !data->dataIn(__gfx.AR_TEXTURE_PIXELS, pixels, AR_CHAR, cPixels)) {
    cerr << "dgTexture error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
} 

int dgBoundingSphere(const string& name, const string& parent, 
                     int visibility, float radius, const arVector3& position){
  arDatabaseNode* node = dgMakeNode(name,parent,"bounding sphere");
  return node && dgBoundingSphere(node->getID(), visibility, radius, position)
    ? node->getID() : -1;
}

bool dgBoundingSphere(int ID, int visibility, float radius, 
                      const arVector3& position){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->boundingSphereData;
  if (!data->dataIn(__gfx.AR_BOUNDING_SPHERE_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_BOUNDING_SPHERE_VISIBILITY,&visibility,AR_INT,1) ||
      !data->dataIn(__gfx.AR_BOUNDING_SPHERE_RADIUS,&radius,AR_FLOAT,1) ||
      !data->dataIn(__gfx.AR_BOUNDING_SPHERE_POSITION,position.v,AR_FLOAT,AR_FLOATS_PER_POINT)) {
    cerr << "dgBoundingSphere error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

bool dgErase(const string& name){
  arStructuredData* data = __currentGraphicsDatabase->eraseData;
  arDatabaseNode* node = __currentGraphicsDatabase->getNode(name);
  if (!node){
    // error message was already printed in the above.
    return false;
  }
  int ID = node->getID();
  if (!data->dataIn(__gfx.AR_ERASE_ID, &ID, AR_INT, 1)) {
    cerr << "dgErase error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgBillboard(const string& name, const string& parent,
                 int visibility, const string& text){
  arDatabaseNode* node = dgMakeNode(name,parent,"billboard");
  return node && dgBillboard(node->getID(), visibility, text) ?
    node->getID() : -1;
}

bool dgBillboard(int ID, int visibility, const string& text){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->billboardData;
  if (!data->dataIn(__gfx.AR_BILLBOARD_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_BILLBOARD_VISIBILITY,&visibility,AR_INT,1) ||
      !data->dataInString(__gfx.AR_BILLBOARD_TEXT, text)) {
    cerr << "dgBillboard error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgVisibility(const string& name, const string& parent, int visibility){
  arDatabaseNode* node = dgMakeNode(name,parent,"visibility");
  return node && dgVisibility(node->getID(), visibility) ?
    node->getID() : -1;
}

bool dgVisibility(int ID, int visibility){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->visibilityData;
  if (!data->dataIn(__gfx.AR_VISIBILITY_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_VISIBILITY_VISIBILITY,&visibility,AR_INT,1)) {
    cerr << "dgVisibility error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgBlend(const string& name, const string& parent, float factor){
  arDatabaseNode* node = dgMakeNode(name,parent,"blend");
  return node && dgBlend(node->getID(), factor) ?
    node->getID() : -1;
}

bool dgBlend(int ID, float factor){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->blendData;
  if (!data->dataIn(__gfx.AR_BLEND_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_BLEND_FACTOR,&factor,AR_FLOAT,1)){
    cerr << "dgBlend error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgNormal3(const string& name, const string& parent, int numNormals,
	      int* IDs, float* normals){
  arDatabaseNode* node = dgMakeNode(name,parent,"normal3");
  return node && dgNormal3(node->getID(), numNormals, IDs, normals) ?
    node->getID() : -1;
}

bool dgNormal3(int ID, int numNormals, int* IDs, float* normals){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->normal3Data;
  if (!data->dataIn(__gfx.AR_NORMAL3_ID,&ID,AR_INT,1) ||
      !data->ptrIn(__gfx.AR_NORMAL3_NORMAL_IDS,IDs,numNormals) ||
      !data->ptrIn(__gfx.AR_NORMAL3_NORMALS,normals,AR_FLOATS_PER_NORMAL*numNormals)) {
    cerr << "dgNormal3 error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgNormal3(const string& name, const string& parent, int numNormals,
	      float* normals){
  arDatabaseNode* node = dgMakeNode(name,parent,"normal3");
  return node && dgNormal3(node->getID(), numNormals, normals) ?
    node->getID() : -1;
}

bool dgNormal3(int ID, int numNormals, float* normals){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->normal3Data;
  int IDs = -1;
  if (!data->dataIn(__gfx.AR_NORMAL3_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_NORMAL3_NORMAL_IDS,&IDs,AR_INT,1) || 
      !data->ptrIn(__gfx.AR_NORMAL3_NORMALS,normals,AR_FLOATS_PER_NORMAL*numNormals)){
    cerr << "dgNormal3 error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

// there sure is alot of cut-and-pasting with the API commands associated
// with the arGraphicsArray descended nodes.... sigh!
// maybe there's something one can do about this eventually!

int dgColor4(const string& name, const string& parent, int numColors,
	     int* IDs, float* colors){
  arDatabaseNode* node = dgMakeNode(name,parent,"color4");
  return node && dgColor4(node->getID(), numColors, IDs, colors) ?
    node->getID() : -1;
}

bool dgColor4(int ID, int numColors, int* IDs, float* colors){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->color4Data;
  if (!data->dataIn(__gfx.AR_COLOR4_ID,&ID,AR_INT,1) ||
      !data->ptrIn(__gfx.AR_COLOR4_COLOR_IDS,IDs,numColors) ||
      !data->ptrIn(__gfx.AR_COLOR4_COLORS,colors,AR_FLOATS_PER_COLOR*numColors)) {
    cerr << "dgColor4 error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgColor4(const string& name, const string& parent, int numColors,
	     float* colors){
  arDatabaseNode* node = dgMakeNode(name,parent,"color4");
  return node && dgColor4(node->getID(), numColors, colors) ?
    node->getID() : -1;
}

bool dgColor4(int ID, int numColors, float* colors){
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __currentGraphicsDatabase->color4Data;
  if (!data->dataIn(__gfx.AR_COLOR4_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_COLOR4_COLOR_IDS,IDs,AR_INT,1) || 
      !data->ptrIn(__gfx.AR_COLOR4_COLORS,colors,AR_FLOATS_PER_COLOR*numColors)){
    cerr << "dgColor4 error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgTex2(const string& name, const string& parent, int numTexcoords,
	   int* IDs, float* coords){
  arDatabaseNode* node = dgMakeNode(name,parent,"tex2");
  return node && dgTex2(node->getID(), numTexcoords, IDs, coords) ?
    node->getID() : -1;
}

bool dgTex2(int ID, int numTexcoords, int* IDs, float* coords){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->tex2Data;
  if (!data->dataIn(__gfx.AR_TEX2_ID,&ID,AR_INT,1) ||
      !data->ptrIn(__gfx.AR_TEX2_TEX_IDS,IDs,numTexcoords) ||
      !data->ptrIn(__gfx.AR_TEX2_COORDS,coords,AR_FLOATS_PER_TEXCOORD*numTexcoords)) {
    cerr << "dgTex2 error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgTex2(const string& name, const string& parent, int numTexcoords,
	   float* coords){
  arDatabaseNode* node = dgMakeNode(name,parent,"tex2");
  return node && dgTex2(node->getID(), numTexcoords, coords) ?
    node->getID() : -1;
}

bool dgTex2(int ID, int numTexcoords, float* coords){
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __currentGraphicsDatabase->tex2Data;
  if (!data->dataIn(__gfx.AR_TEX2_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_TEX2_TEX_IDS,IDs,AR_INT,1) || 
      !data->ptrIn(__gfx.AR_TEX2_COORDS,coords,AR_FLOATS_PER_TEXCOORD*numTexcoords)){
    cerr << "dgTex2 error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgIndex(const string& name, const string& parent, int numIndices,
	    int* IDs, int* indices){
  arDatabaseNode* node = dgMakeNode(name,parent,"index");
  return node && dgIndex(node->getID(), numIndices, IDs, indices) ?
    node->getID() : -1;
}

bool dgIndex(int ID, int numIndices, int* IDs, int* indices){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->indexData;
  if (!data->dataIn(__gfx.AR_INDEX_ID,&ID,AR_INT,1) ||
      !data->ptrIn(__gfx.AR_INDEX_INDEX_IDS,IDs,numIndices) ||
      !data->ptrIn(__gfx.AR_INDEX_INDICES,indices,numIndices)) {
    cerr << "dgIndex error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgIndex(const string& name, const string& parent, int numIndices,
	    int* indices){
  arDatabaseNode* node = dgMakeNode(name,parent,"index");
  return node && dgIndex(node->getID(), numIndices, indices) ?
    node->getID() : -1;
}

bool dgIndex(int ID, int numIndices, int* indices){
  if (ID < 0)
    return false;
  int IDs[1] = {-1};
  arStructuredData* data = __currentGraphicsDatabase->indexData;
  if (!data->dataIn(__gfx.AR_INDEX_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_INDEX_INDEX_IDS,IDs,AR_INT,1) || 
      !data->ptrIn(__gfx.AR_INDEX_INDICES,indices,numIndices)){
    cerr << "dgIndex error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgDrawable(const string& name, const string& parent,
	       int drawableType, int numPrimitives){
  arDatabaseNode* node = dgMakeNode(name,parent,"drawable");
  return node && dgDrawable(node->getID(), drawableType, numPrimitives) ?
    node->getID() : -1;
}

bool dgDrawable(int ID, int drawableType, int numPrimitives){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->drawableData;
  if (!data->dataIn(__gfx.AR_DRAWABLE_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_DRAWABLE_TYPE,&drawableType,AR_INT,1) || 
      !data->dataIn(__gfx.AR_DRAWABLE_NUMBER,&numPrimitives,AR_INT,1)){
    cerr << "dgDrawable error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

int dgMaterial(const string& name, 
               const string& parent, 
               const arVector3& diffuse,
	       const arVector3& ambient, 
               const arVector3& specular, 
               const arVector3& emissive,
	       float exponent, 
               float alpha){
  arDatabaseNode* node = dgMakeNode(name,parent,"material");
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
                float alpha){
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->materialData;
  if (!data->dataIn(__gfx.AR_MATERIAL_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_MATERIAL_DIFFUSE,diffuse.v,AR_FLOAT,3) || 
      !data->dataIn(__gfx.AR_MATERIAL_AMBIENT,ambient.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_MATERIAL_SPECULAR,specular.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_MATERIAL_EMISSIVE,emissive.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_MATERIAL_EXPONENT,&exponent,AR_FLOAT,1) || 
      !data->dataIn(__gfx.AR_MATERIAL_ALPHA,&alpha,AR_FLOAT,1)){
    cerr << "dgMaterial error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

/// note that there is a little weirdness with the way OpenGL interprets
/// light positions... if the fourth value is nonzero, then it is
/// a positional light. If the fourth value is zero, then it is a
/// directional light and the "position" really gives the light direction!
/// of course, each of these styles transforms differently!
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
            float spotExponent){
  arDatabaseNode* node = dgMakeNode(name,parent,"light");
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
             float spotExponent){
  if (ID < 0)
    return false;
  float temp[5] = {spotDirection.v[0],
		   spotDirection.v[1],
		   spotDirection.v[2],
		   spotCutoff,
		   spotExponent };
  arStructuredData* data = __currentGraphicsDatabase->lightData;
  if (!data->dataIn(__gfx.AR_LIGHT_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_LIGHT_LIGHT_ID,&lightID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_LIGHT_POSITION,position.v,AR_FLOAT,4) ||
      !data->dataIn(__gfx.AR_LIGHT_DIFFUSE,diffuse.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_LIGHT_AMBIENT,ambient.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_LIGHT_SPECULAR,specular.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_LIGHT_ATTENUATE,attenuate.v,AR_FLOAT,3) ||
      !data->dataIn(__gfx.AR_LIGHT_SPOT,temp,AR_FLOAT,5)){
    cerr << "dgLight error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}
int dgCamera(const string& name, const string& parent,
	     int cameraID, float leftClip, float rightClip, 
	     float bottomClip, float topClip, float nearClip, float farClip,
             const arVector3& eyePosition,
             const arVector3& centerPosition,
             const arVector3& upDirection){
  arDatabaseNode* node = dgMakeNode(name,parent,"persp camera");
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
              const arVector3& upDirection){
  if (ID < 0)
    return false;
  float temp1[6] = {leftClip, rightClip, bottomClip, topClip, nearClip,
		    farClip};
  float temp2[9] = {eyePosition.v[0], eyePosition.v[1], eyePosition.v[2],
		    centerPosition.v[0], centerPosition.v[1], 
                    centerPosition.v[2],
                    upDirection.v[0], upDirection.v[1], upDirection.v[2]};
  arStructuredData* data = __currentGraphicsDatabase->perspCameraData;
  if (!data->dataIn(__gfx.AR_PERSP_CAMERA_ID,&ID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_PERSP_CAMERA_CAMERA_ID,&cameraID,AR_INT,1) ||
      !data->dataIn(__gfx.AR_PERSP_CAMERA_FRUSTUM,temp1,AR_FLOAT,6) ||
      !data->dataIn(__gfx.AR_PERSP_CAMERA_LOOKAT,temp2,AR_FLOAT,9)){
    cerr << "dgCamera error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}


int dgBumpMap(const string& name, const string& parent,
	      const string& filename, float height) {
  arDatabaseNode* node = dgMakeNode(name,parent,"bump map");
  return node && dgBumpMap(node->getID(), filename, height) ?
    node->getID() : -1;
}

int dgBumpMap(int ID, const string& filename, float height) {
  if (ID < 0)
    return false;
  arStructuredData* data = __currentGraphicsDatabase->bumpMapData;
  if (!data->dataIn(__gfx.AR_BUMPMAP_ID, &ID, AR_INT, 1) ||
      !data->dataInString(__gfx.AR_BUMPMAP_FILE, filename) ||
      !data->dataIn(__gfx.AR_BUMPMAP_HEIGHT, &height, AR_FLOAT, 1)) {
    cerr << "dgBumpMap error: dataIn failed.\n";
    return false;
  }
  return __currentGraphicsDatabase->alter(data) ? true : false;
}

