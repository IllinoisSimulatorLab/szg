#include "arPrecompiled.h"
#include <iostream>
#include <sstream>
#if (defined(__GNUC__)&&(__GNUC__<3))
#include <algo.h>
#else
#include <algorithm>
#endif
#ifdef AR_USE_WIN_32
#include <direct.h>
#endif

#include "arExperiment.h"
#include "arTrialGenerator.h"
#include "arTrialGenBuilder.h"
#include "arDataSaverBuilder.h"

// get rid of after moving conversion routines
//#include <stdlib.h>

using std::cerr;

arExperiment::arExperiment() :
  _configured( false ),
  _running( false ),
  _completed( false ),
  _currentTrialNumber(-1),
  _experimentName(""),
  _trialGenMethod(""),
  _dataStyle(""),
  _trialGen(0),
  _currentPhasePtr(0) {
    _factors.setName("factors");
}
//arExperiment::arExperiment() :
//  _configured( false ),
//  _running( false ),
//  _completed( false ),
//  _currentTrialNumber(-1),
//  _trialGen(0),
//  _dataSaver(0) {
//}

arExperiment::~arExperiment() {
  if (_running)
    stop();
}

/// A valid config file will be required to contain a factor list that matches
/// those registered by the program. Must be called for each factor before init();
/// may not be called after init().
/// @param fname The factor name.
/// @param theType An arDataType (currently must be AR_CHAR, AR_LONG, or AR_DOUBLE,
/// because those are the kinds that we have conversion routines for with error-checking).
/// @param address The address
/// @param theSize The number of elements (not bytes). For numeric data currently
/// this must be 1, for strings it's the max. length.

bool arExperiment::addFactor( const std::string fname, const arDataType theType,
                              const void* address, const unsigned int theSize ) {
  if (_configured) {
    cerr << "arExperiment error: attempt to add factor " << fname << " after calling init().\n";
    return false;
  }
  // NOTE: type restriction here is because these are the only types we can currently
  // convert strings to with error checking.
  if ((theType!=AR_CHAR)&&(theType!=AR_DOUBLE)&&(theType!=AR_LONG)) {
    cerr << "arExperiment error: factor type must be AR_CHAR, AR_LONG, or AR_DOUBLE.\n";
    return false;
  }
  if (_factors.fieldExists( fname )) {
    cerr << "arExperiment error: attempt to add factor " << fname << " more than once.\n";
    return false;
  }
  if (_dataFields.fieldExists( fname )) {
    cerr << "arExperiment error: attempt to add factor " << fname << " with same name as a data field.\n";
    return false;
  }
  if (theSize == 0) {
    cerr << "arExperiment error: a factor may not have size 0.\n";
    return false;
  }
//  if (((theType == AR_LONG)||(theType == AR_DOUBLE))&&(theSize != 1)) {
//    cerr << "arExperiment error: numeric factors must be SCALAR (size=1).\n"
//         << "     (offending factor " << fname << ").\n";
//    return false;
//  }
  void* ptr = const_cast<void*>(address);
  bool ownPtr = false;
  if (!ptr) {
    ptr = ar_allocateBuffer( theType, theSize );
    if (!ptr) {
      cerr << "arExperiment error: memory allocation failed in addFactor().\n";
      return false;
    }
    ownPtr = true;
  }
  bool stat = _factors.addField( fname, theType, const_cast<const void*>(ptr), theSize, ownPtr );
  if (!stat) {
    cerr << "arExperiment error: addFactor() failed.\n";
  }
  return stat;
}

const void* const arExperiment::getFactor( const std::string fname,
                               const arDataType theType,
                               unsigned int& theSize ) {
  arExperimentDataField* df = _factors.getField( fname, theType );
  if (!df) {
    cerr << "arExperiment error: can't get factor named " << fname << endl;
    theSize = 0;
    return (const void* const)0;
  }
  theSize = df->getSize();
  const void* const ptr = const_cast<const void* const>(df->getAddress());
  if (!ptr) {
    cerr << "arExperiment error: getFactor() failed.\n";
  }
  return ptr;
}

/// An alternative to addFactor for character strings.
/// @param fname The factor name
/// @param address The address
/// @param legalValues A pointer to null-terminated char string containing
/// the legal values for this factor, separated by vertical-bar characters ('|').
/// If a value for this factor in the config file does not match one of these,
/// an error will occur.

