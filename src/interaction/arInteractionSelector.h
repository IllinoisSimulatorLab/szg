//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INTERACTION_SELECTOR_H
#define AR_INTERACTION_SELECTOR_H

#include "arMath.h"
#include "arInteractionCalling.h"

class arEffector;

class SZG_CALL arInteractionSelector {
  public:
    virtual ~arInteractionSelector() {}
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const = 0;
    virtual arInteractionSelector* copy() const = 0;
};

class SZG_CALL arDistanceInteractionSelector: public arInteractionSelector {
  public:
    arDistanceInteractionSelector( float maxDistance = -1. );
    virtual ~arDistanceInteractionSelector() {}
    void setMaxDistance( float maxDistance ) { _maxDistance = maxDistance; }
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
    virtual arInteractionSelector* copy() const;
  private:
    float _maxDistance;
};

class SZG_CALL arAlwaysInteractionSelector: public arInteractionSelector {
  public:
    arAlwaysInteractionSelector() {}
    virtual ~arAlwaysInteractionSelector() {}
    virtual float calcDistance( const arEffector& /*effector*/,
                                const arMatrix4& /*objectMatrix*/ ) const {
      return 0.;
    }
    virtual arInteractionSelector* copy() const {
      return (arInteractionSelector*)new arAlwaysInteractionSelector();
    }
};

class SZG_CALL arAngleInteractionSelector: public arInteractionSelector {
  public:
    arAngleInteractionSelector( float maxAngle = ar_convertToRad(10) );
    virtual ~arAngleInteractionSelector() {}
    void setMaxAngle( float maxAngle ) { _maxAngleDist = 1.-(float)cos(maxAngle); }
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
    virtual arInteractionSelector* copy() const;
  private:
    float _maxAngleDist;
};

#endif        //  #ifndefARINTERACTIONSELECTOR_H
