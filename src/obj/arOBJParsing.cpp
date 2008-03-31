//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arMath.h"
#include "arGraphicsDatabase.h"
#include "arDataUtilities.h"
#include "arOBJ.h"

#include <string>
#include <stdio.h>

// Parses one line of an ASCII OBJ file
// @param inputFile file pointer to next line to be read in
bool arOBJ::_parseOneLine(FILE* inputFile) {
  char buffer[1000] = {0};
  char* token[50] = {0};
  float x = -1.;
  float y = -1.;
  float z = -1.;
  unsigned i = 0;
  int numTokens = 0;

  int found = 0;
  char* value = (char *)1;
  while (!found && value) {
    value = fgets(buffer, sizeof(buffer)-1, inputFile);
    // Skip comment, newline, and linefeed.
    if (*buffer == '\n' || *buffer == '#' || *buffer == '\r')
      found = 0;
    else if (value)
      found = 1;
  }
  if (!found) { // End of file
    return false;
  }
  
  // Parse Line
  token[numTokens++] = strtok(buffer, " \t\r\n");
  while (token[numTokens-1])
    token[numTokens++] = strtok(NULL, " \t\r\n");
  --numTokens;

  string lineType(token[0]);

  ///// only one thing on line; bad format /////
  if (numTokens < 2)
    return true;

  ///// vn: vertex normal /////
  else if (lineType == "vn") {
    x = atof(token[1]);
    y = atof(token[2]);
    z = atof(token[3]);
    _normal.push_back(arVector3(x,y,z) / ++arVector3(x,y,z));
  }
  
  ///// vt: vertex texture coord /////
  else if (lineType == "vt") {
    x = atof(token[1]);
    y = atof(token[2]); 
    _texCoord.push_back(arVector3(x,y,0));
  }
  
  ///// vp: parametric vertex /////
  //else if (lineType == "vp") {
  
  ///// v: regular 3D vertex /////
  else if (lineType == "v") {
    x = atof(token[1]);
    y = atof(token[2]);
    z = atof(token[3]);
    _vertex.push_back(arVector3(x,y,z));
  }

  ///// f/fo: face /////
  else if (lineType == "f" || lineType == "fo") {
    _parseFace(numTokens, token);
  }

  ///// s: smoothing group /////
  else if (lineType == "s") {
    const int tempName = (!strcmp(token[1], "off")) ? 0 : atoi(token[1]);
    // "off" is equivalent to zero
    unsigned int iii;
    for (iii=0; iii<_smoothingGroup.size(); iii++) // sg exists?
      if (tempName == _smoothingGroup[iii]._name) {
        _thisSG = iii;
        break;
      }
    if (iii == _smoothingGroup.size()) { // new smoothing group
      _smoothingGroup.push_back(arOBJSmoothingGroup());
      _thisSG = _smoothingGroup.size()-1;
      _smoothingGroup[_thisSG]._name = tempName;
    }
  }

  ///// usemtl /////
  else if (lineType == "usemtl") {
    ar_log_debug() << "Found usemtl token '" << string(token[1]) << "'.\n";
    for (i=0; i<_material.size(); i++)
      if (!strcmp(token[1], _material[i].name)) {
        _thisMaterial = i;
        break;
      }
    if (i == _material.size()) // didn't find material
      _thisMaterial = 0;
    else if (_thisGroup) {
      for (i=0; i<_group[_thisGroup].size(); i++) {
        if (_triangle[_group[_thisGroup][i]].material == 0)
          _triangle[_group[_thisGroup][i]].material = _thisMaterial;
      }
    }
  }

  ///// usemap /////
  //else if (lineType == "usemap") {

  ///// mtllib /////
  else if (lineType == "mtllib") {
    ar_log_debug() << "Found mtllib tag.\n";
    string matFileName;
    if (_subdirectory == "" && _searchPath == "") {
      arPathString pathString(_fileName);
      if (pathString.size() <= 1) {
        // Absolute filename.
        matFileName = token[1];
      } else {
        // Search a path.
        const unsigned iMax = pathString.size() - 1;
        for (i=0; i<iMax; ++i) {
          matFileName += pathString[i] + ar_pathDelimiter();
        }
        matFileName += token[1];
      }
    }
    else{
      matFileName = token[1];
    }
    FILE* matFile = ar_fileOpen(matFileName,_subdirectory,_searchPath,"rb", "arOBJ mtllib");
    if (matFile) {
      ar_log_debug() << "Parsing mtl file '" << matFileName << "'.\n";
      while (_readMaterialsFromFile(matFile));
      _thisMaterial = 0;
      ar_log_debug() << "Parsed mtl file '" << matFileName << "'.\n";
    }
  }

  ///// g: group /////
  else if (lineType == "g") {
    for (i=0; i<_group.size(); i++) {
      if (_groupName[i] == string(token[1])) {
        _thisGroup = i;
        break;
      }
    }
    if (i == _group.size()) {
      _groupName.push_back(string(token[1]));
      _group.push_back(vector<int>());
      _thisGroup = i;
    }
    ar_log_debug() << "Found group token '" << string(token[1]) << "'.\n";
  }

  ///// o: object name /////
  else if (lineType == "o") {
    _name = string(token[1]);
    ar_log_debug() << "Found object name token '" << string(token[1]) << "'.\n";
  }

  ///// otherwise, ignore line /////
  return true;
}

