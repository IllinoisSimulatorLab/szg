//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arGraphicsDatabase::arGraphicsDatabase() :
  _texturePath(new list<string>),
  _alphabet(new arTexture*[26])
{
  _lang = (arDatabaseLanguage*)&_gfx;
  if (!_initDatabaseLanguage())
    return;

  // Have to add the processing callback for the "graphics admin"
  // message. Please note that the arGraphicsPeer processes this
  // differently than here (i.e. the "graphics admin" messages 
  // are culled from the message stream before being sent to the
  // database for processing.
  arDataTemplate* t = _lang->find("graphics admin");
  _databaseReceive[t->getID()] 
    = (arDatabaseProcessingCallback)&arGraphicsDatabase::_processAdmin;
  
  // Initialize the texture data.
  int i;
  for (i=0; i<26; i++)
    _alphabet[i] = NULL;

  // Initialize the texture path list.
  _texturePath->push_back(string("") /* local directory */ );

  ar_mutex_init(&_texturePathLock);

  // make the external parsing storage
  arTemplateDictionary* d = _gfx.getDictionary();
  transformData = new arStructuredData(d, "transform");
  pointsData = new arStructuredData(d, "points");
  textureData = new arStructuredData(d, "texture");
  boundingSphereData = new arStructuredData(d, "bounding sphere");
  billboardData = new arStructuredData(d, "billboard");
  visibilityData = new arStructuredData(d, "visibility");
  viewerData = new arStructuredData(d, "viewer");
  blendData = new arStructuredData(d, "blend");
  normal3Data = new arStructuredData(d, "normal3");
  color4Data = new arStructuredData(d, "color4");
  tex2Data  = new arStructuredData(d, "tex2");
  indexData = new arStructuredData(d, "index");
  drawableData = new arStructuredData(d, "drawable");
  lightData = new arStructuredData(d, "light");
  materialData = new arStructuredData(d, "material");
  perspCameraData = new arStructuredData(d, "persp camera");
  bumpMapData = new arStructuredData(d, "bump map");

  if (!transformData      || !*transformData ||
      !pointsData         || !*pointsData ||
      !textureData        || !*textureData ||
      !boundingSphereData || !*boundingSphereData ||
      !billboardData      || !*billboardData ||
      !visibilityData     || !*visibilityData ||
      !viewerData         || !*viewerData ||
      !blendData          || !*blendData ||
      !normal3Data        || !*normal3Data ||
      !color4Data         || !*color4Data ||
      !tex2Data           || !*tex2Data ||
      !indexData          || !*indexData ||
      !drawableData       || !*drawableData ||
      !lightData          || !*lightData ||
      !materialData       || !*materialData ||
      !perspCameraData    || !*perspCameraData ||
      !bumpMapData        || !*bumpMapData){
    cerr << "arGraphicsDatabase error: incomplete dictionary.\n";
    // Destructor may crash if it deletes a NULL pointer.
  }

  // initialize the light container
  for (i=0; i<8; i++){
    _lightContainer[i] = pair<int,arLight*>(0,NULL);
  }
  // initialize the camera container
//  _cameraID = -1; // make sure we are set up for the default "VR camera"
  for (i=0; i<8; i++){
    _cameraContainer[i] = pair<int,arPerspectiveCamera*>(0,NULL);
  }
}

/// \todo Lots more deleting should really be done here, e.g. the font.
arGraphicsDatabase::~arGraphicsDatabase(){
  delete transformData;
  delete pointsData;
  delete boundingSphereData;
  delete billboardData;
  delete visibilityData;
  delete viewerData;
  delete blendData;
  delete normal3Data;
  delete color4Data;
  delete tex2Data;
  delete indexData;
  delete drawableData;
  delete lightData;
  delete materialData;
  delete perspCameraData;
  delete bumpMapData;
}

arDatabaseNode* arGraphicsDatabase::alter(arStructuredData* inData){
  return arDatabase::alter(inData);
}

