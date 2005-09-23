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
  virtual ~arTextureNode();

  void draw(arGraphicsContext*){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  string getFileName(){ return _fileName; }
  void setFileName(const string& fileName, int alpha = -1);
  void setPixels(int width, int height, char* pixels, bool alpha);

  // Need to be able to get the texture itself for database draw.
  arTexture* getTexture(){ return _texture; }

 protected:
  string _fileName;
  int    _alpha;
  int    _width;
  int    _height;

  // The texture that we are using. Reference counted.
  arTexture* _texture; 
  // Is the texture held locally by us... or is it in some store somwehere,
  // like in the arGraphicsDatabase?
  bool _locallyHeldTexture;

  arStructuredData* _dumpData(const string& fileName, int alpha,
			      int width, int height, const char* pixels);
  void _addLocalTexture(int alpha, int width, int height, char* pixels);
};

#endif
