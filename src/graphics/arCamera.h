//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CAMERA_H
#define AR_CAMERA_H

#include "arMath.h"
#include "arSZGClient.h"
#include "arGraphicsScreen.h"
#include "arGraphicsCalling.h"

#include <string>

class SZG_CALL arCamera{
 public:
  arCamera() : _eyeSign(0), _screen(0) {}
  virtual ~arCamera() {}
  void setEyeSign( float eyeSign ) { _eyeSign = eyeSign; }
  float getEyeSign() const { return _eyeSign; }
  void setScreen( arGraphicsScreen* screen ) { _screen = screen; }
  arGraphicsScreen* getScreen() const { return _screen; }
  virtual arMatrix4 getProjectionMatrix();
  virtual arMatrix4 getModelviewMatrix();
  virtual void loadViewMatrices();
  virtual arCamera* clone() const { return new arCamera(); }
  virtual std::string type( void ) const { return "arCamera"; }
 private:
  // NOTE: these parameters should not be copied, they are transient and only exist
  // so they don't have to be passed to the various get.. and load... matrix functions
  float _eyeSign;
  arGraphicsScreen* _screen;
};

#endif
