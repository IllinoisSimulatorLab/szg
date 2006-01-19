//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arHTR.h"
#include <string.h>
using namespace std;

const int MAXLINE  = 600;
const int MAXTOKENS  = 50;

arHTR::arHTR() :
  fileType(NULL),
  dataType(NULL),
  calibrationUnits(NULL),
  rotationUnits(NULL),
  _currentFrame(0),
  _normCenter(arVector3(0,0,0)),
  _normScaleAmount(1) {
}

arHTR::~arHTR(){
  if (fileType)
    free(fileType);
  if (dataType)
    free(dataType);
  if (calibrationUnits)
    free(calibrationUnits);
  if (rotationUnits)
    free(rotationUnits);
}

/// marks this HTR file as invalid
bool arHTR::setInvalid(void){
  _invalidFile = true;
  return false;
}

// Wrapper for the 3 parameter version.
bool arHTR::readHTR(const char* fileName, string path){
  return readHTR(fileName, "", path);
}

/// Reads HTR file specified
/// @param fileName name of the HTR file, including extension
/// @param path for some reason, a path string
/// Actually checks that it exists and calls readHTR(FILE*)
bool arHTR::readHTR(const char* fileName, string subdirectory, string path){
  string theFileName(fileName);
  FILE* htrFileHandle = ar_fileOpen(theFileName, subdirectory, path, "r");
  if (!htrFileHandle){
    cerr << "arHTR error: failed to open file \""
         << fileName << "\".\n";
    return setInvalid();
  }
  return readHTR(htrFileHandle);
}

/// reads HTR file specified
/// @param htrFileHandle HTR file to read in
bool arHTR::readHTR(FILE* htrFileHandle){
  if (!htrFileHandle){
    cerr << "arHTR error: received invalid file pointer.\n";
    return setInvalid();
  }

  if (!parseHeader(htrFileHandle) ||
      !parseHierarchy(htrFileHandle) ||
      !parseBasePosition(htrFileHandle) ||
      !parseSegmentData(htrFileHandle))
    return setInvalid();
  fclose(htrFileHandle);
  return precomputeData() || setInvalid();
}

/// parses one line of the file, returning false if error/incorrect data on line
/// @param theFile the file we're reading
/// @param theResult pointers to c-strings containing the data
/// @param buffer holds the current line to read from
/// @param desiredTokens how many arguments we're expecting on this line -- 
///	determines whether we pass back true or false
/// @param errorString the argument to write out in case of an error -- 
///	very useful for finding syntax errors in files
bool parseLine(FILE* theFile, char* theResult[], char* buffer, int desiredTokens, string errorString){
  bool found = false;
  char* value = (char *)1;
  int numTokens = 0;

  while (!found && value){
    value = fgets(buffer, MAXLINE, theFile);
    // skip if comment or newline
    if (buffer[0] == '\n' || buffer[0] == '#')
      found = false;
    else if (value)
      found = true;
  }
  if (!found){
    cerr << "arHTR error: premature end of file.\n";
    return false;
  }

  // Parse Line. Note how we need to add the '\r' to acomodate reading Win32
  // text files on Unix.
  theResult[numTokens++] = strtok(buffer, " \t\n\r");
  while (theResult[numTokens-1]){
    theResult[numTokens++] = strtok(NULL, " \t\n\r");
  }
  if (--numTokens < desiredTokens){
    cerr << "arHTR error: invalid or missing \"" << errorString << "\" parameter.\n"
         << "       -Found \"" <<  string(theResult[0])
	 << "\" in line: " << buffer << endl;
    return false;
  }

  return true;
}

