//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef SLICE_H
#define SLICE_H

#include "arGraphicsHeader.h"
#include <stdlib.h>

class slice{
  // Needs assignment operator and copy constructor, for pointer members.
 public:
  slice();
  ~slice();
  char* getPtr();
  void allocate(int, int);
  void activate();
  void reactivate();
  void deactivate();
  int getHeight();
  int getWidth();
 private:
  int _width;
  int _height;
  char* _bytes;
  GLuint _texName;
};

#endif
