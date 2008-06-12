//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MATERIAL_NODE_H
#define AR_MATERIAL_NODE_H

#include "arGraphicsNode.h"
#include "arMaterial.h"
#include "arGraphicsCalling.h"

class SZG_CALL arMaterialNode:public arGraphicsNode{
 public:
  arMaterialNode();
  virtual ~arMaterialNode() {}

  void draw(arGraphicsContext*) {}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  arMaterial getMaterial();
  void setMaterial(const arMaterial& material);

  // Sometimes it seems like a good idea to allow direct access to the
  // material (for instance, so the database draw can set materials
  // without copying data out). BUG BUG BUG. THREAD-SAFETY PROBLEM?
  arMaterial* getMaterialPtr() { return &_lMaterial; }

 protected:
  arMaterial _lMaterial;
  arStructuredData* _dumpData(const arMaterial& material, bool owned);
};

#endif
