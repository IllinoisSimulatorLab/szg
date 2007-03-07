//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_STATE_H
#define AR_INPUT_STATE_H

#include "arMath.h"
#include "arThread.h"
#include "arInputEvent.h"
#include <vector>
#include <ostream>
#include "arSTLalgo.h"
#include "arDriversCalling.h"

template <class eventDataType> class arInputDeviceMap {
  public:
    friend class arInputState;
    arInputDeviceMap() {
      _deviceNumEvents.reserve(10);
      _deviceEventOffsets.reserve(10);
    }
    ~arInputDeviceMap() {
      _deviceNumEvents.clear();
      _deviceEventOffsets.clear();
    }
    unsigned getNumberDevices() const { return _deviceNumEvents.size(); }
    unsigned getNumberEvents() const;
    unsigned getNumberDeviceEvents( const unsigned deviceNum ) const;
    bool getEventOffset( const unsigned deviceNum, unsigned& offset ) const;
    void addInputDevice( const unsigned numEvents,
                         vector<eventDataType>& dataSpace );
    bool remapInputEvents( const unsigned deviceNum, 
                           const unsigned newNumEvents,
                           vector<eventDataType>& dataSpace );
  private:
    vector<unsigned> _deviceNumEvents;
    vector<unsigned> _deviceEventOffsets;
};

class SZG_CALL arInputState {
  public:
    arInputState();
    arInputState( const arInputState& x );
    arInputState& operator=( const arInputState& x );
    ~arInputState();

    // These aren't const because they use a lock.
    int getButton(       const unsigned buttonNumber );
    float getAxis(       const unsigned axisNumber );
    arMatrix4 getMatrix( const unsigned matrixNumber );
    
    bool getOnButton(  const unsigned buttonNumber );
    bool getOffButton( const unsigned buttonNumber );
  
    // For arInputState, the number of buttons equals the button signature.
    unsigned getNumberButtons()  const { return _buttons.size(); }
    unsigned getNumberAxes()     const { return _axes.size(); }
    unsigned getNumberMatrices() const { return _matrices.size(); }
    
    bool setButton( const unsigned buttonNumber, const int value );
    bool setAxis(   const unsigned axisNumber, const float value );
    bool setMatrix( const unsigned matrixNumber, const arMatrix4& value );
  
    bool update( const arInputEvent& event );
    
    void setSignature( const unsigned maxButtons,
                       const unsigned maxAxes,
                       const unsigned maxMatrices,
                       bool printWarnings=false );
                       
    void addInputDevice( const unsigned numButtons,
                         const unsigned numAxes,
                         const unsigned numMatrices );

    void remapInputDevice( const unsigned deviceNum,
                           const unsigned numButtons,
                           const unsigned numAxes,
                           const unsigned numMatrices );

    bool getButtonOffset( unsigned devNum, unsigned& offset ) const
      { return _buttonInputMap.getEventOffset( devNum, offset ); }
    bool getAxisOffset(   unsigned devNum, unsigned& offset ) const
      { return _axisInputMap.getEventOffset( devNum, offset ); }
    bool getMatrixOffset( unsigned devNum, unsigned& offset ) const
      { return _matrixInputMap.getEventOffset( devNum, offset ); }

    bool setFromBuffers( const int* const buttonBuf,
                         const unsigned numButtons,
                         const float* const axisBuf,
                         const unsigned numAxes,
                         const float* const matrixBuf,
                         const unsigned numMatrices );
    bool saveToBuffers( int* const buttonBuf,
                        float* const axisBuf,
                        float* const matrixBuf );
                        
    void updateLastButtons();
    void updateLastButton( const unsigned index );

  private:    
    void _init();
    void _lock();
    void _unlock();
    void _setSignatureNoLock( const unsigned maxButtons,
                              const unsigned maxAxes,
                              const unsigned maxMatrices,
                              bool printWarnings=false );
    int _getButtonNoLock(       unsigned buttonNumber ) const;
    float _getAxisNoLock(       unsigned axisNumber ) const;
    arMatrix4 _getMatrixNoLock( unsigned matrixNumber ) const;
    bool _getOnButtonNoLock(  unsigned buttonNumber ) const;
    bool _getOffButtonNoLock( unsigned buttonNumber ) const;
    bool _setButtonNoLock( const unsigned buttonNumber, const int value );
    bool _setAxisNoLock(   const unsigned axisNumber, const float value );
    bool _setMatrixNoLock( const unsigned matrixNumber, 
                           const arMatrix4& value );
    // event values
    vector<int> _buttons;
    vector<float> _axes;
    vector<arMatrix4> _matrices;

