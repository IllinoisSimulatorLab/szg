//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "H3World.h"
#define N_DODECAHEDRA  927

H3World::H3World() :
  name(string("")),
  levels(1),
  pointsList(new float*[1]),
  changedPoints(false),
  changedTransform(false),
  pointNodeID(-1),
  transformID(-1)
{
  pointsList[0] = new float[N_VERTS*N_DODECAHEDRA*3];
#ifdef UNUSED
  colors[0] = 0.5; colors[1] = 0.9; colors[2] = 0.1;
#endif
}

H3World::~H3World()
{
  int ii;
  for (ii=0;ii<levels;ii++)
    delete pointsList[ii];
  delete pointsList;

  dgErase(name+" transformation");
}

#ifdef AR_USE_WIN_32
inline float drand48()
{
  return double(rand()) / double(RAND_MAX);
}
#endif

#define R { .6, 1., .2},
#define G { .4, 1., .4},
#define B { .2, 1., .6},

const float threecolors[N_EDGES][3] = {
  R B G B G B G R B G B G B G B G R G B G B G R R R B R R R R
};

void H3World::attachMesh(const string& hypTessName, const string& parentName)
{
  int lineSet[N_DODECAHEDRA*N_EDGES*2];
  name=hypTessName;
  int tempLineSet[N_EDGES*2];
  float tempPointsList[N_VERTS*3];

  //int* lineSet = new int[N_DODECAHEDRA*N_EDGES*2];
  //name=hypTessName;
  //int* tempLineSet = new int[N_EDGES*2];
  //float* tempPointsList = new float[N_VERTS*3];

  transformID=dgTransform(name+" transformation", parentName, tessTransform);

  dodecahedra = new HypDodecahedron*[N_DODECAHEDRA];
  int ii,jj;
  for(ii=0;ii<N_DODECAHEDRA;ii++){
    dodecahedra[ii] = new HypDodecahedron(colors, ii);
    dodecahedra[ii]->makeDodecahedron(tempPointsList, tempLineSet);

    for(jj=0;jj<N_EDGES*2;jj++){
      lineSet[ii*N_EDGES*2+jj] = ii*N_VERTS+tempLineSet[jj];
    }

    for(jj=0;jj<N_VERTS*3;jj++){
      pointsList[0][ii*N_VERTS*3+jj] = tempPointsList[jj];
    }
  }

  // each edge has a color associated with each end... a total of
  // 8 floats
  float color[N_DODECAHEDRA*N_EDGES*8];
  for(ii=0; ii<N_DODECAHEDRA*N_EDGES; ii++){
    color[8*ii+0] = threecolors[ii % N_EDGES][0];
    color[8*ii+1] = threecolors[ii % N_EDGES][1];
    color[8*ii+2] = threecolors[ii % N_EDGES][2];
    color[8*ii+3] = 1;
    color[8*ii+4] = threecolors[ii % N_EDGES][0];
    color[8*ii+5] = threecolors[ii % N_EDGES][1];
    color[8*ii+6] = threecolors[ii % N_EDGES][2];
    color[8*ii+7] = 1;
  }

  pointNodeID = dgPoints(name+" points", name+" transformation", 
                         N_DODECAHEDRA*N_VERTS, pointsList[0]);
  dgColor4(name+" colors", name+" points", N_DODECAHEDRA*N_EDGES*2,color);
  dgIndex(name+" index", name+" colors", N_DODECAHEDRA*N_EDGES*2,
          lineSet);
  dgDrawable(name+" lineset", name+" index", DG_LINES, N_DODECAHEDRA*N_EDGES);
}

void H3World::rotate(char axis, float radians)
{
  changedPoints=true;
  for(int ii=0; ii<N_DODECAHEDRA; ii++)
  {
    dodecahedra[ii]->hypRotate(axis,radians);
  }
}

void H3World::translate(float howFar)
{
  changedPoints=true;
  for(int ii=0; ii<N_DODECAHEDRA; ii++)
  {
    dodecahedra[ii]->hypTranslate(howFar);
  }
}

void H3World::scale(float xscale, float yscale, float zscale)
{
  changedTransform=true;
  tessTransform = ar_scaleMatrix(xscale,yscale,zscale) * tessTransform;
}

//precondition - attachMesh already called
void H3World::update()
{
  if (changedPoints)
  {
    changedPoints=false;
    float tempPointsList[3*N_VERTS];
    for(int ii=0;ii<N_DODECAHEDRA;ii++)
    {
      dodecahedra[ii]->updateUnattached(tempPointsList);
      for(int jj=0;jj<N_VERTS*3;jj++)
      {
        pointsList[0][ii*N_VERTS*3+jj]=tempPointsList[jj];
      }
    }
  }

  dgPoints(pointNodeID, N_DODECAHEDRA*N_VERTS, pointsList[0]);

  if (changedTransform)
  {
    changedTransform=false;
    dgTransform(transformID, tessTransform);
  }
}

void H3World::update(double* theMatrix){
  float tempPointsList[3*N_VERTS];
  for(int ii=0;ii<N_DODECAHEDRA;ii++){
    dodecahedra[ii]->updateUnattached(tempPointsList,theMatrix);
      
    for(int jj=0;jj<N_VERTS*3;jj++){
      pointsList[0][ii*N_VERTS*3+jj]=tempPointsList[jj];
    }
  }

  dgPoints(pointNodeID, N_DODECAHEDRA*N_VERTS, pointsList[0]);
}
