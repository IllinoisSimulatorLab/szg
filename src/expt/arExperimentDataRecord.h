#ifndef AREXPERIMENTDATARECORD_H
#define AREXPERIMENTDATARECORD_H

#include "arExperimentDataField.h"
#include <string>
#include <vector>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arExperimentDataRecord {
  friend SZG_CALL std::ostream& operator<<(std::ostream&, const arExperimentDataRecord&);
  public:
    arExperimentDataRecord();
    arExperimentDataRecord( const std::string& name );
    arExperimentDataRecord( const arExperimentDataRecord& x );
    arExperimentDataRecord& operator=( const arExperimentDataRecord& x );
    ~arExperimentDataRecord();
    bool operator==( const arExperimentDataRecord& x );
    bool operator!=( const arExperimentDataRecord& x );

    void setName( const std::string& name );
    std::string getName() const;
    bool addField( const std::string& name, arDataType typ, 
                   const void* address=0, unsigned int size=0, bool ownPtr=false );
    bool addField( const arExperimentDataField& field );

    bool setField( const arExperimentDataField& field );
    arExperimentDataField* getField( const std::string& name );
    arExperimentDataField* getField( const std::string& name, arDataType typ );
    arExperimentDataField* getFirstField();
    arExperimentDataField* getNextField();
    
    bool setFieldValue( const std::string& name, arDataType typ, const void* const address, unsigned int size=1 ); 
    bool setStringFieldValue( const std::string& name, const std::string& value );
    bool getStringFieldValue( const std::string& name, std::string& value );
    void* getFieldAddress( const std::string& name, arDataType typ, unsigned int& size );

    unsigned int getNumberFields();
    std::vector< std::string > getFieldNames() const { return _fieldNames; }

    bool fieldExists( const std::string& name );
    bool fieldExists( const std::string& name, arDataType typ );

    bool matchNamesTypes( arExperimentDataRecord& x );

    static void setCompDiagnostics(bool onoff);
    static void setUseFieldPrintFormat(bool onoff);
//    static void setCompDiagnostics(bool onoff) { _printCompDiagnostics = onoff; }
//    static void setUseFieldPrintFormat(bool onoff) { _useFieldFormat = onoff; }

  private:
    std::string _name;
    std::vector< std::string > _fieldNames;
    arNameDataMap_t _dataFields;
    arNameDataMap_t::iterator _currentFieldIter;
    std::string _currentFieldName;
//    static bool _printCompDiagnostics;
//    static bool _useFieldFormat;
};

SZG_CALL std::ostream& operator<<(std::ostream&, const arExperimentDataRecord&);

#endif        //  #ifndefAREXPERIMENTDATARECORD_H

