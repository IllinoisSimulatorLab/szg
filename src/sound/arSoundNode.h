//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_NODE_H
#define AR_SOUND_NODE_H

#include "arSoundFile.h"
#include "arLightFloatBuffer.h"
#include "arSoundLanguage.h"
#include "arDatabaseNode.h"
#include "arMath.h"

class arSoundDatabase;

/// Node in the scene graph for sound.

class arSoundNode : public arDatabaseNode {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  friend class arSoundDatabase;
  arSoundNode();
  virtual ~arSoundNode();

  virtual void initialize(arDatabase*);
  virtual bool receiveData(arStructuredData*) = 0;
  virtual void render() = 0;
  virtual arStructuredData* dumpData() = 0;
  bool isServer() const;
  bool isClient() const { return !isServer(); }

 protected:
  arSoundDatabase* _owningDatabase;
  arSoundLanguage  _l;
  
  // State of this node.
  arVector3 _point;
  float _amplitude;
  // these 2 are perhaps ignored by some node-kinds.
  // used only by arSoundFileNode

  // Generic array of data, either floats or ints.
  // These are 'arguments' to the 'command' _commandID, for some kinds of node.
  arLightFloatBuffer _commandBuffer;

  // State referred to by (not actually stored in, nor allocated by) this node.
  // _texture is inherited from parent, if parent exists
  arSoundFile** _arSoundFiles; //;; was _texture. ?rename to _inheritedGlobalData
  arSoundFile*  _localSoundFile; //;; was _localTexture. ?rename to _globalDatum

};

  // arSoundTransformNode is an example of a node with fixed size.
  // Other arXXXNodes are examples of a node which stores an array of records.

#endif
