//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arController.h"

arController::arController() :
  _transformID(-1), // uninitialized
  _theDatabase(NULL),
  _transformData(NULL)
{
}

arController::~arController(){
  if (_transformData){
    delete _transformData;
  }
}

void arController::setTransformID(int transformID){
  _transformID = transformID;
}

void arController::setDatabase(arGraphicsDatabase* theDatabase){
  _theDatabase = theDatabase;
}

/// \todo kludge! Need an interface class that returns an appropriate arStructuredData record without going through arGraphicsDatabase.
void arController::setTransform(const arMatrix4& transform){
  _transform = transform;
  if (_transformID>=0 && _theDatabase != NULL){
    if (!_transformData){
      
      _transformData = _theDatabase->_gfx.makeDataRecord
        (_theDatabase->_gfx.AR_TRANSFORM);
    }
    if (!_transformData->dataIn(
           _theDatabase->_gfx.AR_TRANSFORM_ID,&_transformID,AR_INT,1) ||
        !_transformData->dataIn(
           _theDatabase->_gfx.AR_TRANSFORM_MATRIX,transform.v,AR_FLOAT,16)) {
      cerr << "arController warning: problem in setTransform.\n";
    }
    (void)_theDatabase->alter(_transformData);
  }
}
