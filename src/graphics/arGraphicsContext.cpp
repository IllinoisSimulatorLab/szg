//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsContext.h"

arGraphicsContext::arGraphicsContext(){
}

arGraphicsContext::~arGraphicsContext(){
}

void arGraphicsContext::pushNode(arDatabaseNode* node){
  int nodeType = node->getTypeCode();
  switch(nodeType){
  case AR_G_POINTS_NODE: 
    _pointsStack.push_front(node);
    break;
  case AR_G_BLEND_NODE:
    _blendStack.push_front(node);
    break;
  case AR_G_NORMAL3_NODE:
    _normal3Stack.push_front(node);
    break;
  case AR_G_COLOR4_NODE:
    _color4Stack.push_front(node);
    break;
  case AR_G_TEX2_NODE:
    _tex2Stack.push_front(node);
    break;
  case AR_G_INDEX_NODE:
    _indexStack.push_front(node);
    break;
  case AR_G_MATERIAL_NODE:
    _materialStack.push_front(node);
    break;
  case AR_G_TEXTURE_NODE:
    _textureStack.push_front(node);
    break;
  case AR_G_BUMP_MAP_NODE:
    _bumpMapStack.push_front(node);
    break;
  default:
    break;
  }
}
  
void arGraphicsContext::popNode(int nodeType){
  switch(nodeType){
  case AR_G_POINTS_NODE: 
    _pointsStack.pop_front();
    break;
  case AR_G_BLEND_NODE:
    _blendStack.pop_front();
    break;
  case AR_G_NORMAL3_NODE:
    _normal3Stack.pop_front();
    break;
  case AR_G_COLOR4_NODE:
    _color4Stack.pop_front();
    break;
  case AR_G_TEX2_NODE:
    _tex2Stack.pop_front();
    break;
  case AR_G_INDEX_NODE:
    _indexStack.pop_front();
    break;
  case AR_G_MATERIAL_NODE:
    _materialStack.pop_front();
    break;
  case AR_G_TEXTURE_NODE:
    _textureStack.pop_front();
    break;
  case AR_G_BUMP_MAP_NODE:
    _bumpMapStack.pop_front();
    break;
  }
}

arDatabaseNode* arGraphicsContext::getNode(int nodeType){
  arDatabaseNode* result = NULL;
  switch(nodeType){
  case AR_G_POINTS_NODE: 
    if (!_pointsStack.empty()){
      result = _pointsStack.front();
    }
    break;
  case AR_G_BLEND_NODE:
    if (!_blendStack.empty()){
      result = _blendStack.front();
    }
    break;
  case AR_G_NORMAL3_NODE:
    if (!_normal3Stack.empty()){
      result = _normal3Stack.front();
    }
    break;
  case AR_G_COLOR4_NODE:
    if (!_color4Stack.empty()){
      result = _color4Stack.front();
    }
    break;
  case AR_G_TEX2_NODE:
    if (!_tex2Stack.empty()){
      result = _tex2Stack.front();
    }
    break;
  case AR_G_INDEX_NODE:
    if (!_indexStack.empty()){
      result = _indexStack.front();
    }
    break;
  case AR_G_MATERIAL_NODE:
    if (!_materialStack.empty()){
      result = _materialStack.front();
    }
    break;
  case AR_G_TEXTURE_NODE:
    if (!_textureStack.empty()){
      result = _textureStack.front();
    }
    break;
  case AR_G_BUMP_MAP_NODE:
    if (!_bumpMapStack.empty()){
      result = _bumpMapStack.front();
    }
    break;
  }
  return result;
}

void arGraphicsContext::clear(){
  _pointsStack.clear();
  _blendStack.clear();
  _normal3Stack.clear();
  _color4Stack.clear();
  _tex2Stack.clear();
  _indexStack.clear();
  _materialStack.clear();
  _textureStack.clear();
  _bumpMapStack.clear();
}

