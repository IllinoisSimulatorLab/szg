//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"
#include "arLogStream.h"

arGraphicsDatabase::arGraphicsDatabase() :
  _texturePath(new list<string>),
  _viewerNodeID(-1)
{
  _typeCode = AR_GRAPHICS_DATABASE;
  _typeString = "graphics";

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
  graphicsPluginData = new arStructuredData(d, "graphics plugin");

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
      !graphicsPluginData || !*graphicsPluginData ||
      !graphicsStateData  || !*graphicsStateData) {
    ar_log_warning() << "arGraphicsDatabase: incomplete dictionary.\n";
  }

  // Initialize the light container.
  int i = 0;
  for (i=0; i<8; ++i) {
    _lightContainer[i] = pair<arGraphicsNode*,arLight*>(NULL,NULL);
  }
  // Initialize the camera container.
  for (i=0; i<8; ++i) {
    _cameraContainer[i] 
      = pair<arGraphicsNode*,arPerspectiveCamera*>(NULL,NULL);
  }
}

// todo: Lots more deleting should really be done here, e.g. the font.
arGraphicsDatabase::~arGraphicsDatabase() {
  if (transformData) {
    delete transformData;
  }
  if (pointsData) {
    delete pointsData;
  }
  if (boundingSphereData) {
    delete boundingSphereData;
  }
  if (billboardData) {
    delete billboardData;
  }
  if (visibilityData) {
    delete visibilityData;
  }
  if (viewerData) {
    delete viewerData;
  }
  if (blendData) {
    delete blendData;
  }
  if (normal3Data) {
    delete normal3Data;
  }
  if (color4Data) {
    delete color4Data;
  }
  if (tex2Data) {
    delete tex2Data;
  }
  if (indexData) {
    delete indexData;
  }
  if (drawableData) {
    delete drawableData;
  }
  if (lightData) {
    delete lightData;
  }
  if (materialData) {
    delete materialData;
  }
  if (perspCameraData) {
    delete perspCameraData;
  }
  if (bumpMapData) {
    delete bumpMapData;
  }
  if (graphicsStateData) {
    delete graphicsStateData;
  }
  if (graphicsPluginData) {
    delete graphicsPluginData;
  }
  
  // Don't forget to get rid of the textures. However, deleting them isn't
  // so smart. Instead, unref and let that operator delete if no one else
  // is holding a reference.
  for (map<string,arTexture*,less<string> >::iterator 
       i = _textureNameContainer.begin(); i != _textureNameContainer.end(); 
       ++i) {
    i->second->unref();
  }
}

arDatabaseNode* arGraphicsDatabase::alter(arStructuredData* inData,
                                          bool refNode) {
  return arDatabase::alter(inData, refNode);
}

void arGraphicsDatabase::reset() {
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
       ++i) {
    if (i->second) {
      i->second->unref();
      i->second = NULL;
    }
  }
  _textureNameContainer.clear();
}

// ARRGH! these alphabet-handling functions just flat-out *suck*
// hopefully, I'll be able to try again later

void arGraphicsDatabase::loadAlphabet(const string& path) {
  if (_server)
    return;
  if (path == "NULL") {
    ar_log_warning() << "arGraphicsDatabase: no path for texture font.\n"; 
    return;
  }

  string fileName(path);
  ar_pathAddSlash(fileName);
  fileName += "courier-bold.ppm";
  if (!_texFont.load(fileName)) {
    ar_log_warning() << "arGraphicsDatabase failed to load texture font.\n"; 
  }
}

arTexFont* arGraphicsDatabase::getTexFont() {
  return &_texFont; 
}

void arGraphicsDatabase::setTexturePath(const string& thePath) {
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

  while (nextChar < length) {
    result = ar_pathToken(thePath, nextChar); // updates nextChar
    if (result == "NULL")
      continue;
    _texturePath->push_back(ar_pathAddSlash(result));
  }
  ar_mutex_unlock(&_texturePathLock);
}

