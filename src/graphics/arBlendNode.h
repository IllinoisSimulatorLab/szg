//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BLEND_NODE_H
#define AR_BLEND_NODE_H

#include "arGraphicsNode.h"
#include "arGraphicsCalling.h"

// Blending.

class SZG_CALL arBlendNode:public arGraphicsNode{
 public:
  arBlendNode();
  virtual ~arBlendNode() {}

  void draw(arGraphicsContext*) {}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  float getBlend();
  void setBlend(float blendFactor);
 protected:
  float _blendFactor;
  arStructuredData* _dumpData(float blendFactor, bool owned);
};

#endif
