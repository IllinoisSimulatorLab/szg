//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arGraphicsDatabase::arGraphicsDatabase() :
  _texturePath(new list<string>),
  _alphabet(new arTexture*[26]),
  _viewerNodeID(-1)
{
  _lang = (arDatabaseLanguage*)&_gfx;
  if (!_initDatabaseLanguage())
    return;

  // Add the processing callback for the "graphics admin"
  // message. The arGraphicsPeer processes this
  // differently than here (i.e. the "graphics admin" messages 
  // are culled from the message stream before being sent to the
  // database for processing.
  arDataTemplate* t = _lang->find("graphics admin");
  _databaseReceive[t->getID()] =
    (arDatabaseProcessingCallback)&arGraphicsDatabase::_processAdmin;
  
  // Initialize the texture data.
  memset(_alphabet, 0, 26 * sizeof(arTexture*));

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
  graphicsStateData = new arStructuredData(d, "graphics state");

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
      !bumpMapData        || !*bumpMapData ||
      !graphicsStateData  || !*graphicsStateData){
    cerr << "arGraphicsDatabase error: incomplete dictionary.\n";
  }

  // initialize the light container
  int i = 0;
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
  if (transformData){
    delete transformData;
  }
  if (pointsData){
    delete pointsData;
  }
  if (boundingSphereData){
    delete boundingSphereData;
  }
  if (billboardData){
    delete billboardData;
  }
  if (visibilityData){
    delete visibilityData;
  }
  if (viewerData){
    delete viewerData;
  }
  if (blendData){
    delete blendData;
  }
  if (normal3Data){
    delete normal3Data;
  }
  if (color4Data){
    delete color4Data;
  }
  if (tex2Data){
    delete tex2Data;
  }
  if (indexData){
    delete indexData;
  }
  if (drawableData){
    delete drawableData;
  }
  if (lightData){
    delete lightData;
  }
  if (materialData){
    delete materialData;
  }
  if (perspCameraData){
    delete perspCameraData;
  }
  if (bumpMapData){
    delete bumpMapData;
  }
  if (graphicsStateData){
    delete graphicsStateData;
  }
  
  // Don't forget to get rid of the textures. However, deleting them isn't
  // so smart. Instead, unref and let that operator delete if no-one else
  // is holding a reference.
  for (map<string,arTexture*,less<string> >::iterator 
       i = _textureNameContainer.begin(); i != _textureNameContainer.end(); 
       i++){
    i->second->unref();
  }
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
  int j = 0;
  for (j=0; j<8; j++){
    _lightContainer[j] = pair<int,arLight*>(0,NULL);
  }
  // rset camera container
  // NOTE: there is a memory leak here! Not deleting the known camera!
  // This is bad!
  for (j=0; j<8; j++){
    _cameraContainer[j] = pair<int,arPerspectiveCamera*>(0,NULL);
  }

  // Unref the textures. Do not delete them. They will be automatically
  // deleted if no other object has ref-ed them.
  for (map<string,arTexture*,less<string> >::iterator i
        (_textureNameContainer.begin());
       i != _textureNameContainer.end();
       ++i){
    if (i->second) {
      i->second->unref();
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

// Creates a new texture and then refs it before returning. Consequently,
// the caller is responsible for unref'ing to prevent a memory leak.
arTexture* arGraphicsDatabase::addTexture(const string& name, int* theAlpha){
  const map<string,arTexture*,less<string> >::iterator
    iFind(_textureNameContainer.find(name));
  if (iFind != _textureNameContainer.end()){
    // MUST ref the texture!
    iFind->second->ref();
    return iFind->second;
  }

  // A new texture.
  arTexture* theTexture = new arTexture;
  // The default for the arTexture object is to use GL_DECAL mode, but we
  // want LIT textures.
  theTexture->setTextureFunc(GL_MODULATE);
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
	theTexture->mipmap(true);
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
      theTexture->mipmap(true);
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
  // NOTE: It is very important to ref this texture again for its return.
  // This way, the arTextureNode (which is who called addTexture) can unref it
  // on deletion or texture change.
  theTexture->ref();
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

void arGraphicsDatabase::setVRCameraID(int cameraID){
  arStructuredData cameraData(_lang->find("graphics admin"));
  cameraData.dataInString("action", "camera_node");
  cameraData.dataIn("node_ID", &cameraID, AR_INT, 1);
  // This admin message *must* be passed on in the case of an arGraphicsServer.
  alter(&cameraData);
}

void arGraphicsDatabase::draw(arMatrix4* projectionCullMatrix){
  // replaces gl matrix stack... we want to support VERY deep trees
  stack<arMatrix4> transformStack;
  ar_mutex_lock(&_eraseLock);
  arGraphicsContext context;
  // Not using graphics context yet.
  _draw((arGraphicsNode*)&_rootNode, transformStack, &context,
        projectionCullMatrix);
  ar_mutex_unlock(&_eraseLock);
}

void arGraphicsDatabase::_draw(arGraphicsNode* node, 
			       stack<arMatrix4>& transformStack,
                               arGraphicsContext* context,
			       arMatrix4* projectionCullMatrix){

  // Word of warning: lock/unlock is DEFINITELY costly when done per-node.
  // To make the API really thread-safe, some kind of node-level locking
  // is necessary... but it clearly should be a bit constrained (10 node
  // locks/unlocks per node per draw is a 25% performance hit over no locks
  // at all (when drawing a scene graph with a high nodes/geometry ratio...
  // i.e. where scene graph traversal is significant).

  // Store the node on the arGraphicsContext's stack.
  if (context){
    context->pushNode(node);
  }
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
    // These nodes are actually just arDatabaseNodes instead of
    // arGraphicsNodes.
    node->draw(context);
  }
  // Deal with view frustum culling.
  if (projectionCullMatrix && node->getTypeCode() 
      == AR_G_BOUNDING_SPHERE_NODE){
    glGetFloatv(GL_MODELVIEW_MATRIX, tempMatrix.v);
    arBoundingSphere b = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arMatrix4 view = (*projectionCullMatrix)*tempMatrix;
    if (!b.intersectViewFrustum(view)){
      // It is safe to return here... but don't forget to pop the node stack!
      if (context){
        context->popNode(node);
      }
      return;
    }
  }
  // Deal with visibility nodes.
  if ( !(node->getTypeCode() == AR_G_VISIBILITY_NODE 
         && !((arVisibilityNode*)node)->getVisibility() ) ){
    // We are not a visibility node in an invisible state. It is OK to draw
    // the children.
    list<arDatabaseNode*> children = node->getChildren();
    for (list<arDatabaseNode*>::iterator i = children.begin();
	 i != children.end(); i++){
      _draw((arGraphicsNode*)(*i), transformStack, context, 
            projectionCullMatrix);
    }
  }

  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    // Pop from stack.
    tempMatrix = transformStack.top();
    transformStack.pop();
    // Return the matrix state to what it was when we were called.
    glLoadMatrixf(tempMatrix.v);
  }
  // Must remember to pop the node stack upon leaving this function.
  if (context){
    context->popNode(node);
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

arGraphicsNode* arGraphicsDatabase::intersectGeometry(const arRay& theRay,
                                                      int excludeBelow){
  stack<arRay> rayStack;
  rayStack.push(theRay);
  arGraphicsContext context;
  arGraphicsNode* bestNode = NULL;
  float bestDistance = -1;
  _intersectGeometry((arGraphicsNode*)&_rootNode, &context, rayStack,
		     excludeBelow, bestNode, bestDistance);
  return bestNode;
}

float arGraphicsDatabase::_intersectSingleGeometry(arGraphicsNode* node,
                                                   arGraphicsContext* context,
                                                   const arRay& theRay){
  if (!node || !context){
    return -1;
  }
  if (node->getTypeCode() != AR_G_DRAWABLE_NODE){
    return -1;
  }
  arDrawableNode* d = (arDrawableNode*) node;
  if (d->getType() != DG_TRIANGLES){
    cout << "arGraphicsDatabase warning: only able to intersect with triangle "
	 << "soups so far.\n";
    return -1;
  }
  arGraphicsNode* p = (arGraphicsNode*) context->getNode(AR_G_POINTS_NODE);
  if (!p){
    cout << "arGraphicsDatabase error: no points node for drawable.\n";
    return -1;
  }
  arGraphicsNode* i = (arGraphicsNode*) context->getNode(AR_G_INDEX_NODE);
  int number = d->getNumber();
  float* points = p->getBuffer();
  int* index = NULL;
  if (i){
    index = (int*)i->getBuffer();
  }
  float bestDistance = -1;
  float dist;
  arVector3 a, b, c;
  for (int j = 0; j < number; j++){
    if (index){
      a = arVector3(points + 3*index[3*j]);
      b = arVector3(points + 3*index[3*j+1]);
      c = arVector3(points + 3*index[3*j+2]);
    }
    else{
      a = arVector3(points + 9*j);
      b = arVector3(points + 9*j+3);
      c = arVector3(points + 9*j+6);
    }
    dist = ar_intersectRayTriangle(theRay.getOrigin(),
				   theRay.getDirection(),
				   a, b, c);
    if (dist >= 0 && (bestDistance < 0 || dist < bestDistance)){
      bestDistance = dist;
    }
  }
  return bestDistance;
}

void arGraphicsDatabase::_intersectGeometry(arGraphicsNode* node,
                                            arGraphicsContext* context,
                                            stack<arRay>& rayStack,
                                            int excludeBelow,
                                            arGraphicsNode*& bestNode,
                                            float bestDistance){
  if (node->getID() == excludeBelow){
    return;
  }
  // Store the node on the arGraphicsContext's stack.
  if (context){
    context->pushNode(node);
  }
  // If this is a transform node, go ahead and transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    arMatrix4 theMatrix = ((arTransformNode*)node)->getTransform();
    arRay currentRay = rayStack.top();
    rayStack.push(arRay((!theMatrix)*currentRay.getOrigin(),
			(!theMatrix)*currentRay.getDirection()
                        - (!theMatrix)*arVector3(0,0,0)));
  }
  // If it is a bounding sphere, go ahead and intersect.
  // If we do not intersect, return.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE){
    arBoundingSphere sphere 
      = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arRay intRay(rayStack.top());
    float distance = intRay.intersect(sphere.radius, 
				      sphere.position);
    if (distance < 0){
      // Must remember to pop the node stack upon leaving this function.
      if (context){
        context->popNode(node);
      }
      return;
    }
  }
  // If it is a drawable node, go ahead and intersect.
  if (node->getTypeCode() == AR_G_DRAWABLE_NODE){
    arRay localRay = rayStack.top();
    float rawDist = _intersectSingleGeometry(node, context, localRay);
    
    if (rawDist >= 0){
      // NOTE: there can be scaling. So, consequently, we DO NOT yet know how
      // far, in global terms, the intersection is from the ray origin.
      arMatrix4 toGlobal = accumulateTransform(node->getID());
      // Take two points on the ray, the origin and the intersection in the
      // local coordinate frame. Transform these to the global coordinate
      // system and find the distance.
      arVector3 v1 = toGlobal*localRay.getOrigin();
      arVector3 v2 = toGlobal*(localRay.getOrigin() + rawDist*(localRay.getDirection().normalize()));
      float dist = ++(v1-v2);
      if (bestDistance < 0 || dist < bestDistance){
        bestNode = node;
        bestDistance = dist;
      }
    }
  }
  // Deal with intersections with children
  list<arDatabaseNode*> children = node->getChildren();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _intersectGeometry((arGraphicsNode*)(*i), context, rayStack, 
                       excludeBelow, bestNode, bestDistance);
  }
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    rayStack.pop();
  }
  // Must remember to pop the node stack upon leaving this function.
  if (context){
    context->popNode(node);
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
  // This is a bit of a HACK! Somehow, we neeed to know where the head
  // description for the VR camera is located.
  arViewerNode* viewerNode = NULL;
  if (_viewerNodeID != -1){
    viewerNode = (arViewerNode*) getNode(_viewerNodeID);
  }
  if (!viewerNode){
    viewerNode = (arViewerNode*) getNode("szg_viewer");
  }
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
    outNode = (arDatabaseNode*) new arPointsNode();
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
    outNode = (arDatabaseNode*) new arNormal3Node();
  }
  else if (type == "color4"){
    outNode = (arDatabaseNode*) new arColor4Node();
  }
  else if (type == "tex2"){
    outNode = (arDatabaseNode*) new arTex2Node();
  }
  else if (type == "index"){
    outNode = (arDatabaseNode*) new arIndexNode();
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
  else if (type == "graphics state"){
    outNode = (arDatabaseNode*) new arGraphicsStateNode();
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
  string action = data->getDataString("action");
  if (action == "remote_path"){
    cout << "arGraphicsDatabase remark: using texture bundle " << name << "\n";
    arSlashString bundleInfo(name);
    if (bundleInfo.size() != 2){
      cout << "arGraphicsDatabase error: got garbled texture bundle "
	   << "identification.\n";
      return &_rootNode;
    }
    setDataBundlePath(bundleInfo[0], bundleInfo[1]);
  }
  else if (action == "camera_node"){
    int nodeID = data->getDataInt("node_ID");
    // If we have a node with this ID, then go ahead and set the camera to it.
    arDatabaseNode* node = getNode(nodeID);
    if (node && node->getTypeString() == "viewer"){
      _viewerNodeID = nodeID;
    }
    else{
      cout << "arGraphicsDatabase warning: no viewer node with "
	   << "ID=" << nodeID << ".\n";
    }
  }
  return &_rootNode;
}

