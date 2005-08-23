//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDrawableNode.h"
#include "arMath.h"
#include "arGraphicsDatabase.h"
// Consigning CG to the dustbin of history.
//#ifdef USE_CG //Mark's Cg stuff
//#include <Cg/cgGL.h>
//#endif

arDrawableNode::arDrawableNode():
  _firstMessageReceived(false),
  _type(DG_POINTS),
  _number(0){

  // does not compile on RedHat 8 if these statements appear outside the
  // constructor body.
  _typeCode = AR_G_DRAWABLE_NODE;
  _typeString = "drawable";
}

/** @todo We consistently use the float buffer memory
 *  to store data in "int" format for this node.
 *  Instead, handle the types with an internal arStructuredData record.
 */

/// \todo Add errorchecking for howMany, so bad data doesn't segfault.
void arDrawableNode::draw(arGraphicsContext* context){
  // A PROBLEM! Currently, the database node is created with a message to
  // the database. Then, a message initializing it is sent. What if
  // we try to draw between these messages?
  // Well, bad things can happen in this case.
  if (!_firstMessageReceived){
    return;
  }

  const ARint whatKind = _type;
  const ARint howMany = _number;
  // Get the context.
  arGraphicsNode* iNode = NULL;
  arGraphicsNode* pNode = NULL;
  arGraphicsNode* nNode = NULL;
  arGraphicsNode* cNode = NULL;
  arGraphicsNode* tNode = NULL;
  arGraphicsNode* t2Node = NULL;
  if (context){
    iNode = (arGraphicsNode*) context->getNode(AR_G_INDEX_NODE);
    pNode = (arGraphicsNode*) context->getNode(AR_G_POINTS_NODE);
    nNode = (arGraphicsNode*) context->getNode(AR_G_NORMAL3_NODE);
    cNode = (arGraphicsNode*) context->getNode(AR_G_COLOR4_NODE);
    tNode = (arGraphicsNode*) context->getNode(AR_G_TEXTURE_NODE);
    t2Node = (arGraphicsNode*) context->getNode(AR_G_TEX2_NODE);
  }

  float blendFactor = 1.0;
  switch (whatKind) {
  case DG_POINTS:
    if (_0DPreDraw(pNode, context, blendFactor)){
      ar_drawPoints(howMany,
		    (int*) (iNode ? iNode->getBuffer() : NULL),
		    (float*) (pNode ? pNode->getBuffer() : NULL),
		    (float*) (cNode ? cNode->getBuffer(): NULL), blendFactor);
    }
    break;
  case DG_LINES:
    if (_1DPreDraw(pNode, context, blendFactor)){
      ar_drawLines(howMany,
		   (int*) (iNode ? iNode->getBuffer() : NULL),
		   (float*) (pNode ? pNode->getBuffer() : NULL),
		   (float*) (cNode ? cNode->getBuffer() : NULL), blendFactor);
    }
    break;
  case DG_LINE_STRIP:
    if (_1DPreDraw(pNode, context, blendFactor)){
      ar_drawLineStrip(howMany,
		       (int*) (iNode ? iNode->getBuffer() : NULL),
		       (float*) (pNode ? pNode->getBuffer() : NULL),
		       (float*) (cNode ? cNode->getBuffer() : NULL), 
                       blendFactor);
    }
    break;
  case DG_TRIANGLES:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)){
      // There used to be CG code in here... but no longer. That turned out
      // to be a failed experiment.
      ar_drawTriangles(howMany, 
                       (int*) (iNode ? iNode->getBuffer() : NULL), 
                       (float*) (pNode ? pNode->getBuffer() : NULL), 
                       (float*) (nNode ? nNode->getBuffer() : NULL), 
                       (float*) (cNode ? cNode->getBuffer() : NULL), 
                       (float*) (tNode && t2Node ? t2Node->getBuffer() : NULL),
                       blendFactor,
                       0, NULL, NULL);
      //(*_bumpMap) ? 3 : 0,
      //(CGparameter*) ((*_bumpMap) ? (*_bumpMap)->cgTBN() : NULL),
      //(float**) ((*_bumpMap) ? (*_bumpMap)->TBN() : NULL)
      //);
    }
    break;
  case DG_TRIANGLE_STRIP:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)){
      ar_drawTriangleStrip(howMany, 
                           (int*) (iNode ? iNode->getBuffer() : NULL), 
                           (float*) (pNode ? pNode->getBuffer(): NULL), 
                           (float*) (nNode ? nNode->getBuffer() : NULL), 
                           (float*) (cNode ? cNode->getBuffer() : NULL), 
                           (float*) (t2Node ? t2Node->getBuffer() : NULL), 
                           blendFactor);
    }
    break;
  case DG_QUADS:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)){
      ar_drawQuads(howMany, 
                   (int*) (iNode ? iNode->getBuffer() : NULL), 
                   (float*) (pNode ? pNode->getBuffer() : NULL), 
                   (float*) (nNode ? nNode->getBuffer() : NULL), 
                   (float*) (cNode ? cNode->getBuffer() : NULL), 
                   (float*) (t2Node ? t2Node->getBuffer() : NULL), 
                   blendFactor);
    }
    break;
  case DG_QUAD_STRIP:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)){
      ar_drawQuadStrip(howMany, 
                       (int*) (iNode ? iNode->getBuffer() : NULL), 
                       (float*) (pNode ? pNode->getBuffer() : NULL), 
                       (float*) (nNode ? nNode->getBuffer() : NULL), 
                       (float*) (cNode ? cNode->getBuffer() : NULL), 
                       (float*) (t2Node ? t2Node->getBuffer() : NULL), 
                       blendFactor);
    }
    break;
  case DG_POLYGON:
    if (_2DPreDraw(pNode, nNode, context, blendFactor)){
      ar_drawPolygon(howMany, 
                     (int*) (iNode ? iNode->getBuffer() : NULL), 
                     (float*) (pNode ? pNode->getBuffer() : NULL), 
                     (float*) (nNode ? nNode->getBuffer() : NULL), 
                     (float*) (cNode ? cNode->getBuffer() : NULL), 
                     (float*) (t2Node ? t2Node->getBuffer() : NULL), 
		     blendFactor);
    }
    break;
  default:
    cerr << "arDrawableNode warning: ignoring unexpected arDrawableType "
         << whatKind << ".\n";
    break;
  }
  // DEFUNCT
  // Leave this stuff for a little bit until we are REALLY sure that there
  // will be no severly bad consequences from the switch over...
  /*switch (whatKind) {
  case DG_POINTS:
    if (_01DPreDraw()){
      ar_drawPoints(howMany,
		    (int*) (_index ? _index->v : NULL),
		    (float*) (_points ? _points->v : NULL),
		    (float*) (_color ? _color->v : NULL), blendFactor);
      _01DPostDraw();
    }
    break;
  case DG_LINES:
    if (_01DPreDraw()){
      ar_drawLines(howMany,
		   (int*) (_index ? _index->v : NULL),
		   (float*) (_points ? _points->v : NULL),
		   (float*) (_color ? _color->v : NULL), blendFactor);
      _01DPostDraw();
    }
    break;
  case DG_LINE_STRIP:
    if (_01DPreDraw()){
      ar_drawLineStrip(howMany,
		       (int*) (_index ? _index->v : NULL),
		       (float*) (_points ? _points->v : NULL),
		       (float*) (_color ? _color->v : NULL), blendFactor);
      _01DPostDraw();
    }
    break;
  case DG_TRIANGLES:
    if (_2DPreDraw()){
      ar_drawTriangles(howMany, 
                       (int*) (_index ? _index->v : NULL), 
                       (float*) (_points ? _points->v : NULL), 
                       (float*) (_normal3 ? _normal3->v : NULL), 
                       (float*) (_color ? _color->v : NULL), 
                       (float*) (_texture && _tex2 ? _tex2->v : NULL), blendFactor,
		       (*_bumpMap) ? 3 : 0,
		       (CGparameter*) ((*_bumpMap) ? (*_bumpMap)->cgTBN() : NULL),
		       (float**) ((*_bumpMap) ? (*_bumpMap)->TBN() : NULL)
		       );
      _2DPostDraw();
    }
    break;
  case DG_TRIANGLE_STRIP:
    if (_2DPreDraw()){
      ar_drawTriangleStrip(howMany, 
                           (int*) (_index ? _index->v : NULL), 
                           (float*) (_points ? _points->v : NULL), 
                           (float*) (_normal3 ? _normal3->v : NULL), 
                           (float*) (_color ? _color->v : NULL), 
                           (float*) (_tex2 ? _tex2->v : NULL), blendFactor);
      _2DPostDraw();
    }
    break;
  case DG_QUADS:
    if (_2DPreDraw()){
      ar_drawQuads(howMany, 
                   (int*) (_index ? _index->v : NULL), 
                   (float*) (_points ? _points->v : NULL), 
                   (float*) (_normal3 ? _normal3->v : NULL), 
                   (float*) (_color ? _color->v : NULL), 
                   (float*) (_tex2 ? _tex2->v : NULL), blendFactor);
      _2DPostDraw();
    }
    break;
  case DG_QUAD_STRIP:
    if (_2DPreDraw()){
      ar_drawQuadStrip(howMany, 
                       (int*) (_index ? _index->v : NULL), 
                       (float*) (_points ? _points->v : NULL), 
                       (float*) (_normal3 ? _normal3->v : NULL), 
                       (float*) (_color ? _color->v : NULL), 
                       (float*) (_tex2 ? _tex2->v : NULL), blendFactor);
      _2DPostDraw();
    }
    break;
  case DG_POLYGON:
    if (_2DPreDraw()){
      ar_drawPolygon(howMany, 
                     (int*) (_index ? _index->v : NULL), 
                     (float*) (_points ? _points->v : NULL), 
                     (float*) (_normal3 ? _normal3->v : NULL), 
                     (float*) (_color ? _color->v : NULL), 
                     (float*) (_tex2 ? _tex2->v : NULL), blendFactor);
      _2DPostDraw();
    }
    break;
  default:
    cerr << "arDrawableNode warning: ignoring unexpected arDrawableType "
         << whatKind << ".\n";
    break;
    }*/
}

