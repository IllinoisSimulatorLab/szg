#ifndef ARINTERACTIONSELECTOR_H
#define ARINTERACTIONSELECTOR_H

#include "arMath.h"

class arEffector;

class SZG_CALL arInteractionSelector {
  public:
    virtual ~arInteractionSelector() {}
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const = 0;
    virtual arInteractionSelector* copy() const = 0;
};

class arDistanceInteractionSelector: public arInteractionSelector {
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

class arAlwaysInteractionSelector: public arInteractionSelector {
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

class arAngleInteractionSelector: public arInteractionSelector {
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

