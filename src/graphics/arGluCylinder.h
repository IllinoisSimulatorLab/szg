#ifndef ARGLUCYLINDER_H
#define ARGLUCYLINDER_H

#include "arGluQuadric.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arGluCylinder : public arGluQuadric {
	public:
    arGluCylinder( double startRadius, double endRadius, double length, int slices=30, int stacks=5 );
    arGluCylinder( const arGluCylinder& x );
    arGluCylinder& operator=( const arGluCylinder& x );
    virtual ~arGluCylinder() {}
    virtual void draw();
    void setRadii( double startRadius, double endRadius );
    void setLength( double length );
    void setSlicesStacks( int slices, int stacks );
	protected:
	private:
    double _startRadius;
    double _endRadius;
    double _length;
    int _slices;
    int _stacks;
};

#endif        //  #ifndefARGLUCYLINDER_H

