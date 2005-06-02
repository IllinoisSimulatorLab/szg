//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEXTURE_NODE_H
#define AR_TEXTURE_NODE_H

#include "arGraphicsNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// Texture map loaded from a file.

class SZG_CALL arTextureNode: public arGraphicsNode{
 public:
  arTextureNode();
  ~arTextureNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  string getFileName(){ return _fileName; }
  void setFileName(const string& fileName);

 protected:
  string _fileName;
  int    _alpha;
  int    _width;
  int    _height;
  char*  _pixels;

  arStructuredData* _dumpData(const string& fileName, int alpha,
			      int width, int height, char* pixels);
};

#endif
