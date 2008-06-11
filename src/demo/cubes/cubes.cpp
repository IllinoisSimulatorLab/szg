//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

//*********************************************************************
// Display a large number of randomly rotating cubes.
//*********************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDistSceneGraphFramework.h"
#include "arMesh.h"
#include "arSoundAPI.h" // put this someplace better.  like arGraphicsAPI.h is included in arMesh.h.

#include "arInteractionUtilities.h"
#include "arCallbackInteractable.h"

const int NUMBER_OBJECTS = 200;

// Uses matrix 0 (the head matrix).
// Has no other input events.
arEffector headEffector( 0, 0, 0, 0, 0, 0, 0 );

// Uses matrix 1.
// Has no axes.
// Has 6 buttons mapped from input button 2 to output button 2, so you can
// get buttons 2-7 via this arEffector using their input indices.
arEffector dragWand( 1, 6, 2, 2, 0, 0, 0 );

bool fStatic = false;
bool fTeapot = false;
bool fPython = false;

std::list<arInteractable*> interactionList;
arCallbackInteractable interactionArray[NUMBER_OBJECTS];
int objectTextureID[NUMBER_OBJECTS];
int wandID = -1;
int soundTransformID = -1;
int teapotID = -1;
int teapotTransformID = -1;
int pythonID = -1;
int pythonTransformID = -1;
float teapotAngle = 0.;
int cube0SoundTransformID = -1;
arGraphicsStateValue lightingOnOff(AR_G_TRUE);
int lightsOnOffID;
const float WAND_LENGTH = 2.;
arMatrix4 cube0Matrix;
long randomSeed = 0;

arLock databaseLock; // Guards all dgXXX() and dsXXX() alterations to the scene graph?

inline float randDistance(){
  return (float(rand() % 1001) / 1000. - 0.5) * .03;
}

// Caller must lock databaseLock.
void drawWand( const arEffector* effector ) {
  const arMatrix4 wandOffsetMat( ar_translationMatrix( arVector3( 0,0,-0.5*WAND_LENGTH ) ) );
  dgTransform( wandID, effector->getBaseMatrix()*wandOffsetMat );
}

bool inputEventQueueCallback( arSZGAppFramework& fw, arInputEventQueue& eventQueue ) {
  arGuard dummy(databaseLock);
  while (!eventQueue.empty()) {
    const arInputEvent event = eventQueue.popNextEvent();
    dragWand.updateState( event );
    headEffector.updateState( event );
    const arInputEventType t = event.getType();
    // navUpdate with axis events for forward/backward/speed,
    // and with matrix events to know which way the wand points.
    switch (t) {
    case AR_EVENT_MATRIX:
    case AR_EVENT_AXIS:
      fw.navUpdate( event );
      break;
    case AR_EVENT_BUTTON:
      if (event.getIndex() == 0 && fw.getOnButton(0)) {
        lightingOnOff = lightingOnOff==AR_G_TRUE ? AR_G_FALSE : AR_G_TRUE;
        dgStateInt( lightsOnOffID, "lighting", lightingOnOff );
      }
      break;
    default:
      break;
    }
  }
  // The win32 joystick driver might not stream
  // input events while the stick is held steady.
  // So update the nav matrix each frame.
  fw.loadNavMatrix();
  dragWand.draw();
  return true;
}

void setMatrixCallback( arCallbackInteractable* object, const arMatrix4& matrix ) {
  arGuard dummy(databaseLock);
  const int gid = object->getGraphicsTransformID();
  if (gid != -1) {
    dgTransform( gid, matrix );
  }
  const int sid = object->getSoundTransformID();
  if (sid != -1) {
    dsTransform( sid, matrix );
  }
}

bool processCallback( arCallbackInteractable* object, arEffector* effector ) {
  if (effector && effector == &headEffector && object && object->grabbed()) {
    const int iObject = object - interactionArray;
    if (iObject >= 0 && iObject < NUMBER_OBJECTS) {
      arGuard dummy(databaseLock);
      dgTexture( objectTextureID[iObject], "ambrosia.ppm" );
    }
  }
  return true;
}

float teapotColor[] = {.5,.5,.5,1.};

