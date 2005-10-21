//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LIGHT_NODE_H
#define AR_LIGHT_NODE_H

#include "arGraphicsNode.h"
#include "arLight.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arLightNode:public arGraphicsNode{
 public:
  arLightNode();
  virtual ~arLightNode();

  void draw(arGraphicsContext*){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);
  void deactivate();

  arLight getLight();
  void setLight(arLight& light);

 protected:
  arLight _nodeLight;
  arStructuredData* _dumpData(arLight& light, bool owned);
};

#endif
