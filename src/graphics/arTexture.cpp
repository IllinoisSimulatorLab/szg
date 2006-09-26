//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arTexture.h" 
#include "arLogStream.h"

#ifdef EnableJPEG
extern "C"{
#include "jpeglib.h"
}
#include <setjmp.h>

struct arTexture_error_mgr {
  struct jpeg_error_mgr pub;	// "public" fields
  jmp_buf setjmp_buffer;	// for return to caller
};
#endif

arTexture::arTexture() :
  _fDirty(false),
  _width(0),
  _height(0),
  _alpha(false),
  _grayScale(false),
  _repeating(false),
  _mipmap(false),
  _textureFunc( GL_DECAL ),
  _pixels(NULL),
  _refs(1)
{
}

arTexture::~arTexture(){
  if (_pixels) {
    delete [] _pixels;
  }

#ifdef DISABLED
  // This segfaults when called from arGraphicsDatabase::reset(),
  // when the geometry server is dkill'd and szgrender is still running here.
  if (_texName != 0)
    glDeleteTextures( 1, &_texName );
#endif
}

arTexture::arTexture( const arTexture& rhs ) :
  _width( rhs._width ),
  _height( rhs._height ),
  _alpha( rhs._alpha ),
  _grayScale( rhs._grayScale ),
  _repeating( rhs._repeating ),
  _mipmap( rhs._mipmap ),
  _textureFunc( rhs._textureFunc ),
  _refs(1)
{
  // Note above that this object starts out with one reference.

  // We must set the "dirty" flag so that this texture will be loaded into
  // OpenGL on the next try. (Because we have changed the pixels)
  _fDirty = true;

  _pixels = new char[ numbytes() ];
  if (!_pixels) {
    ar_log_warning() << "arTexture out of memory in copy constructor.\n";
    return;
  }
  memcpy(_pixels, rhs._pixels, numbytes());
}

arTexture& arTexture::operator=( const arTexture& rhs ) {
  if (this == &rhs)
    return *this;
  // We must set the "dirty" flag so that this texture will be loaded into
  // OpenGL on the next try. (because we are changing the pixels)
  _fDirty = true;
  _width = rhs._width;
  _height = rhs._height;
  _alpha = rhs._alpha;
  _grayScale = rhs._grayScale;
  _repeating = rhs._repeating;
  _mipmap = rhs._mipmap;
  _textureFunc = rhs._textureFunc;
  // In this case, the number of references should not change. After the
  // assignment operation, the number of external objects using this
  // texture will remain the same. There will just be a different image
  // inside it. 
  if (_reallocPixels()) {
    memcpy(_pixels, rhs._pixels, numbytes());
    ar_log_remark() << "arTexture remark: copied " << numbytes() << " bytes.\n";
  } else {
    ar_log_remark() << "arTexture error: _reallocPixels() failed in operator=().\n";
  }
  return *this;
}

arTexture::arTexture( const arTexture& rhs, 
                      unsigned int left, unsigned int bottom,
                      unsigned int width, unsigned int height ) :
  _alpha( rhs._alpha ),
  _grayScale( rhs._grayScale ),
  _repeating( rhs._repeating ),
  _mipmap( rhs._mipmap ),
  _textureFunc( rhs._textureFunc ),
  _refs(1)
{
  // Note how this texture has exactly one reference (this is what makes
  // sense for copy constructors). (see above)

  // We must set the "dirty" flag so that this texture will be loaded into
  // OpenGL on the next try.
  _fDirty = true;

  // Get a new chunk of memory.
  _pixels = rhs.getSubImage( left, bottom, width, height );
  if (!_pixels) {
    ar_log_warning() << "arTexture failed to rhs.getSubImage().\n";
    _width = 0;
    _height = 0;
  } else {
    _width = width;
    _height = height;
  }
}

bool arTexture::operator!() const {
  return _pixels==0 || numbytes()==0;
}

int arTexture::getRef(){
  _lock.lock();
    const int result = _refs;
  _lock.unlock();
  return result;
}

