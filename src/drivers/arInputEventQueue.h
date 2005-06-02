#ifndef ARINPUTEVENTQUEUE_H
#define ARINPUTEVENTQUEUE_H

#include "arInputEvent.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"
#include <deque>

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
    void appendEvent( const arInputEvent& event );
    void appendQueue( const arInputEventQueue& queue );
    bool empty() const { return _queue.empty(); }
    bool size() const { return _queue.size(); }
    arInputEvent popNextEvent();
    
    unsigned int getNumberButtons() const { return _numButtons; }
    unsigned int getNumberAxes() const { return _numAxes; }
    unsigned int getNumberMatrices() const { return _numMatrices; }

    void setSignature( unsigned int numButtons,
                       unsigned int numAxes,
                       unsigned int numMatrices );
    unsigned int getButtonSignature() const { return _buttonSignature; }
    unsigned int getAxisSignature() const { return _axisSignature; }
    unsigned int getMatrixSignature() const { return _matrixSignature; }
                         
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
    std::deque<arInputEvent> _queue;
    unsigned int _numButtons;
    unsigned int _numAxes;
    unsigned int _numMatrices;
    
    // These are meant to be set from an arStructuredData
    // cf ar_setInputQueueFromStructuredData() in arEventUtilities
    // will be increased if a higher event index comes along
    // then will be used to set outgoing arStructuredData signature
    // in ar_saveInputQueueToStructuredData().
    // The arInputEventQueue will never reject events because of the
    // signature.
    unsigned int _buttonSignature;
    unsigned int _axisSignature;
    unsigned int _matrixSignature;
};    

#endif        //  #ifndefARINPUTEVENTQUEUE_H
