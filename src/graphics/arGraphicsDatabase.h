//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_DATABASE_H
#define AR_GRAPHICS_DATABASE_H

#include "arDatabase.h"

#include "arMath.h"
#include "arRay.h"
#include "arGraphicsHeader.h"
#include "arGraphicsLanguage.h"

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

#ifdef USE_CG // Marks' Cg stuff
// Unfortunately, the internal includes in the Cg headers necessitate that
// the Cg directory is what is on the include path, not the header files
// themselves.
#include <Cg/cgGL.h>
#endif

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// Contains a scene graph created by an arGraphicsServer
/// and rendered by one or more arGraphicsClient objects.

class SZG_CALL arGraphicsDatabase: public arDatabase{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arGraphicsDatabase();
  virtual ~arGraphicsDatabase();

  virtual arDatabaseNode* alter(arStructuredData*);
  virtual void reset();

  void loadAlphabet(const char*);
  arTexture** getAlphabet();
  void setTexturePath(const string& thePath);
  arTexture* addTexture(const string&, int*);
  arTexture* addTexture(int w, int h, bool alpha, const char* pixels);
  arBumpMap* addBumpMap(const string& name, int numPts, int numInd,
		  	float* points, int* indices, float* tex2,
			float height, arTexture* decalTexture);

  arMatrix4 accumulateTransform(int nodeID);
  arMatrix4 accumulateTransform(int startNodeID, int endNodeID);

  void setVRCameraID(int cameraID);

  void draw(arMatrix4* projectionCullMatrix = NULL);
  int intersect(const arRay&);
  list<int>* intersectList(const arRay&);
  arGraphicsNode* arGraphicsDatabase::intersectGeometry(const arRay& theRay,
							int excludeBelow = -1);

  bool registerLight(int owningNodeID, arLight* theLight);
  void activateLights();

  arHead* getHead();
  bool registerCamera(int owningNodeID, arPerspectiveCamera* theCamera);
  // normally we use the default "VR camera"... however, the database can
  // also be drawn from the perspective of one of the attached camera
  arPerspectiveCamera* getCamera( unsigned int cameraID );
//  bool setCamera(int cameraID);
//  void setLocalCamera(float* frustum, float* lookat);

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

  arGraphicsLanguage _gfx;  

#ifdef USE_CG // Mark's Cg Stuff
  //CGcontext     _cg_context;
  //CGprofile     _cg_vertexProfile, _cg_fragmentProfile;
#endif
  //bool          _isCgInited;
  //GLuint	_normLookupTexture;
  //void          initCg();

 protected:
  arMutex                              _texturePathLock;
  list<string>*                        _texturePath;
  map<string,arTexture*,less<string> > _textureNameContainer;
  arTexture**                          _alphabet;

  // information about the lights in the scene
  // there's a bug here... no way is yet coded for deleting a light!
  pair<int,arLight*>             _lightContainer[8];
  // information about the auxilliary cameras in the scene...
  // the "VR camera" is still privileged as a default.
  pair<int,arPerspectiveCamera*> _cameraContainer[8];
  // The ID of the node that contains the VR camera information.
  int _viewerNodeID;

  void _draw(arGraphicsNode*, stack<arMatrix4>&, arGraphicsContext*,
             arMatrix4*);
  void _intersect(arGraphicsNode*, float&, int&, stack<arRay>&);
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
};

#endif