    // previous button values
    vector<int> _lastButtons;
    
    // info for mapping multiple inputs to a single event index space
    // (for each event type)
    arInputDeviceMap<int> _buttonInputMap;
    arInputDeviceMap<float> _axisInputMap;
    arInputDeviceMap<arMatrix4> _matrixInputMap;

    // pretty much every operation on the input state had better be atomic
    arMutex _accessLock;
};  

ostream& operator<<(ostream& os, const arInputState& inp );

template <class eventDataType>
unsigned arInputDeviceMap<eventDataType>::getNumberEvents() const {
  return accumulate( _deviceNumEvents.begin(), _deviceNumEvents.end(), 0 );
}
  
template <class eventDataType>
unsigned arInputDeviceMap<eventDataType>::getNumberDeviceEvents(
    const unsigned deviceNum) const {
  if (deviceNum >= _deviceEventOffsets.size()) {
    ar_log_warning() << "arInputDeviceMap ignoring out-of-range device " <<
      deviceNum << ".\n";
    return 0;
  }
  return _deviceNumEvents[deviceNum];
}

template <class eventDataType>
bool arInputDeviceMap<eventDataType>::getEventOffset(
                                    const unsigned deviceNum,
                                    unsigned& offset ) const {
  if (deviceNum >= _deviceEventOffsets.size()) {
    ar_log_warning() << "arInputDeviceMap ignoring out-of-range device " <<
      deviceNum << ".\n";
    return false;
  }
  offset = _deviceEventOffsets[deviceNum];
  return true;
}

template <class eventDataType>
void arInputDeviceMap<eventDataType>::addInputDevice(
    const unsigned numEvents, vector<eventDataType>& dataSpace ) {
  if (_deviceNumEvents.empty())
    _deviceEventOffsets.push_back(0);
  else
    _deviceEventOffsets.push_back(_deviceEventOffsets.back() + _deviceNumEvents.back() );
  _deviceNumEvents.push_back( numEvents );
  if (numEvents > 0) {
    // do we need to allocate new memory (there are more than 256 events)?
    if (dataSpace.size()+numEvents > dataSpace.capacity())
      dataSpace.reserve( dataSpace.capacity()+256 );
    dataSpace.insert( dataSpace.begin()+_deviceEventOffsets.back(), numEvents, eventDataType() );
  }
}

template <class eventDataType>
bool arInputDeviceMap<eventDataType>::remapInputEvents(
    const unsigned deviceNum, const unsigned newNumEvents,
    vector<eventDataType>& dataSpace ) { // TD
  if (deviceNum >= _deviceNumEvents.size()) {
    ar_log_warning() << "arInputDeviceMap ignoring out-of-range device " <<
      deviceNum << ".\n";
    return false;
  }
  const int oldNumEvents = _deviceNumEvents[deviceNum];
  const int difference = newNumEvents - oldNumEvents;
  if (difference == 0) {
    return true;
  }
  const unsigned devOffset = _deviceEventOffsets[deviceNum];
  if (difference > 0) {
    if (dataSpace.size()+difference > dataSpace.capacity()) {
      // more than 256 events
      dataSpace.reserve( dataSpace.capacity()+256 );
    }
    dataSpace.insert( dataSpace.begin()+devOffset+oldNumEvents, difference, eventDataType() );
  } else {
    // difference < 0
    dataSpace.erase( dataSpace.begin()+devOffset+newNumEvents,
                     dataSpace.begin()+devOffset+oldNumEvents );
  }   
  for (unsigned i=deviceNum+1; i<_deviceEventOffsets.size(); i++)
    _deviceEventOffsets[i] += difference;
  _deviceNumEvents[deviceNum] = newNumEvents;
  return true;
}

#endif
