//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// todo: avoid arHTR.cpp's many implicit string(foo->name) temporaries.

#include "arPrecompiled.h"
#include "arHTR.h"
#include "arGraphicsAPI.h"

#define MAXLINE 500
#define MAXTOKENS 50

// converts degrees to radians
// @param angle degrees
inline double deg2rad(const double &angle){
  return (M_PI/180.) * angle;
}

// returns rotation matrix of 3 euler angles, multiplied in
// the order specified within the HTR file
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

// returns a special HTR matrix, of the form
// (BaseTranslation+FrameTranslation)*(BaseRotation*FrameRotation)
// @param theBP baseposition to use for calculation
// @param theFrame frame to use for calculation
arMatrix4 arHTR::HTRTransform(htrBasePosition *theBP, 
                              htrFrame *theFrame){
  return (theFrame == NULL) ? theBP->trans * theBP->rot :
    theBP->trans *
    ar_translationMatrix(theFrame->Tx, theFrame->Ty, theFrame->Tz) *
    theBP->rot * HTRRotation(theFrame->Rx, theFrame->Ry, theFrame->Rz);
}

bool arHTR::attachMesh(const string& objectName,
		       const string& parent) {
  arGraphicsNode* n = dgGetNode(parent);
  return n && attachMesh(n, objectName, false);
}

bool arHTR::attachMesh(const string& /*baseName*/, 
                       const string& where, 
                       bool withLines){
  arGraphicsNode* n = dgGetNode(where);
  return n && attachMesh(n, where, withLines);
}

// Attaches transform hierarchy to specified node in hierarchy,
// optionally drawing lines for bones.
// @param parent The node to which we will attach the HTR.
// @param baseName Name of the new object to insert (affects the node names).
// @param withLines (optional) draw lines for bones -- useful if no geometry will be 
//		     attached to the transform hierarchy
bool arHTR::attachMesh(arGraphicsNode* parent,
                       const string& objectName,  
                       bool withLines){
  if (_invalidFile){
    ar_log_error() << "arHTR cannot attach mesh: No valid file.\n";
    return false;
  }

  const string transformModifier(".transform");
  unsigned int i = 0;
  htrBasePosition* rootBasePosition = NULL;
  string rootName;
  vector<string> rootNames;
  
  // Create global transform
  arTransformNode* transform = (arTransformNode*) parent->newNode("transform", objectName+".GLOBAL"+transformModifier);
  transform->setTransform(ar_scaleMatrix(scaleFactor)
                          * ar_scaleMatrix(_normScaleAmount)
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
    attachChildNode(transform, objectName, rootBasePosition, withLines);
  }
  return true;
}

