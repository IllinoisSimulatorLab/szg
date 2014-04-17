#include "arPrecompiled.h"
#include "arHumanSubject.h"
#include "arTemplateDictionary.h"
#include "arStructuredDataParser.h"
#include "arDataUtilities.h"
#include "arExperimentUtilities.h"
#if (defined(__GNUC__)&&(__GNUC__<3))
#include <algo.h>
#else
#include <algorithm>
#endif

using std::string;

arHumanSubject::arHumanSubject() :
  _configured(false),
  _headerData(0),
  _subjectRecord(0) {
  addParameter( "name", AR_CHAR );
  addParameter( "label", AR_CHAR );
  addParameter( "eye_spacing_cm", AR_FLOAT );
}

arHumanSubject::~arHumanSubject() {
  _parameters.erase( _parameters.begin(), _parameters.end() );
  if (_headerData != 0)
    delete _headerData;
  if (_subjectRecord != 0)
    delete _subjectRecord;
}

bool arHumanSubject::addParameter( const string fname, const arDataType theType ) {
  if (_configured) {
    cerr << "arHumanSubject error: attempt to add parameter " << fname << " after calling init().\n";
    return false;
  }
  ParamVal_t tempRecord;
  tempRecord._type = theType;
  tempRecord._value = "";
  if (!_parameters.insert( ParamList_t::value_type( fname, tempRecord ) ).second) {
    cerr << "arHumanSubject error: duplicate entry found in parameter list "
         << "for parameter " << fname << endl;
    return false;
  }
  return true;
}

bool arHumanSubject::getStringParameter( const std::string name, std::string& value ) {
  arDataType theType;
  if (!getParameterType( name, theType )) {
    cerr << "arHumanSubject error: parameter " << name << " not found.\n";
    return false;
  }
  if (theType != AR_CHAR) {
    cerr << "arHumanSubject error: parameter " << name << " is not a string.\n";
    return false;
  }
  value = getParameterString( name );
  return true;
}

