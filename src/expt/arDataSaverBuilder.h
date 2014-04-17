#ifndef ARDATASAVERBUILDER_H
#define ARDATASAVERBUILDER_H

//#include "arSZGClient.h"
#include "arDataSaver.h"
#include <string>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arExperimentCalling.h"

class SZG_CALL arDataSaverBuilder {
  public:
    arDataSaver* build( const std::string dataStyle );
//    arDataSaver* build( arSZGClient& SZGClient );
};

#endif        //  #ifndefARDATASAVERBUILDER_H

