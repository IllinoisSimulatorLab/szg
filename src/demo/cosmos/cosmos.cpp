//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMesh.h"
#include "arInterfaceObject.h"
#include "arDistSceneGraphFramework.h"
// put this someplace better.  like arGraphicsAPI.h is included in arMesh.h.
#include "arSoundAPI.h" 

// stuff that's really specific to this app

ARfloat  vertexPosition1[900];
ARfloat  colors[1200];
arStructuredData* linesData = NULL;
arStructuredData* linePointsData = NULL;
int linePointsID = -1;
int visibilityID = -1;
bool bumpMap = false;

arMatrix4 worldTransform, navTransform;
arMatrix4 local1Matrix, local2Matrix, local3Matrix, local4Matrix;
int worldTransformID[2], navTransformID[2];
int local1TransformID, local2TransformID, local3TransformID, local4TransformID;
int billboardTransformID;
arMatrix4 billboardTransform;
int lightTransformID[3];
arMatrix4 lightTransform[3];

void attachLineSet(){
  // seed some random locations for the vertices
  int i;
  for (i=0; i<150; i++){
    arVector3 randPos = arVector3(-5 + 10*(1.0*(rand()%200))/200.0,
				  -5 + 10*(1.0*(rand()%200))/200.0,
				  -5 + 10*(1.0*(rand()%200))/200.0);
    if (++randPos == 0){
      randPos = arVector3(0,0,1);
    }
    randPos.normalize();
    randPos *= 5;
    vertexPosition1[6*i] = randPos[0];
    vertexPosition1[6*i+1] = randPos[1];
    vertexPosition1[6*i+2] = randPos[2];
    vertexPosition1[6*i+3] = 0;
    vertexPosition1[6*i+4] = 0;
    vertexPosition1[6*i+5] = 0;
  }
  // fill the other needed arrays
  for (i=0; i<150; i++){
    colors[8*i] = colors[8*i+4] = rand()%1000/1000.;
    colors[8*i+1] = colors[8*i+5] = rand()%1000/1000.;
    colors[8*i+2] = colors[8*i+6] = rand()%1000/1000.;
    colors[8*i+3] = colors[8*i+7] = 1.;
  }
  linePointsID = dgPoints("line points","world",300,vertexPosition1);
  dgColor4("line colors","line points",300,colors);
  dgDrawable("lines","line colors", DG_LINES, 150);
}

void worldInit(arDistSceneGraphFramework* framework){
  // global transform, as controlled by the input device
  const string navNodeName = framework->getNavNodeName();

//  navTransformID[0] = dgTransform("nav","root",navTransform);
//  navTransformID[1] = dsTransform("nav","root",navTransform);

  // a light
  dgLight("light0","root",0,arVector4(0,0,1,0),arVector3(1,1,1));
  dgLight("light1","root",1,arVector4(0,0,-1,0),arVector3(1,1,1));
  dgLight("light2","root",2,arVector4(0,-1,0,0),arVector3(1,1,1));
  dgLight("light3","root",3,arVector4(0,1,0,0),arVector3(1,1,1));

  //************************************************************************
  // NOTE: the moving spotlights have been disabled for now...
  // there seems to be a weirdness about light position, etc.
  //************************************************************************
  lightTransformID[0] = dgTransform("light trans 0",navNodeName,lightTransform[0]);
  //dgLight("light0","light trans 0",0,arVector4(0,0,20,1),arVector3(1,1,1),
  //        arVector3(0,0,0),arVector3(1,1,1),arVector3(1,0,0),
  //        arVector3(0,0,-1),5,2);
  lightTransformID[1] = dgTransform("light trans 1",navNodeName,lightTransform[1]);
  //dgLight("light1","light trans 1",1,arVector4(0,20,0,1),arVector3(1,1,1),
  //        arVector3(0,0,0),arVector3(1,1,1),arVector3(1,0,0),
  //        arVector3(0,-1,0),5,2);
  lightTransformID[2] = dgTransform("light trans 2",navNodeName,lightTransform[2]);
  //dgLight("light2","light trans 2",2,arVector4(20,0,0,1),arVector3(1,1,1),
  //        arVector3(0,0,0),arVector3(1,1,1),arVector3(1,0,0),
  //        arVector3(-1,0,0),5,2);

  // object transform
  worldTransformID[0] = dgTransform("world",navNodeName,worldTransform);
  worldTransformID[1] = dsTransform("world",navNodeName,worldTransform);

  string myParent;

  // attach torus 1
  local1TransformID = dgTransform("local1","world",local1Matrix);
  dgTexture("texture1", "local1", "WallTexture1.ppm");
  dgMaterial("material1", "texture1", arVector3(1,0.6,0.6));
  arTorusMesh theMesh(60,30,4,0.5);
  if (bumpMap){
    theMesh.setBumpMapName("normal.ppm");
  }
  theMesh.attachMesh("torus1","material1");

  // attach torus 2
  local2TransformID = dgTransform("local2","world",local2Matrix);
  dgTexture("texture2", "local2", "WallTexture2.ppm");
  theMesh.reset(60,30,2,0.5);
  theMesh.attachMesh("torus2","texture2");

  // attach torus 3
  local3TransformID = dgTransform("local3","world",local3Matrix);
  dgTexture("texture3", "local3", "WallTexture3.ppm");
  theMesh.reset(50,30,1,0.5);
  theMesh.attachMesh("torus3","texture3");

  // attach torus 4
  local4TransformID = dgTransform("local4","world",local4Matrix);
  dgTexture("texture4", "local4", "WallTexture4.ppm");
  theMesh.reset(40,30,3,0.5);
  theMesh.attachMesh("torus4","texture4");

  // attach the line set
  attachLineSet();

  // attach the billboard
  visibilityID = dgVisibility("visibility","local1",1);
  billboardTransform = ar_translationMatrix(4.55,0,0)
    *ar_rotationMatrix('x', -1.571)
    *ar_rotationMatrix('y', -1.571)
    *ar_scaleMatrix(0.1,0.1,0.1);
  billboardTransformID = dgTransform("billboard transform","visibility",
              billboardTransform);
  dgBillboard("billboard","billboard transform",1,
	      " syzygy scene graph ");
}

