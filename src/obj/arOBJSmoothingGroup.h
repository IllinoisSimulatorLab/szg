//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_OBJ_SMOOTHING_GROUP
#define AR_OBJ_SMOOTHING_GROUP

// part of the Graphics Language Library, some simple tools for communicating
// 3D graphics between programs
// Written under the GNU General Public License
// -- BJS

#include <stdio.h>
#include <iostream>
#include "arMath.h"
#include "arGraphicsDatabase.h"
#include <string>
#include <vector>

/// Helper class for arOBJ objects
/// \todo get rid of this, or put it in arOBJ's files
class arOBJSmoothingGroup
{
  private:
    vector<int> _triangles;
    //int    _this; // why?
  public:
    int    _name;
    inline unsigned int  total() {return _triangles.size()-1;}
    void        add(int newTriangle);
    inline int& operator[] (int i) { return _triangles[i]; } ///< returning a reference is unsafe!
    inline int  operator[] (int i) const {return _triangles[i];}
};

#endif //AR_OBJ_SMOOTHING_GROUP
