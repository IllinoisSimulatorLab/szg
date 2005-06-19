//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arTexture.h"
#include "arMath.h"
#include "arDataUtilities.h"
#include "slice.h"
#include "arCubeEnvironment.h"
#include "arMasterSlaveFramework.h"

bool stereoMode = false;
string dataPath;
arMatrix4 world;

const int volumeX = 91;
const int volumeY= 109;
const int volumeZ = 91;
const int numberVolumes = 10;
// we pack the data like...
// z*(dimX*dimY) + y*dimX + x
// so... this is basically like sequences of x/y slices
char volume[numberVolumes][volumeX*volumeY*volumeZ];
int currentVolume = 0;

// the poor graphics boards can't take too much!
const int numberSlices = 10;
const int sliceWidth = 128;
const int sliceHeight = 128;
slice** xStack = NULL;
slice** yStack = NULL;
slice** zStack = NULL;

void allocTextures(){
  xStack = new slice*[numberVolumes];
  yStack = new slice*[numberVolumes];
  zStack = new slice*[numberVolumes];
  for (int i=0; i<numberVolumes; i++){
    xStack[i] = new slice[numberSlices];
    yStack[i] = new slice[numberSlices];
    zStack[i] = new slice[numberSlices];
    // create the storage
    for (int j=0; j<numberSlices; j++){
      xStack[i][j].allocate(128,128);
      yStack[i][j].allocate(128,128);
      zStack[i][j].allocate(128,128);
    }
  }
}

void fillTexel(char* data, char value){
  float r,g,b;
  // Color palette: map value to rgb
  // Avoid blue, so a blue background can sit behind the volume.
  float br = value / 255.;
  const float threshold = 0.02; // should be adjustable by user, 0 to 0.5.
  if (br < threshold)
    {
    r = g = b = 0.; // black, i.e., transparent.
    }
  else
    {
    const float edge = 0.1; //;; 0.48;
    if (false /* voxel's value exceeds baseline value */)
      {
      // Dark gray to white.  Larger exponents darken the grays.
      r = g = b = pow(.2 + .7 * (br / edge), 1.4);
      }
    else
      {
      // 0 to edge: red
      // edge to 1:  red to yellow, with another edge in it.
      br = (br - edge) / (1-edge);  // remap to 0..1
      if (br < 0)
	{
	r = .7; g = 0; b = .08;
	}
      else
	{
	r = .7 + (1-.7) * br;
	const float edgeYellow = 0.3; //;; .62;
	g = br*.6;
	if (br > edgeYellow)
	  g = (br-edgeYellow)/(1-edgeYellow) * .25 + .75;
	b = .4 * (1-br);
	}
      }
    }

  const float noclip = 0.5; // avoid clipping color to (255,255,255)
  // try squaring the values, for more midtones
  *data++ = int(r*r * 255. * noclip);
  *data++ = int(g*g * 255. * noclip);
  *data++ = int(b*b * 255. * noclip);
  *data = 255; // transparent under our scheme for doing blending
}

void fillTextures(){
  int x,y,z;
  for (int whichVolume =0; whichVolume<numberVolumes; whichVolume++){
    for (int i=0; i<numberSlices; i++){
      for (int j=0; j<sliceWidth; j++){
        for (int k=0; k<sliceHeight;k++){
	  // fill in the data. we treat i,j,k as voxel coordinates

          // slices stacked along the x axis
          // in this case, i = x coord, j = z coord, k = y coord
          x = int((numberSlices-1-i)*((volumeX*1.0)/numberSlices));
          y = int(k*((volumeY*1.0)/sliceHeight));
          z = int(j*((volumeZ*1.0)/sliceWidth));
          fillTexel(xStack[whichVolume][i].getPtr()+4*(sliceHeight*j+k),
            volume[whichVolume][(z * volumeY + y) * volumeX + x]);

	  // slices stacked along the y axis
          // in this case, i = y coord, j = z, k = x
          y = int(i*(float(volumeY)/numberSlices));
          x = int(k*(float(volumeX)/sliceHeight));
          z = int(j*(float(volumeZ)/sliceWidth));
          fillTexel(yStack[whichVolume][i].getPtr()+4*(sliceHeight*j+k),
            volume[whichVolume][(z * volumeY + y) * volumeX + x]);

          // slices stacked along the z axis
          // in this case, i = z, j = x, k = y;
          z = int((numberSlices-1-i)*((volumeZ*1.0)/numberSlices));
          y = int(k*(float(volumeY)/sliceHeight));
          x = int(j*(float(volumeX)/sliceWidth));
          fillTexel(zStack[whichVolume][i].getPtr()+4*(sliceHeight*j+k),
            volume[whichVolume][(z * volumeY + y) * volumeX + x]);
        }
      }
    }
  }
}

