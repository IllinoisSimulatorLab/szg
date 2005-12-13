//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arHead.h"
#include "arGraphicsScreen.h"

ostream& operator<<(ostream& s, arHead& h) {
  s << "Head:\n  Matrix:\n" << h.getMatrix()
    << "  MidEyeOffset    : " << h.getMidEyeOffset() << endl
    << "  EyeDirection    : " << h.getEyeDirection() << endl
    << "  EyeSpacing      : " << h.getEyeSpacing() << endl
    << "  ClipPlanes      : " << h.getNearClip() << ", " 
                              << h.getFarClip() << endl
    << "  UnitConversion  : " << h.getUnitConversion() << endl
    << "  FixedHeadMode   : " << h.getFixedHeadMode() << endl;
  return s;
}

arHead::arHead() :
  _midEyeOffset(0,0,0),
  _eyeDirection(1,0,0),
  _eyeSpacing(0.2),
  _nearClip(.1),
  _farClip(100.),
  _unitConversion(1.),
  _fixedHeadMode(false) {
}

bool arHead::configure( arSZGClient& client ) {
  stringstream& initResponse = client.initResponse();
  
  // Eye/head parameters, set on master/controller only
  if (!client.getAttributeFloats( "SZG_HEAD", "eye_spacing", &_eyeSpacing )){
    _eyeSpacing = 0.2;
  }
  if (!client.getAttributeVector3( "SZG_HEAD", "eye_direction", _eyeDirection )) {
    _eyeDirection = arVector3(1,0,0);
  }

  if (!client.getAttributeVector3( "SZG_HEAD", "mid_eye_offset", _midEyeOffset )) {
    _midEyeOffset = arVector3(0,0,0);
  }

  _fixedHeadMode = client.getAttribute("SZG_HEAD", "fixed_head_mode", "|false|true|") == "true";

  // Only print this information if, in fact, the arSZGClient is in *verbose* mode.
  if (client.getVerbosity()){
    initResponse << "arHead remark: head configuration: " << *this << endl;
  }
  return true;
}

arVector3 arHead::getEyePosition( float eyeSign, const arMatrix4* useMatrix ) const {
  const arMatrix4* matPtr = useMatrix ? useMatrix : &_matrix;
  const arVector3 eyeOffsetVector = _midEyeOffset +
    0.5 *eyeSign * _eyeSpacing * _eyeDirection.normalize();
  // The eye position is a quantity we need computed.
  return _unitConversion * (*matPtr * eyeOffsetVector);
}

arVector3 arHead::getMidEyePosition( const arMatrix4* useMatrix ) const {
  const arMatrix4* matPtr = useMatrix ? useMatrix : &_matrix;
  // The mid-eyes position is a quantity we need computed.
  return _unitConversion * (*matPtr * _midEyeOffset);
}

arMatrix4 arHead::getMidEyeMatrix() const {
  arMatrix4 mat(_matrix * ar_translationMatrix(_midEyeOffset) );
  if (ar_angleBetween( arVector3(1,0,0), _eyeDirection ) > .01)
    mat = mat * ar_rotateVectorToVector( arVector3(1,0,0),_eyeDirection );
  for (int i=12; i<15; ++i)
    mat.v[i] *= _unitConversion;
  return mat;
}
