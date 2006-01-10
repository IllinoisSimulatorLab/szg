/* Copyright (c) Mark J. Kilgard, 1994. */

/**
 * (c) Copyright 1993, 1994, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "arGraphicsHeader.h"
#include "atlantis.h"
#include "arMasterSlaveFramework.h"
#include "arMath.h"
#include "arDataUtilities.h"
#include "arNavigationUtilities.h"
#include "arInteractionUtilities.h"
#include "arCallbackInteractable.h"
#include <list>
#include <iostream>
using namespace std;

// Syzygy conversion-specific parameters (give or take a string constant).

int anaglyphMode = 0;
bool connected = false;

// Unit conversions.  Tracker (and cube screen descriptions) use feet.
// Atlantis uses 1/2-millimeters (that's what ended up looking best in
// stereo.  Use mm. and the sharks get too huge and far apart).
const float CM_TO_ATLANTISUNITS = 20.;
const float FEET_TO_AU = 12*2.54*CM_TO_ATLANTISUNITS;

// Near & far clipping planes.
//const float nearClipDistance = 20*CM_TO_ATLANTISUNITS;
const float nearClipDistance = 1*CM_TO_ATLANTISUNITS;
const float farClipDistance = 20000.*CM_TO_ATLANTISUNITS;

const int SPHERE_SLICES = 10;
const int SPHERE_STACKS = 6;

// Sound matrix IDs
int whaleSoundTransformID = -1;
int dolphinSoundTransformID = -1;

// Gains to apply to inputs.
float speed = 300.;
float acceleration = 0.95;

// Stuff for interacting with sharks
arEffector spear( 1, 6, 2, 2, 0, 0, 0 );
arCallbackInteractable interactableArray[NUM_SHARKS];
list<arInteractable*> interactionList;
const float SPEAR_LENGTH = 6*FEET_TO_AU;
float gSpearRadius = 2.*FEET_TO_AU;
int gDrawSpearTip = 0;

arTimer tipTimer;

static float lightPosition[] =
    {0.0, 1.0, 0.0, 0.0};

// atlantis-specific stuff.
fishRec sharks[NUM_SHARKS];
fishRec momWhale;
fishRec babyWhale;
fishRec dolph;

int useTexture = 1;
arTexture seaTexture;
GLfloat s_plane[] = { 1, 0, 0, 0 };
GLfloat t_plane[] = { 0, 0, 1, 0 };
GLfloat textureScale = 0.0002;


const int SHARK_SPREAD = 20000;  // originally 6000

// interactable callbacks

static arMatrix4 gSpearBaseMatrix;

void toggleUseTexture() {
  useTexture = !useTexture;
}

void drawTransparentSphere( const arVector3& offset, float radius ) {
  glPushMatrix();
    glTranslatef( offset.v[0], offset.v[1], offset.v[2] );
    glColor4f( .5, .5, 0., 0.5 );
    glEnable( GL_BLEND );
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glutSolidSphere( radius, SPHERE_SLICES, SPHERE_STACKS );
    glDisable( GL_BLEND );
  glPopMatrix();
}

void drawSpear( const arMatrix4& spearBaseMatrix ) {
  const arMatrix4 spearOffsetMat( ar_translationMatrix(
                                      arVector3( 0,0,-0.5*SPEAR_LENGTH ) ) );
  const arMatrix4 spearMatrix( spearBaseMatrix*spearOffsetMat );
//  cerr << ar_getNavMatrix() << endl ;
//  cerr << spearBaseMatrix << endl << "----------------------" << endl;
  glPushMatrix();
    glMultMatrixf(spearMatrix.v);
    glPushMatrix();
      glScalef( FEET_TO_AU/12, FEET_TO_AU/12., SPEAR_LENGTH );
      glColor3f(.4,.4,.4);
      glutSolidCube(1.);
    glPopMatrix();
    glPushMatrix();
     glTranslatef( 0, FEET_TO_AU, 0 );
     glutSolidCube(.2);
    glPopMatrix();
    if (gDrawSpearTip)
      drawTransparentSphere( arVector3(0,0,-0.5*SPEAR_LENGTH), gSpearRadius );
  glPopMatrix();
}

void matrixCallback( arCallbackInteractable* object, const arMatrix4& matrix ) {
  fishRec& shark = sharks[object->getID()];
  arVector3 pos(ar_extractTranslation(matrix));
  // coordinate swapping based on FishTransform()
  shark.y = pos[0];
  shark.z = pos[1];
  shark.x = -pos[2];
}

void setAnaglyphMode(arMasterSlaveFramework&, bool on) {
  if (on) {
    // Make everything gray.
    static const float gray_diffuse[] = {0.8, 0.8, 0.8, 1.0};
    static const float gray_ambient[] = {0.3, 0.3, 0.3, 1.0};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, gray_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, gray_ambient);
    glClearColor(0.8, 0.8, 0.8, 1.0);
    //fw.setViewMode( "anaglyph" );
  } else {
    static const float mat_diffuse[] = {0.6, 0.8, 0.9, 1.0};
    static const float mat_ambient[] = {0.0, 0.2, 0.4, 1.0};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
    glClearColor(0.0, 0.6, 1., 1.0);
    //fw.setViewMode( "normal" );
  }
//  fw.setAnaglyphMode( on );
}

// Arrays for packing fish params into for master/slave communication.
const int NUM_COORDS = (3 + NUM_SHARKS) * 13;
const int NUM_FLAGS  = (3 + NUM_SHARKS) * 2;
static float fishCoords[NUM_COORDS];
static int fishFlags[NUM_FLAGS];

void packOneFish(_fishRec *f, int fishNum) {
  float *coordPtr = fishCoords + fishNum*13;
  int *flagPtr = fishFlags + fishNum*2;

  *coordPtr++ = f->x;
  *coordPtr++ = f->y;
  *coordPtr++ = f->z;
  *coordPtr++ = f->phi;
  *coordPtr++ = f->theta;
  *coordPtr++ = f->psi;
  *coordPtr++ = f->v;
  *coordPtr++ = f->xt;
  *coordPtr++ = f->yt;
  *coordPtr++ = f->zt;
  *coordPtr++ = f->htail;
  *coordPtr++ = f->vtail;
  *coordPtr++ = f->dtheta;

  *flagPtr++ = f->spurt;
  *flagPtr++ = f->attack;
}

void unpackOneFish(_fishRec *f, int fishNum) {
  float *coordPtr = fishCoords + fishNum*13;
  int *flagPtr = fishFlags + fishNum*2;

  f->x = *coordPtr++;
  f->y = *coordPtr++;
  f->z = *coordPtr++;
  f->phi = *coordPtr++;
  f->theta = *coordPtr++;
  f->psi = *coordPtr++;
  f->v = *coordPtr++;
  f->xt = *coordPtr++;
  f->yt = *coordPtr++;
  f->zt = *coordPtr++;
  f->htail = *coordPtr++;
  f->vtail = *coordPtr++;
  f->dtheta = *coordPtr++;

  f->spurt = *flagPtr++;
  f->attack = *flagPtr++;
}

float headUpAngle = 0.;

// Initialize shared parameters.
void initFishs(void) {
    for (int i = 0; i < NUM_SHARKS; i++) {
        sharks[i].x = 70000.0 + rand() % SHARK_SPREAD;
        sharks[i].y = rand() % SHARK_SPREAD;
        sharks[i].z = rand() % SHARK_SPREAD;
        sharks[i].psi = rand() % 360 - 180.0;
        sharks[i].v = 1.0;
    }

    dolph.x = 30000.0;
    dolph.y = 0.0;
    dolph.z = 6000.0;
    dolph.psi = 90.0;
    dolph.theta = 0.0;
    dolph.v = 3.0;

    momWhale.x = 70000.0;
    momWhale.y = 0.0;
    momWhale.z = 0.0;
    momWhale.psi = 90.0;
    momWhale.theta = 0.0;
    momWhale.v = 0.6;

    babyWhale.x = 60000.0;
    babyWhale.y = -2000.0;
    babyWhale.z = -2000.0;
    babyWhale.psi = 90.0;
    babyWhale.theta = 0.0;
    babyWhale.v = 0.6;
    babyWhale.htail = 45.0;
}

void initGL( arMasterSlaveFramework& fw, arGUIWindowInfo* /*windowInfo*/ ) {
  static float ambient[] = {0.1, 0.1, 0.1, 1.0};
  static float diffuse[] = {1.0, 1.0, 1.0, 1.0};
	static float position[] =	{0.0, 1.0, 0.0, 0.0};
  static float mat_shininess[] = {100.0};
  static float mat_specular[] = {0.9, 0.9, 0.9, 1.0};
	static float mat_diffuse[] = {0.46, 0.66, 0.795, 1.0};
	static float mat_ambient[] = {0.0, 0.1, 0.2, 1.0};
  static float lmodel_ambient[] = {0.5, 0.5, 0.5, 1.0};
  static float lmodel_localviewer[] = {0.4};

  glFrontFace(GL_CCW);

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);

  glEnable(GL_CULL_FACE);
  glEnable(GL_NORMALIZE);
  glShadeModel(GL_SMOOTH);

  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_POSITION, position);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_localviewer);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);

  std::string dataPath = fw.getSZGClient()->getAttribute("SZG_DATA","path");
  if (!seaTexture.readJPEG( "sea-texture.jpg", "atlantis", dataPath )) {
    cerr << "atlantis error: failed to read sea-texture.jpg.\n";
    useTexture = 0;
  } else {
    // cerr << "atlantis remark: activating texture in init.\n";
    seaTexture.activate();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexGeni (GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni (GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_2D);
  }
}

