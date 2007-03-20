//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arMaterial.h"
#include "arGraphicsHeader.h"

arMaterial::arMaterial(){
  // Default color is white, other defaults are OpenGL's.
  diffuse = arVector3(1,1,1);
  ambient = arVector3(0.2,0.2,0.2);
  specular = arVector3(0,0,0);
  emissive = arVector3(0,0,0);
  exponent = 0;
  alpha = 1;
}

arMaterial::arMaterial(const arMaterial& rhs){
  diffuse = rhs.diffuse;
  ambient = rhs.ambient;
  specular = rhs.specular;
  emissive = rhs.emissive;
  exponent = rhs.exponent;
  alpha = rhs.alpha;
}

void arMaterial::activateMaterial(){
  arVector4 temp(diffuse[0], diffuse[1], diffuse[2], alpha);
  if (glIsEnabled( GL_LIGHTING )) {
    // Fishy. This doesn't support the *full* OpenGL.
    // This always does GL_FRONT_AND_BACK instead of GL_FRONT or GL_BACK.
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, temp.v);
    temp = arVector4(ambient[0], ambient[1], ambient[2], alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, temp.v);
    temp = arVector4(specular[0], specular[1], specular[2], alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, temp.v);
    temp = arVector4(emissive[0], emissive[1], emissive[2], alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, temp.v);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, exponent);
  } else {
    glColor4fv( temp.v );
  }
}
