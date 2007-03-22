//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arCubeEnvironment.h"

arCubeEnvironment::arCubeEnvironment(){
  _numberWalls = 4;
  
  _vertBound = 0.5;
  _radius = 0.707;

  _cornerX = new float[_numberWalls];
  _cornerZ = new float[_numberWalls];
  _texFileName = new string[_numberWalls+2];

  _calculateRegularWalls();
}

arCubeEnvironment::~arCubeEnvironment(){
  delete [] _cornerX;
  delete [] _cornerZ;
  delete [] _texFileName;
}

void arCubeEnvironment::setHeight(float height){
  if (height<0)
    height = -height;
  _vertBound = height/2;
}

void arCubeEnvironment::setRadius(float radius){
  if (radius<0){
    cerr << "arCubeEnviroment warning: ignoring negative radius "
         << radius << ".\n";
    radius = 0;
  }
  _radius = radius;
  _calculateRegularWalls();
}

void arCubeEnvironment::setOrigin(float x, float z, float height)
{
  _origin.set(x, z, height);
  _calculateRegularWalls();
}


void arCubeEnvironment::setNumberWalls(int howMany){
  if (howMany < 3){
    cerr << "arCubeEnviroment warning: increasing number of walls from "
         << howMany << " to 3.\n";
    howMany = 3;
  }

  delete [] _cornerX;
  delete [] _cornerZ;
  delete [] _texFileName;
  
  _numberWalls = howMany;
  _cornerX = new float[_numberWalls];
  _cornerZ = new float[_numberWalls];
  _texFileName = new string[_numberWalls+2];

  _calculateRegularWalls();
}

void arCubeEnvironment::setCorner(int which, float x, float z){
  if (which<0 || which>_numberWalls){
    cerr << "arCubeEnviroment warning: ignoring out-of range wall number "
         << which << ".\n";
    return;
  }
  _cornerX[which] = x;
  _cornerZ[which] = z;
}

void arCubeEnvironment::setWallTexture(int which, const string& fileName){
  // +2 textures for ceiling and floor
  if (which<0 || which>=_numberWalls+2){
    cerr << "arCubeEnviroment warning: ignoring out-of range wall number "
         << which << ".\n";
    return;
  }
  _texFileName[which] = fileName;
}

