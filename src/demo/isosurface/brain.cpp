//  /x/y/lib/libfmod-3.4.so apparently hardcoded into this, on dix.
//  compile like this, instead, for air:
//  g++ -o brain brain.o libarFramework.a ../graphics/libarGraphics.a ../math/arMath.o ../drivers/libarDrivers.a ../barrier/libarBarrier.a ../phleet/libarPhleet.a ../language/libarLanguage.a -lglut -lGLU -lGL -L/usr/X11R6/lib -lX11 -lXext -lXmu -lXt -lXi -lSM -lICE -lm -lpthread
// This is copied to build/linux/demo/exportit shell script.

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsDatabase.h"
#include "arGraphicsServer.h"
#include "arMath.h"
#include "arMesh.h"
#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arSZGClient.h"
#include "arThread.h"
#include "arGraphicsAPI.h"
#include "arInterfaceObject.h"

arInputNode      joystickClient;
arNetInputSource netInputSource;
char             destinationBuffer[128];
int              destinationPort;
int              serverPort;
string           geometryIP;

arMatrix4 worldMatrix;
arMatrix4 navMatrix;
int worldTransformID;
int navTransformID;
int colorID; // no jokes about telephones, please
arMutex navigationLock;
string dataPath;

void loadParameters(arSZGClient& cli){
  serverPort =
    arStringToInt(cli.getAttribute("SZG_RENDER", "geometry_port"));
  geometryIP = cli.getAttribute("SZG_RENDER", "geometry_IP");
  // see SoundRender
  cerr << "warning: brain loadParameters() under reconstruction!\n";
  dataPath = cli.getAttribute("SZG_DATA", "path");
  if (dataPath == "NULL"){
    cerr << "brain warning: SZG_DATA/path undefined.\n";
  }
}

void handleJoystick(void*){
  // good for Sidewinder Precision Pro
  while (true){
    arMatrix4 eyeTrans = ar_translationMatrix(0, 0, 0);

    // all -1 to 1
    float j[4] = {
      joystickClient.getAxis(0) / 32768.,
      joystickClient.getAxis(1) / 32768.,
      (joystickClient.getAxis(2) - 32768.) / 32768.,
      (joystickClient.getAxis(5) - 32768.) / 32768.,
      };

    // dead zones
    const float dead[4] = {.2, .2, .6, .15};
    for (int i=0; i<4; i++){
      if (j[i] < -dead[i])
        j[i] += dead[i];
      else if (j[i] > dead[i])
        j[i] -= dead[i];
      else
        j[i] = 0.;
    }

    arMatrix4 yRot = ar_rotationMatrix('y', j[0]/25.);
    arMatrix4 newTrans = ar_translationMatrix(0., j[2]/4., j[1]/2.);
    ar_mutex_lock(&navigationLock);
    navMatrix =
      newTrans *
      (!eyeTrans) *
      yRot *
      eyeTrans *
      navMatrix;
    worldMatrix =
      ar_rotationMatrix('z', j[3] / 35.) * worldMatrix;
    ar_mutex_unlock(&navigationLock);
    ar_usleep(30000);
  }
}

bool vfQuit = false;

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    cli->receiveMessage(&messageType, &messageBody);
    if (messageType=="quit"){
      vfQuit = true;
      ar_usleep(200000); // let main thread terminate
      exit(0); // ;;would a return suffice, now?
    }
  }
}

int loading = -1;

const int MAXTRI = 40301/*42000*/;

// 90K is 7.5fps in the CUBE, 4fps on case.
// Therefore, need at most 38K tri for 20fps.

// decimation is keeping every deci'th triangle, not every deci'th *point*.
const int deci = 1;
const int numberTriangles = MAXTRI/deci;
const int numberPoints = 3*numberTriangles;

const int MAXVER = 3*MAXTRI;
int G_TriCnt = 0;
float G_Vertex[3*(MAXVER+5)];
float G_Normal[3*(MAXVER+5)];

