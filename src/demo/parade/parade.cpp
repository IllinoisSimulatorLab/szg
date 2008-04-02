//***************************************************************************
//Originally written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDistSceneGraphFramework.h"
#include "arMesh.h"
#include "arSoundAPI.h" // put this someplace better.  like arGraphicsAPI.h is included in arMesh.h.
#include "Commando.h"

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

int main(int argc, char** argv){
  arDistSceneGraphFramework fw;
  if (!fw.init(argc, argv))
    return 1;

  // Init world.

  dgLight("light0", "root", 0, arVector4(0,0, 1,0), arVector3(1,1,1)); // point down, -z
  dgLight("light1", "root", 1, arVector4(0,0,-1,0), arVector3(1,1,1)); // point down, +z

  // Only for graphics, not for sound.
  (void)dgTransform("local0", fw.getNavNodeName(), ar_translationMatrix(4, 5, -4));
  (void)dgTransform("local1", "local0", ar_translationMatrix(0, 0, -2));

  const int numCommando = 8;
  ARfloat color[numCommando][3]= {
    {0.5, 0.5, 0.9 },
    {0.5, 0.9, 0.5 },
    {0.9, 0.5, 0.5 },
    {0.5, 0,   0.5 },
    {0.7, 0.7, 0   },
    {0.7, 0,   0.75},
    {0,   0.7, 0.75},
    {0.7, 1,   0.75}
  };
  const float paces[numCommando] = { .99, .04, .08, .12, .99, .04, .08, .12 };

  Commando* commando[numCommando];
  int i;
  for (i=0; i<numCommando; ++i) {
    commando[i] = new Commando('y', color[i]);
    commando[i]->attachMesh("commando " + ar_intToString(i),
      i<4 ? "local0" : "local1");
    commando[i]->pace(paces[i], 80, 0.10);
  }

  fw.setNavTransSpeed(4.);
  fw.setEventQueueCallback( inputEventQueueCallback );
  fw.ownNavParam( "translation_speed" );
  if (!fw.start())
    return 1;

  // Disembodied hum coming from the floor, as an approximation to everywhere.
  // Use .wav not .mp3, to avoid a click when the loop wraps around.
  (void)dsLoop("unchanging", fw.getNavNodeName(), "parade.wav", 1, 0.6, arVector3(0,-5,0));

  while (true) {
    ar_usleep(1000000/45);  // 45 fps cpu throttle
    fw.processEventQueue(); // Invoke inputEventQueueCallback.
    // Alter world.
    for (i=0; i<numCommando; ++i)
      commando[i]->execute();
    fw.setViewer();
    fw.setPlayer();
  }
  return 0;
}
