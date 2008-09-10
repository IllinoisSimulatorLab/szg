//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEXTURE_NODE_H
#define AR_TEXTURE_NODE_H

#include "arGraphicsNode.h"
#include "arGraphicsCalling.h"

// Texture map loaded from a file.

class SZG_CALL arTextureNode: public arGraphicsNode{
 public:
  arTextureNode();
  virtual ~arTextureNode();

  void draw(arGraphicsContext*) {}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  string getFileName() { return _fileName; }
  void setFileName(const string& fileName, int alpha = -1);
  void setPixels(int width, int height, char* pixels, bool alpha);

  // For database draw.
  arTexture* getTexture() { return _texture; }

 protected:
  string _fileName; // Empty means bitmap is locally created.
  int    _alpha;
  int    _width;
  int    _height;

  arTexture* _texture;
  bool _locallyHeldTexture; // _texture is owned by us, not the arGraphicsDatabase store.

  arStructuredData* _dumpData(const string& fileName, int alpha,
                              int width, int height, const char* pixels, bool owned);
  void _addLocalTexture(int alpha, int width, int height, char* pixels);
};

#endif
