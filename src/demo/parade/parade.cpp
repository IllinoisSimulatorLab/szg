//***************************************************************************
//Originally written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDistSceneGraphFramework.h"
#include "arInterfaceObject.h"
#include "arMesh.h"
// put this someplace better.  like arGraphicsAPI.h is included in arMesh.h.
#include "arSoundAPI.h" 
#include "Commando.h"

const int numCommando = 8;
Commando* commando[numCommando];
arMatrix4 localMatrix[2];
int worldTransformID[2], navTransformID[2];
int localTransformID[2];

void worldInit(arDistSceneGraphFramework& framework){
  ARfloat color[numCommando][3]= {
    {0.5,  0.5,  0.9 },
    {0.5,  0.9,  0.5 },
    {0.9,  0.5,  0.5 },
    {0.5,  0,    0.5 },
    {0.75, 0.75, 0   },
    {0.75, 0,    0.75},
    {0,    0.75, 0.75},
    {0.75, 1,    0.75}
  };
  const float paces[numCommando] = { .99, .04, .08, .12, .99, .04, .08, .12 };
  const string navNodeName( framework.getNavNodeName() );
  // two directional lights on pointing down negative z and the other
  // pointed down positive z
  dgLight("light0", "root", 0, arVector4(0,0,1,0), arVector3(1,1,1));
  dgLight("light1", "root", 1, arVector4(0,0,-1,0), arVector3(1,1,1));
//  navTransformID[0] = dgTransform("nav","root",ar_identityMatrix());
//  navTransformID[1] = dsTransform("nav","root",ar_identityMatrix());
  worldTransformID[0] = dgTransform("world",navNodeName,ar_identityMatrix());
  worldTransformID[1] = dsTransform("world",navNodeName,ar_identityMatrix());

  // Only for graphics, not for sound.
  localTransformID[0] = dgTransform("local0","world",localMatrix[0]);
  localTransformID[1] = dgTransform("local1","world",localMatrix[1]);

  for (int i=0; i<numCommando; ++i) {
    commando[i] = new Commando('y', color[i]);
    char buf[20];
    sprintf(buf, "commando %d", i);
    commando[i]->attachMesh(buf, i<4 ? "local0" : "local1");
    commando[i]->pace(paces[i], 80, 0.10);
  }
}

void worldAlter()
{
  localMatrix[0]=ar_translationMatrix(4, 5, -4);
  localMatrix[1]=ar_translationMatrix(4, 5, -6);
  for (int i=0; i<numCommando; ++i)
    commando[i]->execute();
  dgTransform(localTransformID[0], localMatrix[0]);
  dgTransform(localTransformID[1], localMatrix[1]);
}

int main(int argc, char** argv){
  arDistSceneGraphFramework framework;
  arInterfaceObject interfaceObject;
  if (!framework.init(argc, argv))
    return 1;

  interfaceObject.setInputDevice(framework.getInputNode());
  worldInit(framework);
  if (!framework.start() || !interfaceObject.start())
    return 1;

  // Disembodied hum coming from the floor, as an approximation to everywhere.
  // Use .wav not .mp3, to avoid a click when the loop wraps around.
  const arVector3 xyz(0,-5,0);
  (void)dsLoop("unchanging", "root", "parade.wav", 1, 0.6, xyz);

  while (true) {
    const arMatrix4& navTransform = interfaceObject.getNavMatrix();
    const arMatrix4& worldTransform = interfaceObject.getObjectMatrix();
    ar_setNavMatrix( navTransform.inverse() );
    dgTransform(worldTransformID[0], worldTransform);
    dsTransform(worldTransformID[1], worldTransform);
    framework.loadNavMatrix();
//    dgTransform(navTransformID[0], navTransform);
//    dsTransform(navTransformID[1], navTransform);
    worldAlter();
    framework.setViewer();
    framework.setPlayer();
    ar_usleep(1000000/30); // 30 fps cpu throttle
  }
  return 0;
}
