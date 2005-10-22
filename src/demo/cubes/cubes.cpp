//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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
#include <list>

#define NUMBER_OBJECTS 200

// dragWand uses matrix 1, has 6 buttons mapped from input button 2
// to output button 2. This means that you can access input buttons
// 2-7 via this arEffector using their input indices. dragWand has 0 axes.
arEffector dragWand( 1, 6, 2, 2, 0, 0, 0 );

// headEffector urses matrix 0 (the head matrix), has no other input events.
arEffector headEffector( 0, 0, 0, 0, 0, 0, 0 );

bool jiggleStuffGlobal(true);

std::list<arInteractable*> interactionList;
arCallbackInteractable interactionArray[NUMBER_OBJECTS];
int objectTextureID[NUMBER_OBJECTS];
int wandID;
arGraphicsStateValue lightingOnOff(AR_G_TRUE);
int lightsOnOffID;
const float WAND_LENGTH = 2.;
arMatrix4 cube0Matrix;

arMutex databaseLock;

inline float randDistance(){
  return (float(rand() % 1001) / 1000. - 0.5) * .03;
}

void drawWand( const arEffector* effector ) {
  // already called inside a mutex block
//  ar_mutex_lock(&databaseLock);
  const arMatrix4 wandOffsetMat( ar_translationMatrix( arVector3( 0,0,-0.5*WAND_LENGTH ) ) );
    dgTransform( wandID, effector->getBaseMatrix()*wandOffsetMat );
//  ar_mutex_unlock(&databaseLock);
}

bool inputEventCallback( arSZGAppFramework& fw, arInputEvent& event, arCallbackEventFilter& filter ) {
  ar_mutex_lock(&databaseLock);
  dragWand.updateState( event );
  headEffector.updateState( event );
  fw.navUpdate( event );
  // Currently, the joystick driver on windows DOES NOT guarantee a stream
  // of input events when the stick is held in a given position. Consequently,
  // the navigation matrix should be updated every frame.
  // NOTE: this callback will generally be called many times/frame.
  fw.loadNavMatrix();
  dragWand.draw();
  
  ar_mutex_unlock(&databaseLock);
  return true;
}

bool inputEventQueueCallback( arSZGAppFramework& fw, arInputEventQueue& eventQueue ) {
  if (eventQueue.size() == 0) {
    return true;
  }
  ar_mutex_lock(&databaseLock);
  while (!eventQueue.empty()) {
    arInputEvent event = eventQueue.popNextEvent();
    dragWand.updateState( event );
    headEffector.updateState( event );
    fw.navUpdate( event );
    if ((event.getType() == AR_EVENT_BUTTON) && (event.getIndex() == 0)) {
      if (fw.getOnButton(0)) {
        lightingOnOff = (lightingOnOff == AR_G_TRUE)?(AR_G_FALSE):(AR_G_TRUE);
        dgStateInt( lightsOnOffID, "lighting", lightingOnOff );
      }
    }
  }
  // Currently, the joystick driver on windows DOES NOT guarantee a stream
  // of input events when the stick is held in a given position. Consequently,
  // the navigation matrix should be updated every frame.
  fw.loadNavMatrix();
  dragWand.draw();
  
  ar_mutex_unlock(&databaseLock);
  return true;
}

void matrixCallback( arCallbackInteractable* object, const arMatrix4& matrix ) {
  ar_mutex_lock(&databaseLock);
    dgTransform( object->getID(), matrix );
  ar_mutex_unlock(&databaseLock);
}

bool processCallback( arCallbackInteractable* object, arEffector* effector ) {
  if (effector == &headEffector) {
    if (object->grabbed()) {
      int iObject = object - interactionArray;
      if ((iObject >= 0)&&(iObject < NUMBER_OBJECTS)) {
        ar_mutex_lock(&databaseLock);
        dgTexture( objectTextureID[iObject], "ambrosia.ppm" );
        ar_mutex_unlock(&databaseLock);
      }
    }
  }
  return true;
}

