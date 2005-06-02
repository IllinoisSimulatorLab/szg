//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_HEAD_H
#define AR_HEAD_H

#include "arMath.h"
#include "arSZGClient.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <iostream>

class arGraphicsScreen;

class SZG_CALL arHead {
  friend class arViewerNode;
  friend class arMasterSlaveFramework;
  friend SZG_CALL bool dgViewer( int ID, const arHead& head );
 public:
  arHead();
  virtual ~arHead() {}

  virtual bool configure( arSZGClient& client );

  void setEyeSpacing( float spacing ) {_eyeSpacing = spacing; }
  float getEyeSpacing() const { return _eyeSpacing; }

  void setMidEyeOffset( const arVector3& midEyeOffset ) 
    { _midEyeOffset = midEyeOffset; }
  arVector3 getMidEyeOffset() const { return _midEyeOffset; }

  void setEyeDirection( const arVector3& eyeDirection ) 
    { _eyeDirection = eyeDirection; }
  arVector3 getEyeDirection() const { return _eyeDirection; }

  void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
  virtual arMatrix4 getMatrix() const { return _matrix; }

  void setClipPlanes( float nearClip, float farClip ) 
    { _nearClip = nearClip; _farClip = farClip; }
  float getNearClip() const { return _nearClip; }
  float getFarClip() const { return _farClip; }

  void setUnitConversion( float conv ) { _unitConversion = conv; }
  float getUnitConversion() const { return _unitConversion; }

  arVector3 getEyePosition( float eyeSign, arMatrix4* useMatrix=0 );
  arVector3 getMidEyePosition( arMatrix4* useMatrix=0 );
  arMatrix4 getMidEyeMatrix() const;

  void setFixedHeadMode( bool onoff ) {_fixedHeadMode = (int)onoff;}
  bool getFixedHeadMode() const { return (bool)_fixedHeadMode; }

 private:
  arMatrix4 _matrix;
  arVector3 _midEyeOffset;
  arVector3 _eyeDirection;
  float _eyeSpacing;
  float _nearClip;
  float _farClip;
  float _unitConversion;
  int _fixedHeadMode;
};

#endif // AR_HEAD_H