void arGraphicsDatabase::reset(){
  // Call base class to do that cleaning.
  arDatabase::reset();

  // reset light container
  // NOTE: there is a memory leak here! Not deleting the known lights!
  // This is bad. Maybe the lights need to be owned by the database,
  // just like the textures?
  // initialize the light container
  int j;
  for (j=0; j<8; j++){
    _lightContainer[j] = pair<int,arLight*>(0,NULL);
  }
  // rset camera container
  // NOTE: there is a memory leak here! Not deleting the known camera!
  // This is bad!
  for (j=0; j<8; j++){
    _cameraContainer[j] = pair<int,arPerspectiveCamera*>(0,NULL);
  }

  // Delete textures.
  for (map<string,arTexture*,less<string> >::iterator i
        (_textureNameContainer.begin());
       i != _textureNameContainer.end();
       ++i){
    if (i->second) {
      delete i->second;
      i->second = NULL;
    }
  }
  _textureNameContainer.clear();
}

// ARRGH! these alphabet-handling functions just flat-out *suck*
// hopefully, I'll be able to try again later

void arGraphicsDatabase::loadAlphabet(const char* path){
  // If our arDatabase is a server, not a client, then we don't do anything
  // with these files.
  if (_server)
    return;

  char buffer[256];
  sprintf(buffer, "%s..ppm", path); // first '.' overwritten by letter
  const int pathLength = strlen(path);
  for (int i=0; i<26; i++){
    buffer[pathLength] = 'A' + i;
    _alphabet[i] = new arTexture;
    // NOTE: the -1 means that no pixel becomes transparent and the false
    // means that no complaint will be printed if the file fails to be read.
    // We actually want no printed complaints!
    _alphabet[i]->readPPM(buffer, -1, false);
  }
}

arTexture** arGraphicsDatabase::getAlphabet(){
  return _alphabet; // Dangerous!  Returns a pointer to a private member.
}

void arGraphicsDatabase::setTexturePath(const string& thePath){
  // this is probably called in a different thread from the data handling
  ar_mutex_lock(&_texturePathLock);

  // Delete the _texturePath object and create a new one.
  delete _texturePath;
  _texturePath = new list<string>;

  // Parse the path.
  int nextChar = 0;
  int length = thePath.length();

  string result(""); // always search local directory
  _texturePath->push_back(result);

  while (nextChar < length){
    result = ar_pathToken(thePath, nextChar); // updates nextChar
    if (result == "NULL")
      continue;
    _texturePath->push_back(ar_pathAddSlash(result));
  }
  ar_mutex_unlock(&_texturePathLock);
}

arTexture* arGraphicsDatabase::addTexture(const string& name, int* theAlpha){
  const map<string,arTexture*,less<string> >::iterator
    iFind(_textureNameContainer.find(name));
  if (iFind != _textureNameContainer.end())
    return iFind->second;

  // A new texture.
  arTexture* theTexture = new arTexture;
  if (name.length() <= 0) {
    cerr << "arGraphicsDatabase warning: "
	 << "ignoring empty filename for texture.\n";
  }
  // Only client, not server, needs the actual bitmap
  else if (!isServer()) {
    // Try everything in the path.
    ar_mutex_lock(&_texturePathLock);
    bool fDone = false;
    string potentialFileName;
    // First, go ahead and look at the bundle path, if such as been set.
    map<string, string, less<string> >::iterator iter 
        = _bundlePathMap.find(_bundlePathName);
    if (_bundlePathName != "NULL" && _bundleName != "NULL"
	&& iter != _bundlePathMap.end()){
      arSemicolonString bundlePath(iter->second);
      for (int n=0; n<bundlePath.size(); n++){
        potentialFileName = bundlePath[n];
        ar_pathAddSlash(potentialFileName);
        potentialFileName += _bundleName;
        ar_pathAddSlash(potentialFileName);
        potentialFileName += name;
        // Scrub path afterwards to allow cross-platform multi-level 
	// bundle names (foo/bar), which can be changed per platform
	// to the right thing (on Windows, foo\bar)
        ar_scrubPath(potentialFileName);
        fDone = theTexture->readImage(potentialFileName.c_str(), *theAlpha,
				      false);
        if (fDone){
	  // Don't look anymore. Success!
	  break;
	}
      }
    }

    // If nothing can be found there, go ahead and look at the texture
    // path.
    for (list<string>::iterator i = _texturePath->begin();
	 !fDone && i != _texturePath->end();
	 ++i){
      potentialFileName = *i + name;
      ar_scrubPath(potentialFileName);
      // Make sure the texture function does not complain (the final
      // false parameter does this). Otherwise, ugly error messages will
      // pop up. Note: it is natural that there be some complaints! Since
      // we are testing for the existence of the file on the texture path,
      // starting with the current working directory, by doing this.
      fDone = theTexture->readImage(potentialFileName.c_str(), *theAlpha, 
                                    false);
    }
    static bool fComplained = false;
    if (!fDone){
      theTexture->dummy();
      if (!fComplained){
	fComplained = true;
	cerr << "arGraphicsDatabase warning: no graphics file \""
	     << name << "\" in ";
	if (_texturePath->size() <= 1)
	  cerr << "empty ";
	cerr << "texture path." << endl;
      }
    }
    ar_mutex_unlock(&_texturePathLock);
  }
  _textureNameContainer.insert(
    map<string,arTexture*,less<string> >::value_type(name,theTexture));
  return theTexture; 
}

