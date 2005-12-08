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
#include "arGraphicsContext.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class arGraphicsDatabase;

/// Node in an arGraphicsDatabase.

class SZG_CALL arGraphicsNode: public arDatabaseNode{
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
  virtual void draw(arGraphicsContext*){};
  inline ARfloat* getBuffer(){ return _commandBuffer.v; }
  inline int getBufferSize() const { return _commandBuffer.size(); }

 protected:
  arGraphicsDatabase* _owningDatabase;
  arGraphicsLanguage* _g;
  
  arLightFloatBuffer _commandBuffer;

  void _accumulateTransform(arGraphicsNode* g, arMatrix4& m);
};

#endif
