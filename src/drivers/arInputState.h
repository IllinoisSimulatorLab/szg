//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_STATE_H
#define AR_INPUT_STATE_H

#include "arMath.h"
#include "arInputEvent.h"
#include "arDriversCalling.h"
#include <vector>
#include <ostream>
#include "arSTLalgo.h"

using namespace std;

template <class eventDataType> class arInputDeviceMap {
  public:
    friend class arInputState;
    arInputDeviceMap() {
      _deviceNumEvents.reserve(10);
      _deviceEventOffsets.reserve(10);
    }
    arInputDeviceMap( const arInputDeviceMap& rhs ) :
      _deviceNumEvents( rhs._deviceNumEvents ),
      _deviceEventOffsets( rhs._deviceEventOffsets ) {
    }
    ~arInputDeviceMap() {
      _deviceNumEvents.clear();
      _deviceEventOffsets.clear();
    }
    unsigned getNumberDevices() const { return _deviceNumEvents.size(); }
    unsigned getNumberEvents() const;
    unsigned getNumberDeviceEvents( const unsigned iDevice ) const;
    bool getEventOffset( const unsigned iDevice, unsigned& offset ) const;
    void addInputDevice( const unsigned numEvents,
                         vector<eventDataType>& dataSpace );
    bool remapInputEvents( const unsigned iDevice,
                           const unsigned newNumEvents,
                           vector<eventDataType>& dataSpace );
  private:
    vector<unsigned> _deviceNumEvents;
    vector<unsigned> _deviceEventOffsets;
};

typedef arInputDeviceMap<int> arIntInputDeviceMap;
typedef arInputDeviceMap<float> arFloatInputDeviceMap;
typedef arInputDeviceMap<arMatrix4> arMatrixInputDeviceMap;

// A snapshot of input-device values,
// in particular button values.
// The dual view is a stream of events, class arInputEventQueue.

class SZG_CALL arInputState {
  public:
    arInputState();
    arInputState( const arInputState& x );
    arInputState& operator=( const arInputState& x );
    ~arInputState();

    // The number of buttons equals the button signature.
    unsigned getNumberButtons() const;
    unsigned getNumberAxes() const;
    unsigned getNumberMatrices() const;

    int getButton(       const unsigned ) const;
    float getAxis(       const unsigned ) const;
    arMatrix4 getMatrix( const unsigned ) const;

    bool getOnButton(  const unsigned ) const;
    bool getOffButton( const unsigned ) const;

    bool setButton( const unsigned iButton, const int value );
    bool setAxis(   const unsigned iAxis, const float value );
    bool setMatrix( const unsigned iMatrix, const arMatrix4& value );

    bool update( const arInputEvent& event );

    void setSignature( const unsigned maxButtons,
                       const unsigned maxAxes,
                       const unsigned maxMatrices,
                       bool printWarnings=false );

    void addInputDevice( const unsigned numButtons,
                         const unsigned numAxes,
                         const unsigned numMatrices );

    void remapInputDevice( const unsigned iDevice,
                           const unsigned numButtons,
                           const unsigned numAxes,
                           const unsigned numMatrices );

    bool getButtonOffset( unsigned iDevice, unsigned& offset );
    bool getAxisOffset(   unsigned iDevice, unsigned& offset );
    bool getMatrixOffset( unsigned iDevice, unsigned& offset );

    bool setFromBuffers( const int* const buttonBuf,
                         const unsigned numButtons,
                         const float* const axisBuf,
                         const unsigned numAxes,
                         const float* const matrixBuf,
                         const unsigned numMatrices );
    bool saveToBuffers( int* const buttonBuf,
                        float* const axisBuf,
                        float* const matrixBuf ) const;

    void updateLastButtons();
    void updateLastButton( const unsigned index );

    arIntInputDeviceMap getButtonDeviceMap() const { return _buttonInputMap; }
    arFloatInputDeviceMap getAxisDeviceMap() const { return _axisInputMap; }
    arMatrixInputDeviceMap getMatrixDeviceMap() const { return _matrixInputMap; }

