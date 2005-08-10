//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

/// \todo: avoid arHTR.cpp's many implicit string(foo->name) temporaries.

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arHTR.h"
#include "arGraphicsAPI.h"

#ifndef M_PI
#define M_PI 3.14159
#endif

#define MAXLINE 500
#define MAXTOKENS 50

/// converts degrees to radians
/// @param angle degrees
inline double deg2rad(const double &angle){
  return (M_PI/180.) * angle;
}

/// returns rotation matrix of 3 euler angles, multiplied in
/// the order specified within the HTR file
arMatrix4 arHTR::HTRRotation(double Rx, double Ry, double Rz){
  arMatrix4 rotMatrix;
  arVector3 xVec(1,0,0), yVec(0,1,0), zVec(0,0,1);

  switch (eulerRotationOrder) {
    case AR_XYZ:
      rotMatrix = ar_rotationMatrix(xVec,deg2rad(Rx)) *
	ar_rotationMatrix(yVec,deg2rad(Ry)) *
	ar_rotationMatrix(zVec,deg2rad(Rz));
      break;
    case AR_XZY:
      rotMatrix = ar_rotationMatrix(xVec,deg2rad(Rx)) *
	ar_rotationMatrix(zVec,deg2rad(Rz)) *
	ar_rotationMatrix(yVec,deg2rad(Ry));
      break;
    case AR_YXZ:
      rotMatrix = ar_rotationMatrix(yVec,deg2rad(Ry)) *
	ar_rotationMatrix(xVec,deg2rad(Rx)) *
	ar_rotationMatrix(zVec,deg2rad(Rz));
      break;
    case AR_YZX:
      rotMatrix = ar_rotationMatrix(yVec,deg2rad(Ry)) *
	ar_rotationMatrix(zVec,deg2rad(Rz)) *
	ar_rotationMatrix(xVec,deg2rad(Rx));
      break;
    case AR_ZXY:
      rotMatrix = ar_rotationMatrix(zVec,deg2rad(Rz)) *
	ar_rotationMatrix(xVec,deg2rad(Rx)) *
	ar_rotationMatrix(yVec,deg2rad(Ry));
      break;
    case AR_ZYX:
      rotMatrix = ar_rotationMatrix(zVec,deg2rad(Rz)) *
        ar_rotationMatrix(yVec,deg2rad(Ry)) *
	ar_rotationMatrix(xVec,deg2rad(Rx));
      break;
    default:
      cerr << "arHTR warning: no valid rotation order found.\n";
      break;
  }
  return rotMatrix;
}

/// returns a special HTR matrix, of the form
/// (BaseTranslation+FrameTranslation)*(BaseRotation*FrameRotation)
/// @param theBP baseposition to use for calculation
/// @param theFrame frame to use for calculation
arMatrix4 arHTR::HTRTransform(struct htrBasePosition *theBP, 
                              struct htrFrame *theFrame){
  return (theFrame == NULL) ?
    theBP->trans * theBP->rot :
    (theBP->trans +
     ar_translationMatrix(theFrame->Tx, theFrame->Ty, theFrame->Tz) -
     arMatrix4()) *
     (theBP->rot * HTRRotation(theFrame->Rx, theFrame->Ry, theFrame->Rz));
}

/// attaches transform hierarchy to specified node in hierarchy,
/// optionally drawing lines for bones
/// @param baseName name of the new OBJ node to insert
/// @param where name of the node to which we attach the OBJ
/// \param withLines (optional) draw lines for bones -- useful if no geometry will be 
///		     attached to the transform hierarchy
void arHTR::attachMesh(const string& baseName, 
                       const string& where, 
                       bool withLines){
  if (_invalidFile){
    printf("cannot attach mesh: No valid file!\n");
    return;
  }
  const string transformModifier(".transform");
  unsigned int i = 0;
  arMatrix4 tempTransform;
  htrBasePosition* rootBasePosition = NULL;
  string rootName;
  vector<string> rootNames;
  
  // create global transform
  dgTransform(baseName+".GLOBAL"+transformModifier, where, 
	 ar_scaleMatrix(scaleFactor)*ar_scaleMatrix(_normScaleAmount)
	 * ar_translationMatrix(-_normCenter) );

  // find root node(s)
  for (i=0; i<childParent.size(); i++){
    if (!strcmp(childParent[i]->parent, "GLOBAL")){
      rootName = string(childParent[i]->child);
      rootNames.push_back(rootName);
    }
  }
  // for each root node
  for (unsigned int ri=0; ri<rootNames.size(); ri++)
  {
    rootName = rootNames[ri];
    for (i=0; i<basePosition.size(); i++){
      if (string(basePosition[i]->name) == rootName){
        rootBasePosition = basePosition[i];
        break;
      }
    }
    // recurse through children
    attachChildNode(baseName, rootBasePosition, withLines);
  }
}