void worldAlter(void* f) {
  arDistSceneGraphFramework* framework = (arDistSceneGraphFramework*) f;
  framework->externalThreadStarted();
  int count = 0;
  while (!framework->stopping()) {
    if (++count%100 == 0)
      ar_usleep(1000); // CPU throttle

    // Change zeroth cube.  (Lissajous motion.)
    cube0Matrix = ar_translationMatrix(6.*cos(count*.00001),
                                       3.*cos(count*.0000141)+5.,
                                       6.*cos(count*.0000177));
    interactionArray[0].setMatrix( cube0Matrix );

    if (!fStatic) {
      // Jiggle any cube except the zeroth.
      const int iObject = 1 + rand() % (NUMBER_OBJECTS-1);
      interactionArray[iObject].setMatrix(
        interactionArray[iObject].getMatrix() *
        ar_translationMatrix(randDistance(), randDistance(), randDistance()) *
	ar_rotationMatrix("xyz"[rand()%3], 0.02));

      if (count%20 == 0){
        char buffer[32];
        sprintf(buffer,"WallTexture%i.ppm",rand()%4+1);
        const string whichTexture(buffer);
	arGuard dummy(databaseLock);
        dgTexture(objectTextureID[iObject], whichTexture);
      }
    }

    if (teapotID != -1) {
      teapotColor[0] = .5*sin(count*.0001)+.5;
      teapotColor[1] = .5*cos(count*.000141)+.5;
      teapotColor[2] = .5*sin(count*.000177)+.5;
      dgPlugin( teapotID, "arTeapotGraphicsPlugin", NULL, 0, teapotColor, 4, NULL, 0, NULL, 0, NULL );
      teapotAngle -= .001;
      if (teapotAngle < 0.) {
        teapotAngle += 360.;
      }
      dgTransform( teapotTransformID, ar_rotationMatrix('y',ar_convertToRad(teapotAngle))
          *ar_translationMatrix(0,5,-5) );
    }
    if (pythonID != -1) {
      teapotColor[0] = .5*sin(count*.0001)+.5;
      teapotColor[1] = .5*cos(count*.000141)+.5;
      teapotColor[2] = .5*sin(count*.000177)+.5;
      dgPython( pythonID, "pyteapot", "teapot", false, NULL, 0, teapotColor, 4, NULL, 0, NULL, 0, NULL );
      teapotAngle += .001;
      if (teapotAngle > 360.) {
        teapotAngle -= 360.;
      }
      dgTransform( pythonTransformID, ar_rotationMatrix('y',ar_convertToRad(teapotAngle))
          *ar_translationMatrix(0,5,-5) );
    }

    // necessary if we're using the inputEventQueueCallback to process buffered
    // events in batches (as opposed to inputEventCallback, which handles each
    // event as it comes in).
    framework->processEventQueue();

    // Cube-dragging.
    ar_pollingInteraction( dragWand, interactionList );
    ar_pollingInteraction( headEffector, interactionList );
  }

  ar_log_remark() << ": alteration thread stopped.\n";
  framework->externalThreadStopped();
}

bool worldInit(arDistSceneGraphFramework& framework) {
  dgLight("light0", "root", 0, arVector4(0,0,1,0),  arVector3(1,1,1));
  dgLight("light1", "root", 1, arVector4(0,0,-1,0), arVector3(1,1,1));
  dgLight("light2", "root", 2, arVector4(0,1,0,0),  arVector3(1,1,1));
  dgLight("light3", "root", 3, arVector4(0,-1,0,0), arVector3(1,1,1));
  
  const string navNodeName = framework.getNavNodeName();
  lightsOnOffID = dgStateInt( "light_switch", navNodeName, "lighting", AR_G_TRUE );
  soundTransformID = dsTransform( "sound_transform", navNodeName, arMatrix4() );
  wandID = dgTransform( "wand_transform", "light_switch", arMatrix4() );
  dgTexture( "wand_texture", "wand_transform", "ambrosia.ppm" );
  arCubeMesh theCube;
  theCube.setTransform(ar_scaleMatrix(.2,.2,WAND_LENGTH));
  theCube.attachMesh( "wand", "wand_texture" );
  
  arSphereMesh theSphere(12);
  arPyramidMesh thePyramid;
  arCylinderMesh theCylinder;
  theCylinder.setAttributes(20,1,1);
  theCylinder.toggleEnds(true);
  const string baseName("foo");
  for (int i=0; i<NUMBER_OBJECTS; ++i) {
    const string objectName(baseName + ar_intToString(i));
    const string objectTexture(objectName + " texture");
    const string objectParent(objectName + " transform");
    const string whichTexture(i==0 ? "YamahaStar.ppm" :
      "WallTexture" + ar_intToString(rand()%4+1) + ".ppm");
recalcpos:
    const float randX = -5. + (10.*(rand()%200))/200.0;
    const float randY = (10.*(rand()%200))/200.0;
    const float randZ = -5. + (10.*(rand()%200))/200.0;
    if (arVector3(randX,randY-5,randZ).magnitude() < 1.5) {
      goto recalcpos;
    }

    const arMatrix4 cubeTransform(ar_translationMatrix(randX,randY,randZ));
    arCallbackInteractable cubeInteractor( 
      dgTransform( objectParent, "light_switch", arMatrix4() ) );
    cubeInteractor.setMatrixCallback( setMatrixCallback );
    cubeInteractor.setMatrix( cubeTransform );
    cubeInteractor.setProcessCallback( processCallback );
    interactionArray[i] = cubeInteractor;
    if (i==0) {
      cube0SoundTransformID = dsTransform( objectParent, navNodeName, arMatrix4() );
      cubeInteractor.setSoundTransformID( cube0SoundTransformID );
    } else {
      // Don't drag zeroth cube, which has its own motion.
      interactionList.push_back( (arInteractable*)(interactionArray+i) );
    }

    objectTextureID[i] = dgTexture(objectTexture,objectParent,whichTexture);

    const float radius = i==0 ? 2.5 : .28 + .1*(rand()%200)/200.0;
    const float whichShape = i==0 ? 1. : (rand()%200)/200.0;
    if (whichShape < 0.2){
      theSphere.setTransform(ar_scaleMatrix(radius));
      if (!theSphere.attachMesh(objectName, objectTexture))
	return false;
    }
    else if (whichShape < 0.4){
      theCylinder.setTransform(ar_scaleMatrix(radius));
      if (!theCylinder.attachMesh(objectName, objectTexture))
	return false;
    }
    else if (whichShape < 0.6){
      thePyramid.setTransform(ar_scaleMatrix(radius));
      if (!thePyramid.attachMesh(objectName, objectTexture))
	return false;
    }
    else{
      theCube.setTransform(ar_scaleMatrix(radius));
      if (!theCube.attachMesh(objectName, objectTexture))
	return false;
    }
  }

  if (fTeapot) {
    teapotTransformID = dgTransform( "teapot_transform", "light_switch", ar_translationMatrix(0,5.,-5.) );
    teapotID = dgPlugin( "teapot", "teapot_transform", "arTeapotGraphicsPlugin",
                          NULL, 0, teapotColor, 4, NULL, 0, NULL, 0, NULL );
    if (teapotID == -1) {
      ar_log_error() << " failed to create teapot plugin node.\n";
      fTeapot = false;
      return false;
    }
    ar_log_remark() << " created teapot plugin node with ID " << teapotID << ar_endl;
  }

  if (fPython) {
    pythonTransformID = dgTransform("python_transform", "light_switch", ar_translationMatrix(0,5.,-5.) );
    pythonID = dgPython( "python", "python_transform", "pyteapot", "teapot", false,
        NULL, 0, teapotColor, 4, NULL, 0, NULL, 0, NULL );
    if (pythonID == -1) {
      ar_log_error() << " failed to create python plugin node.\n";
      fPython = false;
      return false;
    }
    ar_log_remark() << " created python plugin node with ID " << pythonID << ar_endl;
  }
  return true;
}

