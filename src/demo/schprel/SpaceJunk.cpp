/*
 *  spacejunk.c
 *  schprel
 *
 *  Created by mflider on Mon Oct 16 2000.
 *  Copyright (c) 2000 Mark Flider. All rights reserved.
 *
 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "SpaceJunk.h"
#include <stdio.h>
#include <stdlib.h>

#include "SpecialRelativityMath.h"

static int wVert, aa, bb, dd;
static float vex, vey, vez;

/* Initialize vertex positions for all objects in Scene */
SpaceJunk::SpaceJunk()
{
   wVert=0;
   for (vez=-25;vez<=25;vez++)  /* initialize floor matrix */
   for (vex=-25;vex<=25;vex++)
   if ((int)vex % 5 == 0 || (int)vez % 5 == 0)
   {
        flod[wVert][0] = vex;
        flod[wVert][1] = -5;
        flod[wVert][2] = vez;
        wVert++;
   }
   for (aa=0;aa<1001;aa++)
   for (bb=0;bb<3;bb++)
        flodraw[aa][bb] = flod[aa][bb];

   for (aa=0;aa<1001;aa++)                /* init ceiling matrix */
   {
      ceid[aa][0] = ceidraw[aa][0] = flodraw[aa][0];
      ceid[aa][1] = ceidraw[aa][1] = 4;
      ceid[aa][2] = ceidraw[aa][2] = flodraw[aa][2];
   }

   for (aa=0;aa<11;aa++) {     /* init pillars matrix */
      wVert=0;
      for (vey=0;vey<10;vey++)
      for (vex=-.25;vex<=.25;vex+=.5)
      for (vez=-.25;vez<=.25;vez+=.5)
      {
         pillars[aa][wVert][0] = vex-25;
         pillars[aa][wVert][1] = vey-5;
         pillars[aa][wVert][2] = vez-25+aa*5;
         wVert++;
      }
   }
   for (aa=0;aa<11;aa++)
   for (bb=0;bb<40;bb++)
   for (dd=0;dd<3;dd++)
      pilldraw[aa][bb][dd] = pillars[aa][bb][dd];


   vey = 0;                /* init trit, trib matrices */
   aa=0;
   for (bb=-2;bb<=2;bb++)
   for (dd=-2;dd<=2;dd++)
   {
      if ( fabs((double)bb)==2 || fabs((double)dd)==2 )
      {
         trib[aa][0][0] = bb;
         trib[aa][0][1] = -4.9;
         trib[aa][0][2] = dd;
         wVert=1;
         for (vex=-.09;vex<=.09;vex+=.18)
         for (vez=-.09;vez<=.09;vez+=.18)
         {
            trib[aa][wVert][0] = vex+bb;
            trib[aa][wVert][1] = -5;
            trib[aa][wVert][2] = vez+dd;
            wVert++;
         }
         aa++;
      }
   }

   for (aa=0;aa<16;aa++)
   for (wVert=0;wVert<5;wVert++)
   {
      trit[aa][wVert][0] = trib[aa][wVert][0];
      trit[aa][wVert][1] = -1.0-trib[aa][wVert][1];
      trit[aa][wVert][2] = trib[aa][wVert][2];
   }
   for (aa=0;aa<16;aa++)
   for (wVert=0;wVert<5;wVert++)
   for (bb=0;bb<3;bb++)
   {
      tribdraw[aa][wVert][bb] = trib[aa][wVert][bb];
      tritdraw[aa][wVert][bb] = trit[aa][wVert][bb];
   }

   for (aa=0;aa<16;aa++)                /* init beam matrix */
   {
   wVert = 0;
   for (vey = tribdraw[aa][0][1];vey<=tritdraw[aa][0][1];vey+=(tritdraw[aa][0][1]-tribdraw[aa][0][1])/29)
      {
         beamdraw[aa][wVert][0] = beam[aa][wVert][0] = tribdraw[aa][0][0];
         beamdraw[aa][wVert][1] = beam[aa][wVert][1] = vey;
         beamdraw[aa][wVert][2] = beam[aa][wVert][2] = tribdraw[aa][0][2];
         wVert++;
      }
   }

   //initTheGL();
}


/***** --- draw functions down here --*****/

