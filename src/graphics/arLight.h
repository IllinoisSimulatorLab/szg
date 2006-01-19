//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LIGHT_H
#define AR_LIGHT_H

#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// This very simple class encapsulates the parameters for lights in OpenGL.
/// It also includes code for activating the light. The class is used
/// by arGraphicsDatabase in implementing the OpenGL lighting model

class SZG_CALL arLight{
 public:
  arLight();
  arLight(const arLight&);
  ~arLight(){};

  int lightID;      
  arVector4 position;
  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  float     constantAttenuation;
  float     linearAttenuation;
  float     quadraticAttenuation;
  arVector3 spotDirection;
  float     spotCutoff;
  float     spotExponent;

  void activateLight(){ activateLight(ar_identityMatrix()); }
  void activateLight(arMatrix4 lightPositionTransform);
};

#endif