/// gets called by attachMesh()
/** Recursively adds transform nodes, then children of nodes, based on
 *  the structure of the OBJ file.
 */
/// @param baseName name of the new OBJ node to insert
/// @param node name of the node to attach to the database
/// @param withLines (optional) draw lines for bones 
void arHTR::attachChildNode(const string& baseName, 
                            struct htrBasePosition* node, 
                            bool withLines){
  if (!node)
    return;

  const string tempName = "."+string(node->name);
  string tempParent, tempModifier;
  arMatrix4 tempTransform = HTRTransform(node,NULL);
  if (node->segment->parent) {
    tempParent = "."+string(node->segment->parent->name);
    tempModifier = ".preTransform";
  }
  else {
    tempParent = ".GLOBAL";
    tempModifier = ".transform";
    //cout << tempName << ": " << ar_extractScaleMatrix(tempTransform) << endl;
  }
  node->segment->postTransformID = 
    dgTransform(baseName + tempName + ".postTransform",
                baseName + tempParent + tempModifier,
                ar_identityMatrix());
  node->segment->transformID = 
    dgTransform(baseName + tempName + ".transform",
		baseName + tempName + ".postTransform",
		tempTransform);
  node->segment->preTransformID = 
    dgTransform(baseName + tempName + ".preTransform",
		baseName + tempName + ".transform",
		ar_identityMatrix());
  node->segment->localTransformID = 
    dgTransform(baseName + tempName + ".localTransform",
		baseName + tempName + ".preTransform",
		ar_identityMatrix());
  node->segment->invTransformID = 
    dgTransform(baseName + tempName + ".invTransform",
		baseName + tempName + ".localTransform",
		ar_identityMatrix()); 
  node->segment->boundingSphereID =
    dgBoundingSphere(baseName + tempName + ".boundingSphere",
                     baseName + tempName + ".invTransform",
                     0, 1, arVector3(0,0,0));
    
  /// add geom -- adds lines to show bones (placeholder)
  if (withLines) {
    node->segment->scaleID = dgTransform(baseName + tempName + ".bonescale",
                                         baseName + tempName + ".transform",
                                         ar_scaleMatrix(node->boneLength, 
							node->boneLength,
							node->boneLength));
    float pointPositions[6] = {0,0,0,
                               (boneLengthAxis=='X')?1:0,
                               (boneLengthAxis=='Y')?1:0,
                               (boneLengthAxis=='Z')?1:0};
    dgPoints(baseName + tempName + ".points",
	     baseName + tempName + ".bonescale",
	     2, pointPositions);
    arVector3 lineColor((double)rand()/(double)RAND_MAX,
                        (double)rand()/(double)RAND_MAX,
                        (double)rand()/(double)RAND_MAX);
    float colors[8] = {lineColor[0], lineColor[1], lineColor[2], 1,
		       lineColor[0], lineColor[1], lineColor[2], 1};
    dgColor4(baseName + tempName + ".colors",
	     baseName + tempName + ".points",
             2, colors);
    dgDrawable(baseName + tempName + ".drawable",
               baseName + tempName + ".colors",
               DG_LINES, 1);
  }

  /// recurse through children
  for (unsigned int i=0; i<node->segment->children.size(); i++)
    attachChildNode(baseName, node->segment->children[i]->basePosition, 
                    withLines);
}

/// Calculates values for normalization matrix
/// to translate + scales all frames to fit within unit sphere
void arHTR::normalizeModelSize(void) {
  arVector3 minVec(10000,10000,10000), maxVec(-10000,-10000,-10000);
  htrBasePosition* rootBasePosition = NULL;
  string rootName;
  vector<string> rootNames;
  unsigned int i = 0;

  // find root node(s)
  for (i=0; i<childParent.size(); i++){
    if (!strcmp(childParent[i]->parent, "GLOBAL")) {
      rootName = string(childParent[i]->child);
      rootNames.push_back(rootName);
    }
  }
  if (rootNames.size() > 1)
    cerr<<"Found "<<rootNames.size()<<" roots\n";
  // for each root node
  for (unsigned int ri=0; ri<rootNames.size(); ri++) {
    rootName = rootNames[ri];
    for (i=0; i<basePosition.size(); i++) {
      if (string(basePosition[i]->name) == rootName){
        rootBasePosition = basePosition[i];
        break;
      }
    }
    // recurse through children
    subNormalizeModelSize(arVector3(0,0,0), minVec, maxVec, rootBasePosition);
  }
  _normCenter = (maxVec + minVec)/2.;
  _normScaleAmount = 1./(scaleFactor*sqrt( (maxVec-_normCenter)%(maxVec-_normCenter) ));
}

