//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_NODE_H
#define AR_GRAPHICS_NODE_H

#include "arTexture.h"
#include "arBumpMap.h"
#include "arMaterial.h"
#include "arLightFloatBuffer.h"
#include "arGraphicsLanguage.h"
#include "arDatabaseNode.h"

class arGraphicsDatabase;

/// Node in an arGraphicsDatabase.

class arGraphicsNode: public arDatabaseNode{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  friend class arGraphicsDatabase;
  arGraphicsNode();
  virtual ~arGraphicsNode();

  // From arDatabaseNode
  virtual void initialize(arDatabase*);
  virtual bool receiveData(arStructuredData*){ return false; };
  virtual arStructuredData* dumpData(){ return NULL; };

  // unique to arGraphicsNode
  arMatrix4 accumulateTransform();
  virtual void draw(){};

 protected:
  arGraphicsDatabase* _owningDatabase;
  arGraphicsLanguage* _g;
  
  arLightFloatBuffer* _points;   ///< state at this node
  arLightFloatBuffer* _blend;
  arLightFloatBuffer* _normal3;  ///< 3 float normals, packed one after another
  arLightFloatBuffer* _color;
  arLightFloatBuffer* _tex2;
  arLightFloatBuffer* _index;
  arMaterial*         _material;
  
  arTexture** _texture;
  arTexture*  _localTexture; ///< texture, if any, owned by this node
  arBumpMap** _bumpMap;
  arBumpMap*  _localBumpMap; ///< bump map, if any, owned by this node

  arLightFloatBuffer _commandBuffer;

  void _accumulateTransform(arGraphicsNode* g, arMatrix4& m);
};

#endif
