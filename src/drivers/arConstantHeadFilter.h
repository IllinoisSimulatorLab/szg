//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CONSTHEAD_FILTER
#define AR_CONSTHEAD_FILTER

#include "arIOFilter.h"
#include "arDriversCalling.h"

// Fixed head position, for multi-person nonheadtracked demos.

class SZG_CALL arConstantHeadFilter: public arIOFilter{
  public:
    arConstantHeadFilter() {}
    virtual ~arConstantHeadFilter() {}

  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
};

#endif
