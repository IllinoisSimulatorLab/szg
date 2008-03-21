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

class ColIcosahedronMesh {
 public:
  ColIcosahedronMesh();
  ColIcosahedronMesh(const ARfloat* color);
  ~ColIcosahedronMesh();
  void attachMesh(const string&, const string&);
  void changeColor(const ARfloat* color);
  void update();

 private:
  bool changedColor;
  int pointsID, trianglesID, normalsID, colorsID;
  string name;
  ARint triangleIDs[20];
  ARfloat colors[240];

  void _changeColor(const ARfloat* color);
  void makeNormals(ARfloat* points, int numPoints, ARint* triangleVertices,
                  int numTriangleVertices, ARfloat* returnedNormals);
};

#endif
