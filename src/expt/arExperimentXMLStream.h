#ifndef AREXPERIMENTXMLSTREAM_H
#define AREXPERIMENTXMLSTREAM_H

#include "arExperimentDataRecord.h"
#include "arStructuredDataParser.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arExperimentXMLStream {
  public:
    arExperimentXMLStream( arTextStream* instream );
    ~arExperimentXMLStream();

    bool setTextStream( arTextStream* instream );
    bool addRecordType( arExperimentDataRecord& rec );
    arExperimentDataRecord* ar_read();
    bool fail() const { return _fail; }

  private:
    arExperimentXMLStream();
    arExperimentXMLStream( const arExperimentXMLStream& x );
    arExperimentXMLStream& operator=( const arExperimentXMLStream& x );
    
    typedef std::map< std::string, arExperimentDataRecord > arRecTypeMap_t;

    arTextStream* _instream;
    arRecTypeMap_t _recordTypeMap;
    arTemplateDictionary _dictionary;
    arStructuredDataParser* _parser;
    arStructuredData *_data;
    bool _fail;
};

SZG_CALL arExperimentXMLStream& operator>>( arExperimentXMLStream& input, arExperimentDataRecord& record );

#endif // AREXPERIMENTXMLSTREAM_H