bool init( arMasterSlaveFramework& fw, arSZGClient& /*SZGClient*/ ) {
  //  setAnaglyphMode(fw, anaglyphMode);
  initFishs();

  // This is cube-specific (the origin is on the floor, so the center
  // of the screen is 5 feet high).
  ar_navTranslate(arVector3(0., -5.*FEET_TO_AU, 0.));

  if (fw.getMaster()) {
    connected = true;

    // set max interaction distance at .5 ft. in real-world coordinates
    spear.setUnitConversion( FEET_TO_AU );
    spear.setInteractionSelector( arDistanceInteractionSelector( gSpearRadius ) );
    // Set wand to do a normal drag when you click on button 2 or 6 & 7
    // (the wand triggers)
    spear.setDrag( arGrabCondition( AR_EVENT_BUTTON, 0, 0.5 ),
                         arWandRelativeDrag() );
    spear.setDrag( arGrabCondition( AR_EVENT_BUTTON, 6, 0.5 ),
                         arWandRelativeDrag() );
    spear.setDrag( arGrabCondition( AR_EVENT_BUTTON, 7, 0.5 ),
                         arWandRelativeDrag() );
    spear.setTipOffset( arVector3(0,0,-SPEAR_LENGTH) );

    for (int i = 0; i < NUM_SHARKS; i++) {
      arCallbackInteractable cbi(i);
      cbi.setMatrixCallback( matrixCallback );
      interactableArray[i] = cbi;
      interactionList.push_back( (arInteractable*)(interactableArray+i) );
    }
  }

  // Load in sounds & start them looping
  const arMatrix4  ident;
  whaleSoundTransformID = dsTransform( "whale sound matrix", "root", ident );
  dolphinSoundTransformID = dsTransform( "dolphin sound matrix", "root", ident );
  (void)dsLoop("whale song", "whale sound matrix", "whale.mp3", 1,
    1.0, arVector3(0,0,0));
  (void)dsLoop("dolphin song", "dolphin sound matrix", "dolphin.mp3", 1,
    0.05, arVector3(0,0,0));

  // Register the shared memory.
  fw.addTransferField("fishCoords", fishCoords, AR_FLOAT, NUM_COORDS);
  fw.addTransferField("fishFlags", fishFlags, AR_INT, NUM_FLAGS);
  fw.addTransferField("anaglyph", &anaglyphMode, AR_INT, 1);
  fw.addTransferField("spearMatrix", gSpearBaseMatrix.v, AR_FLOAT, 16);
  fw.addTransferField("spearRadius", &gSpearRadius, AR_FLOAT, 1);
  fw.addTransferField("drawSpearTip", &gDrawSpearTip, AR_INT, 1);
  fw.addTransferField("useTexture", &useTexture, AR_INT, 1);

  fw.setNavTransSpeed( 20.*FEET_TO_AU );
  fw.ownNavParam("translation_speed");
  return true;
}

