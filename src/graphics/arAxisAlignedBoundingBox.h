//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_AXIS_ALIGNED_BOUNDING_BOX
#define AR_AXIS_ALIGNED_BOUNDING_BOX

#include "arMath.h"

class arAxisAlignedBoundingBox{
 public:
  arAxisAlignedBoundingBox(){ xSize=ySize=zSize=0; }
  ~arAxisAlignedBoundingBox(){}
 
  arVector3 center;
  float xSize;
  float ySize;
  float zSize;
};

#endif
