//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_IO_FILTER_H
#define AR_IO_FILTER_H

#include "arSZGClient.h"
#include "arInputEventQueue.h"
#include "arInputState.h"
#include "arDriversCalling.h"

// Abstract base class for filtering messages.

class SZG_CALL arIOFilter {
  public:
    arIOFilter();
    virtual ~arIOFilter() {}

    virtual bool configure(arSZGClient*);
    bool filter( arInputEventQueue* qin, arInputState* s );
    int getButton( const unsigned int index ) const;
    bool getOnButton(  const unsigned int buttonNumber );
    bool getOffButton( const unsigned int buttonNumber );
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index ) const;
    arInputState* getInputState() const { return _inputState; }
    void insertNewEvent( const arInputEvent& newEvent );
    void setID( int id ) { _id = id; }
    int getID() const { return _id; }

    virtual void onButtonEvent( arInputEvent&, unsigned /*index*/ ) {}
    virtual void onAxisEvent( arInputEvent&, unsigned /*index*/ ) {}
    virtual void onMatrixEvent( arInputEvent&, unsigned /*index*/ ) {}

  protected:
    virtual bool _processEvent( arInputEvent& );

  private:
    int _id;
    arInputEventQueue _outputQueue;
    arInputEventQueue _tempQueue;
    arInputState* _inputState;
    bool _valid() const;
};

#endif
