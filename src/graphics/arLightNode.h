//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LIGHT_NODE_H
#define AR_LIGHT_NODE_H

#include "arGraphicsNode.h"
#include "arLight.h"

class SZG_CALL arLightNode:public arGraphicsNode{
 public:
  arLightNode();
  ~arLightNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arLight getLight(){ return _nodeLight; }
  void setLight(arLight& light);

 protected:
  arLight _nodeLight;
  arStructuredData* _dumpData(arLight& light);
};

#endif
