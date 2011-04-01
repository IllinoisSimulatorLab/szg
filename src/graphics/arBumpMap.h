//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BUMPMAP_H
#define AR_BUMPMAP_H

#include "arTexture.h"
using namespace std;

#include "arGraphicsCalling.h"

// Bump map loaded from a file, or loaded from a block of memory.

class SZG_CALL arBumpMap : public arTexture {
 public:
  arBumpMap();
  virtual ~arBumpMap();
  arBumpMap( const arBumpMap& rhs );
#if 0
  arBumpMap& operator=( const arBumpMap& rhs );
#endif
  bool operator!();
  void activate();        // initializes bump map in OpenGL (called once at start)
  void reactivate();        // enables in OpenGL -- NOTE: Unused; deprecated???
  void deactivate();        // disables in OpenGL

  void generateFrames(int numTBN);        // force-generates TBNs

  // sets how deep the bump map should be (default is 1)
  void setHeight(float newHeight);
  // these should be sorted by (possibly indexed) vertex number
  const float* tangents() { return _tangents; }
  void setTangents(int number, float *tangents);
  const float* binormals() { return _binormals; }
  void setBinormals(int number, float *binorms);
  const float* normals() { return _normals; }
  void setNormals(int number, float *norms);
  void setTBN(int number, float *tangents, float *binorms, float *norms);
  void setPIT(int numPts, int numInd, float* points, int* index, float* tex2);
  void setDecalTexture(arTexture* newTexture) { _decalTexture = newTexture; }

  // returns how many packed double3's are in each (T, B, N) array
  const int & numberOfTBN(void) { return _numTBN; }

  // TBN float* array or NULL if invalid
  float** TBN();

 private:
  // private functions
  bool                _loadIntoOpenGL();
  void                _cgInit();
  void                _computeFrame();
  void          _initMainCg();

  // data
  int                _numTBN;        // size of TBN arrays
  float                _bumpHeight;        // how deep the bump map is (default: 1)
  float*        _tangents;        // bump map tangents (along u/s)
  float*        _binormals;        // bump map binormals (along v/t)
  float*        _normals;        // bump map normals (TxB; roughly OpenGL normal)
  float*        _TBN[3];        // pointers to the TBN data (for TBN() )
  unsigned int        _decalName;        // GL texture ID to pass to Cg
  arTexture*        _decalTexture;        // underlying color for bump map

  // outside data pointers -- NOTE: these could become invalid at any time...
  int                _numPts;        // size of _points array (& tex2 if no indices)
  int                _numInd;        // size of _indices array (& tex2 if indices)
  float*        _points;
  int*                _indices;
  float*        _tex2;

  // flags
  bool                _fDirtyTex;        // Does texture need to be re-init'ed?
  bool                _fDirtyCg;        // Does Cg need to be re-init'ed?
  bool                _fDirtyTBN;        // Does TBN (bases) need to be re-init'ed?
  bool          _isMainCgInited;
  bool          _isTexParamSet;
};

#endif
