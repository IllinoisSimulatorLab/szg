#ifndef ARGLUTESSOBJECT_H
#define ARGLUTESSOBJECT_H

#include <vector>
#include "arMath.h"
#include "arDrawable.h"

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

//****************************************************
// tesselated object base class
//****************************************************

class SZG_CALL arGluTessObject : public arDrawable {
  public:
    arGluTessObject( bool useDisplayList = false );
    arGluTessObject( const arGluTessObject& x );
    arGluTessObject& operator=( const arGluTessObject& x );
    virtual ~arGluTessObject();
    void setScaleFactors( const arVector3& scales ) {
      _scaleFactors = scales;
    }
    void setScaleFactors( float x, float y, float z ) {
      _scaleFactors = arVector3(x,y,z);
    }
    arVector3 getScaleFactors() {
      return _scaleFactors;
    }
    void setTextureScales( float sScale, float tScale );
    void setTextureOffsets( float sOffset, float tOffset );
    void useDisplayList( bool use );
    bool buildDisplayList();
    virtual void draw();
    void addContour( std::vector< arVector3 >& newContour );
    typedef std::vector< std::vector< arVector3 > > arTessVertices;
  protected:
    bool _drawTess();
    unsigned int _displayListID;
    bool _useDisplayList;
    bool _displayListDirty;
    float _thickness;
    arVector3 _scaleFactors;
    float _textureScaleInfo[4];
    double _windingRule;
    arTessVertices _contours;
  private:
};

#endif
