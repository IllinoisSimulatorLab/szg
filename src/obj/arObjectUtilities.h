#ifndef __AR_OBJECT_UTILITIES_H
#define __AR_OBJECT_UTILITIES_H

//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#include "arObject.h"
#include "arHTR.h"
#include "arOBJ.h"
#include "ar3DS.h"
#include "arGraphicsAPI.h"

bool attachOBJToHTRToNodeInDatabase(arOBJ* theOBJ,
                                    arHTR *theHTR,
				    const string &theNode);

arObject* arReadObjectFromFile(const char *fileName, const string& path);
/*
/// \param numVerts Number of vertices
/// \param vertices Array of vertices as 3 packed floats
/// \param normals Normals per vertex as 3 packed floats
/// \param texCoords Texture coordinates per vertex as 2 packed floats
/// @param numFaces How many faces in index list (or zero if vertices et. al.
///                 are in consecutive order)
/// @param index Array of indices into other arrays, every 3 ints representing
///              exactly one triangle, or NULL if in consecutive order
/// \param tangent3 (output) Pointer to array populated with per-vertex tangents
/// \param binormal3 (output) Pointer to array populated with per-vertex binormals
bool arGenerateLocalFrame(int numVerts, float* vertices, // input
                          float *normals, float *texCoords,
                          int numFaces, int *indices,
                          float* tangent3, float* binormal3); // output
*/

#endif
