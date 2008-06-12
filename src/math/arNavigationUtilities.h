#ifndef ARNAVIGATION_H
#define ARNAVIGATION_H

#include "arMath.h"
#include "arMathCalling.h"

SZG_CALL void ar_setNavMatrix( const arMatrix4& matrix );
SZG_CALL arMatrix4 ar_getNavMatrix();
SZG_CALL arMatrix4 ar_getNavInvMatrix();
SZG_CALL void ar_navTranslate( const arVector3& vec );
SZG_CALL void ar_navRotate( const arVector3& axis, float degrees );
SZG_CALL void ar_navBoundingbox( const arVector3&, const arVector3& );

// Convert from tracker to world coordinates.
SZG_CALL arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix );
SZG_CALL arVector3 ar_pointToNavCoords( const arVector3& vec );
SZG_CALL arVector3 ar_vectorToNavCoords( const arVector3& vec );

// Convert from world to tracker coordinates.
SZG_CALL arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix );
SZG_CALL arVector3 ar_pointFromNavCoords( const arVector3& vec );
SZG_CALL arVector3 ar_vectorFromNavCoords( const arVector3& vec );

#endif