arTexture* arTexture::ref(){
  // Returning the pointer allows us to atomically create
  // a reference to the object.
  _lock.lock();
    _refs++;
  _lock.unlock();
  return this;
}

void arTexture::unref(bool fDebug){
  _lock.lock();
    const bool fDelete = --_refs == 0;
  _lock.unlock();
  if (fDelete){
    if (fDebug)
      ar_log_debug() << "arTexture: deleting.\n";
    delete this;
  }
}

bool arTexture::activate(bool forceRebind) {
  glEnable(GL_TEXTURE_2D);
  _lock.lock();
  const ARint64 threadID =
#ifdef AR_USE_WIN_32
    GetCurrentThreadId();
#else
    ARint64(pthread_self());
#endif
  // Has it been used so far?
  map<ARint64,GLuint,less<ARint64> >::const_iterator i = _texNameMap.find(threadID);
  GLuint temp;
  if (i == _texNameMap.end()){
    glGenTextures(1,&temp);
    if (temp == 0) {
      ar_log_warning() << "arTexture glGenTextures() failed in activate().\n";
      _lock.unlock();
      return false;
    }
    _texNameMap.insert(map<ARint64,GLuint,less<ARint64> >::value_type
                       (threadID, temp));
    // Load the bitmap on the graphics card, anyways.
    forceRebind = true;
  }
  else{
    temp = i->second;
  }
  _lock.unlock();

  glBindTexture(GL_TEXTURE_2D, temp);
  // Make the scene graph use the right GL_TEXTURE_ENV_MODE to draw textures.
  // Weird, glBindTexture() should handle this.
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, _textureFunc );
  if (_fDirty || forceRebind){
    _loadIntoOpenGL();
  }
  return true;
}

void arTexture::deactivate() const {
 glDisable(GL_TEXTURE_2D);
}

// Create a special texture map to indicate a missing texture.
bool arTexture::dummy() {
  _width = _height = 1; // 1 could be some other power of two.
  _alpha = false;
  _textureFunc = GL_MODULATE;
  if (!_reallocPixels()) {
    ar_log_warning() << "arTexture _reallocPixels() failed in dummy().\n";
    return false;
  }
  for (int i = _height*_width - 1; i>=0; --i) {
    char* pPixel = &_pixels[i * getDepth()];
    // Solid blue.
    pPixel[0] = 0;
    pPixel[1] = 0;
    pPixel[2] = 0xff; // 3f is black, 7f through df dark purple, ef through ff pure blue
  }
  return true;
}

// Copies an externally given array of pixels into internal memory.
// DO NOT change this so that only the pointer is copied in.
// Please note: the fill(...) method actually deals with transparency.
// This deals with RGB textures only.
void arTexture::setPixels(char* pixels, int width, int height){
  // AARGH! This does not deal with different pixel formats!!!
  if (_pixels){
    delete [] _pixels;
  }
  _pixels = new char[height*width*3];
  memcpy(_pixels,pixels,height*width*3);
  _width = width;
  _height = height;
  // AARGH! not supporting transparency
  _alpha = false;
}


// Returns a pointer to a buffer containing a sub-image of the original.
// Caller owns the new pointer.
char* arTexture::getSubImage( unsigned int left, unsigned int bottom, 
    unsigned int width, unsigned int height ) const {
  int depth = getDepth();
  char* newPixels = new char[width*height*depth];
  if (!newPixels) {
    ar_log_warning() << "arTexture out of memory in getSubImage().\n";
    return NULL;
  }
  char *newPtr = newPixels;
  for (unsigned int i=bottom; i < bottom+height; ++i) {
    for (unsigned int j=left; j < left+width; ++j) {
      if ((i >= (unsigned int)_height)||(j >= (unsigned int)_width)) { // Not sure if necessary, but what the heck.
        for (unsigned int k=0; k<(unsigned int)depth; ++k) {
          *newPtr++ = (char)0;
        }
      } else {
        char *oldPtr = _pixels + depth*(j+i*_width);
        memcpy( newPtr, oldPtr, depth );
        newPtr += depth;
      }
    }
  }
  return newPixels;
}

void arTexture::mipmap(bool fEnable) {
  _fDirty = true;
  _mipmap = fEnable;
}

