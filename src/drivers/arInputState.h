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
// THIS MUST BE THE LAST SZG INCLUDE!
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
    unsigned int getNumberDevices() const { return _deviceNumEvents.size(); }
    unsigned int getNumberEvents() const;
    unsigned int getNumberDeviceEvents( const unsigned int deviceNumber ) const;
    bool getEventOffset( const unsigned int deviceNumber,
                          unsigned int& offset ) const;
    void addInputDevice( const unsigned int numEvents,
                         std::vector<eventDataType>& dataSpace );
    bool remapInputEvents( const unsigned int deviceNum, 
                           const unsigned int newNumEvents,
                           std::vector<eventDataType>& dataSpace );
  private:
    std::vector<unsigned int> _deviceNumEvents;
    std::vector<unsigned int> _deviceEventOffsets;
};


class SZG_CALL arInputState {
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
    // For arInputState, the number of buttons equals the button signature.
    unsigned int getNumberButtons()  const { return _buttons.size(); }
    unsigned int getNumberAxes()     const { return _axes.size(); }
    unsigned int getNumberMatrices() const { return _matrices.size(); }
    
    bool setButton( const unsigned int buttonNumber, const int value );
    bool setAxis(   const unsigned int axisNumber, const float value );
    bool setMatrix( const unsigned int matrixNumber, const arMatrix4& value );
  
    bool update( const arInputEvent& event );
    
    void setSignature( const unsigned int maxButtons,
                       const unsigned int maxAxes,
                       const unsigned int maxMatrices,
                       bool printWarnings=false );
                       
    void addInputDevice( const unsigned int numButtons,
                         const unsigned int numAxes,
                         const unsigned int numMatrices );

    void remapInputDevice( const unsigned int deviceNum,
                           const unsigned int numButtons,
                           const unsigned int numAxes,
                           const unsigned int numMatrices );

    bool getButtonOffset( unsigned int devNum, unsigned int& offset ) const
      { return _buttonInputMap.getEventOffset( devNum, offset ); }
    bool getAxisOffset(   unsigned int devNum, unsigned int& offset ) const
      { return _axisInputMap.getEventOffset( devNum, offset ); }
    bool getMatrixOffset( unsigned int devNum, unsigned int& offset ) const
      { return _matrixInputMap.getEventOffset( devNum, offset ); }

    bool setFromBuffers( const int* const buttonBuf,
                         const unsigned int numButtons,
                         const float* const axisBuf,
                         const unsigned int numAxes,
                         const float* const matrixBuf,
                         const unsigned int numMatrices );
    bool saveToBuffers( int* const buttonBuf,
                        float* const axisBuf,
                        float* const matrixBuf );
                        
    void updateLastButtons();
    void updateLastButton( const unsigned int index );

  private:    
    void _lock();
    void _unlock();
    void _setSignatureNoLock( const unsigned int maxButtons,
                              const unsigned int maxAxes,
                              const unsigned int maxMatrices,
                              bool printWarnings=false );
    int _getButtonNoLock(       unsigned int buttonNumber ) const;
    float _getAxisNoLock(       unsigned int axisNumber ) const;
    arMatrix4 _getMatrixNoLock( unsigned int matrixNumber ) const;
    bool _getOnButtonNoLock(  unsigned int buttonNumber ) const;
    bool _getOffButtonNoLock( unsigned int buttonNumber ) const;
    bool _setButtonNoLock( const unsigned int buttonNumber, const int value );
    bool _setAxisNoLock(   const unsigned int axisNumber, const float value );
    bool _setMatrixNoLock( const unsigned int matrixNumber, 
                           const arMatrix4& value );
    // event values
    std::vector<int> _buttons;
    std::vector<float> _axes;
    std::vector<arMatrix4> _matrices;

    // previous button values
    std::vector<int> _lastButtons;
    
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
unsigned int arInputDeviceMap<eventDataType>::getNumberEvents() const {
  return accumulate( _deviceNumEvents.begin(), _deviceNumEvents.end(), 0 );
}
  
template <class eventDataType>
unsigned int arInputDeviceMap<eventDataType>::getNumberDeviceEvents(
                                    const unsigned int deviceNumber ) const {
  if (deviceNumber >= _deviceEventOffsets.size()) {
    cerr << "arInputDeviceMap warning: ignoring out-of-range device # "
         << deviceNumber << ".\n";
    return 0;
  }
  return _deviceNumEvents[deviceNumber];
}

template <class eventDataType>
bool arInputDeviceMap<eventDataType>::getEventOffset(
                                    const unsigned int deviceNumber,
                                    unsigned int& offset ) const {
  if (deviceNumber >= _deviceEventOffsets.size()) {
    cerr << "arInputDeviceMap warning: ignoring out-of-range device # "
         << deviceNumber << ".\n";
    return false;
  }
  offset = _deviceEventOffsets[deviceNumber];
  return true;
}

template <class eventDataType>
void arInputDeviceMap<eventDataType>::addInputDevice(
                                        const unsigned int numEvents,
                                        std::vector<eventDataType>& dataSpace ) {
  if (_deviceNumEvents.empty())
    _deviceEventOffsets.push_back(0);
  else
    _deviceEventOffsets.push_back(
        _deviceEventOffsets.back() + _deviceNumEvents.back() );
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
                                      const unsigned int deviceNum, 
                                      const unsigned int newNumEvents,
                                      std::vector<eventDataType>& dataSpace ) { // TD
  if (deviceNum >= _deviceNumEvents.size()) {
    cerr << "arInputDeviceMap error: device # " << deviceNum << " out of bounds.\n";
    return false;
  }
  int oldNumEvents = _deviceNumEvents[deviceNum];
  int difference = newNumEvents - oldNumEvents;
  unsigned int devOffset = _deviceEventOffsets[deviceNum];
//cerr << difference << endl;  
  if (difference == 0) {
    return true;
  } else if (difference > 0) {
    // do we need to allocate new memory (there are more than 256 events)?
    if (dataSpace.size()+difference > dataSpace.capacity())
      dataSpace.reserve( dataSpace.capacity()+256 );
      
    dataSpace.insert( dataSpace.begin()+devOffset+oldNumEvents,
                                       difference, eventDataType() );
  } else { // difference < 0
    
    dataSpace.erase( dataSpace.begin()+devOffset+newNumEvents,
                     dataSpace.begin()+devOffset+oldNumEvents );
  }   
  for (unsigned int i=deviceNum+1; i<_deviceEventOffsets.size(); i++)
    _deviceEventOffsets[i] += difference;
  _deviceNumEvents[deviceNum] = newNumEvents;
//for (unsigned int j=0; j<_deviceEventOffsets.size(); j++)
//  cerr << _deviceNumEvents[j] << " " << _deviceEventOffsets[j] << endl;
  
  return true;
}


#endif        //  #ifndefARINPUTSTATE_H

