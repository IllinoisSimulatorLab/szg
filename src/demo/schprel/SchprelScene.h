#ifndef __SCHPRELSCENE_H__
#define __SCHPRELSCENE_H__

/* 
 * SchprelScene.h
 */

#include "SpecialRelativityMath.h"
#include "arGraphicsHeader.h"
#include "arGraphicsAPI.h"

/** basic scene file object for schprel
 * to allow them to be changed on the fly
 * (C++ abstraction and all...)
*/
class SchprelScene
{
  public:
    /// get ready for drawing
    virtual void initTheGL(void)
    {
      // This stuff is no longer necessary since it is taken care of
      // by the framework itself. In fact, including it here compromises
      // passive stereo display from a single window (where we want two views)
      // glEnable(GL_DEPTH_TEST);
      // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  
    }

    /// draw everything
    virtual void drawAll(void) = 0;

    /// update functions
    /// @param updateValues struct containing all relevent information
    virtual void updateAll(s_updateValues &updateValues) = 0;
};

#endif
