#ifndef LARGE_IMAGE_H
#define LARGE_IMAGE_H

#include "arTexture.h"
#include "arGraphicsCalling.h"

#include <vector>

class SZG_CALL arLargeImage {
public:
  arLargeImage( unsigned int tileWidth=256, unsigned int tileHeight=0 );
  arLargeImage( const arTexture& x, unsigned int tileWidth=256, unsigned int tileHeight=0 );
  arLargeImage( const arLargeImage& x );
  arLargeImage& operator=( const arLargeImage& x );
  virtual ~arLargeImage();

  bool setTileSize( unsigned int tileWidth, unsigned int tileHeight=0 );
  void setImage( const arTexture& image );
  unsigned int getWidth() const { return originalImage.getWidth(); }
  unsigned int getHeight() const { return originalImage.getHeight(); }
  unsigned int numTilesWide() const { return _numTilesWide; }
  unsigned int numTilesHigh() const { return _numTilesHigh; }
  virtual void makeTiles();
  // Indices start at lower left.
  arTexture* getTile( unsigned int colNum, unsigned int rowNum );
  virtual void draw();

  arTexture originalImage;

protected:
  bool _setSizeNoRebuild( unsigned int width, unsigned int height );
  unsigned int _tileWidth;
  unsigned int _tileHeight;
  unsigned int _numTilesWide;
  unsigned int _numTilesHigh;
  std::vector< arTexture > _tiles;
};

#endif
