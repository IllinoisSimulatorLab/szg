//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBumpMap.h"
#include "arMath.h"

GLuint make_lookup_texture(double shininess);

arBumpMap::arBumpMap() :
  arTexture(),
  _numTBN(0),
  _bumpHeight(0),
  _tangents(NULL),
  _binormals(NULL),
  _normals(NULL),
  _decalName(0),
  _decalTexture(NULL),
  _numPts(0),
  _numInd(0),
  _points(NULL),
  _indices(NULL),
  _tex2(NULL),
  _fDirtyTex(true),
  _fDirtyCg(true),
  _fDirtyTBN(true),
  _isMainCgInited(false),
  _isTexParamSet(false)
{
  //_initMainCg();
  //_normLookupTexture = make_lookup_texture(128.);
}

arBumpMap::~arBumpMap() {
  if (_tangents)
    delete [] _tangents;
  if (_binormals)
    delete [] _binormals;
  if (_normals)
    delete [] _normals;
}

arBumpMap::arBumpMap( const arBumpMap& rhs ) : arTexture(rhs) {
  _numTBN = rhs._numTBN;
  _bumpHeight = rhs._bumpHeight;
  if (_numTBN) {        // do we even have data?
    if (rhs._tangents) {
      _tangents = new float[_numTBN];
      memcpy(_tangents, rhs._tangents, sizeof(float)*3*_numTBN);
    } else
      _tangents = NULL;
    if (rhs._binormals) {
      _binormals = new float[_numTBN];
      memcpy(_binormals, rhs._binormals, sizeof(float)*3*_numTBN);
    } else
      _binormals = NULL;
    if (rhs._normals) {
      _normals = new float[_numTBN];
      memcpy(_normals, rhs._normals, sizeof(float)*3*_numTBN);
    } else
      _normals = NULL;
  } else
    _tangents = _binormals = _normals = NULL;
}

#if 0
arBumpMap::arBumpMap& operator=( const arBumpMap& rhs ) {
  // ;;;;;;;; THIS IS BUGGY.  Do a deep copy, like the copy constructor.
  arBumpMap t = arBumpMap(rhs);
  _tangents = t.tangents; //;;;; etc.
}
#endif

void arBumpMap::activate() {
}

void arBumpMap::reactivate() {
}

bool arBumpMap::_loadIntoOpenGL() {
  _fDirtyTex = false;
  return true;
}

void arBumpMap::_cgInit() {
  _fDirtyCg = false;
}

void arBumpMap::deactivate() {
}

void arBumpMap::setHeight(float newHeight) {
  _bumpHeight = newHeight;
}

void arBumpMap::setTangents(int number, float *tangents) {
  _numTBN = number;
  if (_tangents)
    delete [] _tangents;
  _tangents = tangents;
  _fDirtyTBN = true;
}

void arBumpMap::setBinormals(int number, float *binorms) {
  _numTBN = number;
  if (_binormals)
    delete [] _binormals;
  _binormals = binorms;
  _fDirtyTBN = true;
}

void arBumpMap::setNormals(int number, float *norms) {
  _numTBN = number;
  if (_normals)
    delete [] _normals;
  _normals = norms;
  _fDirtyTBN = true;
}

void arBumpMap::setTBN(int number, float *tangents, float *binorms, float *norms) {
  //printf("arBumpMap::setTBN() started\n");
  this->setTangents(number, tangents);
  this->setBinormals(number, binorms);
  this->setNormals(number, norms);
  _computeFrame();
}

void arBumpMap::setPIT(int numPts, int numInd, float* points,
                       int* index, float* tex2) {
  //printf("arBumpMap::setPIT() started\n");
  _numPts = numPts;
  _numInd = numInd;
  _points = points;
  _indices = index;
  _tex2 = tex2;
  if (!_numTBN)
    _numTBN = _indices ? _numInd : _numPts;
  _fDirtyTBN = true;
}

float** arBumpMap::TBN() {
  if (!_tangents || !_binormals || !_normals)
    return NULL;
  if (_fDirtyTBN)
    _computeFrame();
  return _TBN;
}

void arBumpMap::generateFrames(int numTBN) {
  _numTBN = numTBN;
  _computeFrame();
}

// Generates local frame for each vertex of object
/** Given a set of vertices, connectivity information, and texture coords,
 *  we can construct binormal and tangent vectors by taking a tangent
 *  along the "u" direction and binormal around the "v" direction of
 *  a surface.
 *  NOTE: needs u,v coords, so must be inserted under texcoord node
 */
