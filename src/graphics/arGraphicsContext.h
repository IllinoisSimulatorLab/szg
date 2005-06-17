//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_CONTEXT_H
#define AR_GRAPHICS_CONTEXT_H

#include "arTexture.h"
#include "arBumpMap.h"
#include "arMaterial.h"
#include "arPointsNode.h"
#include "arBlendNode.h"
#include "arNormal3Node.h"
#include "arColor4Node.h"
#include "arTex2Node.h"
#include "arIndexNode.h"
#include "arMaterialNode.h"
#include "arTextureNode.h"
#include "arBumpMapNode.h"
#include <stack>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class arGraphicsDatabase;

/// Node in an arGraphicsDatabase.

class SZG_CALL arGraphicsContext{
 public:
  arGraphicsContext();
  virtual ~arGraphicsContext();

  void pushNode(arGraphicsNode* node);
  void popNode(int nodeType);
  arGraphicsNode* getNode(int nodeType);

  void clear();

 protected:
  stack<arPointsNode*>   _pointsStack;
  stack<arBlendNode*>    _blendStack;
  stack<arNormal3Node*>  _normal3Stack;
  stack<arColor4Node*>   _color4Stack;
  stack<arTex2Node*>     _tex2Stack;
  stack<arIndexNode*>    _indexStack;
  stack<arMaterialNode*> _materialStack;
  stack<arTextureNode*>  _textureStack;
  stack<arBumpMapNode*>  _bumpMapStack;
};

#endif
