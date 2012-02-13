#ifndef ARGLUSPHERE_H
#define ARGLUSPHERE_H

#include "arGluQuadric.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arGluSphere : public arGluQuadric {
	public:
    arGluSphere( double radius, int slices=30, int stacks=5 );
    arGluSphere( const arGluSphere& x );
    arGluSphere& operator=( const arGluSphere& x );
    virtual ~arGluSphere() {}
    virtual void draw();
    void setRadius( double radius );
    void setSlicesStacks( int slices, int stacks );
	protected:
	private:
    double _radius;
    int _slices;
    int _stacks;
};

#endif        //  #ifndefARGLUSPHERE_H

