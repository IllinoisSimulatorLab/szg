//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef __AR_HTR_H
#define __AR_HTR_H

// Written by Mark Flider.

#include <stdio.h>
#include <iostream>
#include "arMath.h"
#include "arGraphicsDatabase.h"
#include <string>
#include <vector>
#include "arOBJSmoothingGroup.h"
#include "arObject.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arObjCalling.h"

// Rotation orders (there are many possibilities for Euler angles).
//enum { XYZ = 1, XZY, YXZ, YZX, ZXY, ZYX };

/// Hierarchy pairs
class SZG_CALL htrSegmentHierarchy {
 public:
  char* child;
  char* parent;
  ~htrSegmentHierarchy() {
    if (child)
      free(child);
    if (parent)
      free(parent);
  }
};

class htrBasePosition;
class htrFrame;

/// Segment data
class SZG_CALL htrSegmentData{
 public:
  htrSegmentData(): transformNode(NULL), scaleNode(NULL), preTransformNode(NULL), postTransformNode(NULL),
    localTransformNode(NULL), invTransformNode(NULL), boundingSphereNode(NULL), parent(NULL){}
  ~htrSegmentData(){}

  string segmentName;
  /// Transform node associated with this segment
  arTransformNode* transformNode;
  /// The scale node associated with this segment
  arTransformNode* scaleNode;
  /// To adjust for different actors animating the same
  /// HTR, we need to be able to adjust the position,
  /// size, etc. of each segment from a well-defined
  /// location. This is a pre-multiplication... i.e.
  /// further down the transform chain.
  arTransformNode* preTransformNode;
  /// Another adjustment, a post-multiplication
  /// i.e. futher up the transform chain
  arTransformNode* postTransformNode;
  /// another adjustment, to move the segment without moving its children
  arTransformNode* localTransformNode;
  /// transform pushing the geometry bound to the segment back to
  /// a standard position (i.e. starting at the origin
  /// and pointed along the y-axis)
  arTransformNode* invTransformNode;
  /// Each segment has a bounding sphere associated
  /// with it. We need to be able to pick and manipulate the segments
  arBoundingSphereNode* boundingSphereNode;
  htrSegmentData* parent;
  htrBasePosition* basePosition;
  vector<htrSegmentData*> children;
  vector<htrFrame*> frame;
};

/// Base position Structure
class SZG_CALL htrBasePosition {
 public:
  htrBasePosition() : name(NULL), segment(NULL)
    {}
  ~htrBasePosition()
    { if (name) free(name); }
  char* name;
  htrSegmentData* segment;
  double Tx, Ty, Tz;  
  arMatrix4 trans;
  double Rx, Ry, Rz;
  arMatrix4 rot;
  double boneLength;
};

/// Frame data
class SZG_CALL htrFrame{
 public:
  htrFrame(){}
  ~htrFrame(){}
  int frameNum;
  arMatrix4 trans;  
  double Tx, Ty, Tz; 
  double Rx, Ry, Rz;
  double scale;    
  double totalScale;
};

/// Wrapper for .htr format
class SZG_CALL arHTR : public arObject {
  public:
    arHTR();
    ~arHTR();
    // There cannot be one readHTR because the default parameter order
    // for 2 arguments would be wrong 
    bool readHTR(const string& fileName, const string& path="");
    bool readHTR(const string& fileName, const string& subdirectory, const string& path);
    bool readHTR(FILE* htrFileHandle);
    bool writeToFile(const string& fileName);
    // DEPRECATED! Use the arGraphicsNode* parent version instead!
    bool attachMesh(const string& objectName, const string& parent);
    bool attachMesh(arGraphicsNode* parent, const string& objectName=""){
      return attachMesh(parent, objectName, false);
    }
    // DEPRECATED! Use the arGraphicsNode* parent version instead!
    bool attachMesh(const string& baseName, const string& where, bool withLines);
    bool attachMesh(arGraphicsNode* parent, const string& objectName, bool withLines);
    string type(void) { return "HTR"; }

    void normalizeModelSize(void);
    void basicDataSmoothing();

    // Animation functions.
    bool supportsAnimation(void) { return true; }
    bool setFrame(int newFrame);
    bool nextFrame();
    bool prevFrame();
    bool setBasePosition();

    // Stats.
    inline int numberOfFrames(){ return numFrames; }
    inline int currentFrame(){ return _currentFrame; }
    inline int numberOfSegments(){ return numSegments; }
    inline int version(){ return fileVersion; }
    inline string nameOfSegment(int i){ return segmentData[i]->segmentName; }
    inline arTransformNode* transformForSegment(int i){ return segmentData[i]->transformNode; }
    inline arTransformNode* preTransformForSegment(int i){ return segmentData[i]->preTransformNode; }
    inline arTransformNode* postTransformForSegment(int i){ return segmentData[i]->postTransformNode; }
    inline arTransformNode* localTransformForSegment(int i){ return segmentData[i]->localTransformNode; }
    inline arTransformNode* inverseForSegment(int i){ return segmentData[i]->invTransformNode; }
    inline arBoundingSphereNode* boundingSphereForSegment(int i){ return segmentData[i]->boundingSphereNode; }
    arMatrix4 segmentBaseTransformRelative(int segmentID);
    int	numberOfSegment(const string& segmentName);
    arMatrix4 inverseTransformForSegment(int i);

  protected:
    // Reading in data functions
    bool parseHeader(FILE* htrFileHandle);
    bool parseHierarchy(FILE* htrFileHandle);
    bool parseBasePosition(FILE* htrFileHandle);
    bool parseSegmentData1(FILE* htrFileHandle);
    bool parseSegmentData2(FILE* htrFileHandle);
    bool parseSegmentData(FILE* htrFileHandle);
    bool precomputeData(void);
    bool setInvalid();
    void subNormalizeModelSize(arVector3 thePoint, arVector3 &minVec,
		    	       arVector3 &maxVec, htrBasePosition *theBP);
    bool frameValid(htrFrame* f);
    void frameInterpolate(htrFrame* f, htrFrame* inetrp1, htrFrame* interp2);
    arMatrix4 HTRTransform(htrBasePosition* theBP, htrFrame* theFrame);
    arMatrix4 HTRRotation(double Rx, double Ry, double Rz);
    
    void attachChildNode(arGraphicsNode* parent,
                         const string& baseName, 
                         htrBasePosition* node, 
                         bool withLines=false);

  private:
    int fileVersion;
    int numSegments;
    int numFrames;
    int dataFrameRate;
    char* fileType;
    char* dataType;
    arAxisOrder eulerRotationOrder;    
    char* calibrationUnits;   
    char* rotationUnits;
    char globalAxisOfGravity;
    char boneLengthAxis;
    double scaleFactor;
    vector<htrSegmentHierarchy*> childParent;  
    vector<htrBasePosition*> basePosition;
    vector<htrSegmentData*> segmentData;

    int _currentFrame;
    arVector3 _normCenter;	//< middle of the model
    double _normScaleAmount;	//< how much to scale model by to fit in unit sphere
};

#endif // __AR_HTR_H