bool fillVolume(){
  for (int i=0; i<numberVolumes; i++){
    char dataFile[256];
    sprintf(dataFile,"cavebrain.%i.img",i);
    FILE* fd = ar_fileOpen(dataFile, "volume", dataPath, "rb");
    if (!fd){
      cerr << "error: failed to open data file \""
           << dataFile << "\" in path \""
	   << dataPath << "\".\n";
      return false;
    }
    fread(volume[i], 1, volumeX*volumeY*volumeZ, fd);
    ar_fileClose(fd);
  }

#ifdef UNUSED
  // Visual guide: mark the boundary of the volume.
  for (int z=0; z<volumeZ; z++)
  for (int y=0; y<volumeY; y++)
  for (int x=0; x<volumeX; x++)
    {
    const int a = x==0 || x==volumeX-1 ? 1 : 0;
    const int b = y==0 || y==volumeY-1 ? 1 : 0;
    const int c = z==0 || z==volumeZ-1 ? 1 : 0;
    if (a+b+c >= 2) // edge
      volume[(z * volumeY + y) * volumeX + x] = char(90);
    else if (a+b+c >= 1) // face
      volume[(z * volumeY + y) * volumeX + x] = char(7);
    }
#endif
  return true;
}

void setUpWorld(){
#ifdef DISABLED
  arCubeEnvironment bigCube;
  bigCube.setHeight(200);
  bigCube.setRadius(180);
  bigCube.setOrigin(0,5,0);
  bigCube.setNumberWalls(4);

  bigCube.setWallTexture(0,"fMRI.ppm");
  bigCube.setWallTexture(1,"fMRI.ppm");
  bigCube.setWallTexture(2,"fMRI.ppm");
  bigCube.setWallTexture(3,"fMRI.ppm");
  bigCube.setWallTexture(4,"fMRI.ppm"); // ceiling
  bigCube.setWallTexture(5,"fMRI.ppm"); // floor
  bigCube.attachMesh("room","root");
#endif
}


void showRasterString(int x, int y, char* s){
  glRasterPos2f(x, y);
  char c;
  for (; (c=*s) != '\0'; ++s)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
}

bool init(arMasterSlaveFramework& fw, arSZGClient& cli){
  dataPath = cli.getAttribute("SZG_DATA","path");
  if (dataPath == "NULL"){
    cerr << cli.getLabel() << " error: SZG_DATA/path undefined.\n";
    return false;
  }
  if (!fillVolume())
    return false;

  // note that the texture filling calls need to occur *after* we've read
  // the volumes in from disk
  allocTextures();
  fillTextures();

  setUpWorld();
  // initial viewpoint location
  ar_navTranslate( arVector3(0,-5,5) );

  // must register the networked shared memory
  fw.addTransferField("world",world.v,AR_FLOAT,16);
  fw.addTransferField("currentVolume",&currentVolume,AR_INT,1);
  return true;
}

