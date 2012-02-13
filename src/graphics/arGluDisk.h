#ifndef ARGLUDISK_H
#define ARGLUDISK_H

#include "arGluQuadric.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arGluDisk : public arGluQuadric {
	public:
    arGluDisk( double innerRadius, double outerRadius, int slices=30, int rings=5 );
    arGluDisk( const arGluDisk& x );
    arGluDisk& operator=( const arGluDisk& x );
    virtual ~arGluDisk() {}
    virtual void draw();
    void setRadii( double innerRadius, double outerRadius );
    void setSlicesRings( int slices, int rings );
	protected:
	private:
    double _innerRadius;
    double _outerRadius;
    int _slices;
    int _rings;
};

#endif        //  #ifndefARGLUDISK_H


