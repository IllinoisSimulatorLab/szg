/* Aftershock 3D rendering engine
 * Copyright (C) 1999 Stephen C. Taylor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <stdio.h>
#include <string.h>

#include "arGraphicsHeader.h"

#include "uicommon.h"
#include "rendercontext.h"
#include "renderhud.h"
#include "globalshared.h"

#include "arMasterSlaveFramework.h"
#include "arMath.h"
#include "arNavigationUtilities.h"

//#define SWAY_DEMO

string pakFilePath;

global_shared_t *g = NULL;

static r_context_t *r_context;

int global_start_type = START_TYPE_DEATHMATCH;

/* use regular memory */
void *gc_malloc(size_t size) { return malloc(size); }
void *rc_malloc(size_t size) { return malloc(size); }
void gc_free(void *mem) { free(mem); }
void rc_free(void *mem) { free(mem); }

#include <iostream>
using namespace std;

// the way to pass information into the quake arg parser 
char** g_argv;
int    g_argc;

static bool noNavRotation = false;
static bool noVertNavTrans = false;

bool parseNavArgs(int& argc, char** argv){
  for (int i=0; i<argc; i++){
    bool argFound = false;
    if (!strcmp(argv[i],"-norot")) {
      noNavRotation = true;
      argFound = true;
    } else if (!strcmp(argv[i],"-noverttrans")) {
      noVertNavTrans = true;
      argFound = true;
    } else if (!strcmp(argv[i],"-normalstart")) {
      global_start_type = START_TYPE_START;
      argFound = true;
    }
    if (argFound) {
      // erase the arg
      for (int j=i; j<argc-1; j++){
        argv[j] = argv[j+1];
      }
      // reset the arg count and i
      --argc;
      i--;
    }
  }
  return true;
}

bool init(arMasterSlaveFramework& fw, arSZGClient& cli){
  parseNavArgs( g_argc, g_argv );
  
  pakFilePath = cli.getAttribute("SZG_DATA","path");

  g = (global_shared_t *) gc_malloc(sizeof(global_shared_t));
  init_global_shared(g);
  ui_read_args(g_argc, g_argv);
  strcpy(g->r_help_fname, "paul/helpGL.jpg");

  // Load map.
  ui_init_bsp();
  printf("\n\n");

  // Set up rendering context.
  r_context = (r_context_t *) rc_malloc(sizeof(r_context_t));
  ui_init_gl(r_context);
  
  ar_navRotate( arVector3(0,1,0), g->r_eye_az+90. );

  // Register fields shared over the network.
  fw.addTransferField("eyepos",r_context->r_eyepos,AR_FLOAT,3);
  fw.addTransferField("eye_az",&r_context->r_eye_az,AR_FLOAT,1);
  fw.addTransferField("eye_el",&r_context->r_eye_el,AR_FLOAT,1); 
  fw.addTransferField("time",&r_context->r_frametime,AR_DOUBLE,1);
  fw.addTransferField("eyecluster",&r_context->r_eyecluster,AR_INT,1); 
  return true;
}

#ifdef AR_USE_WIN_32
inline float drand48(){
  return float(rand()) / float(RAND_MAX);
}
#endif

