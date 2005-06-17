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

void arGraphicsContext::pushNode(arGraphicsNode* node){
switch(nodeType){
  case AR_G_POINTS_NODE: 
    _pointsStack.pop();
    break;
  case AR_G_BLEND_NODE:
    _blendStack.pop();
    break;
  case AR_G_NORMAL3_NODE:
    _normal3Stack.pop();
    break;
  case AR_G_COLOR4_NODE:
    _color4Stack.pop();
    break;
  case AR_G_TEX2_NODE:
    _tex2Stack.pop();
    break;
  case AR_G_INDEX_NODE:
    _indexStack.pop();
    break;
  case AR_G_MATERIAL_NODE:
    _materialStack.pop();
    break;
  case AR_G_TEXTURE_NODE:
    _textureStack.pop();
    break;
  case AR_G_BUMP_MAP_NODE:
    _bumpMapStack.pop();
    break;
  }
}
  
void arGraphicsContext::popNode(int nodeType){
  switch(nodeType){
  case AR_G_POINTS_NODE: 
    _pointsStack.pop();
    break;
  case AR_G_BLEND_NODE:
    _blendStack.pop();
    break;
  case AR_G_NORMAL3_NODE:
    _normal3Stack.pop();
    break;
  case AR_G_COLOR4_NODE:
    _color4Stack.pop();
    break;
  case AR_G_TEX2_NODE:
    _tex2Stack.pop();
    break;
  case AR_G_INDEX_NODE:
    _indexStack.pop();
    break;
  case AR_G_MATERIAL_NODE:
    _materialStack.pop();
    break;
  case AR_G_TEXTURE_NODE:
    _textureStack.pop();
    break;
  case AR_G_BUMP_MAP_NODE:
    _bumpMapStack.pop();
    break;
  }
}

arGraphicsNode* arGraphicsContext::getNode(int nodeType){
  arGraphicsNode* result = NULL;
  switch(nodeType){
  case AR_G_POINTS_NODE: 
    if (!_pointsStack.empty()){
      result = _pointsStack.top();
    }
    break;
  case AR_G_BLEND_NODE:
    if (!_blendStack.empty()){
      result = _blendStack.top();
    }
    break;
  case AR_G_NORMAL3_NODE:
    if (!_normal3Stack.empty()){
      result = _normal3Stack.top();
    }
    break;
  case AR_G_COLOR4_NODE:
    if (!_color4Stack.empty()){
      result = _color4Stack.top();
    }
    break;
  case AR_G_TEX2_NODE:
    if (!_tex2Stack.empty()){
      result = _tex2Stack.top();
    }
    break;
  case AR_G_INDEX_NODE:
    if (!_indexStack.empty()){
      result = _indexStack.top();
    }
    break;
  case AR_G_MATERIAL_NODE:
    if (!_materialStack.empty()){
      result = _materialStack.top();
    }
    break;
  case AR_G_TEXTURE_NODE:
    if (!_textureStack.empty()){
      result = _textureStack.top();
    }
    break;
  case AR_G_BUMP_MAP_NODE:
    if (!_bumpMapStack.empty()){
      result = _bumpMapStack.top();
    }
    break;
  }
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

