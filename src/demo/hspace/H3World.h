//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

#ifndef H3_WORLD_H
#define H3_WORLD_H
// Written under the GNU General Public License
// -- BJS
// MJW BCB - 06July01

#include "arGraphicsAPI.h"
#include "arMath.h"
#include <string>
#include "HypDodecahedron.h"
using namespace std;

//hyperbolic tiling with 927 dodecahedra
class H3World{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  H3World();
  
  //colors is a packed array of colors, 3 floats per level
  H3World(int numLevels, float *levelColors);

  ~H3World();

  void attachMesh(const string& hypTessName, const string& parentName);
  void rotate(char axis, float radians);
  void translate(float howFar);  //along z only!!
  void scale(float xscale, float yscale, float zscale);

  void update();
  void update(double*);

 private:
  string name;
  int levels;
  float **pointsList;
  float colors[3];
  int changedPoints, changedTransform;
  int pointNodeID, transformID;
  arMatrix4 tessTransform;
  
  HypDodecahedron **dodecahedra;
  
};

#endif
