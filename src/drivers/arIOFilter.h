//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_IO_FILTER_H
#define AR_IO_FILTER_H

#include "arSZGClient.h"
#include "arInputEventQueue.h"
#include "arInputState.h"

/// Abstract base class for filtering messages.

class SZG_CALL arIOFilter {
  public:
    arIOFilter();
    virtual ~arIOFilter();
  
    virtual bool configure(arSZGClient*);
    bool filter( arInputEventQueue* qin, arInputState* s );
    int getButton( const unsigned int index ) const;
    float getAxis( const unsigned int index ) const;
    arMatrix4 getMatrix( const unsigned int index ) const;
    arInputState* getInputState() const { return _inputState; }
    void insertNewEvent( const arInputEvent& newEvent );
    
  protected:
    virtual bool _processEvent( arInputEvent& /*inputEvent*/ ) { return true; }
    
  private:
    arInputEventQueue _outputQueue;
    arInputEventQueue _tempQueue;
    arInputState* _inputState;
};

#endif
