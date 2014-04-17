#include "arPrecompiled.h"
#include "arEnumeratedTrialGenerator.h"
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arStructuredDataParser.h"
#include "arFileTextStream.h"
#include "arDataUtilities.h"
#include "arExperimentUtilities.h"
#include "arExperimentXMLStream.h"
#if (defined(__GNUC__)&&(__GNUC__<3))
#include <algo.h>
#else
#include <algorithm>
#endif
#include <sstream>

#define DEBUG

using std::string;
using std::vector;

bool arEnumeratedTrialGenerator::init( const string experimentName,
                                       string configPath,
                                       const arHumanSubject& subject,
                                       arExperimentDataRecord& factors,
                                       const arStringSetMap_t& legalStringValues,
                                       arSZGClient& SZGClient ) {
  stringstream& errStream = SZGClient.startResponse();
  // Get config file name from SZGClient
  string fileRoot = SZGClient.getAttribute("SZG_EXPT","file_name");
  if (fileRoot=="NULL") {
    cerr << "arEnumeratedTrialGenerator error: SZG_EXPT/file_name==NULL.\n";
    errStream << "arEnumeratedTrialGenerator error: SZG_EXPT/file_name==NULL.\n";
    return false;
  }
  string configFilename = fileRoot + "_config.xml";
  cerr << "arEnumeratedTrialGenerator remark: config file name is '" << configFilename << "'" << endl;
  
  // Try to construct trial table from config file
  // Setup dictionaries and parser.
  arExperimentDataRecord configHeaderTemplate("experiment_description");
  configHeaderTemplate.addField( "experiment_name", AR_CHAR );
  configHeaderTemplate.addField( "method", AR_CHAR );
  configHeaderTemplate.addField( "subject", AR_CHAR );
  configHeaderTemplate.addField( "num_trials", AR_LONG );
  configHeaderTemplate.addField( "comment", AR_CHAR );
  configHeaderTemplate.addField( "factor_names", AR_CHAR );
  configHeaderTemplate.addField( "factor_types", AR_CHAR );

  arExperimentDataRecord factorTemplate( "trial_record" );
  // we explicitly copy the fields into the template.
  // I originally just used the arExperimentDataRecord copy constructor
  // (from factors), but that doesn't work if the address of one of the factors
  // is owned by the calling program; the address just gets copied.
  arExperimentDataField* fiter;
  for (fiter = factors.getFirstField(); !!fiter; fiter = factors.getNextField()) {
    factorTemplate.addField( fiter->getName(), fiter->getType() );
  }

  // Read the header from the file
  ar_pathAddSlash( configPath );
  arFileTextStream fileStream;
  if (!fileStream.ar_open(configFilename,"",configPath)) {
    cerr << "arEnumeratedTrialGenerator error: couldn't open file " << configFilename << endl
         << "in directory " << configPath << endl;
    errStream << "arEnumeratedTrialGenerator error: couldn't open file " << configFilename << endl
         << "in directory " << configPath << endl;
    return false;
  }  
  arExperimentXMLStream instream( &fileStream );
  instream.addRecordType( configHeaderTemplate );
  instream.addRecordType( factorTemplate );
  arExperimentDataRecord record;

  instream >> record;
  if (instream.fail()) {
    cerr << "arEnumeratedTrialGenerator error: failed to read file header.\n";
    errStream << "arEnumeratedTrialGenerator error: failed to read file header.\n";
    fileStream.ar_close();
    return false;
  }
  
  // Now we get seriously anal
  std::string factorNameString("");
  std::string factorTypeString("");
  arExperimentDataField* df = factors.getFirstField();
  while (!!df) {
    if (factorNameString != "")
      factorNameString += "|";
    factorNameString += df->getName();
    if (factorTypeString != "")
      factorTypeString += "|";
    factorTypeString += arDataTypeName( df->getType() );
    df = factors.getNextField();
  }
  // Copy the comment and num_trials fields from the header to the template
  unsigned int size;
  char* commentPtr = (char*)record.getFieldAddress( "comment", AR_CHAR, size );
  if (!commentPtr)
    return false;
  std::string comment( commentPtr );
  _comment = comment;
  long* numTrialsPtr = (long*)record.getFieldAddress( "num_trials", AR_LONG, size );
  if (!numTrialsPtr)
    return false;
  long numTrials( *numTrialsPtr );
  if (!configHeaderTemplate.setStringFieldValue( "method", "enumerated" ) ||
      !configHeaderTemplate.setStringFieldValue( "subject", subject.getLabel() ) ||
      !configHeaderTemplate.setStringFieldValue( "experiment_name", experimentName ) ||
      !configHeaderTemplate.setStringFieldValue( "comment", comment ) ||
      !configHeaderTemplate.setStringFieldValue( "factor_names", factorNameString ) ||
      !configHeaderTemplate.setStringFieldValue( "factor_types", factorTypeString ) ||
      !configHeaderTemplate.setFieldValue( "num_trials", AR_LONG, (const void* const)&numTrials, 1 )) {
    return false;
  }
  // set so that operator==() will print information about which fields are unequal
  arExperimentDataRecord::setCompDiagnostics(true);
  bool stat(record == configHeaderTemplate);
  arExperimentDataRecord::setCompDiagnostics(false);
  if (!stat) {
    cerr << "arEnumeratedTrialGenerator error: invalid config file header.\n";
    errStream << "arEnumeratedTrialGenerator error: invalid config file header.\n";
    return false;
  }
  
//cerr << "6\n";
  // read in table of parameters
  bool success = true;
  for (long i=0; i<numTrials; i++) {  // try & parse one more than expected (if extra found, fail)
    instream >> record;
    if (instream.fail()) {
      cerr << "arEnumeratedTrialGenerator error: failed to read record #" << i+1 << ", terminating.\n";
      errStream << "arEnumeratedTrialGenerator error: failed to read record #" << i+1 << ", terminating.\n";
      success = false;
      goto finish;
    }
    if (record.getName() != "trial_record") {
      cerr << "arEnumeratedTrialGenerator error: record #" << i+1 << " has type '" << record.getName()
           << "', expected 'trial_record;.\n";
      errStream << "arEnumeratedTrialGenerator error: record #" << i+1 << " has type '" << record.getName()
           << "', expected 'trial_record;.\n";
      success = false;
      goto finish;
    }
//cerr << "7\n";    
    if (validateRecord( record, factors, legalStringValues )) {
//#ifdef DEBUG
//      cerr << record << endl;
//#endif
      _trialTable.push_back( record );
  } else {
      cerr << "arEnumeratedTrialGenerator error: failed to validate record #" << i+1 << endl;
      errStream << "arEnumeratedTrialGenerator error: failed to validate record #" << i+1 << endl;
      success = false;
      goto finish;
    }
  }
  instream >> record;
  if (!instream.fail()) {
    cerr << "arEnumeratedTrialGenerator error: too many trial records found.\n";
    success = false;
    goto finish;
  }
//cerr << "8\n";    
finish:  
  if (!success)
    return false;
    
  _trialNumber = 0;
  return true;
}

