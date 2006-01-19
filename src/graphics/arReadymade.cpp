//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arReadymade.h"
#include "arMesh.h"

arBarbell::arBarbell(const arMatrix4& transform,
    const string& textureWood,
    const string& textureMetal) :
  _transform(transform),
  _woodTextureFile(textureWood),
  _metalTextureFile(textureMetal)
{}

void arBarbell::attachMesh(const string& name, const string& parentName){

  // spheres
  dgTexture(name+" texture1", parentName, _metalTextureFile);
  arSphereMesh theSphere;
  theSphere.setAttributes(15);
  theSphere.setTransform(_transform*ar_translationMatrix(0,0, 0.5) * ar_scaleMatrix(0.3));
  theSphere.setTransform(_transform*ar_translationMatrix(0,0,-0.5) * ar_scaleMatrix(0.3));
  theSphere.attachMesh(name+" weight1", name+" texture1");
  theSphere.attachMesh(name+" weight2", name+" texture1");

  // pole
  dgTexture(name+" texture2", parentName, _woodTextureFile);
  arCylinderMesh theCylinder(_transform);
  theCylinder.setAttributes(20,0.06,0.06);
  theCylinder.attachMesh(name+" pole",name+" texture2");
}

arBookShelf::arBookShelf(const arMatrix4& transform,
    const string& textureWood,
    const string& textureBook) :
  _transform(transform),
  _woodTextureFile(textureWood),
  _bookTextureFile(textureBook)
{}

void arBookShelf::attachMesh(const string& name, const string& parentName){

  // walls
  dgTexture(name+" texture1", parentName, _woodTextureFile);
  arCubeMesh theCube;
  theCube.setTransform(
    _transform*ar_translationMatrix(0.5,0,0)*ar_scaleMatrix(0.1,1.3,1));
  theCube.attachMesh(name+" wall1",name+" texture1");

  theCube.setTransform(
    _transform*ar_translationMatrix(-0.5,0,0)*ar_scaleMatrix(0.1,1.3,1));
  theCube.attachMesh(name+" wall2",name+" texture1");

  theCube.setTransform(
    _transform*ar_translationMatrix(0,0.65,0)*ar_scaleMatrix(1,0.1,1));
  theCube.attachMesh(name+" wall3",name+" texture1");

  theCube.setTransform(
    _transform*ar_translationMatrix(0,-0.65,0)*ar_scaleMatrix(1,0.1,1));
  theCube.attachMesh(name+" wall4",name+" texture1");

  theCube.setTransform(
    _transform*ar_translationMatrix(0,0,-0.5)*ar_scaleMatrix(1,1.3,0.1));
  theCube.attachMesh(name+" wall5",name+" texture1");

  // books
  dgTexture(name+" texture2", parentName, _bookTextureFile);
  theCube.setTransform(
    _transform*ar_translationMatrix(0,0,0.4)*ar_scaleMatrix(1,1.3,0.1));
  theCube.attachMesh(name+" wall6",name+" texture2");
}

arCeilingLamp::arCeilingLamp(const arMatrix4& transform,
    const string& textureWood,
    const string& textureMetal,
    const string& textureShade) :
  _transform(transform),
  _woodTextureFile(textureWood),
  _metalTextureFile(textureMetal),
  _shadeTextureFile(textureShade)
{}

void arCeilingLamp::attachMesh(const string& name, const string& parentName){
  
  // base
  dgTexture(name+" texture1", parentName, _woodTextureFile);
  arCubeMesh theBase(
    _transform*ar_translationMatrix(0,0,-0.5)* ar_scaleMatrix(0.5,0.5,0.1));
  theBase.attachMesh(name+" base", name+" texture1");
  
  // pole
  dgTexture(name+" texture2", parentName, _metalTextureFile);
  arCylinderMesh thePole(_transform);
  thePole.setAttributes(20, 0.06, 0.06);
  thePole.attachMesh(name+" pole", name+" texture2");

  // shade
  arCylinderMesh theShade(
    _transform * ar_translationMatrix(0,0,0.5) *
    ar_rotationMatrix('x',3.14) * ar_scaleMatrix(1,1,0.5));
  dgTexture(name+" texture3", parentName, _shadeTextureFile);
  theShade.setAttributes(20, 0.4, 0.1);
  theShade.attachMesh(name+" shade", name+" texture3");
}

arCouch::arCouch(const arMatrix4& transform,
    const string& textureArm,
    const string& textureCushion) :
  _transform(transform),
  _armTextureFile(textureArm),
  _cushionTextureFile(textureCushion)
{}

