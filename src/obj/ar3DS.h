//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef __AR_3DS_H
#define __AR_3DS_H

// part of the Graphics Language Library, some simple tools for communicating
// 3D graphics between programs
// Written under the GNU General Public License
// -- mflider

#include <stdio.h>
#include <iostream>
#include "arMath.h"
#include "arGraphicsDatabase.h"
#include <string>
#include <vector>
#include "arObject.h"
#ifdef Enable3DS
// NOTE: these *must* be qualified by lib3ds, otherwise compilation
// search paths will replace the *system* float.h by this one!
  #include <lib3ds/file.h>
  #include <lib3ds/vector.h>
  #include <lib3ds/matrix.h>
  #include <lib3ds/camera.h>
  #include <lib3ds/light.h>
  #include <lib3ds/material.h>
  #include <lib3ds/mesh.h>
  #include <lib3ds/node.h>
#endif

/// .3DS wrapper class
/** Needs lib3DS */
class ar3DS : public arObject{

  public:
    ar3DS();
    ~ar3DS();
    bool read3DS(char* fileName);
    //bool writeToFile(char* fileName);
    void normalizeModelSize();
    bool normalizationMatrix(arMatrix4 &theMatrix);

    void attachMesh(const string& baseName, const string& where);
    void attachMesh(const string& baseName, arGraphicsNode* parent);

    // animation
    bool supportsAnimation(void) { return false; }
    bool setFrame(int newFrame);
    bool nextFrame();
    bool prevFrame();

    // stats
    /// returns "3DS" as arObject type
    inline string  type() { return "3DS"; }
    /// returns number of frames in .3DS file
#ifdef Enable3DS
    int    numberOfFrames() { return _file?_file->frames:-1; }
#else
    int    numberOfFrames() { return -1; }
#endif

  protected:
#ifdef Enable3DS
    void attachChildNode(const string& baseName, 
                         arGraphicsNode* parent,
                         Lib3dsNode* node);
#endif
    arVector3 _minVec;	//< vector of minimum x, y, and z values in mesh
    arVector3 _maxVec;	//< vector of maximum x, y, and z values in mesh
    arVector3 _center;	//< center of bounding box for mesh
    bool    _normalize;	//< tells attachMesh() whether or not to add a normalization matrix

  private:
#ifdef Enable3DS
    Lib3dsFile	*_file;	//< The Lib3ds-read File
#else
    void	*_file;
#endif
    int _uniqueName;
    int _currentFrame;	//< Currently displayed frame
    int _numMaterials;	//< Number of materials in the file
#ifdef Enable3DS
    /// helper function for normalizationMatrix; allows easy recursing
    void subNormalizationMatrix(Lib3dsNode* node, arVector3& _minVec, arVector3& _maxVec);
#endif
};


#endif // __AR_3DS_H
