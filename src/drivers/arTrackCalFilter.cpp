//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arTrackCalFilter.h"
#include "arVRConstants.h"

// The methods used by the dynamic library mappers. 
// NOTE: These MUST have "C" linkage!
extern "C"{
  SZG_CALL void* factory(){
    return new arTrackCalFilter();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arIOFilter", buffer, size);
  }
}

arTrackCalFilter::arTrackCalFilter() :
  _useCalibration(false),
  _yRotAngle(0.),
  _xLookupTable(NULL),
  _yLookupTable(NULL),
  _zLookupTable(NULL)
{
}

arTrackCalFilter::~arTrackCalFilter() {
  _cleanup();
}

bool arTrackCalFilter::configure(arSZGClient* szgClient) {
  float floatBuf[3];
  unsigned long i;
  
  // This part is still a bit hokey.
  // y_rot_angle_deg is just a correction for the sensor on the goggles
  // not pointing straight ahead.  At some point we'll do this more formally.
  // y_filter_weight determines behavior of an IIR filter (y(i) = (1-w)y(i) + wy(i-1))
  // applied to the head vertical position to remove jitter.
  if (szgClient->getAttributeFloats("SZG_MOTIONSTAR","y_rot_angle_deg",floatBuf,1))
    _yRotAngle = ar_convertToRad( *floatBuf );
  if (szgClient->getAttributeFloats("SZG_MOTIONSTAR","IIR_filter_weights",floatBuf,3)) {
    for (i=0; i<3; i++) {
      if ((floatBuf[i] < 0)||(floatBuf[i] >= 1)) {
        cerr << "arTrackCalFilter warning: SZG_MOTIONSTAR/IIR_filter_weight value " << floatBuf[i]
             << " out of range [0,1).\n";
        _filterWeights[i] = 0;
      } else {
        _filterWeights[i] = floatBuf[i];
      }
    }
    cerr << "arTrackCalFilter remark: IIR filter weights are ( "
         << _filterWeights[0] << ", " << _filterWeights[1]
         << ", " << _filterWeights[2] << " ).\n";
  }
    
  const string dataPath(szgClient->getAttribute("SZG_DATA", "path"));
  const string calFileName(szgClient->getAttribute("SZG_MOTIONSTAR", "calib_file"));
  FILE *fp = ar_fileOpen( calFileName, dataPath, "r" );
  if (fp == NULL) {
    cerr << "arTrackCalFilter warning: failed to open file \""
         << calFileName << "\" (SZG_MOTIONSTAR/calib_file) in \"" << dataPath << "\" (SZG_CALIB/path).\n";
    return false;
  }

  cerr << "arTrackCalFilter remark: loading file " << calFileName << endl;
  fscanf(fp, "%ld %f %f %ld %f %f %ld %f %f", &_nx, &_xmin, &_dx, &_ny, &_ymin, &_dy, &_nz, &_zmin, &_dz );
  if ((_nx<1) || (_ny<1) || (_nz<1)) {
    cerr << "arTrackCalFilter error: table dimension < 1.\n";
    fclose(fp);
    return false;
  }
  //  cerr << _nx << ", " << _xmin << ", " << _dx << ", " << _ny << ", " << _ymin << ", " << _dy << ", " << _nz << ", " << _zmin << ", " << _dz << endl;
 _n = _nx*_ny*_nz;
  if ( _n<1 ) {
    cerr << "arTrackCalFilter warning: lookup table size must be >= 1" << endl
         << " (and should be a great deal bigger than that, you silly bugger).\n";
    fclose(fp);
    return false;
  }
  
  // BIG grab
  _xLookupTable = new float[_n];
  _yLookupTable = new float[_n];
  _zLookupTable = new float[_n];
  
  if (!_xLookupTable || !_yLookupTable || !_zLookupTable) {
    cerr << "arTrackCalFilter error: failed to allocate memory for lookup tables.\n";
    fclose(fp);
    return false;
  }

  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _xLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "arTrackCalFilter error: failed to read lookup table value #"
           << i << endl;
      fclose(fp);
      return false;
    }
    //    cerr << _xLookupTable[i] << endl;
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _yLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "arTrackCalFilter error: failed to read lookup table value #"
           << i+_n << endl;
      fclose(fp);
      return false;
    }
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _zLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "arTrackCalFilter error: failed to read lookup table value #"
           << i+2*_n << endl;
      fclose(fp);
      return false;
    }
  }
  fclose(fp);
  cout << "arTrackCalFilter remark: loaded " << 3*_n << " table entries.\n";
  _indexOffsets[0] = 0;
  _indexOffsets[1] = 1;
  _indexOffsets[2] = _nx;
  _indexOffsets[3] = _nx*_ny;
  _indexOffsets[4] = 1+_nx;
  _indexOffsets[5] = 1+_nx*_ny;
  _indexOffsets[6] = _nx+_nx*_ny;
  _indexOffsets[7] = 1+_nx+_nx*_ny;
  cout << "arTrackCalFilter remark: using calibration.\n";
  _useCalibration = true;
  return true;
}

#define HEAD_MATRIX_NUMBER 0

bool arTrackCalFilter::_processEvent( arInputEvent& inputEvent ) {
  if (inputEvent.getType() != AR_EVENT_MATRIX)
    return true;

  arMatrix4 newMatrix(inputEvent.getMatrix());
  if (_useCalibration) {
    _doInterpolation( newMatrix );
    newMatrix = newMatrix * ar_rotationMatrix( 'y', _yRotAngle );
  }
  if (inputEvent.getIndex() == HEAD_MATRIX_NUMBER)  // Apply old filter to head matrix
    _doIIRFilter( newMatrix );
  return inputEvent.setMatrix( newMatrix );
}

  
//arStructuredData* arTrackCalFilter::filter(arStructuredData* d) {
//  int count = ar_getNumberEvents(d);
//#ifdef UNUSED
//  int* indexPtr = (int*) d->getDataPtr("indices",AR_INT);
//#endif
//  for (int i=0; i<count; i++) {
//    if (ar_getEventType(i,d) == AR_EVENT_MATRIX) {
//      arMatrix4 newMatrix = ar_getEventMatrixValue(i,d);
//      if (_useCalibration) {
//        _doInterpolation( newMatrix );
//        newMatrix = newMatrix * ar_rotationMatrix( 'y', _yRotAngle );
//      }
//      if (ar_getEventIndex(i,d) == 0)
//        _doIIRFilter( newMatrix );
//      ar_replaceMatrixEvent(i,newMatrix,d);
//    }
//  }
//  return d;
//}

static float _lastPosition[] = {-1000,-1000,-1000};
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

bool arTrackCalFilter::_doInterpolation( arMatrix4& theMatrix ) {
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
  //cerr << x << ", " << y << ", " << z << endl;
  for (int i=0; i<8; i++) {
    const float w = weights[i];
    const int j = lookupIndex + _indexOffsets[i];
    xCal += w*_xLookupTable[j];
    yCal += w*_yLookupTable[j];
    zCal += w*_zLookupTable[j];
    //cerr << w << ", " << j << endl;
    //cerr << _xLookupTable[j] << ", " << _yLookupTable[j] << ", " << _zLookupTable[j] << endl << endl;
  }
  //  cerr << xCal << ", " << yCal << ", " << zCal << endl << "-------------------------------------------------" << endl << endl;
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
