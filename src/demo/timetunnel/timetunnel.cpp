//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "TimeTunnelLayout.h"
#include "arInterfaceObject.h"
#include "arDistSceneGraphFramework.h"
#include "arGraphicsAPI.h"
#include "arSoundAPI.h"

TimeTunnelLayout* theLayout = NULL;
arMutex navLock;

void dataReader(void*){
  while (true) {
    ar_mutex_lock(&navLock);
      theLayout->animateLayout();
    ar_mutex_unlock(&navLock);
    // Yield lock occasionally.
    ar_usleep(1);
  }
}

int main(int argc, char** argv){
  arDistSceneGraphFramework framework;
  arInterfaceObject interfaceObject;

  if (!framework.init(argc,argv)){
    return 1;
  }
  interfaceObject.setInputDevice(framework.getInputDevice());
  ar_mutex_init(&navLock);

  const string navNodeName = framework.getNavNodeName();
  int worldTransformID[2], navTransformID[2];
//  navTransformID[0] = dgTransform("nav","root",ar_identityMatrix());
//  navTransformID[1] = dsTransform("nav","root",ar_identityMatrix());
  worldTransformID[0] = dgTransform("world",navNodeName,ar_identityMatrix());
  worldTransformID[1] = dsTransform("world",navNodeName,ar_identityMatrix());
  theLayout = new TimeTunnelLayout(3,10,32,42,framework.getDataPath());

  // Configure stereo view.
  framework.setEyeSpacing( 6/(2.54*12) );
  framework.setClipPlanes( .1, 500. );
  framework.setUnitConversion( 1. );

  if (!framework.start())
    return 1;
  if (!interfaceObject.start())
    return 1;

  arThread dummy(dataReader);

  // Place scene in the center of the front wall.
  interfaceObject.setNavMatrix(ar_translationMatrix(0,5,0));
  interfaceObject.setSpeedMultiplier(0.15);

  const arVector3 xyz(0,0,0);
  // This background music is loud and annoying...
  // Actually, indirectly, this points out a shortcoming in our
  // sound infrastructure... loops are always spatialized!
  // (sometimes they should be non-spatialized... just ambient)
  (void)dsLoop("unchanging", "world", "timetunnel.mp3", 1, 0.2, xyz);

  while (true) {
    const arMatrix4& navTransform = interfaceObject.getNavMatrix();
    const arMatrix4& worldTransform = interfaceObject.getObjectMatrix();
    ar_setNavMatrix( navTransform.inverse() );
    ar_mutex_lock(&navLock);
//      dgTransform(navTransformID[0],navTransform);
//      dsTransform(navTransformID[1],navTransform);
      framework.loadNavMatrix();
      dgTransform(worldTransformID[0],worldTransform);
      dsTransform(worldTransformID[1],worldTransform);
      framework.setViewer();
      framework.setPlayer();
    ar_mutex_unlock(&navLock);
    ar_usleep(1000000/100); // 100 fps cpu throttle
  } 
  return 0;
}
