//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "slice.h"
#include "arGraphicsHeader.h"

slice::slice(){
  _width = 0;
  _height = 0;
  _bytes = NULL;
  _texName = 0;
}

slice::~slice(){
  if (_bytes){
    delete _bytes;
  }
}

char* slice::getPtr(){
  return _bytes; // dangerous -- returning pointer to private member.
}

void slice::allocate(int width, int height){
  if (_bytes){
    delete [] _bytes;
  }
  // we are allocating an RGBA texture
  _bytes = new char[4*width*height];
  _width = width;
  _height = height;
}

void slice::activate(){
  glEnable(GL_TEXTURE_2D);
  if (_texName == 0){
    glGenTextures(1,&_texName);
    glBindTexture(GL_TEXTURE_2D, _texName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL); 
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA,
		 _width, _height,0,GL_RGBA,
		 GL_UNSIGNED_BYTE, (GLubyte*) _bytes);
  }
  else{
    glBindTexture(GL_TEXTURE_2D, _texName);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL);
  }
}

void slice::reactivate(){
  glEnable(GL_TEXTURE_2D);
  if (_texName == 0){
    glGenTextures(1,&_texName);
    glBindTexture(GL_TEXTURE_2D, _texName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA,
		 _width, _height,0,GL_RGBA,
		 GL_UNSIGNED_BYTE, (GLubyte*) _bytes);
  }
  else{
    glBindTexture(GL_TEXTURE_2D, _texName);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_DECAL);
    glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA,
		 _width, _height,0,GL_RGBA,
		 GL_UNSIGNED_BYTE, (GLubyte*) _bytes);
  }
}

void slice::deactivate(){
 glDisable(GL_TEXTURE_2D);
}

int slice::getWidth(){
  return _width;
}

int slice::getHeight(){
  return _height;
}
