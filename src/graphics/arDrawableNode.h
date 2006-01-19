//********************************************************
// Syzygy is licensed under the BSD license v2
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
  virtual ~arDrawableNode(){}

  void draw(arGraphicsContext*);
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  int getType();
  string getTypeAsString();
  int getNumber();
  void setDrawable(arDrawableType type, int number);
  void setDrawableViaString(const string& type, int number);

 protected:
  // DO NOT DRAW UNTIL WE HAVE BEEN INITIALIZED BY A MESSAGE.
  bool _firstMessageReceived;
  int _type;
  int _number;

  arStructuredData* _dumpData(int type, int number, bool owned);

  bool _0DPreDraw(arGraphicsNode* pointsNode,
		  arGraphicsContext* context,
                  float& blendFactor);
  bool _1DPreDraw(arGraphicsNode* pointsNode,
                  arGraphicsContext* context,
                  float& blendFactor);
  bool _2DPreDraw(arGraphicsNode* pointsNode,
                  arGraphicsNode* normal3Node,
                  arGraphicsContext* context,
                  float& blendFactor);
  string _convertTypeToString(int type);
  arDrawableType _convertStringToType(const string& type);
};

#endif
