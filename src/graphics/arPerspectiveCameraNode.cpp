//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"

#include <string>
using namespace std;

arPerspectiveCameraNode::arPerspectiveCameraNode(){
  _name = "persp_camera_node";
  _typeCode = AR_G_PERSP_CAMERA_NODE;
  _typeString = "persp camera";
}

arStructuredData* arPerspectiveCameraNode::dumpData(){
  // Caller is responsible for deleting.
  arGuard dummy(_nodeLock);
  return _dumpData(_nodeCamera, false);
}

bool arPerspectiveCameraNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_PERSP_CAMERA, inData->getID(), "arLightNode"))
    return false;

  arGuard dummy(_nodeLock);
  inData->dataOut(_g->AR_PERSP_CAMERA_CAMERA_ID, &_nodeCamera.cameraID, AR_INT, 1);
  inData->dataOut(_g->AR_PERSP_CAMERA_FRUSTUM,_nodeCamera.frustum,AR_FLOAT,6);
  inData->dataOut(_g->AR_PERSP_CAMERA_LOOKAT,_nodeCamera.lookat,AR_FLOAT,9);
  _owningDatabase->registerCamera(this,&_nodeCamera);
  return true;
}

void arPerspectiveCameraNode::deactivate(){
  // There must be an owning database since that's who calls this method.
  _owningDatabase->removeCamera(this);
}

arPerspectiveCamera arPerspectiveCameraNode::getCamera(){
  arGuard dummy(_nodeLock);
  return _nodeCamera;
}

void arPerspectiveCameraNode::setCamera(const arPerspectiveCamera& camera){
  if (active()){
    _nodeLock.lock();
      arStructuredData* r = _dumpData(camera, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
      _nodeCamera = camera;
    _nodeLock.unlock();
  }
}

// NOT thread-safe.
arStructuredData* arPerspectiveCameraNode::_dumpData
  (const arPerspectiveCamera& camera, bool owned){
  arStructuredData* r = owned ?
    getOwner()->getDataParser()->getStorage(_g->AR_PERSP_CAMERA) :
    _g->makeDataRecord(_g->AR_PERSP_CAMERA);
  _dumpGenericNode(r, _g->AR_PERSP_CAMERA_ID);
  // todo: test datains' return value
  r->dataIn(_g->AR_PERSP_CAMERA_CAMERA_ID,&camera.cameraID,AR_INT,1);
  r->dataIn(_g->AR_PERSP_CAMERA_FRUSTUM,camera.frustum,AR_FLOAT,6);
  r->dataIn(_g->AR_PERSP_CAMERA_LOOKAT,camera.lookat,AR_FLOAT,9);
  return r;
}
