//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsContext.h"

// The following includes cannot be in arGraphicsContext.h because of problems
// with recursive definition.
#include "arTextureNode.h"
#include "arGraphicsStateNode.h"
#include "arMaterialNode.h"
#include "arBlendNode.h"

arGraphicsContext::arGraphicsContext( arGraphicsWindow* win, arViewport* view ) :
  _graphicsWindow(win),
  _viewport(view) {
}

arGraphicsContext::~arGraphicsContext() {
}

void arGraphicsContext::pushNode(arDatabaseNode* node) {
  int nodeType = node->getTypeCode();
  switch(nodeType) {
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
  case AR_G_GRAPHICS_STATE_NODE:
    // There is quite a variation in what pushing graphics state does.
    _pushGraphicsState(node);
    break;
  default:
    break;
  }
}

void arGraphicsContext::popNode(arDatabaseNode* node) {
  int nodeType = node->getTypeCode();
  switch(nodeType) {
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
  case AR_G_GRAPHICS_STATE_NODE:
    _popGraphicsState(node);
  default:
    break;
  }
}

arDatabaseNode* arGraphicsContext::getNode(int nodeType) {
  arDatabaseNode* result = NULL;
  switch(nodeType) {
  case AR_G_POINTS_NODE:
    if (!_pointsStack.empty()) {
      result = _pointsStack.front();
    }
    break;
  case AR_G_BLEND_NODE:
    if (!_blendStack.empty()) {
      result = _blendStack.front();
    }
    break;
  case AR_G_NORMAL3_NODE:
    if (!_normal3Stack.empty()) {
      result = _normal3Stack.front();
    }
    break;
  case AR_G_COLOR4_NODE:
    if (!_color4Stack.empty()) {
      result = _color4Stack.front();
    }
    break;
  case AR_G_TEX2_NODE:
    if (!_tex2Stack.empty()) {
      result = _tex2Stack.front();
    }
    break;
  case AR_G_INDEX_NODE:
    if (!_indexStack.empty()) {
      result = _indexStack.front();
    }
    break;
  case AR_G_MATERIAL_NODE:
    if (!_materialStack.empty()) {
      result = _materialStack.front();
    }
    break;
  case AR_G_TEXTURE_NODE:
    if (!_textureStack.empty()) {
      result = _textureStack.front();
    }
    break;
  case AR_G_BUMP_MAP_NODE:
    if (!_bumpMapStack.empty()) {
      result = _bumpMapStack.front();
    }
    break;
  }
  return result;
}

void arGraphicsContext::clear() {
  _pointsStack.clear();
  _blendStack.clear();
  _normal3Stack.clear();
  _color4Stack.clear();
  _tex2Stack.clear();
  _indexStack.clear();
  _materialStack.clear();
  _textureStack.clear();
  _bumpMapStack.clear();

  _pointSizeStateStack.clear();
  _lineWidthStateStack.clear();
  _shadeModelStateStack.clear();
  _lightingStateStack.clear();
  _depthTestStateStack.clear();
  _blendStateStack.clear();
  _blendFuncStateStack.clear();
}

void arGraphicsContext::setPointState(float& blendFactor) {
  // Lighting: always disabled for points.
  // Shade model: irrelevant for points.
  // Texture mapping: always disabled for points.
  // Point size: Use value from stack.

  glDisable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_TEXTURE_2D);
  glPointSize(_pointSizeStateStack.empty() ? 1.0 : _pointSizeStateStack.front());
  // Set the common state (over the various types of primitives).
  _setState(blendFactor);
}

void arGraphicsContext::setLineState(float& blendFactor) {
  // Lighting: always disabled for lines.
  // Shade model: irrelevant for lines.
  // Texture mapping: always disabled for lines.
  // Line width: Use value from stack.

  glDisable(GL_LIGHTING);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_TEXTURE_2D);
  glLineWidth(_lineWidthStateStack.empty() ? 1.0 : _lineWidthStateStack.front());
  // Set the common state (over the various types of primitives).
  _setState(blendFactor);
}

