//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEXTURE_H
#define AR_TEXTURE_H

#include "arGraphicsHeader.h"
#include "arCallingConventions.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
using namespace std;

/// Texture map loaded from a file, or loaded from a block of memory.

class SZG_CALL arTexture {
 public:
  arTexture();
  virtual ~arTexture();
  arTexture( const arTexture& rhs );
  arTexture& operator=( const arTexture& rhs );
  bool operator!();
  void activate();
  void reactivate();
  void deactivate();

  int getWidth()  const { return _width; }
  int getHeight() const { return _height; }
  int getDepth()  const { return _alpha ? 4 : 3; }   ///< bytes per pixel
  int numbytes() const
    { return _width * _height * getDepth(); } ///< size of _pixels
  const char* getPixels() const { return _pixels; }
  void setPixels(char* pixels, int width, int height);
  void setTextureFunc( int texfunc ) { _textureFunc = texfunc; }
  void mipmap(bool fEnable);
  void repeating(bool fEnable);
  void grayscale(bool fEnable);
  void dummy();

  bool readImage(const string& fileName, int alpha = -1, bool complain = true);
  bool readImage(const string& fileName, const string& path, int alpha = -1,
		 bool complain = true);
  bool readImage(const string& fileName, 
                 const string& subdirectory, 
                 const string& path,
                 int alpha = -1, bool complain = true);
  bool readPPM(const string& fileName, int alpha = -1, bool complain = true);
  bool readPPM(const string& fileName, const string& path, int alpha = -1,
               bool complain = true);
  bool readPPM(const string& fileName, 
               const string& subdirectory, 
               const string& path,
               int alpha = -1, bool complain = true);
  bool writePPM(const string& fileName); 
  bool writePPM(const string& fileName, const string& path);
  bool writePPM(const string& fileName, const string& subdirectory, 
                const string& path);
  bool readJPEG(const string& fileName, int alpha = -1, bool complain = true);
  bool readJPEG(const string& fileName, const string& path, int alpha = -1,
                bool complain = true);
  bool readJPEG(const string& fileName, 
                const string& subdirectory, 
                const string& path,
                int alpha = -1, bool complain = true);
  bool writeJPEG(const string& fileName);
  bool writeJPEG(const string& fileName, const string& path);
  bool writeJPEG(const string& fileName, const string& subdirectory, 
                 const string& path);
  
  bool fill(int w, int h, bool alpha, const char* pixels);
  bool flipHorizontal();
  GLuint glName() const { return _texName; } ///< OpenGL's name for texture

 protected:
  bool _fDirty; ///< New _pixels need to be _loadIntoOpenGL()'d.
  int _width;
  int _height;
  bool _alpha; ///< true iff alpha channel exists
  bool _grayScale;
  bool _repeating; ///< Is texture periodic for coords outside [0,1]?
  GLuint _texName; ///< internal OpenGL name of texture
  bool _mipmap; ///< use GL_LINEAR_MIPMAP_LINEAR or GL_LINEAR
  int _textureFunc;
  char* _pixels;

  bool _reallocPixels();
  void _assignAlpha(int);
  char* _packPixels();
  virtual void _loadIntoOpenGL();
};

#endif
