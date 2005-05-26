//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arPForth.h"
#include "arPForthDatabaseVocabulary.h"

namespace arPForthSpace {

// variables

arSZGClient* __PForthSZGClient = NULL;

// Run-time behaviors

class GetDatabaseParameter : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetDatabaseParameter::run( arPForth* pf ) {
  if (!pf)
    return false;
  if (arPForthSpace::__PForthSZGClient == 0)
    throw arPForthException("NULL arSZGClient pointer.");
  long paramValAddress    = (long)pf->stackPop();
  long paramNameAddress = (long)pf->stackPop();
  long groupNameAddress = (long)pf->stackPop();
  std::string groupName = pf->getString( groupNameAddress );
  std::string paramName = pf->getString( paramNameAddress );
  std::string paramVal = arPForthSpace::__PForthSZGClient->getAttribute( groupName, paramName );
  cerr << groupName << ", " << paramName << ", " << paramVal << endl;
  pf->putString( paramValAddress, paramVal );
  return true;
}

class GetFloatDatabaseParameters : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetFloatDatabaseParameters::run( arPForth* pf ) {
  if (!pf)
    return false;
  if (arPForthSpace::__PForthSZGClient == 0)
    throw arPForthException("NULL arSZGClient pointer.");
  long paramValAddress    = (long)pf->stackPop();
  unsigned long numParams = (unsigned long)pf->stackPop();
  long paramNameAddress = (long)pf->stackPop();
  long groupNameAddress = (long)pf->stackPop();
  float *ptr = new float[numParams];
  if (!ptr)
    throw arPForthException("Failed to allocate float pointer.");
  std::string groupName = pf->getString( groupNameAddress );
  std::string paramName = pf->getString( paramNameAddress );
  bool stat = arPForthSpace::__PForthSZGClient->getAttributeFloats( groupName, paramName, ptr, (int)numParams );
  if (!stat)
    throw arPForthException("getAttributeFloats() failed.");
  pf->putDataArray( paramValAddress, ptr, numParams );
  delete[] ptr;
  return true;
}

class TrackCalAction : public arPForthAction {
  public:
    ~TrackCalAction() { _cleanup(); }
    bool configure(arSZGClient*);
    bool run( arPForth* pf );

