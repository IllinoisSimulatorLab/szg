/*
 * SpecialRelativityMath.cpp
 * 	contains utility functions for SchpRel
 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "SpecialRelativityMath.h"
#include <math.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif


/// returns distance squared from origin
double dist2(double x1, double y1, double z1) {return x1*x1+y1*y1+z1*z1;}
/// returns distance from origin
double dist(const arVector3 &vec1) { return sqrt(vec1%vec1); }

/// returns the special relativistic gamma value
double gamm (double v1, double c1) {return 1./sqrt(1.-v1*v1/c1/c1);}

/// returns time taken for light to reach this point given this speed
double thetime (const double &x1, const double &y1, const double &z1, const double &c1)
{ return sqrt(dist2(x1,y1,z1))/c1; }

/// changes "z" value according to passed in parameters
/** Basically computes the triangle formed from vector from origin to actual point,
 * and vector from origin to closest point on line consisting of
 * the point's past positions. Then we push the point back in time
 * until the time taken for the photon to reach the
 * origin at the speed of light equals the time taken for
 * the point to travel the distance we push it back. Simple, no?
 */
double updateVertex (double x1, double y1, double z1, double c1, double v1, double thegamma)
{
    double temp = c1*c1-v1*v1;
    double z2 = thegamma*z1;
    z2 += v1*(z2*v1-sqrt(z2*z2*v1*v1+temp*dist2(x1,y1,z2)))/temp;
    return z2;	/*the meat of the (linear) program */
}

#define mode 1
#define FLYMODE 1
#define linearVel 0
/// maps old vertices into drawable vertex positions
/** If linear velocity, we simply call updateVertex().
 *  Otherwise, we use the past selfPosition to determine
 *  the point that most closely satisfies my condition:
 *  The time it takes for the photon to hit our eye is equal to
 *  the amount of time the point is pushed along its previous position path.
 */
void relativisticTransform(ARfloat *vert, ARfloat *drawVert, 
                           s_updateValues &uv)
{
  int dd, ff;
  double temp, temp_best;	// errors we want to minimize
  arVector3 tempVec, bestVec;	// points corresponding to errors

  /// tempVecBase holds euclidean position of vert relative to observer
  arVector3 tempVecBase(vert);
  for (dd=0; dd<3; dd++)
    tempVecBase[dd] -= uv.selfOffset[dd];
  tempVecBase = uv.selfRotation*tempVecBase;

  if (linearVel){	// assume constant, linear velocity
    drawVert[2] = updateVertex(	tempVecBase[0], tempVecBase[1], tempVecBase[2],
				uv.lightspeed, uv.velocity, uv.gamma);
    drawVert[1] = tempVecBase[1];
    drawVert[0] = tempVecBase[0];
  }
  else{			// light propagation delay
    dd=ff=uv.howFarBack/2;
    temp_best = 10000;
    for (;;) {
      ff /= 2;
      if (ff < 1)
        break;
     
      tempVec = arVector3(tempVecBase[0]-uv.selfPosition[dd*3+0],
		          tempVecBase[1]-uv.selfPosition[dd*3+1],
                          tempVecBase[2]-uv.selfPosition[dd*3+2]);
      //tempVec = uv.selfRotation*tempVec;
      temp = uv.lightspeed*uv.selfPositionTimes[dd] - dist(uv.selfRotation*tempVec); 
      // if ( ABS(temp) <= theThreshold) break; // speed things up, if nec.
      if ( fabs(temp) < fabs(temp_best) ){
        bestVec = tempVec;
        temp_best = temp;
      }
      if (temp < 0) // simple binary search
        dd += ff;
      else
        dd -= ff;
    }
    drawVert[0] = bestVec[0];
    drawVert[1] = bestVec[1];
    drawVert[2] = bestVec[2];
  }
}




