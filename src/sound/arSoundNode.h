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

  // State.  Ignored by some subclasses.
  arVector3 _point;
  float _amplitude; // Usually [0, 100], default 1.

  // Generic array of data, either floats or ints.
  // 'Arguments' to the 'command' _commandID, for some subclasses.
  arLightFloatBuffer _commandBuffer;

  // State.  Neither allocated nor stored here, though.
  arSoundFile** _arSoundFiles; //;; was _texture. ?rename to _inheritedGlobalData.  Inherited from parent, if any.
  arSoundFile*  _localSoundFile; //;; was _localTexture. ?rename to _globalDatum

};

  // arSoundTransformNode has fixed size.
  // Other arXXXNodes store arrays of records.

#endif