void arBumpMap::_computeFrame() {
  if (!_tex2 || !_points)
    return;
  //printf("arBumpMap::_computeFrame(%i) starting...\n", _numTBN);

  arVector3 edge1, edge2, s, t, sxt;
  float ds_dx = 0., dt_dx = 0., ds_dy = 0., dt_dy = 0., ds_dz = 0., dt_dz = 0.;
  int i = 0;
  int numFaces = _indices ? _numInd/3 : _numPts/9;
  // storage for new values
  arVector3 *dsList = new arVector3[numFaces];
  arVector3 *dtList = new arVector3[numFaces];
  // Traverse the faces, finding gradients of u and v along surface
  // This code modified from NVIDIA's NVMeshMender.cpp
  for (i=0; i<numFaces; ++i) { // for every face
    // Create an edge out of x, s and t
    if (_indices) {
      edge1.v[0] = _points[3* _indices[3*i +1]] - _points[3* _indices[3*i]];
      edge2.v[0] = _points[3* _indices[3*i +2]] - _points[3* _indices[3*i]];
    }
    else {
      edge1.v[0] = _points[3* (3*i +1) +0] - _points[3* (3*i) +0];
      edge2.v[0] = _points[3* (3*i +2) +0] - _points[3* (3*i) +0];
    }
    edge1.v[1] = _tex2[2* (3*i +1) +0] - _tex2[2* (3*i) +0];
    edge1.v[2] = _tex2[2* (3*i +1) +1] - _tex2[2* (3*i) +1];
    edge2.v[1] = _tex2[2* (3*i +2) +0] - _tex2[2* (3*i) +0];
    edge2.v[2] = _tex2[2* (3*i +2) +1] - _tex2[2* (3*i) +1];
    sxt = edge1 * edge2;        // cross product
    ds_dx = dt_dx = 0.;
    if ( sxt.v[0] ) {                   // if (a != 0)
      ds_dx = -sxt.v[1]/sxt.v[0];       // -b / a;
      dt_dx = -sxt.v[2]/sxt.v[0];       // -c / a;
    }

    // Create an edge out of y, s and t
    if (_indices) {
      edge1.v[0] = _points[3* (int)_indices[3*i +1] +1] - _points[3* (int)_indices[3*i] +1];
      edge2.v[0] = _points[3* (int)_indices[3*i +2] +1] - _points[3* (int)_indices[3*i] +1];
    }
    else {
      edge1.v[0] = _points[3* (3*i +1) +1] - _points[3* (3*i) +1];
      edge2.v[0] = _points[3* (3*i +2) +1] - _points[3* (3*i) +1];
    }
    sxt = edge1 * edge2;        // cross product
    ds_dy = dt_dy = 0.;
    if ( sxt.v[0] ) {                   // if (a != 0)
      ds_dy = -sxt.v[1]/sxt.v[0];       // -b / a;
      dt_dy = -sxt.v[2]/sxt.v[0];       // -c / a;
    }
    // Create an edge out of z, s and t
    if (_indices) {
      edge1.v[0] = _points[3* (int)_indices[3*i +1] +2] - _points[3* (int)_indices[3*i] +2];
      edge2.v[0] = _points[3* (int)_indices[3*i +2] +2] - _points[3* (int)_indices[3*i] +2];
    }
    else {
      edge1.v[0] = _points[3* (3*i +1) +2] - _points[3* (3*i) +2];
      edge2.v[0] = _points[3* (3*i +2) +2] - _points[3* (3*i) +2];
    }

    sxt = edge1 * edge2;        // cross product
    ds_dz = dt_dz = 0.;
    if ( sxt.v[0] ) {                   // if (a != 0)
      ds_dz = -sxt.v[1]/sxt.v[0];       // -b / a;
      dt_dz = -sxt.v[2]/sxt.v[0];       // -c / a;
    }

    // generate coordinate frame from the gradients
    (s = arVector3( ds_dx, ds_dy, ds_dz ));
    (t = arVector3( dt_dx, dt_dy, dt_dz ));

    s /= ++s;
    t /= ++t;

    // save vectors for this face
    dsList[i] = s;       // dsList is consecutive
    dtList[i] = t;       // dtList is consecutive
  }

  if (_tangents)
    delete [] _tangents;
  if (_binormals)
    delete [] _binormals;
  if (_normals)
    delete [] _normals;
  _tangents = new float[numFaces*9];
  _binormals = new float[numFaces*9];
  _normals = new float[numFaces*9];
  for (i=0; i<numFaces*9; ++i)
    _tangents[i] = _binormals[i] = _normals[i] = 0.;

  arVector3 tempT, tempB, tempN, tempN2;
  // second pass; take ds's and dt's and convert to tan and binorm
  for (i=0; i<numFaces; i++) {
    tempT = dsList[i];          tempT /= ++tempT;
    tempB = dtList[i];          tempB /= ++tempB;
    tempN = tempT * tempB;      tempN /= ++tempN;

    _tangents[9*i+0] = _tangents[9*i+3] = _tangents[9*i+6] = tempT.v[0];
    _tangents[9*i+1] = _tangents[9*i+4] = _tangents[9*i+7] = tempT.v[1];
    _tangents[9*i+2] = _tangents[9*i+5] = _tangents[9*i+8] = tempT.v[2];
    _binormals[9*i+0] = _binormals[9*i+3] = _binormals[9*i+6] = tempB.v[0];
    _binormals[9*i+1] = _binormals[9*i+4] = _binormals[9*i+7] = tempB.v[1];
    _binormals[9*i+2] = _binormals[9*i+5] = _binormals[9*i+8] = tempB.v[2];
    _normals[9*i+0] = _normals[9*i+3] = _normals[9*i+6] = tempN.v[0];
    _normals[9*i+1] = _normals[9*i+4] = _normals[9*i+7] = tempN.v[1];
    _normals[9*i+2] = _normals[9*i+5] = _normals[9*i+8] = tempN.v[2];
  }
  delete [] dsList;
  delete [] dtList;
  _TBN[0] = _tangents;
  _TBN[1] = _binormals;
  _TBN[2] = _normals;
  _fDirtyTBN = false;
}

void arBumpMap::_initMainCg() {
}