void worldAlter(void* f){
  arDistSceneGraphFramework* framework = (arDistSceneGraphFramework*) f;
  framework->externalThreadStarted();
  int count = 0;
  while (!framework->stopping()){
    count++;
    // Change zeroth cube.  (Lissajous motion.)
    cube0Matrix = ar_translationMatrix(6.*sin(count*.00001),
                                       3.*sin(count*.0000141)+5.,
                                       6.*sin(count*.0000177));
    interactionArray[0].setMatrix( cube0Matrix );

    if (jiggleStuffGlobal) {
      // Jiggle any cube except the zeroth.
      const int iObject = 1 + rand() % (NUMBER_OBJECTS-1);
      const arMatrix4 randMatrix( interactionArray[iObject].getMatrix() *
                   ar_translationMatrix(randDistance(), randDistance(), randDistance())
                   * ar_rotationMatrix("xyz"[rand()%3], 0.02) );
      interactionArray[iObject].setMatrix( randMatrix );

      if (count%20 == 0){
        char buffer[32];
        sprintf(buffer,"WallTexture%i.ppm",rand()%4+1);
        const string whichTexture(buffer);
        ar_mutex_lock(&databaseLock);
        dgTexture(objectTextureID[iObject], whichTexture);
        ar_mutex_unlock(&databaseLock);
      }
    }

    // necessary if we're using the inputEventQueueCallback to process buffered
    // events in batches (as opposed to inputEventCallback, which handles each
    // event as it comes in).
    framework->processEventQueue();

    // Handle cube-dragging
    ar_pollingInteraction( dragWand, interactionList );
    ar_pollingInteraction( headEffector, interactionList );

    if (count%100 == 0)
      ar_usleep(1000); // CPU throttle
  }
  cout << framework->getLabel() << " remark: alteration thread stopped.\n";
  framework->externalThreadStopped();
}

void worldInit(arDistSceneGraphFramework& framework){
  const string baseName("cube");

  dgLight("light0","root",0,arVector4(0,0,1,0),arVector3(1,1,1));
  dgLight("light1","root",1,arVector4(0,0,-1,0),arVector3(1,1,1));
  dgLight("light2","root",2,arVector4(0,1,0,0),arVector3(1,1,1));
  dgLight("light3","root",3,arVector4(0,-1,0,0),arVector3(1,1,1));
  
  arCubeMesh theCube;
  const string navNodeName = framework.getNavNodeName();
  lightsOnOffID = dgStateInt( "light_switch", navNodeName, "lighting", AR_G_TRUE );
  wandID = dgTransform( "wand_transform", "light_switch", arMatrix4() );
  dgTexture( "wand_texture", "wand_transform", "ambrosia.ppm" );
  theCube.setTransform(ar_scaleMatrix(.2,.2,WAND_LENGTH));
  theCube.attachMesh( "wand", "wand_texture" );
  
  arSphereMesh theSphere(8);
  arPyramidMesh thePyramid;
  arCylinderMesh theCylinder;
  theCylinder.setAttributes(20,1,1);
  theCylinder.toggleEnds(true);
  char buffer[32];
  for (int i=0; i<NUMBER_OBJECTS; i++){
    sprintf(buffer,"%i",i);
    const string objectName(baseName + string(buffer));
    const string objectTexture(objectName + " texture");
    const string objectParent(objectName + " transform");
    if (i==0)
      strcpy(buffer, "YamahaStar.ppm");
    else
      sprintf(buffer, "WallTexture%i.ppm", rand()%4+1);
    const string whichTexture(buffer);
calcpos:
    const float randX = -5. + (10.*(rand()%200))/200.0;
    const float randY = (10.*(rand()%200))/200.0;
    const float randZ = -5. + (10.*(rand()%200))/200.0;
    if (arVector3(randX,randY-5,randZ).magnitude() < 1.5) {
      goto calcpos;
    }
    arMatrix4 cubeTransform = ar_translationMatrix(randX,randY,randZ);
    arCallbackInteractable cubeInteractor( dgTransform( objectParent, "light_switch", arMatrix4() ) );
    cubeInteractor.setMatrixCallback( matrixCallback );
    cubeInteractor.setMatrix( cubeTransform );
    cubeInteractor.setProcessCallback( processCallback );
    interactionArray[i] = cubeInteractor;
    interactionList.push_back( (arInteractable*)(interactionArray+i) );
    objectTextureID[i] = dgTexture(objectTexture,objectParent,whichTexture);

    const float radius = (i==0) ? 2.0  :  .28 + .1*(rand()%200)/200.0;
    const float whichShape = (rand()%200)/200.0;
    if (whichShape < 0.1){
      theSphere.setTransform(ar_scaleMatrix(radius));
      theSphere.attachMesh(objectName, objectTexture);
    }
    else if (whichShape < 0.2){
      theCylinder.setTransform(ar_scaleMatrix(radius));
      theCylinder.attachMesh(objectName, objectTexture);
    }
    else if (whichShape < 0.3){
      thePyramid.setTransform(ar_scaleMatrix(radius));
      thePyramid.attachMesh(objectName, objectTexture);
    }
    else{
      theCube.setTransform(ar_scaleMatrix(radius));
      theCube.attachMesh(objectName, objectTexture);
    }
  }
}

