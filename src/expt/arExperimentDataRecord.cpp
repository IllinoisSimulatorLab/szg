#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arExperimentDataRecord.h"
#include <iostream>
using std::cerr;

//bool arExperimentDataRecord::_printCompDiagnostics = false;
//bool arExperimentDataRecord::_useFieldFormat = false;
bool _EDRprintCompDiagnostics = false;
bool _EDRuseFieldFormat = false;

void arExperimentDataRecord::setCompDiagnostics(bool onoff) {
  _EDRprintCompDiagnostics = onoff;
}

void arExperimentDataRecord::setUseFieldPrintFormat(bool onoff) {
  _EDRuseFieldFormat = onoff;
}

arExperimentDataRecord::arExperimentDataRecord() :
  _name(""),
  _currentFieldName("") {
  _currentFieldIter = _dataFields.end();
}

arExperimentDataRecord::arExperimentDataRecord( const std::string& name ) :
  _name(name),
  _currentFieldName("") {
  _currentFieldIter = _dataFields.end();
}

arExperimentDataRecord::arExperimentDataRecord( const arExperimentDataRecord& x ) :
  _name(x._name),
  _fieldNames(x._fieldNames),
  _dataFields(x._dataFields),
  _currentFieldName(x._currentFieldName) {
  if (_currentFieldName.size() > 0) {
    _currentFieldIter = _dataFields.find( _currentFieldName );
  } else {
    _currentFieldIter = _dataFields.end();
  }
}

arExperimentDataRecord& arExperimentDataRecord::operator=( const arExperimentDataRecord& x ) {
  if (&x == this)
    return *this;
  _name = x._name;
  _fieldNames = x._fieldNames;
  _dataFields = x._dataFields;
  _currentFieldName = x._currentFieldName;
  if (_currentFieldName.size() > 0) {
    _currentFieldIter = _dataFields.find( _currentFieldName );
  } else {
    _currentFieldIter = _dataFields.end();
  }
  return *this;
}

arExperimentDataRecord::~arExperimentDataRecord() {
  _dataFields.clear();
}

bool arExperimentDataRecord::operator==( const arExperimentDataRecord& x ) {
  if (_name != x._name) {
    if (_EDRprintCompDiagnostics) {
      cerr << "arExperimentDataRecord operator==() remark: names('" << _name << "','" 
           << x._name << "') do not match.\n"; 
    }
    return false;
  }
  if (_dataFields.size() != x._dataFields.size()) {
    if (_EDRprintCompDiagnostics) {
      cerr << "arExperimentDataRecord operator==() remark: # data fields ('" 
           << _dataFields.size() << "','" 
           << x._dataFields.size() << "') do not match.\n"; 
    }
    return false;
  }
  arExperimentDataRecord* x2 = (arExperimentDataRecord*)&x;
  arNameDataMap_t::iterator iter;
  for (iter = _dataFields.begin(); iter != _dataFields.end(); ++iter) {
    arExperimentDataField& myField = iter->second;
    arExperimentDataField* yourField = x2->getField( myField.getName() );
    if (!yourField) {
      if (_EDRprintCompDiagnostics)
        cerr << "arExperimentDataRecord operator==() remark: no field " << myField.getName() << endl;
      return false;
    }
    if (myField != *yourField) {
      if (_EDRprintCompDiagnostics) {
        cerr << "arExperimentDataRecord operator==() remark: different values for field " 
             << myField.getName() << ":\n  '" << myField << "' != '" << *yourField << "'.\n";
      }
      return false;
    }
  }
  return true;
}

bool arExperimentDataRecord::operator!=( const arExperimentDataRecord& x ) {
  return !operator==(x);
}

void arExperimentDataRecord::setName( const std::string& name ) {
  _name = name;
}

std::string arExperimentDataRecord::getName() const {
  return _name;
}

bool arExperimentDataRecord::addField( const std::string& name, arDataType typ, 
                                    const void* address, unsigned int size, bool ownPtr ) {
  if (fieldExists( name )) {
    cerr << "arExperimentDataRecord error: attempt to add field " << name
         << " that already exists.\n";
    return false;
  }
  _fieldNames.push_back( name );
  _dataFields.insert( arNameDataMap_t::value_type( name,
                      arExperimentDataField( name, typ, address, size, ownPtr ) ) );
  return true;
}

