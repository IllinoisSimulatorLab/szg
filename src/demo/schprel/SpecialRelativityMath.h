#ifndef __SPECIALRELATIVITYMATH_H__
#define __SPECIALRELATIVITYMATH_H__

/*
 * SpecialRelativityMath.h
 *         contains utility functions for SchpRel
 */

#include "arGraphicsAPI.h"

// info to calculate special relativity
struct s_updateValues {
  double        velocity;
  double        lightspeed;
  double        gamma;
  arMatrix4        selfRotation;
  ARfloat        *selfPosition;
  ARfloat        *selfPositionTimes;
  unsigned int        howFarBack;
  arVector3        selfOffset;
};

double dist2(double x1, double y1, double z1);
double dist(const arVector3 &vec1);

double gamm (double v1, double c1);

double thetime (const double &x1, const double &y1, const double &z1, const double &c1);

double updateVertex (double x1, double y1, double z1, double c1, double v1, double thegamma);

void relativisticTransform(ARfloat *vert, ARfloat *drawVert, s_updateValues &updateValues);

#endif
