//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MATERIAL_H
#define AR_MATERIAL_H

#include "arMath.h"
#include "arGraphicsCalling.h"

// OpenGL material.
// Should eventually manage full surface coloration: multi-texturing, pixel shaders, etc.
// Placeholder for now.

class SZG_CALL arMaterial {
 public:
  arMaterial();
  arMaterial(const arMaterial&);
  ~arMaterial() {}

  arVector3 diffuse;
  arVector3 ambient;
  arVector3 specular;
  arVector3 emissive;
  float exponent;     // i.e. shininess
  float alpha;        // transparency of the material

  void activateMaterial();
};

#endif
