//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arMesh.h"
#include "arInterfaceObject.h"
#include "arDistSceneGraphFramework.h"
#include "arSoundAPI.h" 

const int numLines = 150;
const int numPoints = numLines * 2;
ARfloat  vertexPosition1[numPoints * 3]; // each point has 3 coords, xyz
ARfloat  colorRGBA[numPoints * 4]; // each point has 4 values, rgba
arStructuredData* linesData = NULL;
arStructuredData* linePointsData = NULL;
int linePointsID = -1;
int visibilityID = -1;

arMatrix4 worldTransform, navTransform;
arMatrix4 local1Matrix, local2Matrix, local3Matrix, local4Matrix;
int worldTransformID[2], navTransformID[2];
int local1TransformID, local2TransformID, local3TransformID, local4TransformID;
int billboardTransformID;
arMatrix4 billboardTransform;
int lightTransformID[3];
arMatrix4 lightTransform[3];

bool inputEventQueueCallback( arSZGAppFramework& fw, arInputEventQueue& q ) {
  bool fReload = false;
  while (!q.empty()) {
    const arInputEvent event = q.popNextEvent();
    const arInputEventType t = event.getType();
    // navUpdate with axis events for forward/backward/speed,
    // and with matrix events to know which way the wand points.
    if (t == AR_EVENT_MATRIX || t == AR_EVENT_AXIS) {
      fw.navUpdate( event );
      fReload = true;
    }
  }
  if (fReload)
    fw.loadNavMatrix();
  return true;
}


void attachLineSet(){
  int i;
  // random vertices
  for (i=0; i<numLines; i++){
    arVector3 randPos(-5 + 10*(1.0*(rand()%200))/200.0,
		      -5 + 10*(1.0*(rand()%200))/200.0,
		      -5 + 10*(1.0*(rand()%200))/200.0);
    if (++randPos == 0){
      randPos = arVector3(0,0,1);
    }
    randPos.normalize();
    randPos *= 5;
    vertexPosition1[6*i  ] = randPos[0];
    vertexPosition1[6*i+1] = randPos[1];
    vertexPosition1[6*i+2] = randPos[2];
    vertexPosition1[6*i+3] =
    vertexPosition1[6*i+4] =
    vertexPosition1[6*i+5] = 0;
  }

  // fill the other arrays
  for (i=0; i<numLines; i++){
    colorRGBA[8*i  ] = colorRGBA[8*i+4] = rand()%1000/1000.;
    colorRGBA[8*i+1] = colorRGBA[8*i+5] = rand()%1000/1000.;
    colorRGBA[8*i+2] = colorRGBA[8*i+6] = rand()%1000/1000.;
    colorRGBA[8*i+3] = colorRGBA[8*i+7] = 1.;
  }
  linePointsID = dgPoints("line points","world",2*numLines,vertexPosition1);
  dgColor4("line colors","line points",numPoints,colorRGBA);
  dgDrawable("lines","line colors", DG_LINES, numLines);
}

void worldInit(arDistSceneGraphFramework& fw){
  // global transform, as controlled by the input device
  const string navNodeName = fw.getNavNodeName();

  dgLight("light0", "root", 0, arVector4(0,0,1,0),  arVector3(1,1,1));
  dgLight("light1", "root", 1, arVector4(0,0,-1,0), arVector3(1,1,1));
  dgLight("light2", "root", 2, arVector4(0,-1,0,0), arVector3(1,1,1));
  dgLight("light3", "root", 3, arVector4(0,1,0,0),  arVector3(1,1,1));

  lightTransformID[0] = dgTransform("light trans 0",navNodeName,lightTransform[0]);
  lightTransformID[1] = dgTransform("light trans 1",navNodeName,lightTransform[1]);
  lightTransformID[2] = dgTransform("light trans 2",navNodeName,lightTransform[2]);

  // object transform
  worldTransformID[0] = dgTransform("world",navNodeName,worldTransform);
  worldTransformID[1] = dsTransform("world",navNodeName,worldTransform);
  string myParent;

  // attach torus 1
  local1TransformID = dgTransform("local1","world",local1Matrix);
  dgTexture("texture1", "local1", "WallTexture1.ppm");
  dgMaterial("material1", "texture1", arVector3(1,0.6,0.6));
  arTorusMesh theMesh(60,30,4,0.5);
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

void worldAlter(arDistSceneGraphFramework& fw) {
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
  for (int i=0; i<numLines; i++){
    const arVector3 line = arVector3(&vertexPosition1[6*i]) * length * .05;
    vertexPosition1[6*i+3] = line.v[0];
    vertexPosition1[6*i+4] = line.v[1];
    vertexPosition1[6*i+5] = line.v[2];
  }
  dgPoints(linePointsID,numPoints,vertexPosition1);

  fw.processEventQueue(); // for inputEventQueueCallback
}

int main(int argc, char** argv){
  arDistSceneGraphFramework fw;
  fw.setAutoBufferSwap(false); // Before init().
  if (!fw.init(argc,argv)){
    return 1;
  }

  // Navigation.
  fw.setNavTransSpeed(3.0); // After init().
  fw.setEventQueueCallback( inputEventQueueCallback );

  // Move from center of floor (traditional cave coords) to center of Cube.
  ar_navTranslate(arVector3(0, -5, 0));
  
  // Files containing textures and sounds.
  fw.setDataBundlePath("SZG_DATA", "cosmos");

  worldInit(fw);

  fw.setEyeSpacing( 6/(2.54*12) );
  fw.setClipPlanes( .3, 1000. );
  fw.setUnitConversion( 1. );

  if (!fw.start()) {
    return 1;
  }

  const arVector3 xyz(0,0,0);
  const int idLoop = dsLoop("ambience", "world", "cosmos.mp3", 1, 1, xyz);
  const int idBeep = dsLoop("beep", "world", "q33beep.wav",  0, 0.0, xyz);

  // Main loop.
  while (true) {
    // No lock needed, since there's only one thread.
    dgTransform(worldTransformID[0], worldTransform);
    dsTransform(worldTransformID[1], worldTransform);
    fw.loadNavMatrix();

    worldAlter(fw);

    // Verify spatialized sound.  (Don't turn it on/off.)
    (void)dsLoop(idLoop, "cosmos.mp3", 1, 1, xyz);

    // Sporadically play a beep.
    if (rand() % 200 == 0) {
      (void)dsLoop(idBeep, "q33beep.wav", -1, 1, xyz);
      (void)dsLoop(idBeep, "q33beep.wav",  0, 0, xyz);
    }

    // Change viewpoint.
    fw.setViewer();
    fw.setPlayer();

    // Force a buffer swap.
    fw.swapBuffers();
  }
  return 0;
}
