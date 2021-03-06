//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arSoundNode.h"
#include "arSoundDatabase.h"

arSoundNode::arSoundNode() :
  _point(0, 0, 0),
  _amplitude(1.),
  _arSoundFiles(&_localSoundFile),
  _localSoundFile(NULL)
{
}

void arSoundNode::initialize(arDatabase* owner) {
  _owningDatabase = (arSoundDatabase*) owner;
  arSoundNode* parent = (arSoundNode*)_parent;
  if (parent && parent->getName() != "root") {
    _arSoundFiles = parent->_arSoundFiles;
  }
}

bool arSoundNode::isServer() const {
  return _owningDatabase->isServer();
}
