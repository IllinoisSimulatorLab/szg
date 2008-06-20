//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_OBJECT_H
#define AR_OBJECT_H

#include <stdio.h>
#include <iostream>
#include "arMath.h"
#include "arDataUtilities.h"
#include "arGraphicsDatabase.h"
#include <string>
#include <vector>

// The base class for syzygy Objects (i.e., things that are somehow drawn, not in the sense of "object-oriented")
/** This class is a (mostly virtual) representation for foreign objects.
 *  External or imported objects in Syzygy derive from this class, like
 *  Geometry and Animation data files.
 *  Derived classes should instantiate as many methods as are reasonable;
 *  otherwise a predefined invalid value is returned.
 *  Derived classes should use members _name and _invalidFile.
 *  todo: derive (virtual) arGeometryObject and arAnimationObject.
*/

class SZG_CALL arObject {
 public:
  arObject() : _name(""), _invalidFile(false), _vertexNodeID(-1) {}
  virtual ~arObject() {}

  virtual inline int numberOfTriangles() {return -1;}
  virtual inline int numberOfNormals() {return -1;}
  virtual inline int numberOfVertices() {return -1;}
  virtual inline int numberOfMaterials() {return -1;}
  virtual inline int numberOfTexCoords() {return -1;}
  virtual inline int numberOfSmoothingGroups() {return -1;}
  virtual inline int numberOfGroups() {return -1;}

  // name, as specified by read/attach function
  string name() { return _name; }
  string setName(const string& newName) { return (_name = newName); }

  // Deprecated.
  // Annoyingly, this must be repeated in each subclass (because of the
  // *other* attachMesh virtual method).
  virtual bool attachMesh(const string& objectName, const string& parentName);

  // Add a valid arObject file to the scenegraph.
  virtual bool attachMesh(arGraphicsNode* parent, const string& objectName="") = 0;

  // kind of Object
  virtual string type(void) const = 0;

  // supports basic animation commands?
  virtual bool supportsAnimation(void) const = 0;
  // number of frames in animation
  virtual inline int	numberOfFrames() const { return -1; }
  virtual inline int	currentFrame() const { return -1; }
  // jump to this frame in the animation
  // if newFrame is out-of-bounds, return false and do nothing
  virtual bool	setFrame(int) {return false;}
  // jump to the next frame, or if already at end, return false
  virtual bool	nextFrame() { return false; }
  // jump to previous frame, or if already at start, return false
  virtual bool	prevFrame() { return false; }

  // Resize model to fit in a sphere of radius 1, when called before attachMesh().
  virtual void normalizeModelSize() = 0;

  // Return arDatabaseNode ID where object's vertices exist, or -1 if undefined
  int vertexNodeID() { return _vertexNodeID; }

 protected:
  string _name;
  bool	_invalidFile;	// file could not be found/used
  int	_vertexNodeID;	// node ID of vertices/object, if any
};

#endif
