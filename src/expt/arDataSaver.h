#ifndef ARDATASAVER_H
#define ARDATASAVER_H

#include <vector>
#include <string>
#include <map>
#include "arDataType.h"
#include "arStructuredData.h"
#include "arSZGClient.h"
#include "arExperimentDataRecord.h"
#include "arHumanSubject.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arDataSaver {
  public:
    arDataSaver( const std::string& fileSuffix="" ) :
      _fileSuffix( fileSuffix ),
      _dataFilePath("") {}
    virtual ~arDataSaver() {}
    // should set _configured to true;
    virtual bool init( const std::string experimentName,
                       std::string dataPath, std::string comment,
                       const arHumanSubject& subjectData,
                       arExperimentDataRecord& factors,
                       arExperimentDataRecord& dataRecords,
                       arSZGClient& SZGClient)=0;
    virtual bool saveData( arExperimentDataRecord& factors,
                           arExperimentDataRecord& dataRecords )=0;
    void setFileSuffix( const std::string& fileSuffix ) {
      _fileSuffix = fileSuffix;
    }
    std::string getFileSuffix() const { return _fileSuffix; }
    virtual bool setFilePath( std::string& dataPath, arSZGClient& szgClient );
    std::string getFilePath() const { return _dataFilePath; }
   protected:
      std::string _fileSuffix;
      std::string _dataFilePath;
};

typedef std::map< std::string,arDataSaver* > arDataSaverMap_t;

#endif        //  #ifndefARDATASAVER_H

