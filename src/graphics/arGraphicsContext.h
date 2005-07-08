//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_CONTEXT_H
#define AR_GRAPHICS_CONTEXT_H

#include "arDatabaseNode.h"
#include "arGraphicsHeader.h"
#include <list>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"


/// Information maintained during the traversal of a scene graph.

class SZG_CALL arGraphicsContext{
 public:
  arGraphicsContext();
  virtual ~arGraphicsContext();

  void pushNode(arDatabaseNode* node);
  void popNode(int nodeType);
  arDatabaseNode* getNode(int nodeType);

  void clear();

 protected:
  list<arDatabaseNode*>   _pointsStack;
  list<arDatabaseNode*>    _blendStack;
  list<arDatabaseNode*>  _normal3Stack;
  list<arDatabaseNode*>   _color4Stack;
  list<arDatabaseNode*>     _tex2Stack;
  list<arDatabaseNode*>    _indexStack;
  list<arDatabaseNode*> _materialStack;
  list<arDatabaseNode*>  _textureStack;
  list<arDatabaseNode*>  _bumpMapStack;
};

#endif
