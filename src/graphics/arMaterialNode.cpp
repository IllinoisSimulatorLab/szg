//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

arMaterialNode::arMaterialNode(){
  // A sensible default name.
  _name = "material_node";
  _typeCode = AR_G_MATERIAL_NODE;
  _typeString = "material";
}

arStructuredData* arMaterialNode::dumpData(){
  return _dumpData(_lMaterial);
}

bool arMaterialNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_MATERIAL){
    cerr << "arMaterialNode error: expected "
    << _g->AR_MATERIAL
    << " (" << _g->_stringFromID(_g->AR_MATERIAL) << "), not "
    << inData->getID()
    << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }
  inData->dataOut(_g->AR_MATERIAL_DIFFUSE,_lMaterial.diffuse.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_AMBIENT,_lMaterial.ambient.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_SPECULAR,_lMaterial.specular.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_EMISSIVE,_lMaterial.emissive.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_MATERIAL_EXPONENT,&_lMaterial.exponent,AR_FLOAT,1);
  inData->dataOut(_g->AR_MATERIAL_ALPHA,&_lMaterial.alpha,AR_FLOAT,1); 
  // DEFUNCT
  //_material = &_lMaterial;
  return true;
}

void arMaterialNode::setMaterial(const arMaterial& material){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(material);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _lMaterial = material;
  }
}

arStructuredData* arMaterialNode::_dumpData(const arMaterial& material){
  arStructuredData* result = _g->makeDataRecord(_g->AR_MATERIAL);
  _dumpGenericNode(result,_g->AR_MATERIAL_ID);
  result->dataIn(_g->AR_MATERIAL_DIFFUSE,material.diffuse.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_AMBIENT,material.ambient.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_SPECULAR,material.specular.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_EMISSIVE,material.emissive.v,AR_FLOAT,3);
  result->dataIn(_g->AR_MATERIAL_EXPONENT,&material.exponent,AR_FLOAT,1);
  result->dataIn(_g->AR_MATERIAL_ALPHA,&material.alpha,AR_FLOAT,1);
  return result;
}
