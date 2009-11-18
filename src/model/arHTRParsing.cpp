//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arHTR.h"
#include "arLogStream.h"
#include <string>
using namespace std;

const int MAXLINE  = 600;
const int MAXTOKENS  = 50;

arHTR::arHTR() :
  fileType(NULL),
  dataType(NULL),
  calibrationUnits(NULL),
  rotationUnits(NULL),
  _currentFrame(0),
  _normCenter(arVector3(0, 0, 0)),
  _normScaleAmount(1) {
}

arHTR::~arHTR() {
  if (fileType)
    free(fileType);
  if (dataType)
    free(dataType);
  if (calibrationUnits)
    free(calibrationUnits);
  if (rotationUnits)
    free(rotationUnits);
}

// marks this HTR file as invalid
bool arHTR::setInvalid(void) {
  _invalidFile = true;
  return false;
}

// Wrapper for the 3 parameter version.
bool arHTR::readHTR(const string& fileName, const string& path) {
  return readHTR(fileName, "", path);
}

// Reads HTR file specified
// @param fileName name of the HTR file, including extension
// @param path for some reason, a path string
// Actually checks that it exists and calls readHTR(FILE*)
bool arHTR::readHTR(const string& fileName, const string& subdirectory, const string& path) {
  FILE* htrFileHandle = ar_fileOpen(fileName, subdirectory, path, "r", "arHTR");
  return htrFileHandle ? readHTR(htrFileHandle) : setInvalid();
}

// reads HTR file specified
// @param htrFileHandle HTR file to read in
bool arHTR::readHTR(FILE* htrFileHandle) {
  if (!htrFileHandle) {
    ar_log_error() << "arHTR: invalid file pointer.\n";
    return setInvalid();
  }

  if (!parseHeader(htrFileHandle) ||
      !parseHierarchy(htrFileHandle) ||
      !parseBasePosition(htrFileHandle) ||
      !parseSegmentData(htrFileHandle)) {
    return setInvalid();
  }
  fclose(htrFileHandle);
  return precomputeData() || setInvalid();
}

// parses one line of the file, returning false if error/incorrect data on line
// @param theFile the file we're reading
// @param theResult pointers to c-strings containing the data
// @param buffer holds the current line to read from
// @param desiredTokens how many arguments we're expecting on this line --
//        determines whether we pass back true or false
// @param errorString the argument to write out in case of an error --
//        very useful for finding syntax errors in files
bool parseLine(FILE* theFile, char* theResult[], char* buffer, int desiredTokens, string errorString) {
  bool found = false;
  char* value = (char *)1;
  int numTokens = 0;

  while (!found && value) {
    value = fgets(buffer, MAXLINE, theFile);
    if (buffer[0] == '\n' || buffer[0] == '#')
      // Skip comment or newline.
      found = false;
    else if (value)
      found = true;
  }
  if (!found) {
    ar_log_error() << "arHTR: premature end of file.\n";
    return false;
  }

  // Parse Line. Note how we need to add the '\r' to acomodate reading Win32
  // text files on Unix.
  theResult[numTokens++] = strtok(buffer, " \t\n\r");
  while (theResult[numTokens-1]) {
    theResult[numTokens++] = strtok(NULL, " \t\n\r");
  }
  if (--numTokens < desiredTokens) {
    ar_log_error() << "arHTR: invalid or missing parameter '" << errorString <<
      "'.\n  Found '" <<  string(theResult[0]) << "' in line '" << buffer << "'.\n";
    return false;
  }

  return true;
}

