//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDrawableNode.h"
#include "arMath.h"
#include "arGraphicsDatabase.h"
#ifdef USE_CG //Mark's Cg stuff
#include <Cg/cgGL.h>
#endif

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

bool arDrawableNode::_01DPreDraw(){
  if (!_points)
    return false;

  glDisable(GL_LIGHTING);
  glColor4f(1,1,1,1);
  float blendFactor = 1;
  if (_blend){
    blendFactor *= _blend->v[0];
  }
  if (_material){
    const arVector4 temp(_material->diffuse[0], _material->diffuse[1], 
                         _material->diffuse[2], _material->alpha*blendFactor);
    glColor4fv(temp.v);
  }
  if (blendFactor < 1.0){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  return true;
}

void arDrawableNode::_01DPostDraw(){
  if (_blend && _blend->v[0] < 1.0){
    glDisable(GL_BLEND);
  }
}

#ifdef USE_CG
extern CGcontext myContext;
extern void cgErrorCallback(void);
#endif

bool arDrawableNode::_2DPreDraw(){
//	for (int i=0; i<5; ++i)
//		  printf("arDrawableNode::_2DPreDraw dump[%i]: %f\n", i, _index->v[i]);
  // some data is necessary to draw triangles
  if (!_points){
    cerr << "arDrawableNode error: missing points set for 2D geometry.\n";
    return false;
  }
  if (!_normal3){
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

  float blendFactor = 1.0;
  if (_blend){
    blendFactor *= _blend->v[0];
  }
  
  /// @todo Replace with a call to _material->activate() by changing
  ///       _material to actual arMaterial pointer, not floatbuffer.
  if (_material) {
    // we actually have a material
    arVector4 temp(_material->diffuse[0], _material->diffuse[1], 
                   _material->diffuse[2], _material->alpha*blendFactor);
    // since we enabling GL_COLOR_MATERIAL, this needs to happen as well
    glColor4fv(temp.v);
    // set the material normally also
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, temp.v);
    temp = arVector4(_material->ambient[0], _material->ambient[1], 
                     _material->ambient[2], _material->alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, temp.v);
    temp = arVector4(_material->specular[0], _material->specular[1], 
                     _material->specular[2], _material->alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, temp.v);
    temp = arVector4(_material->emissive[0], _material->emissive[1], 
                     _material->emissive[2], _material->alpha);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, temp.v);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, _material->exponent);
  }

  if (blendFactor < 1.0){
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  glEnable(GL_COLOR_MATERIAL);

#ifdef USE_CG
  if (*_bumpMap)	// bump map overrides texture call
    (*_bumpMap)->activate();
  else
#endif
    if (*_texture)
      (*_texture)->activate();
  
  return true;
}

void arDrawableNode::_2DPostDraw(){

#ifdef USE_CG
  if (*_bumpMap)
    (*_bumpMap)->deactivate();
  else
#endif
    if (*_texture)
      (*_texture)->deactivate();
  
  if (_blend && _blend->v[0] < 1.0)
    glDisable(GL_BLEND);
  
  glDisable(GL_LIGHTING);
}

/// \todo Add errorchecking for howMany, so bad data doesn't segfault.
void arDrawableNode::draw(){
  // A PROBLEM! Currently, the database node is created with a message to
  // the database. Then, a message initializing it is sent. What if
  // we try to draw between these messages?
  // Well, bad things can happen in this case.
  if (!_firstMessageReceived){
    return;
  }

  const ARint whatKind = _type;
  const ARint howMany = _number;
  // AARGH! this code adds blending but it is repeated in the post/pre draws
  float blendFactor = 1.0;
  if (_blend){
    blendFactor *= _blend->v[0];    
  }
  switch (whatKind) {
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
  }
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
