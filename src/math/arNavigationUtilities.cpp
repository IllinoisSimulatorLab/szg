//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNavigationUtilities.h"
#include "arThread.h"

namespace arNavigationSpace {
  bool _navInverseDirty = false;
  bool _mutexInited = false;
  bool _fBbox = false;
  arMutex _navMutex;
  arVector3 _vBboxMin, _vBboxMax;
  arMatrix4 _navMatrix;
  arMatrix4 _navInvMatrix;
  void _set(const arMatrix4&);
  void _bound();
  void _lock();
  void _unlock();
};

void arNavigationSpace::_lock() {
  if (!arNavigationSpace::_mutexInited){
    ar_mutex_init( &arNavigationSpace::_navMutex );
    arNavigationSpace::_mutexInited = true;
  }
  ar_mutex_lock( &arNavigationSpace::_navMutex );
}

void arNavigationSpace::_unlock() {
  ar_mutex_unlock( &arNavigationSpace::_navMutex );
}

void arNavigationSpace::_bound() {
  if (!arNavigationSpace::_fBbox)
    return;

  // Translate back inside bounding box.
  // Use shortcut, the guts of ar_extractTranslation().
  float* pos = (arNavigationSpace::_navMatrix.v + 12);
  for (int i=0; i<3; ++i) {
    if (pos[i] < arNavigationSpace::_vBboxMin[i])
      pos[i] = arNavigationSpace::_vBboxMin[i];
    else if (pos[i] > arNavigationSpace::_vBboxMax[i])
      pos[i] = arNavigationSpace::_vBboxMax[i];
  }
}

inline void arNavigationSpace::_set(const arMatrix4& m) {
  arNavigationSpace::_navMatrix = m;
  arNavigationSpace::_bound();
  arNavigationSpace::_navInverseDirty = true;
}

void ar_setNavMatrix( const arMatrix4& matrix ) {
  arNavigationSpace::_lock();
  arNavigationSpace::_set(matrix);
  arNavigationSpace::_unlock();
}

void ar_navTranslate( const arVector3& vec ) {
  arNavigationSpace::_lock();
  arNavigationSpace::_set(
    arNavigationSpace::_navMatrix * ar_translationMatrix(vec));
  arNavigationSpace::_unlock();
}

void ar_navRotate( const arVector3& axis, float degrees ) {
  arNavigationSpace::_lock();
  arNavigationSpace::_navMatrix = arNavigationSpace::_navMatrix *
    ar_rotationMatrix(axis, M_PI/180.*degrees);
  arNavigationSpace::_navInverseDirty = true;
  arNavigationSpace::_unlock();
}

void ar_navBoundingbox( const arVector3& v1, const arVector3& v2 ) {
  // v1 and v2 can be *anything* -- _bound() behaves sensibly.
  arNavigationSpace::_lock();
  arNavigationSpace::_fBbox = true;
  for (int i=0; i<3; ++i) {
    arNavigationSpace::_vBboxMin[i] = min(v1.v[i], v2.v[i]);
    arNavigationSpace::_vBboxMax[i] = max(v1.v[i], v2.v[i]);
  }
  arNavigationSpace::_unlock();
}

arMatrix4 ar_getNavMatrix() {
  arNavigationSpace::_lock();
  arMatrix4 result( arNavigationSpace::_navMatrix );
  arNavigationSpace::_unlock();
  return result;
}

arMatrix4 ar_getNavInvMatrix() {
  arNavigationSpace::_lock();
  if (arNavigationSpace::_navInverseDirty) {
    // lazy evaluation
    arNavigationSpace::_navInverseDirty = false;
    arNavigationSpace::_navInvMatrix = arNavigationSpace::_navMatrix.inverse();
  }
  arMatrix4 result( arNavigationSpace::_navInvMatrix );
  arNavigationSpace::_unlock();
  return result;
}

arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix ) {
  return ar_getNavMatrix() * matrix;
}

arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix ) {
  return ar_getNavInvMatrix() * matrix;
}

arVector3 ar_pointFromNavCoords( const arVector3& vec ){
  return ar_getNavInvMatrix() * vec;
}

arVector3 ar_pointToNavCoords( const arVector3& vec ){
  return ar_getNavMatrix() * vec;
}

// "vector" is just a "point" relative to the origin.  Same thing.
//
// Wrong. Or rather, what these routines do is _not_ the same
// as e.g. ar_pointFromNavCoords(). Restored original functionality.
// JAC 10/20/06.
//
arVector3 ar_vectorFromNavCoords( const arVector3& vec ) {
  arVector3 result( ar_pointFromNavCoords(vec) 
                    - ar_pointFromNavCoords(arVector3(0,0,0)) );
  return result;
}

arVector3 ar_vectorToNavCoords( const arVector3& vec ){
  arVector3 result( ar_pointToNavCoords(vec) 
                    - ar_pointToNavCoords(arVector3(0,0,0)) );
  return result;
}