bool arExperimentDataRecord::addField( const arExperimentDataField& field ) {
  if (fieldExists( field.getName() )) {
    cerr << "arExperimentDataRecord error: attempt to add field " << field.getName()
         << " that already exists.\n";
    return false;
  }
  _fieldNames.push_back( field.getName() );
  _dataFields.insert( arNameDataMap_t::value_type( field.getName(), field ) );
  return true;
}

bool arExperimentDataRecord::setField( const arExperimentDataField& field ) {
  // setData() below checks that types match, so we don't need to here.
  arExperimentDataField* df = getField( field.getName() );
  if (!df) {
    cerr << "arExperimentDataRecord error: failed to find field " << field.getName()
         << " in  setField().\n";
    return false;
  }
  return df->setData( field.getType(), field.getAddress(), field.getSize() );
}

arExperimentDataField* arExperimentDataRecord::getField( const std::string& name ) {
  arNameDataMap_t::iterator iter = _dataFields.find( name );
  if (iter == _dataFields.end()) {
    cerr << "arExperimentDataRecord error: field " << name << " not found in getField().\n";
    return (arExperimentDataField*)0;
  }
  return &iter->second;
}

arExperimentDataField* arExperimentDataRecord::getField( const std::string& name, arDataType typ ) {
  arExperimentDataField* df = getField( name );
  if (!df) {
    return df;
  }
  if (df->getType() != typ) {
    cerr << "arExperimentDataRecord error: attempt to get field " << name
         << " using a field of the wrong type:" << endl
         << "     Should be " << arDataTypeName( df->getType() )
         << ", not " << arDataTypeName( typ ) << endl;
    return (arExperimentDataField*)0;
  }
  return df;
}

arExperimentDataField* arExperimentDataRecord::getFirstField() {
  if (_dataFields.empty()) {
    cerr << "arExperimentDataRecord error: getFirstField() called with empty dataset.\n";
    return (arExperimentDataField*)0;
  }
  _currentFieldIter = _dataFields.begin();
  return &(_currentFieldIter->second);
}

arExperimentDataField* arExperimentDataRecord::getNextField() {
  if (_dataFields.empty()) {
    cerr << "arExperimentDataRecord error: getNextField() called with empty dataset.\n";
    return (arExperimentDataField*)0;
  }
  if (_currentFieldIter == _dataFields.end()) {
    return (arExperimentDataField*)0;
  }
  ++_currentFieldIter;
  if (_currentFieldIter == _dataFields.end()) {
    return (arExperimentDataField*)0;
  }
  return &(_currentFieldIter->second);
}

bool arExperimentDataRecord::setFieldValue( const std::string& name, arDataType typ, 
                                         const void* const address, unsigned int size ) {
  // setData() below checks that types match, so we don't need to here.
  arExperimentDataField* df = getField( name );
  if (!df) {
    cerr << "arExperimentDataRecord error: failed to find field " << name
         << " in setFieldValue().\n";
    return false;
  }
  bool stat = df->setData( typ, address, size );
  if (!stat) {
    cerr << "arExperimentDataRecord error: setFieldValue() failed.\n";
  }
  return stat;
}

bool arExperimentDataRecord::setStringFieldValue( const std::string& name, const std::string& value ) {
  return setFieldValue( name, AR_CHAR, (const void* const)value.c_str(), value.size() );
}

bool arExperimentDataRecord::getStringFieldValue( const std::string& name, std::string& value ) {
  unsigned int size;
  char* ptr = (char*)getFieldAddress( name, AR_CHAR, size );
  if (!ptr) {
    cerr << "arExperimentDataRecord error: getFieldAddress failed in getStringFieldValue().\n";
    return false;
  }
  value = std::string(ptr);
  return true;
}