  private:
    void _init();

    // Call  setX and  getX only while unlocked.
    // Call _setX and _getX only while locked (arGuard _l).

    void _setSignature( const unsigned maxButtons,
                        const unsigned maxAxes,
                        const unsigned maxMatrices,
                        const bool printWarnings=false );
    int _getButton( const unsigned ) const;
    float _getAxis( const unsigned ) const;
    arMatrix4 _getMatrix( const unsigned ) const;
    bool _getOnButton( const unsigned ) const;
    bool _getOffButton( const unsigned ) const;
    bool _setButton( const unsigned iButton, const int );
    bool _setAxis( const unsigned iAxis, const float );
    bool _setMatrix( const unsigned iMatrix, const arMatrix4& );

    // event values
    vector<int> _buttons;
    vector<float> _axes;
    vector<arMatrix4> _matrices;
    vector<int> _lastButtons;

    // Map multiple inputs to a single event index space.
    arIntInputDeviceMap _buttonInputMap;
    arFloatInputDeviceMap _axisInputMap;
    arMatrixInputDeviceMap _matrixInputMap;

    mutable arLock _l;
};

ostream& operator<<(ostream& os, const arInputState& inp );

template <class eventDataType>
unsigned arInputDeviceMap<eventDataType>::getNumberEvents() const {
  return accumulate( _deviceNumEvents.begin(), _deviceNumEvents.end(), 0 );
}

template <class eventDataType>
unsigned arInputDeviceMap<eventDataType>::getNumberDeviceEvents(
    const unsigned iDevice) const {
  if (iDevice >= _deviceEventOffsets.size()) {
    ar_log_error() << "arInputDeviceMap ignoring out-of-range device " <<
      iDevice << ".\n";
    return 0;
  }
  return _deviceNumEvents[iDevice];
}

template <class eventDataType>
bool arInputDeviceMap<eventDataType>::getEventOffset(
    const unsigned iDevice, unsigned& offset ) const {
  if (iDevice >= _deviceEventOffsets.size()) {
    ar_log_error() << "arInputDeviceMap ignoring out-of-range device " <<
      iDevice << ".\n";
    return false;
  }
  offset = _deviceEventOffsets[iDevice];
  return true;
}

template <class eventDataType>
void arInputDeviceMap<eventDataType>::addInputDevice(
    const unsigned numEvents, vector<eventDataType>& dataSpace ) {
  _deviceEventOffsets.push_back( _deviceNumEvents.empty() ?
    0 : _deviceEventOffsets.back() + _deviceNumEvents.back() );
  _deviceNumEvents.push_back( numEvents );
  if (numEvents > 0) {
    if (dataSpace.size()+numEvents > dataSpace.capacity()) {
      dataSpace.reserve( dataSpace.capacity()+256 );
    }
    dataSpace.insert( dataSpace.begin()+_deviceEventOffsets.back(), numEvents, eventDataType() );
  }
}

template <class eventDataType>
bool arInputDeviceMap<eventDataType>::remapInputEvents(
    const unsigned iDevice, const unsigned newNumEvents,
    vector<eventDataType>& dataSpace ) { // TD
  if (iDevice >= _deviceNumEvents.size()) {
    ar_log_error() << "arInputDeviceMap ignoring out-of-range device " <<
      iDevice << ".\n";
    return false;
  }
  const int oldNumEvents = _deviceNumEvents[iDevice];
  const int difference = newNumEvents - oldNumEvents;
  if (difference == 0) {
    return true;
  }
  const unsigned devOffset = _deviceEventOffsets[iDevice];
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
  for (unsigned i=iDevice+1; i<_deviceEventOffsets.size(); i++)
    _deviceEventOffsets[i] += difference;
  _deviceNumEvents[iDevice] = newNumEvents;
  return true;
}

#endif
