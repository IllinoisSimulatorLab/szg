//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef __AR_HTR_H
#define __AR_HTR_H

// written by Mark Flider

#include <stdio.h>
#include <iostream>
#include "arMath.h"
#include "arGraphicsDatabase.h"
#include <string>
#include <vector>
#include "arOBJSmoothingGroup.h"
#include "arObject.h"

// rotation orders
enum { XYZ = 1, XZY, YXZ, YZX, ZXY, ZYX };

/// Hierarchy pairs
class htrSegmentHierarchy {
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

/// Segment data
class htrSegmentData{
 public:
  htrSegmentData(): parent(NULL)
    {}
  ~htrSegmentData()
    { if (name) delete [] name; }
  char *name;
  /// ID of the transform node associated with this segment
  int transformID;
  /// to adjust for different actors animating the same
  /// HTR, we need to be able to adjust the position,
  /// size, etc. of each segment from a well-defined
  /// location. This is a pre-multiplication... i.e.
  /// further down the transform chain
  int preTransformID;
  /// another adjustment, a post-multiplication
  /// i.e. futher up the transform chain
  int postTransformID;
  /// another adjustment, to move the segment without moving its children
  int localTransformID;
  /// transform pushing the geometry bound to the segment back to
  /// a standard position (i.e. starting at the origin
  /// and pointed along the y-axis)
  int invTransformID;
  /// each segment has a bounding sphere associated
  /// with it. We need to be able to pick and manipulate the segments
  int boundingSphereID;
  struct htrSegmentData* parent;
  struct htrBasePosition* basePosition;
  vector<struct htrSegmentData*> children;
  vector<struct htrFrame*> frame;
};

/// Base position Structure
class htrBasePosition {
 public:
  htrBasePosition() : name(NULL), segment(NULL)
    {}
  ~htrBasePosition()
    { if (name) free(name); }
  char* name;
  struct htrSegmentData* segment;
  double Tx, Ty, Tz;    ///< initial values
  arMatrix4 trans;
  double Rx, Ry, Rz;
  arMatrix4 rot;
  double boneLength;
};

/// Frame data
struct htrFrame{
  int frameNum;
  arMatrix4 trans;   ///< local transformation
  double Tx, Ty, Tz; ///< transformations
  double Rx, Ry, Rz;
  double scale;      ///< segment scaling factor
};

/// Wrapper for .htr format
class arHTR : public arObject {
  public:
    arHTR();
    ~arHTR();
    // There cannot be one readHTR because the default parameter order
    // for 2 arguments would be wrong 
    bool readHTR(const char* fileName, string path="");
    bool readHTR(const char* fileName, string subdirectory, string path);
    bool readHTR(FILE* htrFileHandle);
    bool writeToFile(char* fileName);

    void attachMesh(const string& objectName,
		    const string& parent) { 
      attachMesh(objectName,parent,false); 
    }
    void attachMesh(const string& objectName,
		    arGraphicsNode* parent){
      attachMesh(objectName,parent->getName());
    }
    void attachMesh(const string& baseName, 
                    const string& where, 
                    bool withLines);
    string type(void) { return "HTR"; }

    /// @todo implement this function; make it so the base position fits
    /// in a unit sphere...
    void normalizeModelSize(void);

    // animation functions
    bool supportsAnimation(void) { return true; }
    bool setFrame(int newFrame);
    bool nextFrame();
    bool prevFrame();
    bool setBasePosition();

    // stats
    inline int	numberOfFrames()	{ return numFrames; }
    inline int currentFrame()		{ return _currentFrame; }
    inline int	numberOfSegments()	{ return numSegments; }
    inline int	version()		{ return fileVersion; }
    inline string	nameOfSegment(int i) { return string(segmentData[i]->name); }
    inline int	transformIDForSegment(int i) 
                    { return segmentData[i]->transformID; }
    inline int	preTransformIDForSegment(int i)
                    { return segmentData[i]->preTransformID; }
    inline int	postTransformIDForSegment(int i)
                    { return segmentData[i]->postTransformID; }
    inline int	localTransformIDForSegment(int i)
                    { return segmentData[i]->localTransformID;}
    inline int	inverseIDForSegment(int i)
                    { return segmentData[i]->invTransformID; }
    inline int	boundingSphereIDForSegment(int i)
                    { return segmentData[i]->boundingSphereID; }
    arMatrix4	segmentBaseTransformRelative(int segmentID);
    int		numberOfSegment(const string& segmentName);
    arMatrix4	inverseTransformForSegment(int i);

  protected:
    // Reading in data functions
    bool parseHeader(FILE* htrFileHandle);
    bool parseHierarchy(FILE* htrFileHandle);
    bool parseBasePosition(FILE *htrFileHandle);
    bool parseSegmentData(FILE* htrFileHandle);
    bool precomputeData(void);
    bool setInvalid();
    void subNormalizeModelSize(arVector3 thePoint, arVector3 &minVec,
		    	       arVector3 &maxVec, htrBasePosition *theBP);
    arMatrix4 HTRTransform(struct htrBasePosition* theBP, struct htrFrame* theFrame);
    arMatrix4 HTRRotation(double Rx, double Ry, double Rz);
    
    void attachChildNode(const string& baseName, 
                         struct htrBasePosition* node, 
                         bool withLines=false);

  private:
    int fileVersion;
    int numSegments;
    int numFrames;
    int dataFrameRate;
    char *fileType;
    char *dataType;
    int  eulerRotationOrder;	//< global order of rotations
    char *calibrationUnits;	//< units of translation
    char *rotationUnits;
    char globalAxisOfGravity;
    char boneLengthAxis;
    double scaleFactor;
    vector<struct htrSegmentHierarchy*> childParent;	//< hierarchy
    vector<struct htrBasePosition*> basePosition;
    vector<struct htrSegmentData*> segmentData;

    int _currentFrame;
    arVector3 _normCenter;	//< middle of the model
    double _normScaleAmount;	//< how much to scale model by to fit in unit sphere
};

#endif // __AR_HTR_H
