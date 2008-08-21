//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsNode.h"
#include "arGraphicsDatabase.h"

arGraphicsNode::arGraphicsNode() :
  _owningDatabase(NULL),
  _g(NULL)
{
}

arGraphicsNode::~arGraphicsNode() {
}

void arGraphicsNode::initialize(arDatabase* owner) {
  arDatabaseNode::initialize(owner);
  // Aliased pointer, to avoid casts.
  _owningDatabase = (arGraphicsDatabase*) owner;
  _g = &(_owningDatabase->_gfx);
}

// Return the accumulated transform ABOVE the current node,
// excluding the node's own transform if it's an arTransformNode.
//
// Not thread-safe w.r.t. _owningDatabase (not using the global arDatabase lock),
// because e.g. arGraphicsDatabase::activateLights() needs that unlocked.
// For thread safety, use arGraphicsDatabase::accumulateTransform().
arMatrix4 arGraphicsNode::accumulateTransform() {
  arMatrix4 r;
  const arGraphicsNode* n = (const arGraphicsNode*) getParent();
  if (n) {
    _accumulateTransform(n, r);
  }
  return r;
}

void arGraphicsNode::_accumulateTransform(const arGraphicsNode* n, arMatrix4& m) {
  if (n->getTypeCode() == AR_G_TRANSFORM_NODE) {
    m = ((arTransformNode*)n)->getTransform() * m;
  }
  const arGraphicsNode* p = (const arGraphicsNode*) n->getParent();
  if (p) {
    _accumulateTransform(p, m);
  }
}

arStructuredData* arGraphicsNode::_getRecord(const bool owned, const int id) {
  arStructuredData* r = owned ? getStorage(id) : _g->makeDataRecord(id);
  if (!r) {
    ar_log_error() << "arGraphicsNode got no record of type " <<
      _g->numstringFromID(id) << ".\n";
  }
  return r;
}
