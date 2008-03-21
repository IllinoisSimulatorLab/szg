//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_EFFECTOR_H
#define AR_EFFECTOR_H

#include "arInputState.h"
#include "arDragManager.h"
#include "arInteractionSelector.h"
#include "arInteractionCalling.h"

#include <map>

class arInteractable;
class arSZGAppFramework;

class SZG_CALL arEffector {
  public:
    arEffector();
    // NOTE: the "lo" parameters, e.g. "loButton", tell the effector which indices
    // to grab from the input. If numButtons = 3 and loButton = 2, then input button
    // events with indices 2-4 will be captured here. By default they get mapped to
    // a starting index of 0, e.g. in the case just described asking the effector
    // for button 0 will get you what was input button event 2. You can change that
    // with the buttonOffset parameter, e.g. if it is also set to to then you will
    // ask the effector for button 2 to get input event 2.
    arEffector( const unsigned int matrixIndex,
                const unsigned int numButtons,
                const unsigned int loButton,
                const unsigned int buttonOffset,
                const unsigned int numAxes,
                const unsigned int loAxis,
                const unsigned int axisOffset );
    arEffector( const unsigned int matrixIndex,
                const unsigned int numButtons,
                const unsigned int loButton,
                const unsigned int numAxes,
                const unsigned int loAxis );
    virtual ~arEffector();
    arEffector( const arEffector& e );
    arEffector& operator=( const arEffector& e );
    void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    arSZGAppFramework* getFramework() const { return _framework; }
    void setInteractionSelector( const arInteractionSelector& selector );
    float calcDistance( const arMatrix4& mat );
    void setUnitConversion( float conv ) { _unitConversion = conv; }
    void setTipOffset( const arVector3& offset );
    void updateState( const arInputEvent& event );
    void updateState( arInputState* state );
    virtual bool requestGrab( arInteractable* grabee );
    virtual void requestUngrab( arInteractable* grabee );
    virtual void forceUngrab();
    int getButton( unsigned int index );
    float getAxis( unsigned int index );
    arMatrix4 getMatrix() const { return _tipMatrix; }
    arMatrix4 getBaseMatrix() const { return _matrix; }
    arMatrix4 getOffsetMatrix() const { return _offsetMatrix; }
    arMatrix4 getInputMatrix() const { return _inputMatrix; }
    arMatrix4 getCenterMatrix() const;
    bool getOnButton( unsigned int index );
    bool getOffButton( unsigned int index );
    virtual void setMatrix( const arMatrix4& matrix );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    const arDragManager* getDragManager() const;
    virtual const arInteractable* getGrabbedObject();
    virtual void setTouchedObject( arInteractable* touched );
    virtual arInteractable* getTouchedObject();
    void setDrawCallback( void (*drawCallback)( const arEffector* effector ) ) {
      _drawCallback = drawCallback;
    }
    virtual void draw() const {
      if (_drawCallback)
        _drawCallback( (const arEffector*)this );
    }
  protected:
    arInputState _inputState;
    arMatrix4 _inputMatrix;
    arMatrix4 _matrix;
    arMatrix4 _centerMatrix;
    arMatrix4 _offsetMatrix;
    arMatrix4 _tipMatrix;
    unsigned int _matrixIndex;
    unsigned int _numButtons;
    unsigned int _loButton;
    unsigned int _buttonOffset;
    unsigned int _numAxes;
    unsigned int _loAxis;
    unsigned int _axisOffset;
    float _unitConversion;
    arInteractionSelector* _selector;
    arInteractable* _touchedObject;
    arInteractable* _grabbedObject;
    arDragManager _dragManager;
    arSZGAppFramework* _framework;
    void (*_drawCallback)( const arEffector* effector );
};

#endif        //  #ifndefAREFFECTOR_H
