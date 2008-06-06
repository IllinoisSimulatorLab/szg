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

#include "arPrecompiled.h"

#include "arCallbackInteractable.h"
#include "arInteractionUtilities.h"
#include "arMasterSlaveFramework.h"

#include "atlantis.h"

// Syzygy conversion-specific parameters (give or take a string constant).

int anaglyphMode = 0;
bool connected = false;

// Unit conversions.  Tracker (and cube screen descriptions) use feet.
// Atlantis uses 1/2-millimeters (that's what ended up looking best in
// stereo.  Use mm. and the sharks get too huge and far apart).
const float ATLANTISUNITS_PER_CM = 20.;
const float ATLANTISUNITS_PER_FOOT = 12*2.54*ATLANTISUNITS_PER_CM;

// Near & far clipping planes.
//const float nearClipDistance = 20*ATLANTISUNITS_PER_CM;
const float nearClipDistance = 1*ATLANTISUNITS_PER_CM;
const float farClipDistance = 20000.*ATLANTISUNITS_PER_CM;

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
const float SPEAR_LENGTH = 6*ATLANTISUNITS_PER_FOOT;
float gSpearRadius = 2.*ATLANTISUNITS_PER_FOOT;
int gDrawSpearTip = 0;

arTimer tipTimer;


// atlantis-specific stuff.
fishRec sharks[NUM_SHARKS];
fishRec momWhale;
fishRec babyWhale;
fishRec dolph;

int useTexture = 1;
int drawVerticalBar = 0;
arTexture seaTexture;
GLfloat s_plane[] = { 1, 0, 0, 0 };
GLfloat t_plane[] = { 0, 0, 1, 0 };
GLfloat textureScale = 0.0002;


const int SHARK_SPREAD = 20000;  // originally 6000

// interactable callbacks

static arMatrix4 gSpearBaseMatrix;

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
  glPushMatrix();
    glMultMatrixf(spearMatrix.v);
    glPushMatrix();
      glScalef( ATLANTISUNITS_PER_FOOT/12., ATLANTISUNITS_PER_FOOT/12., SPEAR_LENGTH );
      glColor3f(.4,.4,.4);
      glutSolidCube(1.);
    glPopMatrix();
    if (drawVerticalBar) {
      glPushMatrix();
       glTranslatef( 0, .5*ATLANTISUNITS_PER_FOOT, 0. );
       glScalef( ATLANTISUNITS_PER_FOOT/12., ATLANTISUNITS_PER_FOOT, ATLANTISUNITS_PER_FOOT/12. );
       glutSolidCube(1.);
      glPopMatrix();
    }
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