static int spearTipIndex = 0;
float distances[3] = {2*FEET_TO_AU,1*FEET_TO_AU,.5*FEET_TO_AU};
static bool sharkAttack = false;

// This is where we pack data into the networked shared memory
// to be sent to the slaves
void preExchange(arMasterSlaveFramework& fw) {
  int i;
  static bool firstAttack(true);
  fw.navUpdate();
  if (fw.getOnButton(5)) {
    sharkAttack = !sharkAttack;
    if (sharkAttack) {
      if (firstAttack) {
        dsSpeak( "warning speech", "root", "Now Jim has excited the sharks into a feeding frenzy. See, here they come." );
        firstAttack = false;
      } else {
        dsSpeak( "warning speech", "root", "Oh <emph>no</emph>, here they come again." );
      }
    } else {
      dsSpeak( "warning speech", "root", "I guess they're not hungry." );
    }
  }

  // Animate the fishies
  const arMatrix4 headMatrix = ar_matrixToNavCoords( fw.getMatrix(0) );
  for (i = 0; i < NUM_SHARKS; i++) {
    SharkPilot(&sharks[i], headMatrix, sharkAttack);
    SharkMiss(i);
    UpdateShark(&sharks[i]);
  }
  DolphinPilot(&dolph);
  dolph.phi++;
  UpdateDolphin(&dolph);
  WhalePilot(&momWhale);
  momWhale.phi++;
  UpdateWhale(&momWhale);
  WhalePilot(&babyWhale);
  babyWhale.phi++;
  UpdateWhale(&babyWhale);

  if (fw.getOnButton(2)) {
    spearTipIndex = (spearTipIndex > 1)?(0):(spearTipIndex+1);
    gSpearRadius = distances[spearTipIndex];
    spear.setInteractionSelector( arDistanceInteractionSelector( gSpearRadius ) );
    gDrawSpearTip = 1;
    tipTimer.start( 1.e6 ); // 1 sec.
    cerr << "atlantis remark: target radius set to " << gSpearRadius << endl;
  }
  if ((gDrawSpearTip)&&tipTimer.done())
    gDrawSpearTip = 0;
  for (i=0; i<NUM_SHARKS; i++) {
    interactableArray[i].setMatrix( ar_translationMatrix(
                         sharks[i].y, sharks[i].z, -sharks[i].x ) );
  }
  spear.updateState( fw.getInputState() );
  ar_pollingInteraction( spear, interactionList );
  gSpearBaseMatrix = spear.getBaseMatrix();

  // Send fish position data to slaves.
  packOneFish(&momWhale, 0);
  packOneFish(&babyWhale, 1);
  packOneFish(&dolph, 2);
  for (i = 0; i<NUM_SHARKS; i++)
    packOneFish(&sharks[i], i+3);

  // Calculate transformation matrix from head to whale (should be whale's
  // mouth (or blowhole?), haven't gotten that far yet) for sound rendering.
  arMatrix4 fishMatrix( ar_translationMatrix( momWhale.y, momWhale.z, -momWhale.x ) );
  const arMatrix4 navMatrix( ar_getNavInvMatrix() );
  const arMatrix4 headInvMatrix( fw.getMatrix(0).inverse() );
  arMatrix4 soundMatrix( headInvMatrix*navMatrix*fishMatrix );

  // Decrease attenuation 'cause we're under water (and it sounds better)
  for (i=12; i<15; i++ )
    soundMatrix[i] /= 25*FEET_TO_AU;

  // Load matrices for whale and dolphin.
  dsTransform( whaleSoundTransformID, soundMatrix );
  fishMatrix = ar_translationMatrix( dolph.y, dolph.z, -dolph.x );

  soundMatrix = headInvMatrix*navMatrix*fishMatrix;
  for (i=12; i<15; i++ )
    soundMatrix[i] /= 25*FEET_TO_AU;
  dsTransform( dolphinSoundTransformID, soundMatrix );
}

