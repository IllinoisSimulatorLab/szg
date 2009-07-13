//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef __AR_3DS_H
#define __AR_3DS_H

// part of the Graphics Language Library, some simple tools for communicating
// 3D graphics between programs
// -- mflider

#ifdef Enable3DS
#include <lib3ds/file.h>
#include <lib3ds/vector.h>
#include <lib3ds/matrix.h>
#include <lib3ds/camera.h>
#include <lib3ds/light.h>
#include <lib3ds/material.h>
#include <lib3ds/mesh.h>
#include <lib3ds/node.h>
#endif

#include "arMath.h"
#include "arGraphicsDatabase.h"
#include "arObject.h"

// lib3ds/ explicitly here, not in "cc -I", so ALL .cpp files don't bogusly get lib3ds/float.h.

#include "arObjCalling.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

// .3DS wrapper class
/** Needs lib3DS */
class SZG_CALL ar3DS : public arObject{

  public:
    ar3DS();
    ~ar3DS();
    bool read3DS(const string& fileName);
    void normalizeModelSize();
    bool normalizationMatrix(arMatrix4 &theMatrix);

    bool attachMesh(const string& baseName, const string& where);
    bool attachMesh(arGraphicsNode* parent, const string& baseName="");

    // animation
    bool supportsAnimation(void) const { return false; }
    bool setFrame(int newFrame);
    bool nextFrame();
    bool prevFrame();

    // stats
    string type() const { return "3DS"; } // arObject type
#ifdef Enable3DS
    int    numberOfFrames() const { return _file?_file->frames:-1; }
#else
    int    numberOfFrames() const { return -1; }
#endif

  protected:
#ifdef Enable3DS
    void attachChildNode(const string& baseName, arGraphicsNode* parent, Lib3dsNode* node);
#endif
    arVector3 _minVec;        // vector of minimum x, y, and z values in mesh
    arVector3 _maxVec;        // vector of maximum x, y, and z values in mesh
    arVector3 _center;        // center of bounding box for mesh
    bool    _normalize;        // tells attachMesh() whether or not to add a normalization matrix

  private:
#ifdef Enable3DS
    // helper function for normalizationMatrix; allows easy recursing
    void subNormalizationMatrix(Lib3dsNode* node, arVector3& _minVec, arVector3& _maxVec);
    Lib3dsFile        *_file;
#else
    void        *_file;
#endif
    int _uniqueName;
    int _currentFrame;        // displayed frame
    int _numMaterials;        // Number of materials in _file
};

#endif // __AR_3DS_H