void do_sounds(double currentTime, bool fCollided){
  static int fooID = -1;
  static int barID = -1;
  static int zipID = -1;
  static bool finitSound = false;
  if (!finitSound){
    finitSound = true;
    const float xyz[3] = { 2.34,0,0};
      fooID = dsLoop("foo", "root", "q33beep.wav", 0, 0.0, xyz);
      barID = dsLoop("bar", "root", "q33move.mp3", 1, 0.03, xyz);
      zipID = dsLoop("zip", "root", "q33collision.wav", 0, 0.0, xyz);
  }

  // Trigger sounds, if needed.

  // Speeds are 0 to 200.  Typical is 30 to 200.  (Hardcoded in move_eye().)
  const float v = g->g_velocity / 200;

  // Trigger a sound if player ran into something.
  {
    const float xyz[3] = { -2.34,0,0 };
    static bool fReset = true;
    if (fCollided){
      dsLoop(zipID, "q33collision.wav", -1, .07 + .65 * v*v, xyz);
      fReset = false;
    }
    else if (!fReset){
      dsLoop(zipID, "q33collision.wav",  0, 0, xyz);
      fReset = true;
    }
  }

  // Periodically update background sounds.
  static double tPrev = currentTime;
  if (currentTime - tPrev > 0.03) {
    tPrev = currentTime;

    // Play a beep sporadically.
    {
      const float xyzBeep[3] 
        = { 5.*(drand48()-.5), 5.*(drand48()-.5), 5.*(drand48()-.5) };
      static bool fReset = true;
      if (drand48() < .02) {
	dsLoop(fooID, "q33beep.wav", -1, .02, xyzBeep);
	fReset = false;
      }
      else if (!fReset) {
	dsLoop(fooID, "q33beep.wav", 0, 0.0, xyzBeep);
	fReset = true;
      }
    }

    // Player's speed adjusts background's loudness.
    {
      static float moveAmplPrev = 0.;
      float moveAmpl = 0.02 + 0.4 * v*v;
      // Clamp fadeout rate.
      if (moveAmpl < moveAmplPrev)
	moveAmpl = moveAmplPrev;
      moveAmplPrev = moveAmpl * .97;
      const float xyzMove[3] = { 2.34,0,0 };
      dsLoop(barID, "q33move.mp3", 1, moveAmpl, xyzMove);
    }

  }
  // NEVER, NEVER, NEVER, NEVER, NEVER PUT SLEEPS LIKE THIS INTO THE MAIN LOOP
  // CAN'T COUNT ON A DELAY OF LESS THAN 10 MS
  //ar_usleep(1000); // CPU throttle, in case other apps are running
}

#define ALLOW_JUMP

