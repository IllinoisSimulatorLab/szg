//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_NODE_H
#define AR_SOUND_NODE_H

#include "arSoundFile.h"
#include "arLightFloatBuffer.h"
#include "arSoundLanguage.h"
#include "arDatabaseNode.h"
#include "arMath.h"
#include "arSoundCalling.h"

class arSoundDatabase;

// Node in the scene graph for sound.

class SZG_CALL arSoundNode : public arDatabaseNode {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  friend class arSoundDatabase;
  arSoundNode();
  virtual ~arSoundNode() {}

  virtual void initialize(arDatabase*);
  virtual bool receiveData(arStructuredData*) = 0;
  virtual bool render() = 0;
  virtual arStructuredData* dumpData() = 0;
  bool isServer() const;
  bool isClient() const { return !isServer(); }

 protected:
  arSoundDatabase* _owningDatabase;
  arSoundLanguage  _l;
  
  // State.
  arVector3 _point;
  float _amplitude; // usually [0,100], default 1.
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

  // arSoundTransformNode has fixed size.
  // Other arXXXNodes store arrays of records.

#endif