bool arExperiment::addCharFactor( const std::string fname, 
                                  std::vector<string>& legalValues,
                                  const char* address ) {
  arStringSet_t lv;
  int maxLen = 0;
  std::vector<string>::iterator iter;
  for (iter = legalValues.begin(); iter != legalValues.end(); ++iter) {
    std::string& s = *iter;
    if (s!="") {
      lv.insert(s);
      if (s.size() > (unsigned int)maxLen)
        maxLen = s.size();
    }
  }
  if (!_legalStringValues.insert( arStringSetMap_t::value_type( fname, lv ) ).second) {
    cerr << "arExperiment error: duplicate entry found in legal value table "
         << "for char factor " << fname << endl;
    return false;
  }
  bool stat = addFactor( fname, AR_CHAR, static_cast<const void*>(address), maxLen+1 );
  if (!stat) {
    cerr << "arExperiment error: addCharFactor() failed.\n";
  }
  return stat;
}
  
/// An alternative to addFactor for character strings.
/// @param fname The factor name
/// @param address The address
/// @param legalValues A pointer to null-terminated char string containing
/// the legal values for this factor, separated by vertical-bar characters ('|').
/// If a value for this factor in the config file does not match one of these,
/// an error will occur.

bool arExperiment::addCharFactor( const std::string fname, 
                                  const char* const legalValues,
                                  const char* address ) {
  arStringSet_t lv;
  std::string legalString( legalValues );
  istringstream legalStream( legalString );
  int maxLen = 0;
  do {
    std::string s;
    getline( legalStream, s, '|' );  // vertical bar delimiter
    if (s!="") {
      lv.insert(s);
      if (s.size() > (unsigned int)maxLen)
        maxLen = s.size();
    }
  } while (!legalStream.fail());
  if (!_legalStringValues.insert( arStringSetMap_t::value_type( fname, lv ) ).second) {
    cerr << "arExperiment error: duplicate entry found in legal value table "
         << "for char factor " << fname << endl;
    return false;
  }
  bool stat = addFactor( fname, AR_CHAR, static_cast<const void*>(address), maxLen+1 );
  if (!stat) {
    cerr << "arExperiment error: addCharFactor() failed.\n";
  }
  return stat;
}
  
/// Data in these fields will be saved to a file between trials.  Can
/// only be called before init().
/// @param fname The field name.
/// @param theType An arDataType (AR_INT, AR_CHAR, AR_FLOAT, AR_LONG, AR_DOUBLE)

bool arExperiment::addDataField( const std::string fname,
                                 const arDataType theType ) {
  if (_configured) {
    cerr << "arExperiment error: attempt to add data field " << fname << " after calling init().\n";
    return false;
  }
  if (_factors.fieldExists( fname )) {
    cerr << "arExperiment error: attempt to add data field name " << fname << " to same as a factor.\n";
    return false;
  }
  if (_dataFields.fieldExists( fname )) {
    cerr << "arExperiment error: attempt to add data field " << fname
         << ", which already exists.\n";
    return false;
  }
  bool stat = _dataFields.addField( arExperimentDataField( fname, theType ) );
  if (!stat) {
    cerr << "arExperiment error: addDataField() failed.\n";
  }
  return stat;
}

/// Data in these fields will be saved to a file between trials.  New fields can only be added
/// before init() is called. Note that this usage is deprecated,
/// use addDataField() instead. After init(), this function can only be called to change the
/// pointer and/or size of the data field; the field name and type must match one that was set
/// before init().
/// @param fname The field name.
/// @param theType An arDataType (AR_INT, AR_CHAR, AR_FLOAT, AR_LONG, AR_DOUBLE)
/// @param address The address.
/// @param theSize Number of data elements (not bytes) to save.

bool arExperiment::setDataField( const std::string fname,
                                 const arDataType theType,
                                 const void* address,
                                 const unsigned int theSize ) {
  if (address && (theSize==0)) {
    cerr << "arExperiment warning: attempt to add non-NULL data field pointer for field "
         << fname << endl << "   with 0 field size.\n";
    return true;
  }
  if (_factors.fieldExists( fname )) {
    cerr << "arExperiment error: attempt to set data field name " << fname << " to same as a factor.\n";
    return false;
  }
  if (!_dataFields.fieldExists( fname )) {
    if (_configured) {
      cerr << "arExperiment error: attempt to add data field " << fname << " after calling init().\n";
      return false;
    }
    _dataFields.addField( fname, theType, address, theSize );
    return true;
  }
  arExperimentDataField* df = _dataFields.getField( fname );
  if (df->getType() != theType) { // exists but has wrong type
    cerr << "arExperiment error: attempt to change type of data field " << df->getName() << endl;
    return false;
  }
  *df = arExperimentDataField( fname, theType, address, theSize );
  return true;
}

