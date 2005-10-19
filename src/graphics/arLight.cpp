//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arLight.h"
#include "arGraphicsHeader.h"

arLight::arLight(){
  // these are the defaults, as given in OpenGL
  lightID = 0;
  position = arVector4(0,0,1,0);
  diffuse = arVector3(1,1,1);
  ambient = arVector3(0,0,0);
  specular = arVector3(1,1,1);
  constantAttenuation = 1;
  linearAttenuation = 0;
  quadraticAttenuation = 0;
  spotDirection = arVector3(0,0,-1);
  spotCutoff = 180;
  spotExponent = 0;
}

void arLight::activateLight(arMatrix4 lightPositionTransform){
  // convert the light ID to the OpenGL constant
  const GLenum lights[8] = {
    GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
    GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7
  };
  const GLenum lightIdentifier =
    (lightID>=0 && lightID<8) ? lights[lightID] : lights[0];

  // note that there is a little weirdness with the way OpenGL interprets
  // light positions... if the fourth value is nonzero, then it is
  // a positional light. If the fourth value is zero, then it is a
  // directional light and the "position" really gives the light direction!
  // of course, each of these styles transforms differently!
  arVector4 transformedPosition;
  arVector3 dir(position[0],position[1],position[2]);
  const arVector3 transformedSpotDirection =
    lightPositionTransform * (spotDirection - arVector3(0,0,0));
  
  if (position[3] == 0){
    // directional light
    dir = lightPositionTransform*dir - lightPositionTransform*arVector3(0,0,0);
    transformedPosition = arVector4(dir[0],dir[1],dir[2],0);
  }
  else{
    // positional light
    // note that we mangle the OpenGL a little bit here. in the original code
    // it is possible to specify a w-coordinate != 0 or 1. And this would
    // change the final on-screen coordinate via a division. Not so here.
    dir = lightPositionTransform*dir;
    transformedPosition = arVector4(dir[0],dir[1],dir[2],1);
  }
  glLightfv(lightIdentifier, GL_POSITION, transformedPosition.v);
  arVector4 temp(diffuse[0], diffuse[1], diffuse[2], 1);
  glLightfv(lightIdentifier, GL_DIFFUSE, temp.v);
  temp = arVector4(ambient[0], ambient[1], ambient[2], 1);
  glLightfv(lightIdentifier, GL_AMBIENT, temp.v);
  temp = arVector4(specular[0], specular[1], specular[2], 1);
  glLightfv(lightIdentifier, GL_SPECULAR, temp.v);
  glLightf(lightIdentifier, GL_CONSTANT_ATTENUATION, constantAttenuation);
  glLightf(lightIdentifier, GL_LINEAR_ATTENUATION, linearAttenuation);
  glLightf(lightIdentifier, GL_QUADRATIC_ATTENUATION, quadraticAttenuation);
  glLightfv(lightIdentifier, GL_SPOT_DIRECTION, transformedSpotDirection.v);
  glLightf(lightIdentifier, GL_SPOT_CUTOFF, spotCutoff);
  glLightf(lightIdentifier, GL_SPOT_EXPONENT, spotExponent);

  glEnable(lightIdentifier);
}
