//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arTexture.h" 

#ifdef EnableJPEG
// it is necessary to have the extern declaration
extern "C"{
#include "jpeglib.h"
}
#include <setjmp.h>

struct arTexture_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};
#endif

arTexture::arTexture() :
  _fDirty(false),
  _width(0),
  _height(0),
  _alpha(false),
  _grayScale(false),
  _repeating(false),
  _texName(0),
  _mipmap(false),
  _textureFunc( GL_DECAL ),
  _pixels(NULL)
{
}

arTexture::~arTexture(){
  if (_pixels)
    delete [] _pixels;

#ifdef DISABLED
  // This causes a seg fault when called from arGraphicsDatabase::reset()
  // (when the geometry server is dkill'd and szgrender is still running here).
  if (_texName != 0)
    glDeleteTextures( 1, &_texName );
#endif
}

arTexture::arTexture( const arTexture& rhs ) :
  _fDirty( rhs._fDirty ),
  _width( rhs._width ),
  _height( rhs._height ),
  _alpha( rhs._alpha ),
  _grayScale( rhs._grayScale ),
  _repeating( rhs._repeating ),
  _texName( 0 ),  // activate() assigns a new opengl texture object
  _mipmap( rhs._mipmap ),
  _textureFunc( rhs._textureFunc )
{
  _pixels = new char[ numbytes() ];
  memcpy(_pixels, rhs._pixels, numbytes());
}

arTexture& arTexture::operator=( const arTexture& rhs ) {
  if (this == &rhs)
    return *this;
  _fDirty = rhs._fDirty;
  _width = rhs._width;
  _height = rhs._height;
  _alpha = rhs._alpha;
  _grayScale = rhs._grayScale;
  _repeating = rhs._repeating;
  _texName = 0; // activate() assigns a new opengl texture object
  _mipmap = rhs._mipmap;
  _textureFunc = rhs._textureFunc;
  if (_reallocPixels()) {
    memcpy(_pixels, rhs._pixels, numbytes());
    cout << "arTexture remark: copied " << numbytes() << " bytes.\n";
  } else {
    cout << "arTexture error: out of memory.\n";
  }
  return *this;
}

arTexture::arTexture( const arTexture& rhs, unsigned int left, unsigned int bottom,
                                            unsigned int width, unsigned int height ) :
  _fDirty( rhs._fDirty ),
  _alpha( rhs._alpha ),
  _grayScale( rhs._grayScale ),
  _repeating( rhs._repeating ),
  _texName( 0 ),  // activate() assigns a new opengl texture object
  _mipmap( rhs._mipmap ),
  _textureFunc( rhs._textureFunc )
{
  _pixels = rhs.getSubImage( left, bottom, width, height );
  if (!_pixels) {
    _width = 0;
    _height = 0;
  } else {
    _width = width;
    _height = height;
  }
}

bool arTexture::operator!() {
  return _pixels==0 || numbytes()==0;
}

void arTexture::activate() {
  glEnable(GL_TEXTURE_2D);

  if (_texName == 0) {
    glGenTextures(1,&_texName);
    _loadIntoOpenGL();
  } else {
    glBindTexture(GL_TEXTURE_2D, _texName);

    if (_fDirty) {
      _fDirty = false;
      _loadIntoOpenGL();
    }

    //**********************************************************
    /// A big kludge! It turns out that my trickiness
    /// in letting the remote render client draw partial scenes
    /// during the initial load bit me in the ass. I need better
    /// locking! Specifically, a texture context can have a different
    /// glTexEnvf bound to it at start-up than was intended...
    /// didn't see this before because I was using uniform contexts.
    /// The solution? Simply rebind as appropriate each time!
    //***********************************************************
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, _textureFunc );
  }
}

void arTexture::reactivate(){
  glEnable(GL_TEXTURE_2D);

  if (_texName == 0)
    glGenTextures(1,&_texName);
  _loadIntoOpenGL();
}

void arTexture::deactivate() {
 glDisable(GL_TEXTURE_2D);
}

// Create a special texture map to indicate a missing texture.
void arTexture::dummy() {
  _width = _height = 1; // 1 could be some other power of two.
  _alpha = false;
  _textureFunc = GL_DECAL;
  if (!_reallocPixels())
    return;
  for (int i= _height*_width - 1; i>=0; i--){
    char* pPixel = &_pixels[i * getDepth()];
    // Solid blue.
    pPixel[0] = 0;
    pPixel[1] = 0;
    pPixel[2] = 0xff; // 3f is black, 7f through df dark purple, ef through ff pure blue
  }
}