arStructuredData* arDrawableNode::dumpData(){
  return _dumpData(_type, _number);
}

bool arDrawableNode::receiveData(arStructuredData* inData){
  // Deals with the problem that the node is created and then initialized
  // in two different messages.
  _firstMessageReceived = true;

  if (inData->getID() != _g->AR_DRAWABLE){
    cerr << "arDrawableNode error: expected "
    << _g->AR_DRAWABLE
    << " (" << _g->_stringFromID(_g->AR_DRAWABLE) << ") not "
    << inData->getID()
    << " (" << _g->_stringFromID(inData->getID()) << ")\n";
    return false;
  }

  // NOTE: drawable node type changes must be atomic with respect
  // to node drawing... hmmm... a potential source of crashes!
  _type = inData->getDataInt(_g->AR_DRAWABLE_TYPE);
  _number = inData->getDataInt(_g->AR_DRAWABLE_NUMBER);
  return true;
}

int arDrawableNode::getType(){
  return _type;
}

int arDrawableNode::getNumber(){
  return _number;
}

void arDrawableNode::setDrawable(arDrawableType type, int number){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(type, number);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _type = type;
    _number = number;
  }
}

arStructuredData* arDrawableNode::_dumpData(int type, int number){
  arStructuredData* result = _g->makeDataRecord(_g->AR_DRAWABLE);
  _dumpGenericNode(result,_g->AR_DRAWABLE_ID);
  result->dataIn(_g->AR_DRAWABLE_TYPE, &type, AR_INT, 1);
  result->dataIn(_g->AR_DRAWABLE_NUMBER, &number, AR_INT, 1);
  return result;
}

bool arDrawableNode::_0DPreDraw(arGraphicsNode* pointsNode,
				arGraphicsContext* context,
                                float& blendFactor){
  if (!pointsNode){
    return false;
  }
  if (context){
    context->setPointState(blendFactor);
  }
  return true;
}

bool arDrawableNode::_1DPreDraw(arGraphicsNode* pointsNode,
                                arGraphicsContext* context,
                                float& blendFactor){
  if (!pointsNode){
    return false;
  }
  if (context){
    context->setLineState(blendFactor);
  }
  return true;
}

// Consigning CG to the dustbin of history.
//#ifdef USE_CG
//extern CGcontext myContext;
//extern void cgErrorCallback(void);
//#endif

bool arDrawableNode::_2DPreDraw(arGraphicsNode* pointsNode,
                                arGraphicsNode* normal3Node,
                                arGraphicsContext* context,
                                float& blendFactor){
  // Some data is necessary to draw triangles.
  if (!pointsNode){
    cerr << "arDrawableNode error: missing points set for 2D geometry.\n";
    return false;
  }
  if (!normal3Node){
    cerr << "arDrawableNode error: missing normals set for 2D geometry.\n";
    return false;
  }
  if (context){
    context->setTriangleState(blendFactor);
  }
  return true;
}
