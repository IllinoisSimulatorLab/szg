//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_HEAD_H
#define AR_HEAD_H

#include "arMath.h"
#include "arSZGClient.h"
#include "arLogStream.h"
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

  arVector3 getEyePosition( float eyeSign, const arMatrix4* useMatrix=0 ) const;
  arVector3 getMidEyePosition( const arMatrix4* useMatrix=0 ) const;
  arMatrix4 getMidEyeMatrix() const;

  void setFixedHeadMode( bool onoff ) { _fixedHeadMode = onoff; }
  bool getFixedHeadMode() const { return _fixedHeadMode; }

 private:
  arMatrix4 _matrix;
  arVector3 _midEyeOffset;
  arVector3 _eyeDirection;
  float _eyeSpacing;
  float _nearClip;
  float _farClip;
  float _unitConversion;
  bool _fixedHeadMode;
};

SZG_CALL ostream& operator<<(ostream& s, arHead& h);
SZG_CALL arLogStream& operator<<(arLogStream& s, arHead& h);

#endif // AR_HEAD_H
