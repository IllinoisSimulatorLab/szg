#ifndef ARHUMANSUBJECT_H
#define ARHUMANSUBJECT_H

#include <string>
#include <map>
#include <vector>
#include "arDataType.h"
#include "arStructuredData.h"
#include "arSZGClient.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arHumanSubject {
  public:
    arHumanSubject();
    virtual ~arHumanSubject();
    bool init( string exptDirectory, arSZGClient& SZGClient );
    bool addParameter( const std::string fname, const arDataType theType );
    bool getParameterValue( const std::string fname, const arDataType theType, void* address );
    bool getParameterType( const std::string fname, arDataType& theType ) const;
    unsigned int getParameterSize( const std::string fname ) const;
    std::string getParameterString( const std::string fname ) const;
    bool getParameterNames( std::vector< std::string >& nameList ) const;
    const arStructuredData* getHeaderRecord() const;
    const arStructuredData* getSubjectRecord() const;
    const string getLabel() const;

    bool getStringParameter( const std::string name, std::string& value );
//    bool getIntParameter( const std::string name, int& value );
//    bool getLongParameter( const std::string name, long& value );
//    bool getFloatParameter( const std::string name, float& value );
//    bool getDoubleParameter( const std::string name, double& value );
    
  private:
    typedef struct {
      arDataType _type;
      string _value;
    } ParamVal_t;
    typedef std::map< std::string, ParamVal_t, less<std::string> > ParamList_t;
    bool _configured;
    arStructuredData* _headerData;
    arStructuredData* _subjectRecord;
    ParamList_t _parameters;
    bool validateParameter( ParamList_t::iterator iter );
};
  
#endif        //  #ifndefARHUMANSUBJECT_H

