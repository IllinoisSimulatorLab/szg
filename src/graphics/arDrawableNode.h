//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DRAWABLE_NODE
#define AR_DRAWABLE_NODE

#include "arGraphicsNode.h"
#include "arGraphicsUtilities.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

enum arDrawableType {DG_POINTS = 0, DG_LINES = 1, DG_LINE_STRIP = 2, 
                     DG_TRIANGLES = 3, DG_TRIANGLE_STRIP = 4,
                     DG_QUADS = 5, DG_QUAD_STRIP = 6,
                     DG_POLYGON = 7};

class SZG_CALL arDrawableNode:public arGraphicsNode{
 public:
  arDrawableNode();
  ~arDrawableNode(){}

  void draw(arGraphicsContext*);
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  bool _01DPreDraw();
  void _01DPostDraw();
  bool _2DPreDraw();
  void _2DPostDraw();

  int getType();
  int getNumber();
  void setDrawable(arDrawableType type, int number);

 protected:
  // DO NOT DRAW UNTIL WE HAVE BEEN INITIALIZED BY A MESSAGE.
  bool _firstMessageReceived;
  int _type;
  int _number;

  arStructuredData* _dumpData(int type, int number);
};

#endif