void arCubeEnvironment::attachMesh(const string& name,
				   const string& parentName){
  int i=0;
  for (i=0; i<_numberWalls; i++){
    const int j = (i+1 == _numberWalls) ? 0 : i+1;
    float pointPositions[12] = {
      _cornerX[i],
      _vertBound + _origin[2],
      _cornerZ[i],
      _cornerX[j],
      _vertBound + _origin[2],
      _cornerZ[j],
      _cornerX[j],
      -_vertBound + _origin[2],
      _cornerZ[j],
      _cornerX[i],
      -_vertBound + _origin[2],
      _cornerZ[i]
    };

    int triangleVertices[6] = {2,0,1, 2,3,0};
    float texCoords[12] = {1,0, 0,1, 1,1, 1,0, 0,0, 0,1};
    float normals[18] = {0};
    arVector3 normalDir =
      (arVector3(pointPositions+6) - arVector3(pointPositions)) *
      (arVector3(pointPositions+3) - arVector3(pointPositions));
    normalDir.normalize();
    for (int k=0; k<6; k++){
      normals[3*k] = normalDir[0];
      normals[3*k+1] = normalDir[1];
      normals[3*k+2] = normalDir[2];
    }
    const char index = '0'+i;
    dgTexture(name+index+" texture",parentName,_texFileName[i]);
    dgPoints(name+index+" points",name+index+" texture",
             4,pointPositions);
    dgIndex(name+index+" index",name+index+" points",
            6, triangleVertices);
    dgNormal3(name+index+" normals",name+index+" index",
              6, normals);
    dgTex2(name+index+" tex2",name+index+" normals",
           6, texCoords);
    dgDrawable(name+index+" triangles",name+index+" tex2", DG_TRIANGLES, 2);
  }
  // do the ceiling
  // in reverse orientation, to work better with
  // alpha-channel texture maps on buildings inside this CubeEnvironment.
  float* pointPositions = new float[(_numberWalls+1)*3];
  float* normals = new float[9*_numberWalls];
  int* triangleVertices = new int[_numberWalls*3];
  float* texCoords = new float[6*_numberWalls];

  for (i=0; i<_numberWalls; i++){
    pointPositions[3*i] = _cornerX[i];
    pointPositions[3*i+1] = _vertBound + _origin[2];
    pointPositions[3*i+2] = _cornerZ[i];
    normals[9*i] = 0;
    normals[9*i+1] = -1;
    normals[9*i+2] = 0;
    normals[9*i+3] = 0;
    normals[9*i+4] = -1;
    normals[9*i+5] = 0;
    normals[9*i+6] = 0;
    normals[9*i+7] = -1;
    normals[9*i+8] = 0;
    const int j = (i+1 == _numberWalls) ? 0 : i+1;

    // swap i and j
    // to draw the ceiling's polygons in reversed orientation
    // so alpha-channel texture maps on buildings look nicer.
    triangleVertices[3*i] = _numberWalls;
    triangleVertices[3*i+1] = j;
    triangleVertices[3*i+2] = i;
    texCoords[6*i] = 0.5;
    texCoords[6*i+1] = 0.5;
    texCoords[6*i+2] = 0.5+0.5*cos((6.283*j)/_numberWalls);
    texCoords[6*i+3] = 0.5+0.5*sin((6.283*j)/_numberWalls);
    texCoords[6*i+4] = 0.5+0.5*cos((6.283*i)/_numberWalls);
    texCoords[6*i+5] = 0.5+0.5*sin((6.283*i)/_numberWalls);
  }

  pointPositions[_numberWalls*3] = _origin[0];
  pointPositions[_numberWalls*3+1] = _vertBound + _origin[2];
  pointPositions[_numberWalls*3+2] = _origin[1];

  dgTexture(name+" ceiling texture",parentName,_texFileName[_numberWalls]);
  dgPoints(name+" ceiling points",name+" ceiling texture",
           _numberWalls+1,pointPositions);
  dgIndex(name+" ceiling index",name+" ceiling points",
          3*_numberWalls, triangleVertices);
  dgNormal3(name+" ceiling normals",name+" ceiling index",
            3*_numberWalls, normals);
  dgTex2(name+" ceiling tex2",name+" ceiling normals",
         3*_numberWalls, texCoords);
  dgDrawable(name+" ceiling triangles",name+" ceiling tex2", DG_TRIANGLES,
	     _numberWalls);

  delete [] pointPositions;
  delete [] normals;
  delete [] triangleVertices;
  delete [] texCoords;

  // do the floor
  pointPositions = new float[(_numberWalls+1)*3];
  triangleVertices = new int[_numberWalls*3];
  normals = new float[9*_numberWalls];
  texCoords = new float[6*_numberWalls];

  for (i=0; i<_numberWalls; i++){
    pointPositions[3*i] = _cornerX[i];
    pointPositions[3*i+1] = -_vertBound + _origin[2];
    pointPositions[3*i+2] = _cornerZ[i];
    normals[9*i] = 0;
    normals[9*i+1] = -1;
    normals[9*i+2] = 0;
    normals[9*i+3] = 0;
    normals[9*i+4] = -1;
    normals[9*i+5] = 0;
    normals[9*i+6] = 0;
    normals[9*i+7] = -1;
    normals[9*i+8] = 0;
    const int j = (i+1 == _numberWalls) ? 0 : i+1;
    triangleVertices[3*i] = _numberWalls;
    triangleVertices[3*i+1] = i;
    triangleVertices[3*i+2] = j;
    texCoords[6*i] = 0.5;
    texCoords[6*i+1] = 0.5;
    texCoords[6*i+2] = 0.5+0.5*cos((6.283*i)/_numberWalls);
    texCoords[6*i+3] = 0.5+0.5*sin((6.283*i)/_numberWalls);
    texCoords[6*i+4] = 0.5+0.5*cos((6.283*j)/_numberWalls);
    texCoords[6*i+5] = 0.5+0.5*sin((6.283*j)/_numberWalls);
  }

  pointPositions[_numberWalls*3] = _origin[0];
  pointPositions[_numberWalls*3+1] = -_vertBound + _origin[2];
  pointPositions[_numberWalls*3+2] = _origin[1];

  dgTexture(name+" floor texture",parentName,_texFileName[_numberWalls+1]);
  dgPoints(name+" floor points",name+" floor texture",
           _numberWalls+1,pointPositions);
  dgIndex(name+" floor index",name+" floor points",
          3*_numberWalls, triangleVertices);
  dgNormal3(name+" floor normals",name+" floor index",
            3*_numberWalls, normals);
  dgTex2(name+" floor tex2",name+" floor normals",
         3*_numberWalls, texCoords);
  dgDrawable(name+" floor triangles",name+" floor tex2", DG_TRIANGLES,
	     _numberWalls);

  delete [] pointPositions;
  delete [] triangleVertices;
  delete [] normals;
  delete [] texCoords;
}

void arCubeEnvironment::_calculateRegularWalls(){
  for (int i=0; i<_numberWalls; i++){
    _cornerX[i] = _origin[0] + _radius*cos( (2*M_PI*i)/_numberWalls );
    _cornerZ[i] = _origin[1] + _radius*sin( (2*M_PI*i)/_numberWalls );
  }
}

