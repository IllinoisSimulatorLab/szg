//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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

/// The base class for syzygy Objects (i.e., things that are somehow drawn, not in the sense of "object-oriented")
/** This class is a (mostly virtual) representation for foreign objects.
 *  External or imported objects in Syzygy derive from this class, like
 *  Geometry and Animation data files.
 *  Derived classes should instantiate as many methods as are reasonable;
 *  otherwise a predefined invalid value is returned.
 *  Derived classes should use members _name and _invalidFile.
 * \todo perhaps make (virtual) arGeometryObject, arAnimationObject, deriving from arObject
*/
class arObject {
 public:
  arObject() : _name(""), _invalidFile(false), _vertexNodeID(-1) {}
  virtual ~arObject() {}

  /// returns the number of triangles, or -1 if undefined
  virtual inline int numberOfTriangles() {return -1;}
  /// returns the number of normals, or -1 if undefined
  virtual inline int numberOfNormals() {return -1;}
  /// returns the number of vertices, or -1 if undefined
  virtual inline int numberOfVertices() {return -1;}
  /// returns the number of materials, or -1 if undefined
  virtual inline int numberOfMaterials() {return -1;}
  /// returns the number of texture coordinates, or -1 if undefined
  virtual inline int numberOfTexCoords() {return -1;}
  /// returns the number of smoothing groups in the (OBJ)arObject, or -1 if undefined
  virtual inline int numberOfSmoothingGroups() {return -1;}
  /// returns the number of groups, or -1 if undefined
  virtual inline int numberOfGroups() {return -1;}

  /// returns the name of the arObject (as specified by read/attach function)
  const string& name() { return _name; }
  /// sets and returns the new name
  /// @param newName the new name
  const string & setName(const string& newName) { return (_name = newName); }

  /// @param objectName The name of the object
  /// @param parent Where to attach the object in the scenegraph
  /// This takes a valid arObject file and attaches it to the scenegraph
  virtual void attachMesh(const string& objectName, const string& parent) = 0;
  virtual void attachMesh(const string& objectName, arGraphicsNode* parent) = 0;
  /// automagically inserts the object name as baseName
  virtual inline void attachMesh(const string& parent) { attachMesh(parent, _name); }
  
  /// returns what kind of Object this is
  virtual inline string type(void) = 0;

  /// does this object support the basic animation commands?
  virtual bool supportsAnimation(void) = 0;
  /// number of frames in animation
  virtual inline int	numberOfFrames() { return -1; }
  /// returns current frame number, or -1 if not implemented
  virtual inline int	currentFrame() { return -1; }
  /// go to this frame in the animation
  /// \param newFrame frame to jump to
  /// if newFrame is out-of-bounds, return false and do nothing
  virtual bool	setFrame(int) {return false;}
  /// go to the next frame, or if already at end, return false
  virtual bool	nextFrame() { return false; }
  /// go to previous frame, or if already at start, return false
  virtual bool	prevFrame() { return false; }

  /// Resizes the model to fit in a sphere of radius 1
  /// Note: MUST be called before attachMesh()!
  virtual void normalizeModelSize() = 0;

  /// Returns arDatabaseNode ID where object's vertices exist, or -1 if undefined
  int vertexNodeID () { return _vertexNodeID; } 


 protected:
  string _name;		//< name of object
  bool	_invalidFile;	//< true iff the file could not be found/used
  int	_vertexNodeID;	//< node ID of vertices/object, if any

};

#endif