void worldAlter(){
  static bool init = false;
  static ar_timeval oldTime;
  float factor = 1.;
  if (!init)
    init = true;
  else
    factor = ar_difftime(ar_time(),oldTime)/30000.0;
  oldTime = ar_time();

  // Precomputing these rotation matrices would be faster, of course.
  lightTransform[0] = ar_rotationMatrix('y',factor*0.005) * lightTransform[0];
  lightTransform[1] = ar_rotationMatrix('x',factor*0.009) * lightTransform[1];
  lightTransform[2] = ar_rotationMatrix('z',factor*0.011) * lightTransform[2];
  dgTransform(lightTransformID[0], lightTransform[0]);
  dgTransform(lightTransformID[1], lightTransform[1]);
  dgTransform(lightTransformID[2], lightTransform[2]);

  billboardTransform = ar_rotationMatrix('z',factor*0.01)*billboardTransform;
  dgTransform(billboardTransformID, billboardTransform);

  local1Matrix = ar_rotationMatrix('x', factor *  0.016) * local1Matrix;
  local2Matrix = ar_rotationMatrix('y', factor *  0.031) * local2Matrix;
  local3Matrix = ar_rotationMatrix('z', factor *  0.021) * local3Matrix;
  local4Matrix = ar_rotationMatrix('z', factor * -0.055) * local4Matrix;
  dgTransform(local1TransformID,local1Matrix * !local3Matrix);
  dgTransform(local2TransformID,local2Matrix * !local4Matrix * local3Matrix);
  dgTransform(local3TransformID,local3Matrix * !local2Matrix * !local1Matrix);
  dgTransform(local4TransformID,local4Matrix * !local1Matrix);
  // Gratuitous matrix-multiplication produces a wilder donut-dance.

  dgTransform(worldTransformID[0],worldTransform);

  static int currentVis = 1;
  dgVisibility(visibilityID,currentVis);
  static int howMany = 0;
  if (++howMany>30){
    currentVis = 1-currentVis;
    howMany = 0;
  }

  // Change the line length.
  static float length = 1.;
  length += .25;
  if (length >= 20.0){
    length = 1.0;
  }
  for (int i=0; i<150; i++){
    const arVector3 line = arVector3(&vertexPosition1[6*i]) * length * .05;
    vertexPosition1[6*i+3] = line.v[0];
    vertexPosition1[6*i+4] = line.v[1];
    vertexPosition1[6*i+5] = line.v[2];
  }
  dgPoints(linePointsID,300,vertexPosition1);
}

int main(int argc, char** argv){
  arDistSceneGraphFramework* framework = new arDistSceneGraphFramework;
  // We could dispense with the pointer, but it *might* cause that
  // intermittent constructor-hang.  To be tested more.

  if (argc > 1 && !strcmp(argv[1],"-b")) {
    bumpMap = true;
  }

  // Initialize everything.
  if (!framework->init(argc,argv))
    return 1;

  arInterfaceObject interfaceObject;
  interfaceObject.setInputDevice(framework->getInputDevice());
  // The following is VERY important... otherwise the navigation is
  // WAY too fast.
  interfaceObject.setSpeedMultiplier(0.15);
  framework->setAutoBufferSwap(false);
  worldInit(framework);

  // Configure stereo view.
  framework->setEyeSpacing( 6/(2.54*12) );
  framework->setClipPlanes( .3, 1000. );
  framework->setUnitConversion( 1. );

  // More initializing.
  if (!framework->start() || !interfaceObject.start())
    return 1;

  // Place scene in the center of the front wall.
  interfaceObject.setNavMatrix(ar_translationMatrix(0,5,-5));

  const arVector3 xyz(0,0,0);
  const int idLoop = dsLoop("ambience", "world", "cosmos.mp3", 1, 1, xyz);
  const int idBeep = dsLoop("beep", "world", "q33beep.wav",  0, 0.0, xyz);

  // Main loop.
  while (true) {
    navTransform = interfaceObject.getNavMatrix().inverse();
    worldTransform = interfaceObject.getObjectMatrix();
    ar_setNavMatrix( navTransform );

    // No lock needed, since there's only one thread.
    dgTransform(worldTransformID[0], worldTransform);
    dsTransform(worldTransformID[1], worldTransform);
    framework->loadNavMatrix();
//    dgTransform(navTransformID[0], navTransform);
//    dsTransform(navTransformID[1], navTransform);

    worldAlter();

    // DO NOT TURN THE AMBIENT SOUND ON/OFF, THIS IS A NICE TEST OF
    // "IS SPATIALIZED SOUNDS WORKING" AND TURNING ON/OFF MESSES THAT
    // UP.
    (void)dsLoop(idLoop, "cosmos.mp3", 1, 0, xyz);

    // Play a beep sporadically.
    if (rand() % 200 == 0) {
      (void)dsLoop(idBeep, "q33beep.wav", -1, 1, xyz);
      (void)dsLoop(idBeep, "q33beep.wav",  0, 0, xyz);
    }

    // Change viewpoint.
    framework->setViewer();
    framework->setPlayer();

    // and now we go ahead and force a buffer swap (which we are controlling
    // manually)
    framework->swapBuffers();
  }
  return 0;
}
