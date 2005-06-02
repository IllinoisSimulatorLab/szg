//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_AXIS_ALIGNED_BOUNDING_BOX
#define AR_AXIS_ALIGNED_BOUNDING_BOX

#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

// WARNING: If a class ONLY exists in the .h, then we SHOULD NOT decorate it
// with SZG_CALL. In the Win32 case, this will lead to the linker getting
// confused when trying to import the class.
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