const void* arExperiment::getDataField( const std::string fname,
                                        const arDataType theType,
                                        unsigned int& theSize ) {
  const void* ptr = _dataFields.getFieldAddress( fname, theType, theSize );
  if (!ptr) {
    cerr << "arExperiment error: getDataField() failed.\n";
  }
  return ptr;
}

/// Must be called with all requisite parameter names and types before init().
///.
/// The human subject database is an xml file contained in the main directory for this
/// experiment, e.g. in <SZG_EXPT/path>/<executable name>Data/. It contains records
/// for each subject in the experiment, accessed by the current value of SZG_EXPT/subject
/// (generally the subject's initials). A record of this database must contain minimally
/// the subjects label, full name, and interocular spacing (in cm.). Other subject
/// parameters specific to the current experiment may be added if desired. The list
/// of parameters in the database must match those requested by the program.
/// @param theName The parameter name, e.g. eye_spacing_cm.
/// @param theType An arDataType (currently must be AR_CHAR, AR_LONG, or AR_DOUBLE)

bool arExperiment::addSubjectParameter( const std::string theName, arDataType theType ) {
  return _subject.addParameter( theName, theType );
}

/// May only be called after init().
/// @param theName The parameter name.
/// @param theType The arDataType of the parameter.
/// @param address Where to stick the value.

bool arExperiment::getSubjectParameter( const std::string theName,
                                     arDataType theType,
                                     void* address ) {
  return _subject.getParameterValue( theName, theType, address );
}

/// May only be called after init().
/// @param theName The parameter name.
/// @param value Where to stick the value.

bool arExperiment::getCharSubjectParameter( const std::string& theName,
                                     std::string& value ) {
  return _subject.getStringParameter( theName, value );
}

/// Set the experiment name. This would normally be the name of the executable
/// (minus .exe under windows). Use this only in special cases, e.g. when
/// creating a demo that will reside in the same directory as an experiment.
/// Must be called before init().
/// Initialize the experiment.
bool arExperiment::setName( const std::string& name ) {
  if (_configured) {
    cerr << "arExperiment error: calling setName() after init().\n";
    return false;
  }
  _experimentName = name;
  return true;
}

/// Determine whether or not data will be saved. This would normally be read
/// from SZG_EXPT/save_data, but if this has been called that parameter will
/// be ignored. Use this only in special cases, e.g. when
/// creating a demo that will reside in the same directory as an experiment.
/// Must be called before init().
bool arExperiment::saveData( bool yesno ) {
  if (_configured) {
    cerr << "arExperiment error: calling saveData() after init().\n";
    return false;
  }
  if (!yesno) {
    _dataStyle = "none";
  } else {
    _dataStyle = "xml";
  }
  return true;
}

/// Set the trial-generation method. This would normally be read from
/// SZG_EXPT/method, but if this has been called that parameter will be ignored.
/// Must be called before init().
bool arExperiment::setTrialGenMethod( const std::string& method ) {
  if (_configured) {
    cerr << "arExperiment error: calling setTrialGenMethod() after init().\n";
    return false;
  }
  _trialGenMethod = method;
  return true;
}
bool arExperiment::setTrialGenMethod( arTrialGenerator* methodPtr ) {
  if (_configured) {
    cerr << "arExperiment error: calling setTrialGenMethod() after init().\n";
    return false;
  }
  if (!methodPtr) {
    cerr << "arExperiment error: setTrialGenMethod() called with NULL pointer.\n";
    return false;
  }
  if ((_trialGen != 0)&&(_trialGenMethod != "experiment-defined")) {
    delete _trialGen;
  }
  _trialGen = methodPtr;
  _trialGenMethod = "experiment-defined";
  return true;
}