bool ReadTPoly(char *filename)
{
  const int BUFLEN = 300;
  char buffer[BUFLEN];
  FILE* fptr = arFileOpen(filename, dataPath, "rb");
  if (!fptr) {
    cerr << "brain error: failed to open data file \""
	 << filename << "\" in path "
	 << dataPath << "\".\n";
    return false;
  }
  // no errorchecking
  char* eoftest = buffer;
  float min[3] = {1e9,1e9,1e9};
  float max[3] = {-1e9,-1e9,-1e9};
  while (G_TriCnt<MAXTRI && eoftest!=NULL)
  {
    if (G_TriCnt % (MAXTRI/('z'-'a'+2)) == 0){
      char sz[100];
      sprintf(sz, "loading data/   %c of z", 
              'a' + int(G_TriCnt*('z'-'a'+1)/MAXTRI));
      dgBillboard(loading, 1, sz);
    }
    if (fgets(buffer, BUFLEN, fptr)==NULL)
      break; // EOF

    for (int i=0 ; i<3 ; i++)
    {
      if ((eoftest=fgets(buffer, BUFLEN, fptr))==NULL)
        break;
      int offset = 9*(G_TriCnt/deci) + 3*i;
      sscanf(buffer, "%f %f %f %f %f %f",
             &(G_Vertex[offset]), &(G_Vertex[offset+1]), &(G_Vertex[offset+2]),
             &(G_Normal[offset]), &(G_Normal[offset+1]), 
             &(G_Normal[offset+2]));
      for (int j=0 ; j<3 ; j++) /* Find the bounding box */
      {
        if (G_Vertex[offset+j] > max[j])
          max[j] = G_Vertex[offset+j];
        if (G_Vertex[offset+j] < min[j])
          min[j] = G_Vertex[offset+j];
      }
    }
    G_TriCnt++;
  }
  arFileClose(fptr);

  // recenter dataset on origin
  const float shift[3] = { (min[0]-max[0])/2, (min[1]-max[1])/2, 
                           (min[2]-max[2])/2 };
  for (int i=0; i<G_TriCnt/deci * 3; i++){
    G_Vertex[i*3  ] += shift[0];
    G_Vertex[i*3+1] += shift[1];
    G_Vertex[i*3+2] += shift[2];
  }

  dgBillboard(loading, 1, "loading data/   complete");
  return true;
}

int triangleIDs[numberTriangles];
float colors[12*numberTriangles];
float activation[numberTriangles] = {0}; // 0 to 1, for each triangle
float activationSent[numberTriangles] = {0}; // value which actually got sent

// Quicksort scaffolding (ascending order).
struct sorter { int id; float diff; } rgsort[numberTriangles];
int sorterCompare(const void* pv1, const void* pv2){
  const float d1 = ((const struct sorter*)pv1)->diff;
  const float d2 = ((const struct sorter*)pv2)->diff;
  return d1<d2 ? 1 : d1>d2 ? -1 : 0;
}

const int cSlice = 181;
int idVis[cSlice] = {0};

#ifdef AR_USE_WIN_32
inline float drand48()
{
  return double(rand()) / double(RAND_MAX);
}
#endif

#ifndef M_PI
#define M_PI (3.14159265358979)
#endif

void Activate(arVector3* centroids){
    arVector3 c;
    do {
      const float x = drand48() * 26 - 13; // -13 to 13, port to starboard
      const float y = drand48() * 42 - 12; // -12 to 30, back to front
      const float z = drand48() * 30 - 15; // -15 to 15 approx, bottom to top
      c = arVector3(x, y, z);
    } while (++c < 5); // avoid center where there's not much to see

    const float r = .6 + drand48() * drand48() * 2;
    for (int i=0; i<numberTriangles; i++){
      const float distance = ++(centroids[i] - c);
      if (distance < r){
        activation[i] += 1.;
      }
      else if (distance < 2*r){
	// as distance goes from r to 2*r, 
        // activation_increase drops from 1 to 0
        activation[i] += 1. - (distance-r)/r;
      }
      if (activation[i] > 1.)
        activation[i] = 1.;
    }
}

