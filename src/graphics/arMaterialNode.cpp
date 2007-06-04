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

arStructuredData* arMaterialNode::dumpData() {
  // Caller is responsible for deleting.
  arGuard dummy(_nodeLock);
  return _dumpData(_lMaterial, false);
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
    _owningDatabase->getDataParser()->recycle(r);
  } else {
    _nodeLock.lock();
      _lMaterial = material;
    _nodeLock.unlock();
  }
}

// NOT thread-safe.
arStructuredData* arMaterialNode::_dumpData(const arMaterial& material,
                                            bool owned) {
  arStructuredData* r = owned ?
    getOwner()->getDataParser()->getStorage(_g->AR_MATERIAL) :
    _g->makeDataRecord(_g->AR_MATERIAL);
  _dumpGenericNode(r, _g->AR_MATERIAL_ID);
  // todo: test datain ret val, like billboardnode
  r->dataIn(_g->AR_MATERIAL_DIFFUSE,material.diffuse.v,AR_FLOAT,3);
  r->dataIn(_g->AR_MATERIAL_AMBIENT,material.ambient.v,AR_FLOAT,3);
  r->dataIn(_g->AR_MATERIAL_SPECULAR,material.specular.v,AR_FLOAT,3);
  r->dataIn(_g->AR_MATERIAL_EMISSIVE,material.emissive.v,AR_FLOAT,3);
  r->dataIn(_g->AR_MATERIAL_EXPONENT,&material.exponent,AR_FLOAT,1);
  r->dataIn(_g->AR_MATERIAL_ALPHA,&material.alpha,AR_FLOAT,1);
  return r;
}