// Creates a new texture and then refs it before returning. Consequently,
// the caller is responsible for unref'ing to prevent a memory leak.
arTexture* arGraphicsDatabase::addTexture(const string& name, int* theAlpha) {
  const map<string,arTexture*,less<string> >::iterator
    iFind(_textureNameContainer.find(name));
  if (iFind != _textureNameContainer.end()) {
    iFind->second->ref(); // Ref the texture.
    return iFind->second;
  }

  // A new texture.
  arTexture* theTexture = new arTexture;
  // The default for the arTexture object is to use GL_DECAL mode, but we
  // want LIT textures.
  theTexture->setTextureFunc(GL_MODULATE);
  std::vector<std::string> triedPaths;
  if (name.length() <= 0) {
    ar_log_warning() << "arGraphicsDatabase warning: "
	             << "ignoring empty filename for texture.\n";
  }
  else if (!isServer()) {
    // Client. Get the actual bitmap.
    // Try everything in the path.
    ar_mutex_lock(&_texturePathLock);
    bool fDone = false;
    string potentialFileName;
    // Look at the bundle path, if it's defined.
    map<string, string, less<string> >::iterator iter =
        _bundlePathMap.find(_bundlePathName);
    if (_bundlePathName != "NULL" && _bundleName != "NULL"
	&& iter != _bundlePathMap.end()) {
      arSemicolonString bundlePath(iter->second);
      for (int n=0; n<bundlePath.size() && !fDone; n++) {
        potentialFileName = bundlePath[n];
        ar_pathAddSlash(potentialFileName);
        potentialFileName += _bundleName;
        ar_pathAddSlash(potentialFileName);
        potentialFileName += name;
        ar_scrubPath(potentialFileName);
        triedPaths.push_back( potentialFileName );
        fDone = theTexture->readImage(potentialFileName.c_str(), *theAlpha,
				      false);
	theTexture->mipmap(true);
      }
    }

    // If nothing was found, look at the texture path.
    for (list<string>::iterator i = _texturePath->begin();
	 !fDone && i != _texturePath->end();
	 ++i) {
      potentialFileName = *i + name;
      ar_scrubPath(potentialFileName);
      triedPaths.push_back( potentialFileName );
      fDone = theTexture->readImage(potentialFileName.c_str(), *theAlpha, false);
      theTexture->mipmap(true);
    }
    static bool fComplained = false;
    if (!fDone) {
      theTexture->dummy();
      if (!fComplained) {
	fComplained = true;
	ar_log_warning() << "arGraphicsDatabase: no image file '"
		       << name << "'. Tried ";
        std::vector<std::string>::iterator iter;
        for (iter = triedPaths.begin(); iter != triedPaths.end(); ++iter) {
          ar_log_warning() << *iter << " ";
        }
        ar_log_warning() << ".\n";
      }
    }
    ar_mutex_unlock(&_texturePathLock);
  }
  triedPaths.clear();
  _textureNameContainer.insert(
    map<string,arTexture*,less<string> >::value_type(name,theTexture));

  // Ref this texture again for its return, so the
  // arTextureNode, who called addTexture, can unref it
  // on deletion or texture change.
  theTexture->ref();
  return theTexture; 
}

arBumpMap* arGraphicsDatabase::addBumpMap(const string& name,
		int numPts, int numInd, float* points,
		int* index, float* tex2, float height, arTexture* decalTexture) {
// @todo Let textureNameContainer hold either glNames or both arBumpMap
//       and arTexture nodes, so we are not re-using the same texture
//       memory for bump and texture maps...
//       (Actually, a separate bumpNameContainer should be enough...)
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
    ar_log_warning() << "arGraphicsDatabase warning: "
	             << "ignoring empty filename for PPM bump texture.\n";
  }
  // Only client, not server, needs the actual bitmap.
  else if (!isServer()) {
    // todo: factor out copypasting between this and texture map code
    // Try everything in the path.
    ar_mutex_lock(&_texturePathLock);
    char fileNameBuffer[512];

    bool fDone = false;
    for (list<string>::iterator i = _texturePath->begin();
	 !fDone && i != _texturePath->end();
	 ++i) {
      const string tmp(*i + buffer);
      ar_stringToBuffer(tmp, fileNameBuffer, sizeof(fileNameBuffer));
      fDone = theBumpMap->readPPM(fileNameBuffer, 1);
    }
    static bool fComplained = false;
    if (!fDone) {
      theBumpMap->dummy();
      if (!fComplained) {
	fComplained = true;
	ar_log_warning() << "arGraphicsDatabase: no PPM file '"
	               << buffer << "' in ";
	if (_texturePath->size() <= 1) {
	  ar_log_warning() << "empty ";
	}
	ar_log_warning() << "bump path." << ar_endl;
      }
    }
    ar_mutex_unlock(&_texturePathLock);
  }
 /* _textureNameContainer.insert(
    map<string,arTexture*,less<string> >::value_type(name,theBumpMap));
 */
  return theBumpMap; 
}

