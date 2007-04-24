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

arStructuredData* arLightNode::dumpData(){
  // Caller is responsible for deleting.
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_nodeLight, false);
  _nodeLock.unlock();
  return r;
}

bool arLightNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_LIGHT){
    cerr << "arLightNode error: expected "
    << _g->AR_LIGHT
    << " (" << _g->_stringFromID(_g->AR_LIGHT) << "), not "
    << inData->getID()
    << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }

  _nodeLock.lock();
  inData->dataOut(_g->AR_LIGHT_LIGHT_ID,&_nodeLight.lightID,AR_INT,1);
  inData->dataOut(_g->AR_LIGHT_POSITION,_nodeLight.position.v,AR_FLOAT,4);
  inData->dataOut(_g->AR_LIGHT_DIFFUSE,_nodeLight.diffuse.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_LIGHT_AMBIENT,_nodeLight.ambient.v,AR_FLOAT,3);
  inData->dataOut(_g->AR_LIGHT_SPECULAR,_nodeLight.specular.v,AR_FLOAT,3);
  // Each data member should get a seperate field. THIS IS A PROBLEM.
  float temp[5];
  inData->dataOut(_g->AR_LIGHT_ATTENUATE,temp,AR_FLOAT,3);
  _nodeLight.constantAttenuation = temp[0];
  _nodeLight.linearAttenuation = temp[1];
  _nodeLight.quadraticAttenuation = temp[3];
  inData->dataOut(_g->AR_LIGHT_SPOT,temp,AR_FLOAT,5);
  _nodeLight.spotDirection = arVector3(temp[0], temp[1], temp[2]);
  _nodeLight.spotCutoff = temp[3];
  _nodeLight.spotExponent = temp[4];

  // Register it with the database.
  _owningDatabase->registerLight(this,&_nodeLight);
  _nodeLock.unlock();
  return true;
}

void arLightNode::deactivate(){
  // There must be an owning database since that's who calls this method.
  _owningDatabase->removeLight(this);
}


arLight arLightNode::getLight(){
  _nodeLock.lock();
  arLight r = _nodeLight;
  _nodeLock.unlock();
  return r;
}

void arLightNode::setLight(arLight& light){
  if (active()){
    _nodeLock.lock();
    arStructuredData* r = _dumpData(light, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
    _nodeLight = light;
    _nodeLock.unlock();
  }
}

arStructuredData* arLightNode::_dumpData(arLight& light, bool owned){
  arStructuredData* result = NULL;
  if (owned){
    result = getOwner()->getDataParser()->getStorage(_g->AR_LIGHT);
  }
  else{
    result = _g->makeDataRecord(_g->AR_LIGHT);
  }
  _dumpGenericNode(result,_g->AR_LIGHT_ID); 
  result->dataIn(_g->AR_LIGHT_LIGHT_ID,&light.lightID,AR_INT,1);
  result->dataIn(_g->AR_LIGHT_POSITION,light.position.v,AR_FLOAT,4);
  result->dataIn(_g->AR_LIGHT_DIFFUSE,light.diffuse.v,AR_FLOAT,3);
  result->dataIn(_g->AR_LIGHT_AMBIENT,light.ambient.v,AR_FLOAT,3);
  result->dataIn(_g->AR_LIGHT_SPECULAR,light.specular.v,AR_FLOAT,3);
  // Each data member should get a seperate field. THIS IS A PROBLEM.
  float temp[5];
  temp[0] = light.constantAttenuation;
  temp[1] = light.linearAttenuation;
  temp[2] = light.quadraticAttenuation;
  result->dataIn(_g->AR_LIGHT_ATTENUATE,temp,AR_FLOAT,3);
  temp[0] = light.spotDirection[0];
  temp[1] = light.spotDirection[1];
  temp[2] = light.spotDirection[2];
  temp[3] = light.spotCutoff;
  temp[4] = light.spotExponent;
  result->dataIn(_g->AR_LIGHT_SPOT,&temp,AR_FLOAT,5);
  return result;
}
