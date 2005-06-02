//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMasterSlaveFramework.h"
#include "arMath.h"
#include "arMesh.h"
#include "arSZGClient.h"
#include "arThread.h"
#include "arGraphicsAPI.h"
#include "HypDodecahedron.h"
#include "H3World.h"
#ifndef AR_USE_WIN_32
#include <sys/types.h>
#include <signal.h>
#endif

string interfaceType;
bool stereoMode;
int windowSizeX, windowSizeY;
arMatrix4 hyperTransform;
arMutex hyperLock;

arMatrix4 worldMatrix;
int worldTransformID;

H3World *worldH3 = NULL;

arMatrix4 hyperRotate(char axis, float radians)
{
  arMatrix4 result;
  switch (axis)
  {
  case 'x':
    result[5]  =  cos(radians);
    result[6]  =  sin(radians);
    result[9]  = -sin(radians);
    result[10] =  cos(radians);
    break;
  case 'y':
    result[0]  =  cos(radians);
    result[2]  =  sin(radians);
    result[8]  =  -sin(radians);
    result[10] =  cos(radians);
    break;
  case 'z':
    result[0] =  cos(radians);
    result[1] =  sin(radians);
    result[4] = -sin(radians);
    result[5] =  cos(radians);
    break;
  }
  return result;
}

arMatrix4 hyperTranslate(float speed)
{
  arMatrix4 result;
  result[10] = result[15] = cosh(speed);
  result[11] = result[14] = sinh(speed);
  return result;
}

//***********************************************************************
// arServerFramework callbacks
//***********************************************************************

bool init(arMasterSlaveFramework& fw, arSZGClient&){
  fw.addTransferField("hyperTransform",hyperTransform.v,AR_FLOAT,16);
  return true;
}

void preExchange(arMasterSlaveFramework& fw){
  const float j0 = fw.getAxis(0);
  const float j1 = fw.getAxis(1);
  // The time it took to draw the last frame... in milliseconds.
  float frametime = fw.getLastFrameTime();
  hyperTransform =
    hyperTranslate(frametime*j1/3200.0) * hyperTransform 
    * hyperRotate('y',frametime*j0/3200.0);
}

void postExchange(arMasterSlaveFramework&){
  if (!worldH3)
    return;
  // Convert from floats to doubles.
  double m[16];
  for (int i=0; i<16; i++)
    m[i] = double(hyperTransform[i]);
  worldH3->update(m);
}

void drawCallback(arMasterSlaveFramework& fw){
  if (fw.getConnected()) {
    glTranslatef(0,5,0);
    glScalef(10,10,10);
    fw.drawGraphicsDatabase();
  }
}

int main(int argc, char** argv){
  arMasterSlaveFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setStartCallback(init);
  framework.setPreExchangeCallback(preExchange);
  framework.setDrawCallback(drawCallback);
  framework.setPostExchangeCallback(postExchange);
  framework.setClipPlanes( .1, 1000 );
  framework.setEyeSpacing(6/(12*2.54));

  // Initialize the application.
  worldTransformID = dgTransform("world","root",worldMatrix);
  worldH3 = new H3World;
  worldH3->attachMesh("H3 world", "world"); 

  return framework.start() ? 0 : 1;
}
