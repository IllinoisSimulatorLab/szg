//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arOBJ.h"
#include "arGraphicsAPI.h"

/// Adds triangle (as index) to the group
/// @param newTriangle (index of) triangle to add
void arOBJSmoothingGroup::add(int newTriangle)
{
  _triangles.push_back(newTriangle);
}

