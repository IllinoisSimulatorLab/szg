#ifndef __SPACEJUNK_H__
#define __SPACEJUNK_H__

/*
 *
 * SpaceJunk.h -- scene file for schprel
 *
 */

#include "SchprelScene.h"

/**
 * "Orange pillars and cage" scene file for SchpRel
 */
class SpaceJunk : public SchprelScene
{
public:
    // get ready for drawing
    //void initTheGL(void);
    SpaceJunk();
		
    /// draw everything
    void drawAll(void);

    // update functions
    void updateAll(s_updateValues &updateValues);

    void drawfloor();
    void drawpillars();
    void drawceiling();
    void drawbeams();
    void drawtris();

private:
    float flod[1001][3];       // floor
    float flodraw[1001][3];    // drawable floor
    float ceid[1001][3];       // ceiling
    float ceidraw[1001][3];    // drawable ceiling
    float beam[16][30][3];     // "cage"
    float beamdraw[16][30][3]; // drawable "cage"
    float pillars[40][40][3];  // orange pillars
    float pilldraw[40][40][3]; // drawable orange pillars
    float trib[16][5][3];      // floor triangles
    float tribdraw[16][5][3];  // drawable floor triangles
    float trit[16][5][3];      // ceiling triangles
    float tritdraw[16][5][3];  // drawable ceiling triangles

};

#endif