/// Copies an externally given array of pixels into internal memory.
/// DO NOT change this so that only the pointer is copied in.
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


/// Returns a pointer to a buffer containing a sub-image of the original.
/// Caller owns the new pointer.
char* arTexture::getSubImage( unsigned int left, unsigned int bottom, 
    unsigned int width, unsigned int height ) const {
  int depth = getDepth();
  char* newPixels = new char[width*height*depth];
  if (!newPixels) {
    cerr << "arTexture error: memory allocation failed in getSubImage().\n";
    return NULL;
  }
  char *newPtr = newPixels;
  for (unsigned int i=bottom; i < bottom+height; ++i) {
    for (unsigned int j=left; j < left+width; ++j) {
      if ((i >= _height)||(j >= _width)) { // Not sure if necessary, but what the heck.
        for (unsigned int k=0; k<depth; ++k) {
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
  const bool reload = (fEnable != _mipmap) && (_texName != 0);
  _mipmap = fEnable;
  if (reload)
    reactivate();
}

void arTexture::repeating(bool fEnable) {
  const bool reload = (fEnable != _repeating) && (_texName != 0);
  _repeating = fEnable;
  if (reload)
    reactivate();
}

void arTexture::grayscale(bool fEnable) {
  const bool reload = (fEnable != _grayScale) && (_texName != 0);
  _grayScale = fEnable;
  if (reload)
    reactivate();
}

bool arTexture::readImage(const string& fileName, int alpha, bool complain){
  return readImage(fileName, "", "", alpha, complain);
}

bool arTexture:: readImage(const string& fileName, const string& path,
			   int alpha, bool complain){
  return readImage(fileName, "", path, alpha, complain);
}

bool arTexture::readImage(const string& fileName, 
                          const string& subdirectory,
                          const string& path,
                          int alpha, bool complain){
  string extension = ar_getExtension(fileName);
  if (extension == "jpg"){
    return readJPEG(fileName, subdirectory, path, alpha, complain);
  }
  else if (extension == "ppm"){
    return readPPM(fileName, subdirectory, path, alpha, complain);
  }
  
  // if we've made it here, there must have been an unknown extension
  cerr << "arTexture error: asked to read image with unsupported extension "
       << extension << "\n";
  return false;  
}

bool arTexture::readPPM(const string& fileName, int alpha, bool complain){
  return readPPM(fileName, "", "", alpha, complain);
}

bool arTexture::readPPM(const string& fileName, const string& path,
			int alpha, bool complain){
  return readPPM(fileName, "", path, alpha, complain);
}

bool arTexture::readPPM(const string& fileName, 
                        const string& subdirectory,
                        const string& path,
                        int alpha, 
                        bool complain){
  // TODO TODO TODO TODO: HOW ABOUT GRAYSCALE PPM????????????
  FILE* fd = ar_fileOpen(fileName, subdirectory, path, "rb");
  if (!fd){
    if (complain){
      cerr << "arTexture error: readPPM(...) could not open file\n  "
	   << fileName << " for reading.\n";
    }
    return false;
  }

  char PPMHeader[3];
  fscanf(fd, "%s ", PPMHeader);
  if (strcmp(PPMHeader, "P3") && strcmp(PPMHeader, "P6")) {
    cout << "arTexture error: Unexpected header \""
	 << PPMHeader << "\" in PPM file \""
	 << "\" (not in binary format?).\n";
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

  // AT SOME POINT, WE MIGHT WANT TO CHECK IF THE IMAGE IS A VALID
  // TEXTURE (in terms of dimensions)... AND, IF NOT, DO SOME RESIZING
  // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO

  int maxGrey = -1;
  fscanf(fd, "%d", &maxGrey);
  _alpha = (alpha != -1);
  if (!_reallocPixels()){
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
    fread(localBuffer, _width*_height*3, 1, fd);
    int count = 0;
    for (int i= _height*_width - 1; i>=0; i--) {
      char* pPixel = &_pixels[i*getDepth()];
      pPixel[0] = localBuffer[count++];
      pPixel[1] = localBuffer[count++];
      pPixel[2] = localBuffer[count++];
    } 
    delete [] localBuffer;
  }
  else{
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
  return true;
}

/// Attempts to write the texture as a jpeg with the given file name in
/// the current working directory.
bool arTexture::writePPM(const string& fileName){
  return writePPM(fileName, "", "");
}

/// Attempts to write texture as a jpeg with the given file name on
/// the given path.
bool arTexture::writePPM(const string& fileName, const string& path){
  return writePPM(fileName, "", path);
}

/// Attempts to write texture as a jpeg with the given file name on
/// the given subdirectory of the given path
bool arTexture::writePPM(const string& fileName, const string& subdirectory, 
                         const string& path){
  
  FILE* fp = ar_fileOpen(fileName, subdirectory, path, "wb");

  if (fp == NULL) {
    cerr << "arTexture error: could not write ppm with\n"
	 << " file name = " << fileName << "\n"
	 << " on subdirectory (" << subdirectory << ")\n"
	 << " of path (" << path << ")\n";
    return false;
  }
  fprintf(fp,"P6\n");
  fprintf(fp,"%i %i\n", _width, _height);
  fprintf(fp,"255\n");
  // we might have everything stored internally as RGBA, this will return
  // pixel data in RGB format
  char* buffer = _packPixels();
  fwrite(buffer,1,_height*_width*3,fp);
  fclose(fp);
  return true;
}

bool arTexture::readJPEG(const string& fileName, int alpha, bool complain){
  return readJPEG(fileName, "", "", alpha, complain);
}

bool arTexture::readJPEG(const string& fileName, const string& path,
			 int alpha, bool complain){
  return readJPEG(fileName, "", path, alpha, complain);
}

/// Attempts to read a jpeg image file. 
bool arTexture::readJPEG(const string& fileName, 
                         const string& subdirectory,
                         const string& path,
                         int alpha, 
                         bool complain){
#ifdef EnableJPEG
  FILE* fd = ar_fileOpen(fileName, subdirectory, path, "rb");
  if (!fd){
    if (complain){
      cerr << "arTexture error: readJPEG(...) could not open file\n  "
  	   << fileName << " for reading.\n";
    }
    return false;
  }
  
  struct jpeg_decompress_struct _cinfo;
  struct arTexture_error_mgr _jerr;
  // We set up the normal JPEG error routines, then override error_exit.
  // THIS MUST OCCUR BEFORE jpeg_create_decompress!
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
    fclose(fd);
    return false;
  }
  while (_cinfo.output_scanline < _cinfo.output_height) {
    jpeg_read_scanlines(&_cinfo, buffer, 1);
    for (int i=0; i<row_stride; i++){
      if (_cinfo.output_components  == 3) {
        int x = i/3;
        int component = i%3;
        // getDepth() returns the internal depth, which might be 4 because
        // of RGBA
        // IMPORTANT NOTE: output_scanline increments by 1 after the
        // jpeg_read_scanlines command. Consequently, one needs to subtract
        // one below (but this is counterbalanced by another +1 due to the
        // line image flip).
        // ANOTHER IMPORTANT NOTE: OpenGL textures have as their first
        // line the bottom of the image... but jpeg's have as their first
        // line the top of the image
        _pixels[(_height-_cinfo.output_scanline)*_width*getDepth() 
                + x*getDepth() + component] = buffer[0][i];
      } else {
        // 1 component
        // NOTE: there might be a better way to do this using OpenGL
        // grayscale textures, which do exist. For now, just going ahead
        // and expanding the grayscale jpeg into an RGB one.
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
  
  return true;
#else
  cout << "arTexture warning: jpeg library not present. Could not read.\n";
  return false;
#endif
}

bool arTexture::writeJPEG(const string& fileName){
  return writeJPEG(fileName, "", "");
}

/// Attempts to write texture as a jpeg with the given file name on
/// the given path.
bool arTexture::writeJPEG(const string& fileName, const string& path){
  return writeJPEG(fileName, "", path);
}

/// Attempts to write texture as a jpeg with the given file name on
/// the given subdirectory of the given path
bool arTexture::writeJPEG(const string& fileName, const string& subdirectory, 
                          const string& path){
#ifdef EnableJPEG
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];	

  FILE* outFile = ar_fileOpen(fileName, subdirectory, path, "wb");

  if (outFile == NULL) {
    cerr << "arTexture error: could not write jpeg with\n"
	 << " file name = " << fileName << "\n"
	 << " on subdirectory (" << subdirectory << ")\n"
	 << " of path (" << path << ")\n";
    return false;
  }

  // we might be (internally) in RGBA format
  char* buf1 = _packPixels();

  JSAMPLE* image_buffer = (JSAMPLE*) buf1;
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
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  fclose(outFile);
  jpeg_destroy_compress(&cinfo);
  // don't forget to delete the allocated memory
  delete [] buf1;
  return true;
#else
  cout << "arTexture warning: jpeg library not present. Could not write.\n";
  return false;
#endif
}

bool arTexture::fill(int w, int h, bool alpha, const char* pixels) {
  if (w<=0 || h<=0 || !pixels)
    return false;

  if (_width != w || _height != h || _alpha != alpha || !_pixels) {
    _width = w;
    _height = h;
    _alpha = alpha;
    if (!_reallocPixels())
      return false;
  }
  if (_alpha)
    _textureFunc = GL_MODULATE;
  else
    _textureFunc = GL_DECAL;
  // else, we're just updating the pixel data and everything else is unchanged.
  memcpy(_pixels, pixels, numbytes());
  _fDirty = true;
  return true;
}

bool arTexture::flipHorizontal() {
  char *temp = _pixels;
  _pixels = new char[numbytes()];
  if (!_pixels) {
    cerr << "arTexture error: flipHorizontal() failed.\n";
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
  return _pixels ? true : false;
}

/// Certain pixels can be made transparent in our image. The file
/// reading routine packs pixels in an RGB fashion... and then this routine
/// goes ahead and does the alpha channel, if such has been requested.
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
  
  // AARGH! THIS MIGHT NOT BE THE RIGHT THING... PROBABLY THE CURRENT
  // DEFAULT IS TO BE DOING GL_MODULATE (i.e. lit textures)
  if (_alpha){
    _textureFunc = GL_MODULATE;
  }
  else{
    _textureFunc = GL_DECAL;
  }
}

/// Return an RGB array of pixels suitable for writing to a file. 
/// Note that we may have to overcome the fact that pixels are packed
/// internally in RGBA.
/// We also have to account for the fact that pixels are stored internally
/// in OpenGL format (i.e. as returned form glReadPixels or as in texture
/// memory, which means that the 1st line in memory is the bottom line of
/// the image. Note that this is the reverse of the way in which an image
/// is stored in a file, where the 1st line in the file is the top line of
/// the image. Consequently, we reverse that here as well.
char* arTexture::_packPixels(){
  char* buffer = new char[_width*_height*3];
  const int depth = getDepth();
  for (int i = 0; i < _height; i++){
    for (int j=0; j < _width; j++){
      // reverse the order of the scanlines
      buffer[3*((_height-i-1)*_width + j)] = _pixels[depth*(i*_width + j)];
      buffer[3*((_height-i-1)*_width + j)+1] = _pixels[depth*(i*_width + j)+1];
      buffer[3*((_height-i-1)*_width + j)+2] = _pixels[depth*(i*_width + j)+2];
    }
  }
  return buffer;
}

void arTexture::_loadIntoOpenGL() {
  glBindTexture(GL_TEXTURE_2D, _texName);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  _repeating ? GL_REPEAT : GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    _repeating ? GL_REPEAT : GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                  _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                  _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR); 
  // _width and _height must both be a power of two... otherwise
  // no texture will appear when this is invoked.
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, _textureFunc );
  GLint internalFormat = 0;
  if (_grayScale)
    internalFormat = _alpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
  else
    internalFormat = _alpha ? GL_RGBA : GL_RGB;
  if (_mipmap)
    gluBuild2DMipmaps(GL_TEXTURE_2D,
                      internalFormat,
                      _width, _height,
	              _alpha ? GL_RGBA : GL_RGB,
                      GL_UNSIGNED_BYTE, (GLubyte*) _pixels);
  else
    glTexImage2D(GL_TEXTURE_2D, 0,
                  internalFormat,
                  _width, _height, 0,
                  _alpha ? GL_RGBA : GL_RGB,
                  GL_UNSIGNED_BYTE, (GLubyte*) _pixels);
}
