#ifndef ARGLUQUADRIC_H
#define ARGLUQUADRIC_H

#include "arGraphicsHeader.h"
#include "arDrawable.h"

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arGluQuadric : public arDrawable {
  public:
    arGluQuadric();
    arGluQuadric( const arGluQuadric& x );
    arGluQuadric& operator=( const arGluQuadric& x );
    virtual ~arGluQuadric();
    void setPointStyle() { _drawStyle = GLU_POINT; }
    void setLineStyle() { _drawStyle = GLU_LINE; }
    void setSilhouetteStyle() { _drawStyle = GLU_SILHOUETTE; }
    void setFillStyle() { _drawStyle = GLU_FILL; }
    void setNormalsOutside( bool trueFalse ) {
      _normalDirection = (trueFalse)?(GLU_OUTSIDE):(GLU_INSIDE);
    }
    void setNoNormals() { _normalStyle = GLU_NONE; }
    void setFlatNormals() { _normalStyle = GLU_FLAT; }
    void setSmoothNormals() { _normalStyle = GLU_SMOOTH; }
    virtual void draw() {}
  protected:
    bool _prepareQuadric();
    static GLUquadricObj* _quadric;
	private:
    GLenum _drawStyle;
    GLenum _normalDirection;
    GLenum _normalStyle;
    static unsigned int _refCount;
};

#endif        //  #ifndefARGLUQUADRIC_H

