//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <iostream>
#include "arGraphicsNode.h"
#include "arGraphicsDatabase.h"

arGraphicsNode::arGraphicsNode() :
  _owningDatabase(NULL),
  // DEFUNCT
  //_points(NULL),
  //_blend(NULL),
  //_normal3(NULL),
  //_color(NULL),
  //_tex2(NULL),
  //_index(NULL),
  //_material(NULL),
  //_texture(&_localTexture),
  _localTexture(NULL)
  //_bumpMap(&_localBumpMap),
  //_localBumpMap(NULL)
{
}

arGraphicsNode::~arGraphicsNode(){
  // Why don't we need to delete all the pointer members here?
  // ... Note that the node itself does not necessarily own these
  // pointers... consequently, it should not delete them! But...
  // it should delete it's local storage as indicated by _commandBuffer,
  // even here, though, one needs to be careful. what if you delete
  // the local storage and another node tries to use it?
}

void arGraphicsNode::initialize(arDatabase* owner){
  // AARGH! There are two different pointers to the same thing... DOH!
  _databaseOwner = owner;
  _owningDatabase = (arGraphicsDatabase*) owner;
  // we want to share the database's language among all the
  // various nodes... otherwise a scene graph with too many nodes
  // uses *much* too much memory
  // AARGH! There are also two different pointers to the language...
  _g = &(_owningDatabase->_gfx);
  _dLang = &(_owningDatabase->_gfx);
  // very important that we do not do the following if the parent node
  // is the root node... as this node is an arDatabaseNode but not
  // an arGraphicsNode
  arGraphicsNode* parent = (arGraphicsNode*) _parent;
  // THIS IS A BIG PROBLEM! THE WAY INHERITANCE WORKS ASSUMES THAT
  // ALL NODES EXCEPT THE ROOT NODE ARE arGraphicsNodes!
  // And, also, the inheritance model sucks since it doesn't allow inserts!
  // DEFUNCT
  /*if (parent && parent->getName() != "root" 
      && parent->getTypeString() != "name"){
    _points = parent->_points;
    _blend = parent->_blend;
    _normal3 = parent->_normal3;
    _color = parent->_color;
    _tex2 = parent->_tex2;
    _index = parent->_index;
    _material = parent->_material;
    _texture = parent->_texture;
    _bumpMap = parent->_bumpMap;
    }*/
}

arMatrix4 arGraphicsNode::accumulateTransform(){
  arMatrix4 result;
  arGraphicsNode* g = (arGraphicsNode*) getParent();
  if (g){
    _accumulateTransform(g, result);
  }
  return result;
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
