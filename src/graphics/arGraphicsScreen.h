//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICSSCREEN_H
#define AR_GRAPHICSSCREEN_H

#include "arMath.h"
#include "arSZGClient.h"

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
  float getWidth() const { return _width; }
  float getHeight() const { return _height; }

  void setHeadMounted( bool hmd ) { _headMounted = hmd; }
  bool getHeadMounted() const { return _headMounted; }

  void setTile( int tileX, int numberTilesX, int tileY, int numberTilesY );

  void setIgnoreFixedHeadMode( bool ignore ) { _ignoreFixedHeadMode = ignore; }
  bool getIgnoreFixedHeadMode() const { return _ignoreFixedHeadMode; }

  arVector3 getFixedHeadHeadPosition() const { return _fixedHeadPosition; }
  float getFixedHeadHeadUpAngle() const { return _fixedHeadUpAngle; }
  
 private:
  void _updateTileCenter();
  arVector3 _setCenter;
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
  arVector3 _fixedHeadPosition;
  float _fixedHeadUpAngle;
};

#endif // AR_GRAPHICSSCREEN_H