// Figure out the total transformation matrix from ABOVE the current node
// to the database's root. This call is thread-safe with respect to 
// database operations (it uses hidden global database locks). Consequently,
// this cannot be called from any message handling code.
arMatrix4 arGraphicsDatabase::accumulateTransform(int nodeID) {
  arMatrix4 result = ar_identityMatrix();
  arDatabaseNode* thisNode = getNodeRef(nodeID);
  if (!thisNode) {
    ar_log_warning() << "arGraphicsDatabase: invalid node ID for accumulateTransform.\n";
    return result;
  }
  arDatabaseNode* temp = thisNode->getParentRef();
  // Must release our reference to the node to prevent a memory leak.
  thisNode->unref();
  thisNode = temp;
  while (thisNode && thisNode->getID() != 0) {
    if (thisNode->getTypeCode() == AR_G_TRANSFORM_NODE) {
      arTransformNode* transformNode = (arTransformNode*) thisNode;
      result = transformNode->getTransform()*result;
    }
    temp = thisNode->getParentRef();
    // Must release our reference to the node to prevent a memory leak.
    thisNode->unref();
    thisNode = temp;
  }
  if (thisNode) {
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

void arGraphicsDatabase::setVRCameraID(int cameraID) {
  arStructuredData cameraData(_lang->find("graphics admin"));
  cameraData.dataInString("action", "camera_node");
  cameraData.dataIn("node_ID", &cameraID, AR_INT, 1);
  // This admin message *must* be passed on in the case of an arGraphicsServer
  // (i.e. we CANNOT use arGraphicsDatabase::alter here).
  alter(&cameraData);
}

void arGraphicsDatabase::draw( arGraphicsWindow& win, arViewport& view ) {
  // Replaces gl matrix stack... we want to support VERY deep trees
  stack<arMatrix4> transformStack;
  arGraphicsContext context( &win, &view );
  arMatrix4 projectionCullMatrix(view.getCamera()->getProjectionMatrix());
  // Note how we use a "graphics context" for rendering.
  _draw((arGraphicsNode*)&_rootNode, transformStack, &context,
        &projectionCullMatrix);
}


void arGraphicsDatabase::draw(arMatrix4* projectionCullMatrix) {
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
                               arMatrix4* projectionCullMatrix) {

  // Word of warning: lock/unlock is DEFINITELY costly when done per-node.
  // To make the API really thread-safe, some kind of node-level locking
  // is necessary... but it clearly should be a bit constrained (10 node
  // locks/unlocks per node per draw is a 25% performance hit over no locks
  // at all (when drawing a scene graph with a high nodes/geometry ratio...
  // i.e. where scene graph traversal is significant).

  // Store the node on the arGraphicsContext's stack.
  if (context) {
    context->pushNode(node);
  }
  arMatrix4 tempMatrix;
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    // Push current onto the matrix stack.
    glGetFloatv(GL_MODELVIEW_MATRIX, tempMatrix.v);
    transformStack.push(tempMatrix);
  }
  // Draw the node and its children except:
  //   a. If this is the root node, do not draw (no draw method)
  //   b. If this is a visibility node in invisible state, do not
  //      draw either node or children.
  if (node->getTypeCode() != -1 && node->getTypeCode() != AR_D_NAME_NODE) {
    // We are not the root node or a name node, so it is OK to draw.
    // These nodes are actually just arDatabaseNodes instead of
    // arGraphicsNodes.
    node->draw(context);
  }
  // Deal with view frustum culling.
  if (projectionCullMatrix && node->getTypeCode() 
      == AR_G_BOUNDING_SPHERE_NODE) {
    glGetFloatv(GL_MODELVIEW_MATRIX, tempMatrix.v);
    arBoundingSphere b = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arMatrix4 view = (*projectionCullMatrix)*tempMatrix;
    if (!b.intersectViewFrustum(view)) {
      // It is safe to return here... but don't forget to pop the node stack!
      if (context) {
        context->popNode(node);
      }
      return;
    }
  }
  // Deal with visibility nodes.
  if ( !(node->getTypeCode() == AR_G_VISIBILITY_NODE 
         && !((arVisibilityNode*)node)->getVisibility() ) ) {
    // We are not a visibility node in an invisible state. It is OK to draw
    // the children.
    list<arDatabaseNode*> children = node->getChildren();
    for (list<arDatabaseNode*>::iterator i = children.begin(); i != children.end(); ++i) {
      _draw((arGraphicsNode*)(*i), transformStack, context, projectionCullMatrix);
    }
  }

  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    // Pop from stack.
    tempMatrix = transformStack.top();
    transformStack.pop();
    // Return the matrix state to what it was when we were called.
    glLoadMatrixf(tempMatrix.v);
  }
  // Must remember to pop the node stack upon leaving this function.
  if (context) {
    context->popNode(node);
  }
}

