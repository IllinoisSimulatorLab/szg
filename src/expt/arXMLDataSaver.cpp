#include "arPrecompiled.h"
#include "arXMLDataSaver.h"
#include <sstream>

using std::vector;
using std::string;

arXMLDataSaver::arXMLDataSaver() :
  arDataSaver("xml"),
  _configured(false),
  _template("trial_data"),
  _structuredData(0) {
}

arXMLDataSaver::~arXMLDataSaver() {
  if (_structuredData!=0)
    delete _structuredData;
}

bool arXMLDataSaver::init( const string experimentName,
                           string dataPath, string comment,
                           const arHumanSubject& subjectData,
                           arExperimentDataRecord& factors,
                           arExperimentDataRecord& dataRecords,
                           arSZGClient& szgClient) {
  stringstream& errStream = szgClient.startResponse();
  if (!setFilePath( dataPath, szgClient )) {
    cerr << "arXMLDataSaver error: setFilePath() failed.\n";
    errStream << "arXMLDataSaver error: setFilePath() failed.\n";
    return false;
  }
  
  // Setup data template for data file header
  // NOTE: at some point should probably add hooks so user can add optional fields
  arDataTemplate headerTemplate( "experiment_description" );
  headerTemplate.addAttribute( "experiment_name", AR_CHAR );
  headerTemplate.addAttribute( "runtime", AR_CHAR );
  headerTemplate.addAttribute( "comment", AR_CHAR );
  
  // Pass 1: have a separate field in the header with the type of each parameter
  // (parameter name suffixed with _type)
//  for (i=dataRecords.begin(); i!=dataRecords.end(); i++)
//    headerTemplate.addAttribute( i->_name + "_type", AR_CHAR );
    
  // Pass 2: have a 4 fields in the header for parameters & data fields.  First field lists names
  // (separated by |, I think), second lists types (also |-delimited).
  // Advantage: Header record format doesn't vary from one experiment to the next
  // (which would require a separate executable to parse each experiment's data),
  // but header still contains all info needed to parse data records. Also more compact.
  // Disadvantage: field has to be parsed when data is analysed.
  headerTemplate.addAttribute( "factor_names", AR_CHAR );
  headerTemplate.addAttribute( "factor_types", AR_CHAR );    
  headerTemplate.addAttribute( "data_names", AR_CHAR );
  headerTemplate.addAttribute( "data_types", AR_CHAR );    
    
  // Setup arStructuredData for file header
  arStructuredData* headerData = new arStructuredData( &headerTemplate );
  if (!headerData) {
    cerr << "headerData error: failed to construct header arStructuredData.\n";
    return false;
  }
  headerData->dataIn( "experiment_name", (void*)experimentName.c_str(), AR_CHAR, experimentName.size());  
  string theTime = ar_currentTimeString();
  headerData->dataIn( "runtime", (void*)theTime.c_str(), AR_CHAR, theTime.size());
  headerData->dataIn( "comment", (void*)comment.c_str(), AR_CHAR, comment.size());
  string nameString, typeString;

  arExperimentDataField* df = factors.getFirstField();
  arExperimentDataField* first = df;
  while (!!df) {
    if (df != first) {
      nameString = nameString + "|"; // add vertical bar deilimiter
      typeString = typeString + "|";
    }
    nameString = nameString + df->getName();
    typeString = typeString + arDataTypeName(df->getType());
    df = factors.getNextField();
  }
  headerData->dataIn( "factor_names", (void*)nameString.c_str(), AR_CHAR, nameString.size());
  headerData->dataIn( "factor_types", (void*)typeString.c_str(), AR_CHAR, typeString.size());
  nameString = "";
  typeString = "";
  df = dataRecords.getFirstField();
  first = df;
  while (!!df) {
    if (df != first) {
      nameString = nameString + "|"; // add vertical bar deilimiter
      typeString = typeString + "|";
    }
    nameString = nameString + df->getName();
    typeString = typeString + arDataTypeName(df->getType());
    df = dataRecords.getNextField();
  }
  headerData->dataIn( "data_names", (void*)nameString.c_str(), AR_CHAR, nameString.size());
  headerData->dataIn( "data_types", (void*)typeString.c_str(), AR_CHAR, typeString.size());

  arStructuredData* subjectHeader = const_cast<arStructuredData*>(subjectData.getHeaderRecord());
  arStructuredData* subjectRecord = const_cast<arStructuredData*>(subjectData.getSubjectRecord());
  
  if ((subjectHeader == 0)||(subjectRecord == 0)) {
    cerr << "arXMLDataSaver error: NULL subject data.\n";
    delete headerData;
    return false;
  }
  
  df = factors.getFirstField();
  while (!!df) {
    _template.addAttribute( df->getName(), df->getType() );
    df = factors.getNextField();
  }
  df = dataRecords.getFirstField();
  while (!!df) {
    _template.addAttribute( df->getName(), df->getType() );
    df = dataRecords.getNextField();
  }
  _structuredData = new arStructuredData(&_template);
  if (!_structuredData) {
    cerr << "arXMLDataSaver error: failed to construct dataRecord arStructuredData.\n";
    delete headerData;
    return false;
  }
  
  FILE* filePtr = fopen( _dataFilePath.c_str(), "w" );
  if (filePtr == NULL) {
    cerr << "arXMLDataSaver error: couldn't open data file " << _dataFilePath << endl;
    errStream << "arXMLDataSaver error: couldn't open data file " << _dataFilePath << endl;
    delete headerData;
    return false;
  }
  headerData->print( filePtr );
  delete headerData;
  subjectHeader->print( filePtr );
  subjectRecord->print( filePtr );
  fclose( filePtr );
  
  _factors = factors;
  _dataFields = dataRecords;
  _configured = true;
  return true;
}