void alterScene()
{
  // Hide this slice and show the next one.  Swing back and forth.
  static int iSlice = 0;
  static int diSlice = 1;
  dgVisibility(idVis[iSlice], 0);
  iSlice += diSlice;
  if (iSlice < 0){
    iSlice = 0;
    diSlice = 1;
  }
  if (iSlice >= cSlice){
    iSlice = cSlice-1;
    diSlice = -1;
  }
  dgVisibility(idVis[iSlice], 1);

  // G_Vertex[3*iPoint + {0,1,2}] is xyz coords of iPoint'th point
  // i'th triangle has vertices 3*i+{0,1,2}.
  // So i'th triangle's position is the centroid (mean) of those 3 vertices.

  int i;

  // (centroids[i][0] > 0)  // starboard half
  // (centroids[i][1] > 0)  // forward half
  // (centroids[i][2] > 0)  // top third

  static int fInit = false;
  static arVector3 centroids[numberTriangles];
  if (!fInit){
    fInit = true;
    // Compute centroids of all triangles.
    for (i=0; i<numberTriangles; i++){
      const arVector3* a = (const arVector3*)&G_Vertex[3*(3*i+0)];
      const arVector3* b = (const arVector3*)&G_Vertex[3*(3*i+1)];
      const arVector3* c = (const arVector3*)&G_Vertex[3*(3*i+2)];
      centroids[i] = *a/3 + *b/3 + *c/3;
      activation[i] = 0.;
      activationSent[i] = 0.;
    }
  }

  // Fade activation values.
  for (i=0; i<numberTriangles; i++){
    activation[i] *= 0.95;
  }

  // Set activation values: an occasional sphere.
  static float rate = .4, drate = .01;
  rate += drate;
  if (rate > 1.)
    drate = -.005;
  if (rate < .4)
    drate = .005;
  // at 15fps, 120 steps up and 120 down is 16 seconds.
  if (drand48() < rate){
    Activate(centroids);
    Activate(centroids);
    Activate(centroids);
  }

  // If we send the colors for *all* the triangles, 
  // the frame rate drops to 1.5fps.
  // So send only the "top" few values, 
  // those which will make the most difference.
  // Difference is: largest diff between actual value 
  // and last sent value, i.e.,
  // fabs(activation[i] - activationSent[i]).
  // Sort these values, pick the top ones, update activationSent[], send.

  for (i=0; i<numberTriangles; i++){
    rgsort[i].id = i;
    rgsort[i].diff = fabs(activation[i] - activationSent[i]);
  }
  qsort(rgsort, numberTriangles, sizeof(struct sorter), sorterCompare);
  // from 10000 down to 5 seems to make no diff to the fps hit!
  const int numberTrianglesSent = 2800;
  static int colorsSent[3*numberTrianglesSent];

  for (i=0; i<numberTrianglesSent; i++){
    const int id = rgsort[i].id;
    colorsSent[3*i] = id;
    colorsSent[3*i+1] = id+1;
    colorsSent[3*i+2] = id+2;
    activationSent[id] = activation[id];
    const float blue = activation[id];
    const float yellow = 1. - blue;
    // each vertex has the same color!
    colors[12*i  ] = blue*.05 + yellow*1.0;
    colors[12*i+1] = blue*.05 + yellow*.70;
    colors[12*i+2] = blue*1.0 + yellow*.1;
    colors[12*i+3] = 1;
    colors[12*i+4] = blue*.05 + yellow*1.0;
    colors[12*i+5] = blue*.05 + yellow*.70;
    colors[12*i+6] = blue*1.0 + yellow*.1;
    colors[12*i+7] = 1;
    colors[12*i+8] = blue*.05 + yellow*1.0;
    colors[12*i+9] = blue*.05 + yellow*.70;
    colors[12*i+10] = blue*1.0 + yellow*.1;
    colors[12*i+11] = 1;
  }

  dgColor4(colorID, 3*numberTrianglesSent, colorsSent, colors);
}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;
  loadParameters(szgClient);
  ar_mutex_init(&navigationLock);
  arThread dummy(messageTask, &szgClient);

  string interfaceType;
  if (szgClient.getAttribute("SZG_INPUT", "mode", "|tracker|joystick|")
      == "tracker"){
    interfaceType = "tracker";
    //********************************************************************
    // the arInterfaceObject no longer starts threads itself. instead,
    // it will act as a recepticle for input device events.
    // consequently, this call is no longer supported
    //(void)interfaceObject.start(destinationBuffer, destinationPort);
    //********************************************************************
  }
  else{
    interfaceType = "joystick";
    netInputSource.setService("SZG_INPUT");
    joystickClient.addInputSource(&netInputSource,false);
    if (!joystickClient.init(szgClient) || !joystickClient.start())
      return 1;
    arThread joystickThread(handleJoystick);
  }

  // set up the graphics
  arGraphicsServer theDatabase;
  dgSetGraphicsDatabase(&theDatabase);
  // global transform, as controlled by the joystick
  navTransformID = dgTransform("nav", "root", navMatrix);
  worldTransformID = dgTransform("world", "nav", worldMatrix);

  // splash screen
  dgTransform("billboard transform", "world",
    ar_translationMatrix(0,0,5) /* *ar_rotationMatrix('y',3.14159)*/ );
  loading = dgBillboard("loading", "billboard transform", 1,
    "Loading data/please wait");

  {
  // array of optionally visible quads.
  arRectangleMesh r;
  char ppmfile[50], vis[50], slice[50], tex[50];

  cerr << "brain remark: loading texture data, please wait.\n\n";
  for (int i=0; i<cSlice; i++) {
    sprintf(ppmfile, "BrainSlice%03d.ppm", i);
    sprintf(vis, "vis%d", i);
    sprintf(slice, "slice%d", i);
    sprintf(tex, "tex%d", i);
    idVis[i] = dgVisibility(vis, "world", 0);
    dgTransform(slice, vis,
      ar_translationMatrix(-23 + i/float(cSlice-1)*45 /*to starboard*/,
                           0 /*to nose*/,
			   2.85 /*to crown*/) *
      ar_rotationMatrix('z', M_PI/2) *
      ar_scaleMatrix(60/*fwd/bkwd*/, 1, 48/*crown/neck*/));
    dgTexture(tex, slice, ppmfile, 0/*black = transparent*/);
    r.attachMesh(slice, tex);
    }
  }

  // Display splash screen while loading data.
  // This splash screen is actually a progress meter.
  if (!theDatabase.setInterface(geometryIP) ||
      !theDatabase.setPort(serverPort)) {
    cerr << "brain error: invalid IP:port "
         << geometryIP << ":" << serverPort
	 << " for database.\n";
    return 1;
  }
  if (!theDatabase.beginListening())
    return 1;

  // actual data

  if (!ReadTPoly("head-8.bin"))
    return 1;

  int i;
  float* pointPositions = new float[3*numberPoints];
  for (i=0; i<numberPoints; i++){
    pointPositions[3*i  ] = G_Vertex[3*i  ];
    pointPositions[3*i+1] = G_Vertex[3*i+1];
    pointPositions[3*i+2] = G_Vertex[3*i+2];
  }
  dgPoints("foo a", "world", numberPoints, pointPositions);
  delete [] pointPositions;
  float* normals = new float[9*numberTriangles];
  for (i=0; i<numberTriangles; i++){
    //;; int offset = 9*i + 3* 0,1,2;
    normals[9*i  ] = G_Normal[9*i  ];
    normals[9*i+1] = G_Normal[9*i+1];
    normals[9*i+2] = G_Normal[9*i+2];
    normals[9*i+3] = G_Normal[9*i+3];
    normals[9*i+4] = G_Normal[9*i+4];
    normals[9*i+5] = G_Normal[9*i+5];
    normals[9*i+6] = G_Normal[9*i+6];
    normals[9*i+7] = G_Normal[9*i+7];
    normals[9*i+8] = G_Normal[9*i+8];
  }
  dgNormal3("foo b", "foo a", 3*numberTriangles, normals);
  delete [] normals;
  for (i=0; i<numberTriangles; i++){
    // yellow, one RGBA vector for each vertex
    colors[12*i  ] = 1.;
    colors[12*i+1] = .7;
    colors[12*i+2] = .1;
    colors[12*i+3] = 1.;
    colors[12*i+4] = 1.;
    colors[12*i+5] = .7;
    colors[12*i+6] = .1;
    colors[12*i+7] = 1.;
    colors[12*i+8] = 1.;
    colors[12*i+9] = .7;
    colors[12*i+10] = .1;
    colors[12*i+11] = 1.;
  }
  colorID = dgColor4("foo c", "foo b", 3*numberTriangles, colors);
  dgDrawable("foo d", "foo b", DG_TRIANGLES, numberTriangles);

  dgErase("loading");

  arInterfaceObject interfaceObject;
  interfaceObject.setNavMatrix(ar_rotationMatrix('x', M_PI/2));
  navMatrix = interfaceObject.getNavMatrix();

  // Do this for joystick as well as tracker.
  ar_mutex_lock(&navigationLock);
    dgTransform(worldTransformID, navMatrix);
  ar_mutex_unlock(&navigationLock);

  while (!vfQuit){
    if (interfaceType == "tracker"){
      navMatrix = interfaceObject.getNavMatrix();
      worldMatrix = interfaceObject.getObjectMatrix();
    }
    ar_mutex_lock(&navigationLock);
      dgTransform(worldTransformID, worldMatrix);
      dgTransform(navTransformID, navMatrix);
    ar_mutex_unlock(&navigationLock);
    alterScene();
    // cpu throttle, cube's 4110's can only do 15fps anyways.
    ar_usleep(1000000/20);
  }
  return 0;
}
