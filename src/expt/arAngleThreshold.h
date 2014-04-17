#ifndef ARANGLETHRESHOLD_H
#define ARANGLETHRESHOLD_H

#include "arMath.h"
#include <vector>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arAngleThreshold {
  public:
    arAngleThreshold( const arVector3& view,
                const float h, const float v=-1. ) :
      _viewPosition( view ),
      _horizThreshold( h ),
      _vertThreshold( v ),
      _referencePosition(0,0,0),
      _refDistance(view.magnitude()),
      _debug( false ) {
        if (v <= 0.)
          _vertThreshold = _horizThreshold;
      }
    void setThresholds( const float h, const float v=-1. ) {
      _horizThreshold = h;
      _vertThreshold = v;
      if (v <=0.) {
        _vertThreshold = h;
      }
    }
    void setViewPosition( const arVector3& view ) {
      arVector3 newRefPos = _referencePosition+_viewPosition;
      _viewPosition = view;
      setReferencePosition( newRefPos );
    }
    void setReferencePosition( const arVector3& ref ) {
      _referencePosition = ref-_viewPosition;
      _refDistance = _referencePosition.magnitude();
    }
    float scaledDistance( const arVector3 pos ) const;
    bool operator()( const arVector3 pos ) {
      bool found = scaledDistance( pos ) <= 1.;
      return found;
    }
    bool anyTooClose( std::vector<arVector3>& positions );
    bool anyTooClose( const arVector3& ref, std::vector<arVector3>& positions );

  private:
    arVector3 _viewPosition;
    float _horizThreshold;
    float _vertThreshold;
    arVector3 _referencePosition;
    float _refDistance;
    bool _debug;
};

#endif        //  #ifndefARANGLETHRESHOLD_H