// Finds the bounding sphere node (if any) with the closest point of
// intersection to the given ray and returns its ID. If no bounding sphere
// intersects, return -1 (which is the ID of no node). This method is
// thread-safe.
int arGraphicsDatabase::intersect(const arRay& theRay) {
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
                                    stack<arRay>& rayStack) {
  // If this is a transform node, transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    arMatrix4 theMatrix = ((arTransformNode*)node)->getTransform();
    arRay currentRay = rayStack.top();
    rayStack.push(arRay((!theMatrix)*currentRay.getOrigin(),
	                    (!theMatrix)*currentRay.getDirection()
                        - (!theMatrix)*arVector3(0,0,0)));
  }
  // If this is a bounding sphere, intersect.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE) {
    arBoundingSphere sphere 
      = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arRay intRay(rayStack.top());
    float distance = intRay.intersect(sphere.radius, 
                                      sphere.position);
    if (distance>0) {
      // intersection
      if (bestDistance < 0 || distance < bestDistance) {
        bestDistance = distance;
		bestNodeID = node->getID();
      }
    }
  }
  // Deal with intersections with children. NOTE: we must use getChildrenRef
  // instead of getChildren for thread-safety.
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); ++i) {
    _intersect((arGraphicsNode*)(*i), bestDistance, bestNodeID, rayStack);
  }
  // Must unref the nodes to prevent a memory leak.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    rayStack.pop();
  }
}

// Returns a list of bounding sphere nodes that either intersect or contain the
// given bounding sphere. The sphere "closest" to the given sphere is
// first in the list. If addRef is true, all nodes returned have an extra reference
// added to them (and consequently this is thread-safe). 
// Otherwise, no extra ref (the default).
list<arDatabaseNode*> arGraphicsDatabase::intersect(const arBoundingSphere& b, bool addRef) {
  list<arDatabaseNode*> result;
  stack<arMatrix4> matrixStack;
  matrixStack.push(ar_identityMatrix());
  float bestDist = -1;
  arDatabaseNode* bestNode = NULL;
  _intersect((arGraphicsNode*)&_rootNode, b, matrixStack, result, bestNode, bestDist, addRef);
  // The best node is maintained seperately from the intersection list.
  if (bestNode) {
    result.push_back(bestNode);
  }
  return result;
}

// This function is thread-safe! Since all node pointers it returns have references to them!
list<arDatabaseNode*> arGraphicsDatabase::intersectRef(const arBoundingSphere& b) {
  return intersect(b, true);
}

