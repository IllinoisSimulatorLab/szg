//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NORMAL3_NODE
#define AR_NORMAL3_NODE

#include "arGraphicsArrayNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arNormal3Node: public arGraphicsArrayNode{
 public:
  arNormal3Node(arGraphicsDatabase*);
  ~arNormal3Node(){}

  float* getNormal3(int& number);
  void   setNormal3(int number, float* normal3, int* IDs = NULL);
};

#endif
