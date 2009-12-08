//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICSSCREEN_H
#define AR_GRAPHICSSCREEN_H

#include "arMath.h"
#include "arSZGClient.h"
#include "arGraphicsCalling.h"

class SZG_CALL arGraphicsScreen {
 public:
  arGraphicsScreen();
  virtual ~arGraphicsScreen() {}
  // These five functions are inherited from arCamera
  virtual bool configure( arSZGClient& client );
  virtual bool configure( const string& screenName, arSZGClient& client );

  void setCenter(const arVector3& center);
  void setNormal(const arVector3& normal);
  void setUp(const arVector3& up);
  void setDimensions(float width, float height);
  arVector3 getNormal() const { return _normal; }
  arVector3 getUp() const { return _up; }
  arVector3 getCenter() const { return _center; }

  void setWidth( float width ) { setDimensions( width, _height ); }
  void setHeight( float height ) { setDimensions( _width, height ); }
  float getWidth() const { return _width; }
  float getHeight() const { return _height; }

  void setHeadMounted( bool hmd ) { _headMounted = hmd; }
  bool getHeadMounted() const { return _headMounted; }

  void setTile( arVector4& tile ) { setTile( int( tile[ 0 ] ), int( tile[ 1 ] ),
                                             int( tile[ 2 ] ), int( tile[ 3 ] ) ); }

  void setTile( int tileX, int numberTilesX, int tileY, int numberTilesY );
  arVector4 getTile() { return arVector4(_tileX, _numberTilesX,
                                        _tileY, _numberTilesY); }

  bool setUseFixedHeadMode( const std::string& usageMode );
  bool getIgnoreFixedHeadMode() const { return _ignoreFixedHeadMode; }
  bool getAlwaysFixedHeadMode() const { return _alwaysFixedHeadMode; }

  arVector3 getFixedHeadHeadPosition() const { return _fixedHeadPosition; }
  void setFixedHeadPosition( const arVector3& position ) { _fixedHeadPosition = position; }
  float getFixedHeadHeadUpAngle() const { return _fixedHeadUpAngle; }
  void setFixedHeadHeadUpAngle( float angle ) { _fixedHeadUpAngle = angle; }

 private:
  void _updateTileCenter();
  arVector3 _setCenter;
  float     _setHeight;
  float     _setWidth;
  arVector3 _center;
  arVector3 _normal;
  arVector3 _up;
  float     _height;
  float     _width;
  int _tileX;
  int _numberTilesX;
  int _tileY;
  int _numberTilesY;

  bool _headMounted;

  bool _ignoreFixedHeadMode;
  bool _alwaysFixedHeadMode;
  arVector3 _fixedHeadPosition;
  float _fixedHeadUpAngle;
};

SZG_CALL ostream& operator<<(ostream& s, arGraphicsScreen& g);

#endif // AR_GRAPHICSSCREEN_H

