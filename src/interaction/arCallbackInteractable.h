#ifndef ARCALLBACKINTERACTABLE_H
#define ARCALLBACKINTERACTABLE_H

#include "arInteractable.h"

class arCallbackInteractable : public arInteractable {
  public:
    arCallbackInteractable(int ID = -1);
    arCallbackInteractable( const arCallbackInteractable& bi );
    arCallbackInteractable& operator=( const arCallbackInteractable& bi );
    // Yes, it is necessary to call _cleanup here rather than in the base
    // class.
    virtual ~arCallbackInteractable() { _cleanup(); }
    void setTouchCallback( bool (*callback)( arCallbackInteractable* object, arEffector* effector ) ) {
      _touchCallback = callback;
    }
    void setProcessCallback( bool (*callback)( arCallbackInteractable* object, arEffector* effector ) ) {
      _processCallback = callback;
    }
    void setUntouchCallback( bool (*callback)( arCallbackInteractable* object ) ) {
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
    virtual bool _untouch();
    int _id;
    bool (*_touchCallback)( arCallbackInteractable* object, arEffector* effector );
    bool (*_processCallback)( arCallbackInteractable* object, arEffector* effector );
    bool (*_untouchCallback)( arCallbackInteractable* object );
    void (*_matrixCallback)( arCallbackInteractable* object, const arMatrix4& matrix );
};

#endif        //  #ifndefARCALLBACKINTERACTABLE_H

