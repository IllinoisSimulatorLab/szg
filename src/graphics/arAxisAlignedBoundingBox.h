//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_AXIS_ALIGNED_BOUNDING_BOX
#define AR_AXIS_ALIGNED_BOUNDING_BOX

#include "arMath.h"
#include "arGraphicsCalling.h"

class arAxisAlignedBoundingBox{
 public:
  arAxisAlignedBoundingBox() { xSize=ySize=zSize=0; }
  ~arAxisAlignedBoundingBox() {}

  arVector3 center;
  float xSize;
  float ySize;
  float zSize;
};

#endif
