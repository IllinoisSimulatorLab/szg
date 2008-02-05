//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAB_CONDITION_H
#define AR_GRAB_CONDITION_H

#include "arInputEvent.h"
#include "arInteractionCalling.h"

class arEffector;

class SZG_CALL arGrabCondition {
  public:
    arGrabCondition();
    arGrabCondition( arInputEventType eventType,
                     unsigned int eventIndex,
                     float thresholdValue );
    virtual ~arGrabCondition() {}
    arGrabCondition( const arGrabCondition& g );
    arGrabCondition& operator=( const arGrabCondition& g );
    bool operator==( const arGrabCondition& g ) const;
    virtual bool check( arEffector* effector );
    virtual float type() const { return _type; }
    virtual float index() const { return _index; }
    virtual float threshold() const { return _threshold; }
    virtual float value() const { return _currentValue; }
    virtual arGrabCondition* copy() const;
  protected:
    arInputEventType _type;
    unsigned int _index;
    float _threshold;
    float _currentValue;
};

class SZG_CALL arDeltaGrabCondition : public arGrabCondition {
  public:
    arDeltaGrabCondition();
    arDeltaGrabCondition( unsigned int eventIndex,
                           bool on=true );
    arDeltaGrabCondition( unsigned int eventIndex,
                           bool on, bool current );
    arDeltaGrabCondition( const arDeltaGrabCondition& x );
    arDeltaGrabCondition& operator=( const arDeltaGrabCondition& x );
    virtual ~arDeltaGrabCondition() {}
    virtual bool check( arEffector* effector );
    virtual arGrabCondition* copy() const;
  protected:
    bool _isOnButtonEvent;
    bool _currentState;
  private:
};

class SZG_CALL arAlwaysGrabCondition : public arGrabCondition {
  public:
    arAlwaysGrabCondition() {}
    float type() const { return AR_EVENT_GARBAGE; }
    float index() const { return 0; }
    float threshold() const { return 0.; }
    float value() const { return 0.; }
    virtual bool check( arEffector* /*effector*/ ) { return true; }
  protected:
  private:
};

#endif        //  #ifndefARGRABCONDITION_H
