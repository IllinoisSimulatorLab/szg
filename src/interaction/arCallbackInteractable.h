//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CALLBACK_INTERACTABLE_H
#define AR_CALLBACK_INTERACTABLE_H

#include "arInteractable.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arInteractionCalling.h"

class SZG_CALL arCallbackInteractable : public arInteractable {
  public:
    arCallbackInteractable(int ID = -1);
    arCallbackInteractable( const arCallbackInteractable& bi );
    arCallbackInteractable& operator=( const arCallbackInteractable& bi );
    // Yes, it is necessary to call _cleanup here rather than in the base
    // class.
//    virtual ~arCallbackInteractable() { _cleanup(); }
//  No, it probably isn't; I just noticed a boneheaded mistake in _untouch()
//  (wrong signature)
    virtual ~arCallbackInteractable() {}
    void setTouchCallback( bool (*callback)( arCallbackInteractable* object, arEffector* effector ) ) {
      _touchCallback = callback;
    }
    void setProcessCallback( bool (*callback)( arCallbackInteractable* object, arEffector* effector ) ) {
      _processCallback = callback;
    }
    void setUntouchCallback( bool (*callback)( arCallbackInteractable* object, arEffector* effector ) ) {
      _untouchCallback = callback;
    }
    void setMatrixCallback( void (*callback)( arCallbackInteractable* object, const arMatrix4& matrix ) ) {
      _matrixCallback = callback;
    }
    void setID( int ID ) { _id = ID; }
    int getID() const { return _id; }
    virtual void setMatrix( const arMatrix4& matrix );
  protected:
    virtual bool _processInteraction( arEffector& effector );
    virtual bool _touch( arEffector& effector );
    virtual bool _untouch( arEffector& effector );
    int _id;
    bool (*_touchCallback)( arCallbackInteractable* object, arEffector* effector );
    bool (*_processCallback)( arCallbackInteractable* object, arEffector* effector );
    bool (*_untouchCallback)( arCallbackInteractable* object, arEffector* effector );
    void (*_matrixCallback)( arCallbackInteractable* object, const arMatrix4& matrix );
};

#endif        //  #ifndefARCALLBACKINTERACTABLE_H

