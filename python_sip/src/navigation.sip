%Include typemaps.sip

%ModuleHeaderCode
#include "arNavigationUtilities.h"
%End

void ar_setNavMatrix( const arMatrix4& matrix );
arMatrix4 ar_getNavMatrix() /TransferBack/;
arMatrix4 ar_getNavInvMatrix() /TransferBack/;
void ar_navTranslate( const arVector3& vec );
void ar_navRotate( const arVector3& axis, float degrees );
/// replace master/slave virtualize...() calls
/// Specifically, this is taking a vector, etc. in tracker coordinates
/// and putting it into world coordinates.
arMatrix4 ar_matrixToNavCoords( const arMatrix4& matrix ) /TransferBack/;
arVector3 ar_pointToNavCoords( const arVector3& vec ) /TransferBack/;
arVector3 ar_vectorToNavCoords( const arVector3& vec ) /TransferBack/;
/// replace master/slave reify...() calls
/// Taking a vector, etc. in world coordinates and putting it in tracker
/// coordinates.
arMatrix4 ar_matrixFromNavCoords( const arMatrix4& matrix ) /TransferBack/;
arVector3 ar_pointFromNavCoords( const arVector3& vec ) /TransferBack/;
arVector3 ar_vectorFromNavCoords( const arVector3& vec ) /TransferBack/;

