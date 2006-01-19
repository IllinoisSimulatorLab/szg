//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_READYMADE_H
#define AR_READYMADE_H

#include "arGraphicsDatabase.h"
#include "arMath.h"
#include <string>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

/// 2 balls joined by a cylinder.

class SZG_CALL arBarbell{
 public:
  arBarbell(const arMatrix4& transform,
    const string& textureWood,
    const string& textureMetal);
  ~arBarbell() {}

  void attachMesh(const string&, const string&);
  
 private:
  arMatrix4 _transform;
  string _woodTextureFile;
  string _metalTextureFile; 
};

/// Shelf with books.

class SZG_CALL arBookShelf{
 public:
  arBookShelf(const arMatrix4& transform,
    const string& textureWood,
    const string& textureBook);
  ~arBookShelf() {}

  void attachMesh(const string&, const string&);
  
 private:
  arMatrix4 _transform;
  string _woodTextureFile;
  string _bookTextureFile;  
};

/// Ceiling-hung lamp with base and lampshade.

class SZG_CALL arCeilingLamp{
 public:
  arCeilingLamp(const arMatrix4& transform,
    const string& textureWood,
    const string& textureMetal,
    const string& textureShade);
  ~arCeilingLamp() {}

  void attachMesh(const string&, const string&);
  
 private:
  arMatrix4 _transform;
  string _woodTextureFile;
  string _metalTextureFile; 
  string _shadeTextureFile;
};

/// Tell me vatt you are theenking...

class SZG_CALL arCouch{
 public:
  arCouch(const arMatrix4& transform,
    const string& textureArm,
    const string& textureCushion);
  ~arCouch() {}

  void attachMesh(const string&, const string&);
  
 private:
  arMatrix4 _transform;
  string _armTextureFile;
  string _cushionTextureFile;
};

/// Floor-standing lamp with base and lampshade.

class SZG_CALL arFloorLamp{
 public:
  arFloorLamp(const arMatrix4& transform,
    const string& textureWood,
    const string& textureMetal,
    const string& textureShade);
  ~arFloorLamp() {}

  void attachMesh(const string&, const string&);

 private:
  arMatrix4 _transform;
  string _woodTextureFile;
  string _metalTextureFile;
  string _shadeTextureFile;
};

/// Disc on a stick.

class SZG_CALL arLollypop{
 public:
  arLollypop(const arMatrix4& transform,
    const string& textureWood,
    const string& textureCandy);
  ~arLollypop() {}

  void attachMesh(const string&, const string&);
  
 private:
  arMatrix4 _transform;
  string _woodTextureFile;
  string _candyTextureFile; 
};

/// Planar equiangular quadrilateral polytope.

class SZG_CALL arTexturedSquare{
 public:
  arTexturedSquare(const arMatrix4&);
  ~arTexturedSquare() {};
  void attachMesh(const string&, const string&);
 private:
  arMatrix4 _transform;
};

/// Quadri-wheeled vehicle.

class SZG_CALL arToyCar{
 public:
  arToyCar(const arMatrix4& transform,
    const string& textureWheel,
    const string& textureBody,
    const string& textureTop);
  ~arToyCar() {}

  void attachMesh(const string&, const string&);

 private:
  arMatrix4 _transform;
  string _wheelTextureFile;
  string _bodyTextureFile;
  string _topTextureFile;
};

/// Another quadri-wheeled vehicle.

class SZG_CALL arToyTruck{
 public:
  arToyTruck(const arMatrix4& transform,
    const string& textureWheel,
    const string& textureBody,
    const string& textureTop);
  ~arToyTruck() {}

  void attachMesh(const string&, const string&);

 private:
  arMatrix4 _transform;
  string _wheelTextureFile;
  string _bodyTextureFile;
  string _topTextureFile;
};

#endif
#ifndef AR_SIMPLE_CHAIR_MESH_H
#define AR_SIMPLE_CHAIR_MESH_H

/// No!  Not the comfy chair!

class SZG_CALL arSimpleChairMesh{
 public:
  arSimpleChairMesh(){}
  ~arSimpleChairMesh(){}

  void setTransform(const arMatrix4&);
  void setWoodTexture(const string&);
  void setMetalTexture(const string&);

  void attachMesh(const string&,const string&);
 private:
  arMatrix4 _transform;
  string _woodTextureFile;
  string _metalTextureFile;
};

/// Put this in front of the arSimpleChairMesh.

class SZG_CALL arSimpleTableMesh{
 public:
  arSimpleTableMesh(){}
  ~arSimpleTableMesh(){}

  void setTransform(const arMatrix4&);
  void setWoodTexture(const string&);
  void setMetalTexture(const string&);

  void attachMesh(const string&,const string&);
 private:
  string _woodTextureFile;
  string _metalTextureFile;

  arMatrix4 _transform;
};

#endif
