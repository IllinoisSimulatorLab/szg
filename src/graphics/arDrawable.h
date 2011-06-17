#ifndef ARDRAWABLE_H
#define ARDRAWABLE_H

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arDrawable {
  public:
    virtual ~arDrawable() {}
    virtual void draw()=0;
};

#endif        //  #ifndefARDRAWABLE_H


