#ifndef ARXMLDATASAVER_H
#define ARXMLDATASAVER_H

#include "arDataSaver.h"
#include "arStructuredData.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arXMLDataSaver : public arDataSaver {
  public:
    arXMLDataSaver();
    virtual ~arXMLDataSaver();
    virtual bool init( const std::string experimentName,
                       std::string dataPath, std::string comment,
                       const arHumanSubject& subjectData,
                       arExperimentDataRecord& factors,
                       arExperimentDataRecord& dataRecords,
                       arSZGClient& SZGClient);
    virtual bool saveData( arExperimentDataRecord& factors, 
                           arExperimentDataRecord& dataRecords );
  private:
    bool _configured;
    arDataTemplate _template;
    arStructuredData* _structuredData;
    std::string _dataPath;
    arExperimentDataRecord _factors;
    arExperimentDataRecord _dataFields;
};

#endif        //  #ifndefARXMLDATASAVER_H

