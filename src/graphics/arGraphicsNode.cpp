//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsNode.h"
#include "arGraphicsDatabase.h"

arGraphicsNode::arGraphicsNode() :
  _owningDatabase(NULL)
{
}

arGraphicsNode::~arGraphicsNode(){
}

void arGraphicsNode::initialize(arDatabase* owner){
  arDatabaseNode::initialize(owner);
  // Aliased pointer, to avoid casts.
  _owningDatabase = (arGraphicsDatabase*) owner;
  _g = &(_owningDatabase->_gfx);
}

// Not thread-safe with respect to the owning database (since we aren't using
// the global lock). This feature is used, for instance, in
// arGraphicsDatabase::activateLights()... because there we depend on the
// fact that the global arDatabase lock is not used. If a thread-safe
// method is desired, use arGraphicsDatabase::accumulateTransform.
// NOTE: This gives the accumulated transform ABOVE the current node, and
// so does not include OUR transform if this is an arTransformNode.
arMatrix4 arGraphicsNode::accumulateTransform(){
  arMatrix4 r;
  arGraphicsNode* g = (arGraphicsNode*) getParent();
  if (g){
    _accumulateTransform(g, r);
  }
  return r;
}

void arGraphicsNode::_accumulateTransform(arGraphicsNode* g, arMatrix4& m){
  if (g->getTypeCode() == AR_G_TRANSFORM_NODE){
    arTransformNode* t = (arTransformNode*) g;
    m = t->getTransform()*m;
  }
  arGraphicsNode* p = (arGraphicsNode*) g->getParent();
  if (p){
    _accumulateTransform(p, m);
  }
}
