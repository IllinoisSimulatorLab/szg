//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arTrackCalFilter.h"
#include "arVRConstants.h"

DriverFactory(arTrackCalFilter, "arIOFilter")

arTrackCalFilter::arTrackCalFilter() :
  _useCalibration(false),
  _yRotAngle(0.),
  _xLookupTable(NULL),
  _yLookupTable(NULL),
  _zLookupTable(NULL)
{
  _lastPosition[0] = _lastPosition[1] = _lastPosition[2] = -1000;
}

arTrackCalFilter::~arTrackCalFilter() {
  _cleanup();
}

bool arTrackCalFilter::configure(arSZGClient* szgClient) {
  unsigned long i = 0;
  float floatBuf[3] = {0};

  // This part is still a bit hokey.
  // y_rot_angle_deg is just a correction for the sensor on the goggles
  // not pointing straight ahead.  At some point we'll do this more formally.
  // y_filter_weight determines behavior of an IIR filter (y(i) = (1-w)y(i) + wy(i-1))
  // applied to the head vertical position to remove jitter.
  if (szgClient->getAttributeFloats("SZG_MOTIONSTAR", "IIR_filter_weights", floatBuf, 3)) {
    for (i=0; i<3; i++) {
      if ((floatBuf[i] < 0)||(floatBuf[i] >= 1)) {
        ar_log_error() << "arTrackCalFilter SZG_MOTIONSTAR/IIR_filter_weight value " << floatBuf[i]
             << " out of range [0,1).\n";
        _filterWeights[i] = 0;
      } else {
        _filterWeights[i] = floatBuf[i];
      }
    }
    ar_log_remark() << "arTrackCalFilter: IIR filter weights are ( "
         << _filterWeights[0] << ", " << _filterWeights[1]
         << ", " << _filterWeights[2] << " ).\n";
  }

  // copypasted about 30 lines with drivers/arPForthDatabaseVocabulary.cpp TrackCalAction::configure
  const string dataPath = szgClient->getDataPath();
  const string calFileName(szgClient->getAttribute("SZG_MOTIONSTAR", "calib_file"));
  FILE *fp = ar_fileOpen( calFileName, dataPath, "r" );
  if (fp == NULL) {
    ar_log_error() << "arTrackCalFilter failed to open file '"
         << calFileName << "' (SZG_MOTIONSTAR/calib_file) in '" << dataPath << "' (SZG_CALIB/path).\n";
    return false;
  }

  ar_log_remark() << "arTrackCalFilter loading file " << calFileName << "\n";
  fscanf(fp, "%ld %f %f %ld %f %f %ld %f %f", &_nx, &_xmin, &_dx, &_ny, &_ymin, &_dy, &_nz, &_zmin, &_dz );
  if ((_nx<1) || (_ny<1) || (_nz<1)) {
    ar_log_error() << "arTrackCalFilter table has nonpositive dimension.\n";
    fclose(fp);
    return false;
  }
  //  ar_log_error() << _nx << ", " << _xmin << ", " << _dx << ", " << _ny << ", " << _ymin << ", " << _dy << ", " << _nz << ", " << _zmin << ", " << _dz << "\n";
 _n = _nx*_ny*_nz;
  if ( _n<1 ) {
    ar_log_error() << "arTrackCalFilter lookup table size must be much more than 1.\n";
    fclose(fp);
    return false;
  }

  // BIG grab
  _xLookupTable = new float[_n];
  _yLookupTable = new float[_n];
  _zLookupTable = new float[_n];
  if (!_xLookupTable || !_yLookupTable || !_zLookupTable) {
    ar_log_error() << "arTrackCalFilter failed to allocate memory for lookup tables.\n";
    fclose(fp);
    return false;
  }

  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _xLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      ar_log_error() << "arTrackCalFilter failed to read lookup table value #" << i << "\n";
      fclose(fp);
      return false;
    }
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _yLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      ar_log_error() << "arTrackCalFilter failed to read lookup table value #" << i+_n << "\n";
      fclose(fp);
      return false;
    }
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _zLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      ar_log_error() << "arTrackCalFilter failed to read lookup table value #" << i+2*_n << "\n";
      fclose(fp);
      return false;
    }
  }
  fclose(fp);
  _indexOffsets[0] = 0;
  _indexOffsets[1] = 1;
  _indexOffsets[2] = _nx;
  _indexOffsets[3] = _nx*_ny;
  _indexOffsets[4] = 1+_nx;
  _indexOffsets[5] = 1+_nx*_ny;
  _indexOffsets[6] = _nx+_nx*_ny;
  _indexOffsets[7] = 1+_nx+_nx*_ny;
  ar_log_remark() << "arTrackCalFilter using calibration.\n";
  _useCalibration = true;
  return true;
}

