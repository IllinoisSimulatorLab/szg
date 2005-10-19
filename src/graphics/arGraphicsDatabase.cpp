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

  // Initialize the light container.
  int i = 0;
  for (i=0; i<8; i++){
    _lightContainer[i] = pair<arGraphicsNode*,arLight*>(NULL,NULL);
  }
  // Initialize the camera container.
  for (i=0; i<8; i++){
    _cameraContainer[i] 
      = pair<arGraphicsNode*,arPerspectiveCamera*>(NULL,NULL);
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
  // so smart. Instead, unref and let that operator delete if no one else
  // is holding a reference.
  for (map<string,arTexture*,less<string> >::iterator 
       i = _textureNameContainer.begin(); i != _textureNameContainer.end(); 
       i++){
    i->second->unref();
  }
}

arDatabaseNode* arGraphicsDatabase::alter(arStructuredData* inData,
                                          bool refNode){
  return arDatabase::alter(inData, refNode);
}

void arGraphicsDatabase::reset(){
  // Call base class to do that cleaning.
  arDatabase::reset();

  // The light container and the camera container are automatically cleared
  // by arDatabase::reset() and the deactivate methods of arLightNode and
  // arPerspectiveCameraNode.

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

/// Figure out the total transformation matrix from ABOVE the current node
/// to the database's root. This call is thread-safe with respect to 
/// database operations (it uses hidden global database locks). Consequently,
/// this cannot be called from any message handling code.
arMatrix4 arGraphicsDatabase::accumulateTransform(int nodeID){
  arMatrix4 result = ar_identityMatrix();
  arDatabaseNode* thisNode = getNodeRef(nodeID);
  if (!thisNode){
    cerr << "arGraphicsDatabase error: accumulateTransform was passed "
	 << "an invalid node ID.\n";
    return result;
  }
  arDatabaseNode* temp = thisNode->getParentRef();
  // Must release our reference to the node to prevent a memory leak.
  thisNode->unref();
  thisNode = temp;
  while (thisNode && thisNode->getID() != 0){
    if (thisNode->getTypeCode() == AR_G_TRANSFORM_NODE){
      arTransformNode* transformNode = (arTransformNode*) thisNode;
      result = transformNode->getTransform()*result;
    }
    temp = thisNode->getParentRef();
    // Must release our reference to the node to prevent a memory leak.
    thisNode->unref();
    thisNode = temp;
  }
  if (thisNode){
    // Must release our reference to the node to prevent a memory leak.
    thisNode->unref();
  }
  return result;
}

arMatrix4 arGraphicsDatabase::accumulateTransform(int startNodeID, 
                                                  int endNodeID) {
  return accumulateTransform(startNodeID).inverse() 
         * accumulateTransform(endNodeID);
}

void arGraphicsDatabase::setVRCameraID(int cameraID){
  arStructuredData cameraData(_lang->find("graphics admin"));
  cameraData.dataInString("action", "camera_node");
  cameraData.dataIn("node_ID", &cameraID, AR_INT, 1);
  // This admin message *must* be passed on in the case of an arGraphicsServer
  // (i.e. we CANNOT use arGraphicsDatabase::alter here).
  alter(&cameraData);
}

void arGraphicsDatabase::draw(arMatrix4* projectionCullMatrix){
  // Replaces gl matrix stack... we want to support VERY deep trees
  stack<arMatrix4> transformStack;
  arGraphicsContext context;
  // Note how we use a "graphics context" for rendering.
  _draw((arGraphicsNode*)&_rootNode, transformStack, &context,
        projectionCullMatrix);
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

/// Finds the bounding sphere node (if any) with the closest point of
/// intersection to the given ray and returns its ID. If no bounding sphere
/// intersects, return -1 (which is the ID of no node). This method is
/// thread-safe.
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
  // Deal with intersections with children. NOTE: we must use getChildrenRef
  // instead of getChildren for thread-safety.
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _intersect((arGraphicsNode*)(*i), bestDistance, bestNodeID, rayStack);
  }
  // Must unref the nodes to prevent a memory leak.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    rayStack.pop();
  }
}

/// Returns the IDs of all bounding sphere nodes that intersect the given
/// ray. Caller is responsible for deleting the list. Thread-safe.
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
  // Deal with intersections with children. NOTE: we must use
  // getChildrenRef instead of getChildren for thread-safety.
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _intersectList((arGraphicsNode*)(*i), result, rayStack);
  }
  // Must unref the nodes to prevent a memory leak.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    rayStack.pop();
  }
}

/// Intersects a ray with the database. At bounding sphere nodes, if there
/// is no intersection, go ahead and skip that subtree. At drawable nodes
/// (consisting of triangles or quads... incompletely implemented) 
/// intersect the ray with the polygons, figuring out the point of closest
/// intersection. In the end, return a pointer to the geometry node with
/// the closest intersection point or NULL if there is no intersection at
/// all.
///
/// This method is thread-safe.
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

/// A helper function for _intersectGeometry, which is, in turn, a helper
/// function for intersectGeometry. 
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
      arVector3 v2 = toGlobal*(localRay.getOrigin() 
                               +rawDist*(localRay.getDirection().normalize()));
      float dist = ++(v1-v2);
      if (bestDistance < 0 || dist < bestDistance){
        bestNode = node;
        bestDistance = dist;
      }
    }
  }
  // Deal with intersections with children. For thread-safety, hold references
  // to the node pointers,
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); i++){
    _intersectGeometry((arGraphicsNode*)(*i), context, rayStack, 
                       excludeBelow, bestNode, bestDistance);
  }
  // To prevent a memory leak, must release the references.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE){
    rayStack.pop();
  }
  // Must remember to pop the node stack upon leaving this function.
  if (context){
    context->popNode(node);
  }
}

