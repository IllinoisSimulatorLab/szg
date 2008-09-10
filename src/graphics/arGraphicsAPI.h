//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_API_H
#define AR_GRAPHICS_API_H

#include "arGraphicsDatabase.h"
#include "arGraphicsCalling.h"

// Naming conventions:
// arXXX refers to classes in the library.
// Important executables like the render client
// should have names starting with SZG.
// Global functions like those below should start with a
// unique two char identifier.
// dg = distributed graphics
// ds = distributed sound

// Some commands create/modify nodes, others only modify nodes.
// Node creation commands return -1 on error and otherwise the node ID.
// Node modification commands return false on error and true otherwise.

//;;;; used only in arGraphicsAPI.cpp?  private to it?  or use these elsewhere?
const int AR_FLOATS_PER_MATRIX = 16;
const int AR_FLOATS_PER_POINT = 3;
const int AR_FLOATS_PER_NORMAL = 3;
const int AR_FLOATS_PER_TEXCOORD = 2;
const int AR_FLOATS_PER_COLOR = 4;

SZG_CALL void dgSetGraphicsDatabase(arGraphicsDatabase*);

SZG_CALL string dgGetNodeName(int);

SZG_CALL arGraphicsNode* dgGetNode(const string& nodeName);

SZG_CALL arDatabaseNode* dgMakeNode(const string&, const string&,
                                    const string&);

SZG_CALL int  dgViewer( const string& parent, const arHead& head);
SZG_CALL bool dgViewer( int ID, const arHead& head );

SZG_CALL int dgTransform(const string&, const string&, const arMatrix4&);
SZG_CALL bool dgTransform(int, const arMatrix4&);

SZG_CALL int dgPoints(const string& name, const string& parent, int numPoints, int* IDs, float* positions);
SZG_CALL bool dgPoints(int ID, int numPoints, int* IDs, float* positions);
SZG_CALL int dgPoints(const string& name, const string& parent, int numPoints,
                      float* positions);
SZG_CALL bool dgPoints(int ID, int numPoints, float* positions);

SZG_CALL int dgTexture(const string& name, const string& parent,
                       const string& filename, int alphaValue=-1);
SZG_CALL bool dgTexture(int, const string& filename, int alphaValue=-1);

SZG_CALL int dgTexture(const string& name, const string& parent,
                       bool alpha, int w, int h, const char* pixels);
SZG_CALL bool dgTexture(int ID, bool alpha, int w, int h, const char* pixels);

SZG_CALL int  dgBoundingSphere(const string&, const string&, int,
                      float, const arVector3&);
SZG_CALL bool dgBoundingSphere(int, int, float, const arVector3&);

SZG_CALL bool dgErase(const string&);

SZG_CALL int dgBillboard(const string&, const string&, int, const string&);
SZG_CALL bool dgBillboard(int, int, const string&);

SZG_CALL int dgVisibility(const string&, const string&, int);
SZG_CALL bool dgVisibility(int, int);

SZG_CALL int dgBlend(const string&, const string&, float);
SZG_CALL bool dgBlend(int, float);

SZG_CALL int dgStateInt(const string& nodeName, const string& parentName,
                        const string& stateName,
                        arGraphicsStateValue val1,
                        arGraphicsStateValue val2 = AR_G_FALSE );
SZG_CALL bool dgStateInt(int nodeID, const string& stateName,
    arGraphicsStateValue val1, arGraphicsStateValue val2 = AR_G_FALSE);

SZG_CALL int dgStateFloat(const string& nodeName, const string& parentName,
                          const string& stateName, float value );
SZG_CALL bool dgStateFloat(int nodeID, const string& stateName, float value);

SZG_CALL int dgNormal3(const string& name, const string& parent,
                       int numNormals,
                       int* IDs, float* normals);
SZG_CALL bool dgNormal3(int ID, int numNormals, int* IDs, float* normals);
SZG_CALL int dgNormal3(const string& name, const string& parent,
                       int numNormals,
                       float* normals);
SZG_CALL bool dgNormal3(int ID, int numNormals, float* normals);

SZG_CALL int dgColor4(const string& name, const string& parent, int numColors,
                      int* IDs, float* colors);
SZG_CALL bool dgColor4(int ID, int numColors, int* IDs, float* colors);
SZG_CALL int dgColor4(const string& name, const string& parent, int numColors,
                      float* colors);
SZG_CALL bool dgColor4(int ID, int numColors, float* colors);

SZG_CALL int dgTex2(const string& name, const string& parent, int numTexcoords,
                    int* IDs, float* coords);
SZG_CALL bool dgTex2(int ID, int numTexcoords, int* IDs, float* coords);
SZG_CALL int dgTex2(const string& name, const string& parent, int numTexcoords,
                    float* coords);
SZG_CALL bool dgTex2(int ID, int numTexcoords, float* coords);

SZG_CALL int dgIndex(const string& name, const string& parent, int numIndices,
                     int* IDs, int* indices);
SZG_CALL bool dgIndex(int ID, int numIndices, int* IDs, int* indices);
SZG_CALL int dgIndex(const string& name, const string& parent, int numIndices,
                     int* indices);
SZG_CALL bool dgIndex(int ID, int numIndices, int* indices);

SZG_CALL int dgDrawable(const string& name, const string& parent,
                        int drawableType, int numPrimitives);
SZG_CALL bool dgDrawable(int ID, int drawableType, int numPrimitives);

SZG_CALL int dgMaterial(const string& name, const string& parent,
                        const arVector3& diffuse,
                        const arVector3& ambient = arVector3(0.2, 0.2, 0.2),
                        const arVector3& specular = arVector3(0, 0, 0),
                        const arVector3& emissive = arVector3(0, 0, 0),
                        float exponent = 0.,
                        float alpha = 1.);
