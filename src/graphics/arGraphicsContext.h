//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_CONTEXT_H
#define AR_GRAPHICS_CONTEXT_H

#include "arDatabaseNode.h"
#include "arGraphicsHeader.h"
#include "arGraphicsWindow.h"
#include "arViewport.h"
#include "arGraphicsCalling.h"

#include <list>

// Information maintained during the traversal of a scene graph.

class SZG_CALL arGraphicsContext {
 public:
  arGraphicsContext( arGraphicsWindow* win=NULL, arViewport* view=NULL );
  virtual ~arGraphicsContext();

  void pushNode(arDatabaseNode* node);
  void popNode(arDatabaseNode* node);
  arDatabaseNode* getNode(int nodeType);

  void clear();

  void setPointState(float& blendFactor);
  void setLineState(float& blendFactor);
  void setTriangleState(float& blendFactor);

  arGraphicsWindow* getWindow() { return _graphicsWindow; }
  arViewport* getViewport() { return _viewport; }

 protected:
  arGraphicsWindow* _graphicsWindow;
  arViewport*       _viewport;
  list<arDatabaseNode*>   _pointsStack;
  list<arDatabaseNode*>    _blendStack;
  list<arDatabaseNode*>  _normal3Stack;
  list<arDatabaseNode*>   _color4Stack;
  list<arDatabaseNode*>     _tex2Stack;
  list<arDatabaseNode*>    _indexStack;
  list<arDatabaseNode*> _materialStack;
  list<arDatabaseNode*>  _textureStack;
  list<arDatabaseNode*>  _bumpMapStack;

  list<float>                _pointSizeStateStack;
  list<float>                _lineWidthStateStack;
  list<arGraphicsStateValue> _shadeModelStateStack;
  list<bool>                 _lightingStateStack;
  list<bool>                 _depthTestStateStack;
  list<bool>                 _blendStateStack;
  list<pair<arGraphicsStateValue, arGraphicsStateValue> > _blendFuncStateStack;

  void _pushGraphicsState(arDatabaseNode* node);
  void _popGraphicsState(arDatabaseNode* node);
  GLenum _decodeBlendFunction(arGraphicsStateValue v);
  void _setState(float& blendFactor, bool forceBlend = false);

};

#endif
