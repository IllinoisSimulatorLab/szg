#ifndef __SCHPRELOBJECTSCENE_H__
#define __SCHPRELOBJECTSCENE_H__

/* 
 * SchprelObjectScene.h
 */

#include "SchprelScene.h"
#include "arObject.h"

/**
 * Uses an arObject as a Schprel Scene
 * (Not implemented yet)
*/
class SchprelObjectScene : public SchprelScene
{
  public:
    /// draw everything
    virtual void drawAll(void);

    /// update functions
    virtual void updateAll(float v, float theGamma);

  private:
    arObject *_object;
    int      _vertexNode;
};

#endif