SZG_CALL bool dgMaterial(int ID, const arVector3& diffuse,
                         const arVector3& ambient = arVector3(0.2, 0.2, 0.2),
                         const arVector3& specular = arVector3(0, 0, 0),
                         const arVector3& emissive = arVector3(0, 0, 0),
                         float exponent = 0.,
                         float alpha = 1.);

SZG_CALL int dgLight(const string& name, const string& parent,
                     int lightID, arVector4 position,
                     const arVector3& diffuse,
                     const arVector3& ambient = arVector3(0, 0, 0),
                     const arVector3& specular = arVector3(1, 1, 1),
                     const arVector3& attenuate = arVector3(1, 0, 0),
                     const arVector3& spotDiection = arVector3(0, 0, -1),
                     float spotCutoff = 180.,
                     float spotExponent = 0.);

SZG_CALL bool dgLight(int ID,
                      int lightID, arVector4 position,
                      const arVector3& diffuse,
                      const arVector3& ambient = arVector3(0, 0, 0),
                      const arVector3& specular = arVector3(1, 1, 1),
                      const arVector3& attenuate = arVector3(1, 0, 0),
                      const arVector3& spotDiection = arVector3(0, 0, -1),
                      float spotCutoff = 180.,
                      float spotExponent = 0.);

// Attach a perspective camera to the scene graph.
SZG_CALL int dgCamera(const string& name, const string& parent,
                      int cameraID, float leftClip, float rightClip,
                      float bottomClip, float topClip,
                      float nearClip, float farClip,
                      const arVector3& eyePosition = arVector3(0, 0, 0),
                      const arVector3& centerPosition = arVector3(0, 0, -1),
                      const arVector3& upDirection = arVector3(0, 1, 0));

SZG_CALL bool dgCamera(int ID,
                       int cameraID, float leftClip, float rightClip,
                       float bottomClip, float topClip,
                       float nearClip, float farClip,
                       const arVector3& eyePosition = arVector3(0, 0, 0),
                       const arVector3& centerPosition = arVector3(0, 0, -1),
                       const arVector3& upDirection = arVector3(0, 1, 0));

// Attach a bump map node to the scene graph
SZG_CALL int dgBumpMap(const string& name, const string& parent,
                       const string& filename, float height=1.);

SZG_CALL bool dgBumpMap(int ID, const string& filename, float height=1.);

SZG_CALL int dgPlugin(const string& name, const string& parent, const string& fileName,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

SZG_CALL bool dgPlugin(int ID, const string& fileName,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

SZG_CALL int dgPlugin(const string& name,
              const string& parent,
              const string& fileName,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );

SZG_CALL bool dgPlugin( int ID, const string& fileName,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );


int SZG_CALL dgPython(const string& name,
              const string& parent,
              const string& moduleName,
              const string& factoryName,
              bool reloadModule,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

bool SZG_CALL dgPython( int ID, const string& moduleName,
               const string& factoryName,
               bool reloadModule,
               int* intData, int numInts,
               float* floatData, int numFloats,
               long* longData, int numLongs,
               double* doubleData, int numDoubles,
               std::vector< std::string >* stringData );

int SZG_CALL dgPython(const string& name,
              const string& parent,
              const string& moduleName,
              const string& factoryName,
              bool reloadModule,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );

bool SZG_CALL dgPython( int ID, const string& moduleName,
               const string& factoryName,
               bool reloadModule,
               std::vector<int>& intData,
               std::vector<float>& floatData,
               std::vector<long>& longData,
               std::vector<double>& doubleData,
               std::vector< std::string >& stringData );


// Higher-leve versions to simplify SIP Python bindings.
//
SZG_CALL int dgPoints(const string& name, const string& parent, vector<int>& ids, vector<arVector3>& positions);
SZG_CALL bool dgPoints(int ID, vector<int>& IDs, vector<arVector3>& positions);
SZG_CALL int dgPoints(const string& name, const string& parent, vector<arVector3>&);
SZG_CALL bool dgPoints(int ID, vector<arVector3>&);

SZG_CALL int dgNormal3(const string& name, const string& parent, vector<int>& IDs, vector<arVector3>& normals);
SZG_CALL bool dgNormal3(int ID, vector<int>& IDs, vector<arVector3>& normals);
SZG_CALL int dgNormal3(const string& name, const string& parent, vector<arVector3>& normals);
SZG_CALL bool dgNormal3(int ID, vector<arVector3>& normals);

SZG_CALL int dgColor4(const string& name, const string& parent, vector<int>& IDs, vector<arVector4>& colors);
SZG_CALL bool dgColor4(int ID, vector<int>& IDs, vector<arVector4>& colors);
SZG_CALL int dgColor4(const string& name, const string& parent, vector<arVector4>& colors);
SZG_CALL bool dgColor4(int ID, vector<arVector4>& colors);

SZG_CALL int dgTex2(const string& name, const string& parent, vector<int>& IDs, vector<arVector2>& coords);
SZG_CALL bool dgTex2(int ID, vector<int>& IDs, vector<arVector2>& coords);
SZG_CALL int dgTex2(const string& name, const string& parent, vector<arVector2>& coords);
SZG_CALL bool dgTex2(int ID, vector<arVector2>& coords);

SZG_CALL int dgIndex(const string& name, const string& parent, vector<int>& IDs, vector<int>& indices);
SZG_CALL bool dgIndex(int ID, vector<int>& IDs, vector<int>& indices);
SZG_CALL int dgIndex(const string& name, const string& parent, vector<int>& indices);
SZG_CALL bool dgIndex(int ID, vector<int>& indices);

#endif
