#ifndef ARDUMMYDATASAVER_H
#define ARDUMMYDATASAVER_H

#include <vector>
#include <string>
#include "arDataType.h"
#include "arStructuredData.h"
#include "arSZGClient.h"
#include "arExperimentDataRecord.h"
#include "arHumanSubject.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arDummyDataSaver : public arDataSaver {
  public:
    virtual bool init( const std::string /*experimentName*/,
                       std::string /*dataPath*/, std::string /*comment*/,
                       const arHumanSubject& /*subjectData*/,
                       arExperimentDataRecord& /*factors*/,
                       arExperimentDataRecord& /*dataRecords*/,
                       arSZGClient& /*SZGClient*/) {
      return true;
    }
    virtual bool saveData( arExperimentDataRecord& /*factors*/,
                           arExperimentDataRecord& /*dataRecords*/ ) {
      return true;
    }
};

#endif        //  #ifndefARDUMMYDATASAVER_H