void arGraphicsDatabase::_intersect(arGraphicsNode* node,
                                    const arBoundingSphere& b,
                                    stack<arMatrix4>& matrixStack, 
                                    list<arDatabaseNode*>& nodes,
									arDatabaseNode*& bestNode,
                                    float& bestDistance,
                                    bool useRef) {
  // If this is a transform node, transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    arMatrix4 theMatrix = matrixStack.top()*((arTransformNode*)node)->getTransform();
    matrixStack.push(theMatrix);
  }
  // If this is a bounding sphere, intersect.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE) {
    arBoundingSphere sphere =
      ((arBoundingSphereNode*)node)->getBoundingSphere();
    arMatrix4 m(matrixStack.top());
    arBoundingSphere tmp(b);
    tmp.transform(!m);
    float distance = sphere.intersect(tmp);
    if (distance >= 0) {
      // intersection or containment
      if (bestDistance < 0 || distance < bestDistance) {
        bestDistance = distance;
		// The best node is kept seperately from the list of intersecting nodes.
		if (bestNode) {
		  // If there was already a best node, save it.
		  nodes.push_back(bestNode);
		}
		bestNode = node;
      }
      else {
	    // The best node is kept seperately from the list of intersecting nodes.
	    nodes.push_back(node);
	  }
	  if (useRef) {
	    // In either case, add an extra ref to our node.
	    node->ref();
	  }
    }
  }
  // Deal with intersections with children. NOTE: we must use getChildrenRef
  // instead of getChildren for thread-safety.
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); ++i) {
    _intersect((arGraphicsNode*)(*i), b, matrixStack, nodes, bestNode, bestDistance, useRef);
  }
  // Must unref the nodes to prevent a memory leak.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    matrixStack.pop();
  }
}

// Returns the IDs of all bounding sphere nodes that intersect the given
// ray. Caller is responsible for deleting the list. Thread-safe.
list<int>* arGraphicsDatabase::intersectList(const arRay& theRay) {
  list<int>* result = new list<int>;
  stack<arRay> rayStack;
  rayStack.push(theRay);
  _intersectList((arGraphicsNode*)&_rootNode, result, rayStack);
  return result;
}

void arGraphicsDatabase::_intersectList(arGraphicsNode* node,
                                        list<int>* result,
                                        stack<arRay>& rayStack) {
  // Copypaste "transform the ray" with 2 previous instances in this file.
  // If this is a transform node, transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    arMatrix4 theMatrix = ((arTransformNode*)node)->getTransform();
    arRay currentRay = rayStack.top();
    rayStack.push(arRay((!theMatrix)*currentRay.getOrigin(),
			(!theMatrix)*currentRay.getDirection()
                        - (!theMatrix)*arVector3(0,0,0)));
  }
  // Copypaste "intersect" with 2 previous instances in this file.
  // If this is a bounding sphere, intersect.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE) {
    arBoundingSphere sphere 
      = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arRay intRay(rayStack.top());
    float distance = intRay.intersect(sphere.radius, 
				      sphere.position);
    if (distance>0) {
      // intersection
      result->push_back(node->getID());
    }
  }
  // Deal with intersections with children. NOTE: we must use
  // getChildrenRef instead of getChildren for thread-safety.
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); ++i) {
    _intersectList((arGraphicsNode*)(*i), result, rayStack);
  }
  // Must unref the nodes to prevent a memory leak.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    rayStack.pop();
  }
}

// Intersect a ray with the database. At bounding sphere nodes, if there
// is no intersection, skip that subtree. At drawable nodes
// (consisting of triangles or quads... incompletely implemented) 
// intersect the ray with the polygons, figuring out the point of closest
// intersection. Return a pointer to the geometry node with
// the closest intersection point, or NULL if none intersect.
//
// This method is thread-safe.
arGraphicsNode* arGraphicsDatabase::intersectGeometry(const arRay& theRay,
                                                      int excludeBelow) {
  stack<arRay> rayStack;
  rayStack.push(theRay);
  arGraphicsContext context;
  arGraphicsNode* bestNode = NULL;
  float bestDistance = -1;
  _intersectGeometry((arGraphicsNode*)&_rootNode, &context, rayStack,
		     excludeBelow, bestNode, bestDistance);
  return bestNode;
}

