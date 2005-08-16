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

bool arDrawableNode::_01DPreDraw(arGraphicsNode* pointsNode,
                                 arGraphicsNode* blendNode,
                                 arGraphicsNode* materialNode){
  if (!pointsNode){
    return false;
  }

  glDisable(GL_LIGHTING);
  glColor4f(1,1,1,1);
  float blendFactor = 1;
  if (blendNode){
    blendFactor *= (blendNode->getBuffer())[0];
  }
  if (materialNode){
    arMaterialNode* mn = (arMaterialNode*) materialNode;
    arMaterial* m = mn->getMaterialPtr();
    const arVector4 temp(m->diffuse[0], m->diffuse[1], 
                         m->diffuse[2], m->alpha*blendFactor);
    glColor4fv(temp.v);
  }
  if (blendFactor < 1.0){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  return true;
}

void arDrawableNode::_01DPostDraw(arGraphicsNode* blendNode){
  if (blendNode && (blendNode->getBuffer())[0] < 1.0){
    glDisable(GL_BLEND);
  }
}

// Consigning CG to the dustbin of history.
//#ifdef USE_CG
//extern CGcontext myContext;
//extern void cgErrorCallback(void);
//#endif

bool arDrawableNode::_2DPreDraw(arGraphicsNode* pointsNode,
                                arGraphicsNode* normal3Node,
                                arGraphicsNode* blendNode,
                                arGraphicsNode* materialNode,
                                arGraphicsNode* textureNode){

  arMaterialNode* mn = (arMaterialNode*) materialNode;
  arTextureNode* tn = (arTextureNode*) textureNode;

  // GRUMBLE. This function essentially MAXIMIZES OpenGL state changes before
  // a new drawable node. It seems like a little bit of extra machinery might
  // result in something better (i.e. we might be keeping track of state
  // changes).

  // some data is necessary to draw triangles
  if (!pointsNode){
    cerr << "arDrawableNode error: missing points set for 2D geometry.\n";
    return false;
  }
  if (!normal3Node){
    cerr << "arDrawableNode error: missing normals set for 2D geometry.\n";
    return false;
  }
  // OK the following block gives the overall configuration that I'm
  // using... these are a little arbitrary
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
  glEnable(GL_LIGHTING);
  glEnable(GL_NORMALIZE);
  glColorMaterial(GL_FRONT_AND_BACK,GL_DIFFUSE);
  glColor4f(1,1,1,1);

  // Determine if blending has been requested.
  float blendFactor = 1.0;
  if (blendNode){
    blendFactor *= (blendNode->getBuffer())[0];
  }
  
  if (materialNode) {
    // We actually have a material
    arMaterial* m = mn->getMaterialPtr();
    arVector4 temp(m->diffuse[0], m->diffuse[1], 
                   m->diffuse[2], m->alpha*blendFactor);
    // Since we are enabling GL_COLOR_MATERIAL, this needs to happen as well
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

  if (blendFactor < 1.0){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  glEnable(GL_COLOR_MATERIAL);

  // CG is being consigned to the dustbin of history.
  //#ifdef USE_CG
  //  if (*_bumpMap)	// bump map overrides texture call
  //   (*_bumpMap)->activate();
  //  else
  //#endif
  if (textureNode){
    arTexture* t = tn->getTexture();
    if (t){
      t->activate();
    }
  }
  
  return true;
}

void arDrawableNode::_2DPostDraw(arGraphicsNode* blendNode,
                                 arGraphicsNode* textureNode){

  // CG is being consigned to the dustbin of history.
  //#ifdef USE_CG
  //if (*_bumpMap)
  //  (*_bumpMap)->deactivate();
  //else
  //#endif
  arTextureNode* tn = (arTextureNode*) textureNode;
  if (textureNode){
    arTexture* t = tn->getTexture();
    if (t){
      t->deactivate();
    }
  }
  
  if (blendNode && (blendNode->getBuffer())[0] < 1.0){
    glDisable(GL_BLEND);
  }
  
  glDisable(GL_LIGHTING);
}

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
  arGraphicsNode* bNode = NULL;
  arGraphicsNode* iNode = NULL;
  arGraphicsNode* pNode = NULL;
  arGraphicsNode* nNode = NULL;
  arGraphicsNode* cNode = NULL;
  arGraphicsNode* tNode = NULL;
  arGraphicsNode* mNode = NULL;
  arGraphicsNode* t2Node = NULL;
  if (context){
    bNode = (arGraphicsNode*) context->getNode(AR_G_BLEND_NODE);
    iNode = (arGraphicsNode*) context->getNode(AR_G_INDEX_NODE);
    pNode = (arGraphicsNode*) context->getNode(AR_G_POINTS_NODE);
    nNode = (arGraphicsNode*) context->getNode(AR_G_NORMAL3_NODE);
    cNode = (arGraphicsNode*) context->getNode(AR_G_COLOR4_NODE);
    tNode = (arGraphicsNode*) context->getNode(AR_G_TEXTURE_NODE);
    mNode = (arGraphicsNode*) context->getNode(AR_G_MATERIAL_NODE);
    t2Node = (arGraphicsNode*) context->getNode(AR_G_TEX2_NODE);
  }

  // AARGH! this code adds blending but it is repeated in the post/pre draws
  float blendFactor = 1.0;
  if (bNode){
    blendFactor *= (bNode->getBuffer())[0];    
  }
  switch (whatKind) {
  case DG_POINTS:
    if (_01DPreDraw(pNode, bNode, mNode)){
      ar_drawPoints(howMany,
		    (int*) (iNode ? iNode->getBuffer() : NULL),
		    (float*) (pNode ? pNode->getBuffer() : NULL),
		    (float*) (cNode ? cNode->getBuffer(): NULL), blendFactor);
      _01DPostDraw(bNode);
    }
    break;
  case DG_LINES:
    if (_01DPreDraw(pNode, bNode, mNode)){
      ar_drawLines(howMany,
		   (int*) (iNode ? iNode->getBuffer() : NULL),
		   (float*) (pNode ? pNode->getBuffer() : NULL),
		   (float*) (cNode ? cNode->getBuffer() : NULL), blendFactor);
      _01DPostDraw(bNode);
    }
    break;
  case DG_LINE_STRIP:
    if (_01DPreDraw(pNode, bNode, mNode)){
      ar_drawLineStrip(howMany,
		       (int*) (iNode ? iNode->getBuffer() : NULL),
		       (float*) (pNode ? pNode->getBuffer() : NULL),
		       (float*) (cNode ? cNode->getBuffer() : NULL), blendFactor);
      _01DPostDraw(bNode);
    }
    break;
  case DG_TRIANGLES:
    if (_2DPreDraw(pNode, nNode, bNode, mNode, tNode)){
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
      _2DPostDraw(bNode, tNode);
    }
    break;
  case DG_TRIANGLE_STRIP:
    if (_2DPreDraw(pNode, nNode, bNode, mNode, tNode)){
      ar_drawTriangleStrip(howMany, 
                           (int*) (iNode ? iNode->getBuffer() : NULL), 
                           (float*) (pNode ? pNode->getBuffer(): NULL), 
                           (float*) (nNode ? nNode->getBuffer() : NULL), 
                           (float*) (cNode ? cNode->getBuffer() : NULL), 
                           (float*) (t2Node ? t2Node->getBuffer() : NULL), blendFactor);
      _2DPostDraw(bNode, tNode);
    }
    break;
  case DG_QUADS:
    if (_2DPreDraw(pNode, nNode, bNode, mNode, tNode)){
      ar_drawQuads(howMany, 
                   (int*) (iNode ? iNode->getBuffer() : NULL), 
                   (float*) (pNode ? pNode->getBuffer() : NULL), 
                   (float*) (nNode ? nNode->getBuffer() : NULL), 
                   (float*) (cNode ? cNode->getBuffer() : NULL), 
                   (float*) (t2Node ? t2Node->getBuffer() : NULL), blendFactor);
      _2DPostDraw(bNode, tNode);
    }
    break;
  case DG_QUAD_STRIP:
    if (_2DPreDraw(pNode, nNode, bNode, mNode, tNode)){
      ar_drawQuadStrip(howMany, 
                       (int*) (iNode ? iNode->getBuffer() : NULL), 
                       (float*) (pNode ? pNode->getBuffer() : NULL), 
                       (float*) (nNode ? nNode->getBuffer() : NULL), 
                       (float*) (cNode ? cNode->getBuffer() : NULL), 
                       (float*) (t2Node ? t2Node->getBuffer() : NULL), blendFactor);
      _2DPostDraw(bNode, tNode);
    }
    break;
  case DG_POLYGON:
    if (_2DPreDraw(pNode, nNode, bNode, mNode, tNode)){
      ar_drawPolygon(howMany, 
                     (int*) (iNode ? iNode->getBuffer() : NULL), 
                     (float*) (pNode ? pNode->getBuffer() : NULL), 
                     (float*) (nNode ? nNode->getBuffer() : NULL), 
                     (float*) (cNode ? cNode->getBuffer() : NULL), 
                     (float*) (t2Node ? t2Node->getBuffer() : NULL), blendFactor);
      _2DPostDraw(bNode, tNode);
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