// Gets called by attachMesh(). Recursively adds transform nodes, then children of nodes, 
// based on the structure of the OBJ file.
// @param objectName name of the new OBJ node to insert
// @param node name of the node to attach to the database
// @param withLines (optional) draw lines for bones 
void arHTR::attachChildNode(arGraphicsNode* parent,
                            const string& objectName, 
                            htrBasePosition* node, 
                            bool withLines){
  if (!node){
    return;
  }

  const string tempName = "." + string(node->name);
  arMatrix4 tempTransform = HTRTransform(node, NULL);
 
  node->segment->postTransformNode = (arTransformNode*)
    parent->newNode("transform", objectName + tempName + ".postTransform");
  node->segment->postTransformNode->setTransform(ar_identityMatrix());
  node->segment->transformNode = (arTransformNode*)
    node->segment->postTransformNode->newNode("transform", objectName + tempName + ".transform");
  node->segment->transformNode->setTransform(tempTransform);
  node->segment->preTransformNode = (arTransformNode*)
    node->segment->transformNode->newNode("transform", objectName + tempName + ".preTransform");
  node->segment->preTransformNode->setTransform(ar_identityMatrix());
  node->segment->localTransformNode = (arTransformNode*)
    node->segment->preTransformNode->newNode("transform", objectName + tempName + ".localTransform");
  node->segment->localTransformNode->setTransform(ar_identityMatrix());
  node->segment->invTransformNode = (arTransformNode*)
    node->segment->localTransformNode->newNode("transform", objectName + tempName + ".invTransform");
  node->segment->invTransformNode->setTransform(ar_identityMatrix()); 
  node->segment->boundingSphereNode =(arBoundingSphereNode*)
    node->segment->invTransformNode->newNode("bounding sphere", objectName + tempName + ".boundingSphere");
  arBoundingSphere b;
  b.visibility = false;
  b.radius = 1;
  b.position = arVector3(0,0,0);
  node->segment->boundingSphereNode->setBoundingSphere(b);
    
  // add geom -- adds lines to show bones (placeholder)
  if (withLines) {
    node->segment->scaleNode = (arTransformNode*)
      node->segment->transformNode->newNode("transform", objectName + tempName + ".bonescale");
    node->segment->scaleNode->setTransform(ar_scaleMatrix(node->boneLength, 
							  node->boneLength,
							  node->boneLength));
    float pointPositions[6] = {0,0,0,
                               (boneLengthAxis=='X')?1:0,
                               (boneLengthAxis=='Y')?1:0,
                               (boneLengthAxis=='Z')?1:0};
    arPointsNode* points = (arPointsNode*) 
      node->segment->scaleNode->newNode("points", objectName + tempName + ".points");
    points->setPoints(2, pointPositions);
    arVector3 lineColor((double)rand()/(double)RAND_MAX,
                        (double)rand()/(double)RAND_MAX,
                        (double)rand()/(double)RAND_MAX);
    float colors[8] = {lineColor[0], lineColor[1], lineColor[2], 1,
		       lineColor[0], lineColor[1], lineColor[2], 1};
    arColor4Node* color4 = (arColor4Node*) points->newNode("color4", objectName + tempName + ".colors");
    color4->setColor4(2, colors);
    arDrawableNode* drawable = (arDrawableNode*) color4->newNode("drawable", objectName + tempName + ".drawable");
    drawable->setDrawable(DG_LINES, 1);
  }

  // recurse through children
  for (unsigned int i=0; i<node->segment->children.size(); i++)
    attachChildNode(node->segment->preTransformNode, objectName, node->segment->children[i]->basePosition, withLines);
}

// Calculates values for normalization matrix
// to translate + scales all frames to fit within unit sphere
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

// Helper function for normalizeModelSize recursion
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

// MotionAnalysis represent bogus values as 9999999 (infinity).
bool arHTR::frameValid(htrFrame* f){
  const float big = 9000000;
  return
    fabs(f->Tx) < big && fabs(f->Ty) < big && fabs(f->Tz) < big &&
    fabs(f->Rx) < big && fabs(f->Ry) < big && fabs(f->Rz) < big &&
    fabs(f->scale) < big;
}

#define lerp(w, a, b) ((w) * (a) + (1.0-(w)) * (b));
#define lerpField(w, dst, src1, src2, field) dst->field = lerp(w, src1->field, src2->field)