  private:
    bool _doInterpolation( arMatrix4& newMatrix );
    void _cleanup();
    long _nx, _ny, _nz;
    unsigned long _n;
    float _xmin, _ymin, _zmin, _dx, _dy, _dz;
    float* _xLookupTable;
    float* _yLookupTable;
    float* _zLookupTable;
    int _indexOffsets[8];
};
bool TrackCalAction::run( arPForth* pf ) {
  if (!pf)
    return false;
  long outMatrixAddress = (long)pf->stackPop();
  long inMatrixAddress    = (long)pf->stackPop();
  arMatrix4 newMatrix( pf->getDataMatrix( inMatrixAddress ) );
  _doInterpolation( newMatrix );
  pf->putDataMatrix( outMatrixAddress, newMatrix );
  return true;
}
bool TrackCalAction::configure(arSZGClient* szgClient) {
  const string dataPath(szgClient->getAttribute("SZG_DATA", "path"));
  const string calFileName(szgClient->getAttribute("SZG_MOTIONSTAR", "calib_file"));
  FILE *fp = ar_fileOpen( calFileName, dataPath, "r" );
  if (fp == NULL) {
    cerr << "TrackCalAction warning: failed to open file \""
         << calFileName << "\" (SZG_MOTIONSTAR/calib_file) in \"" << dataPath << "\" (SZG_CALIB/path).\n";
    return false;
  }

  cout << "TrackCalAction remark: loading file " << calFileName << endl;
  fscanf(fp, "%ld %f %f %ld %f %f %ld %f %f", &_nx, &_xmin, &_dx, &_ny, &_ymin, &_dy, &_nz, &_zmin, &_dz );
  if ((_nx<1) || (_ny<1) || (_nz<1)) {
    cerr << "TrackCalAction error: table dimension < 1.\n";
    fclose(fp);
    return false;
  }
  //  cerr << _nx << ", " << _xmin << ", " << _dx << ", " << _ny << ", " << _ymin << ", " << _dy << ", " << _nz << ", " << _zmin << ", " << _dz << endl;
 _n = _nx*_ny*_nz;
  if ( _n<1 ) {
    cerr << "TrackCalAction warning: lookup table size must be >= 1" << endl
         << " (and should be a great deal bigger than that, you silly bugger).\n";
    fclose(fp);
    return false;
  }
  
  // BIG grab
  _xLookupTable = new float[_n];
  _yLookupTable = new float[_n];
  _zLookupTable = new float[_n];
  if (!_xLookupTable || !_yLookupTable || !_zLookupTable) {
    cerr << "TrackCalAction error: failed to allocate memory for lookup tables.\n";
    fclose(fp);
    return false;
  }

  unsigned long i = 0;
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _xLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "TrackCalAction error: failed to read lookup table value #"
           << i << endl;
      fclose(fp);
      return false;
    }
    //    cerr << _xLookupTable[i] << endl;
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _yLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "TrackCalAction error: failed to read lookup table value #"
           << i+_n << endl;
      fclose(fp);
      return false;
    }
  }
  for (i=0; i<_n; i++) {
    const int stat = fscanf( fp, "%f", _zLookupTable+i );
    if ((stat == 0)||(stat == EOF)) {
      cerr << "TrackCalAction error: failed to read lookup table value #"
           << i+2*_n << endl;
      fclose(fp);
      return false;
    }
  }
  fclose(fp);
  //cout << "TrackCalAction remark: loaded " << 3*_n << " table entries.\n";
  _indexOffsets[0] = 0;
  _indexOffsets[1] = 1;
  _indexOffsets[2] = _nx;
  _indexOffsets[3] = _nx*_ny;
  _indexOffsets[4] = 1+_nx;
  _indexOffsets[5] = 1+_nx*_ny;
  _indexOffsets[6] = _nx+_nx*_ny;
  _indexOffsets[7] = 1+_nx+_nx*_ny;
  cout << "TrackCalAction remark: using calibration.\n";
  return true;
}
bool TrackCalAction::_doInterpolation( arMatrix4& theMatrix ) {
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

void TrackCalAction::_cleanup() {
  if (_xLookupTable)
    delete[] _xLookupTable;
  if (_yLookupTable)
    delete[] _yLookupTable;
  if (_zLookupTable)
    delete[] _zLookupTable;
}

class TrackCalCompiler : public arPForthCompiler {
  public:
    virtual ~TrackCalCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool TrackCalCompiler::compile( arPForth* pf,
                         vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (!pf)
    return false;
  if (arPForthSpace::__PForthSZGClient == 0)
    throw arPForthException("NULL arSZGClient pointer.");
  string theName = "doTrackerCalibration";
  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");
  TrackCalAction* action = new TrackCalAction();
  if (action == 0)
    throw arPForthException("failed to allocate TrackCalAction object.");
  if (!action->configure( arPForthSpace::__PForthSZGClient )) {
    throw arPForthException("failed to configure TrackCalAction object.");
  }
  pf->addAction( action );
  arPForthCompiler* compiler = new SimpleCompiler( action );
  if (compiler == 0)
    throw arPForthException("failed to allocate compiler object.");    
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}

class IIRFilterAction : public arPForthAction {
  public:
    bool configure(arSZGClient*);
    bool run( arPForth* pf );

  private:
    void _doIIRFilter( arMatrix4& newMatrix );
    float _filterWeights[3];
};
bool IIRFilterAction::run( arPForth* pf ) {
  if (!pf)
    return false;
  long outMatrixAddress = (long)pf->stackPop();
  long inMatrixAddress    = (long)pf->stackPop();
  arMatrix4 newMatrix( pf->getDataMatrix( inMatrixAddress ) );
  _doIIRFilter( newMatrix );
  pf->putDataMatrix( outMatrixAddress, newMatrix );
  return true;
}
bool IIRFilterAction::configure(arSZGClient* szgClient) {
  float floatBuf[3] = {0};
  // y_filter_weight determines behavior of an IIR filter (y(i) = (1-w)y(i) + wy(i-1))
  // applied to the head vertical position to remove jitter.
  if (szgClient->getAttributeFloats("SZG_MOTIONSTAR","IIR_filter_weights",floatBuf,3)) {
    for (unsigned int i=0; i<3; i++) {
      if ((floatBuf[i] < 0)||(floatBuf[i] >= 1)) {
        cerr << "IIRFilterAction warning: SZG_MOTIONSTAR/IIR_filter_weight value " << floatBuf[i]
             << " out of range [0,1).\n";
        _filterWeights[i] = 0;
      } else {
        _filterWeights[i] = floatBuf[i];
      }
    }
    cerr << "IIRFilterAction remark: IIR filter weights are ( "
         << _filterWeights[0] << ", " << _filterWeights[1]
         << ", " << _filterWeights[2] << " ).\n";
  }
  return true;
}
static float _lastPosition[] = {-1000,-1000,-1000};
void IIRFilterAction::_doIIRFilter( arMatrix4& m ) {
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

class IIRFilterCompiler : public arPForthCompiler {
  public:
    virtual ~IIRFilterCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool IIRFilterCompiler::compile( arPForth* pf,
                         vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (!pf)
    return false;
  if (arPForthSpace::__PForthSZGClient == 0)
    throw arPForthException("NULL arSZGClient pointer.");
  string theName = "doIIRFilter";
  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");
  IIRFilterAction* action = new IIRFilterAction();
  if (action == 0)
    throw arPForthException("failed to allocate IIRFilterAction object.");
  if (!action->configure( arPForthSpace::__PForthSZGClient )) {
    throw arPForthException("failed to configure IIRFilterAction object.");
  }
  pf->addAction( action );
  arPForthCompiler* compiler = new SimpleCompiler( action );
  if (compiler == 0)
    throw arPForthException("failed to allocate compiler object.");    
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}


}; // PForthSpace namespace

// functions

// installer function
bool ar_PForthAddDatabaseVocabulary( arPForth* pf ) {
  if (!pf->addSimpleActionWord( "getStringParameter", 
                             new arPForthSpace::GetDatabaseParameter() ))
    return false;
  if (!pf->addSimpleActionWord( "getFloatParameters", 
                             new arPForthSpace::GetFloatDatabaseParameters() ))
    return false;
  arPForthSpace::arPForthCompiler* compiler = new arPForthSpace::TrackCalCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthSpace::arPForthDictionaryEntry( "initTrackerCalibration", compiler ) ))
    return false;
  compiler = new arPForthSpace::IIRFilterCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthSpace::arPForthDictionaryEntry( "initIIRFilter", compiler ) ))
    return false;
  return true;
}

void ar_PForthSetSZGClient( arSZGClient* client ) {
  arPForthSpace::__PForthSZGClient = client;
}

arSZGClient* ar_PForthGetSZGClient() {
  return arPForthSpace::__PForthSZGClient;
}

