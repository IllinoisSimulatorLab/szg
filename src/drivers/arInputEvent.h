//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_EVENT_H
#define AR_INPUT_EVENT_H

#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

SZG_CALL enum arInputEventType {AR_EVENT_GARBAGE=-1, AR_EVENT_BUTTON=0, 
                                AR_EVENT_AXIS=1, AR_EVENT_MATRIX=2};

class SZG_CALL arInputEvent {
  public:
    arInputEvent();
    arInputEvent( const arInputEventType type, const unsigned int index );
    virtual ~arInputEvent();
    arInputEvent( const arInputEvent& e );
    arInputEvent& operator=( const arInputEvent& e );
    operator bool() { return (_type != AR_EVENT_GARBAGE); }
    
    arInputEventType getType() const { return _type; }
    unsigned int getIndex() const { return _index; }
    int getButton() const;
    float getAxis() const;
    arMatrix4 getMatrix() const;
    
    void setIndex( const unsigned int i ) { _index = i; }
    bool setButton( const unsigned int b );
    bool setAxis( const float a );
    bool setMatrix( const float* v );
    bool setMatrix( const arMatrix4& m );
    void trash();
    void zero();
    
  protected:
    arInputEvent( const arInputEventType type,
                  const unsigned int index,
                  const int value );
    arInputEvent( const arInputEventType type,
                  const unsigned int index,
                  const float value );
    arInputEvent( const arInputEventType type,
                  const unsigned int index,
                  const float* v );
    arInputEventType _type;
    unsigned int _index;
    int _button;
    float _axis;
    arMatrix4* _matrix;
};

class SZG_CALL arButtonEvent : public arInputEvent {
  public:
    arButtonEvent( const unsigned int index, const int b ) :
      arInputEvent( AR_EVENT_BUTTON, index, b ) {
      }
};

class SZG_CALL arAxisEvent : public arInputEvent {
  public:
    arAxisEvent( const unsigned int index, const float a ) :
      arInputEvent( AR_EVENT_AXIS, index, a ) {
      }
};

class SZG_CALL arMatrixEvent : public arInputEvent {
  public:
    arMatrixEvent( const unsigned int index, const float* v ) :
      arInputEvent( AR_EVENT_MATRIX, index, v ) {
      }
    arMatrixEvent( const unsigned int index,  const arMatrix4& m ) :
      arInputEvent( AR_EVENT_MATRIX, index, m.v ) {
      }
};

class SZG_CALL arGarbageEvent : public arInputEvent {
  public:
    arGarbageEvent() : arInputEvent() {}
};

SZG_CALL ostream& operator<<(ostream&, const arInputEvent&);

#endif        //  #ifndef AR_INPUT_EVENT_H

