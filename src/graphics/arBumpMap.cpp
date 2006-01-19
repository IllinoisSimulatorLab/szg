//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
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
  if (_numTBN) {	// do we even have data?
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

/// @todo Verify this copy constructor; I don't remember exactly how they work
//  with derived classes...
/*arBumpMap::arBumpMap& operator=( const arBumpMap& rhs ) {
  Texture t = rhs;
  return arBumpMap(t);
}*/

void arBumpMap::activate() {
#ifdef USE_CG
  //printf("arBumpMap::activate() starting...\n");
  if (_texName == 0)
    _loadIntoOpenGL();
  else {
    if (_fDirtyTex)
      _loadIntoOpenGL();
    if (_fDirtyCg)
      _cgInit();
    if (_fDirtyTBN)
      _computeFrame();

    // Bind the programs
    cgGLBindProgram(_cg_vertexProgram);
    cgGLBindProgram(_cg_fragmentProgram);
    // Enable the profiles
    cgGLEnableProfile(_cg_vertexProfile);
    cgGLEnableProfile(_cg_fragmentProfile);

    // Set the uniform parameters that don't change every vertex
    cgGLSetStateMatrixParameter(_cg_modelViewProj,
		    		CG_GL_MODELVIEW_PROJECTION_MATRIX,
				CG_GL_MATRIX_IDENTITY);
    if (!_isTexParamSet) {
      cgGLSetTextureParameter(_cg_decalMap, _decalName);
      cgGLSetTextureParameter(_cg_normalMap, _texName);
      _isTexParamSet = true;
      //printf("cgGLSetTextureParameter called: %i, %i\n", _texName, _decalName);
    }

    cgGLEnableTextureParameter(_cg_normalMap);
    cgGLEnableTextureParameter(_cg_decalMap);

  }
#endif
}

void arBumpMap::reactivate(){
  //printf("arBumpMap::reactivate() starting...\n");
}

void arBumpMap::_loadIntoOpenGL() {
#ifdef USE_CG
  //printf("arBumpMap::_loadIntoOpenGL() starting...\n");
  
  //glActiveTexture(GL_TEXTURE1);

  glGenTextures(1,&_texName);
  glBindTexture(GL_TEXTURE_2D, _texName);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                  _repeating ? GL_REPEAT : GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                  _repeating ? GL_REPEAT : GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  // _width and _height must both be a power of two.
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
            _alpha ? GL_MODULATE : GL_DECAL);
  if (_mipmap)
    gluBuild2DMipmaps(GL_TEXTURE_2D, _alpha ? GL_RGBA : GL_RGB,
		      _width, _height, _alpha ? GL_RGBA : GL_RGB,
		      GL_UNSIGNED_BYTE, (GLubyte*)_pixels);
  else
    glTexImage2D(GL_TEXTURE_2D, 0, _alpha ? GL_RGBA : GL_RGB,
		 _width, _height, 0, _alpha ? GL_RGBA : GL_RGB,
		 GL_UNSIGNED_BYTE, (GLubyte*)_pixels);

  // Has the decal texture been init'ed?
  // Is there a valid decalTexture? -- if not, we're gonna see black
  if (_decalName == 0 && _decalTexture) {
    cout << _decalTexture->glName() << endl;
    _decalTexture->activate();		// load texture into Graphics memory
    _decalTexture->deactivate();
    cout << _decalTexture->glName() << endl;
    _decalName = _decalTexture->glName();	// GL Name to pass to Cg
  }
#endif
  _fDirtyTex = false;
}

