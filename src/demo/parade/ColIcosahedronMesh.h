//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

#ifndef COL_ICOSAHEDRON_H
#define COL_ICOSAHEDRON_H

#include "arGraphicsAPI.h"
#include "arMath.h"
#include <string>
using namespace std;

class ColIcosahedronMesh{
 public:
  ColIcosahedronMesh();
  ColIcosahedronMesh(ARfloat color[3]);
  ~ColIcosahedronMesh();

  void attachMesh(const string&, const string&);

  void changeColor(ARfloat color[3]);
  void update();

 private:

  int pointsID, trianglesID, normalsID, colorsID;

  ARfloat colors[240];
  string name;
  int changedColor;
  ARint triangleIDs[20];

  void makeNormals(ARfloat* points, int numPoints, ARint* triangleVertices,
                  int numTriangleVertices, ARfloat* returnedNormals);
};

#endif
