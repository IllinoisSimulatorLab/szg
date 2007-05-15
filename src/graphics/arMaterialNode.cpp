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
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_lMaterial, false);
  _nodeLock.unlock();
  return r;
}

bool arMaterialNode::receiveData(arStructuredData* inData) {
  if (!_g->checkNodeID(_g->AR_MATERIAL, inData->getID(), "arMaterialNode"))
    return false;

  _nodeLock.lock();
  inData->dataOut(_g->AR_MATERIAL_DIFFUSE,_lMaterial.diffuse.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_AMBIENT,_lMaterial.ambient.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_SPECULAR,_lMaterial.specular.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_EMISSIVE,_lMaterial.emissive.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_EXPONENT,&_lMaterial.exponent,AR_FLOAT,1);
  inData->dataOut(_g->AR_MATERIAL_ALPHA,&_lMaterial.alpha,AR_FLOAT,1); 
  _nodeLock.unlock();
  return true;
}

arMaterial arMaterialNode::getMaterial() {
  _nodeLock.lock();
    const arMaterial r = _lMaterial;
  _nodeLock.unlock();
  return r;
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
  arStructuredData* result = owned ?
    getOwner()->getDataParser()->getStorage(_g->AR_MATERIAL) :
    _g->makeDataRecord(_g->AR_MATERIAL);
  _dumpGenericNode(result,_g->AR_MATERIAL_ID);
  result->dataIn(_g->AR_MATERIAL_DIFFUSE,material.diffuse.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_AMBIENT,material.ambient.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_SPECULAR,material.specular.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_EMISSIVE,material.emissive.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_EXPONENT,&material.exponent,AR_FLOAT,1);
  result->dataIn(_g->AR_MATERIAL_ALPHA,&material.alpha,AR_FLOAT,1);
  return result;
}