void arHTR::frameInterpolate(htrFrame* dst,
			     const htrFrame* src1,
			     const htrFrame* src2){
  if (!src1 || !src2)
    return;

  const arQuaternion q1(HTRRotation(src1->Rx, src1->Ry, src1->Rz));
  arQuaternion q2(HTRRotation(src2->Rx, src2->Ry, src2->Rz));
  // Don't lerp between a quarternion and its negative (which is the same rotation).
  if (q1.dot(q2) < 0){
    q2 = -q2;
  }
  const float weight = 1. -
    float(dst->frameNum - src1->frameNum) / float(src2->frameNum - src1->frameNum); 

  lerpField(weight, dst, src1, src2, Tx);
  lerpField(weight, dst, src1, src2, Ty);
  lerpField(weight, dst, src1, src2, Tz);
  lerpField(weight, dst, src1, src2, scale);

  const arQuaternion q = lerp(weight, q1, q2);
  arVector3 euler(ar_convertToDeg(ar_extractEulerAngles(q.normalize(), eulerRotationOrder)));
  switch(eulerRotationOrder) {
  case AR_XYZ:
    dst->Rx = euler[2];
    dst->Ry = euler[1];
    dst->Rz = euler[0];
    break;
  case AR_XZY:
    dst->Rx = euler[2];
    dst->Ry = euler[0];
    dst->Rz = euler[1];
    break;
  case AR_YXZ:
    dst->Rx = euler[1];
    dst->Ry = euler[2];
    dst->Rz = euler[0];
    break;
  case AR_YZX:
    dst->Rx = euler[0];
    dst->Ry = euler[2];
    dst->Rz = euler[1];
    break;
  case AR_ZXY:
    dst->Rx = euler[1];
    dst->Ry = euler[0];
    dst->Rz = euler[2];
    break;
  case AR_ZYX:
    dst->Rx = euler[0];
    dst->Ry = euler[1];
    dst->Rz = euler[2];
    break;
  }
}

// Lerp between gaps in data found by frameValid().
void arHTR::basicDataSmoothing(){
  for (unsigned i=0; i<segmentData.size(); i++){
    for (unsigned j=0; j<segmentData[i]->frame.size(); j++){
      htrFrame* f = segmentData[i]->frame[j];
      if (!frameValid(f)){
	// Find the latest previous valid frame, if any.
	int prev = j; // could go negative
	while (prev >= 0 && !frameValid(segmentData[i]->frame[prev]))
	  --prev;
	unsigned next = j;
	// Find the next valid frame, if any.
	while (next < segmentData[i]->frame.size()
	       && !frameValid(segmentData[i]->frame[next]))
	  ++next;
	htrFrame* interp1 = (prev >= 0) ?
	  segmentData[i]->frame[prev] : NULL;
	htrFrame* interp2 = (next < segmentData[i]->frame.size()) ?
	  segmentData[i]->frame[next] : NULL;
        frameInterpolate(f, interp1, interp2);
	// Update the matrices.
        f->trans = HTRTransform(segmentData[i]->basePosition, f);
        f->totalScale = segmentData[i]->basePosition->boneLength * f->scale;
      }
    }
  }
}

// returns internal index of specified segment
// @param segmentName name of the segment whose index we want
int arHTR::numberOfSegment(const string& segmentName){
  for(unsigned int i=0; i<segmentData.size(); i++)
    if (segmentData[i]->segmentName == segmentName)
      return i;
  return 0;
}

// returns matrix that negates all prev. transforms up hierarchy
// @param i HTR internal index number of segment
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

// sets the frame of the .htr animation
// @param newFrame the frame to go to
bool arHTR::setFrame(int newFrame){
  if (newFrame >= numFrames || newFrame < 0)
    return false;

  for(unsigned int i=0; i<segmentData.size(); i++){
    if (segmentData[i]->transformNode){
      segmentData[i]->transformNode->setTransform(segmentData[i]->frame[newFrame]->trans);
    }
    // There is a scale matrix for the bone.
    if (segmentData[i]->scaleNode){
      segmentData[i]->scaleNode->setTransform(ar_scaleMatrix(segmentData[i]->frame[newFrame]->totalScale));
    }
  }
  
  _currentFrame = newFrame;
  return true;
}

// goes to next frame, or returns false if at last frame
bool arHTR::nextFrame(){
  return setFrame(_currentFrame+1);
}

// goes to previous frame, or returns false if at first frame
bool arHTR::prevFrame(){
  return setFrame(_currentFrame-1);
}

// sets transforms to base position (default pose)
bool arHTR::setBasePosition(){
  for(unsigned int i=0; i<segmentData.size(); i++){
    if (segmentData[i]->transformNode){
      segmentData[i]->transformNode->setTransform(HTRTransform(segmentData[i]->basePosition, NULL));
    }
  }
  return true;
}
