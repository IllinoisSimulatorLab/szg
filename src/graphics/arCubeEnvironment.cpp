//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arCubeEnvironment.h"

arCubeEnvironment::arCubeEnvironment() :
  _vertBound(0.5),
  _radius(0.707),
  _cornerX(NULL),
  _cornerZ(NULL),
  _cosWall(NULL),
  _sinWall(NULL),
  _texFileName(NULL) {
  setNumberWalls(4);
}

arCubeEnvironment::~arCubeEnvironment(){
  delete [] _cornerX;
  delete [] _cornerZ;
  delete [] _texFileName;
}

void arCubeEnvironment::setHeight(float height){
  if (height < 0)
    height = -height;
  _vertBound = height/2;
}

void arCubeEnvironment::setRadius(float radius){
  if (radius<0){
    ar_log_error() << "arCubeEnviroment ignoring negative radius " << radius << ".\n";
    radius = 0;
  }
  _radius = radius;
  _computeSideWalls();
}

void arCubeEnvironment::setOrigin(float x, float z, float height)
{
  _origin.set(x, z, height);
  _computeSideWalls();
}


void arCubeEnvironment::setNumberWalls(int howMany){
  if (howMany < 3){
    ar_log_warning() << "arCubeEnviroment increasing number of walls from " <<
      howMany << " to 3.\n";
    howMany = 3;
  }

  if (_cosWall) {
    delete [] _cornerX;
    delete [] _cornerZ;
    delete [] _cosWall;
    delete [] _sinWall;
    delete [] _texFileName;
  }

  _numWalls = howMany;
  // Recompute everything that depends on _numWalls.
  _cornerX = new float[_numWalls];
  _cornerZ = new float[_numWalls];
  _cosWall = new float[_numWalls];
  _sinWall = new float[_numWalls];
  _texFileName = new string[_numWalls+2];
  for (int i=0; i<_numWalls; ++i){
    _cosWall[i] = cos((2*M_PI*i)/_numWalls);
    _sinWall[i] = sin((2*M_PI*i)/_numWalls);
  }
  _computeSideWalls();
}

void arCubeEnvironment::setCorner(int which, float x, float z){
  if (which<0 || which>_numWalls){
    ar_log_error() << "arCubeEnviroment ignoring out-of-range wall " << which << ".\n";
    return;
  }
  _cornerX[which] = x;
  _cornerZ[which] = z;
}

void arCubeEnvironment::setWallTexture(int which, const string& fileName){
  // +2 is for textures of ceiling and floor
  if (which<0 || which>=_numWalls+2){
    ar_log_error() << "arCubeEnviroment ignoring out-of-range wall " << which << ".\n";
    return;
  }
  _texFileName[which] = fileName;
}

