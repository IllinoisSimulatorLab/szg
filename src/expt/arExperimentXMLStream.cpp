#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arSTLalgo.h"
#include "arExperimentXMLStream.h"
#include <sstream>

arExperimentXMLStream::arExperimentXMLStream( arTextStream* instream ) :
  _instream( instream ),
  _parser(0),
  _data(0),
  _fail(false) {
}

arExperimentXMLStream::~arExperimentXMLStream() {
  _recordTypeMap.clear();
  if (_parser)
    delete _parser;
}

bool arExperimentXMLStream::setTextStream( arTextStream* instream ) {
  if (!instream) {
    cerr << "arExperimentXMLStream error: attempt to set arTextStream pointer to NULL.\n";
    return false;
  }
  _instream = instream;
  _fail = false;
  return true;
}

bool arExperimentXMLStream::addRecordType( arExperimentDataRecord& rec ) {
  if (_parser) {
    cerr << "arExperimentXMLStream error: attempt to addRecordType() after performing I/O.\n";
    return false;
  }
  std::string name = rec.getName();
  arRecTypeMap_t::iterator iter = _recordTypeMap.find( name );
  if (iter != _recordTypeMap.end()) {
    cerr << "arExperimentXMLStream error: addRecordType(" << name << ") when it already exists.\n";
    return false;
  }
  arDataTemplate* tmplate = new arDataTemplate( name );
  if (!tmplate) {
    cerr << "arExperimentXMLStream error: failed to allocate arDataTemplate.\n";
    return false;
  }
  arExperimentDataField *field;
  for (field = rec.getFirstField(); !!field; field = rec.getNextField()) {
    // NOTE: eventually we'll set all field types to CHAR here because we'll convert them ourselves (ugh)
//    tmplate->addAttribute( field.getName(), AR_CHAR );
    tmplate->addAttribute( field->getName(), field->getType() );
  }
  // I _think_ the dictionary owns the template after you add it.
  _dictionary.add( tmplate );
  _recordTypeMap.insert( arRecTypeMap_t::value_type( name, rec ) );
  return true;
}

struct arXMLStreamException {
  std::string _message; 
  arXMLStreamException( const std::string message ):_message(message) {}
  bool loaded() { return _message != ""; }
};

arExperimentDataRecord* arExperimentXMLStream::ar_read() {
  ostringstream exStream;
  try {
    if (!_instream) {
      exStream.str("");
      exStream << "arExperimentXMLStream error: arTextStream pointer = NULL.";
      throw arXMLStreamException( exStream.str() );
    }
    if (!_parser) {
      _parser = new arStructuredDataParser( &_dictionary );
      if (!_parser) {
        exStream.str("");
        exStream << "arExperimentXMLStream error: failed to allocate arStructuredDataParser.";
        throw arXMLStreamException( exStream.str() );
      }
    }
    arStructuredData* data = _parser->parse( _instream );
    if (!data) {
      throw arXMLStreamException( "" );
    }
    const string& recordName = data->getName();
    arRecTypeMap_t::iterator recIter = _recordTypeMap.find( recordName );
    if (recIter == _recordTypeMap.end()) {
      exStream.str("");
      exStream << "arExperimentXMLStream error: arStructuredData record type (" << recordName
                << ") does not match any stored type.";
      throw arXMLStreamException( exStream.str() );
    }
    arExperimentDataRecord& dataRecord = recIter->second;
    std::vector< std::string > fieldNames;
    data->getFieldNames( fieldNames );
    if (fieldNames.size() != dataRecord.getNumberFields()) {
      exStream.str("");
      exStream << "arExperimentXMLStream error: arStructuredData record # fields (" << fieldNames.size()
                << ") != stored # fields (" << dataRecord.getNumberFields() << ").";
      throw arXMLStreamException( exStream.str() );
    }
    std::vector< std::string>::iterator iter;
    for (iter = fieldNames.begin(); iter != fieldNames.end(); ++iter) {
      std::string fieldName = *iter;
      arDataType typ = data->getDataType( fieldName );
      if (!dataRecord.fieldExists( fieldName, typ )) {
        exStream.str("");
        exStream << "arExperimentXMLStream error: field(name,type) = (" << fieldName << ","
                  << typ << ") does not match stored fields.";
        throw arXMLStreamException( exStream.str() );
      }
      const void *const ptr = data->getDataPtr( fieldName, typ );
      int siz = data->getDataDimension( fieldName );
      if (siz < 0) {
        exStream.str("");
        exStream << "arExperimentXMLStream error: arStructuredData->getDataDimension( " << fieldName << " ) < 0.";
        throw arXMLStreamException( exStream.str() );
      }
      unsigned int usiz( siz );
//      if (typ == AR_CHAR) {
//        std::string tmp((char*)ptr);
//        if (tmp.substr(0,6) == "friggy") {
//          cerr << "SIZE FOR friggy = " << usiz << endl;
//        }
//      }
      if (!dataRecord.setFieldValue( fieldName, typ, ptr, usiz )) {
        exStream.str("");
        exStream << "arExperimentXMLStream error: setFieldValue( " << fieldName << ", " << typ << ", "
                   << ptr << ", " << siz << " ) failed.";
        throw arXMLStreamException( exStream.str() );
      } 
    }
    _parser->recycle( data );
    arExperimentDataRecord* val = new arExperimentDataRecord;
    *val = dataRecord;
    return val;
  }
  catch (arXMLStreamException ex) {
    if (ex.loaded())
      cerr << ex._message << endl;
    _fail = true;
    return (arExperimentDataRecord*)0;
  }
}

arExperimentXMLStream& operator>>( arExperimentXMLStream& instream, arExperimentDataRecord& record ) {
  if (instream.fail())
    return instream;
  arExperimentDataRecord* rec = instream.ar_read();
  if (!!rec) {
    record = *rec;
    delete rec;
  }
  return instream;
}

