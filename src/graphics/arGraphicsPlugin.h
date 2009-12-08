//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_PLUGIN_H
#define AR_GRAPHICS_PLUGIN_H

#include "arGraphicsWindow.h"
#include "arViewport.h"
#include "arGraphicsCalling.h"

class SZG_CALL arGraphicsPlugin {
  public:
    arGraphicsPlugin() {}
    virtual ~arGraphicsPlugin() {}
    virtual void draw( arGraphicsWindow& /*win*/, arViewport& /*view*/ ) {}
    virtual bool setState(
      vector<int>& /*intData*/,
      vector<long>& /*longData*/,
      vector<float>& /*floatData*/,
      vector<double>& /*doubleData*/,
      vector< string >& /*stringData*/ )
        { return false; }
};

#endif
