//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MATERIAL_H
#define AR_MATERIAL_H

#include "arMath.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// A simple class describing an OpenGL material...
/// Later, this should expand to manage everything about the surface
/// coloration of an object... for instance, managing multi-texturing,
/// pixel shaders, etc.
/// NOTE: right now, this does NOTHING in the actual code. So, a 
/// placeholder for now.

class SZG_CALL arMaterial{
 public:
  arMaterial();
  arMaterial(const arMaterial&);
  ~arMaterial(){}

  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  arVector3 emissive;
  float exponent;     // i.e. shininess
  float alpha;        // transparency of the material
 
  void activateMaterial();
};

#endif
