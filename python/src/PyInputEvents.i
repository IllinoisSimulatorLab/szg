//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

%{
#include "arInputEvent.h"
#include "arInputState.h"
%}

enum arInputEventType {AR_EVENT_GARBAGE=-1, AR_EVENT_BUTTON=0, 
                                AR_EVENT_AXIS=1, AR_EVENT_MATRIX=2};

class arInputEvent {
  public:
    arInputEvent();
    arInputEvent( const arInputEventType type, const unsigned int index );
    virtual ~arInputEvent();
    operator bool();
    
    arInputEventType getType() const;
    unsigned int getIndex() const;
    int getButton() const;
    float getAxis() const;
    arMatrix4 getMatrix() const;
    
    void setIndex( const unsigned int i );
    bool setButton( const unsigned int b );
    bool setAxis( const float a );
    // SWIG cant handle having both of these setMatrix() signatures.
//    bool setMatrix( const float* v );
    bool setMatrix( const arMatrix4& m );
    void trash();
    void zero();

%extend{
    string __str__(void) {
      ostringstream s(ostringstream::out);
      switch (self->getType()) {
        case AR_EVENT_BUTTON:
          s << "BUTTON[" << self->getIndex() << "]: " << self->getButton();
          break;
        case AR_EVENT_AXIS:
          s << "AXIS[" << self->getIndex() << "]: " << self->getAxis();
            break;
        case AR_EVENT_MATRIX:
          s << "MATRIX[" << self->getIndex() << "]:\n" << self->getMatrix();
          break;
        case AR_EVENT_GARBAGE:
          s << "GARBAGE[" << self->getIndex() << "]";
          break;
        default:
          s << "EVENT_ERROR[" << self->getIndex() << "]";
      }
      return s.str();
    }
}

};

class arInputState {
  public:
    arInputState();
    arInputState( const arInputState& x );
    arInputState& operator=( const arInputState& x );
    ~arInputState();

    // the "get" functions cannot be const since they involve 
    // a mutex lock/unlock 
    int getButton(       const unsigned int buttonNumber );
    float getAxis(       const unsigned int axisNumber );
    arMatrix4 getMatrix( const unsigned int matrixNumber );
    
    bool getOnButton(  const unsigned int buttonNumber );
    bool getOffButton( const unsigned int buttonNumber );
  
    /// \todo some classes use getNumberButtons, others getNumButtons (etc).  Be consistent.
    // Note that for the arInputState the number of buttons and the button
    // signature are the same.
    unsigned int getNumberButtons()  const { return _buttons.size(); }
    unsigned int getNumberAxes()     const { return _axes.size(); }
    unsigned int getNumberMatrices() const { return _matrices.size(); }
    
    bool setButton( const unsigned int buttonNumber, const int value );
    bool setAxis(   const unsigned int axisNumber, const float value );
    bool setMatrix( const unsigned int matrixNumber, const arMatrix4& value );
  
    bool update( const arInputEvent& event );
};

