//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_API_H
#define AR_GRAPHICS_API_H

#include "arGraphicsDatabase.h"

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

/** @todo: this language isn't really
 *  very well designed... ideally there would be (XYZ) coordinate
 *  arrays, ID arrays, tex coord arrays, etc... but these would be
 *  be used differently by a tri-strip drawable or a tri soup drawable
 *  or a line drawable... for instance. what I have here dangerously
 *  conflates the coord or color or whatever arrays with the drawable type.
 */

//;;;; used only in arGraphicsAPI.cpp?  private to it?  or use these elsewhere?
const int AR_FLOATS_PER_MATRIX = 16;
const int AR_FLOATS_PER_POINT = 3;
const int AR_FLOATS_PER_NORMAL = 3;
const int AR_FLOATS_PER_TEXCOORD = 2;
const int AR_FLOATS_PER_COLOR = 4;

void dgSetGraphicsDatabase(arGraphicsDatabase*);

string dgGetNodeName(int);

arGraphicsNode* dgGetNode(const string& nodeName);

arDatabaseNode* dgMakeNode(const string&, const string&, const string&);

bool dgViewer(const arMatrix4&, const arVector3&, const arVector3&, 
              float, float, float, float);

int dgTransform(const string&, const string&, const arMatrix4&);
bool dgTransform(int, const arMatrix4&);

int dgPoints(const string&, const string&, int, int*, float*);
bool dgPoints(int, int, int*, float*);
int dgPoints(const string& name, const string& parent, int numPoints, 
             float* positions);
bool dgPoints(int ID, int numPoints, float* positions);

int dgTexture(const string& name, const string& parent,
              const string& filename, int alphaValue=-1);
bool dgTexture(int, const string& filename, int alphaValue=-1);

int dgTexture(const string& name, const string& parent,
               bool alpha, int w, int h, const char* pixels);
bool dgTexture(int ID, bool alpha, int w, int h, const char* pixels);

int  dgBoundingSphere(const string&, const string&, int, 
                      float, const arVector3&);
bool dgBoundingSphere(int, int, float, const arVector3&);

bool dgErase(const string&);

int dgBillboard(const string&, const string&, int, const string&);
bool dgBillboard(int, int, const string&);

int dgVisibility(const string&, const string&, int);
bool dgVisibility(int, int);

int dgBlend(const string&, const string&, float);
bool dgBlend(int, float);

int dgNormal3(const string& name, const string& parent, int numNormals,
	      int* IDs, float* normals);
bool dgNormal3(int ID, int numNormals, int* IDs, float* normals);
int dgNormal3(const string& name, const string& parent, int numNormals,
	      float* normals);
bool dgNormal3(int ID, int numNormals, float* normals);

int dgColor4(const string& name, const string& parent, int numColors,
	      int* IDs, float* colors);
bool dgColor4(int ID, int numColors, int* IDs, float* colors);
int dgColor4(const string& name, const string& parent, int numColors,
	      float* colors);
bool dgColor4(int ID, int numColors, float* colors);

int dgTex2(const string& name, const string& parent, int numTexcoords,
	   int* IDs, float* coords);
bool dgTex2(int ID, int numTexcoords, int* IDs, float* coords);
int dgTex2(const string& name, const string& parent, int numTexcoords,
	   float* coords);
bool dgTex2(int ID, int numTexcoords, float* coords);

int dgIndex(const string& name, const string& parent, int numIndices,
	    int* IDs, int* indices);
bool dgIndex(int ID, int numIndices, int* IDs, int* indices);
int dgIndex(const string& name, const string& parent, int numIndices,
	    int* indices);
bool dgIndex(int ID, int numIndices, int* indices);

int dgDrawable(const string& name, const string& parent,
	       int drawableType, int numPrimitives);
bool dgDrawable(int ID, int drawableType, int numPrimitives);

int dgMaterial(const string& name, const string& parent, 
               const arVector3& diffuse,
	       const arVector3& ambient = arVector3(0.2,0.2,0.2), 
               const arVector3& specular = arVector3(0,0,0), 
               const arVector3& emissive = arVector3(0,0,0),
	       float exponent = 0., 
               float alpha = 1.);
bool dgMaterial(int ID, const arVector3& diffuse,
	        const arVector3& ambient = arVector3(0.2,0.2,0.2), 
                const arVector3& specular = arVector3(0,0,0), 
                const arVector3& emissive = arVector3(0,0,0),
	        float exponent = 0., 
                float alpha = 1.);

int dgLight(const string& name, const string& parent,
	    int lightID, arVector4 position,
	    const arVector3& diffuse,
	    const arVector3& ambient = arVector3(0,0,0),
	    const arVector3& specular = arVector3(1,1,1),
	    const arVector3& attenuate = arVector3(1,0,0),
            const arVector3& spotDiection = arVector3(0,0,-1),
	    float spotCutoff = 180.,
	    float spotExponent = 0.);

bool dgLight(int ID,
	     int lightID, arVector4 position,
	     const arVector3& diffuse,
	     const arVector3& ambient = arVector3(0,0,0),
	     const arVector3& specular = arVector3(1,1,1),
	     const arVector3& attenuate = arVector3(1,0,0),
             const arVector3& spotDiection = arVector3(0,0,-1),
	     float spotCutoff = 180.,
	     float spotExponent = 0.);

/// Attach a perspective camera to the scene graph.
int dgCamera(const string& name, const string& parent,
	     int cameraID, float leftClip, float rightClip, 
	     float bottomClip, float topClip, float nearClip, float farClip,
             const arVector3& eyePosition = arVector3(0,0,0),
             const arVector3& centerPosition = arVector3(0,0,-1),
             const arVector3& upDirection = arVector3(0,1,0));

bool dgCamera(int ID,
	      int cameraID, float leftClip, float rightClip, 
	      float bottomClip, float topClip, float nearClip, float farClip,
              const arVector3& eyePosition = arVector3(0,0,0),
              const arVector3& centerPosition = arVector3(0,0,-1),
              const arVector3& upDirection = arVector3(0,1,0));


/// Attach a bump map node to the scene graph
int dgBumpMap(const string& name, const string& parent,
	      const string& filename, float height=1.);

int dgBumpMap(int ID, const string& filename, float height=1.);


#endif