/// Should only be called from arLightNode::receiveData. This guarantees that
/// in cases where thread-safety matters (like arGraphicsServer and 
/// arGraphicsPeer) that _lightContainer will be modified atomically.
bool arGraphicsDatabase::registerLight(arGraphicsNode* node, 
                                       arLight* theLight){
  if (!theLight){
    cerr << "arGraphicsDatabase error: light pointer does not exist.\n";
    return false;
  }
  if (!node || !node->active() || node->getOwner() != this){
    cerr << "arGraphicsDatabase error: node not owned by this database.\n";
    return false;
  }
  if (theLight->lightID < 0 || theLight->lightID > 7){
    cerr << "arGraphicsDatabase error: light has invalid ID.\n";
    return false;
  }
  // If the light ID changed, should remove other instances from the
  // container.
  (void) removeLight(node);
  // Go ahead and put the new light in.
  _lightContainer[theLight->lightID] 
    = pair<arGraphicsNode*,arLight*>(node, theLight);
  return true;
}

/// Should only be called from arLightNode::deactivate and 
/// arGraphicsDatabase::registerLight.
bool arGraphicsDatabase::removeLight(arGraphicsNode* node){
  if (!node || !node->active() || node->getOwner() != this){
    cerr << "arGraphicsDatabase error: node not owned by this database.\n";
    return false;
  }
  for (int i=0; i<8; i++){
    if (_lightContainer[i].first == node){
      _lightContainer[i].first = NULL;
      _lightContainer[i].second = NULL;
    }
  }
  return true;
}

/// Thread-safety with respect to light deletion requires that this
/// call is locked with _databaseLock. Consequently, we must be very careful
/// to guarantee that no call inside also locks the global lock (thus
/// creating a deadlock). Note that accumulateTransform(int) does, in fact,
/// do so, requiring us to use arDatabaseNode::accumulateTransform().
void arGraphicsDatabase::activateLights(){
  ar_mutex_lock(&_databaseLock);
  for (int i=0; i<8; i++){
    if (_lightContainer[i].first){
      // A light has been registered for this ID.
      arMatrix4 lightPositionTransform 
	= _lightContainer[i].first->accumulateTransform();
      _lightContainer[i].second->activateLight(lightPositionTransform);
    }
    else{
      // No light in this slot. Better go ahead and disable.
      const GLenum lights[8] = {
        GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
        GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7
      };
      glDisable(lights[i]);
    }
  }
  ar_mutex_unlock(&_databaseLock);
}

/// Tells us where the main camera node is located and returns a pointer
/// to the head matrix it holds (because of the VR camera, which needs
/// a head matrix, all other cameras have one as well).
/// Thread-safe.
arHead* arGraphicsDatabase::getHead() {
  arViewerNode* viewerNode = NULL;
  // Thread-safety requires using getNodeRef instead of getNode.
  if (_viewerNodeID != -1){
    viewerNode = (arViewerNode*) getNodeRef(_viewerNodeID);
  }
  if (!viewerNode){
    viewerNode = (arViewerNode*) getNodeRef("szg_viewer");
  }
  if (!viewerNode) {
    cerr << "arGraphicsDatabase error: getHead() failed.\n";
    return NULL;
  }
  arHead* result = viewerNode->getHead();
  // There will be a memory leak if we do not unref.
  viewerNode->unref();
  return result;
}

/// Should only be called from arPerspectiveCamera::receiveData.
bool arGraphicsDatabase::registerCamera(arGraphicsNode* node,
					arPerspectiveCamera* theCamera){
  if (!theCamera){
    cerr << "arGraphicsDatabase error: camera pointer does not exist.\n";
    return false;
  }
  if (theCamera->cameraID < 0 || theCamera->cameraID > 7){
    cerr << "arGraphicsDatabase error: camera has invalid ID.\n";
    return false;
  }
  if (!node || !node->active() || node->getOwner() != this){
    cerr << "arGraphicsDatabase error: registerCamera(...) failed because "
	 << "of invalid node ID.\n";
    return false;
  }
  // Just in case the camera ID has changed, must remove it from other slots.
  removeCamera(node);
  _cameraContainer[theCamera->cameraID]
    = pair<arGraphicsNode*,arPerspectiveCamera*>(node,theCamera);
  return true;
}

/// Should only be called from arPerspectiveCamera::deactivate and
/// arGraphicsDatabase::registerCamera.
bool arGraphicsDatabase::removeCamera(arGraphicsNode* node){
  if (!node || !node->active() || node->getOwner() != this){
    cerr << "arGraphicsDatabase error: node not owned by this database.\n";
    return false;
  }
  for (int i=0; i<8; i++){
    if (_cameraContainer[i].first == node){
      _cameraContainer[i].first = NULL;
      _cameraContainer[i].second = NULL;
    }
  }
  return true;
}

arPerspectiveCamera* arGraphicsDatabase::getCamera( unsigned int cameraID ) {
  // Do not need to check < 0 since the parameter is unsigned int.
  if (cameraID > 7){
    cerr << "arGraphicsDatabase error: invlid camera ID.\n";
    return NULL;
  }
  return _cameraContainer[cameraID].second;
}

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
    // NOTE: We MUST use _getNodeNoLock instead of getNode here, otherwise 
    // there will be deadlocks (since _processAdmin is called from within
    // the global arDatabase lock).
    arDatabaseNode* node = _getNodeNoLock(nodeID);
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

