//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_OBJ_SMOOTHING_GROUP
#define AR_OBJ_SMOOTHING_GROUP

#include "arMath.h"
#include "arGraphicsDatabase.h"
#include "arObjCalling.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

// Helper class for arOBJ objects
// todo: get rid of this, or put it in arOBJ's files
class SZG_CALL arOBJSmoothingGroup
{
  private:
    vector<int> _triangles;
  public:
    int    _name;
    inline unsigned int  total() {return _triangles.size()-1;}
    void        add(int newTriangle);
    inline int& operator[] (int i) { return _triangles[i]; } // returning a reference is unsafe!
    inline int  operator[] (int i) const {return _triangles[i];}
};

#endif //AR_OBJ_SMOOTHING_GROUP
