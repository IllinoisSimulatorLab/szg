//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

#ifndef HYP_DODECAHEDRON_H
#define HYP_DODECAHEDRON_H

#include "arGraphicsAPI.h"
#include "arMath.h"
#include <string>
#include "dod.descrip.h"
#include "927hyp.h"
using namespace std;

//hyperbolic tiling with 927 dodecahedra
class HypDodecahedron{
 public:
  HypDodecahedron();
  HypDodecahedron(ARfloat color[3], int whichDod, double scale = HKAPPA*0.98);
  ~HypDodecahedron();

  void attachMesh(const string& hypDodName, const string& parentName);

  //destPoints, destLines are arrays where pointset and lineset are to be put
  //destPoints must be of size N_VERTS*3 and destLines of size N_EDGES*2
  void makeDodecahedron(float *destPoints, int *destLines);

  //every function between here and update uses update to enact changes
  void changeColor(ARfloat color[3]);
  void hypTranslate(double speed); //only along z
  void hypRotate(char axis, double radians);
  void updateAttached();
  void updateUnattached(float destPoints[N_VERTS*3]); 
  void updateUnattached(float destPoints[N_VERTS*3],double* theMatrix); 

 private:
  string name;
  int changedColor;
  int changedPoints;
  float color[3];
  int whichDod;
  double dodScale;
  
  int pointNodeID, lineNodeID;
  ARint lineEndPoints[N_EDGES*2];

  double dodPoints[N_VERTS][4];

  double transformation[16];

  void multiplyVectorByMatrix
                (double vector[4], double matrix[16], double dest[4]);
  void multiplyMatrixByVector
                (double matrix[16], double vector[4], double dest[4]);
  void multiplyMatrixByMatrix
                (double left[16], double right[16], double product[16]);
  void project4DTo3D(float points3D[N_VERTS*3]);
  void project4DTo3D(float points3D[N_VERTS*3],double*);
  void updatePoints();

};

#endif
