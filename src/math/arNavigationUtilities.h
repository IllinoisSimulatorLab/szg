#ifndef ARNAVIGATION_H
#define ARNAVIGATION_H

#include "arMath.h"
  
void ar_setNavMatrix( const arMatrix4& matrix );
arMatrix4 ar_getNavMatrix();
arMatrix4 ar_getNavInvMatrix();
void ar_navTranslate( const arVector3& vec );
void ar_navRotate( const arVector3& axis, float degrees );
/// replace master/slave virtualize...() calls
/// Specifically, this is taking a vector, etc. in tracker coordinates
/// and putting it into world coordinates.
arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix );
arVector3 ar_pointToNavCoords( const arVector3& vec );
arVector3 ar_vectorToNavCoords( const arVector3& vec );
/// replace master/slave reify...() calls
/// Taking a vector, etc. in world coordinates and putting it in tracker
/// coordinates.
arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix );
arVector3 ar_pointFromNavCoords( const arVector3& vec );
arVector3 ar_vectorFromNavCoords( const arVector3& vec );

#endif        //  #ifndefARNAVIGATION_H