bool arHTR::parseHeader(FILE* htrFileHandle) {
  bool found = false;
  char textLine[MAXLINE];
  char *token[MAXTOKENS];

  while (!found && fgets(textLine, MAXLINE, htrFileHandle)) {
    if (!strncmp("[Header]", textLine, strlen("[Header]")))
      found = true;
  }
  if (!found) {
    ar_log_error() << "arHTR: no [Header].\n";
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
    ar_log_error() << "arHTR: unexpected rotation order '" << token[1] << "'\n";

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
  if (!parseLine(htrFileHandle, token, textLine, 2, "scaleFactor")) {
    scaleFactor = .1;
    rewind(htrFileHandle);
  }
  else
    scaleFactor = 0.1*atof(token[1]);
  return true;
}

// parses [SegmentNames&Hierarchy] section of .htr file
// @param htrFileHandle the HTR file
// Will read from pointer to end of file until it finds
// "[SegmentNames&Hierarchy]"... or doesn't
bool arHTR::parseHierarchy(FILE *htrFileHandle) {
  bool found = false;
  char textLine[MAXLINE];
  char *token[MAXTOKENS];
  while (!found && fgets(textLine, MAXLINE, htrFileHandle)) {
    if (!strncmp("[SegmentNames&Hierarchy]", textLine, strlen("[SegmentNames&Hierarchy]")))
      found = true;
  }
  if (!found) {
    ar_log_error() << "arHTR: no [SegmentNames&Hierarchy].\n";
    return false;
  }

  for (int j=0; j<numSegments; j++) {
    if (!parseLine(htrFileHandle, token, textLine, 2, "SegmentNames&Hierarchy"))
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

// parses [BasePosition] section of .htr file
// @param htrFileHandle the HTR file
// Will read from pointer to end of file until it finds
// "[BasePosition]"... or doesn't
bool arHTR::parseBasePosition(FILE *htrFileHandle) {
  bool found = false;
  char textLine[MAXLINE];
  char *token[MAXTOKENS];

  while (!found && fgets(textLine, MAXLINE, htrFileHandle)) {
    if (!strncmp("[BasePosition]", textLine, strlen("[BasePosition]")))
      found = true;
  }
  if (!found) {
    ar_log_error() << "arHTR: no [BasePosition].\n";
    return false;
  }

  for (int i=0; i<numSegments; i++) {
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
  }
  return true;
}

// Parses the segment data for a .htr file.
bool arHTR::parseSegmentData1(FILE* htrFileHandle) {
  char* value = NULL;
  char textLine[MAXLINE] = {0};
  char *token[MAXTOKENS] = {0};
  htrSegmentData *newSegmentData = NULL;
  htrFrame *newFrame = NULL;
  for (int i=0; i<numSegments; ++i) {
    newSegmentData = new htrSegmentData;
    // todo: test for out of memory
    bool found = false;
    value = (char *)1;
    while (!found && value) {
      value = fgets(textLine, MAXLINE, htrFileHandle);
      if (textLine[0] == '\n' || textLine[0] == '#')
        // Skip comment or newline.
        found = false;
      else if (value)
        found = true;
    }
    if (!found) {
      ar_log_error() << "arHTR: premature end of file.\n";
      return false;
    }

    char* genStr = strrchr(textLine, ']');
    if (genStr) {
      genStr[0] = '\0';
    }
    newSegmentData->segmentName = string(textLine+1);
    for (int j=0; j<numFrames; j++) {
      if (!parseLine(htrFileHandle, token, textLine, 8, "SegmentData"))
        return false;

      newFrame = new htrFrame;
      newFrame->frameNum = atoi(token[0]);
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

// Parse the segment data for a .htr2 file. This file type's format
// differs from the .htr data:
// it is streamable, instead of placing all
// of one segment's frames together.
bool arHTR::parseSegmentData2(FILE* htrFileHandle) {
  char* value = NULL;
  char textLine[MAXLINE] = {0};
  char *token[MAXTOKENS] = {0};
  htrSegmentData *newSegmentData = NULL;
  htrFrame *newFrame = NULL;
  // Create storage for each segment's data.
  // Store values for all frames in an htrSegmentData object.
  for (int j=0; j<numSegments; j++) {
    newSegmentData = new htrSegmentData();
    htrSegmentHierarchy* segment = childParent[j];
    newSegmentData->segmentName = string(segment->child);
    segmentData.push_back(newSegmentData);
  }
  for (int i=0; i<numFrames; i++) {
    newSegmentData = new htrSegmentData;
    bool found = false;
    value = (char *)1;
    while (!found && value) {
      value = fgets(textLine, MAXLINE, htrFileHandle);
      if (textLine[0] == '\n' || textLine[0] == '#' || textLine[0] == '\r') {
        // Comment or newline.
        continue;
      }
      if (value)
        found = true;
    }
    if (!found) {
      ar_log_error() << "arHTR: premature end of file.\n";
      return false;
    }

    const string check("Frame " + ar_intToString(i) + ":");
    if (false && textLine != check) {
      ar_log_error() << "arHTR: no marker for frame " << i << " (" << textLine << ").\n";
      return false;
    }
    // Found marker "Frame n:" for start of frame data.

    for (int k=0; k<numSegments; k++) {
      if (!parseLine(htrFileHandle, token, textLine, 8, "SegmentData"))
        return false;

      newFrame = new htrFrame;
      newFrame->frameNum = i;
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

// Parse the frame data of .htr file
// @param htrFileHandle the HTR file
bool arHTR::parseSegmentData(FILE* htrFileHandle) {
  if (fileVersion == 1) {
    return parseSegmentData1(htrFileHandle);
  }
  if (fileVersion == 2) {
    return parseSegmentData2(htrFileHandle);
  }
  ar_log_error() << "arHTR: unhandled HTR file version " << fileVersion << ".\n";
  return false;
}

// fills in the data structures for easy, efficient access
bool arHTR::precomputeData(void) {
  unsigned int k=0, l=0;
  int i=0, j=0;

  // connecting parents and children
  for (k=0; k<childParent.size(); k++)
    for (j=0; j<numSegments; j++)
      if (!strcmp(childParent[k]->child, segmentData[j]->segmentName.c_str()))
        for (i=0; i<numSegments; i++)
          if (!strcmp(childParent[k]->parent, segmentData[i]->segmentName.c_str())) {
            segmentData[j]->parent = segmentData[i];
            segmentData[i]->children.push_back(segmentData[j]);
            break;
          }
  // connect segment data and base position
  for (k=0; k<segmentData.size(); k++)
    for (l=0; l<basePosition.size(); l++)
      if (!strcmp(basePosition[l]->name, segmentData[k]->segmentName.c_str())) {
        basePosition[l]->segment = segmentData[k];
        segmentData[k]->basePosition = basePosition[l];
      }
  // precompute basePosition transform matrices
  htrBasePosition *b;
  for (k=0; k<basePosition.size(); k++) {
    b = basePosition[k];
    b->trans = ar_translationMatrix(b->Tx, b->Ty, b->Tz);
    b->rot   = HTRRotation(b->Rx, b->Ry, b->Rz);
  }
  // precompute matrices for frames
  htrFrame *h;
  for (k=0; k<segmentData.size(); k++)
    for (l=0; l<segmentData[k]->frame.size(); l++) {
      h = segmentData[k]->frame[l];
      h->trans = HTRTransform(segmentData[k]->basePosition, h);
      h->totalScale = segmentData[k]->basePosition->boneLength * h->scale;
    }
  return true;
}

// writes out current class data to an HTR file
// @param fileName name of new OBJ file (including extension) to write out
bool arHTR::writeToFile(const string& fileName) {
  FILE *htrFile = fopen(fileName.c_str(), "w");
  if (!htrFile) {
    ar_log_error() << "arHTR failed to write to file '" << fileName << "'.\n";
    return false;
  }

  fprintf(htrFile, "# Created by Syzygy HTR component.\n[Header]\n");
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

  fprintf(htrFile, "\n[SegmentNames&Hierarchy]\n#CHILD\tPARENT\n");
  unsigned i;
  for (i=0; i<childParent.size(); i++) {
    fprintf(htrFile, "%s\t%s\n", childParent[i]->child, childParent[i]->parent);
  }

  fprintf(htrFile, "\n[BasePosition]\n#SegmentName\tTx\tTy\tTx\tRx\tRy\tRz\tBoneLength\n");
  for (i=0; i<basePosition.size(); i++)
    fprintf(htrFile, "%s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", basePosition[i]->name,
            basePosition[i]->Tx, basePosition[i]->Ty, basePosition[i]->Tz,
            basePosition[i]->Rx, basePosition[i]->Ry, basePosition[i]->Rz,
            basePosition[i]->boneLength);

  fprintf(htrFile, "\n");
  for (int j=0; j<numSegments; j++) {
    fprintf(htrFile, "[%s]\n", segmentData[j]->segmentName.c_str());
    fprintf(htrFile, "#Fr\tTx\tTy\tTz\tRx\tRy\tRz\tSF\n");
    for (i=0; i<segmentData[j]->frame.size(); i++) {
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
