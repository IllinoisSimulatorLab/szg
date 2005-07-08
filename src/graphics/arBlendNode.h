//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BLEND_NODE_H
#define AR_BLEND_NODE_H

#include "arGraphicsNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// Blending.

class SZG_CALL arBlendNode:public arGraphicsNode{
 public:
  arBlendNode();
  ~arBlendNode(){}

  void draw(arGraphicsContext*){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  float getBlend();
  void setBlend(float blend);
 protected:
  float _blendFactor;
  arStructuredData* _dumpData(float blendFactor);
};

#endif
