//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_PLUGIN_H
#define AR_GRAPHICS_PLUGIN_H

#include "arGraphicsWindow.h"
#include "arViewport.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"


class SZG_CALL arGraphicsPlugin {
  public:
    arGraphicsPlugin() {}
    virtual ~arGraphicsPlugin() {}
    virtual void draw( arGraphicsWindow& /*win*/, arViewport& /*view*/ ) {}
    virtual bool setState( std::vector<int>& /*intData*/,
                           std::vector<long>& /*longData*/,
                           std::vector<float>& /*floatData*/,
                           std::vector<double>& /*doubleData*/,
                           std::vector< std::string >& /*stringData*/ ) { return false; }
};

#endif