/// Add a data-saver. Must be called before init().
bool arExperiment::addDataSaver( const std::string& dsName, arDataSaver* dsPtr ) {
  if (_configured) {
    cerr << "arExperiment error: calling addDataSaver() after init().\n";
    return false;
  }
  if (!dsPtr) {
    cerr << "arExperiment error: addDataSaver() called with NULL pointer.\n";
    return false;
  }
  if (!_dataSavers.insert( arDataSaverMap_t::value_type( dsName, dsPtr ) ).second) {
    cerr << "arExperiment error: failed to insert data saver " << dsName << " in map.\n";
    return false;
  }
  return true;
}

/// Config files will typically be read in and validated against the list of
/// factor names, types, and sizes set within the progam.  The human subject
/// database will be checked for a record with the current subject's label,
/// and his or her parameters will be read.  The data saver engine will
/// be instantiated and inited.
/// @param SZGClient An arSZGClient, for getting additional database parameters

bool arExperiment::init( arSZGAppFramework* framework, arSZGClient& SZGClient ) {
//  stringstream& errStream= SZGClient.startResponse();
  if (_configured) {
    cerr << "arExperiment error: attempt to init() inited experiment.\n";
    return false;
  }
  if (!framework) {
    cerr << "arExperiment error: attempt to init() with NULL arSZGAppFramework pointer.\n";
    SZGClient.startResponse() << "arExperiment error: attempt to init() with NULL arSZGAppFramework pointer.\n";
    return false;
  }
  if (_experimentName == "")
    _experimentName = SZGClient.getLabel();
  cerr << "arExperiment remark: experiment name == " << _experimentName << endl;
  
  std::string rootDirectory = SZGClient.getAttribute("SZG_EXPT","path");
  if (rootDirectory== "NULL") {
    cerr << "arExperiment error: SZG_EXPT/path==NULL.\n";
    SZGClient.startResponse() << "arExperiment error: SZG_EXPT/path==NULL.\n";
    return false;
  }
  cerr << "arExperiment remark: SZG_EXPT/path == " << rootDirectory << endl;

  ar_pathAddSlash( rootDirectory );
  std::string exptDirectory = rootDirectory + _experimentName + "Data";
  ar_pathAddSlash( exptDirectory );

  std::string subjectLabel;
  if (!_subject.init( exptDirectory, SZGClient )) {
    cerr << "arExperiment error: failed to init() arHumanSubject.\n";
    SZGClient.startResponse() << "arExperiment error: failed to init() arHumanSubject.\n";
    return false;
  }
  
  std::string subjectDirectory = exptDirectory + _subject.getLabel();
  
  bool itExists, isDirectory;
  if (!ar_directoryExists( subjectDirectory, itExists, isDirectory )) {
    cerr << "arExperiment error: directory lookup failed for " << subjectDirectory << endl;
    SZGClient.startResponse() << "arExperiment error: directory lookup failed for " << subjectDirectory << endl;
    return false;
  }
  if (!itExists) {
    cerr << "arExperiment error: directory " << subjectDirectory << " does not exist.\n"
         << "    Sorry, but currently you must create it manually.\n";
    return false;
  }
  if (!isDirectory) {
    cerr << "arExperiment error: an item exists at " << subjectDirectory << ",\n"
         << "    but it is not a directory.  What were you thinking?\n";
    return false;
  }
  
  // add _factors to beginning of _dataFields
//  std::string::size_type dataLength = _dataFields.size();
//  std::copy( _factors.begin(), _factors.end(), std::back_inserter( _dataFields ) );
//  std::rotate( _dataFields.begin(), _dataFields.begin()+dataLength, _dataFields.end() );
  
  if (_trialGen == 0) {
    arTrialGenBuilder tgBuilder;
    if (_trialGenMethod == "") {
      _trialGen = tgBuilder.build( SZGClient );
    } else {
      _trialGen = tgBuilder.build( _trialGenMethod );
    }
  }
  if (_trialGen==0) {
    cerr << "arExperiment error: failed to build trial generator.\n";
    return false;
  }
  if (!_trialGen->init( _experimentName, subjectDirectory, _subject,
                        _factors, _legalStringValues, SZGClient )) {
    cerr << "arExperiment error: failed to init trial generator.\n";
    SZGClient.startResponse() << "arExperiment error: A problem with the specification\n"
              << "  or contents of the config file.\n";
    stop();
    return true;
//    return false;
  }
  if (_dataStyle == "") {
    std::string saveData = SZGClient.getAttribute("SZG_EXPT", "save_data", "|true|false|");
    if (saveData == "false") {
      _dataStyle = "none";
    } else {
      _dataStyle = "xml";
    }
  }
  arDataSaverBuilder dsBuilder;
  arDataSaver* dsTemp;
  dsTemp = dsBuilder.build( _dataStyle );
  if (!dsTemp) {
    cerr << "arExperiment error: failed to build data saver of style "
         << _dataStyle << endl;
    stop();
    return false;
  }
  if (!_dataSavers.insert( arDataSaverMap_t::value_type( _dataStyle, dsTemp ) ).second) {
    cerr << "arExperiment error: failed to insert data saver " << _dataStyle << " in map.\n";
    stop();
    return false;
  }
  arDataSaverMap_t::iterator dsIter;
  for (dsIter = _dataSavers.begin(); dsIter != _dataSavers.end(); dsIter++) {
    if (!dsIter->second->init( _experimentName, subjectDirectory, 
                               _trialGen->comment(), _subject,
                               _factors, _dataFields, SZGClient )) {
      cerr << "arExperiment error: failed to init data saver of style "
           << dsIter->first << endl;
      SZGClient.startResponse() << "arExperiment error: failed to initialize data saver\n"
                << "  (either there's a problem with the path to the\n"
                << "  specified data file, or the data file already exists).\n";
      stop();
      return false;
    }
  }
  _configured = true;
  return true;
}

