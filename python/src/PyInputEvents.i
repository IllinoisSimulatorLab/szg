//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

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
    bool setMatrix( const float* v );
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