arBumpMap* arGraphicsDatabase::addBumpMap(const string& name,
		int numPts, int numInd, float* points,
		int* index, float* tex2, float height, arTexture* decalTexture){
/// @todo Let textureNameContainer hold either glNames or both arBumpMap
///       and arTexture nodes, so we are not re-using the same texture
///       memory for bump and texture maps...
///       (Actually, a separate bumpNameContainer should be enough...)
  // A new bump map.
  const int numTBN = index ? numInd : numPts;
  //printf("arGraphicsDatabase: Create new bump map\n");
  arBumpMap* theBumpMap = new arBumpMap();
  theBumpMap->setDecalTexture(decalTexture);
  //printf("arGraphicsDatabase: set bumpmap height\n");
  theBumpMap->setHeight(height);
  //printf("arGraphicsDatabase: set bumpmap PIT\n");
  theBumpMap->setPIT(numPts, numInd, points, index, tex2);
  //printf("arGraphicsDatabase: force bumpmap generate frames\n");
  theBumpMap->generateFrames(numTBN);
  char buffer[512];
  ar_stringToBuffer(name, buffer, sizeof(buffer));
  if (strlen(buffer) <= 0) {
    cerr << "arGraphicsDatabase warning: "
	 << "ignoring empty filename for PPM bump texture.\n";
  }
  // Only client, not server, needs the actual bitmap.
  else if (!isServer()) {
    /// \todo factor out copypasting between this and texture map code
    // Try everything in the path.
    ar_mutex_lock(&_texturePathLock);
    char fileNameBuffer[512];

    bool fDone = false;
    for (list<string>::iterator i = _texturePath->begin();
	 !fDone && i != _texturePath->end();
	 ++i){
      const string tmp(*i + buffer);
      ar_stringToBuffer(tmp, fileNameBuffer, sizeof(fileNameBuffer));
      fDone = theBumpMap->readPPM(fileNameBuffer, 1);
    }
    static bool fComplained = false;
    if (!fDone){
      theBumpMap->dummy();
      if (!fComplained){
	fComplained = true;
	cerr << "arGraphicsDatabase warning: no PPM file \""
	     << buffer << "\" in ";
	if (_texturePath->size() <= 1)
	  cerr << "empty ";
	cerr << "bump path." << endl;
      }
    }
    ar_mutex_unlock(&_texturePathLock);
  }
 /* _textureNameContainer.insert(
    map<string,arTexture*,less<string> >::value_type(name,theBumpMap));
 */
  return theBumpMap; 
}



arMatrix4 arGraphicsDatabase::accumulateTransform(int nodeID){
  arMatrix4 result = ar_identityMatrix();
  arDatabaseNode* thisNode = getNode(nodeID);
  if (!thisNode){
    cerr << "arGraphicsDatabase error: accumulateTransform was passed "
	 << "an invalid node ID.\n";
    return result;
  }
  thisNode = thisNode->getParent();
  while (thisNode->getID() != 0){
    if (thisNode->getTypeCode() == AR_G_TRANSFORM_NODE){
      arTransformNode* transformNode = (arTransformNode*) thisNode;
      result = transformNode->getTransform()*result;
    }
    thisNode = thisNode->getParent();
  }
  return result;
}

arMatrix4 arGraphicsDatabase::accumulateTransform(int startNodeID, int endNodeID) {
  return accumulateTransform(startNodeID).inverse() * accumulateTransform(endNodeID);
}

