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
  _pixels(NULL){
  // Does not compile on RedHat 8 if these are not in the constructor's body.
  _typeCode = AR_G_TEXTURE_NODE;
  _typeString = "texture";
}

arStructuredData* arTextureNode::dumpData(){
  return _dumpData(_fileName, _alpha, _width, _height, _pixels);
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
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  if (true || inData->getDataDimension(_g->AR_TEXTURE_WIDTH) == 0) {
    _alpha = inData->getDataInt(_g->AR_TEXTURE_ALPHA);
    // SADLY, I DO NOT THINK THAT ALPHA IS BEING HANDLED CORRECTLY!
    _localTexture = _owningDatabase->addTexture(_fileName, &_alpha);
    _texture = &_localTexture;
    // zero out the pixels, if they exist from previous.
    if (_pixels){
      delete [] _pixels;
      _pixels = NULL;
    }
  }
  else {
    // Raw pixels, not a filename.
    _width = inData->getDataInt(_g->AR_TEXTURE_WIDTH);
    _height = inData->getDataInt(_g->AR_TEXTURE_HEIGHT);
    _alpha = inData->getDataInt(_g->AR_TEXTURE_ALPHA);
    ARchar* pixels 
      = (ARchar*)inData->getDataPtr(_g->AR_TEXTURE_PIXELS, AR_CHAR);
    if (!_localTexture) {
      // first time
      _localTexture = _owningDatabase->addTexture(_width, _height, 
                                                  _alpha, pixels);
      _texture = &_localTexture;
    }
    else {
      // later times
      ((arTexture*)_localTexture)->fill(_width, _height, _alpha, pixels);
    }
    const int bytesPerPixel = _alpha ? 4 : 3;
    const int cPixels = _width * _height * bytesPerPixel;
    if (_pixels){
      delete _pixels;
    }
    _pixels = new char[cPixels];
    // must store these for later dumps.
    memcpy(_pixels, pixels, cPixels);
  }

  return true;
}

void arTextureNode::setFileName(const string& fileName){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(fileName, 0, 0, 0, NULL);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _fileName = fileName;
    _pixels = NULL;
  }
}

arStructuredData* arTextureNode::_dumpData(const string& fileName, int alpha,
			                   int width, int height, 
                                           char* pixels){
  arStructuredData* result = _g->makeDataRecord(_g->AR_TEXTURE);
  _dumpGenericNode(result,_g->AR_TEXTURE_ID);
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  if (true || pixels == NULL){
    // filename. 
    // Make sure we have set the data dimension of the _width to 0.
    // This is the flag the remote node uses to know it is not rendering
    // pixels.
    result->setDataDimension(_g->AR_TEXTURE_WIDTH, 0);
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
    ARchar* outPixels 
      = (ARchar*)result->getDataPtr(_g->AR_TEXTURE_PIXELS, AR_CHAR);
    memcpy(outPixels, pixels, cPixels);
  }
  return result;
}
