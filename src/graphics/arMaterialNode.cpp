//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arMaterialNode::arMaterialNode() {
  _name = "material_node";
  _typeCode = AR_G_MATERIAL_NODE;
  _typeString = "material";
}

bool arMaterialNode::receiveData(arStructuredData* inData) {
  if (!_g->checkNodeID(_g->AR_MATERIAL, inData->getID(), "arMaterialNode"))
    return false;

  arGuard dummy(_nodeLock);
  inData->dataOut(_g->AR_MATERIAL_DIFFUSE,_lMaterial.diffuse.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_AMBIENT,_lMaterial.ambient.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_SPECULAR,_lMaterial.specular.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_EMISSIVE,_lMaterial.emissive.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_EXPONENT,&_lMaterial.exponent,AR_FLOAT,1);
  inData->dataOut(_g->AR_MATERIAL_ALPHA,&_lMaterial.alpha,AR_FLOAT,1); 
  return true;
}

arMaterial arMaterialNode::getMaterial() {
  arGuard dummy(_nodeLock);
  return _lMaterial;
}

void arMaterialNode::setMaterial(const arMaterial& material) {
  if (active()) {
    _nodeLock.lock();
      arStructuredData* r = _dumpData(material, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  } else {
    arGuard dummy(_nodeLock);
    _lMaterial = material;
  }
}

arStructuredData* arMaterialNode::dumpData() {
  arGuard dummy(_nodeLock);
  return _dumpData(_lMaterial, false);
}

arStructuredData* arMaterialNode::_dumpData(const arMaterial& material, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_MATERIAL);
  _dumpGenericNode(r, _g->AR_MATERIAL_ID);
  if (!r->dataIn(_g->AR_MATERIAL_DIFFUSE,material.diffuse.v,AR_FLOAT,3) ||
      !r->dataIn(_g->AR_MATERIAL_AMBIENT,material.ambient.v,AR_FLOAT,3) ||
      !r->dataIn(_g->AR_MATERIAL_SPECULAR,material.specular.v,AR_FLOAT,3) ||
      !r->dataIn(_g->AR_MATERIAL_EMISSIVE,material.emissive.v,AR_FLOAT,3) ||
      !r->dataIn(_g->AR_MATERIAL_EXPONENT,&material.exponent,AR_FLOAT,1) ||
      !r->dataIn(_g->AR_MATERIAL_ALPHA,&material.alpha,AR_FLOAT,1)) {
    delete r;
    return NULL;
  }
  return r;
}