/// Helper function for normalizeModelSize recursion
void arHTR::subNormalizeModelSize(arVector3 thePoint, arVector3& minVec,
				  arVector3& maxVec, htrBasePosition* theBP) {
  unsigned int i = 0;
  for (int j=0; j<numFrames; j++) {
    arVector3 newPoint = HTRTransform(theBP,theBP->segment->frame[j])*thePoint;
    // Throw out bogus data (the MotionAnalysis system tags problematic points
    // with 9999999.
    if (++newPoint < 1000000){
      for (i=0; i<3; i++) {
        maxVec[i] = newPoint[i] > maxVec[i] ? newPoint[i] : maxVec[i];
        minVec[i] = newPoint[i] < minVec[i] ? newPoint[i] : minVec[i];
      }
    }
  }
  for (i=0; i<theBP->segment->children.size(); i++)
    subNormalizeModelSize(thePoint, minVec, maxVec,
  		    	    theBP->segment->children[i]->basePosition);
}

arMatrix4 arHTR::segmentBaseTransformRelative(int segmentID){
  return HTRTransform(basePosition[segmentID], NULL);
}

// Here, we are relying on the fact that the MotionAnalysis data tells us
// it is bogus by having absurdly large values (actually 9999999).
bool arHTR::frameValid(htrFrame* f){
  if (fabs(f->Tx) < 9000000 && fabs(f->Ty) < 9000000 && 
      fabs(f->Tz) < 9000000 && fabs(f->Rx) < 9000000 && 
      fabs(f->Ry) < 9000000 && fabs(f->Rz) < 9000000 &&
      fabs(f->scale) < 9000000){
    return true;
  }
  return false;
}

void arHTR::frameInterpolate(htrFrame* f,
			     htrFrame* interp1,
			     htrFrame* interp2){
  // We only try to interpolate if both interp1 and interp2 are valid.
  if (interp1 && interp2){
    arQuaternion q1 = HTRRotation(interp1->Rx, interp1->Ry, interp1->Rz);
    arQuaternion q2 = HTRRotation(interp2->Rx, interp2->Ry, interp2->Rz);
    // Must determine *which* quaternion representation to use since the 
    // negative of a quaternion is the SAME rotation! We must, for instance,
    // NOT linearly interpolate between a quaternion and its negative.
    float dot = q1.real*q2.real + q1.pure[0]*q2.pure[0]
      + q1.pure[1]*q2.pure[1] + q1.pure[2]*q2.pure[2];
    if (dot < 0){
      q2 = -q2;
    }
    float weight = 1 - ((float)(f->frameNum - interp1->frameNum))/
                   ((float)(interp2->frameNum - interp1->frameNum)); 
    arQuaternion newRot = weight*q1 + (1-weight)*q2;
    newRot = newRot/++newRot;
    arMatrix4 m = newRot;
    
    // Notice how we allow for different euler angle orders.
    arVector3 euler = ar_extractEulerAngles(m, eulerRotationOrder);
    switch(eulerRotationOrder){
    case AR_XYZ:
      f->Rx = ar_convertToDeg(euler[2]);
      f->Ry = ar_convertToDeg(euler[1]);
      f->Rz = ar_convertToDeg(euler[0]);
      break;
    case AR_XZY:
      f->Rx = ar_convertToDeg(euler[2]);
      f->Ry = ar_convertToDeg(euler[0]);
      f->Rz = ar_convertToDeg(euler[1]);
      break;
    case AR_YXZ:
      f->Rx = ar_convertToDeg(euler[1]);
      f->Ry = ar_convertToDeg(euler[2]);
      f->Rz = ar_convertToDeg(euler[0]);
      break;
    case AR_YZX:
      f->Rx = ar_convertToDeg(euler[0]);
      f->Ry = ar_convertToDeg(euler[2]);
      f->Rz = ar_convertToDeg(euler[1]);
      break;
    case AR_ZXY:
      f->Rx = ar_convertToDeg(euler[1]);
      f->Ry = ar_convertToDeg(euler[0]);
      f->Rz = ar_convertToDeg(euler[2]);
      break;
    case AR_ZYX:
      f->Rx = ar_convertToDeg(euler[0]);
      f->Ry = ar_convertToDeg(euler[1]);
      f->Rz = ar_convertToDeg(euler[2]);
      break;
    }
    f->Tx = weight*interp1->Tx + (1-weight)*interp2->Tx;
    f->Ty = weight*interp1->Ty + (1-weight)*interp2->Ty;
    f->Tz = weight*interp1->Tz + (1-weight)*interp2->Tz;
    f->scale = weight*interp1->scale+(1-weight)*interp2->scale;
  }
}