bool arEnumeratedTrialGenerator::newTrial( arExperimentDataRecord& factors ) {
  if (_trialTable.size()==0) {
    cerr << "arEnumeratedTrialGenerator error: empty trial table.\n";
    return false;
  }
  if (_trialNumber >= _trialTable.size()) {
    return false;
  }
  arExperimentDataRecord& currentTrial = _trialTable[_trialNumber];
  if (!currentTrial.matchNamesTypes( factors )) {
    cerr << "arEnumeratedTrialGenerator error: trial table fields do not match factors.\n";
    return false;
  }
  arExperimentDataField* field;
  for (field = currentTrial.getFirstField(); !!field; field = currentTrial.getNextField()) {
    factors.setFieldValue( field->getName(), field->getType(), field->getAddress(), field->getSize() );
  }
#ifdef DEBUG
  arExperimentDataRecord::setUseFieldPrintFormat(true);
  cerr << factors << endl;
  arExperimentDataRecord::setUseFieldPrintFormat(false);
#endif
  _trialNumber++;
  return true;  
}

long arEnumeratedTrialGenerator::numberTrials() const {
  return _trialTable.size();
}

bool arEnumeratedTrialGenerator::validateRecord( arExperimentDataRecord& trialData,
                                                arExperimentDataRecord& factors,
                                                const arStringSetMap_t& legalStringValues ) {
  bool stat( true );
  // Validate record
  if (!factors.matchNamesTypes( trialData )) {
    cerr << "arEnumeratedTrialGenerator error: matchNamesTypes() failed in validateRecord().\n";
    return false;
  }
  for (arExperimentDataField* df = factors.getFirstField(); !!df; df = factors.getNextField()) {
    bool goodItem(true);
    arExperimentDataField* newField = trialData.getField( df->getName() );
    if (!newField) {
      cerr << "arEnumeratedTrialGenerator error: failed to get new field " << df->getName() << endl;
      goodItem = false;
      goto enditemcheck;
    }
    if (df->getType() == AR_CHAR ) {
      char* const ptr = (char* const)newField->getAddress();
      if (!ptr) {
        cerr << "arEnumeratedTrialGenerator error: failed to get address for new field " << df->getName() << endl;
        goodItem = false;
        goto enditemcheck;
      }
      std::string tempString(ptr);
      if (tempString.size() >= df->getSize()) {
        cerr << "arEnumeratedTrialGenerator error: size for field '" << df->getName() << "'" << endl
             << " (" << tempString << ")" << endl
             << "exceeds maximum specified dimension (" << df->getSize()-1 << ")" << endl;
        goodItem = false;
        goto enditemcheck;
      }
      arStringSetMap_t::const_iterator mapiter = legalStringValues.find( df->getName() );
      if (mapiter != legalStringValues.end() ) { // we have a list of legal values for this factor
        const arStringSet_t* legalSet = &(mapiter->second);
        arStringSet_t::const_iterator setiter = legalSet->find( tempString );
        if (setiter == legalSet->end() ) { // tempString doesn't match any of the default values
          cerr << "arEnumeratedTrialGenerator error: value read for char field " << df->getName() << endl
               << "(" << tempString << ") is not a legal value for this field.\n"
               << "Legal values are: ";
          for (setiter = legalSet->begin(); setiter != legalSet->end(); setiter++ )
            cerr << *setiter << " ";
          cerr << endl;
          goodItem = false;
        }
      }
      goto enditemcheck;
    } else if ((df->getType() == AR_LONG )||(df->getType() == AR_DOUBLE )) {
    } else { // should never happen
        cerr << "arEnumeratedTrialGenerator error: invalid factor type.\n"
             << arDataTypeName(df->getType()) << " in field '" << df->getName() << "'" << endl;
        goodItem = false;
        goto enditemcheck;
    }
enditemcheck:
    if (!goodItem) {
      stat = false;
    }
  }
  return stat;
}
    

