#include "arPrecompiled.h"
#include "arLargeImage.h"

arLargeImage::arLargeImage( unsigned int tileWidth, unsigned int tileHeight ) {
  _setSizeNoRebuild( tileWidth, tileHeight );
}

arLargeImage::arLargeImage( const arTexture& x, unsigned int tileWidth, unsigned int tileHeight ) {
  if (_setSizeNoRebuild( tileWidth, tileHeight )) {
    setImage( x );
  } else {
    // go ahead & install image, don't make tiles.
    originalImage = x;
  }
}

arLargeImage::arLargeImage( const arLargeImage& x ) {
  if (_setSizeNoRebuild( x._tileWidth, x._tileHeight )) {
    setImage( x.originalImage );
  } else {
    // go ahead & install image, don't make tiles.
    originalImage = x.originalImage;
  }
}

arLargeImage& arLargeImage::operator=( const arLargeImage& x ) {
  if (this == &x)
    return *this;
  if (_setSizeNoRebuild( x._tileWidth, x._tileHeight )) {
    setImage( x.originalImage );
  } else {
    // go ahead & install image, don't make tiles.
    _tiles.clear();
    originalImage = x.originalImage;
  }
  return *this;
}  

arLargeImage::~arLargeImage(){
  _tiles.clear();
}

bool arLargeImage::setTileSize( unsigned int tileWidth, unsigned int tileHeight ) {
  if (!_setSizeNoRebuild( tileWidth, tileHeight )) {
    return false;
  }
  makeTiles();
  return true;
}

bool arLargeImage::_setSizeNoRebuild( unsigned int tileWidth, unsigned int tileHeight ) {
  if (tileWidth == 0) {
    cerr << "arLargeImage error: ignoring setTileSize(0).\n";
    return false;
  }
  _tileWidth = tileWidth;
  _tileHeight = (tileHeight == 0)?(_tileWidth):(tileHeight);
  cout << "arLargeImage remark: set tile size to (" << _tileWidth << "," << _tileHeight << ").\n";
  return true;
}

void arLargeImage::setImage( const arTexture& image ) {
  originalImage = image;
  makeTiles();
}

void arLargeImage::makeTiles() {
  _tiles.clear();
  unsigned int imageWidth = (unsigned int)originalImage.getWidth();
  unsigned int imageHeight = (unsigned int)originalImage.getHeight();
  _numTilesWide = imageWidth/_tileWidth;
  if (imageWidth % _tileWidth != 0) {
    _numTilesWide += 1;
  }
  _numTilesHigh = imageHeight/_tileHeight;
  if (imageHeight % _tileHeight != 0) {
    _numTilesHigh += 1;
  }
  for (int i=(_numTilesHigh-1); i>=0; --i) {
    unsigned int y = i*_tileHeight;
    for (unsigned int j=0; j<_numTilesWide; ++j) {
      unsigned int x = j*_tileWidth;
      _tiles.push_back( arTexture( originalImage, x, y, _tileWidth, _tileHeight ) );
    }
  }
  cout << "arLargeImage remark: made " << _tiles.size() << " tiles in a "
       << _numTilesWide << " X " << _numTilesHigh << " array.\n";
}

arTexture* arLargeImage::getTile( unsigned int colNum, unsigned int rowNum ) {
  unsigned int index( colNum + (_numTilesHigh-rowNum-1)*_numTilesWide );
  if (index >= _tiles.size()) {
    cerr << "arLargeImage error: operator[" << colNum << "," << rowNum << "] out of bounds, "
         << "max = [" << _numTilesWide-1 << "," << _numTilesHigh-1 << "].\n";
    return NULL;
  }
  return &_tiles[index];
}

// Not const because of activate().
void arLargeImage::draw() {
  const unsigned int imageWidth = originalImage.getWidth();
  const unsigned int imageHeight = originalImage.getHeight();
  const float xScale = ((float)_tileWidth*_numTilesWide)/imageWidth;
  const float yScale = ((float)_tileHeight*_numTilesHigh)/imageHeight;
  
  glPushMatrix();
  const float xDelta = xScale/((float)_numTilesWide);
  const float yDelta = yScale/((float)_numTilesHigh);
  float xlo = -.5;
  float ylo = -.5 + (_numTilesHigh-1)*yDelta;
  unsigned int colCount = 0;
  unsigned int rowCount = 0;
  std::vector< arTexture >::iterator iter;
  for (iter = _tiles.begin(); iter != _tiles.end(); ++iter) {
    iter->activate();

    float xTexHi = 1.;
    float yTexHi = 1.;
    float xhi = xlo + xDelta;
    float yhi = ylo + yDelta;

    if (colCount == _numTilesWide-1) {
      xhi = .5;
      if (imageWidth % _tileWidth != 0)
        xTexHi = (imageWidth%_tileWidth)/((float)_tileWidth);
    }
    if (rowCount == 0) {
      yhi = .5;
      if (imageHeight % _tileHeight != 0)
        yTexHi = (imageHeight%_tileHeight)/((float)_tileHeight);
    }

    glBegin(GL_QUADS);
      glTexCoord2f( 0., 0. );
      glVertex2f( xlo, ylo);
      glTexCoord2f( xTexHi, 0. );
      glVertex2f( xhi, ylo );
      glTexCoord2f( xTexHi, yTexHi );
      glVertex2f( xhi, yhi );
      glTexCoord2f( 0., yTexHi );
      glVertex2f( xlo, yhi );
    glEnd();

    ++colCount;
    xlo += xDelta;
    if (colCount == _numTilesWide) {
      ++rowCount;
      ylo -= yDelta;
      colCount = 0;
      xlo = -.5;
    }
  }
  // turn texturing off
  iter->deactivate();
  glPopMatrix();
}