void arCouch::attachMesh(const string& name, const string& parentName){

  // arms
  dgTexture(name+" texture1", parentName, _armTextureFile);
  arCubeMesh theCube(
    _transform*ar_translationMatrix(0,-0.15,-0.5)*ar_scaleMatrix(1,0.7,0.2));
  theCube.attachMesh(name+" arm1",name+" texture1");
  theCube.setTransform(
    _transform*ar_translationMatrix(0,-0.15,0.5)*ar_scaleMatrix(1,0.7,0.2));
  theCube.attachMesh(name+" arm2",name+" texture1");

  // cushions
  dgTexture(name+" texture2", parentName, _cushionTextureFile);
  theCube.setTransform(
    _transform*ar_translationMatrix(-0.5,0,0)*ar_scaleMatrix(0.2,1,1));
  theCube.attachMesh(name+" back",name+" texture2");
  theCube.setTransform(
    _transform*ar_translationMatrix(0,-0.31,0)*ar_scaleMatrix(1.1,0.4,1));
  theCube.attachMesh(name+" seat",name+" texture2");
}

arFloorLamp::arFloorLamp(const arMatrix4& transform,
    const string& textureWood,
    const string& textureMetal,
    const string& textureShade) :
  _transform(transform),
  _woodTextureFile(textureWood),
  _metalTextureFile(textureMetal),
  _shadeTextureFile(textureShade)
{}

void arFloorLamp::attachMesh(const string& name, const string& parentName){

  // base
  dgTexture(name+" texture1", parentName, _woodTextureFile);
  arCubeMesh theBase(
    _transform*ar_translationMatrix(0,0,-0.5)* ar_scaleMatrix(0.5,0.5,0.1));
  theBase.attachMesh(name+" base", name+" texture1");

  // pole
  dgTexture(name+" texture2", parentName, _metalTextureFile);
  arCylinderMesh thePole(_transform);
  thePole.setAttributes(20, 0.06, 0.06);
  thePole.attachMesh(name+" pole", name+" texture2");

  // shade
  arCylinderMesh theShade(
    _transform*ar_translationMatrix(0,0,0.5)*ar_scaleMatrix(1,1,0.5));
  dgTexture(name+" texture3", parentName, _shadeTextureFile);
  theShade.setAttributes(20, 0.4, 0.1);
  theShade.attachMesh(name+" shade", name+" texture3");
}

arLollypop::arLollypop(const arMatrix4& transform,
    const string& textureWood,
    const string& textureCandy) :
  _transform(transform),
  _woodTextureFile(textureWood),
  _candyTextureFile(textureCandy)
{}

void arLollypop::attachMesh(const string& name, const string& parentName){
  
  // stick
  dgTexture(name+" texture1", parentName, _woodTextureFile);
  arCylinderMesh theStick(_transform);
  theStick.toggleEnds(true);
  theStick.setAttributes(20,0.05,0.05);
  theStick.attachMesh(name+" stick", name+" texture1");

  // candy
  dgTexture(name+" texture2", parentName, _candyTextureFile);
  arCylinderMesh theSugar(
    _transform*ar_translationMatrix(0,0,0.5)*
    ar_rotationMatrix('x',1.57)*ar_scaleMatrix(1,1,0.15));
  theSugar.setAttributes(20,0.4,0.4);
  theSugar.attachMesh(name+" candy", name+" texture2");
}

arTexturedSquare::arTexturedSquare(const arMatrix4& transform):
  _transform(transform){
}

void arTexturedSquare::attachMesh(const string& name, 
                                  const string& parentName){
  float pointCoords[12] = {-0.5,-0.5,0, 0.5,-0.5,0, 0.5,0.5,0, -0.5,0.5,0};
  int triangle[6] = {0,1,2,0,2,3};
  float normals[18] = {0,0,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1};
  float textureCoords[12] = {0,0, 1,0, 1,1, 0,0, 1,1, 0,1};

  for (int i=0; i<4; i++){
    arVector3 temp(pointCoords[i*3], pointCoords[i*3+1], pointCoords[i*3+2]);
    temp = _transform*temp;
    pointCoords[i*3] = temp[0];
    pointCoords[i*3+1] = temp[1];
    pointCoords[i*3+2] = temp[2];
  }

  dgPoints(name+".points", parentName, 4, pointCoords);
  dgIndex(name+".index", name+".points", 6, triangle);
  dgNormal3(name+".normal", name+".index",6, normals);
  dgTex2(name+".tex2",name+".normal",6, textureCoords);
  dgDrawable(name+".triangles",name+".tex2",DG_TRIANGLES,2);
}

arToyCar::arToyCar(const arMatrix4& transform,
    const string& textureWheel,
    const string& textureBody,
    const string& textureTop) :
  _transform(transform),
  _wheelTextureFile(textureWheel),
  _bodyTextureFile(textureBody),
  _topTextureFile(textureTop)
{}

