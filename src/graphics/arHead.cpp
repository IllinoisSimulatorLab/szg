//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arHead.h"
#include "arGraphicsScreen.h"

arHead::arHead() :
  _midEyeOffset(0,0,0),
  _eyeDirection(1,0,0),
  _eyeSpacing(0.2),
  _nearClip(.1),
  _farClip(100.),
  _unitConversion(1.),
  _fixedHeadMode(0) {
}

bool arHead::configure( arSZGClient& client ) {
  stringstream& initResponse = client.initResponse();
  //
  // Eye/head parameters, set on master/controller only
  if (!client.getAttributeFloats( "SZG_HEAD","eye_spacing",&_eyeSpacing )){
    initResponse << "arScreenObject remark: "
		 << "SZG_HEAD/eye_spacing "
		 << "defaulting to " << _eyeSpacing << endl;
  }
  if (!client.getAttributeVector3( "SZG_HEAD", "eye_direction", _eyeDirection )) {
    _eyeDirection = arVector3(1,0,0);
  }

  if (!client.getAttributeVector3( "SZG_HEAD", "mid_eye_offset", _midEyeOffset )) {
    _midEyeOffset = arVector3(0,0,0);
  }

  _fixedHeadMode = client.getAttribute("SZG_HEAD","fixed_head_mode","|false|true|") == "true";

  cerr << "Head configuration: " << *this << endl;
  return true;
}

arVector3 arHead::getEyePosition( float eyeSign, arMatrix4* useMatrix ) {
  arMatrix4* matPtr = (useMatrix)?(useMatrix):(&_matrix);
  const arVector3 eyeOffsetVector = _midEyeOffset +
    0.5 *eyeSign * _eyeSpacing * _eyeDirection.normalize();
  // The eye position is a quantity we need computed.
  return _unitConversion * (*matPtr * eyeOffsetVector);
}

arVector3 arHead::getMidEyePosition( arMatrix4* useMatrix ) {
  arMatrix4* matPtr = (useMatrix)?(useMatrix):(&_matrix);
  // The mid-eyes position is a quantity we need computed.
  return _unitConversion * (*matPtr * _midEyeOffset);
}

arMatrix4 arHead::getMidEyeMatrix() const {
  return _unitConversion * (_matrix * ar_translationMatrix(_midEyeOffset));
}

ostream& operator<<(ostream& s, const arHead& h) {
  s << "Head:\n  Matrix:\n" << h._matrix;
  s << "  MidEyeOffset  : " << h._midEyeOffset << endl;
  s << "  EyeDirection  : " << h._eyeDirection << endl;
  s << "  EyeSpacing    : " << h._eyeSpacing << endl;
  s << "  ClipPlanes    : " << h._nearClip << ", " << h._farClip << endl;
  s << "  UnitConversion: " << h._unitConversion << endl;
  s << "  FixedHeadMode      : " << h._fixedHeadMode << endl;
  return s;
}
