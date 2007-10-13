//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arHead.h"
#include "arGraphicsScreen.h"

ostream& operator<<(ostream& s, arHead& h) {
  s << "  Matrix         ";
  const arMatrix4 m(h.getMatrix());
  if (m==ar_identityMatrix())
    s << "identity\n";
  else
    s << m;

  s << "  MidEyeOffset   " << h.getMidEyeOffset() << endl
    << "  EyeDirection   " << h.getEyeDirection() << endl
    << "  EyeSpacing     " << h.getEyeSpacing() << endl
    << "  ClipPlanes     " << h.getNearClip() << " " << h.getFarClip() << endl
    << "  UnitConversion " << h.getUnitConversion() << endl
    << "  FixedHeadMode  " << h.getFixedHeadMode() << endl;
  return s;
}

arLogStream& operator<<(arLogStream& s, arHead& h) {
  s << "  Matrix         ";
  const arMatrix4 m(h.getMatrix());
  if (m==ar_identityMatrix())
    s << "identity\n";
  else
    s << m;

  s << "  MidEyeOffset   " << h.getMidEyeOffset() << ar_endl
    << "  EyeDirection   " << h.getEyeDirection() << ar_endl
    << "  EyeSpacing     " << h.getEyeSpacing() << ar_endl
    << "  ClipPlanes     " << h.getNearClip() << " " << h.getFarClip() << ar_endl
    << "  UnitConversion " << h.getUnitConversion() << ar_endl
    << "  FixedHeadMode  " << h.getFixedHeadMode() << ar_endl;
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
  ar_log_remark() << "Head:\n" << *this;
  return true;
}

arVector3 arHead::getEyePosition( float eyeSign, const arMatrix4* M ) const {
  const arVector3 offset(_midEyeOffset +
    0.5 *eyeSign * _eyeSpacing * _eyeDirection.normalize());
  return _unitConversion * ((M ? *M : _matrix) * offset);
}

arVector3 arHead::getMidEyePosition( const arMatrix4* M ) const {
  return _unitConversion * ((M ? *M : _matrix) * _midEyeOffset);
}

arMatrix4 arHead::getMidEyeMatrix() const {
  arMatrix4 M(_matrix * ar_translationMatrix(_midEyeOffset) );
  if (ar_angleBetween( arVector3(1,0,0), _eyeDirection ) > .01)
    M = M * ar_rotateVectorToVector( arVector3(1,0,0), _eyeDirection );
  for (int i=12; i<15; ++i)
    M.v[i] *= _unitConversion;
  return M;
}
