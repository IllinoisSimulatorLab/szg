#include "arPrecompiled.h"
#include "arAngleThreshold.h"
#include "arSTLalgo.h"

float arAngleThreshold::scaledDistance( const arVector3 pos ) const {
  arVector3 position = pos - _viewPosition;
  const float distance = position.magnitude();
//cerr << _referencePosition << pos;
  position = position * (_refDistance/distance);
  const arVector3 diff = position - _referencePosition;
  // cast into r (radial), x (horizontal), y (orthogonal to other two) coords
  const arVector3 oldxhat(1,0,0), oldyhat(0,1,0), oldzhat(0,0,1);
  const arVector3 rhat = -_referencePosition.normalize();
  const arVector3 xhat = (oldyhat * rhat).normalize();
  const arVector3 yhat = rhat * xhat;
  const float x = diff.dot( xhat );
  const float y = diff.dot( yhat );
  const float xAngleScaled = atan( x/_refDistance )/_horizThreshold;
  const float yAngleScaled = atan( y/_refDistance )/_vertThreshold;
  float scDist = xAngleScaled*xAngleScaled + yAngleScaled*yAngleScaled;
//cerr << scDist << endl;
  return scDist;
}

bool arAngleThreshold::anyTooClose( std::vector<arVector3>& positions ) {
  return std::find_if( positions.begin(), positions.end(), *this )
                                                != positions.end();
}

bool arAngleThreshold::anyTooClose( const arVector3& ref,
                                    std::vector<arVector3>& positions ) {
  setReferencePosition( ref );
  return anyTooClose( positions );
}

