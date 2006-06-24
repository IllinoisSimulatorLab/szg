//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInteractionSelector.h"
#include "arEffector.h"

arDistanceInteractionSelector::arDistanceInteractionSelector(
                                       float maxDistance ) :
  _maxDistance( maxDistance ) {
}

float arDistanceInteractionSelector::calcDistance(
                              const arEffector& effector,
                              const arMatrix4& objectMatrix ) const {
  float distance = (ar_extractTranslation( effector.getMatrix() ) -
                    ar_extractTranslation( objectMatrix )).magnitude();
  if ((_maxDistance > 0.)&&(distance > _maxDistance))
    return -1.;
  return distance;
}

arInteractionSelector* arDistanceInteractionSelector::copy() const {
  return (arInteractionSelector*)new arDistanceInteractionSelector( _maxDistance );
}

arAngleInteractionSelector::arAngleInteractionSelector(
                                       float maxAngle ) :
  _maxAngleDist( (float)(1. - cos((double)maxAngle)) ) {
}
float arAngleInteractionSelector::calcDistance(
                              const arEffector& effector,
                              const arMatrix4& objectMatrix ) const {
  const arVector3 basePos = ar_extractTranslation( effector.getBaseMatrix() );
  const arVector3 tipPos = ar_extractTranslation( effector.getMatrix() );
  const arVector3 objectPos = ar_extractTranslation( objectMatrix );
  const arVector3 objectDirection = (objectPos - basePos).normalize();
  const arVector3 tipDirection = (tipPos - basePos).normalize();
//  const float angleDist = ar_convertToDeg(acos( objectDirection.dot( tipDirection ) ));
//  cerr << objectPos << tipPos << basePos << endl;
  const float angleDist = 1. - objectDirection.dot( tipDirection );
  if (angleDist > _maxAngleDist) {
    return -1.;
  }
  return angleDist;
}

arInteractionSelector* arAngleInteractionSelector::copy() const {
  arAngleInteractionSelector* tmp = new arAngleInteractionSelector();
  tmp->_maxAngleDist = _maxAngleDist;
  return (arInteractionSelector*)tmp;
}