bool arHumanSubject::getParameterValue( const string fname,
                                     const arDataType theType,
                                     void* address ) {
  if (!_configured) {
    cerr << "arHumanSubject error: attempt to get parameter value before init().\n";
    return false;
  }
  ParamList_t::const_iterator iter;
//  iter = std::find_if( _parameters.begin(), _parameters.end(),
//                       bind2nd( equal_to<string>(), fname ) );
  iter = _parameters.find( fname );
  if (iter == _parameters.end()) {
    cerr << "arHumanSubject error: parameter " << fname << " not found.\n";
    return false;
  }
  if (iter->second._type != theType) {
    cerr << "arHumanSubject error: parameter " << fname << " has type\n"
         << arDataTypeName( iter->second._type ) << ", not "
         << arDataTypeName( theType ) << endl;
    return false;
  }
  long lTemp;
  double dTemp;
  switch ( theType ) {
    case AR_CHAR:
      memcpy( address, iter->second._value.c_str(), iter->second._value.size() + 1 );
      break;
    case AR_LONG:
      if (!ar_stringToLongValid( iter->second._value, *((long*)address) )) {
        cerr << "arHumanSubject error: string->long conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    case AR_INT:
      if (!ar_stringToLongValid( iter->second._value, lTemp )) {
        cerr << "arHumanSubject error: string->int conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }      
      if (!ar_longToIntValid( lTemp, *((int*)address) )) {
        cerr << "arHumanSubject error: string->int conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    case AR_DOUBLE:
      if (!ar_stringToDoubleValid( iter->second._value, *(double*)address )) {
        cerr << "arHumanSubject error: string->double conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    case AR_FLOAT:
      if (!ar_stringToDoubleValid( iter->second._value, dTemp )) {
        cerr << "arHumanSubject error: string->float conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }      
      if (!ar_doubleToFloatValid( dTemp, *(float*)address )) {
        cerr << "arHumanSubject error: string->float conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    default:
      cerr << "arHumanSubject error: unknown type for\n"
             << "parameter " << iter->first << endl;
      return false;
      break;
  }
  return true;
}

bool arHumanSubject::getParameterType( const std::string fname, arDataType& theType ) const {
  if (!_configured) {
    cerr << "arHumanSubject error: attempt to get parameter type before init().\n";
    return false;
  }
  ParamList_t::const_iterator iter;
  iter = _parameters.find( fname );
  if (iter == _parameters.end()) {
    cerr << "arHumanSubject error: parameter " << fname << " not found.\n";
    return false;
  }
  theType = iter->second._type;
  return true;
}

unsigned int arHumanSubject::getParameterSize( const std::string fname ) const {
  if (!_configured) {
    cerr << "arHumanSubject error: attempt to get parameter size before init().\n";
    return 0;
  }
  ParamList_t::const_iterator iter;
  iter = _parameters.find( fname );
  if (iter == _parameters.end()) {
    cerr << "arHumanSubject error: parameter " << fname << " not found.\n";
    return 0;
  }
  if (iter->second._type != AR_CHAR)
    return 1;
  return iter->second._value.size();
}

std::string arHumanSubject::getParameterString( const std::string fname ) const {
  if (!_configured) {
    cerr << "arHumanSubject error: attempt to get parameter string before init().\n";
    return "";
  }
  ParamList_t::const_iterator iter;
  iter = _parameters.find( fname );
  if (iter == _parameters.end()) {
    cerr << "arHumanSubject error: parameter " << fname << " not found.\n";
    return "";
  }
  return iter->second._value;
}

bool arHumanSubject::getParameterNames( std::vector< std::string >& nameList ) const {
  if (!_configured) {
    cerr << "arHumanSubject error: attempt to get parameter names before init().\n";
    return false;
  }
  ParamList_t::const_iterator iter;
  for (iter = _parameters.begin(); iter != _parameters.end(); iter++)
    nameList.push_back( iter->first );
  return true;
}

const arStructuredData* arHumanSubject::getHeaderRecord() const {
  return const_cast<const arStructuredData*>( _headerData );
}

const arStructuredData* arHumanSubject::getSubjectRecord() const {
  return const_cast<const arStructuredData*>( _subjectRecord );
}

const string arHumanSubject::getLabel() const {
  if (!_configured) {
    cerr << "arHumanSubject error: attempt to call getLabel() before init().\n";
    return "NULL";
  }
  ParamList_t::const_iterator iter;
  iter = _parameters.find( "label" );
  if (iter == _parameters.end()) {
    cerr << "arHumanSubject error: label not found.\n";
    return "NULL";
  }
  return iter->second._value;
}

bool arHumanSubject::init( string exptDirectory, arSZGClient& SZGClient ) {
  if (_configured) {
    cerr << "arHumanSubject error: called init() more than once.\n";
    return false;
  }
  stringstream& errStream = SZGClient.startResponse();
  string subjectLabel = SZGClient.getAttribute("SZG_EXPT", "subject");
  if (subjectLabel=="NULL") {
    cerr << "arHumanSubject error: SZG_EXPT/subject==NULL.\n";
    errStream << "arHumanSubject error: SZG_EXPT/subject==NULL.\n";
    return false;
  }
  cerr << "arHumanSubject remark: SZG_EXPT/subject == " << subjectLabel << endl;
  
  ar_pathAddSlash( exptDirectory );
  string subjectDatabaseFilename = "subject_data.xml";

  arFileTextStream fileStream;
  if (!fileStream.ar_open( subjectDatabaseFilename, "", exptDirectory )) {
    cerr << "arHumanSubject error: couldn't open file " << exptDirectory << subjectDatabaseFilename << endl;
    errStream << "arHumanSubject error: couldn't open file " << exptDirectory << subjectDatabaseFilename << endl;
    return false;
  }

  // Setup dictionaries and parsers.
  // First we read in the header (which just confirms the field names and types)
  arDataTemplate headerTemplate( "subject_record_description" );
  headerTemplate.addAttribute( "field_names", AR_CHAR );
  headerTemplate.addAttribute( "field_types", AR_CHAR );
  arTemplateDictionary headerDict;
  headerDict.add( &headerTemplate );
  arStructuredDataParser headerParser( &headerDict );
  _headerData = headerParser.parse( &fileStream ); // does this need to be a class member?
  if (_headerData==0) {
    cerr << "arHumanSubject error: failed to parse subject database header.\n";
    errStream << "arHumanSubject error: failed to parse subject database header.\n";
    fileStream.ar_close();
    return false;
  }
  const string& recordName = _headerData->getName();
  if (recordName != "subject_record_description") {
    cerr << "arHumanSubject error: config file must begin\n"
         << "with a <subject_record_description> record.\n";
    fileStream.ar_close();
    return false;
  }
  // Verify that the list and types of subject parameters in config file are correct
  vector<string> fieldNameArray;
  if (!ar_extractTokenList( _headerData, "field_names", fieldNameArray, '|' )) {
    fileStream.ar_close();
    return false;
  }
  vector<string> fieldTypeArray;
  if (!ar_extractTokenList( _headerData, "field_types", fieldTypeArray, '|' )) {
    fileStream.ar_close();
    return false;
  }
  if (fieldNameArray.size() != fieldTypeArray.size()) {
    cerr << "arHumanSubject error: different numbers of fields listed\n"
         << "in <field_names> and <field_types> of database header.\n";
    fileStream.ar_close();
    return false;
  }
  if (fieldNameArray.size() != _parameters.size()) {
    cerr << "arHumanSubject error: numbers of parameters listed\n"
         << "in database header (" << fieldNameArray.size() << ") does\n"
         << "not match number required by program (" << _parameters.size() << ").\n";
    fileStream.ar_close();
    return false;
  }
  // Verify that all field names and types in the subject file header are correct
  ParamList_t::iterator iter;
  vector<string>::const_iterator stringIter;
  int i;
  for (iter = _parameters.begin(); iter != _parameters.end(); iter++) {
    stringIter = std::find_if( fieldNameArray.begin(), fieldNameArray.end(),
                      bind2nd( equal_to<string>(), iter->first ) );
    if (stringIter == fieldNameArray.end()) {
      cerr << "arHumanSubject error: parameter " << iter->first << " not found\n"
           << "in subjectdatabase header.\n";
      fileStream.ar_close();
      return false;
    }
    i = stringIter - fieldNameArray.begin();
    if (arDataNameType(fieldTypeArray[i].c_str()) != iter->second._type) {
      cerr << "arHumanSubject error: incorrect type (" << fieldTypeArray[i] << ")\n"
           << "for parameter " << iter->first << " in subject database header.\n"
           << "Should be " << arDataTypeName(iter->second._type) << endl;
      fileStream.ar_close();
      return false;
    }
  }
  
  // Now start looking for the correct record
  // construct record parser
  arDataTemplate subjectTemplate( "subject_record" );
  for (iter=_parameters.begin(); iter!=_parameters.end(); iter++)
    subjectTemplate.addAttribute( iter->first, AR_CHAR );    
  arTemplateDictionary subjectDict;
  subjectDict.add( &subjectTemplate );
  arStructuredDataParser subjectParser( &subjectDict );
  
  // parse
  while (true) { // Loop until we find it or run out of records
    _subjectRecord = subjectParser.parse( &fileStream ); // does this need to be a class member?
    if (_subjectRecord==0) {
      cerr << "arHumanSubject error: No record found for subject "
           << subjectLabel << endl;
      errStream << "arHumanSubject error: No record found for subject "
           << subjectLabel << " in the subject database.\n";
      fileStream.ar_close();
      return false;
    }
    // Verify that subject and experiment names in config file are correct
    string subjID;
    if (!ar_getStringField( _subjectRecord, "label", subjID )) {
      cerr << "arHumanSubject error: failed to read <label> field.\n";
      subjectParser.recycle( _subjectRecord );
      _subjectRecord = 0;
      fileStream.ar_close();
      return false;
    }
    if (subjID == subjectLabel) {
      fileStream.ar_close();
      goto RecordFound;
    }
  }
RecordFound:
  // correct record found, validate parameter values
  for (iter = _parameters.begin(); iter != _parameters.end(); iter++) {
    if (!ar_getStringField( _subjectRecord, iter->first, iter->second._value )) {
      cerr << "arHumanSubject error: failed to extract " << iter->first << " parameter.\n";
      return false;
    }
    if (!validateParameter( iter )) {
      return false;
    }
  }
  _configured = true;
  return true;
}

bool arHumanSubject::validateParameter( ParamList_t::iterator iter ) {
  ParamVal_t val = iter->second;
  long lTemp;
  int iTemp;
  double dTemp;
  float fTemp;
  switch (val._type) {
    case AR_CHAR:
      break;
    case AR_LONG:
      if (!ar_stringToLongValid( val._value, lTemp )) {
        cerr << "arHumanSubject error: string->long conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    case AR_INT:
      if (!ar_stringToLongValid( val._value, lTemp )) {
        cerr << "arHumanSubject error: string->int conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }      
      if (!ar_longToIntValid( lTemp, iTemp )) {
        cerr << "arHumanSubject error: string->int conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    case AR_DOUBLE:
      if (!ar_stringToDoubleValid( val._value, dTemp )) {
        cerr << "arHumanSubject error: string->double conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    case AR_FLOAT:
      if (!ar_stringToDoubleValid( val._value, dTemp )) {
        cerr << "arHumanSubject error: string->float conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }      
      if (!ar_doubleToFloatValid( dTemp, fTemp )) {
        cerr << "arHumanSubject error: string->float conversion failed for\n"
             << "parameter " << iter->first << endl;
        return false;
      }
      break;
    default:
      cerr << "arHumanSubject error: unknown type for\n"
             << "parameter " << iter->first << endl;
      return false;
      break;
  }
  return true;
}
 
