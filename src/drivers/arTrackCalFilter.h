//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TRACKCAL_FILTER
#define AR_TRACKCAL_FILTER

#include "arIOFilter.h"
#include "arMath.h"
#include "arDriversCalling.h"

// Calibration table for an input device.

class SZG_CALL arTrackCalFilter: public arIOFilter {
 public:
  arTrackCalFilter();
  ~arTrackCalFilter();

  bool configure(arSZGClient*);
 protected:
  virtual bool _processEvent( arInputEvent& inputEvent );

 private:
  bool _interpolate(arMatrix4&);
  void _doIIRFilter( arMatrix4& m );
  void _cleanup();
  bool _useCalibration;
  float _yRotAngle;
  float _filterWeights[3];
  long _nx, _ny, _nz;
  unsigned long _n;
  float _xmin, _ymin, _zmin, _dx, _dy, _dz;
  float* _xLookupTable;
  float* _yLookupTable;
  float* _zLookupTable;
  int _indexOffsets[8];
  float _lastPosition[3];
};

#endif