void initGL( arMasterSlaveFramework& fw, arGUIWindowInfo* ) {
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
    ar_log_warning() << "atlantis failed to read sea-texture.jpg.\n";
    useTexture = 0;
  } else {
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


bool start( arMasterSlaveFramework& fw, arSZGClient& ) {
  //  setAnaglyphMode(fw, anaglyphMode);
  initFishs();

  // Move from center of floor (traditional cave coord origin) to center of Cube.
  ar_navTranslate(arVector3(0., -5.*ATLANTISUNITS_PER_FOOT, 0.));

  if (fw.getMaster()) {
    connected = true;

    // set max interaction distance at .5 ft. in real-world coordinates
    spear.setUnitConversion( ATLANTISUNITS_PER_FOOT );
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

  // Register the shared memory.
  fw.addTransferField("fishCoords", fishCoords, AR_FLOAT, NUM_COORDS);
  fw.addTransferField("fishFlags", fishFlags, AR_INT, NUM_FLAGS);
  fw.addTransferField("anaglyph", &anaglyphMode, AR_INT, 1);
  fw.addTransferField("spearMatrix", gSpearBaseMatrix.v, AR_FLOAT, 16);
  fw.addTransferField("spearRadius", &gSpearRadius, AR_FLOAT, 1);
  fw.addTransferField("drawSpearTip", &gDrawSpearTip, AR_INT, 1);
  fw.addTransferField("drawVerticalBar", &drawVerticalBar, AR_INT, 1);
  fw.addTransferField("useTexture", &useTexture, AR_INT, 1);

  fw.setNavTransSpeed( 20.*ATLANTISUNITS_PER_FOOT );
  fw.ownNavParam("translation_speed");

  // Load and play looping sounds.
  const arMatrix4 ident;
  whaleSoundTransformID = dsTransform( "whale sound matrix", "root", ident );
  dolphinSoundTransformID = dsTransform( "dolphin sound matrix", "root", ident );
  (void)dsLoop("whale song", "whale sound matrix", "whale.mp3",
    1, 1.0, arVector3(0,0,0));
  (void)dsLoop("dolphin song", "dolphin sound matrix", "dolphin.mp3",
    1, 0.05, arVector3(0,0,0));
  return true;
}

static int spearTipIndex = 0;
float distances[3] = {2*ATLANTISUNITS_PER_FOOT,1*ATLANTISUNITS_PER_FOOT,.5*ATLANTISUNITS_PER_FOOT};
static bool sharkAttack = false;

// Pack data into the networked shared memory to send to slaves
void preExchange(arMasterSlaveFramework& fw) {
  int i;
  static bool firstAttack = true;
  fw.navUpdate();
  if (fw.getOnButton(0)) {
    drawVerticalBar = !drawVerticalBar;
  }

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

  // Animate
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
    ar_log_remark() << "atlantis: target radius set to " << gSpearRadius << "\n";
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

  // Calculate head-to-whale transformation for sound rendering.
  const arMatrix4 navMatrix( ar_getNavInvMatrix() );
  const arMatrix4 headInvMatrix( fw.getMatrix(0).inverse() );

  arMatrix4 fishMatrix( ar_translationMatrix( momWhale.y, momWhale.z, -momWhale.x ) );
  arMatrix4 soundMatrix( headInvMatrix*navMatrix*fishMatrix );
  // Decrease attenuation 'cause we're under water (and it sounds better)
  for (i=12; i<15; i++ )
    soundMatrix[i] /= ATLANTISUNITS_PER_FOOT;
  dsTransform( whaleSoundTransformID, soundMatrix );

  fishMatrix = ar_translationMatrix( dolph.y, dolph.z, -dolph.x );
  soundMatrix = headInvMatrix*navMatrix*fishMatrix;
  // Decrease attenuation 'cause we're under water (and it sounds better)
  for (i=12; i<15; i++ )
    soundMatrix[i] /= ATLANTISUNITS_PER_FOOT;
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
  // Ack! No longer legal to have OpenGL commands in postExchange!!!!
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
    offset += .00001*ATLANTISUNITS_PER_FOOT*sin(2*M_PI*angle/30.);
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
  if (anaglyphMode) {
    glClearColor(0.8, 0.8, 0.8, 1.0);
  } else {
    glClearColor(0.0, 0.6, 1., 1.0);
  }
  fw.loadNavMatrix(); // drive around.
  static const float lightPosition[] = {0.0, 1.0, 0.0, 0.0};
  glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
  drawFishies();
  drawSpear( gSpearBaseMatrix );
}

void windowEvent( arMasterSlaveFramework& fw, arGUIWindowInfo* wI ) {
  if( !wI ) {
    return;
  }

  switch( wI->getState() ) {
    case AR_WINDOW_RESIZE:
      fw.getWindowManager()->setWindowViewport(
        wI->getWindowID(), 0, 0, wI->getSizeX(), wI->getSizeY() );
    break;

    case AR_WINDOW_CLOSE:
      fw.stop( false );
    break;

    default:
    break;
  }
}

int main(int argc, char** argv){
  arMasterSlaveFramework fw;

  fw.setStartCallback(start);
  fw.setPreExchangeCallback(preExchange);
  fw.setPostExchangeCallback(postExchange);
  fw.setDrawCallback(display);
  fw.setWindowEventCallback( windowEvent );
  fw.setWindowStartGLCallback( initGL );
  fw.setClipPlanes(nearClipDistance, farClipDistance);

  fw.setUnitConversion(ATLANTISUNITS_PER_FOOT); // half-millimeter units

  fw.setDataBundlePath("SZG_DATA", "atlantis");
  return fw.init(argc, argv) && fw.start() ? 0 : 1;
}
