//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFaroCalFilter.h"
#include "arVRConstants.h"

extern "C"{
  SZG_CALL void* factory()
    { return new arFaroCalFilter(); }
  SZG_CALL void baseType(char* buffer, int size)
    { ar_stringToBuffer("arIOFilter", buffer, size); }
}

arFaroCalFilter::arFaroCalFilter() :
  _useCalibration(false),
  _yRotAngle(0.),
  _yFilterWeight(0.),
  _xLookupTable(NULL),
  _yLookupTable(NULL),
  _zLookupTable(NULL),
  _yOld(-1000)
{
}

arFaroCalFilter::~arFaroCalFilter() {
  _cleanup();
}

enum { 
  HEAD_MATRIX_NUMBER = 0,
  WAND_MATRIX_NUMBER = 1,
  FARO_MATRIX_NUMBER = 3
};

bool arFaroCalFilter::_processEvent( arInputEvent& inputEvent ) {
  if (inputEvent.getType() != AR_EVENT_MATRIX)
    return true;

  arMatrix4 newMatrix = inputEvent.getMatrix();
  const unsigned eventIndex = inputEvent.getIndex();
  if (eventIndex == FARO_MATRIX_NUMBER) { // Apply faro coordinate transformation to faro tip.
    newMatrix = _faroCoordMatrix * newMatrix;
  } else {
    if (_useCalibration)
      _interpolate( newMatrix );
    if (eventIndex == HEAD_MATRIX_NUMBER)  // Apply old filter to head matrix
      _doIIRFilter( newMatrix );
  }
  return inputEvent.setMatrix( newMatrix );
}
    
//arStructuredData* arFaroCalFilter::filter(arStructuredData* d) {
//  int count = ar_getNumberEvents(d);
//  for (int i=0; i<count; i++) {
//    if (ar_getEventType(i,d) == AR_EVENT_MATRIX) {
//      arMatrix4 newMatrix = ar_getEventMatrixValue(i,d);
//      if (ar_getEventIndex(i,d)==FARO_MATRIX_NUMBER) { // Apply faro coordinate transformation to faro tip.
//        newMatrix = _faroCoordMatrix*newMatrix;
//      } else {
//        if (_useCalibration)
//          _interpolate( newMatrix );
//        if (ar_getEventIndex(i,d)==HEAD_MATRIX_NUMBER)  // Apply old filter to head matrix
//          _doIIRFilter( newMatrix );
//      }
//      ar_replaceMatrixEvent(i,newMatrix,d);
//    }
//  }
//  return d;
//}