bool arExperiment::start() {
  if (!_configured) {
    cerr << "arExperiment error: attempt to start() before configured.\n";
    return false;
  }
  _running = true;
  _currentTrialNumber = 0;
  return true;
}

/// Deletes any dynamically-allocated internal objects (the trial-parameter
/// generator and the data-saver).

bool arExperiment::stop() {
  ar_log_debug() << "arExperiment.stop().\n";
  if (!_configured) {
    cerr << "arExperiment warning: attempt to stop() unconfigured experiment.\n";
  } else if (!_completed) {
    cerr << "arExperiment warning: stop() called before completion.\n";
  }
    
  if (_trialGen!=0) {
    if (_trialGenMethod != "experiment-defined") {
      ar_log_debug() << "deleting trial generator.\n";
      delete _trialGen;
    }
    _trialGen = 0;
    _trialGenMethod = "";
  }
  arDataSaverMap_t::iterator dsIter;
  for (dsIter = _dataSavers.begin(); dsIter != _dataSavers.end(); dsIter++) {
    if (dsIter->second) {
      ar_log_debug() << "Deleting '" << dsIter->first << "' data saver.\n";
      delete dsIter->second;
      dsIter->second = 0;
    }
  }
  _dataSavers.clear();
  arExperimentTrialPhaseMap_t::iterator phaseIter;
  for (phaseIter = _trialPhases.begin(); phaseIter != _trialPhases.end(); phaseIter++) {
    if (phaseIter->second) {
      ar_log_debug() << "Deleting '" << phaseIter->first << "' trial phase.\n";
      delete phaseIter->second;
      phaseIter->second = 0;
    }
  }
  _trialPhases.clear();
  _currentTrialNumber = -1;
  _currentPhasePtr = 0;
  _running = false;

  ar_log_debug() << "arExperiment.stop() done.\n";
  return true;
}

/// Returns false and calls stop() if session is done or an error occurs;
/// check for successful completion using completed().

bool arExperiment::newTrial( bool saveLastTrial ) {
  if (saveLastTrial) {
    if (_currentTrialNumber != 0) {
      arDataSaverMap_t::iterator dsIter;
      for (dsIter = _dataSavers.begin(); dsIter != _dataSavers.end(); dsIter++) {
        ar_log_debug() << "arExperiment saving data.\n"; 
        if (!dsIter->second->saveData( _factors, _dataFields )) {
          cerr << "arExperiment error: failed to save data from preceding trial.\n";
          stop();
          return false;
        }
      }
    }
  }
  arExperimentDataRecord::setUseFieldPrintFormat(true);
//  cerr << "Factors before: " << _factors << endl;
  ar_log_debug() << "arExperiment calling trial generator.\n";
  if (!_trialGen->newTrial( _factors )) {
    cerr << "arExperiment remark: session completed.\n";
    _completed = true;
    stop();
    return false;
  }
//  cerr << "Factors after: " << _factors << endl;
  arExperimentDataRecord::setUseFieldPrintFormat(false);
  if (saveLastTrial)
    _currentTrialNumber++;
  return true;
}

/// Sometimes it's easier on the subject if they have this information.