void preExchange(arMasterSlaveFramework& fw){

  // time is in seconds, but these calls return milliseconds
  const double currentTime = fw.getTime() / 1000.0;
  const double timeDelta = fw.getLastFrameTime() / 1000.0;
  
  // Translate forwards/backwards, in direction wand is pointing.
  g->g_move =
    (fw.getAxis(1) >  0.5) ?  1 :
    (fw.getAxis(1) < -0.5) ? -1 : 0;
  if (g->g_move != 0){
    arVector3 wandDirection = ar_extractRotationMatrix(fw.getMatrix(1)) * arVector3(0,0,-1);
    if (noVertNavTrans) {
      wandDirection[1] = 0.;
      if (wandDirection.magnitude() < .001) {
        wandDirection = arVector3(0,0,-1);
      }
      wandDirection = wandDirection.normalize();
    }
    const arVector3 locWandDirection =
      ar_rotationMatrix('x',  M_PI/2)
      * ar_rotationMatrix('y', M_PI)
      * ar_getNavMatrix()
      * wandDirection;
    g->r_movedir[0] = locWandDirection.v[0];
    g->r_movedir[1] = locWandDirection.v[1];
    g->r_movedir[2] = locWandDirection.v[2];
  }

  g->r_setup_projection = 0;
  g->g_keylook = 0;
  g->g_acceleration = 100.;
  g->g_maxvel = 100.;
  
  // Button 3 enables scooting: translate sideways instead of rotating world
  // button 2 disables it.
  static bool fScoot = false;
  
  if (!noNavRotation) {
    if (fw.getButton(2))
      fScoot = true;
    if (fw.getButton(1))
      fScoot = false;
  }

  // NOTE: lateral translation doesn't work properly with the noVertNavTrans option,
  // so it's temporarily disabled (til I get it working)
  if (noNavRotation || fScoot) {
    if (!noVertNavTrans) {
      // Translate sideways, relative to wand's direction.
      // This is useful to move away from walls you've gotten stuck against.
      // Only scoot if we're not already moving forwards/backwards;
      // I'm too lazy to combine both translations right now.
      if (g->g_move == 0){
        g->g_move =
          (fw.getAxis(0) >  0.5) ?  1 :
          (fw.getAxis(0) < -0.5) ? -1 : 0;
        if (g->g_move != 0){
          const arVector3 wandDirectionLateral =
            ar_rotationMatrix('x', M_PI/2)
            * ar_rotationMatrix('y', M_PI)
            * ar_getNavMatrix()
            * ar_extractRotationMatrix(fw.getMatrix(1))
            * arVector3(1,0,0);
          g->r_movedir[0] = wandDirectionLateral.v[0];
          g->r_movedir[1] = wandDirectionLateral.v[1];
          g->r_movedir[2] = wandDirectionLateral.v[2];
        }
      }
    }
  }
  else{
    // Rotate the world about me, about a vertical axis.
    if (fw.getAxis(0)>0.5){
      ar_navRotate( arVector3(0,1,0), -30*timeDelta );
    }
    else if (fw.getAxis(0)<-0.5){
      ar_navRotate( arVector3(0,1,0), 30*timeDelta );
    }
  }
  
#ifdef SWAY_DEMO
  // testing a vection/sway demo
  static bool lastB4 = false;
  static bool lastB5 = false;
  static arTimer swayTimer;
  // const float SWAY_MAGNITUDE = 1.;
  const float SWAY_MAGNITUDE = 5.; // degrees
  const float SWAY_FREQUENCY = .1;
  float swayOffset = 0.;
  static float lastSwayOffset = 0;
  bool b4 = fw.getButton(4) > .5;
  bool b5 = fw.getButton(5) > .5;
  if (b5 && !lastB5) {
    if (!swayTimer.running()) {
      swayTimer.start();
    }
  }
  if (b4 && !lastB4) {
    if (swayTimer.running()) {
      swayTimer.reset();
    }
  }
  lastB4 = b4;
  lastB5 = b5;
  if (swayTimer.running()){
    swayOffset = SWAY_MAGNITUDE*sin(2*M_PI*SWAY_FREQUENCY*swayTimer.totalTime()/1.e6);
  } else {
    swayOffset = 0.;
  }
  ar_navRotate( arVector3(1,0,0), swayOffset-lastSwayOffset );
  // ar_navTranslate( arVector3(0,0,swayOffset-lastSwayOffset) );
  lastSwayOffset = swayOffset;
  
#endif

#ifdef ALLOW_JUMP
  static bool lastb3 = false;
  bool b3 = fw.getButton(3) > .5;
  if (b3) {
    g->r_eyepos[2] += 2.;
    g->r_eyepos_sav[2] += 2.;
  }
  lastb3 = b3;
  static bool lastb0 = false;
  bool b0 = fw.getButton(0) > .5;
  if (b0) {
    g->r_eyepos[2] -= 2.;
    g->r_eyepos_sav[2] -= 2.;
  }
  lastb0 = b0;
#endif
  
  if (fw.soundActive()){
    do_sounds(currentTime, ui_move(currentTime));
  }
  else{
    ui_move(currentTime);
  }
  ui_sync(r_context,currentTime);
}

void playCallback(arMasterSlaveFramework& fw){
  if (!fw.getMaster())
    return;
  if (!fw.soundActive())
    return;
  // do something here.  root -> world -> other things in sound database,
  // modify world from position in q33 world, 
  // that's the nav matrix, setHeadMatrix from that?

  // this sets head matrix: /* navMatrix * */ 
  // ar_rotationMatrix('y',3.14) * ar_rotationMatrix('x',-1.57)
  // see the glMultMatrixf call in drawCallback().
}

void windowCallback(arMasterSlaveFramework&){
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void drawCallback(arMasterSlaveFramework& fw){
  glEnable(GL_DEPTH_TEST);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  if (fw.getConnected()) {
    glEnable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    fw.loadNavMatrix();
    const arMatrix4 worldCoordinateRotation = 
      ar_rotationMatrix('y',3.14)*ar_rotationMatrix('x',-1.57);
    glMultMatrixf(worldCoordinateRotation.v);
    double placeHolder = 0;
    ui_display_nosync(r_context, placeHolder);
  }
}

int main(int argc, char** argv){
  // Set global variables for quake argument parser.
  g_argc = argc;
  g_argv = argv;

  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setClipPlanes(0.5,3000);
  framework.setStartCallback(init);
  framework.setPreExchangeCallback(preExchange);
  framework.setWindowCallback(windowCallback);
  framework.setDrawCallback(drawCallback);
  framework.setPlayCallback(playCallback);
  framework.setEyeSpacing(6/(12*2.54));
  return framework.start() ? 0 : 1;
}