bool arTrackCalFilter::_processEvent( arInputEvent& inputEvent ) {
  if (inputEvent.getType() != AR_EVENT_MATRIX)
    return true;

  arMatrix4 newMatrix(inputEvent.getMatrix());
  if (_useCalibration) {
    _interpolate( newMatrix );
  }
  const unsigned HEAD_MATRIX_NUMBER = 0;
  if (inputEvent.getIndex() == HEAD_MATRIX_NUMBER)  // Apply old filter to head matrix
    _doIIRFilter( newMatrix );
  return inputEvent.setMatrix( newMatrix );
}

void arTrackCalFilter::_doIIRFilter( arMatrix4& m ) {
  for (int i=0; i<3; i++) {
    float p = m[12+i];
    const float lp = _lastPosition[i];
    if (lp != -1000) {
      p = (1-_filterWeights[i])*p + _filterWeights[i]*lp;
      m[12+i] = p;
    }
    _lastPosition[i] = p;
  }
}

bool arTrackCalFilter::_interpolate( arMatrix4& theMatrix ) {
  // Trilinear interpolation.
  const float x = theMatrix[12];
  const float y = theMatrix[13];
  const float z = theMatrix[14];
  const float xBase = (x-_xmin)/_dx;
  const float yBase = (y-_ymin)/_dy;
  const float zBase = (z-_zmin)/_dz;
  const int xIndex = int(floor(xBase));
  const int yIndex = int(floor(yBase));
  const int zIndex = int(floor(zBase));
  const float xFrac = xBase - xIndex;
  const float yFrac = yBase - yIndex;
  const float zFrac = zBase - zIndex;
  const long lookupIndex = xIndex + _nx*yIndex + _nx*_ny*zIndex;
  if (lookupIndex < 0 || lookupIndex+1+_nx+_nx*_ny >= long(_n)) {
    return false;
  }
  const float weights[8] = {
    (1-xFrac)*(1-yFrac)*(1-zFrac),
    (xFrac)*(1-yFrac)*(1-zFrac),
    (1-xFrac)*(yFrac)*(1-zFrac),
    (1-xFrac)*(1-yFrac)*(zFrac),
    (xFrac)*(yFrac)*(1-zFrac),
    (xFrac)*(1-yFrac)*(zFrac),
    (1-xFrac)*(yFrac)*(zFrac),
    (xFrac)*(yFrac)*(zFrac) };
  float xCal = 0.;
  float yCal = 0.;
  float zCal = 0.;
  for (int i=0; i<8; i++) {
    const float w = weights[i];
    const int j = lookupIndex + _indexOffsets[i];
    xCal += w*_xLookupTable[j];
    yCal += w*_yLookupTable[j];
    zCal += w*_zLookupTable[j];
  }
  theMatrix[12] = xCal;
  theMatrix[13] = yCal;
  theMatrix[14] = zCal;
  return true;
}

void arTrackCalFilter::_cleanup() {
  if (_xLookupTable)
    delete[] _xLookupTable;
  if (_yLookupTable)
    delete[] _yLookupTable;
  if (_zLookupTable)
    delete[] _zLookupTable;
}