bool arHTR::parseHeader(FILE* htrFileHandle){
  bool found = false;
  char textLine[MAXLINE];
  char *token[MAXTOKENS];

  while (!found && fgets(textLine, MAXLINE, htrFileHandle)){
    if (!strncmp("[Header]", textLine, strlen("[Header]")))
      found = true;
  }
  if (!found){
    cerr << "arHTR error: 'Header' not found.\n";
    return false;
  }

  if (!parseLine(htrFileHandle, token, textLine, 2, "fileType"))
    return false;

  fileType = (char *)malloc((strlen(token[1])+1)*sizeof(char));
  strcpy(fileType, token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "dataType"))
    return false;

  dataType = (char *)malloc((strlen(token[1])+1)*sizeof(char));
  strcpy(dataType, token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "fileVersion"))
    return false;

  fileVersion = atoi(token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "numSegments"))
    return false;

  numSegments = atoi(token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "numFrames"))
    return false;

  numFrames = atoi(token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "dataFrameRate"))
    return false;

  dataFrameRate = atoi(token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "eulerRotationOrder"))
    return false;

  const string eRO(token[1]);
  if (eRO=="XYZ")
    eulerRotationOrder = AR_XYZ;
  else if (eRO=="XZY")
    eulerRotationOrder = AR_XZY;
  else if (eRO=="YXZ")
    eulerRotationOrder = AR_YXZ;
  else if (eRO=="YZX")
    eulerRotationOrder = AR_YZX;
  else if (eRO=="ZXY")
    eulerRotationOrder = AR_ZXY;
  else if (eRO=="ZYX")
    eulerRotationOrder = AR_ZYX;
  else
    printf("Invalid rotation order: %s\n", token[1]);

  if (!parseLine(htrFileHandle, token, textLine, 2, "calibrationUnits"))
    return false;

  calibrationUnits = (char *)malloc((strlen(token[1])+1)*sizeof(char));
  strcpy(calibrationUnits, token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "rotationUnits"))
    return false;

  rotationUnits = (char *)malloc((strlen(token[1])+1)*sizeof(char));
  strcpy(rotationUnits, token[1]);
  if (!parseLine(htrFileHandle, token, textLine, 2, "globalAxisOfGravity"))
    return false;

  globalAxisOfGravity = *token[1];
  if (!parseLine(htrFileHandle, token, textLine, 2, "boneLengthAxis"))
    return false;

  boneLengthAxis = *token[1];
  if (!parseLine(htrFileHandle, token, textLine, 2, "scaleFactor")){
    scaleFactor = .1;
    rewind(htrFileHandle);
  }
  else
    scaleFactor = 0.1*atof(token[1]);
  return true;
}

/// parses [SegmentNames&Hierarchy] section of .htr file
/// @param htrFileHandle the HTR file
/// Will read from pointer to end of file until it finds
/// "[SegmentNames&Hierarchy]"... or doesn't
bool arHTR::parseHierarchy(FILE *htrFileHandle){
  bool found = false;
  char textLine[MAXLINE];
  char *token[MAXTOKENS];
  while (!found && fgets(textLine, MAXLINE, htrFileHandle)){
    if (!strncmp("[SegmentNames&Hierarchy]",textLine,strlen("[SegmentNames&Hierarchy]")))
      found = true;
  }
  if (!found){
    cerr << "arHTR error: 'SegmentNames&Hierarchy' not found.\n";
    return false;
  }

  for (int j=0; j<numSegments; j++){
    if (!parseLine(htrFileHandle,token,textLine,2,"SegmentNames&Hierarchy"))
      return false;

    htrSegmentHierarchy *newSegmentHierarchy = new htrSegmentHierarchy;
    newSegmentHierarchy->child = (char *)malloc((strlen(token[0])+1)*sizeof(char));
    strcpy(newSegmentHierarchy->child, token[0]);
    newSegmentHierarchy->parent = (char *)malloc((strlen(token[1])+1)*sizeof(char));
    strcpy(newSegmentHierarchy->parent, token[1]);
    childParent.push_back(newSegmentHierarchy);
  }

  return true;
}