bool arXMLDataSaver::saveData( arExperimentDataRecord& newFactors, arExperimentDataRecord& newDataRecords ) {
  if (!_configured) {
    cerr << "arXMLDataSaver error: attempt to save data before init().\n";
    return false;
  }
  // validate data record names and types (must match EXACTLY)
  if (newDataRecords.getNumberFields() != _dataFields.getNumberFields()) {
    cerr << "arXMLDataSaver error: data records to save do not match template.\n";
    return false;
  }

  if (!_factors.matchNamesTypes( newFactors )) {
      cerr << "arXMLDataSaver error: saveData() failed.\n";
      return false;
  } 
  if (!_dataFields.matchNamesTypes( newDataRecords )) {
      cerr << "arXMLDataSaver error: saveData() failed.\n";
      return false;
  } 
  
  // copy factors & data fields into arStructuredData
  // NOTE: This is too inflexible.  Need to add some way for user to set
  // rules for converting numberic data to strings (controlling precision of output)
  int theSize;
//  cerr << "Saving Factors:\n";
  arExperimentDataField* df = newFactors.getFirstField();
  while (!!df) {
//    cerr << df-><< endl;
    theSize = df->getSize();
    if (df->getType() == AR_CHAR) {
//      const string temp( (char*)iNew->_address );
//      unsigned int len = temp.size();
      int len = strlen((char*)df->getAddress());
      if (len > df->getSize()) {
        cerr << "arXMLDataSaver warning: factor " << df->getName() << " has value ("
             << (char*)df->getAddress() << ") and length ("
             << len << ")\n  which is > specified limit (" << df->getSize() << ");\n"
             << "output will be truncated.\n";
      } else {
        theSize = len;
      }
    }
    if (!_structuredData->dataIn( df->getName(), df->getAddress(), df->getType(), theSize )) {
      cerr << "arXMLDataSaver error: failed to place record " << df->getName() << " in arStructuredData.\n";
      return false;
    }
    df = newFactors.getNextField();
  }
//  cerr << "Saving Data:\n";
  df = newDataRecords.getFirstField();
  while (!!df) {
//    cerr << df-> << endl;
    theSize = df->getSize();
    if (df->getType() == AR_CHAR) {
      int len = strlen((const char*)df->getAddress());
      if (len > df->getSize()) {
        cerr << "arXMLDataSaver warning: data field " << df->getName() << " length ("
             << len << ") > specified limit (" << df->getSize() << ");\n"
             << "output will be truncated.\n";
      } else
        theSize = len;
    }
    if (!_structuredData->dataIn( df->getName(), df->getAddress(), df->getType(), theSize )) {
      cerr << "arXMLDataSaver error: failed to place record " << df->getName() << " in arStructuredData.\n";
      return false;
    }
    df = newDataRecords.getNextField();
  }
  
  // Get the data to the file
  FILE* filePtr = fopen( _dataFilePath.c_str(), "a" );
  if (filePtr == NULL) {
    cerr << "arXMLDataSaver error: couldn't append to data file " << _dataFilePath << endl;
    return false;
  }
  _structuredData->print( filePtr );
  fclose( filePtr );

//  cerr << *_structuredData << endl;
  
  return true;
}