/// Draw the floor
void SpaceJunk::drawfloor(void){

  float color = 1.;

  for (aa=0;aa<1001;aa+=95){
    glBegin(GL_LINE_STRIP);
    glColor4f(0,color/2+.5,color/2+.5,1);
    for (wVert=aa;wVert<51+aa;wVert++)
      glVertex3f(flodraw[wVert][0],flodraw[wVert][1]+.01,flodraw[wVert][2]);
    glEnd();
  }
  for (aa=0;aa<51;aa+=5){
    glBegin(GL_LINE_STRIP);
    glColor4f(0,color/2+.5,color/2+.5,1);
    glVertex3f(flodraw[aa][0],flodraw[aa][1]+.01,flodraw[aa][2]);
    for (bb=51+aa/5;bb<1001;bb+=95){
      for (wVert=bb;wVert<11*4+bb;wVert+=11)
        glVertex3f(flodraw[wVert][0],flodraw[wVert][1]+.01,flodraw[wVert][2]);
      glVertex3f(flodraw[wVert+aa/5*4][0],flodraw[wVert+aa/5*4][1]+.01,flodraw[wVert+aa/5*4][2]);
    }
    glEnd();
  }

  for (dd=0;dd<950;dd+=95)
  for (aa=dd;aa<dd+50;aa+=5){
    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0,color,color,.2);
    wVert=aa+1;
    glVertex3fv(flodraw[aa]);
    for (bb=aa+51-(aa-dd)/5*4;bb<aa+85-(aa-dd)/5*4;bb+=11){
      glVertex3fv(flodraw[wVert]);
      glVertex3fv(flodraw[bb]);
      wVert++;
    }
    glVertex3fv(flodraw[wVert]);
    glVertex3fv(flodraw[aa+95]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0,color,color,.2);
    wVert=aa+95;
    glVertex3fv(flodraw[aa+5]);
    for (bb=aa+56-(aa+5-dd)/5*4;bb<aa+90-(aa+5-dd)/5*4;bb+=11){
      glVertex3fv(flodraw[wVert]);
      glVertex3fv(flodraw[bb]);
      wVert++;
    }
    glVertex3fv(flodraw[wVert]);
    glVertex3fv(flodraw[aa+100]);
    glEnd();
  }
}

/// Draw the ceiling
void SpaceJunk::drawceiling(void){
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);

  for (aa=0;aa<1001;aa+=95){
    glBegin(GL_LINE_STRIP);
    glColor4f(0,.5,1.,1.);
    for (wVert=aa;wVert<51+aa;wVert++)
      glVertex3f(ceidraw[wVert][0],ceidraw[wVert][1]-.01,ceidraw[wVert][2]);
    glEnd();
  }
  for (aa=0;aa<51;aa+=5){
    glBegin(GL_LINE_STRIP);
    glColor4f(0,.5,1.,1);
      glVertex3f(ceidraw[aa][0],ceidraw[aa][1]-.01,ceidraw[aa][2]);
    for (bb=51+aa/5;bb<1001;bb+=95){
      for (wVert=bb;wVert<11*4+bb;wVert+=11)
        glVertex3f(ceidraw[wVert][0],ceidraw[wVert][1]-.01,ceidraw[wVert][2]);
      glVertex3f(ceidraw[wVert+aa/5*4][0],ceidraw[wVert+aa/5*4][1]-.01,ceidraw[wVert+aa/5*4][2]);
    }
    glEnd();
  }

//#if 0 // disabled because it chops off the figure's head
  for (dd=0;dd<950;dd+=95)
  for (aa=dd;aa<dd+50;aa+=5)
  {
    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0,.5,1.,.1);
    wVert=aa+1;
    glVertex3fv(ceidraw[aa]);
    for (bb=aa+51-(aa-dd)/5*4;bb<aa+85-(aa-dd)/5*4;bb+=11)
    {
      glVertex3fv(ceidraw[wVert]);
      glVertex3fv(ceidraw[bb]);
      wVert++;
    }
    glVertex3fv(ceidraw[wVert]);
    glVertex3fv(ceidraw[aa+95]);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(0,.5,1.,.1);
    wVert=aa+95;
    glVertex3fv(ceidraw[aa+5]);
    for (bb=aa+56-(aa+5-dd)/5*4;bb<aa+90-(aa+5-dd)/5*4;bb+=11)
    {
      glVertex3fv(ceidraw[wVert]);
      glVertex3fv(ceidraw[bb]);
      wVert++;
    }
    glVertex3fv(ceidraw[wVert]);
    glVertex3fv(ceidraw[aa+100]);
    glEnd();
  }
//#endif
}