// A helper function for _intersectGeometry, which is, in turn, a helper
// function for intersectGeometry. 
float arGraphicsDatabase::_intersectSingleGeometry(arGraphicsNode* node,
                                                   arGraphicsContext* context,
                                                   const arRay& theRay) {
  if (!node || !context) {
    return -1;
  }
  if (node->getTypeCode() != AR_G_DRAWABLE_NODE) {
    return -1;
  }
  arDrawableNode* d = (arDrawableNode*) node;
  if (d->getType() != DG_TRIANGLES) {
    ar_log_warning() << "arGraphicsDatabase only able to intersect with triangle "
	           << "soups so far.\n";
    return -1;
  }
  arGraphicsNode* p = (arGraphicsNode*) context->getNode(AR_G_POINTS_NODE);
  if (!p) {
    ar_log_warning() << "arGraphicsDatabase: no points node for drawable.\n";
    return -1;
  }
  arGraphicsNode* i = (arGraphicsNode*) context->getNode(AR_G_INDEX_NODE);
  int number = d->getNumber();
  float* points = p->getBuffer();
  int* index = NULL;
  if (i) {
    index = (int*)i->getBuffer();
  }
  float bestDistance = -1;
  float dist;
  arVector3 a, b, c;
  for (int j = 0; j < number; j++) {
    if (index) {
      a = arVector3(points + 3*index[3*j]);
      b = arVector3(points + 3*index[3*j+1]);
      c = arVector3(points + 3*index[3*j+2]);
    }
    else {
      a = arVector3(points + 9*j);
      b = arVector3(points + 9*j+3);
      c = arVector3(points + 9*j+6);
    }
    dist = ar_intersectRayTriangle(theRay.getOrigin(),
				   theRay.getDirection(),
				   a, b, c);
    if (dist >= 0 && (bestDistance < 0 || dist < bestDistance)) {
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
                                            float bestDistance) {
  if (node->getID() == excludeBelow) {
    return;
  }
  // Store the node on the arGraphicsContext's stack.
  if (context) {
    context->pushNode(node);
  }
  // If this is a transform node, transform the ray.
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    arMatrix4 theMatrix = ((arTransformNode*)node)->getTransform();
    arRay currentRay = rayStack.top();
    rayStack.push(arRay((!theMatrix)*currentRay.getOrigin(),
			(!theMatrix)*currentRay.getDirection()
                        - (!theMatrix)*arVector3(0,0,0)));
  }
  // If this is a bounding sphere, intersect.
  // If we do not intersect, return.
  if (node->getTypeCode() == AR_G_BOUNDING_SPHERE_NODE) {
    arBoundingSphere sphere 
      = ((arBoundingSphereNode*)node)->getBoundingSphere();
    arRay intRay(rayStack.top());
    float distance = intRay.intersect(sphere.radius, 
				      sphere.position);
    if (distance < 0) {
      // Must remember to pop the node stack upon leaving this function.
      if (context) {
        context->popNode(node);
      }
      return;
    }
  }
  // If this is a drawable node, intersect.
  if (node->getTypeCode() == AR_G_DRAWABLE_NODE) {
    arRay localRay = rayStack.top();
    float rawDist = _intersectSingleGeometry(node, context, localRay);
    
    if (rawDist >= 0) {
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
      if (bestDistance < 0 || dist < bestDistance) {
        bestNode = node;
        bestDistance = dist;
      }
    }
  }
  // Deal with intersections with children. For thread-safety, hold references
  // to the node pointers,
  list<arDatabaseNode*> children = node->getChildrenRef();
  for (list<arDatabaseNode*>::iterator i = children.begin();
       i != children.end(); ++i) {
    _intersectGeometry((arGraphicsNode*)(*i), context, rayStack, 
                       excludeBelow, bestNode, bestDistance);
  }
  // To prevent a memory leak, must release the references.
  ar_unrefNodeList(children);
  // On the way out, undo the effects of the transform
  if (node->getTypeCode() == AR_G_TRANSFORM_NODE) {
    rayStack.pop();
  }
  // Must remember to pop the node stack upon leaving this function.
  if (context) {
    context->popNode(node);
  }
}

// Only call from arLightNode::receiveData. This guarantees that
// _lightContainer is modified atomically when thread-safety matters
// (like arGraphicsServer and arGraphicsPeer),
bool arGraphicsDatabase::registerLight(arGraphicsNode* node, 
                                       arLight* theLight) {
  if (!theLight) {
    ar_log_warning() << "arGraphicsDatabase: no light pointer.\n";
    return false;
  }
  if (!node || !node->active() || node->getOwner() != this) {
    ar_log_warning() << "arGraphicsDatabase: unowned node.\n";
    return false;
  }
  if (theLight->lightID < 0 || theLight->lightID > 7) {
    ar_log_warning() << "arGraphicsDatabase: light has invalid ID.\n";
    return false;
  }
  // If the light ID changed, remove other instances from the container.
  (void) removeLight(node);
  // Insert the new light.
  _lightContainer[theLight->lightID] =
    pair<arGraphicsNode*,arLight*>(node, theLight);
  return true;
}

// Only call from arLightNode::deactivate and arGraphicsDatabase::registerLight.
bool arGraphicsDatabase::removeLight(arGraphicsNode* node) {
  if (!node || !node->active() || node->getOwner() != this) {
    ar_log_warning() << "arGraphicsDatabase: unowned node.\n";
    return false;
  }

  for (int i=0; i<8; ++i) {
    if (_lightContainer[i].first == node) {
      _lightContainer[i].first = NULL;
      _lightContainer[i].second = NULL;
    }
  }
  return true;
}

// For thread safety w.r.t. light deletion, lock this call with
// _databaseLock. Take care that no call inside
// also locks the global lock, 
// creating a deadlock. Since accumulateTransform(int) does do that,
// use arDatabaseNode::accumulateTransform() instead.
void arGraphicsDatabase::activateLights() {
  ar_mutex_lock(&_databaseLock);
  for (int i=0; i<8; ++i) {
    if (_lightContainer[i].first) {
      // A light has been registered for this ID.
      const arMatrix4 lightPositionTransform(
	_lightContainer[i].first->accumulateTransform());
      _lightContainer[i].second->activateLight(lightPositionTransform);
    }
    else {
      // No light in this slot. Disable.
      const GLenum lights[8] = {
        GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
        GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7
      };
      glDisable(lights[i]);
    }
  }
  ar_mutex_unlock(&_databaseLock);
}

// Report the main camera node's location.  Return a pointer
// to the head matrix it holds (because of the VR camera, which needs
// a head matrix, all other cameras have one as well).
// Thread-safe.
arHead* arGraphicsDatabase::getHead() {
  arViewerNode* viewerNode = NULL;
  // Thread-safety requires using getNodeRef instead of getNode.
  if (_viewerNodeID != -1)
    viewerNode = (arViewerNode*) getNodeRef(_viewerNodeID);
  if (!viewerNode)
    viewerNode = (arViewerNode*) getNodeRef("szg_viewer");
  if (!viewerNode) {
    ar_log_warning() << "arGraphicsDatabase: getHead() failed.\n";
    return NULL;
  }

  arHead* result = viewerNode->getHead();
  viewerNode->unref(); // Avoid memory leak.
  return result;
}

// Should only be called from arPerspectiveCamera::receiveData.
bool arGraphicsDatabase::registerCamera(arGraphicsNode* node,
					arPerspectiveCamera* theCamera) {
  if (!theCamera) {
    ar_log_warning() << "arGraphicsDatabase: no camera pointer.\n";
    return false;
  }
  if (theCamera->cameraID < 0 || theCamera->cameraID > 7) {
    ar_log_warning() << "arGraphicsDatabase: camera has invalid ID.\n";
    return false;
  }
  if (!node || !node->active() || node->getOwner() != this) {
    ar_log_warning() << "arGraphicsDatabase: invalid node ID for registerCamera.\n";
    return false;
  }
  // Just in case the camera ID has changed, must remove it from other slots.
  removeCamera(node);
  _cameraContainer[theCamera->cameraID]
    = pair<arGraphicsNode*,arPerspectiveCamera*>(node,theCamera);
  return true;
}

// Should only be called from arPerspectiveCamera::deactivate and
// arGraphicsDatabase::registerCamera.
bool arGraphicsDatabase::removeCamera(arGraphicsNode* node) {
  if (!node || !node->active() || node->getOwner() != this) {
    ar_log_warning() << "arGraphicsDatabase: unowned node.\n";
    return false;
  }
  for (int i=0; i<8; ++i) {
    if (_cameraContainer[i].first == node) {
      _cameraContainer[i].first = NULL;
      _cameraContainer[i].second = NULL;
    }
  }
  return true;
}

arPerspectiveCamera* arGraphicsDatabase::getCamera( unsigned int cameraID ) {
  // Do not need to check < 0 since the parameter is unsigned int.
  if (cameraID > 7) {
    ar_log_warning() << "arGraphicsDatabase: invlid camera ID.\n";
    return NULL;
  }
  return _cameraContainer[cameraID].second;
}

arDatabaseNode* arGraphicsDatabase::_makeNode(const string& type) {
  arDatabaseNode* outNode = NULL;
  // The node could be natively handles by the arDatabase class
  outNode = arDatabase::_makeNode(type);
  if (outNode) {
    // this was a base node type.
    return outNode;
  }
  // Perhaps it will be one of the arGraphicsDatabase nodes...
  if (type=="transform") {
    outNode = (arDatabaseNode*) new arTransformNode();
  }
  else if (type=="points") {
    outNode = (arDatabaseNode*) new arPointsNode();
  }
  else if (type=="texture") {
    outNode = (arDatabaseNode*) new arTextureNode();
  }
  else if (type=="bounding sphere") {
    outNode = (arDatabaseNode*) new arBoundingSphereNode();
  }
  else if (type=="billboard") {
    outNode = (arDatabaseNode*) new arBillboardNode();
  }
  else if (type=="visibility") {
    outNode = (arDatabaseNode*) new arVisibilityNode();
  }
  else if (type=="viewer") {
    outNode = (arDatabaseNode*) new arViewerNode();
  }
  else if (type=="blend") {
    outNode = (arDatabaseNode*) new arBlendNode();
  }
  else if (type=="state") {
    outNode = (arDatabaseNode*) new arGraphicsStateNode();
  }
  else if (type == "normal3") {
    outNode = (arDatabaseNode*) new arNormal3Node();
  }
  else if (type == "color4") {
    outNode = (arDatabaseNode*) new arColor4Node();
  }
  else if (type == "tex2") {
    outNode = (arDatabaseNode*) new arTex2Node();
  }
  else if (type == "index") {
    outNode = (arDatabaseNode*) new arIndexNode();
  }
  else if (type == "drawable") {
    outNode = (arDatabaseNode*) new arDrawableNode();
  }
  else if (type == "light") {
    outNode = (arDatabaseNode*) new arLightNode();
  }
  else if (type == "material") {
    outNode = (arDatabaseNode*) new arMaterialNode();
  }
  else if (type == "persp camera") {
    outNode = (arDatabaseNode*) new arPerspectiveCameraNode();
  }
  else if (type == "bump map") {
    outNode = (arDatabaseNode*) new arBumpMapNode();
  }
  else if (type == "graphics state") {
    outNode = (arDatabaseNode*) new arGraphicsStateNode();
  }
  else if (type == "graphics plugin") {
    outNode = (arDatabaseNode*) new arGraphicsPluginNode();
  }
  else {
    ar_log_warning() << "arGraphicsDatabase: makeNode factory got unknown type="
                   << type << ".\n";
    return NULL;
  }

  return outNode;
}

arDatabaseNode* arGraphicsDatabase::_processAdmin(arStructuredData* data) {
  const string name = data->getDataString("name");
  const string action = data->getDataString("action");
  if (action == "remote_path") {
    ar_log_remark() << "arGraphicsDatabase remark: using texture bundle " << name << "\n";
    arSlashString bundleInfo(name);
    if (bundleInfo.size() != 2) {
      ar_log_remark() << "arGraphicsDatabase got garbled texture bundle ID.\n";
      return &_rootNode;
    }
    setDataBundlePath(bundleInfo[0], bundleInfo[1]);
  }
  else if (action == "camera_node") {
    int nodeID = data->getDataInt("node_ID");
    // If we have a node with this ID, then set the camera to it.
    // Use _getNodeNoLock instead of getNode, to avoid deadlocks
    // (since _processAdmin is called from within the global arDatabase lock).
    arDatabaseNode* node = _getNodeNoLock(nodeID);
    if (node && node->getTypeString() == "viewer") {
      _viewerNodeID = nodeID;
    }
    else {
      ar_log_remark() << "arGraphicsDatabase: no viewer node with "
	              << "ID=" << nodeID << ".\n";
    }
  }
  return &_rootNode;
}