/// parses [BasePosition] section of .htr file
/// @param htrFileHandle the HTR file
/// Will read from pointer to end of file until it finds
/// "[BasePosition]"... or doesn't
bool arHTR::parseBasePosition(FILE *htrFileHandle){
  bool found = false;
  char textLine[MAXLINE];
  char *token[MAXTOKENS];

  while (!found && fgets(textLine, MAXLINE, htrFileHandle)){
    if (!strncmp("[BasePosition]",textLine,strlen("[BasePosition]")))
      found = true;
  }
  if (!found){
    cerr << "arHTR error: 'BasePosition' not found.\n";
    return false;
  }

  for (int i=0; i<numSegments; i++){
    if (!parseLine(htrFileHandle, token, textLine, 8, "BasePosition"))
      return false;

    htrBasePosition *newBasePosition = new htrBasePosition;
    newBasePosition->name = (char *)malloc((strlen(token[0])+1)*sizeof(char));
    strcpy(newBasePosition->name, token[0]);
    newBasePosition->Tx = atof(token[1]);
    newBasePosition->Ty = atof(token[2]);
    newBasePosition->Tz = atof(token[3]);
    newBasePosition->Rx = atof(token[4]);
    newBasePosition->Ry = atof(token[5]);
    newBasePosition->Rz = atof(token[6]);
    newBasePosition->boneLength = atof(token[7]);
    basePosition.push_back(newBasePosition);
/**/
  }
  return true;
}

/// Parses the segment data for a .htr file.
bool arHTR::parseSegmentData1(FILE* htrFileHandle){
  char* value = NULL;
  char textLine[MAXLINE] = {0};
  char *token[MAXTOKENS] = {0};
  htrSegmentData *newSegmentData = NULL;
  htrFrame *newFrame = NULL;
  for (int i=0; i<numSegments; i++){
    //printf("new Seg Data[%i]:\n", i);
    newSegmentData = new htrSegmentData;
    //printf("/new seg data\n");
    bool found = false;
    value = (char *)1;
    while (!found && value){
      value = fgets(textLine, MAXLINE, htrFileHandle);
      // skip if comment or newline
      if (textLine[0] == '\n' || textLine[0] == '#')
        found = false;
      else
        if (value)
          found = true;
    }
    if (!found){
      cerr << "arHTR error: premature end of file.\n";
      return false;
    }

    char *genStr = strrchr(textLine, ']');
    if (genStr)
      genStr[0] = '\0';
    //newSegmentData->name = (char*)malloc((strlen(textLine)-1)*sizeof(char));
    newSegmentData->name = new char[strlen(textLine)];
    strcpy(newSegmentData->name, textLine+1);
    for (int j=0; j<numFrames; j++){
      if (!parseLine(htrFileHandle, token, textLine, 8, "SegmentData"))
        return false;

      //printf("New frame[%i][%i]:\n", i, j);
      newFrame = new htrFrame;
      //printf("/new frame\n");
      newFrame->frameNum = atoi(token[0]);
    //printf("numFrames: %i\n", numFrames);
      newFrame->Tx = atof(token[1]);
      newFrame->Ty = atof(token[2]);
      newFrame->Tz = atof(token[3]);
      newFrame->Rx = atof(token[4]);
      newFrame->Ry = atof(token[5]);
      newFrame->Rz = atof(token[6]);
      newFrame->scale = atof(token[7]);
      newSegmentData->frame.push_back(newFrame);
    }
    segmentData.push_back(newSegmentData);
  }

  return true;
}