void arTexture::repeating(bool fEnable) {
  _fDirty = true;
  _repeating = fEnable;
}

void arTexture::grayscale(bool fEnable) {
  _fDirty = true;
  _grayScale = fEnable;
}

bool arTexture::readImage(const string& fileName, int alpha, bool complain) {
  return readImage(fileName, "", "", alpha, complain);
}

bool arTexture:: readImage(const string& fileName, const string& path,
			   int alpha, bool complain) {
  return readImage(fileName, "", path, alpha, complain);
}

bool arTexture::readImage(const string& fileName, 
                          const string& subdirectory,
                          const string& path,
                          int alpha, bool complain) {
  const string& extension = ar_getExtension(fileName);
  if (extension == "jpg")
    return readJPEG(fileName, subdirectory, path, alpha, complain);

  if (extension == "ppm")
    return readPPM(fileName, subdirectory, path, alpha, complain);
  
  ar_log_warning() << "unsupported texture file format '." << extension << "'.\n";
  return false;  
}

bool arTexture::readPPM(const string& fileName, int alpha, bool complain) {
  return readPPM(fileName, "", "", alpha, complain);
}

bool arTexture::readPPM(const string& fileName, const string& path,
			int alpha, bool complain) {
  return readPPM(fileName, "", path, alpha, complain);
}

bool arTexture::readPPM(const string& fileName, 
                        const string& subdirectory,
                        const string& path,
                        int alpha, 
                        bool complain) {
  // todo: grayscale ppm
  FILE* fd = ar_fileOpen(fileName, subdirectory, path, "rb");
  if (!fd){
    if (complain){
      ar_log_warning() << "arTexture readPPM failed to open '" <<
        fileName << "'.\n";
    }
    return false;
  }

#ifndef BROKEN
  char PPMHeader[4]; // fscanf's %3c averts danger of buffer overflow
  fscanf(fd, "%3c ", PPMHeader);
  if (PPMHeader[2] == '\n') {
    PPMHeader[2] = '\0';
  }
  // This reads in the end-of-line, which it should discard but doesn't.
#else
  char PPMHeader[3]; // buffer overflow
  fscanf(fd, "%s ", PPMHeader);
#endif
  if (strcmp(PPMHeader, "P3") && strcmp(PPMHeader, "P6")) {
    ar_log_warning() << "unexpected (nonbinary?) header '" <<
      PPMHeader << "' in PPM file '" << fileName << "'.\n";
    return false;
  }
  
  // Strip some characters.
  char buffer[512];
  while (true) {
    const char nextChar = getc(fd);
    ungetc(nextChar,fd);
    if (nextChar != '#'){
      break;
    }
    fgets(buffer, sizeof(buffer), fd);
  }

  fscanf(fd, "%d %d", &_width, &_height);

  // todo: validate image dimensions (powers of two),
  // todo: if invalid, resize.

  int maxGrey = -1;
  fscanf(fd, "%d", &maxGrey);
  _alpha = (alpha != -1);
  if (!_reallocPixels()){
    ar_log_warning() << "arTexture readPPM _reallocPixels out of memory.\n";
    fclose(fd);
    return false;
  }

  // Get the carriage return before the pixel data.
  fgetc(fd);
  // AARGH!!!! I think that the code below is definitely WRONG in
  // reordering the pixels for the ppm from file order to texture order...
  // It seems to flip things horizontally as well as verically... DOH!!!
  // This would, in fact, be GLOBALLY obnoxious to change!!!
  // Better make sure everything is working correctly first... and then
  // change it.
  if (!strcmp(PPMHeader,"P6")){
    // BINARY PPM FILE
    char* localBuffer = new char[ _width*_height*3 ];
    if (!localBuffer) {
      ar_log_warning() << "arTexture readPPM localBuffer out of memory.\n";
      return false;
    }
    fread(localBuffer, _width*_height*3, 1, fd);
    int count = 0;
    for (int i= _height*_width - 1; i>=0; i--) {
      char* pPixel = &_pixels[i*getDepth()];
      pPixel[0] = localBuffer[count++];
      pPixel[1] = localBuffer[count++];
      pPixel[2] = localBuffer[count++];
    } 
    delete [] localBuffer;
  } else {
    // ASCII PPM FILE
    int aPixel[3];
    // AARGH!!!! definitely reversed horizontally, much like the above!
    for (int i = _height*_width -1; i >= 0; i--) {
      fscanf(fd, "%d %d %d", &(aPixel[0]), &(aPixel[1]), &(aPixel[2]));
      _pixels[( i*getDepth())] = (unsigned char)aPixel[0];
      _pixels[( i*getDepth()) + 1] = (unsigned char)aPixel[1];
      _pixels[( i*getDepth()) + 2] = (unsigned char)aPixel[2];
    }
  }

  _assignAlpha(alpha);
  
  fclose(fd);
  _fDirty = true;
  return true;
}

