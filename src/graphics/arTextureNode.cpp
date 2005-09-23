//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arTextureNode.h"
#include "arGraphicsDatabase.h"

arTextureNode::arTextureNode():
  _fileName(""),
  _alpha(0),
  _width(0),
  _height(0),
  _texture(NULL),
  _locallyHeldTexture(true){
  // A sensible default name.
  _name = "texture_node";
  // Does not compile on RedHat 8 if these are not in the constructor's body.
  _typeCode = AR_G_TEXTURE_NODE;
  _typeString = "texture";
}

arTextureNode::~arTextureNode(){
  // The only thing to do is to remove our reference to the arTexture.
  // This will result in it being deleted if we are the only object
  // still referencing it.
  if (_texture){
    _texture->unref();
  }
}

arStructuredData* arTextureNode::dumpData(){
  return _dumpData(_fileName, _alpha, _width, _height, 
                   _texture ? _texture->getPixels() : NULL);
}

bool arTextureNode::receiveData(arStructuredData* inData){
  if (inData->getID() != _g->AR_TEXTURE){
    cerr << "arTextureNode error: expected "
         << _g->AR_TEXTURE
         << " (" << _g->_stringFromID(_g->AR_TEXTURE) << "), not "
         << inData->getID()
         << " (" << _g->_stringFromID(inData->getID()) << "), for node \""
	 << getName() << "\".\n";
    return false;
  }

  _fileName = inData->getDataString(_g->AR_TEXTURE_FILE);
  // NOTE: Here is the flag via which we determine if the texture is
  // based on a resource handle or is based on a bitmap.
  if (inData->getDataDimension(_g->AR_TEXTURE_WIDTH) == 0) {
    _alpha = inData->getDataInt(_g->AR_TEXTURE_ALPHA);
    // If we are currently holding a referenced texture pointer, release.
    // If the texture is locally held, it will be deleted. If the texture
    // was from the arGraphicsDatabase store, then it will have its
    // reference count decremented... but will still exist in the store.
    // Either way, this _texture ptr should not be used by us anymore.
    if (_texture){
      _texture->unref();
    }
    // NOTE: The database returns a pointer with an extra ref added to it.
    _texture = _owningDatabase->addTexture(_fileName, &_alpha);
    // The texture is not locally held. The arGraphicsDatabase store owns it.
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

/// If alpha is < 0, then no alpha blending. This is the default.
/// If alpha is >= 0, then the low order 3 bytes of alpha are interpreted
/// as R (first 8 bits), G (next 8 bits), and B (next 8 bits).
void arTextureNode::setFileName(const string& fileName, int alpha){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(fileName, alpha, 0, 0, NULL);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _fileName = fileName;
    _alpha = alpha;
    if (_texture){
      // This will cause a delete in the case of a purely locally held texture.
      _texture->unref();
    }
    // We are free to create a new texture now. It will be locally held only.
    _texture = new arTexture();
    _texture->readImage(fileName.c_str(), _alpha, true);
    // We must note that the texture is locally held.
    _locallyHeldTexture = true;
  }
}

void arTextureNode::setPixels(int width, int height, char* pixels, bool alpha){
  if (_owningDatabase){
    arStructuredData* r = _dumpData("", alpha ? 1 : 0, width, height, pixels);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    // We're the only object that will actually use this.
    _addLocalTexture(alpha ? 1 : 0, width, height, pixels);
  }
}

arStructuredData* arTextureNode::_dumpData(const string& fileName, int alpha,
			                   int width, int height, 
                                           const char* pixels){
  arStructuredData* result = _g->makeDataRecord(_g->AR_TEXTURE);
  _dumpGenericNode(result,_g->AR_TEXTURE_ID);
  if (fileName != ""){
    // filename. 
    // Make sure we have set the data dimension of the _width to 0.
    // This is the flag the remote node uses to know it is not rendering
    // pixels.
    result->setDataDimension(_g->AR_TEXTURE_WIDTH, 0);
    // Make sure that we do not send unnecessary pixel info!
    result->setDataDimension(_g->AR_TEXTURE_PIXELS, 0);
    if (!result->dataIn(_g->AR_TEXTURE_ALPHA, &alpha, AR_INT, 1)
        || !result->dataInString(_g->AR_TEXTURE_FILE, fileName)) {
      delete result;
      return NULL;
    }
  }
  else {
    // Raw pixels, not a filename.
    if (!result->dataInString(_g->AR_TEXTURE_FILE, "") ||
        !result->dataIn(_g->AR_TEXTURE_ALPHA, &alpha, AR_INT, 1) ||
	!result->dataIn(_g->AR_TEXTURE_WIDTH, &width, AR_INT, 1) ||
	!result->dataIn(_g->AR_TEXTURE_HEIGHT, &height, AR_INT, 1)) {
      delete result;
      return NULL;
    }
    const int bytesPerPixel = alpha ? 4 : 3;
    const int cPixels = width * height * bytesPerPixel;
    result->setDataDimension(_g->AR_TEXTURE_PIXELS, cPixels);
    if (cPixels > 0){
      ARchar* outPixels 
        = (ARchar*)result->getDataPtr(_g->AR_TEXTURE_PIXELS, AR_CHAR);
      // If cPixels > 0, then it MUST be the case that there is a _texture.
      memcpy(outPixels, pixels, cPixels);
    }
  }
  return result;
}

void arTextureNode::_addLocalTexture(int alpha, int width, int height,
				     char* pixels){
  _width = width;
  _height = height;
  _alpha = alpha;
  // Important to set the file name to the empty string, since this is one way
  // we know whether or not there is a locally created bitmap.
  _fileName = "";
  // If _texture is NULL, allocate a new texture.
  // If _texture is non-NULL but locally held, reuse it (to avoid reallocating
  // a potentially large block of memory).
  // If _texture is non-NULL but not locally held, unref it and allocate a new
  // texture.
  if (!_texture) {
    _texture = new arTexture();
  }
  else{
    if (!_locallyHeldTexture){
      _texture->unref();
      _texture = new arTexture();
    }
  }
  // We assume that GL_MODULATE is the way to go.
  // This is mirrored in the addTexture method of arGraphicsDatabase.
  _texture->setTextureFunc(GL_MODULATE);
  _texture->fill(width, height, alpha, pixels);
  // This texture is locally held by us.
  _locallyHeldTexture = true;
}
