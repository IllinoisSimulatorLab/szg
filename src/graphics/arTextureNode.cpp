//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arTextureNode.h"
#include "arGraphicsDatabase.h"

arTextureNode::arTextureNode():
  _fileName(""),
  _alpha(0),
  _width(0),
  _height(0),
  _texture(NULL),
  _locallyHeldTexture(true) {
  _name = "texture_node";
  // Does not compile on RedHat 8 if these are not in the constructor's body.
  _typeCode = AR_G_TEXTURE_NODE;
  _typeString = "texture";
}

arTextureNode::~arTextureNode() {
  // The only thing to do is to remove our reference to the arTexture.
  // This will result in it being deleted if we are the only object
  // still referencing it.
  if (_texture) {
    _texture->unref();
  }
}

bool arTextureNode::receiveData(arStructuredData* inData) {
  if (!_g->checkNodeID(_g->AR_TEXTURE, inData->getID(), "arTextureNode"))
    return false;

  arGuard _(_nodeLock, "arTextureNode::receiveData");
  _fileName = inData->getDataString(_g->AR_TEXTURE_FILE);
  // NOTE: Here is the flag via which we determine if the texture is
  // based on a resource handle or is based on a bitmap.
  if (inData->getDataDimension(_g->AR_TEXTURE_WIDTH) == 0) {
    _alpha = inData->getDataInt(_g->AR_TEXTURE_ALPHA);
    if (_texture) {
      _texture->unref();
    }
    // addTexture() returns an already ref'd pointer.
    _texture = _owningDatabase->addTexture(_fileName, &_alpha);
    _locallyHeldTexture = false;
  }
  else {
    // Raw pixels, not a filename. We're the only object that will actually
    // use this. Consequently, there is no need to reference the texture.
    _addLocalTexture(inData->getDataInt(_g->AR_TEXTURE_ALPHA),
                  inData->getDataInt(_g->AR_TEXTURE_WIDTH),
                  inData->getDataInt(_g->AR_TEXTURE_HEIGHT),
		  (ARchar*)inData->getDataPtr(_g->AR_TEXTURE_PIXELS, AR_CHAR));
  }
  return true;
}

// If alpha is < 0, then no alpha blending. This is the default.
// If alpha is >= 0, then the low order 3 bytes of alpha are interpreted
// as R (first 8 bits), G (next 8 bits), and B (next 8 bits).
void arTextureNode::setFileName(const string& fileName, int alpha) {
  if (active()) {
    _nodeLock.lock("arTextureNode::setFileName active");
      arStructuredData* r = _dumpData(fileName, alpha, 0, 0, NULL, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arTextureNode::setFileName inactive");
    _fileName = fileName;
    _alpha = alpha;
    if (_texture) {
      _texture->unref();
    }
    _texture = new arTexture();
    _texture->readImage(fileName.c_str(), _alpha, true);
    _locallyHeldTexture = true;
  }
}

void arTextureNode::setPixels(int width, int height, char* pixels, bool alpha) {
  if (active()) {
    _nodeLock.lock("arTextureNode::setPixels active");
    arStructuredData* r =
      _dumpData("", alpha ? 1 : 0, width, height, pixels, true);
    _nodeLock.unlock();
    _owningDatabase->alter(r);
    recycle(r);
  }
  else{
    arGuard _(_nodeLock, "arTextureNode::setPixels inactive");
    // No other object will use this.
    _addLocalTexture(alpha ? 1 : 0, width, height, pixels);
  }
}

arStructuredData* arTextureNode::dumpData() {
  arGuard _(_nodeLock, "arTextureNode::dumpData");
  return _dumpData(
      _fileName, _alpha, _width, _height,
      _texture ? _texture->getPixels() : NULL,
      false);
}

arStructuredData* arTextureNode::_dumpData(
    const string& fileName, int alpha, int width, int height,
    const char* pixels, bool owned) {
  arStructuredData* r = _getRecord(owned, _g->AR_TEXTURE);
  _dumpGenericNode(r, _g->AR_TEXTURE_ID);
  if (fileName != "") {
    // Tell the remote node to not render pixels.
    r->setDataDimension(_g->AR_TEXTURE_WIDTH, 0);
    // Don't send unnecessary pixels.
    r->setDataDimension(_g->AR_TEXTURE_PIXELS, 0);
    if (!r->dataIn(_g->AR_TEXTURE_ALPHA, &alpha, AR_INT, 1) ||
        !r->dataInString(_g->AR_TEXTURE_FILE, fileName)) {
      delete r;
      return NULL;
    }
  }
  else {
    // Raw pixels, not a filename.
    if (!r->dataInString(_g->AR_TEXTURE_FILE, "") ||
        !r->dataIn(_g->AR_TEXTURE_ALPHA, &alpha, AR_INT, 1) ||
	!r->dataIn(_g->AR_TEXTURE_WIDTH, &width, AR_INT, 1) ||
	!r->dataIn(_g->AR_TEXTURE_HEIGHT, &height, AR_INT, 1)) {
      delete r;
      return NULL;
    }
    const int bytesPerPixel = alpha ? 4 : 3;
    const int cPixels = width * height * bytesPerPixel;
    r->setDataDimension(_g->AR_TEXTURE_PIXELS, cPixels);
    if (cPixels > 0) {
      ARchar* outPixels = (ARchar*)r->getDataPtr(_g->AR_TEXTURE_PIXELS, AR_CHAR);
      // There must be a _texture.
      memcpy(outPixels, pixels, cPixels);
    }
  }
  return r;
}

void arTextureNode::_addLocalTexture(int alpha, int width, int height, char* pixels) {
  _width = width;
  _height = height;
  _alpha = alpha;
  _fileName = "";
  if (!_texture) {
    _texture = new arTexture();
  }
  else if (!_locallyHeldTexture) {
    _texture->unref();
    _texture = new arTexture();
  }
  // Assume GL_MODULATE, like arGraphicsDatabase::addTexture().
  _texture->setTextureFunc(GL_MODULATE);
  _texture->fill(width, height, alpha, pixels);
  _locallyHeldTexture = true;
}