int main(int argc, char** argv) {
  for (int i=1; i < argc; ++i) {
    fStatic |= !strcmp(argv[i], "-static"  ); // Don't change cubes' position.
    fTeapot |= !strcmp(argv[i], "-teapot"  ); // Load and display the teapot plugin
    fPython |= !strcmp(argv[i], "-pyteapot"); // Load and display a python module plugin.
  }
  
  arDistSceneGraphFramework framework;

//  framework.setUnitConversion(3.);
//  framework.setUnitSoundConversion(3.);
  
  if (!framework.init(argc,argv))
    return 1;

  framework.setNavTransSpeed(3.);
  
  // Configure stereo view.
  framework.setEyeSpacing( 6/(2.54*12) );
  framework.setClipPlanes( .2, 100. );
//  framework.setEventCallback( inputEventCallback );
  framework.setEventQueueCallback( inputEventQueueCallback );
  // Framework should halt the worldAlter thread.
  framework.useExternalThread();

  // set max interaction distance at 5 ft.
  dragWand.setInteractionSelector( arDistanceInteractionSelector( 5 ) );
  // Set wand to do a normal drag when you click on button 2 or 6 & 7
  // (the wand triggers)
  dragWand.setDrag( arGrabCondition( AR_EVENT_BUTTON, 2, 0.5 ), arWandRelativeDrag() );
  dragWand.setDrag( arGrabCondition( AR_EVENT_BUTTON, 6, 0.5 ), arWandRelativeDrag() );
  dragWand.setDrag( arGrabCondition( AR_EVENT_BUTTON, 7, 0.5 ), arWandRelativeDrag() );
  dragWand.setTipOffset( arVector3(0,0,-WAND_LENGTH) );
  dragWand.setDrawCallback( drawWand );
  
  headEffector.setInteractionSelector( arDistanceInteractionSelector( 2 ) );

  randomSeed = -long(ar_time().usec);
  if (!worldInit(framework) || !framework.start())
    return 1;
    
  // Not working yet.
//  framework.setWorldRotGrabCondition( AR_EVENT_BUTTON, 0, 0.5 );

  arThread dummy(worldAlter, &framework);

  // Attach sound to big yellow box with star on it (cube0)
  (void)dsLoop("foo", "cube0 transform", "cubes.mp3", 1, 0.9, arVector3(0,0,0));

  while (true) {
    ar_usleep(1000000/200); // 200 fps cpu throttle
    arGuard dummy(databaseLock);
    framework.setViewer();
    framework.setPlayer();
  }
  return 0;
}  
