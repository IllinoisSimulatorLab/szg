//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SCREEN_OBJECT_H
#define AR_SCREEN_OBJECT_H

#include "arMath.h"
#include "arSZGClient.h"
#include "arCamera.h"

// This object is a bit of a catch-all and will need to be changed...
// It includes everything necessary to set up a fixed-screen projection,
// except for head position and "which eye". So... it includes information
// about the wall (i.e. the screen onto which we are projecting), plus
// information about the physical layout of the head, plus our "demo mode"
// provision. It might be better to seperate these things out into their
// logical components.

class SZG_CALL arScreenObject: public arCamera{
 public:
  arScreenObject();
  virtual ~arScreenObject() {}
  // These five functions are inherited from arCamera
  virtual bool configure(arSZGClient*);
  virtual bool configure(const string& screenName, arSZGClient* client);
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();

  // This method computes some internally used info based on our
  // now deprecated interface.
  void setEyeInfo(float eyeSign, const arMatrix4& headMatrix);
  // These next 3 methods are deprecated and included for temporary
  // backwards compatibility only. These are virtual (and still around)
  // since descendants of arScreenObject use them.
  virtual arMatrix4 getProjectionMatrix(const float eyeSign,
				        const arMatrix4& headMatrix);
  virtual arMatrix4 getModelviewMatrix(const float eyeSign,
			               const arMatrix4& headMatrix);
  virtual void loadViewMatrices( const float eyeSign, 
                                 const arMatrix4& headMatrix );
  void setDemoMode(bool demoMode)
    { _useFixedHeadPos = demoMode; }
  // The functions that follow are a little confusing. Quite a bit
  // of stuff has been jammed into arScreenObject... the wall, the
  // head description, etc. This will changed.
  void setViewTransform(float nearClip, float farClip, float unitConversion,
                        float eyeSpacing, const arVector3& midEyeOffset,
			const arVector3& eyeDirection);
  void setCenter(const arVector3& center);
  void setNormal(const arVector3& normal);
  void setUp(const arVector3& up);
  void setDimensions(float width, float height);
  void setTile(int tileX, int numberTilesX, int tileY, int numberTilesY);
  void setClipPlanes(float nearClip, float farClip);
  void setUnitConversion(float unitConversion);
  void setEyeSpacing(float feet);
  void setMidEyeOffset(const arVector3& offset){ _midEyeOffset = offset; }
  arVector3 getMidEyeOffset() const { return _midEyeOffset; }
  void setEyeDirection(const arVector3& dir){ _eyeDirection = dir; }
  float eyeSpacing() { return _eyeSpacing; }
  arVector3 getNormal() const { return _normal; }
  arVector3 getUp() const { return _up; }
  arVector3 getCenter() const { return _center; }
  float getWidth() const { return _width; }
  float getHeight() const { return _height; }
  int   getTileX() const { return _tileX; }
  int   getTileY() const { return _tileY; }
  int   getNumberTilesX() const { return _numberTilesX; }
  int   getNumberTilesY() const { return _numberTilesY; }

  // derived classes should be able to use the below
 protected:
  arVector3 _center;
  arVector3 _normal;
  arVector3 _up;
  float     _height;
  float     _width;
  int       _tileX;
  int       _tileY;
  int       _numberTilesX;
  int       _numberTilesY;

  float     _nearClip;
  float     _farClip;

  // vector from head tracker to midpoint of eyes, in feet
  arVector3 _midEyeOffset; 
  // unit vector from midpoint in direction of right eye
  arVector3 _eyeDirection; 
  // distance between eyes, in feet
  float _eyeSpacing; 

  float _unitConversion;

  float     _demoHeadUpAngle;
  bool      _useFixedHeadPos;
  arVector3 _fixedHeadPos;

  bool _fixToHead;

  // internal storage for quantities needed to compute the viewing matrices
  float     _eyeSign;
  arMatrix4 _headMatrix;
  arVector3 _eyePosition;
  float     _localNearClip;
  arMatrix4 _locHeadMatrix;

  // which screen are we? SZG_SCREEN0, SZG_SCREEN1, SZG_SCREEN2, etc.
  string _screenName;

  arMatrix4 _demoHeadMatrix( const arMatrix4& );
  void _getVector3(arSZGClient* szgClient, arVector3& v, const char* param);
};

#endif
