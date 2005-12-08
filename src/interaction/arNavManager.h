//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NAV_MANAGER_H
#define AR_NAV_MANAGER_H

#include "arEffector.h"
#include "arInteractable.h"
#include "arGrabCondition.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arInteractionCalling.h"

class SZG_CALL arNavInteractable : public arInteractable {
  friend class arNavManager;
  public:
    arNavInteractable();
    arNavInteractable( const arNavInteractable& ni );
    arNavInteractable& operator=( const arNavInteractable& ni );
    ~arNavInteractable() {_cleanup();}
    virtual void setMatrix( const arMatrix4& matrix );    
};

class SZG_CALL arNavManager {
  public:
    arNavManager();
    arNavManager( const arNavManager& nm );
    arNavManager& operator=( const arNavManager& nm );
    ~arNavManager();
    
    bool setTransCondition( char axis,
                            arInputEventType type,
                            unsigned int index,
                            float threshold );
    bool setRotCondition( char axis,
                          arInputEventType type,
                          unsigned int index,
                          float threshold );
//    bool setWorldRotGrabCondition( arInputEventType type,
//                                   unsigned int index,
//                                   float threshold );
    void setTransSpeed( float speed );
    void setRotSpeed( float speed );
    void setEffector( const arEffector& effector );
    void update( arInputState* inputState );
    void update( arInputEvent& event );
    
  private:
    void _clearCondition( const arGrabCondition& condition );
    arEffector _effector;
    arNavInteractable _navObject;
    arGrabCondition _transConditions[3];
    arGrabCondition _rotConditions[3];
    arGrabCondition _worldGrabCondition;
    float _transSpeeds[3];
    float _rotSpeeds[3];
};

#endif        //  #ifndefARNAVMANAGER_H