void arCubeEnvironment::attachMesh(const string& name, const string& parentName){
  int i;

  // Side walls.
  for (i=0; i<_numWalls; ++i){
    const int j = (i+1 == _numWalls) ? 0 : i+1;
    float pointPositions[12] = {
      _cornerX[i],  _vertBound + _origin[2], _cornerZ[i],
      _cornerX[j],  _vertBound + _origin[2], _cornerZ[j],
      _cornerX[j], -_vertBound + _origin[2], _cornerZ[j],
      _cornerX[i], -_vertBound + _origin[2], _cornerZ[i]
    };

    int triangleVertices[6] = {2,0,1, 2,3,0};
    float texCoords[12] = {1,0, 0,1, 1,1, 1,0, 0,0, 0,1};
    float normals[18] = {0};
    arVector3 normalDir(
      (arVector3(pointPositions+6) - arVector3(pointPositions)) *
      (arVector3(pointPositions+3) - arVector3(pointPositions)));
    if (normalDir.zero()) {
      ar_log_error() << "arCubeEnvironment overriding zero normal\n";
      normalDir = arVector3(1,0,0);
    }
    normalDir.normalize();
    for (int k=0; k<6; ++k){
      normalDir.get(normals + 3*k);
    }
    const char index = '0' + i;
    dgTexture(name+index+" tex", parentName, _texFileName[i]);
    dgPoints(name+index+" points", name+index+" tex",  4, pointPositions);
    dgIndex(name+index+" index", name+index+" points",  6,  triangleVertices);
    dgNormal3(name+index+" normals", name+index+" index",  6,  normals);
    dgTex2(name+index+" tex2", name+index+" normals",  6,  texCoords);
    dgDrawable(name+index+" triangles", name+index+" tex2",  DG_TRIANGLES,  2);
  }

  const arVector3 n(0,-1,0);
  const float r = 0.5;

  // Ceiling.

  float* pointPositions = new float[(_numWalls+1)*3];
  float* normals = new float[9*_numWalls];
  int* triangleVertices = new int[_numWalls*3];
  float* texCoords = new float[6*_numWalls];
  int* ptri = triangleVertices;
  float* ptex = texCoords;

  for (i=0; i<_numWalls; ++i){
    arVector3(_cornerX[i], _vertBound + _origin[2], _cornerZ[i]).get(pointPositions + 3*i);
    n.get(normals + 9*i  );
    n.get(normals + 9*i+3);
    n.get(normals + 9*i+6);
    const int j = (i+1) % _numWalls;

    // Reverse texture's orientation (swap i and j),
    // so alpha-channel texture maps on buildings look nicer.
    *ptri++ = _numWalls;
    *ptri++ = j;
    *ptri++ = i;
    *ptex++ = r;
    *ptex++ = r;
    *ptex++ = r + r*_cosWall[j];
    *ptex++ = r + r*_sinWall[j];
    *ptex++ = r + r*_cosWall[i];
    *ptex++ = r + r*_sinWall[i];
  }

  arVector3(_origin[0], _vertBound + _origin[2], _origin[1]).
    get(pointPositions + _numWalls*3);

  dgTexture(name+" ceil tex",parentName,_texFileName[_numWalls]);
  dgPoints(name+" ceil points",name+" ceil tex", _numWalls+1,pointPositions);
  dgIndex(name+" ceil index",name+" ceil points", 3*_numWalls, triangleVertices);
  dgNormal3(name+" ceil normals",name+" ceil index", 3*_numWalls, normals);
  dgTex2(name+" ceil tex2",name+" ceil normals", 3*_numWalls, texCoords);
  dgDrawable(name+" ceil triangles",name+" ceil tex2", DG_TRIANGLES, _numWalls);

  // Floor.
  ptri = triangleVertices;
  ptex = texCoords;

  for (i=0; i<_numWalls; ++i){
    arVector3(_cornerX[i], -_vertBound + _origin[2], _cornerZ[i]).get(pointPositions + 3*i);
    n.get(normals + 9*i  );
    n.get(normals + 9*i+3);
    n.get(normals + 9*i+6);
    const int j = (i+1) % _numWalls;
    *ptri++ = _numWalls;
    *ptri++ = i;
    *ptri++ = j;
    *ptex++ = r;
    *ptex++ = r;
    *ptex++ = r + r*_cosWall[i];
    *ptex++ = r + r*_sinWall[i];
    *ptex++ = r + r*_cosWall[j];
    *ptex++ = r + r*_sinWall[j];
  }

  arVector3(_origin[0], -_vertBound + _origin[2], _origin[1]).
    get(pointPositions + _numWalls*3);

  dgTexture(name+" floor tex",parentName,_texFileName[_numWalls+1]);
  dgPoints(name+" floor points",name+" floor tex", _numWalls+1,pointPositions);
  dgIndex(name+" floor index",name+" floor points", 3*_numWalls, triangleVertices);
  dgNormal3(name+" floor normals",name+" floor index", 3*_numWalls, normals);
  dgTex2(name+" floor tex2",name+" floor normals", 3*_numWalls, texCoords);
  dgDrawable(name+" floor triangles",name+" floor tex2", DG_TRIANGLES, _numWalls);

  delete [] pointPositions;
  delete [] triangleVertices;
  delete [] normals;
  delete [] texCoords;
}

void arCubeEnvironment::_computeSideWalls(){
  for (int i=0; i<_numWalls; ++i){
    _cornerX[i] = _origin[0] + _radius * _cosWall[i];
    _cornerZ[i] = _origin[1] + _radius * _sinWall[i];
  }
}
