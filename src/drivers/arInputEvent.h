#ifndef ARINPUTEVENT_H
#define ARINPUTEVENT_H

#include "arMath.h"

enum arInputEventType {AR_EVENT_GARBAGE=-1, AR_EVENT_BUTTON=0, 
      AR_EVENT_AXIS=1, AR_EVENT_MATRIX=2};

class arInputEvent {
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

class arButtonEvent : public arInputEvent {
  public:
    arButtonEvent( const unsigned int index, const int b ) :
      arInputEvent( AR_EVENT_BUTTON, index, b ) {
      }
};

class arAxisEvent : public arInputEvent {
  public:
    arAxisEvent( const unsigned int index, const float a ) :
      arInputEvent( AR_EVENT_AXIS, index, a ) {
      }
};

class arMatrixEvent : public arInputEvent {
  public:
    arMatrixEvent( const unsigned int index, const float* v ) :
      arInputEvent( AR_EVENT_MATRIX, index, v ) {
      }
    arMatrixEvent( const unsigned int index,  const arMatrix4& m ) :
      arInputEvent( AR_EVENT_MATRIX, index, m.v ) {
      }
};

class arGarbageEvent : public arInputEvent {
  public:
    arGarbageEvent() : arInputEvent() {}
};

ostream& operator<<(ostream&, const arInputEvent&);

#endif        //  #ifndefARINPUTEVENT_H

