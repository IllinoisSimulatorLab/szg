//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TRACKCAL_FILTER
#define AR_TRACKCAL_FILTER

#include "arIOFilter.h"
#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

/// Calibration table for an input device.

class SZG_CALL arTrackCalFilter: public arIOFilter {
 public:
  arTrackCalFilter();
  ~arTrackCalFilter();

  bool configure(arSZGClient*);
 protected:
  virtual bool _processEvent( arInputEvent& inputEvent );

 private:
  bool _doInterpolation(arMatrix4&);
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
};

#endif
