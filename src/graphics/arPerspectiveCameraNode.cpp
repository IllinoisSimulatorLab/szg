//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"
#include <string>
using namespace std;

arPerspectiveCameraNode::arPerspectiveCameraNode(){
  // A sensible default name.
  _name = "persp_camera_node";
  _typeCode = AR_G_PERSP_CAMERA_NODE;
  _typeString = "persp camera";
}

arStructuredData* arPerspectiveCameraNode::dumpData(){
  // Caller is responsible for deleting.
  ar_mutex_lock(&_nodeLock);
  arStructuredData* r = _dumpData(_nodeCamera, false);
  ar_mutex_unlock(&_nodeLock);
  return r;
}

bool arPerspectiveCameraNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_PERSP_CAMERA){
    cerr << "arLightNode error: expected "
    << _g->AR_PERSP_CAMERA
    << " (" << _g->_stringFromID(_g->AR_PERSP_CAMERA) << "), not "
    << inData->getID()
    << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }
  ar_mutex_lock(&_nodeLock);
  inData->dataOut(_g->AR_PERSP_CAMERA_CAMERA_ID,
                  &_nodeCamera.cameraID, AR_INT, 1);
  inData->dataOut(_g->AR_PERSP_CAMERA_FRUSTUM,_nodeCamera.frustum,AR_FLOAT,6);
  inData->dataOut(_g->AR_PERSP_CAMERA_LOOKAT,_nodeCamera.lookat,AR_FLOAT,9);

  // register it with the database
  _owningDatabase->registerCamera(this,&_nodeCamera);
  ar_mutex_unlock(&_nodeLock);
  return true;
}

void arPerspectiveCameraNode::deactivate(){
  // There must be an owning database since that's who calls this method.
  _owningDatabase->removeCamera(this);
}

arPerspectiveCamera arPerspectiveCameraNode::getCamera(){
  ar_mutex_lock(&_nodeLock);
  arPerspectiveCamera r = _nodeCamera;
  ar_mutex_unlock(&_nodeLock);
  return r;
}

void arPerspectiveCameraNode::setCamera(const arPerspectiveCamera& camera){
  if (active()){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(camera, true);
    ar_mutex_unlock(&_nodeLock);
    _owningDatabase->alter(r);
    _owningDatabase->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _nodeCamera = camera;
    ar_mutex_unlock(&_nodeLock);
  }
}

/// NOT thread-safe.
arStructuredData* arPerspectiveCameraNode::_dumpData
  (const arPerspectiveCamera& camera, bool owned){
  arStructuredData* result = NULL;
  if (owned){
    result = getOwner()->getDataParser()->getStorage(_g->AR_PERSP_CAMERA);
  }
  else{
    result = _g->makeDataRecord(_g->AR_PERSP_CAMERA);
  }
  _dumpGenericNode(result,_g->AR_PERSP_CAMERA_ID);
  result->dataIn(_g->AR_PERSP_CAMERA_CAMERA_ID,&camera.cameraID,AR_INT,1);
  result->dataIn(_g->AR_PERSP_CAMERA_FRUSTUM,camera.frustum,AR_FLOAT,6);
  result->dataIn(_g->AR_PERSP_CAMERA_LOOKAT,camera.lookat,AR_FLOAT,9);
  return result;
}