// Write the texture as a jpeg with the given file name in
// the current working directory.
bool arTexture::writePPM(const string& fileName) {
  return writePPM(fileName, "", "");
}

// Write texture as a jpeg with the given file name on the given path.
bool arTexture::writePPM(const string& fileName, const string& path) {
  return writePPM(fileName, "", path);
}

// Write texture as a jpeg with the given file name on
// the given subdirectory of the given path
bool arTexture::writePPM(const string& fileName, const string& subdirectory, 
                         const string& path) {
  
  FILE* fp = ar_fileOpen(fileName, subdirectory, path, "wb");

  if (fp == NULL) {
    ar_log_warning() << "arTexture failed to write ppm file '" <<
      fileName << "' on subdirectory '" << subdirectory << "' of path '" <<
      path << "'.\n";
    return false;
  }
  fprintf(fp,"P6\n");
  fprintf(fp,"%i %i\n", _width, _height);
  fprintf(fp,"255\n");
  // we might have everything stored internally as RGBA, this will return
  // pixel data in RGB format
  char* buffer = _packPixels();
  if (!buffer) {
    ar_log_warning() << "arTexture writePPM _packPixels failed.\n";
    return false;
  }
  fwrite(buffer,1,_height*_width*3,fp);
  fclose(fp);
  delete[] buffer;
  return true;
}

bool arTexture::readJPEG(const string& fileName, int alpha, bool complain) {
  return readJPEG(fileName, "", "", alpha, complain);
}

bool arTexture::readJPEG(const string& fileName, const string& path,
			 int alpha, bool complain) {
  return readJPEG(fileName, "", path, alpha, complain);
}

// Read a jpeg file. 
bool arTexture::readJPEG(const string& fileName, 
                         const string& subdirectory,
                         const string& path,
                         int alpha, 
                         bool complain) {
#ifndef EnableJPEG
  ar_log_warning() << "compiled without jpeg support.  No textures.\n";
  return false;
#else
  FILE* fd = ar_fileOpen(fileName, subdirectory, path, "rb");
  if (!fd) {
    if (complain) {
      ar_log_warning() << "arTexture failed to read jpg file '" <<
  	fileName << "'.\n";
    }
    return false;
  }
  
  struct jpeg_decompress_struct _cinfo;
  struct arTexture_error_mgr _jerr;
  // Set up the normal JPEG error routines, then override error_exit.
  // Before jpeg_create_decompress!
  _cinfo.err = jpeg_std_error(&_jerr.pub);
  jpeg_create_decompress(&_cinfo);
  
  jpeg_stdio_src(&_cinfo, fd);
  jpeg_read_header(&_cinfo, TRUE);
  jpeg_start_decompress(&_cinfo);
  int row_stride = _cinfo.output_width * _cinfo.output_components;
  JSAMPLE* buffer[1];
  buffer[0] = new JSAMPLE[row_stride];
  // initialize storage here
  _width = _cinfo.output_width;
  _height = _cinfo.output_height;
  _alpha = (alpha != -1);
  if (!_reallocPixels()){
    ar_log_warning() << "arTexture readJPEG _reallocPixels failed.\n";
    fclose(fd);
    return false;
  }

  while (_cinfo.output_scanline < _cinfo.output_height) {
    jpeg_read_scanlines(&_cinfo, buffer, 1);
    for (int i=0; i<row_stride; i++){
      if (_cinfo.output_components  == 3) {
        const int x = i/3;
        const int component = i%3;
        // getDepth() returns the internal depth, which might be 4 because
        // of RGBA
        // Output_scanline increments after the
        // jpeg_read_scanlines command, so decrement
        // below (counterbalanced by another increment for the
        // line image flip).
        // OpenGL textures have as their first
        // line the bottom of the image, jpeg's the top.
        _pixels[(_height-_cinfo.output_scanline)*_width*getDepth() 
                + x*getDepth() + component] = buffer[0][i];
      } else {
        // 1 component
        // Might be a better way to do this using OpenGL
        // grayscale textures. For now, just
        // expand the grayscale jpeg into an RGB one.
        _pixels[(_height-_cinfo.output_scanline)*_width*getDepth() 
                + i*getDepth()] = buffer[0][i];
        _pixels[(_height-_cinfo.output_scanline)*_width*getDepth() 
                + i*getDepth() + 1] = buffer[0][i];
        _pixels[(_height-_cinfo.output_scanline)*_width*getDepth() 
                + i*getDepth() + 2] = buffer[0][i]; 
      }
    }
  }
  jpeg_finish_decompress(&_cinfo);
  _assignAlpha(alpha);
  fclose(fd);
  _fDirty = true;
  return true;
#endif
}