void preExchange(arMasterSlaveFramework& fw){
  if (!fw.getMaster())
    return;

  const arMatrix4 headMatrix = fw.getMatrix(0);
  const arMatrix4 wandMatrix = fw.getMatrix(1);
  const float caveJoystickY = fw.getAxis(1);
  const int caveButton1 = fw.getButton(0);
  const int caveButton2 = fw.getButton(1);
  const int caveButton3 = fw.getButton(2);

  // need timestamp-based navigation, i.e. scaling for framerate!

  static bool grabState = false;
  static bool probeState = false;
  static bool readyToChangeVolume = true;
  static arMatrix4 grabMatrix;

  // Get location of wand, from which pings will emanate.
  const arVector3 wand(wandMatrix.v[12], wandMatrix.v[13], wandMatrix.v[14]);

  if (caveButton1 && readyToChangeVolume){
#ifdef OBSOLETE
    // tick beep: next frame
#endif
    currentVolume = (currentVolume+1)%numberVolumes;
    readyToChangeVolume = false;
  }
  if (!caveButton1){
    readyToChangeVolume = true;
  }
  if (fabs(caveJoystickY)>0.4){
    ar_navTranslate(
      ar_extractRotationMatrix(wandMatrix) *
      arVector3(0,0,-0.09*caveJoystickY/fabs(caveJoystickY)));
  }
  if (caveButton3 && !grabState){
#ifdef OBSOLETE
    // rising beep: grabbed
#endif
    grabState = true;
    grabMatrix = !world * ar_extractRotationMatrix(wandMatrix);
  }
  else if (caveButton3 && grabState){
    world = ar_extractRotationMatrix(wandMatrix) * !grabMatrix;
  }
  else if (!caveButton3 && grabState){
#ifdef OBSOLETE
    // falling beep: ungrabbed
#endif
    grabState = false;
  }
  if (caveButton2 && !probeState){
    probeState = true;
    cout << "Probe start!\n";
  }
  else if (caveButton2  && probeState){
    cout << "Probing!\n";
  }
  else if (!caveButton2 && probeState){
    probeState = false;
    cout << "Probe end!\n";
  }
}

void drawCallback(arMasterSlaveFramework& fw){
  arMatrix4 wandMatrix = fw.getMatrix(1);
  arMatrix4 headMatrix = fw.getMatrix(0);
  static float FPS = 20.;
  static float FPSPrev = 20.;
  static ar_timeval timePrev = ar_time();
  // ar_timeval time1 = ar_time();
  glClearColor(0,0,0,0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);

  if (fw.getConnected()) {
    glEnable(GL_DEPTH_TEST);

    // draw a wand
    glPushMatrix();
    glTranslatef(0,5,-4);
    const arMatrix4 m(ar_extractTranslationMatrix(wandMatrix));
    glMultMatrixf(m.v);
    glBegin(GL_LINES);
      glColor3f(1,0,0);
        glVertex3f(0,0,0);
        glVertex3f(0,0,-1);
      glColor3f(0,1,0);
        glVertex3f(0,0,0);
        glVertex3f(1,0,0);
      glColor3f(0,0,1);
        glVertex3f(0,0,0);
        glVertex3f(0,1,0);
    glEnd();
    glPopMatrix();

    fw.loadNavMatrix();

    glPushMatrix();
      // DEPRECATED in this use, re-purposed by arGUI
      // fw.draw();
    glPopMatrix();
    glMultMatrixf(world.v);
    const float size = 2.0;

    // draw the volume
    arVector3 eyeVector =
      !world * ar_pointToNavCoords(headMatrix * arVector3(0,0,0));
    const float dirX = arVector3(1,0,0) % eyeVector;
    const float dirY = arVector3(0,1,0) % eyeVector;
    const float dirZ = arVector3(0,0,1) % eyeVector;
    int which = (fabs(dirZ) >= fabs(dirY) && fabs(dirZ) >= fabs(dirX)) ? 0 :
                (fabs(dirY) >= fabs(dirZ) && fabs(dirY) >= fabs(dirX)) ? 1 : 2;
    glColor3f(0,0,0);  // Doesn't matter what this is.
    const float dirs[3] = { dirZ, dirY, dirX };
    const bool dir = dirs[which] >= 0.;
    // these 3 should be an array to begin with!
    slice* stacks[3] = { zStack[currentVolume], yStack[currentVolume],
                         xStack[currentVolume] };
    slice* stack = stacks[which];
    for (int i=0; i<numberSlices; i++){
      const int textureIndex = dir ? i : numberSlices-1-i;
      const float z = (textureIndex*2*size)/(numberSlices-1) - size;
      stack[textureIndex].activate();
      glBegin(GL_QUADS);
      if (which == 0){
        glTexCoord2f(1,0);
        glVertex3f(size,size,z);
        glTexCoord2f(1,1);
        glVertex3f(-size,size,z);
        glTexCoord2f(0,1);
        glVertex3f(-size,-size,z);
        glTexCoord2f(0,0);
        glVertex3f(size,-size,z);
      }
      else if (which == 1){
        glTexCoord2f(0,0);
        glVertex3f(size,z,size);
        glTexCoord2f(1,0);
        glVertex3f(-size,z,size);
        glTexCoord2f(1,1);
        glVertex3f(-size,z,-size);
        glTexCoord2f(0,1);
        glVertex3f(size,z,-size);
      }
      else{
        glTexCoord2f(1,0);
        glVertex3f(z,size,size);
        glTexCoord2f(0,0);
        glVertex3f(z,-size,size);
        glTexCoord2f(0,1);
        glVertex3f(z,-size,-size);
        glTexCoord2f(1,1);
        glVertex3f(z,size,-size);
      }
      glEnd();
      stack[textureIndex].deactivate();
    }

    // Draw wireframe box around the data volume.
    // After the volume's drawn, for cool hidden-line effect.
    glColor3f(1,1,1);
    glutWireCube(2.01*size);

#if 0
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,3000,0,3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glColor3f(1,1,0);
    char buf[100];
    sprintf(buf,"FPS = %5.1f %s which=%d",
      FPS,
      fw.getMaster() ? "MASTER" : "",
      which);
    showRasterString(100,300,buf);
#endif
  }
  const ar_timeval time2 = ar_time();
  FPSPrev = FPS;
  FPS = 1000000. / ar_difftime(time2, timePrev);
  FPS = .05 * FPS + .95 * FPSPrev;
  timePrev = time2;
}

