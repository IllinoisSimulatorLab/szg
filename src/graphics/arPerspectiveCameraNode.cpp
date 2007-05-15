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
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_nodeCamera, false);
  _nodeLock.unlock();
  return r;
}

bool arPerspectiveCameraNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_PERSP_CAMERA, inData->getID(), "arLightNode"))
    return false;

  _nodeLock.lock();
  inData->dataOut(_g->AR_PERSP_CAMERA_CAMERA_ID,
                  &_nodeCamera.cameraID, AR_INT, 1);
  inData->dataOut(_g->AR_PERSP_CAMERA_FRUSTUM,_nodeCamera.frustum,AR_FLOAT,6);
  inData->dataOut(_g->AR_PERSP_CAMERA_LOOKAT,_nodeCamera.lookat,AR_FLOAT,9);

  // register it with the database
  _owningDatabase->registerCamera(this,&_nodeCamera);
  _nodeLock.unlock();
  return true;
}

void arPerspectiveCameraNode::deactivate(){
  // There must be an owning database since that's who calls this method.
  _owningDatabase->removeCamera(this);
}

arPerspectiveCamera arPerspectiveCameraNode::getCamera(){
  _nodeLock.lock();
    const arPerspectiveCamera r = _nodeCamera;
  _nodeLock.unlock();
  return r;
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
  arStructuredData* result = owned ?
    getOwner()->getDataParser()->getStorage(_g->AR_PERSP_CAMERA) :
    _g->makeDataRecord(_g->AR_PERSP_CAMERA);
  _dumpGenericNode(result,_g->AR_PERSP_CAMERA_ID);
  result->dataIn(_g->AR_PERSP_CAMERA_CAMERA_ID,&camera.cameraID,AR_INT,1);
  result->dataIn(_g->AR_PERSP_CAMERA_FRUSTUM,camera.frustum,AR_FLOAT,6);
  result->dataIn(_g->AR_PERSP_CAMERA_LOOKAT,camera.lookat,AR_FLOAT,9);
  return result;
}
