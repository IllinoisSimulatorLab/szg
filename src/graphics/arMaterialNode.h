//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MATERIAL_NODE_H
#define AR_MATERIAL_NODE_H

#include "arGraphicsNode.h"
#include "arMaterial.h"

class SZG_CALL arMaterialNode:public arGraphicsNode{
 public:
  arMaterialNode();
  ~arMaterialNode(){}

  void draw(){} 
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arMaterial getMaterial(){ return _lMaterial; }
  void setMaterial(const arMaterial& material);

 protected:
  arMaterial _lMaterial;
  arStructuredData* _dumpData(const arMaterial& material);
};

#endif
