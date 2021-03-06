//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arConstantHeadFilter.h"
#include "arVRConstants.h"

DriverFactory(arConstantHeadFilter, "arIOFilter")

bool arConstantHeadFilter::_processEvent( arInputEvent& inputEvent ) {
  if (inputEvent.getType() != AR_EVENT_MATRIX || inputEvent.getIndex() != AR_VR_HEAD_MATRIX_ID)
    return true;

  return inputEvent.setMatrix(
    arMatrix4(1,0,0,0, 0,1,0,5, 0,0,1,0, 0,0,0,1) *
    ar_extractRotationMatrix(inputEvent.getMatrix()));
}
