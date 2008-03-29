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
const static arMatrix4 localMatrix[2] = {
  ar_translationMatrix(4, 5, -4),
  ar_translationMatrix(4, 5, -6) };

int localTransformID[2];

bool inputEventQueueCallback( arSZGAppFramework& fw, arInputEventQueue& eventQueue ) {
  while (!eventQueue.empty()) {
    const arInputEvent event = eventQueue.popNextEvent();
    const arInputEventType t = event.getType();
    // navUpdate with axis events for forward/backward/speed,
    // and with matrix events to know which way the wand points.
    switch (t) {
    case AR_EVENT_MATRIX:
    case AR_EVENT_AXIS:
      fw.navUpdate( event );
      break;
    default:
      break;
    }
  }
  // The win32 joystick driver might not stream
  // input events while the joystick is held steady.
  // So update the nav matrix each frame.
  fw.loadNavMatrix();
  return true;
}

void worldInit(arDistSceneGraphFramework& framework){
  // Directional lights.
  dgLight("light0", "root", 0, arVector4(0,0, 1,0), arVector3(1,1,1)); // point down, -z
  dgLight("light1", "root", 1, arVector4(0,0,-1,0), arVector3(1,1,1)); // point down, +z

  // Only for graphics, not for sound.
  const string navNodeName = framework.getNavNodeName();
  localTransformID[0] = dgTransform("local0", navNodeName, localMatrix[0]);
  localTransformID[1] = dgTransform("local1", navNodeName, localMatrix[1]);

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

  for (int i=0; i<numCommando; ++i) {
    commando[i] = new Commando('y', color[i]);
    commando[i]->attachMesh("commando " + ar_intToString(i),
      i<4 ? "local0" : "local1");
    commando[i]->pace(paces[i], 80, 0.10);
  }
}

void worldAlter(void* f)
{
  arDistSceneGraphFramework* framework = (arDistSceneGraphFramework*) f;
  // Invoke inputEventQueueCallback.
  framework->processEventQueue();

  for (int i=0; i<numCommando; ++i)
    commando[i]->execute();
  dgTransform(localTransformID[0], localMatrix[0]);
  dgTransform(localTransformID[1], localMatrix[1]);
}

int main(int argc, char** argv){
  arDistSceneGraphFramework framework;
  if (!framework.init(argc, argv))
    return 1;

  framework.setNavTransSpeed(4.);
  worldInit(framework);
  framework.setEventQueueCallback( inputEventQueueCallback );
  framework.ownNavParam( "translation_speed" );
  if (!framework.start())
    return 1;

  // Disembodied hum coming from the floor, as an approximation to everywhere.
  // Use .wav not .mp3, to avoid a click when the loop wraps around.
  const arVector3 xyz(0,-5,0);
  (void)dsLoop("unchanging", framework.getNavNodeName(), "parade.wav", 1, 0.6, xyz);

  while (true) {
    ar_usleep(1000000/30); // 30 fps cpu throttle
    worldAlter(&framework);
    framework.setViewer();
    framework.setPlayer();
  }
  return 0;
}
