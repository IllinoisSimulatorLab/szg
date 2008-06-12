//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CALLBACK_INTERACTABLE_H
#define AR_CALLBACK_INTERACTABLE_H

#include "arInteractable.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arInteractionCalling.h"

class SZG_CALL arCallbackInteractable : public arInteractable {
  public:
    arCallbackInteractable(int graphicsTransformID = -1, int soundTransformID=-1);
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
    void setGraphicsTransformID( int ID ) { _graphicsTransformID = ID; }
    int getGraphicsTransformID() const { return _graphicsTransformID; }
    void setSoundTransformID( int ID ) { _soundTransformID = ID; }
    int getSoundTransformID() const { return _soundTransformID; }

    // For backwards compatibility...
    void setID( int ID ) { _graphicsTransformID = ID; }
    int getID() const { return _graphicsTransformID; }

    virtual void setMatrix( const arMatrix4& matrix );
  protected:
    virtual bool _processInteraction( arEffector& effector );
    virtual bool _touch( arEffector& effector );
    virtual bool _untouch( arEffector& effector );
    int _graphicsTransformID;
    int _soundTransformID;
    bool (*_touchCallback)( arCallbackInteractable* object, arEffector* effector );
    bool (*_processCallback)( arCallbackInteractable* object, arEffector* effector );
    bool (*_untouchCallback)( arCallbackInteractable* object, arEffector* effector );
    void (*_matrixCallback)( arCallbackInteractable* object, const arMatrix4& matrix );
};

#endif        //  #ifndefARCALLBACKINTERACTABLE_H
