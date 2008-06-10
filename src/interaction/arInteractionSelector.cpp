//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInteractionSelector.h"
#include "arEffector.h"

arDistanceInteractionSelector::arDistanceInteractionSelector( float maxDistance ) :
  _maxDistance( maxDistance ) {
}

float arDistanceInteractionSelector::calcDistance(
                              const arEffector& effector,
                              const arMatrix4& objectMatrix ) const {
  const float distance = (ar_ET(effector.getMatrix()) - ar_ET(objectMatrix)).magnitude();
  return (_maxDistance > 0.) && (distance > _maxDistance) ? -1. : distance;
}

arInteractionSelector* arDistanceInteractionSelector::copy() const {
  return (arInteractionSelector*) new arDistanceInteractionSelector( _maxDistance );
}

arAngleInteractionSelector::arAngleInteractionSelector(
                                       float maxAngle ) :
  _maxAngleDist( (float)(1. - cos((double)maxAngle)) ) {
}
float arAngleInteractionSelector::calcDistance(
                              const arEffector& effector,
                              const arMatrix4& objectMatrix ) const {
  const arVector3 basePos(ar_ET( effector.getBaseMatrix() ));
  const arVector3 tipPos(ar_ET( effector.getMatrix() ));
  const arVector3 objectPos(ar_ET( objectMatrix ));

  arVector3 objectDirection(objectPos - basePos);
  if (objectDirection.zero()) {
    // ar_log_error() << "arAngleInteractionSelector: zero distance.\n";
    return 0.; // is this correct?
  }
  objectDirection = objectDirection.normalize();

  arVector3 tipDirection(tipPos - basePos);
  if (tipDirection.zero()) {
    // ar_log_error() << "arAngleInteractionSelector: zero distance.\n";
    return 0.; // is this correct?
  }
  tipDirection = tipDirection.normalize();

  //  const float angleDist = ar_convertToDeg(acos( objectDirection.dot( tipDirection ) ));
  //  cerr << objectPos << tipPos << basePos << endl;
  const float angleDist = 1. - objectDirection.dot( tipDirection );
  return (angleDist > _maxAngleDist) ? -1. : angleDist;
}

arInteractionSelector* arAngleInteractionSelector::copy() const {
  arAngleInteractionSelector* tmp = new arAngleInteractionSelector();
  if (!tmp) {
    ar_log_error() << "arAngleInteractionSelector copier out of memory.\n";
    return NULL;
  }
  tmp->_maxAngleDist = _maxAngleDist;
  return (arInteractionSelector*)tmp;
}