arTexture* arGraphicsDatabase::addTexture(int w, int h, 
                                          bool alpha, const char* pixels){
  arTexture* t = new arTexture;
  t->fill(w, h, alpha, pixels);
  return t;
}

//void arGraphicsDatabase::setViewTransform(arScreenObject* screenObject,
//                                          float eyeSign){
////   When eyeSign = -1, we're talking about the left eye. When eyeSign = 1,
////   we're talking about the right eye.  Consequently, eyeOffset is the
////   vector from midEyeOffset (between the eyes) to the right eye!

//  arMatrix4 headMatrix(ar_identityMatrix());
//  arVector3 midEyeOffset(0,0,0);
//  arVector3 eyeDirection(1,0,0);
//  float eyeSpacing = 0.;
//  float nearClip = 1.;
//  float farClip = 1000.;
//  float unitConversion = 1.;

//  arDatabaseNode* viewerNode = getNode("szg_viewer");
//  if (_cameraID == -2){
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    glFrustum(_localCameraFrustum[0], _localCameraFrustum[1], 
//              _localCameraFrustum[2], _localCameraFrustum[3], 
//              _localCameraFrustum[4], _localCameraFrustum[5]);
//    glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();
//      
//    gluLookAt(_localCameraLookat[0], _localCameraLookat[1], 
//              _localCameraLookat[2], _localCameraLookat[3], 
//              _localCameraLookat[4], _localCameraLookat[5],
//              _localCameraLookat[6], _localCameraLookat[7], 
//              _localCameraLookat[8]);
//  }
//  else if (_cameraID == -1 || !_cameraContainer[_cameraID].second){
////     if we are set to be *not* the default camera but there is no referenced
////     camera, then set camera back to default
//    _cameraID = -1;
////     this is the default "VR camera"
//    if (viewerNode) {
//      arHead head = ((arViewerNode*)viewerNode)->getHead();
//      headMatrix = head.transform;
//      midEyeOffset = head.midEyeOffset;
//      eyeDirection = head.eyeDirection;
//      eyeSpacing = head.eyeSpacing;
//      nearClip = head.nearClip;
//      farClip = head.farClip;
//      unitConversion = head.unitConversion;
//    }

//    screenObject->setViewTransform(
//      nearClip, farClip, unitConversion, eyeSpacing, 
//      midEyeOffset, eyeDirection);
//    screenObject->loadViewMatrices( eyeSign, headMatrix );
//  }
//  else{
//    _cameraContainer[_cameraID].second->loadViewMatrices();
//    arMatrix4 cameraTransform 
//      = !accumulateTransform(_cameraContainer[_cameraID].first);
//    glMultMatrixf(cameraTransform.v);
//  }
//}

void arGraphicsDatabase::draw(){
  // replaces gl matrix stack... we want to support VERY deep trees
  stack<arMatrix4> transformStack;
  ar_mutex_lock(&_eraseLock);
  _draw((arGraphicsNode*)&_rootNode, transformStack);
  ar_mutex_unlock(&_eraseLock);
}

void arGraphicsDatabase::_draw(arGraphicsNode* node, 
			       stack<arMatrix4>& transformStack){
  arMatrix4 tempMatrix;
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    // Push current onto the matrix stack.
    glGetFloatv(GL_MODELVIEW_MATRIX, tempMatrix.v);
    transformStack.push(tempMatrix);
  }
  // Draw the node and its children except:
  //   a. If this is the root node, do not draw (no draw method)
  //   b. If this is a visibility node in invisible state, do not
  //      draw either node or children.
  if (node->getTypeCode() != -1 && node->getTypeCode() != AR_D_NAME_NODE){
    // We are not the root node or a name node, so it is OK to draw.
    node->draw();
  }
  if ( !(node->getTypeCode() == AR_G_VISIBILITY_NODE 
         && !((arVisibilityNode*)node)->getVisibility() ) ){
    // We are not a visibility node in an invisible state. It is OK to draw
    // the children.
    list<arDatabaseNode*> children = node->getChildren();
    for (list<arDatabaseNode*>::iterator i = children.begin();
	 i != children.end(); i++){
      _draw((arGraphicsNode*)(*i), transformStack);
    }
  }

  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    // Pop from stack.
    tempMatrix = transformStack.top();
    transformStack.pop();
    // Return the matrix state to what it was when we were called.
    glLoadMatrixf(tempMatrix.v);
  }
}