void postExchange(arMasterSlaveFramework& fw){
  // Get fish positions from master.
  unpackOneFish(&momWhale, 0);
  unpackOneFish(&babyWhale, 1);
  unpackOneFish(&dolph, 2);
  for (int i = 0; i<NUM_SHARKS; i++)
    unpackOneFish(&sharks[i], i+3);

  // As we go forward, bring the far clipping plane in
  // to increase z-buffer precision.
  const arMatrix4 vt = ar_getNavInvMatrix();
  fw.setClipPlanes(nearClipDistance, farClipDistance-vt.v[14]);
  if (useTexture)
    glEnable( GL_TEXTURE_2D );
  else
    glDisable( GL_TEXTURE_2D );
  //setAnaglyphMode(fw, anaglyphMode);
}

#ifdef UNUSED
// NOTE: we can't use this as is, assumes a vertical screen
static void drawGradient() {
  static GLuint gradientDisplayList = 0;
  static GLuint gradientTexObject = 0;

  if (gradientDisplayList == 0) {
    unsigned char *pixels = 0;
    int start = 64;
    int end = start + 128;
    int size = 4 * (end - start);
    int i;

    pixels = (unsigned char *) malloc (size);
    i = 0;
    while (i < size) {
      pixels[i] = 0; i++;
      pixels[i] = (start + (i>>2)) * 0.56; i++;
      pixels[i] = (start + (i>>2)); i++;
      pixels[i] = 255; i++;
    }

    glGenTextures(1, &gradientTexObject);
    glBindTexture(GL_TEXTURE_1D, gradientTexObject);

    glTexImage1D(GL_TEXTURE_1D, 0, 4,
                 (end - start), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    gradientDisplayList = glGenLists(1);
    glNewList(gradientDisplayList, GL_COMPILE);

    glDepthMask (false);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_1D);
    glBindTexture(GL_TEXTURE_1D, gradientTexObject);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glRotatef(90, 0, 0, 1);
    glTranslatef(-1, -1, 0);
    glScalef(2, 2, 1);

    glBegin(GL_QUADS);
    glTexCoord1i(1); glVertex3i(1, 0, 0);
    glTexCoord1i(0); glVertex3i(0, 0, 0);
    glTexCoord1i(0); glVertex3i(0, 1, 0);
    glTexCoord1i(1); glVertex3i(1, 1, 0);
    glEnd();

    glPopMatrix();

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_TEXTURE_1D);

    glDepthMask (true);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glEndList();
  }

  glCallList(gradientDisplayList);
}
#endif

