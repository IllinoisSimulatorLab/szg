//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEXTURE_H
#define AR_TEXTURE_H

#include "arGraphicsHeader.h"
#include "arDataType.h"
#include "arThread.h" // for arLock
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include "arGraphicsCalling.h"

using namespace std;

void SZG_CALL ar_setShareTexturesAmongContexts( bool onoff );
bool SZG_CALL ar_getShareTexturesAmongContexts();

// What should arTexture::_loadIntoOpenGL() do if texture
// dimensions are not powers of 2? Default is to print
// an error message and return. To get it to try anyway
// (will crash on some systems), call
// ar_setTextureAllowNotPowOf2(true);
void SZG_CALL ar_setTextureAllowNotPowOf2( bool onoff );
bool SZG_CALL ar_getTextureAllowNotPowOf2();
bool SZG_CALL ar_warnTextureAllowNotPowOf2();

// Texture map loaded from a file or from memory.

class SZG_CALL arTexture {
  friend void arTexture_setThreaded(bool);
 public:
  arTexture();
  virtual ~arTexture();
  arTexture( const arTexture& rhs );
  arTexture& operator=( const arTexture& rhs );
  arTexture( const arTexture& rhs, unsigned int left, unsigned int bottom,
                                   unsigned int width, unsigned int height );
  bool operator!() const;

  // Reference-counted because of how textures are shared in arGraphicsDatabase.
  int getRef() const;
  arTexture* ref();
  // Caller may not use *this after unref(), just like after ~arTexture().
  void unref(bool debug = false);

  bool activate(bool forceReload = false);
  void deactivate() const;

  int getWidth()  const { return _width; }
  int getHeight() const { return _height; }
  int getDepth()  const { return _alpha ? 4 : 3; }   // bytes per pixel
  int numbytes() const
    { return _width * _height * getDepth(); } // size of _pixels
  const char* getPixels() const { return _pixels; }
  void setPixels(char* pixels, int width, int height);
  char* getSubImage( unsigned int left, unsigned int bottom,
                     unsigned int width, unsigned int height ) const;
  void setTextureFunc( int texfunc ) { _textureFunc = texfunc; }
  void mipmap(bool fEnable);
  void repeating(bool fEnable);
  void grayscale(bool fEnable);
  bool dummy();

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

  bool readAlphaImage(const string& fileName, bool complain = true);
  bool readAlphaImage(const string& fileName, const string& path, bool complain = true);
  bool readAlphaImage(const string& fileName,
                 const string& subdirectory,
                 const string& path,
                 bool complain = true);
  bool readAlphaPPM(const string& fileName, bool complain = true);
  bool readAlphaPPM(const string& fileName, const string& path,
               bool complain = true);
  bool readAlphaPPM(const string& fileName,
               const string& subdirectory,
               const string& path,
               bool complain = true);
  bool readAlphaJPEG(const string& fileName, bool complain = true);
  bool readAlphaJPEG(const string& fileName, const string& path,
                bool complain = true);
  bool readAlphaJPEG(const string& fileName,
                const string& subdirectory,
                const string& path,
                bool complain = true);

  bool fill(int w, int h, bool alpha, const char* pixels);
  bool fillColor(int w, int h, char r, char g, char b, int alpha=-1);
  bool flipHorizontal();

 protected:
  bool _fDirty; // New _pixels need to be _loadIntoOpenGL()'d.
  int _width;
  int _height;
  bool _alpha; // true iff alpha channel exists
  bool _grayScale;
  bool _repeating; // Is texture periodic for coords outside [0, 1]?
  bool _mipmap; // use GL_LINEAR_MIPMAP_LINEAR or GL_LINEAR
  int _textureFunc;
  char* _pixels;

  arLock _lock; // guards _texNameMap

  // Handles to OpenGL textures, one per graphics context.
  static bool _threaded;
  GLuint _texName;
  map<ARint64, GLuint, less<ARint64> > _texNameMap;

  GLuint _sharedTextureID;

  arIntAtom _refs;

  bool _reallocPixels();
  void _assignAlpha(int);
  char* _packPixels() const;
  virtual bool _loadIntoOpenGL();
};

extern void arTexture_setThreaded(bool f = false);

#endif