/// Parses the segment data for a .htr2 file. This file type has
/// a significantly different format than the .htr data.
/// Most importantly, it is streamable (instead of placing all
/// of one segment's frames together... all of the segments for
/// a frame are placed together).
bool arHTR::parseSegmentData2(FILE* htrFileHandle){
  char* value = NULL;
  char textLine[MAXLINE] = {0};
  char *token[MAXTOKENS] = {0};
  htrSegmentData *newSegmentData = NULL;
  htrFrame *newFrame = NULL;
  // First, we need to create storage for each segment's data.
  // The arHTR object stores all data for a segment together
  // (i.e. values for all frames) in an htrSegmentData object.
  for (int j=0; j<numSegments; j++){
    newSegmentData = new htrSegmentData();
    htrSegmentHierarchy* segment = childParent[j];
    newSegmentData->name = new char[strlen(segment->child)+1];
    strcpy(newSegmentData->name, segment->child);
    segmentData.push_back(newSegmentData);
  }
  for (int i=0; i<numFrames; i++){
    //printf("new Seg Data[%i]:\n", i);
    newSegmentData = new htrSegmentData;
    // First, go ahead and skip over comments, blank lines, etc.
    // until we get to the magic "Frame n:".
    bool found = false;
    value = (char *)1;
    while (!found && value){
      value = fgets(textLine, MAXLINE, htrFileHandle);
      // Skip if comment or newline. Note how we need to handle '\r' so
      // that 
      if (textLine[0] == '\n' || textLine[0] == '#' || textLine[0] == '\r')
        found = false;
      else
        if (value)
          found = true;
    }
    if (!found){
      cerr << "arHTR error: premature end of file.\n";
      return false;
    }
    // Check that we've found the correct marker for the
    // beginning of the frame data. 
    stringstream checkStream;
    checkStream << "Frame " << i << ":";
    if (false && strcmp(textLine, checkStream.str().c_str())){
      cout << "arHTR error: failed to find correct frame marker "
	   << "for frame=" << i << " (" << textLine << ").\n";
      return false;
    }
  
    for (int k=0; k<numSegments; k++){
      if (!parseLine(htrFileHandle, token, textLine, 8, "SegmentData"))
        return false;

      //printf("New frame[%i][%i]:\n", i, j);
      newFrame = new htrFrame;
      //printf("/new frame\n");
      newFrame->frameNum = i;
    //printf("numFrames: %i\n", numFrames);
      newFrame->Tx = atof(token[1]);
      newFrame->Ty = atof(token[2]);
      newFrame->Tz = atof(token[3]);
      newFrame->Rx = atof(token[4]);
      newFrame->Ry = atof(token[5]);
      newFrame->Rz = atof(token[6]);
      newFrame->scale = atof(token[7]);
      segmentData[k]->frame.push_back(newFrame);
    }
  }

  return true;
}

/// parses the frame data of .htr file
/// @param htrFileHandle the HTR file
bool arHTR::parseSegmentData(FILE* htrFileHandle){
  if (fileVersion == 1){
    return parseSegmentData1(htrFileHandle);
  }
  if (fileVersion == 2){
    return parseSegmentData2(htrFileHandle);
  }
  cout << "arHTR error: unhandled HTR file version = "
       << fileVersion << ".\n";
  return false;
}

/// fills in the data structures for easy, efficient access
bool arHTR::precomputeData(void){
  unsigned int k=0, l=0;
  int i=0, j=0;

  // connecting parents and children
  for (k=0; k<childParent.size(); k++)
    for (j=0; j<numSegments; j++)
      if (!strcmp(childParent[k]->child, segmentData[j]->name))
        for (i=0; i<numSegments; i++)
          if (!strcmp(childParent[k]->parent, segmentData[i]->name)){
            segmentData[j]->parent = segmentData[i];
            segmentData[i]->children.push_back(segmentData[j]);
            break;
	  }
  // connect segment data and base position
  for (k=0; k<segmentData.size(); k++)
    for (l=0; l<basePosition.size(); l++)
      if (!strcmp(basePosition[l]->name, segmentData[k]->name)){
        basePosition[l]->segment = segmentData[k];
        segmentData[k]->basePosition = basePosition[l];
      }
  // precompute basePosition transform matrices
  htrBasePosition *b;
  for (k=0; k<basePosition.size(); k++){
    b = basePosition[k];
    b->trans = ar_translationMatrix(b->Tx, b->Ty, b->Tz);
    b->rot   = HTRRotation(b->Rx, b->Ry, b->Rz);
  }
  // precompute matrices for frames
  htrFrame *h;
  for (k=0; k<segmentData.size(); k++)
    for (l=0; l<segmentData[k]->frame.size(); l++){
      h = segmentData[k]->frame[l];
      h->trans = HTRTransform(segmentData[k]->basePosition, h);
      h->totalScale = segmentData[k]->basePosition->boneLength * h->scale;
    }
  return true;
}

