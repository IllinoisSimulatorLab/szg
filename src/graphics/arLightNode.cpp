//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLightNode.h"
#include "arGraphicsDatabase.h"

arLightNode::arLightNode(){
  _name = "light_node";
  _typeCode = AR_G_LIGHT_NODE;
  _typeString = "light";
}

arLightNode::~arLightNode(){
  
}

bool arLightNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_LIGHT, inData->getID(), "arLightNode"))
    return false;

  arGuard dummy(_nodeLock);
  inData->dataOut(_g->AR_LIGHT_LIGHT_ID,&_nodeLight.lightID,AR_INT,1);
  inData->dataOut(_g->AR_LIGHT_POSITION,_nodeLight.position.v,AR_FLOAT,4);
  inData->dataOut(_g->AR_LIGHT_DIFFUSE,_nodeLight.diffuse.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_LIGHT_AMBIENT,_nodeLight.ambient.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_LIGHT_SPECULAR,_nodeLight.specular.v,AR_FLOAT,3);
  // bug: each data member should get a seperate field.
  float temp[5];
  inData->dataOut(_g->AR_LIGHT_ATTENUATE,temp,AR_FLOAT,3);
  _nodeLight.constantAttenuation = temp[0];
  _nodeLight.linearAttenuation = temp[1];
  _nodeLight.quadraticAttenuation = temp[3];
  inData->dataOut(_g->AR_LIGHT_SPOT,temp,AR_FLOAT,5);
  _nodeLight.spotDirection = arVector3(temp[0], temp[1], temp[2]);
  _nodeLight.spotCutoff = temp[3];
  _nodeLight.spotExponent = temp[4];

  _owningDatabase->registerLight(this,&_nodeLight);
  return true;
}

void arLightNode::deactivate(){
  // There must be an owning database since that's who calls this method.
  _owningDatabase->removeLight(this);
}

arLight arLightNode::getLight(){
  arGuard dummy(_nodeLock);
  return _nodeLight;
}

void arLightNode::setLight(arLight& light){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(light, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard dummy(_nodeLock);
    _nodeLight = light;
  }
}

arStructuredData* arLightNode::dumpData(){
  arGuard dummy(_nodeLock);
  return _dumpData(_nodeLight, false);
}

arStructuredData* arLightNode::_dumpData(arLight& light, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_LIGHT);
  _dumpGenericNode(r, _g->AR_LIGHT_ID); 
  bool ok =
    r->dataIn(_g->AR_LIGHT_LIGHT_ID,&light.lightID,AR_INT,1) &&
    r->dataIn(_g->AR_LIGHT_POSITION,light.position.v,AR_FLOAT,4) &&
    r->dataIn(_g->AR_LIGHT_DIFFUSE,light.diffuse.v,AR_FLOAT,3) &&
    r->dataIn(_g->AR_LIGHT_AMBIENT,light.ambient.v,AR_FLOAT,3) &&
    r->dataIn(_g->AR_LIGHT_SPECULAR,light.specular.v,AR_FLOAT,3);
  // bug: each data member should get a seperate field.
  float temp[5];
  temp[0] = light.constantAttenuation;
  temp[1] = light.linearAttenuation;
  temp[2] = light.quadraticAttenuation;
  ok &= r->dataIn(_g->AR_LIGHT_ATTENUATE,temp,AR_FLOAT,3);
  temp[0] = light.spotDirection[0];
  temp[1] = light.spotDirection[1];
  temp[2] = light.spotDirection[2];
  temp[3] = light.spotCutoff;
  temp[4] = light.spotExponent;
  ok &= r->dataIn(_g->AR_LIGHT_SPOT,&temp,AR_FLOAT,5);
  if (!ok) {
    delete r;
    return NULL;
  }
  return r;
}
