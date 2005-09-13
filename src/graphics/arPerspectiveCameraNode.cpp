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
  return _dumpData(_nodeCamera);
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

  inData->dataOut(_g->AR_PERSP_CAMERA_CAMERA_ID,
                  &_nodeCamera.cameraID, AR_INT, 1);
  inData->dataOut(_g->AR_PERSP_CAMERA_FRUSTUM,_nodeCamera.frustum,AR_FLOAT,6);
  inData->dataOut(_g->AR_PERSP_CAMERA_LOOKAT,_nodeCamera.lookat,AR_FLOAT,9);

  // register it with the database
  _owningDatabase->registerCamera(getID(),&_nodeCamera);
  return true;
}

void arPerspectiveCameraNode::setCamera(const arPerspectiveCamera& camera){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(camera);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _nodeCamera = camera;
  }
}

arStructuredData* arPerspectiveCameraNode::_dumpData
  (const arPerspectiveCamera& camera){
  arStructuredData* result = _g->makeDataRecord(_g->AR_PERSP_CAMERA);
  _dumpGenericNode(result,_g->AR_PERSP_CAMERA_ID);
  result->dataIn(_g->AR_PERSP_CAMERA_CAMERA_ID,&camera.cameraID,AR_INT,1);
  result->dataIn(_g->AR_PERSP_CAMERA_FRUSTUM,camera.frustum,AR_FLOAT,6);
  result->dataIn(_g->AR_PERSP_CAMERA_LOOKAT,camera.lookat,AR_FLOAT,9);
  return result;
}
