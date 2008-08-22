//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNavigationUtilities.h"

namespace arNavigationSpace {
  bool _navInverseDirty = false;
  bool _fBbox = false;
  arLock _l;
  arVector3 _vBboxMin, _vBboxMax;
  arMatrix4 _navMatrix;
  arMatrix4 _navInvMatrix;
  void _set(const arMatrix4&);
  void _bound();
};

void arNavigationSpace::_bound() {
  if (!arNavigationSpace::_fBbox)
    return;

  // Translate back into bounding box.
  // Use shortcut, the guts of ar_extractTranslation().
  float* pos = (arNavigationSpace::_navMatrix.v + 12);
  for (int i=0; i<3; ++i) {
    if (pos[i] < arNavigationSpace::_vBboxMin[i])
      pos[i] = arNavigationSpace::_vBboxMin[i];
    if (pos[i] > arNavigationSpace::_vBboxMax[i])
      pos[i] = arNavigationSpace::_vBboxMax[i];
  }
}

inline void arNavigationSpace::_set(const arMatrix4& m) {
  arNavigationSpace::_navMatrix = m;
  arNavigationSpace::_bound();
  arNavigationSpace::_navInverseDirty = true;
}

void ar_setNavMatrix( const arMatrix4& matrix ) {
  arGuard _(arNavigationSpace::_l, "ar_setNavMatrix");
  arNavigationSpace::_set(matrix);
}

void ar_navTranslate( const arVector3& vec ) {
  arGuard _(arNavigationSpace::_l, "ar_navTranslate");
  arNavigationSpace::_set(
    arNavigationSpace::_navMatrix * ar_translationMatrix(vec));
}

void ar_navRotate( const arVector3& axis, float degrees ) {
  arGuard _(arNavigationSpace::_l, "ar_navRotate");
  arNavigationSpace::_navMatrix = arNavigationSpace::_navMatrix *
    ar_rotationMatrix(axis, M_PI/180.*degrees);
  arNavigationSpace::_navInverseDirty = true;
}

void ar_navBoundingbox( const arVector3& v1, const arVector3& v2 ) {
  // v1 and v2 can be *anything* -- _bound() behaves sensibly.
  arGuard _(arNavigationSpace::_l, "ar_navBoundingbox");
  arNavigationSpace::_fBbox = true;
  for (int i=0; i<3; ++i) {
    arNavigationSpace::_vBboxMin[i] = min(v1.v[i], v2.v[i]);
    arNavigationSpace::_vBboxMax[i] = max(v1.v[i], v2.v[i]);
  }
}

arMatrix4 ar_getNavMatrix() {
  arGuard _(arNavigationSpace::_l, "ar_getNavMatrix");
  return arNavigationSpace::_navMatrix;
}

arMatrix4 ar_getNavInvMatrix() {
  arGuard _(arNavigationSpace::_l, "ar_getNavInvMatrix");
  if (arNavigationSpace::_navInverseDirty) {
    // lazy evaluation
    arNavigationSpace::_navInverseDirty = false;
    arNavigationSpace::_navInvMatrix = arNavigationSpace::_navMatrix.inverse();
  }
  return arNavigationSpace::_navInvMatrix;
}

arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix ) {
  return ar_getNavMatrix() * matrix;
}

arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix ) {
  return ar_getNavInvMatrix() * matrix;
}

arVector3 ar_pointFromNavCoords( const arVector3& vec ) {
  return ar_getNavInvMatrix() * vec;
}

arVector3 ar_pointToNavCoords( const arVector3& vec ) {
  return ar_getNavMatrix() * vec;
}

// "vector" is just a "point" relative to the origin.  Same thing.
//
// Wrong. Or rather, what these routines do is _not_ the same
// as e.g. ar_pointFromNavCoords(). Restored original functionality.
// JAC 10/20/06.
//
arVector3 ar_vectorFromNavCoords( const arVector3& vec ) {
  return ar_pointFromNavCoords(vec) - ar_pointFromNavCoords(arVector3(0, 0, 0));
}

arVector3 ar_vectorToNavCoords( const arVector3& vec ) {
  return ar_pointToNavCoords(vec) - ar_pointToNavCoords(arVector3(0, 0, 0));
}
