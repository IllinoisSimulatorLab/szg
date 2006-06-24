//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arNavigationUtilities.h"
#include "arThread.h"

namespace arNavigationSpace {
  arMatrix4 _navMatrix;
  arMatrix4 _navInvMatrix;
  bool _navInverseDirty = false;
  bool _mutexInited = false;
  arMutex _navMutex;
  void _navLock();
  void _navUnlock();
};

void arNavigationSpace::_navLock() {
  if (!arNavigationSpace::_mutexInited){
    ar_mutex_init( &arNavigationSpace::_navMutex );
    arNavigationSpace::_mutexInited = true;
  }
  ar_mutex_lock( &arNavigationSpace::_navMutex );
}

void arNavigationSpace::_navUnlock() {
  ar_mutex_unlock( &arNavigationSpace::_navMutex );
}

void ar_setNavMatrix( const arMatrix4& matrix ) {
  arNavigationSpace::_navLock();
  arNavigationSpace::_navMatrix = matrix;
  arNavigationSpace::_navInverseDirty = true;
  arNavigationSpace::_navUnlock();
}

arMatrix4 ar_getNavMatrix() {
  arNavigationSpace::_navLock();
  arMatrix4 result( arNavigationSpace::_navMatrix );
  arNavigationSpace::_navUnlock();
  return result;
}

arMatrix4 ar_getNavInvMatrix() {
  arNavigationSpace::_navLock();
  if (arNavigationSpace::_navInverseDirty) {
    arNavigationSpace::_navInvMatrix = arNavigationSpace::_navMatrix.inverse();
    arNavigationSpace::_navInverseDirty = false;
  }
  arMatrix4 result( arNavigationSpace::_navInvMatrix );
  arNavigationSpace::_navUnlock();
  return result;
}

void ar_navTranslate( const arVector3& vec ) {
  arNavigationSpace::_navLock();
  arNavigationSpace::_navMatrix 
    = arNavigationSpace::_navMatrix * ar_translationMatrix(vec);
  arNavigationSpace::_navInverseDirty = true;
  arNavigationSpace::_navUnlock();
}

void ar_navRotate( const arVector3& axis, float degrees ) {
  arNavigationSpace::_navLock();
  arNavigationSpace::_navMatrix = arNavigationSpace::_navMatrix 
    * ar_rotationMatrix(axis, 3.1415926535/180.*degrees);
  arNavigationSpace::_navInverseDirty = true;
  arNavigationSpace::_navUnlock();
}

arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix ) {
  return ar_getNavMatrix() * matrix;
}

arVector3 ar_pointFromNavCoords( const arVector3& vec ){
  return ar_getNavInvMatrix() * vec;
}

arVector3 ar_pointToNavCoords( const arVector3& vec ){
  return ar_getNavMatrix() * vec;
}

arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix ) {
  return ar_getNavInvMatrix() * matrix;
}

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


