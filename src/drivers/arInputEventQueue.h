#ifndef ARINPUTEVENTQUEUE_H
#define ARINPUTEVENTQUEUE_H

#include "arInputEvent.h"
#include "arDriversCalling.h"
#include <deque>

// A stream of events.  Useful for time-domain filtering.
// The dual view is a snapshot of input-device values, class arInputState.

class SZG_CALL arInputEventQueue {
  public:
    arInputEventQueue() :
      _numButtons(0),
      _numAxes(0),
      _numMatrices(0),
      _buttonSignature(0),
      _axisSignature(0),
      _matrixSignature(0) {
      }
    ~arInputEventQueue();
    arInputEventQueue( const arInputEventQueue& q );
    arInputEventQueue& operator=( const arInputEventQueue& q );
    void appendEvent( const arInputEvent& event );
    void appendQueue( const arInputEventQueue& queue );
    bool empty() const { return _queue.empty(); }
    unsigned size() const { return _queue.size(); }
    arInputEvent popNextEvent();

    unsigned getNumberButtons()  const { return _numButtons; }
    unsigned getNumberAxes()     const { return _numAxes; }
    unsigned getNumberMatrices() const { return _numMatrices; }

    void setSignature( unsigned numButtons,
                       unsigned numAxes,
                       unsigned numMatrices );
    unsigned getButtonSignature() const { return _buttonSignature; }
    unsigned getAxisSignature() const { return _axisSignature; }
    unsigned getMatrixSignature() const { return _matrixSignature; }

    bool setFromBuffers( const int* const typeData,
                         const int* const indexData,
                         const int* const buttonData,
                         const unsigned int numButtons,
                         const float* const axisData,
                         const unsigned int numAxes,
                         const float* const matrixData,
                         const unsigned int numMatrices );
    bool saveToBuffers( int* const typeBuf,
                        int* const indexBuf,
                        int* const buttonBuf,
                        float* const axisBuf,
                        float* const matrixBuf ) const;

    void clear();

  private:
    std::deque<arInputEvent> _queue; // Container of events.

    // Number of each "type" of event in _queue.  They sum to size().
    unsigned _numButtons;
    unsigned _numAxes;
    unsigned _numMatrices;

    // These are meant to be set from an arStructuredData,
    // see ar_setInputQueueFromStructuredData() in arEventUtilities.
    // Will be increased if a higher event index comes along.
    // Then will be used to set outgoing arStructuredData signature
    // in ar_saveInputQueueToStructuredData().

    // Signature of a device which could have generated these events.
    unsigned _buttonSignature;
    unsigned _axisSignature;
    unsigned _matrixSignature;

    // arInputEventQueue doesn't reject events "larger than" the signature.
    // Instead, it grows the signature to accept the events.
};

#endif