/// Goes through the HTR data, finds where gaps exist (as indicated by the
/// MotionAnalysis 9999999 representation of "infinity" and interpolates them
/// out of existence (just a simple linear interpolation, but that's actually
/// good enough for most cases).
void arHTR::basicDataSmoothing(){
  for (unsigned int i=0; i<segmentData.size(); i++){
    for (unsigned int j=0; j<segmentData[i]->frame.size(); j++){
      htrFrame* f = segmentData[i]->frame[j];
      if (!frameValid(f)){
	// Find the latest previous valid frame, if any.
	int prev = j;
	while (prev >= 0 && !frameValid(segmentData[i]->frame[prev])){
	  prev--;
	}
	int next = j;
	// Find the next valid frame, if any.
	while (next < segmentData[i]->frame.size()
	       && !frameValid(segmentData[i]->frame[next])){
	  next++;
	}
	htrFrame* interp1 = NULL;
	htrFrame* interp2 = NULL;
	if (prev >= 0){
	  interp1 = segmentData[i]->frame[prev];
	}
	if (next < segmentData[i]->frame.size()){
	  interp2 = segmentData[i]->frame[next];
	}
        frameInterpolate(f, interp1, interp2);
	// Must update the matrices.
        f->trans = HTRTransform(segmentData[i]->basePosition, f);
        f->totalScale = segmentData[i]->basePosition->boneLength * f->scale;
      }
    }
  }
}

/// returns internal index of specified segment
/// @param segmentName name of the segment whose index we want
int arHTR::numberOfSegment(const string& segmentName){
  for(unsigned int i=0; i<segmentData.size(); i++)
    if (segmentData[i]->name == segmentName)
      return i;
  return 0;
}

/// returns matrix that negates all prev. transforms up hierarchy
/// @param i HTR internal index number of segment
arMatrix4 arHTR::inverseTransformForSegment(int i){
  htrSegmentData* theSegment = segmentData[i];
  arMatrix4 theTransform = HTRTransform(theSegment->basePosition, NULL);
  while (theSegment->parent != NULL) {
    theSegment = theSegment->parent;
    theTransform = HTRTransform(theSegment->basePosition, NULL)*theTransform;
  }
  return !theTransform * !ar_scaleMatrix(scaleFactor,scaleFactor,scaleFactor);
}


// // Animation functions // //

/// sets the frame of the .htr animation
/// @param newFrame the frame to go to
bool arHTR::setFrame(int newFrame){
  if (newFrame >= numFrames || newFrame < 0)
    return false;

  for(unsigned int i=0; i<segmentData.size(); i++){    
    dgTransform(segmentData[i]->transformID, 
                segmentData[i]->frame[newFrame]->trans);
    // There is a scale matrix for the bone.
    if (segmentData[i]->scaleID > 0){
      dgTransform(segmentData[i]->scaleID,
                  ar_scaleMatrix(segmentData[i]->frame[newFrame]->totalScale));
    }
  }
  
  _currentFrame = newFrame;
  return true;
}

/// goes to next frame, or returns false if at last frame
bool arHTR::nextFrame(){
  return setFrame(_currentFrame+1);
}

/// goes to previous frame, or returns false if at first frame
bool arHTR::prevFrame(){
  return setFrame(_currentFrame-1);
}

/// sets transforms to base position (default pose)
bool arHTR::setBasePosition(){
  for(unsigned int i=0; i<segmentData.size(); i++)
    dgTransform(segmentData[i]->transformID,
                HTRTransform(segmentData[i]->basePosition, NULL));
  return true;
}