void arGraphicsContext::setTriangleState(float& blendFactor) {
  // Lighting: Might be enabled or disabled.
  // Shade model: Can be either GL_FLAT or GL_SMOOTH
  // Texture mapping: Can be enabled or disabled, depending on the presence
  //                  of an ancestor texture node.

  // Lighting.
  glEnable(GL_NORMALIZE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  if (_lightingStateStack.empty() ||
      _lightingStateStack.front() != AR_G_FALSE) {
    glEnable(GL_LIGHTING);
  }
  else{
    glDisable(GL_LIGHTING);
  }

  // Shade model
  if (_shadeModelStateStack.empty() ||
      _shadeModelStateStack.front() != AR_G_FLAT) {
    glShadeModel(GL_SMOOTH);
  }
  else{
    glShadeModel(GL_FLAT);
  }

  // Texture.
  bool forceBlend = false;
  if (_textureStack.empty()) {
    glDisable(GL_TEXTURE_2D);
  }
  else{
    arTextureNode* tn = (arTextureNode*)_textureStack.front();
    arTexture* t = tn->getTexture();
    if (t) {
      if (t->getDepth() == 4) {
        forceBlend = true;
      }
      // Activating the texture also enables GL_TEXTURE_2D.
      t->activate();
    }
  }

  // Set the common state (over the various types of primitives).
  _setState(blendFactor, forceBlend);
}

void arGraphicsContext::_pushGraphicsState(arDatabaseNode* node) {
  arGraphicsStateNode* g = (arGraphicsStateNode*) node;
  arGraphicsStateValue v1, v2;
  float f;
  switch(g->getStateID()) {
  case AR_G_POINT_SIZE:
    if (g->getStateValueFloat(f)) {
      _pointSizeStateStack.push_front(f);
    }
    else{
      _pointSizeStateStack.push_front(1.0);
    }
    break;
  case AR_G_LINE_WIDTH:
    if (g->getStateValueFloat(f)) {
      _lineWidthStateStack.push_front(f);
    }
    else{
      _lineWidthStateStack.push_front(1.0);
    }
    break;
  case AR_G_SHADE_MODEL:
    g->getStateValuesInt(v1, v2);
    _shadeModelStateStack.push_front(v1);
    break;
  case AR_G_LIGHTING:
    g->getStateValuesInt(v1, v2);
    _lightingStateStack.push_front(v1);
    break;
  case AR_G_BLEND:
    g->getStateValuesInt(v1, v2);
    _blendStateStack.push_front(v1);
    break;
  case AR_G_DEPTH_TEST:
    g->getStateValuesInt(v1, v2);
    _depthTestStateStack.push_front(v1);
    break;
  case AR_G_BLEND_FUNC:
    g->getStateValuesInt(v1, v2);
    _blendFuncStateStack.push_front(make_pair<arGraphicsStateValue, arGraphicsStateValue>(v1, v2));
    break;
  default:
    break;
  }
}

void arGraphicsContext::_popGraphicsState(arDatabaseNode* node) {
arGraphicsStateNode* g = (arGraphicsStateNode*) node;
  switch(g->getStateID()) {
  case AR_G_POINT_SIZE:
    _pointSizeStateStack.pop_front();
    break;
  case AR_G_LINE_WIDTH:
    _lineWidthStateStack.pop_front();
    break;
  case AR_G_SHADE_MODEL:
    _shadeModelStateStack.pop_front();
    break;
  case AR_G_LIGHTING:
    _lightingStateStack.pop_front();
    break;
  case AR_G_BLEND:
    _blendStateStack.pop_front();
    break;
  case AR_G_DEPTH_TEST:
    _depthTestStateStack.pop_front();
    break;
  case AR_G_BLEND_FUNC:
    _blendFuncStateStack.pop_front();
    break;
  default:
    break;
  }
}

GLenum arGraphicsContext::_decodeBlendFunction(arGraphicsStateValue v) {
  switch(v) {
  case AR_G_ZERO:
    return GL_ZERO;
  case AR_G_ONE:
    return GL_ONE;
  case AR_G_DST_COLOR:
    return GL_DST_COLOR;
  case AR_G_SRC_COLOR:
    return GL_SRC_COLOR;
  case AR_G_ONE_MINUS_DST_COLOR:
    return GL_ONE_MINUS_DST_COLOR;
  case AR_G_ONE_MINUS_SRC_COLOR:
    return GL_ONE_MINUS_SRC_COLOR;
  case AR_G_SRC_ALPHA:
    return GL_SRC_ALPHA;
  case AR_G_ONE_MINUS_SRC_ALPHA:
    return GL_ONE_MINUS_SRC_ALPHA;
  case AR_G_DST_ALPHA:
    return GL_DST_ALPHA;
  case AR_G_ONE_MINUS_DST_ALPHA:
    return GL_ONE_MINUS_DST_ALPHA;
  case AR_G_SRC_ALPHA_SATURATE:
    return GL_SRC_ALPHA_SATURATE;
  default:
    return GL_ONE;
  }
}

void arGraphicsContext::_setState(float& blendFactor, bool forceBlend) {
  // 0, 1, 2 D primitives all have the following state setting code.
  // Blending, color (also materials), and depth testing.

  // Blend. Must come before color, since that uses the blendFactor we
  // compute here. Note how this computation is passed out to the caller.
  // There are two ways blending can be enabled: either we have a blend node
  // explicitly set to blend (value < 1.0) OR we are forcing blending
  // (which might happen if a texture has an alpha channel).
  if (_blendStack.empty() && !forceBlend) {
    glDisable(GL_BLEND);
  }
  else{
    // We can get here because forceBlend is true, so make sure that
    // the blendStack isn't empty.
    if (!_blendStack.empty()) {
      blendFactor = ((arBlendNode*)_blendStack.front())->getBlend();
    }
    if (blendFactor < 1.0 || forceBlend) {
      glEnable(GL_BLEND);
      // The blend functions need to get set now.
      if (_blendFuncStateStack.empty()) {
        // The default if nothing is specified.
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
      else{
        pair<arGraphicsStateValue, arGraphicsStateValue> p =
          _blendFuncStateStack.front();
        glBlendFunc(_decodeBlendFunction(p.first),
                    _decodeBlendFunction(p.second));
      }
    }
    else{
      // There is indeed a blend node... but it is set to non-transparent.
      glDisable(GL_BLEND);
    }
  }

  // Color.
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
  if (_materialStack.empty()) {
    glColor4f(1, 1, 1, 1);
  }
  else {
    arMaterialNode* mn = (arMaterialNode*) _materialStack.front();
    arMaterial* m = mn->getMaterialPtr();
    arVector4 temp(m->diffuse[0], m->diffuse[1],
                   m->diffuse[2], m->alpha*blendFactor);
    glColor4fv(temp.v);
    // Set the material normally also. Grumble.
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, temp.v);
    temp = arVector4(m->ambient[0], m->ambient[1],
                     m->ambient[2], m->alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, temp.v);
    temp = arVector4(m->specular[0], m->specular[1],
                     m->specular[2], m->alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, temp.v);
    temp = arVector4(m->emissive[0], m->emissive[1],
                     m->emissive[2], m->alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, temp.v);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, m->exponent);
  }

  // Depth test.
  if (_depthTestStateStack.empty() ||
      _depthTestStateStack.front() != AR_G_FALSE) {
    glEnable(GL_DEPTH_TEST);
  }
  else{
    glDisable(GL_DEPTH_TEST);
  }
}