void* arExperimentDataRecord::getFieldAddress( const std::string& name, arDataType typ, 
                                            unsigned int& size ) {
  arExperimentDataField* df = getField( name, typ );
  if (!df) {
    cerr << "arExperimentDataRecord error: getFieldAddress() failed.\n";
    size = 0;
    return (void*)0;
  }
  size = df->getSize();
  return df->getAddress();
}

unsigned int arExperimentDataRecord::getNumberFields() {
  return _dataFields.size();
}

bool arExperimentDataRecord::fieldExists( const std::string& name ) {
  return _dataFields.find( name ) != _dataFields.end(); 
}

bool arExperimentDataRecord::fieldExists( const std::string& name, arDataType typ ) {
  arNameDataMap_t::iterator iter = _dataFields.find( name );
  if (iter == _dataFields.end()) {
    return false;
  }
  if (iter->second.getType() != typ) {
    return false;
  }
  return true;
}

bool arExperimentDataRecord::matchNamesTypes( arExperimentDataRecord& x ) {
  arExperimentDataField* df = getFirstField();
  arExperimentDataField* df2 = x.getFirstField();
  while (!!df) {
    if (!df2) {
//      cerr << "arExperimentDataRecord error: matchNamesTypes() found too few fields in comparison set.\n";
      return false;
    }
    if (df->getName() != df2->getName()) {
//      cerr << "arExperimentDataRecord error: field name " << df->getName()
//           << " does not match " << df2->getName() << endl;
      return false;
    }
    if (df->getType() != df2->getType()) {
//      cerr << "arExperimentDataRecord error: type of field name " << df->getName() 
//           << " (" << arDataTypeName(df->getType()) << ")\n"
//           << "     does not match that in comparison set (" << df2->getType() << ").\n";
      return false;
    }
    df = getNextField();
    df2 = x.getNextField();
  }
  if (!!df2) {
//    cerr << "arExperimentDataRecord error: matchNamesTypes() found too many fields in comparison set.\n";
    return false;
  }
  return true;
}

ostream& operator<<(ostream& s, const arExperimentDataRecord& d) {
  s << "<" << d.getName() << ">\n";
  std::vector< std::string >* fieldNames = (std::vector< std::string >*)&d._fieldNames;
  std::vector< std::string >::iterator iter;
  arExperimentDataRecord* d2 = (arExperimentDataRecord*)&d;
  for (iter = fieldNames->begin(); iter != fieldNames->end(); ++iter) {
    arExperimentDataField* field = d2->getField( *iter );
    if (!field) {
      cerr << "operator<<( arExperimentDataRecord ) error: field '" << *iter << "' not found.\n";
      return s;
    }
    if (_EDRuseFieldFormat) {
      s << *field << endl;
    } else {
      std::string name = field->getName();
      arDataType typ = field->getType();
      unsigned int siz = field->getSize();
      void* const add = field->getAddress();

      s << "  <" << name << ">";
      if (siz > 0) {
        unsigned int i;
        int* p;
        long* q;
        float* r;
        double* t;
        char* u;
        switch (typ) {
          case AR_INT: 
            p = static_cast<int*>(add);
            for (i=0; i<siz; ++i) {
              s << p[i];
              if (i != siz-1) {
                s << " ";
              }
            }
            break;
          case AR_LONG: 
            q = static_cast<long*>(add);
            for (i=0; i<siz; ++i) {
              s << q[i];
              if (i != siz-1) {
                s << " ";
              }
            }
            break;
          case AR_FLOAT: 
            r = static_cast<float*>(add);
            for (i=0; i<siz; ++i) {
              s << r[i];
              if (i != siz-1) {
                s << " ";
              }
            }
            break;
          case AR_DOUBLE: 
            t = static_cast<double*>(add);
            for (i=0; i<siz; ++i) {
              s << t[i];
              if (i != siz-1) {
                s << " ";
              }
            }
            break;
          case AR_CHAR: 
            u = static_cast<char*>(add);
            for (i=0; (i<siz)&&(u[i]!='\0'); ++i) {
              s << u[i];
            }
            break;
          default:
            ;
        }
      }
      s << "</" << name << ">\n";
    }
  }
  s << "</" << d.getName() << ">\n\n";
  return s;
}