bool arFaroCalFilter::configure(arSZGClient* szgClient) {
  float floatBuf = 0.;
  string received(szgClient->getAttribute("SZG_FAROCAL", "coord_transform"));
  if (received == "NULL") {
    cerr << "arFaroCalFilter warning: SZG_FAROCAL/coord_transform undefined.\n";
    cerr << "     Defaulting to identity matrix.\n";
  } else {
    float transformData[16];
    char receivedBuffer[256];
    ar_stringToBuffer( received, receivedBuffer, sizeof(receivedBuffer) );
    int numElements = ar_parseFloatString( receivedBuffer, transformData, 16 );
    if (numElements != 16) {
      cerr << "arFaroCalFilter warning: SZG_FAROCAL/coord_transform does not contain 16 elements.\n";
      cerr << "     Defaulting to identity matrix.\n";
    } else {
      _faroCoordMatrix = arMatrix4( transformData );
      cerr << "FaroArm Coordinate Transformation:\n"
           << _faroCoordMatrix << endl;
    }
  }

  // This part is still a bit hokey.
  // y_rot_angle_deg is just a correction for the sensor on the goggles
  // not pointing straight ahead.  At some point we'll do this more formally.
  // y_filter_weight determines behavior of an IIR filter (y(i) = (1-w)y(i) + wy(i-1))
  // applied to the head vertical position to remove jitter.
  if (szgClient->getAttributeFloats("SZG_CALIB","y_rot_angle_deg",&floatBuf,1))
    _yRotAngle = ar_convertToRad( floatBuf );
  if (szgClient->getAttributeFloats("SZG_MOTIONSTAR","y_filter_weight",&floatBuf,1)) {
    if (floatBuf < 0 || floatBuf >= 1) {
      cerr << "arFaroCalFilter warning: SZG_MOTIONSTAR/y_filter_weight value " << floatBuf
           << " out of range [0,1).\n";
    } else {
      _yFilterWeight = floatBuf;
    }
  }
  const string& dataPath = szgClient->getDataPath();
  const string& calFileName = szgClient->getAttribute("SZG_CALIB", "calib_file");
  FILE *fp = ar_fileOpen( calFileName, dataPath, "r" );
  if (fp == NULL) {
    cerr << "arFaroCalFilter warning: failed to open file \""
         << calFileName << "\" (SZG_CALIB/calib_file) in \"" << dataPath << "\" (SZG_CALIB/path).\n";
    return false;
  }

  cerr << "arFaroCalFilter remark: loading file " << calFileName << endl;
  fscanf(fp, "%ld %f %f %ld %f %f %ld %f %f", &_nx, &_xmin, &_dx, &_ny, &_ymin, &_dy, &_nz, &_zmin, &_dz );
  if ((_nx<1) || (_ny<1) || (_nz<1)) {
    cerr << "arFaroCalFilter error: not all table dimensions are positive.\n";
    fclose(fp);
    return false;
  }
  //  cerr << _nx << ", " << _xmin << ", " << _dx << ", " << _ny << ", " << _ymin << ", " << _dy << ", " << _nz << ", " << _zmin << ", " << _dz << endl;
 _n = _nx*_ny*_nz;
  if ( _n<1 ) {
    cerr << "arFaroCalFilter warning: lookup table needs FAR more than one element.\n";
    fclose(fp);
    return false;
  }
  
  // Grab a huge block of memory.
  _xLookupTable = new float[_n];
  _yLookupTable = new float[_n];
  _zLookupTable = new float[_n];
  
  if (!_xLookupTable || !_yLookupTable || !_zLookupTable) {
    cerr << "arFaroCalFilter warning: out of memory for lookup tables.\n";
    fclose(fp);
    return false;
  }

  unsigned long i = 0;
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _xLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "arFaroCalFilter error: failed to read lookup table value #"
           << i << endl;
      fclose(fp);
      return false;
    }
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _yLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "arFaroCalFilter error: failed to read lookup table value #"
           << i+_n << endl;
      fclose(fp);
      return false;
    }
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _zLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "arFaroCalFilter error: failed to read lookup table value #"
           << i+2*_n << endl;
      fclose(fp);
      return false;
    }
  }
  fclose(fp);
  // cerr << "arFaroCalFilter remark: loaded " << 3*_n << "table entries.\n";
  _indexOffsets[0] = 0;
  _indexOffsets[1] = 1;
  _indexOffsets[2] = _nx;
  _indexOffsets[3] = _nx*_ny;
  _indexOffsets[4] = 1+_nx;
  _indexOffsets[5] = 1+_nx*_ny;
  _indexOffsets[6] = _nx+_nx*_ny;
  _indexOffsets[7] = 1+_nx+_nx*_ny;
  cout << "arFaroCalFilter remark: using calibration.\n";
  _useCalibration = true;
  return true;
}

void arFaroCalFilter::_doIIRFilter( arMatrix4& m ) {
  float y = m[13];
  if (_yOld != -1000)
    y = (1-_yFilterWeight)*y + _yFilterWeight*_yOld;
  m[13] = y;
  _yOld = y;
}

bool arFaroCalFilter::_interpolate( arMatrix4& theMatrix ) {
  const float x = theMatrix[12];
  const float y = theMatrix[13];
  const float z = theMatrix[14];
  const float xBase = (x-_xmin)/_dx;
  const float yBase = (y-_ymin)/_dy;
  const float zBase = (z-_zmin)/_dz;
  const int xIndex = int(floor(xBase));
  const int yIndex = int(floor(yBase));
  const int zIndex = int(floor(zBase));
  const long lookupIndex = xIndex + _nx*yIndex + _nx*_ny*zIndex;
  if (lookupIndex < 0 || lookupIndex+1+_nx+_nx*_ny >= long(_n))
    return false;

  const float xFrac = xBase - xIndex;
  const float yFrac = yBase - yIndex;
  const float zFrac = zBase - zIndex;
  const float weights[8] = {
    (1-xFrac)*(1-yFrac)*(1-zFrac),
    (xFrac)*(1-yFrac)*(1-zFrac),
    (1-xFrac)*(yFrac)*(1-zFrac),
    (1-xFrac)*(1-yFrac)*(zFrac),
    (xFrac)*(yFrac)*(1-zFrac),
    (xFrac)*(1-yFrac)*(zFrac),
    (1-xFrac)*(yFrac)*(zFrac),
    (xFrac)*(yFrac)*(zFrac)
    };
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

void arFaroCalFilter::_cleanup() {
  if (_xLookupTable)
    delete [] _xLookupTable;
  if (_yLookupTable)
    delete [] _yLookupTable;
  if (_zLookupTable)
    delete [] _zLookupTable;
}