int main(int argc, char** argv){
  if (argc > 1) {
    if (!strcmp(argv[1], "-static")){
      // don't do all the random jiggling.
      jiggleStuffGlobal = false;
    }
  }
    
  arDistSceneGraphFramework framework;

  ar_mutex_init(&databaseLock);
  
  // As a general rule, this should be done before the init() if
  // framework-mediated navigation is being used. Doesn't really
  // matter as it's 1...
  framework.setUnitConversion( 1. );
  
  // Initialize everything.
  if (!framework.init(argc,argv))
    return 1;

  // This must come AFTER the init(...)
  framework.setNavTransSpeed(3.);
  
  // Configure stereo view.
  framework.setEyeSpacing( 6/(2.54*12) );
  framework.setClipPlanes( .2, 1000. );
//  framework.setEventCallback( inputEventCallback );
  framework.setEventQueueCallback( inputEventQueueCallback );
  // the worldAlter thread is an application thread that we want the
  // framework to halt
  framework.useExternalThread();

  // set max interaction distance at 5 ft.
  dragWand.setInteractionSelector( arDistanceInteractionSelector( 5 ) );
  // Set wand to do a normal drag when you click on button 2 or 6 & 7
  // (the wand triggers)
  dragWand.setDrag( arGrabCondition( AR_EVENT_BUTTON, 2, 0.5 ),
                       arWandRelativeDrag() );
  dragWand.setDrag( arGrabCondition( AR_EVENT_BUTTON, 6, 0.5 ),
                       arWandRelativeDrag() );
  dragWand.setDrag( arGrabCondition( AR_EVENT_BUTTON, 7, 0.5 ),
                       arWandRelativeDrag() );
  dragWand.setTipOffset( arVector3(0,0,-WAND_LENGTH) );
  dragWand.setDrawCallback( drawWand );
  
  headEffector.setInteractionSelector( arDistanceInteractionSelector( 2 ) );
  
  worldInit(framework);

  if (!framework.start())
    return 1;
    
  // Not working yet.
//  framework.setWorldRotGrabCondition( AR_EVENT_BUTTON, 0, 0.5 );

  arThread dummy(worldAlter, &framework);

  const arVector3 xyz(cube0Matrix * arVector3(0,0,0));
  /*int idSound = */dsLoop("foo", framework.getNavNodeName(), "cubes.mp3", 1, 0.9, xyz);

  // Main loop.
  while (true) {
    // Adjust attributes of the sound:  its loudness and position.
    static float ramp = 0.;
    ramp += .025;
    //unused const float loudness = .42 + .4 * sin(ramp);
    // cube0Matrix is being updated by worldAlter().
    const arVector3 xyz(cube0Matrix * arVector3(0,5,0));

    ar_mutex_lock(&databaseLock);
      framework.setViewer();
      framework.setPlayer();
    ar_mutex_unlock(&databaseLock);
    ar_usleep(1000000/200); // 200 fps cpu throttle
  }
}  
