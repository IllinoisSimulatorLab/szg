//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CONSTHEAD_FILTER
#define AR_CONSTHEAD_FILTER

#include "arIOFilter.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"

/// Fixed head position (for multi-person demos in a headtracked setup).

class SZG_CALL arConstantHeadFilter: public arIOFilter{
  public:
    arConstantHeadFilter(){}
    virtual ~arConstantHeadFilter(){}

  protected:
    virtual bool _processEvent( arInputEvent& inputEvent );
};

#endif