/// \todo Space-partitioning would speed up intersection testing.
int arGraphicsDatabase::intersect(const arRay& theRay){
  float bestDistance = -1;
  int bestNodeID = -1;
  stack<arRay> rayStack;
  rayStack.push(theRay);
  _intersect((arGraphicsNode*)&_rootNode, bestDistance, bestNodeID, rayStack);
  return bestNodeID;
}

void arGraphicsDatabase::_intersect(arGraphicsNode* node,
                                    float& bestDistance, 
                                    int& bestNodeID,
                                    stack<arRay>& rayStack){
  // If this is a transform node, go ahead and transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    arMatrix4 theMatrix = ((arTransformNode*)node)->getTransform();
    arRay currentRay = rayStack.top();
    rayStack.push(arRay((!theMatrix)*currentRay.getOrigin(),
			(!theMatrix)*currentRay.getDirection()
                        - (!theMatrix)*arVector3(0,0,0)));
  }
  // If it is a bounding sphere, go ahead and intersect.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE){
    arBoundingSphere sphere 
      = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arRay intRay(rayStack.top());
    float distance = intRay.intersect(sphere.radius, 
				      sphere.position);
    if (distance>0){
      // intersection
      if (bestDistance < 0 || distance < bestDistance){
        bestDistance = distance;
	bestNodeID = node->getID();
      }
    }
  }
  // Deal with intersections with children
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _intersect((arGraphicsNode*)(*i), bestDistance, bestNodeID, rayStack);
  }
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    rayStack.pop();
  }
}

/// \todo Space-partitioning would speed up intersection testing.
list<int>* arGraphicsDatabase::intersectList(const arRay& theRay){
  list<int>* result = new list<int>;
  stack<arRay> rayStack;
  rayStack.push(theRay);
  _intersectList((arGraphicsNode*)&_rootNode, result, rayStack);
  return result;
}

void arGraphicsDatabase::_intersectList(arGraphicsNode* node,
                                        list<int>* result,
                                        stack<arRay>& rayStack){
  // If this is a transform node, go ahead and transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    arMatrix4 theMatrix = ((arTransformNode*)node)->getTransform();
    arRay currentRay = rayStack.top();
    rayStack.push(arRay((!theMatrix)*currentRay.getOrigin(),
			(!theMatrix)*currentRay.getDirection()
                        - (!theMatrix)*arVector3(0,0,0)));
  }
  // If it is a bounding sphere, go ahead and intersect.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE){
    arBoundingSphere sphere 
      = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arRay intRay(rayStack.top());
    float distance = intRay.intersect(sphere.radius, 
				      sphere.position);
    if (distance>0){
      // intersection
      result->push_back(node->getID());
    }
  }
  // Deal with intersections with children
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _intersectList((arGraphicsNode*)(*i), result, rayStack);
  }
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    rayStack.pop();
  }
}

bool arGraphicsDatabase::registerLight(int owningNodeID, arLight* theLight){
  if (!theLight){
    cerr << "arGraphicsDatabase error: light pointer does not exist.\n";
    return false;
  }
  if (theLight->lightID < 0 || theLight->lightID > 7){
    cerr << "arGraphicsDatabase error: light has invalid ID.\n";
    return false;
  }
  if (!getNode(owningNodeID)){
    cerr << "arGraphicsDatabase error: registerLight(...) failed because "
	 << "of invalid node ID.\n";
    return false;
  }
  _lightContainer[theLight->lightID] 
    = pair<int,arLight*>(owningNodeID,theLight);
  return true;
}

void arGraphicsDatabase::activateLights(){
  for (int i=0; i<8; i++){
    if (_lightContainer[i].second){
      // a light has been registered for this ID
      arMatrix4 lightPositionTransform 
	= accumulateTransform(_lightContainer[i].first);
      _lightContainer[i].second->activateLight(lightPositionTransform);
    }
  }
}

arHead* arGraphicsDatabase::getHead() {
  arViewerNode* viewerNode = (arViewerNode*)getNode("szg_viewer");
  if (!viewerNode) {
    cerr << "arGraphicsDatabase error: getHead() failed.\n";
    return 0;
  }
  return viewerNode->getHead();
}

