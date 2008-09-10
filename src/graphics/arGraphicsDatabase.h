//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_DATABASE_H
#define AR_GRAPHICS_DATABASE_H

#include "arDatabase.h"

#include "arMath.h"
#include "arRay.h"
#include "arGraphicsHeader.h"
#include "arGraphicsLanguage.h"
#include "arTexFont.h"
#include "arTransformNode.h"
#include "arPointsNode.h"
#include "arTextureNode.h"
#include "arBoundingSphereNode.h"
#include "arBillboardNode.h"
#include "arVisibilityNode.h"
#include "arViewerNode.h"
#include "arBlendNode.h"
#include "arNormal3Node.h"
#include "arColor4Node.h"
#include "arTex2Node.h"
#include "arIndexNode.h"
#include "arDrawableNode.h"
#include "arLightNode.h"
#include "arMaterialNode.h"
#include "arLight.h"
#include "arPerspectiveCamera.h"
#include "arPerspectiveCameraNode.h"
#include "arHead.h"
#include "arBumpMapNode.h"
#include "arGraphicsStateNode.h"
#include "arGraphicsPluginNode.h"

#include "arGraphicsCalling.h"

// Contain a scene graph created by an arGraphicsServer
// and rendered by one or more arGraphicsClient objects.

class SZG_CALL arGraphicsDatabase: public arDatabase{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arGraphicsDatabase();
  virtual ~arGraphicsDatabase();

  // Default should be false (i.e. we do not need an extra reference
  // tacked on to the indicated node in the case of node creation).
  virtual arDatabaseNode* alter(arStructuredData*, bool refNode=false);
  virtual void reset();

  void loadAlphabet(const string& path);
  arTexFont* getTexFont();
  void setTexturePath(const string& thePath);
  arTexture* addTexture(const string& name, int* theAlpha);
  arBumpMap* addBumpMap(const string& name, int numPts, int numInd,
                        float* points, int* indices, float* tex2,
                        float height, arTexture* decalTexture);
  // Higher-level version added to simplify python bindings
  arBumpMap* addBumpMap(const string& name, 
                        vector<arVector3>& points,
                        vector<int>& indices,
                        vector<arVector2>& texCoords,
                        float height, arTexture* decalTexture);

  arMatrix4 accumulateTransform(int nodeID);
  arMatrix4 accumulateTransform(int startNodeID, int endNodeID);

  void setVRCameraID(int cameraID);

  void draw( arGraphicsWindow& win, arViewport& view );
  void draw(const arMatrix4* projectionMatrix = NULL);
  int intersect(const arRay&);
  list<arDatabaseNode*> intersect(const arBoundingSphere& b, bool addRef=false);
  list<arDatabaseNode*> intersectRef(const arBoundingSphere& b);
  list<int>* intersectList(const arRay&);
  arGraphicsNode* intersectGeometry(const arRay& theRay, int excludeBelow = -1);

  bool registerLight(arGraphicsNode* node, arLight* theLight);
  bool removeLight(arGraphicsNode* node);
  void activateLights();

  arHead* getHead();
  bool registerCamera(arGraphicsNode* node, arPerspectiveCamera* theCamera);
  bool removeCamera(arGraphicsNode* node);
  // Normally we use the default "VR camera"... however, the database can
  // also be drawn from the perspective of one of the attached camera
  arPerspectiveCamera* getCamera( unsigned int cameraID );

  // Deliberately public, for external data input.
  arStructuredData* transformData;
  arStructuredData* pointsData;
  arStructuredData* textureData;
  arStructuredData* boundingSphereData;
  arStructuredData* billboardData;
  arStructuredData* visibilityData;
  arStructuredData* viewerData;
  arStructuredData* blendData;
  arStructuredData* normal3Data;
  arStructuredData* color4Data;
  arStructuredData* tex2Data;
  arStructuredData* indexData;
  arStructuredData* drawableData;
  arStructuredData* lightData;
  arStructuredData* materialData;
  arStructuredData* perspCameraData;
  arStructuredData* bumpMapData;
  arStructuredData* graphicsStateData;
  arStructuredData* graphicsPluginData;

  arGraphicsLanguage _gfx;

  //GLuint        _normLookupTexture; //unused cruft from graphics/arBumpMap.cpp

 protected:
  arLock _texturePathLock; // guards _texturePath
  list<string>* _texturePath;
  map<string, arTexture*, less<string> > _textureNameContainer;
  arTexFont _texFont;
  string _pathTexFont;
  bool _fFirstTexFont;
  void _loadAlphabet(const string&);

  // information about the lights in the scene
  // there's a bug here... no way is yet coded for deleting a light!
  pair<arGraphicsNode*, arLight*>             _lightContainer[8];
  // information about the auxilliary cameras in the scene...
  // the "VR camera" is still privileged as a default.
  pair<arGraphicsNode*, arPerspectiveCamera*> _cameraContainer[8];
  // The ID of the node that contains the VR camera information.
  int _viewerNodeID;

  void _draw(arGraphicsNode*, stack<arMatrix4>&, arGraphicsContext*,
             const arMatrix4*);
  void _intersect(arGraphicsNode*, float&, int&, stack<arRay>&);
  void _intersect(arGraphicsNode* node,
                  const arBoundingSphere& b,
                  stack<arMatrix4>& matrixStack,
                  list<arDatabaseNode*>& nodes,
                  arDatabaseNode*& bestNode,
                  float& bestDistance,
                  bool useRef);
  void _intersectList(arGraphicsNode*, list<int>*, stack<arRay>&);
  float _intersectSingleGeometry(arGraphicsNode* node,
                                 arGraphicsContext* context,
                                 const arRay& theRay);
  void _intersectGeometry(arGraphicsNode* node,
                          arGraphicsContext* context,
                          stack<arRay>& rayStack,
                          int excludeBelow,
                          arGraphicsNode*& bestNode,
                          float bestDistance);
  virtual arDatabaseNode* _makeNode(const string& type);
  arDatabaseNode* _processAdmin(arStructuredData*);

 private:
  bool _fComplainedImage;
  bool _fComplainedPPM;
};

#endif