/// Draw the orange pillars
void SpaceJunk::drawpillars(void) {
  for (aa=0;aa<11;aa++) {
    glColor3f(1,1,1);
    for (bb=0;bb<4;bb++) {
      glBegin(GL_LINE_STRIP);
      for (wVert=bb;wVert<40;wVert+=4)
        glVertex3fv(pilldraw[aa][wVert]);
      glEnd();
    }
    
    glColor3f(.5,.25,0);
    glBegin(GL_TRIANGLE_STRIP);
    for (wVert=0;wVert<40;wVert+=4) {
      glVertex3fv(pilldraw[aa][wVert]);
      glVertex3fv(pilldraw[aa][wVert+1]);
    }
    glEnd();
    
    glColor3f(.75,.375,.25);
    glBegin(GL_TRIANGLE_STRIP);
    for (wVert=0;wVert<40;wVert+=4) {
      glVertex3fv(pilldraw[aa][wVert]);
      glVertex3fv(pilldraw[aa][wVert+2]);
    }
    glEnd();
    
    glColor3f(1,.5,0);
    glBegin(GL_TRIANGLE_STRIP);
    for (wVert=3;wVert<40;wVert+=4) {
      glVertex3fv(pilldraw[aa][wVert]);
      glVertex3fv(pilldraw[aa][wVert-1]);
    }
    glEnd();
    
    glColor3f(.75,.375,.25);
    glBegin(GL_TRIANGLE_STRIP);
    for (wVert=3;wVert<40;wVert+=4) {
      glVertex3fv(pilldraw[aa][wVert]);
      glVertex3fv(pilldraw[aa][wVert-2]);
    }
    glEnd();
  }
}


/// Draw the floor and ceiling triangles
void SpaceJunk::drawtris(void) {
  for (aa=0;aa<16;aa++)
  {
    glColor4f(1,1,1,1);
    glBegin(GL_LINE_STRIP);
      glVertex3fv(tribdraw[aa][1]);
      glVertex3fv(tribdraw[aa][2]);
      glVertex3fv(tribdraw[aa][0]);
      glVertex3fv(tribdraw[aa][3]);
      glVertex3fv(tribdraw[aa][4]);
    glEnd();
    glBegin(GL_LINE_STRIP);
      glVertex3fv(tribdraw[aa][1]);
      glVertex3fv(tribdraw[aa][3]);
      glVertex3fv(tribdraw[aa][0]);
      glVertex3fv(tribdraw[aa][2]);
      glVertex3fv(tribdraw[aa][4]);
    glEnd();
    glBegin(GL_LINE_STRIP);
      glVertex3fv(tritdraw[aa][1]);
      glVertex3fv(tritdraw[aa][2]);
      glVertex3fv(tritdraw[aa][0]);
      glVertex3fv(tritdraw[aa][3]);
      glVertex3fv(tritdraw[aa][4]);
    glEnd();
    glBegin(GL_LINE_STRIP);
      glVertex3fv(tritdraw[aa][1]);
      glVertex3fv(tritdraw[aa][3]);
      glVertex3fv(tritdraw[aa][0]);
      glVertex3fv(tritdraw[aa][2]);
      glVertex3fv(tritdraw[aa][4]);
    glEnd();
   
    glColor4f(.5,1,.2,1);
    glBegin(GL_TRIANGLE_FAN);
      glVertex3fv(tribdraw[aa][0]);
      glVertex3fv(tribdraw[aa][1]);
      glVertex3fv(tribdraw[aa][2]);
      glVertex3fv(tribdraw[aa][4]);
      glVertex3fv(tribdraw[aa][3]);
      glVertex3fv(tribdraw[aa][1]);
    glEnd();
    glColor4f(.5,.2,1,1);
    glBegin(GL_TRIANGLE_FAN);
      glVertex3fv(tritdraw[aa][0]);
      glVertex3fv(tritdraw[aa][1]);
      glVertex3fv(tritdraw[aa][2]);
      glVertex3fv(tritdraw[aa][4]);
      glVertex3fv(tritdraw[aa][3]);
      glVertex3fv(tritdraw[aa][1]);
    glEnd();
  }
}

/// Draw the "cage"
void SpaceJunk::drawbeams(void){
  for (aa=0;aa<16;aa++){
    glColor4f(1,1-((float)aa)/20.0,1-16.0/20.0+((float)aa)/20.0,1);
    glBegin(GL_LINE_STRIP);
    for (bb=0;bb<30;bb++)
      glVertex3fv(beamdraw[aa][bb]);
    glEnd();
  }
}

/// Draw everything in the scene
void SpaceJunk::drawAll() {
  drawbeams();
  drawtris();
  drawpillars();
  drawfloor();
  drawceiling();
}

/// Update everything in the scene
void SpaceJunk::updateAll (s_updateValues &updateValues)
{
  for (aa=0; aa<1001; aa++){
    relativisticTransform(ceid[aa], ceidraw[aa], updateValues);
    relativisticTransform(flod[aa], flodraw[aa], updateValues);
  }
  for (aa=0; aa<11; aa++)
    for (bb=0; bb<40; bb++)
      relativisticTransform(pillars[aa][bb], pilldraw[aa][bb], updateValues);
  for (aa=0; aa<16; aa++){
    for (bb=0; bb<5; bb++){
      relativisticTransform(trit[aa][bb], tritdraw[aa][bb], updateValues);
      relativisticTransform(trib[aa][bb], tribdraw[aa][bb], updateValues);
    }
    for (bb=0; bb<30; bb++)
      relativisticTransform(beam[aa][bb], beamdraw[aa][bb], updateValues);
  }
}