bool arGraphicsDatabase::registerCamera(int owningNodeID,
					arPerspectiveCamera* theCamera){
  if (!theCamera){
    cerr << "arGraphicsDatabase error: camera pointer does not exist.\n";
    return false;
  }
  if (theCamera->cameraID < 0 || theCamera->cameraID > 7){
    cerr << "arGraphicsDatabase error: camera has invalid ID.\n";
    return false;
  }
  if (!getNode(owningNodeID)){
    cerr << "arGraphicsDatabase error: registerCamera(...) failed because "
	 << "of invalid node ID.\n";
    return false;
  }
  _cameraContainer[theCamera->cameraID]
    = pair<int,arPerspectiveCamera*>(owningNodeID,theCamera);
  return true;
}

arPerspectiveCamera* arGraphicsDatabase::getCamera( unsigned int cameraID ) {
  if (cameraID > 7){
    cerr << "arGraphicsDatabase error: invlid camera ID.\n";
    return 0;
  }
  return _cameraContainer[cameraID].second;
}

//bool arGraphicsDatabase::setCamera(int cameraID){
//  if (cameraID < -1 || cameraID > 7){
//    cerr << "arGraphicsDatabase error: invlid camera ID.\n";
//    return false;
//  }
//  _cameraID = cameraID;
//  return true;
//}

//void arGraphicsDatabase::setLogetUnitConversion()(float* frustum, float* lookat){
//  memcpy(_localCameraFrustum,frustum,6*sizeof(float));
//  memcpy(_localCameraLookat,lookat,9*sizeof(float));
//  _cameraID = -2;
//}

arDatabaseNode* arGraphicsDatabase::_makeNode(const string& type){
  arDatabaseNode* outNode = NULL;
  // The node could be natively handles by the arDatabase class
  outNode = arDatabase::_makeNode(type);
  if (outNode){
    // this was a base node type.
    return outNode;
  }
  // Perhaps it will be one of the arGraphicsDatabase nodes...
  if (type=="transform"){
    outNode = (arDatabaseNode*) new arTransformNode();
  }
  else if (type=="points"){
    outNode = (arDatabaseNode*) new arPointsNode(this);
  }
  else if (type=="texture"){
    outNode = (arDatabaseNode*) new arTextureNode();
  }
  else if (type=="bounding sphere"){
    outNode = (arDatabaseNode*) new arBoundingSphereNode();
  }
  else if (type=="billboard"){
    outNode = (arDatabaseNode*) new arBillboardNode();
  }
  else if (type=="visibility"){
    outNode = (arDatabaseNode*) new arVisibilityNode();
  }
  else if (type=="viewer"){
    outNode = (arDatabaseNode*) new arViewerNode();
  }
  else if (type=="blend"){
    outNode = (arDatabaseNode*) new arBlendNode();
  }
  else if (type == "normal3"){
    outNode = (arDatabaseNode*) new arNormal3Node(this);
  }
  else if (type == "color4"){
    outNode = (arDatabaseNode*) new arColor4Node(this);
  }
  else if (type == "tex2"){
    outNode = (arDatabaseNode*) new arTex2Node(this);
  }
  else if (type == "index"){
    outNode = (arDatabaseNode*) new arIndexNode(this);
  }
  else if (type == "drawable"){
    outNode = (arDatabaseNode*) new arDrawableNode();
  }
  else if (type == "light"){
    outNode = (arDatabaseNode*) new arLightNode();
  }
  else if (type == "material"){
    outNode = (arDatabaseNode*) new arMaterialNode();
  }
  else if (type == "persp camera"){
    outNode = (arDatabaseNode*) new arPerspectiveCameraNode();
  }
  else if (type == "bump map"){
    outNode = (arDatabaseNode*) new arBumpMapNode();
  }
  else{
    cerr << "arGraphicsDatabase error: makeNode factory got unknown type="
         << type << ".\n";
    return NULL;
  }

  return outNode;
}

arDatabaseNode* arGraphicsDatabase::_processAdmin(arStructuredData* data){
  string name = data->getDataString("name");
  cout << "arGraphicsDatabase remark: using texture bundle " << name << "\n";
  arSlashString bundleInfo(name);
  if (bundleInfo.size() != 2){
    cout << "arGraphicsDatabase error: got garbled texture bundle "
	 << "identification.\n";
    return &_rootNode;
  }
  setBundlePtr(bundleInfo[0], bundleInfo[1]);
  return &_rootNode;
}