void arBumpMap::_cgInit() {
#ifdef USE_CG
  //printf("arBumpMap::_cgInit() starting...\n");
  _initMainCg();

  _cg_position      = cgGetNamedParameter(_cg_vertexProgram, "IN.Position");
  _cg_tangent       = cgGetNamedParameter(_cg_vertexProgram, "IN.Tangent");
  _cg_binormal      = cgGetNamedParameter(_cg_vertexProgram, "IN.Binormal");
  _cg_normal        = cgGetNamedParameter(_cg_vertexProgram, "IN.Normal");
  _cg_modelViewProj = cgGetNamedParameter(_cg_vertexProgram, "modelViewProj");
  if (!_cg_position || !_cg_tangent || !_cg_binormal || !_cg_normal || !_cg_modelViewProj)
    cerr << "Cg Error: Error getting vertex program values!\n";

  //_cg_lookupTable = cgGetNamedParameter(_cg_fragmentProgram, "lookupTable");
  _cg_decalMap   = cgGetNamedParameter(_cg_fragmentProgram, "decalMap");
  _cg_normalMap   = cgGetNamedParameter(_cg_fragmentProgram, "normalMap");
  if (!_cg_decalMap || !_cg_normalMap )
    cerr << "Cg Error: Error getting fragment program values!\n";

  _cgTBN[0] = _cg_tangent;
  _cgTBN[1] = _cg_binormal;
  _cgTBN[2] = _cg_normal;
#endif
  _fDirtyCg = false;
}

void arBumpMap::deactivate() {
#ifdef USE_CG
 //printf("arBumpMap::deactivate() starting...\n");
 cgGLDisableTextureParameter(_cg_decalMap);
 cgGLDisableTextureParameter(_cg_normalMap);
 // Disable the profiles
 cgGLDisableProfile(_cg_vertexProfile);
 cgGLDisableProfile(_cg_fragmentProfile);
#endif
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

CGparameter* arBumpMap::cgTBN() {
#ifdef USE_CG
  if (!_cg_tangent || !_cg_binormal || !_cg_normal)
    return NULL;
  if (_fDirtyCg)
    _cgInit();
  return _cgTBN;
#else
  return NULL;
#endif
}

void arBumpMap::generateFrames(int numTBN) {
  _numTBN = numTBN;
  _computeFrame();
}

/// Generates local frame for each vertex of object
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
  /// run through all the faces, finding gradients of u and v along surface
  /// This code modified from NVIDIA's NVMeshMender.cpp
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

//===============================================================//
//================ MAIN Cg init (only once) =====================//

#ifdef USE_CG
CGcontext myContext;
// used to print out any Cg errors that may occur
void cgErrorCallback(void) {
  CGerror LastError = cgGetError();
  if(LastError) {
    const char *Listing = cgGetLastListing(myContext);
    printf("\n---------------------------------------------------\n");
    printf("%s\n\n", cgGetErrorString(LastError));
    printf("%s\n", Listing);
    printf("---------------------------------------------------\n");
    printf("Cg error, exiting...\n");
    exit(0);
  }
}
#endif

void arBumpMap::_initMainCg() {
#ifdef USE_CG
  if (_isMainCgInited)
    return;
  //printf("arBumpMap::_initMainCg() starting...\n");

  // Create context
  _cg_context = cgCreateContext();
  // Error checking
  myContext = _cg_context;
  cgSetErrorCallback(cgErrorCallback);
  // Initialize profiles and compiler options
  _cg_vertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
  cgGLSetOptimalOptions(_cg_vertexProfile);
  _cg_fragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
  cgGLSetOptimalOptions(_cg_fragmentProfile);
  //printf("Cg Vert Profile: %s\n", cgGetProfileString(_cg_vertexProfile));
  //printf("Cg Frag Profile: %s\n", cgGetProfileString(_cg_fragmentProfile));
  // Create and load the vertex & fragment programs
  _cg_vertexProgram = cgCreateProgramFromFile(_cg_context, CG_SOURCE,
		  "/home/public/Data/cg_bump_mapping_vertex.cg",
		  _cg_vertexProfile, NULL, NULL);
  _cg_fragmentProgram = cgCreateProgramFromFile(_cg_context, CG_SOURCE,
		  "/home/public/Data/cg_bump_mapping_fragment.cg",
		  _cg_fragmentProfile, NULL, NULL);
  cgGLLoadProgram(_cg_vertexProgram);
  cgGLLoadProgram(_cg_fragmentProgram);
  _isMainCgInited = true;   // Only do this function once
  //printf("arBumpMap::_initMainCg() finished.\n");
#endif
}


