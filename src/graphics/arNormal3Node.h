//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NORMAL3_NODE
#define AR_NORMAL3_NODE

#include <vector>
#include "arGraphicsArrayNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arNormal3Node: public arGraphicsArrayNode{
 public:
  arNormal3Node();
  virtual ~arNormal3Node(){}

  virtual void initialize(arDatabase* database);

  const float* getNormal3(int& number);
  void setNormal3(int number, float* normal3, int* IDs = NULL);
  vector<arVector3> getNormal3();
  void setNormal3(vector<arVector3>& normal3);
  void setNormal3(vector<arVector3>& normal3,
                  vector<int>& IDs);
};

#endif