void arToyCar::attachMesh(const string& name, const string& parentName){

  // wheels
  dgTexture(name+" texture1", parentName, _wheelTextureFile);
  arCylinderMesh theCylinder(
    _transform*ar_translationMatrix(0,0,.5)*ar_rotationMatrix('y',1.57));
  theCylinder.setAttributes(20,.3,.3);
  theCylinder.toggleEnds(true);
  theCylinder.attachMesh(name+" wheel1", name+" texture1");
  theCylinder.setTransform(
    _transform*ar_translationMatrix(0,0,-.5)*ar_rotationMatrix('y',1.57));
  theCylinder.attachMesh(name+" wheel2", name+" texture1");

  // body
  dgTexture(name+" texture2", parentName, _bodyTextureFile);
  arCubeMesh theBody(
    _transform*ar_translationMatrix(0,.2,0)*ar_scaleMatrix(.8,.5,1.8));
  theBody.attachMesh(name+" body", name+" texture2");
  
  // top
  dgTexture(name+" texture3", parentName, _topTextureFile);
  arCubeMesh theTop(
    _transform*ar_translationMatrix(0,.5,-.2)*
          ar_scaleMatrix(.6,.5,.8));
  theTop.attachMesh(name+" top", name+" texture3");
}

arToyTruck::arToyTruck(const arMatrix4& transform,
    const string& textureWheel,
    const string& textureBody,
    const string& textureTop) :
  _transform(transform),
  _wheelTextureFile(textureWheel),
  _bodyTextureFile(textureBody),
  _topTextureFile(textureTop)
{}

void arToyTruck::attachMesh(const string& name, const string& parentName){

  // wheels
  dgTexture(name+" texture1", parentName, _wheelTextureFile);
  arTorusMesh theTorus(20,10,0.27,0.18);
  string nameWheel(name+" texture1");

  dgTransform(name+" trans1", nameWheel,
    _transform*ar_translationMatrix(.5,-.1,.5)*ar_rotationMatrix('y',1.57));
  theTorus.attachMesh(name+" wheel1",name+" trans1");

  dgTransform(name+" trans2", nameWheel,
    _transform*ar_translationMatrix(.5,-.1,-.5)*ar_rotationMatrix('y',1.57));
  theTorus.attachMesh(name+" wheel2",name+" trans2");

  dgTransform(name+" trans3", nameWheel,
    _transform*ar_translationMatrix(-.5,-.1,.5)*ar_rotationMatrix('y',1.57));
  theTorus.attachMesh(name+" wheel3",name+" trans3");

  dgTransform(name+" trans4", nameWheel,
    _transform*ar_translationMatrix(-.5,-.1,-.5)*ar_rotationMatrix('y',1.57));
  theTorus.attachMesh(name+" wheel4",name+" trans4");

  // body
  dgTexture(name+" texture2", parentName, _bodyTextureFile);
  arCubeMesh theBody(
    _transform*ar_translationMatrix(0,.2,0)*ar_scaleMatrix(.8,.5,1.8));
  theBody.attachMesh(name+" body",name+" texture2");
  
  // top
  dgTexture(name+" texture3", parentName, _topTextureFile);
  arCubeMesh theTop(
    _transform*ar_translationMatrix(0,.5,-.2)*ar_scaleMatrix(.6,.5,.8));
  theTop.attachMesh(name+" top",name+" texture3");
}

void arSimpleChairMesh::setTransform(const arMatrix4& transform){
  _transform = transform;
}

void arSimpleChairMesh::setWoodTexture(const string& wood){
  _woodTextureFile = wood;
}

void arSimpleChairMesh::setMetalTexture(const string& metal){
  _metalTextureFile = metal;
}

void arSimpleChairMesh::attachMesh(const string& name, const string& parent){
  arCubeMesh cubeMesh;

  dgTexture(name+" wood",parent,_woodTextureFile);
  dgTexture(name+" metal",parent,_metalTextureFile);

  // chair legs
  arMatrix4 legTransform;
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0.8,-1,-0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg1",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(-0.8,-1,-0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg2",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(-0.8,-1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg3",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0.8,-1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg4",name+" metal");

  // back supports
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0.8,1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" back1",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0,1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" back2",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(-0.8,1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" back3",name+" metal");

  // seat and back
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_scaleMatrix(2,0.1,2);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" seat",name+" wood");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0,1.5,0.8)
    *ar_scaleMatrix(2,1,0.15);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" back",name+" wood");
}

void arSimpleTableMesh::setTransform(const arMatrix4& transform){
  _transform = transform;
}

void arSimpleTableMesh::setWoodTexture(const string& wood){
  _woodTextureFile = wood;
}

void arSimpleTableMesh::setMetalTexture(const string& metal){
  _metalTextureFile = metal;
}

void arSimpleTableMesh::attachMesh(const string& name, const string& parent){
  arCubeMesh cubeMesh;
  
  dgTexture(name+" wood",parent,_woodTextureFile);
  dgTexture(name+" metal",parent,_metalTextureFile);
  // do the chair legs
  arMatrix4 legTransform;
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0.8,-1,-0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg1",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(-0.8,-1,-0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg2",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(-0.8,-1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg3",name+" metal");
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_translationMatrix(0.8,-1,0.8)
    *ar_scaleMatrix(0.1,2,0.1);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" leg4",name+" metal");

  // do the seat and the back
  legTransform = _transform * ar_scaleMatrix(0.25)
    *ar_scaleMatrix(2,0.1,2);
  cubeMesh.setTransform(legTransform);
  cubeMesh.attachMesh(name+" seat",name+" wood");
}