bool arTexture::writeJPEG(const string& fileName) {
  return writeJPEG(fileName, "", "");
}

// Write texture as a jpeg with the given file name on the given path.
bool arTexture::writeJPEG(const string& fileName, const string& path) {
  return writeJPEG(fileName, "", path);
}

// Write texture as a jpeg with the given file name on
// the given subdirectory of the given path
bool arTexture::writeJPEG(const string& fileName, const string& subdirectory, 
                          const string& path) {
#ifndef EnableJPEG
  ar_log_warning() << "compiled without jpeg support.  No textures.\n";
  return false;
#else

  FILE* outFile = ar_fileOpen(fileName, subdirectory, path, "wb");
  if (outFile == NULL) {
    ar_log_warning() << "arTexture failed to write jpeg file '" <<
      fileName << "' on subdirectory '" << subdirectory << "' of path '" <<
      path << "'.\n";
    return false;
  }

  // might be (internally) in RGBA format
  char* buf1 = _packPixels();
  if (!buf1) {
    ar_log_warning() << "arTexture writeJPEG _packPixels() failed.\n";
    return false;
  }

  JSAMPLE* image_buffer = (JSAMPLE*) buf1;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, outFile);
  cinfo.image_width = _width; 
  cinfo.image_height = _height;
  cinfo.input_components = 3;	
  cinfo.in_color_space = JCS_RGB; 
  jpeg_set_defaults(&cinfo);
  // I think this means the highest quality settings
  jpeg_set_quality(&cinfo, 100, TRUE);
  jpeg_start_compress(&cinfo, TRUE);
  int row_stride = _width * 3;

  while (cinfo.next_scanline < cinfo.image_height) {
    const JSAMPROW row_pointer = &image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, &row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(outFile);
  jpeg_destroy_compress(&cinfo);
  delete[] buf1;
  return true;
#endif
}

bool arTexture::fill(int w, int h, bool alpha, const char* pixels) {
  if (w<=0 || h<=0 || !pixels) {
    return false;
  }

  // For speed, call _reallocPixels only if the number of bytes allocated
  // will change (or no memory has been allocated yet).
  if (_width != w || _height != h || _alpha != alpha || !_pixels) {
    _width = w;
    _height = h;
    _alpha = alpha;
    if (!_reallocPixels()) {
      ar_log_warning() << "arTexture fill _reallocPixels failed.\n";
      return false;
    }
  }
  // Texture blending requires GL_MODULATE mode.
  if (_alpha)
    _textureFunc = GL_MODULATE;
  // Otherwise, update only the pixels.
  memcpy(_pixels, pixels, numbytes());
  _fDirty = true;
  return true;
}

