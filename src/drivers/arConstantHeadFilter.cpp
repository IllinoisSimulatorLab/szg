//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arConstantHeadFilter.h"
#include "arVRConstants.h"

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arConstantHeadFilter();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arIOFilter", buffer, size);
  }
}

bool arConstantHeadFilter::_processEvent( arInputEvent& inputEvent ) {
  if ((inputEvent.getType() == AR_EVENT_MATRIX)&&
    (inputEvent.getIndex() == AR_VR_HEAD_MATRIX_ID)) {
      arMatrix4 rotation = ar_extractRotationMatrix(inputEvent.getMatrix());
      arMatrix4 constantHead = arMatrix4(1,0,0,0,
					 0,1,0,5,
					 0,0,1,0,
					 0,0,0,1)*rotation;
      return inputEvent.setMatrix(constantHead);
  }
  return true;
}  

//arStructuredData* arConstantHeadFilter::filter(arStructuredData* d) {
//  int count = ar_getNumberEvents(d);
//  int* indexPtr = (int*) d->getDataPtr("indices",AR_INT);
//  for (int i=0; i<count; i++){
//    if (ar_getEventType(i,d) == AR_EVENT_MATRIX
//	&& indexPtr[i] == AR_VR_HEAD_MATRIX_ID){
//      arMatrix4 rotation = ar_extractRotationMatrix
//	(ar_getEventMatrixValue(i,d));
//      arMatrix4 constantHead = arMatrix4(1,0,0,0,
//					 0,1,0,5,
//					 0,0,1,0,
//					 0,0,0,1)*rotation;
//      ar_replaceMatrixEvent(i,constantHead,d);
//    }
//  }
//  return d;
//}