int worldTransformID = -1;
int navTransformID = -1;

void playCallback(arMasterSlaveFramework& fw){
  if (!fw.getMaster() || !fw.soundActive())
    return;

  static int barID = -1;
  static int zipID = -1;
  static bool finitSound = false;
  if (!finitSound){
    finitSound = true;
    navTransformID = dsTransform("nav","root",ar_getNavInvMatrix());

    const float xyz[3] = { 0, 0, 0 };
      barID = dsLoop("bar", "nav", "q33move.mp3", 1, 0.03, xyz);
      zipID = dsLoop("zip", "nav", "q33collision.wav", 0, 0.0, xyz);
      (void)dsLoop("unchanging", "nav", "q33beep.wav", 1, 0.1, xyz);
  }

  // Trigger sounds, if needed.
  const float xyz[3] = { 0,0,0 };
  static bool fReset = true;
  if (rand() % 800 == 0){
    dsLoop(zipID, "q33collision.wav", -1, .00008, xyz);
    fReset = false;
  }
  else if (!fReset){
    dsLoop(zipID, "q33collision.wav",  0, 0, xyz);
    fReset = true;
  }

  // Periodically update background sounds.
  static struct ar_timeval tPrev = ar_time();
  struct ar_timeval tNow = ar_time();
  if (ar_difftime(tNow, tPrev) > 0.05e6){
    tPrev = tNow;
    float xyz[3] = { 0,0,0 };
    static float v = 0.1; v += 0.007; if (v>=1.) v=0.1;
    const float moveAmpl = 0.07 + 0.6 * v*v;
    dsLoop(barID, "q33move.mp3", 1, moveAmpl, xyz);
  }
}

/// \todo move dataPath into the framework, to avoid referencing arSZGClient explicitly.

int main(int argc, char** argv){
  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setStartCallback(init);
  framework.setPreExchangeCallback(preExchange);
  framework.setDrawCallback(drawCallback);
  framework.setPlayCallback(playCallback);
  framework.setEyeSpacing(6/(12*2.54)); // 6 cm
  return framework.start() ? 0 : 1;
}