long arExperiment::currentTrialNumber() const {
  return _currentTrialNumber;
}

long arExperiment::numberTrials() const {
  return _trialGen->numberTrials();
}

/// If newTrial() returned false, did we finish successfully?

bool arExperiment::completed() const {
  return _completed;
}

bool arExperiment::setTrialPhase( const std::string theName, arExperimentTrialPhase* thePhase ) {
  if (!thePhase) {
    cerr << "arExperiment error: NULL trial phase pointer.\n";
    return false;
  }
  arExperimentTrialPhaseMap_t::iterator iter = _trialPhases.find( theName );
  if (iter != _trialPhases.end()) {
    delete iter->second;
    iter->second = thePhase;
  } else {
    _trialPhases[theName] = thePhase;
  }
  return true;
}

std::string arExperiment::currentTrialPhase() {
  if (!_configured) {
    cerr << "arExperiment error: calling currentTrialPhase() before init().\n";
    return false;
  }
  arExperimentTrialPhaseMap_t::iterator iter;
  for (iter = _trialPhases.begin(); iter != _trialPhases.end(); iter++) {
    if (iter->second == _currentPhasePtr)
      return iter->first;
  }
  return "NULL";
}

bool arExperiment::activateTrialPhase( const std::string theName, arSZGAppFramework* fw ) {
  if (!_configured) {
    cerr << "arExperiment error: calling activateTrialPhase() before init().\n";
    return false;
  }
  arExperimentTrialPhaseMap_t::iterator iter = _trialPhases.find( theName );
  if (iter == _trialPhases.end()) {
    cerr << "arExperiment error: trial phase " << theName << " not found.\n";
    return false;
  }
  arExperimentTrialPhase* phasePtr = iter->second;
  if (!phasePtr) {
    cerr << "arExperiment error: NULL phase pointer for trial phase " << theName << endl;
    return false;
  }
  _currentPhasePtr = phasePtr;
  return _currentPhasePtr->init( fw, this );
}

bool arExperiment::updateTrialPhase( arSZGAppFramework* fw ) {
  if (!_configured) {
    cerr << "arExperiment error: calling updateTrialPhase() before init().\n";
    return false;
  }
  if (!_running) {
    return true;
  }
  if (!_currentPhasePtr) {
    return false;
  }
  return _currentPhasePtr->update( fw, this );
}

bool arExperiment::updateTrialPhase( arSZGAppFramework* fw, arIOFilter* filter, arInputEvent& event ) {
  if (!_configured) {
    cerr << "arExperiment error: calling updateTrialPhase() before init().\n";
    return false;
  }
  if (!_running) {
    return true;
  }
  if (!_currentPhasePtr) {
    return false;
  }
  return _currentPhasePtr->update( fw, this, filter, event );
}

void* const arExperiment::allocateDataField( const std::string& name, arDataType typ, unsigned int size ) {
//  cerr << "arExperiment remark: entering allocateDataField().\n";  
  arExperimentDataField* df = _dataFields.getField( name, typ );
  if (!df) {
    cerr << "arExperiment error: _dataFields.getField() in allocateDataField(" << name << ") failed.\n";
    return (void*)0;
  }
  if (!df->selfOwned()) {
    cerr << "arExperiment error: you are asking me to allocate storage for data field '"
         << name << "', which you own! Dummy!.\n";
    return (void*)0;
  }
//  cerr << "arExperiment remark: calling df.makeStorage(" << size << ").\n";  
  void* const p = df->makeStorage( size );
  if (!p) {
    cerr << "arExperiment error: makeStorage() in allocateDataField(" << name << ") failed.\n";
  }
//  cerr << df << endl;
//  cerr << "arExperiment remark: leaving df.makeStorage().\n";  
  return p;
}

bool arExperiment::setDataFieldData( const std::string& name, arDataType typ, 
                                     const void* const address, unsigned int size ) {
  arExperimentDataField* df = _dataFields.getField( name, typ );
  if (!df) {
    cerr << "arExperiment error: setDataFieldData() failed.\n";
    return false;
  }
  bool stat = df->setData( typ, address, size );
  if (!stat) {
    cerr << "arExperiment error: setDataFieldData() failed.\n";
  }
  return stat;
}

void arExperiment::_printDataFields() {
  arExperimentDataField* df = _dataFields.getFirstField();
  while (!!df) {
    cerr << *df << endl;
    df = _dataFields.getNextField();
  }
}