bool arTexture::flipHorizontal() {
  if (!_pixels) {
    ar_log_warning() << "arTexture flipHorizontal had no pixels.\n";
    return false;
  }
  char *temp = _pixels;
  _pixels = new char[numbytes()];
  if (!_pixels) {
    ar_log_warning() << "arTexture flipHorizontal out of memory.\n";
    return false;
  }
  const int depth = getDepth();
  for (int i = 0; i<_height; i++) {
    int k = _width-1;
    for (int j = 0; j<_width; j++, k--) {
      const char *pOld = temp + depth*(k+i*_width);
      char *pNew = _pixels + depth*(j+i*_width);
      memcpy(pNew, pOld, depth);
    }
  }
  delete[] temp;
  return true;
}

bool arTexture::_reallocPixels() {
  if (_pixels)
    delete [] _pixels;
  _pixels = new char[numbytes()];
  _fDirty = true;
  return _pixels;
}

// Certain pixels can be made transparent in our image. The file
// reading routine packs pixels in an RGB fashion... and then this routine
// goes ahead and does the alpha channel, if such has been requested.
void arTexture::_assignAlpha(int alpha){
  if (_alpha) {
    // Parse all the pixels and turn them into an alpha channel.
    for (int i = _height*_width -1; i >= 0; i--) {
      unsigned char* pPixel = (unsigned char*)&_pixels[i * getDepth()];
      const int test = (int(pPixel[2])<<16) 
                        | (int(pPixel[1])<<8) 
                        | int(pPixel[0]);
      pPixel[3] = (test == alpha) ? 0 : 255;
    }
  }
  
  // Texture blending only works if we are in GL_MODULATE mode.
  if (_alpha){
    _textureFunc = GL_MODULATE;
  }
}

// Return an RGB array of pixels suitable for writing to a file. 
// Maybe work around the pixels' internal RGBA packing.
// Also account for the pixels' internal storage
// in OpenGL format (as returned form glReadPixels or as in texture
// memory, which means that the 1st line in memory is the bottom line of
// the image). This is the reverse of how an image
// is stored in a file, where the 1st line in the file is the top line of
// the image. So reverse that as well.
char* arTexture::_packPixels(){
  char* buffer = new char[_width*_height*3];
  if (!buffer) {
    ar_log_warning() << "arTexture _packPixels out of memory.\n";
    return NULL;
  }
  const int depth = getDepth();
  for (int i = 0; i < _height; i++){
    for (int j=0; j < _width; j++){
      // Reverse the scanlines' order.
      // todo: use memcpy instead of the j-loops.
      buffer[3*((_height-i-1)*_width + j)] = _pixels[depth*(i*_width + j)];
      buffer[3*((_height-i-1)*_width + j)+1] = _pixels[depth*(i*_width + j)+1];
      buffer[3*((_height-i-1)*_width + j)+2] = _pixels[depth*(i*_width + j)+2];
    }
  }
  return buffer;
}

void arTexture::_loadIntoOpenGL() {
  if (!_pixels) {
    return;
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  _repeating ? GL_REPEAT : GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    _repeating ? GL_REPEAT : GL_CLAMP);
//  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
//                  _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                  _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR); 
  // _width and _height must both be a power of two... otherwise
  // no texture will appear when this is invoked.
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, _textureFunc );
  GLint internalFormat = 0;
  if (_grayScale) {
    internalFormat = _alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
  } else {
    internalFormat = _alpha ? GL_RGBA : GL_RGB;
  }
  if (_mipmap) {
    gluBuild2DMipmaps(GL_TEXTURE_2D,
                      internalFormat,
                      _width, _height,
	              _alpha ? GL_RGBA : GL_RGB,
                      GL_UNSIGNED_BYTE, (GLubyte*) _pixels);
  } else {
    glTexImage2D(GL_TEXTURE_2D, 0,
                  internalFormat,
                  _width, _height, 0,
                  _alpha ? GL_RGBA : GL_RGB,
                  GL_UNSIGNED_BYTE, (GLubyte*) _pixels);
  }
  _fDirty = false;
}