/// writes out current class data to an HTR file
/// @param fileName name of new OBJ file (including extension) to write out 
bool arHTR::writeToFile(char *fileName){
  FILE *htrFile = fopen(fileName, "w");
  if (!htrFile){
    cerr << "arHTR error: failed to write to file \""
         << fileName << "\".\n";
    return false;
  }

  fprintf(htrFile, "#Created by syzygy HTR component\n#Written by mflider\n");
  fprintf(htrFile, "[Header]\n");
  fprintf(htrFile, "FileType %s\n", fileType);
  fprintf(htrFile, "DataType %s\n", dataType);
  fprintf(htrFile, "FileVersion %i\n", fileVersion);
  fprintf(htrFile, "NumSegments %i\n", numSegments);
  fprintf(htrFile, "NumFrames %i\n", numFrames);
  fprintf(htrFile, "DataFrameRate %i\n", dataFrameRate);
  fprintf(htrFile, "EulerRotationOrder %s\n", eulerRotationOrder==AR_XYZ?"XYZ":
          eulerRotationOrder==AR_XZY?"XZY":eulerRotationOrder==AR_YXZ?"YXZ":
	  eulerRotationOrder==AR_YZX?"YZX":eulerRotationOrder==AR_ZXY?"ZXY":"ZYX");
  fprintf(htrFile, "CalibrationUnits %s\n", calibrationUnits);
  fprintf(htrFile, "RotationUnits %s\n", rotationUnits);
  fprintf(htrFile, "GlobalAxisofGravity %c\n", globalAxisOfGravity);
  fprintf(htrFile, "BoneLengthAxis %c\n", boneLengthAxis);
  fprintf(htrFile, "ScaleFactor %f\n", scaleFactor);

  fprintf(htrFile, "\n[SegmentNames&Hierarchy]\n");
  fprintf(htrFile, "#CHILD\tPARENT\n");
  unsigned int i;
  for (i=0; i<childParent.size(); i++){
    fprintf(htrFile, "%s\t%s\n", childParent[i]->child, childParent[i]->parent);
  }

  fprintf(htrFile, "\n[BasePosition]\n");
  fprintf(htrFile, "#SegmentName\tTx\tTy\tTx\tRx\tRy\tRz\tBoneLength\n");
  for (i=0; i<basePosition.size(); i++)
    fprintf(htrFile, "%s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", basePosition[i]->name,
            basePosition[i]->Tx, basePosition[i]->Ty, basePosition[i]->Tz,
	    basePosition[i]->Rx, basePosition[i]->Ry, basePosition[i]->Rz,
	    basePosition[i]->boneLength);

  fprintf(htrFile, "\n");
  for (int j=0; j<numSegments; j++){
    fprintf(htrFile, "[%s]\n", segmentData[j]->name);
    fprintf(htrFile, "#Fr\tTx\tTy\tTz\tRx\tRy\tRz\tSF\n");
    for (i=0; i<segmentData[j]->frame.size(); i++){
      fprintf(htrFile, "%i\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
	      segmentData[j]->frame[i]->frameNum, segmentData[j]->frame[i]->Tx,
	      segmentData[j]->frame[i]->Ty, segmentData[j]->frame[i]->Tz,
	      segmentData[j]->frame[i]->Rx, segmentData[j]->frame[i]->Ry,
	      segmentData[j]->frame[i]->Rz, segmentData[j]->frame[i]->scale);
    }
  }

  fprintf(htrFile, "\n[EndOfFile]\n");
  fclose(htrFile);
  return true;
}