// Reads in .mtl file conforming to OBJ spec
// @param matFile the .mtl file to read in
// Hides colors and textures of OBJ files in the difficult .mtl file format.
bool arOBJ::_readMaterialsFromFile(FILE* matFile) { 
  char buffer[1000] = {0};
  int typeChar = fgetc(matFile);

  switch (typeChar) {
  case EOF: // last line, and empty
    return false;

  case 'n':                   // newmtl
    {
    fseek(matFile,-1,SEEK_CUR);
    char mtlNameBuf[256] = {0};
    fscanf(matFile,"%s %s",buffer,mtlNameBuf);
    string mtlName(mtlNameBuf);
    if (!strcmp(buffer, "newmtl")) {
      ar_log_debug() << "Found newmtl token '" << mtlName << "'.\n";      
      if (mtlName.size() > 64) {
        ar_log_error() << "Material name '" << mtlName << "' is too long.\n";
        ar_log_error() << "arOBJ cannot handle material names longer than 64 characters.\n";
        return false;
      }
      unsigned int i;
      for (i=0; i<_material.size(); i++)
        if (strcmp(buffer, _material[i].name) == 0) {
          _thisMaterial = i;
          ar_log_debug() << "Material '" << mtlName << "' already exists.\n";      
          break;
        }
      if (i == _material.size()) {
        ar_log_debug() << "Adding new material '" << mtlName << "'.\n";      
        _material.push_back(arOBJMaterial());
        _thisMaterial = _material.size()-1;
        memcpy( _material[_thisMaterial].name, mtlName.c_str(), mtlName.size()+1 );
//        sprintf((_material[_thisMaterial]).name, "%s", mtlName);
      }
      ar_log_debug() << "Done handling newmtl token '" << string(mtlName) << "'.\n";      
    }
    break;
    }

  case 'K':
    typeChar=fgetc(matFile);
    switch (typeChar) {
      float x1, y1, z1;
      case 'a':      // ambient coeff
        fscanf(matFile,"%f %f %f",&x1, &y1, &z1);
        _material[_thisMaterial].Ka = arVector3(x1,y1,z1);
        break;
      case 'd':      // diffuse coeff (MAIN)
        fscanf(matFile,"%f %f %f",&x1, &y1, &z1);
        _material[_thisMaterial].Kd = arVector3(x1,y1,z1);
        break;
      case 's':      // specular coeff
        fscanf(matFile,"%f %f %f",&x1, &y1, &z1);
        _material[_thisMaterial].Ks = arVector3(x1,y1,z1);
        break;
      default:        // ignore line
        fgets(buffer,1000,matFile);
        break;
    }
    break;

  case 'm':
  case 'M':
    //fseek(matFile,-1,SEEK_CUR);
    char mapName[256];
    fscanf(matFile,"%s %s",buffer,mapName);
    if (!strcmp(buffer,"ap_Kd")) {
      ar_log_debug() << "OBJ texture image: " << (_material[_thisMaterial].map_Kd = string(mapName)) << ar_endl;
    } else if (!strcmp(buffer,"ap_Bump")) {
      ar_log_debug() << "OBJ texture image: " << (_material[_thisMaterial].map_Bump = string(mapName)) << ar_endl;
    }
    break;

  case '^':       // other platform linebreaks
  case '\r':      // carriage return
  case '\v':      // vertical tab
  case '\t':      // tab
  case '\n':      // empty line
    break;

  case '#':       // comment; ignore line
    fgets(buffer,1000,matFile);
    break;

  default:        // ignore line
  //  printf("(MTL file)Got char: %c\n", typeChar);
    fgets(buffer,1000,matFile);
    break;
  } // end case
  return true;
}