void drawFishies()
{
  static float angle(0);
  static double offset(0);
  if (useTexture) {
    glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
    glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0,offset,0);
    glScalef(textureScale, textureScale, 1);
    glMatrixMode(GL_MODELVIEW);
    offset += .00001*FEET_TO_AU*sin(2*M_PI*angle/30.);
    angle += .1;
    if (angle > 360)
      angle -= 360;
  }
  for (int i = 0; i < NUM_SHARKS; i++) {
    glPushMatrix();
      FishTransform(&sharks[i]);
      DrawShark(&sharks[i]);
    glPopMatrix();
  }
  glPushMatrix();
    FishTransform(&dolph);
    DrawDolphin(&dolph);
  glPopMatrix();
  glPushMatrix();
    FishTransform(&momWhale);
    DrawWhale(&momWhale);
  glPopMatrix();
  glPushMatrix();
    FishTransform(&babyWhale);
    glScalef(0.45, 0.45, 0.3);
    DrawWhale(&babyWhale);
  glPopMatrix();
}

void display(arMasterSlaveFramework& fw)
{
  if (!fw.getConnected()) {
    glClearColor(0,0,0,0);
    return;
  }

  if (anaglyphMode) {
    glClearColor(0.8, 0.8, 0.8, 1.0);
  } else {
    glClearColor(0.0, 0.6, 1., 1.0);
  }
  fw.loadNavMatrix(); // drive around.
  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
  drawFishies();
  drawSpear( gSpearBaseMatrix );
}

void windowEvent( arMasterSlaveFramework& fw, arGUIWindowInfo* windowInfo ) {
  if( !windowInfo ) {
    return;
  }

  switch( windowInfo->getState() ) {
    case AR_WINDOW_RESIZE:
      fw.getWindowManager()->setWindowViewport( windowInfo->getWindowID(),
                                                0, 0, windowInfo->getSizeX(), windowInfo->getSizeY() );
    break;

    case AR_WINDOW_CLOSE:
      fw.stop( false );
    break;

    default:
    break;
  }
}

/*
void reshape(arMasterSlaveFramework&, int width, int height) {
  glViewport(0,0,width,height);
}
*/

void exitCallback( arMasterSlaveFramework& ) {
  cerr << "atlantis remark: exiting.\n";
}

int main(int argc, char** argv){
  arMasterSlaveFramework framework;

  framework.setStartCallback(init);
  framework.setPreExchangeCallback(preExchange);
  framework.setPostExchangeCallback(postExchange);
  framework.setDrawCallback(display);
  // framework.setReshapeCallback(reshape);
  framework.setWindowEventCallback( windowEvent );
  framework.setWindowStartGLCallback( initGL );
  framework.setClipPlanes(nearClipDistance, farClipDistance);
  framework.setExitCallback( exitCallback );
  // Tell the framework that we're using half-millimeter units.
  // Do this before framework.init() if we use framework-based navigation.
  framework.setUnitConversion(FEET_TO_AU);

  if (!framework.init(argc, argv)){
    return 1;
  }
  framework.setDataBundlePath("SZG_DATA", "atlantis");

  if (!framework.start()){
    return 1;
  }
  // Never get here.
  return 0;
}
