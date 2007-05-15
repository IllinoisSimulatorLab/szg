//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arBillboardNode.h"
#include "arGraphicsDatabase.h"

arBillboardNode::arBillboardNode():
  _visibility(true),
  _text(""){
  _name = "billboard_node";
  // Doesn't compile on gcc/Linux (RedHat 8.0) if these are outside
  // the constructor body.
  _typeCode = AR_G_BILLBOARD_NODE;
  _typeString = "billboard";
}

void arBillboardNode::draw(arGraphicsContext*){
  // Copy _text and visibility for thread-safety.
  _nodeLock.lock();
    const string text = _text;
    const bool visibility = _visibility;
  _nodeLock.unlock();
  if (!visibility)
    return; 

  arTexFont* alphabet = _owningDatabase->getTexFont();
  list<string> parse = ar_parseLineBreaks(text);
  arTextBox format;
  format.lineSpacing = 1.0;
  format.color = arVector3(1,1,1);
  format.width = 1;
  // Want columns to be essentially infinite at first so there is no wrap around in
  // the calculation of text size.
  format.columns = 1000;
  format.upperLeft = arVector3(-0.5, 0.5, 0);
  float width = 0;
  float height = 0;
  alphabet->getTextMetrics(parse, format, width, height);
  // For the width we get back, each character has width 0.001. We want the character to
  // have width 1.
  width *= 1000;
  height *= 1000;
  format.width = width;
  format.columns = int(ceil(float(width)));
  format.upperLeft = arVector3(-format.width/2.0, height/2.0, 0);
  // Render the background.
  const float cornerY=height/2.0;
  const float cornerX=width/2.0;
  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);
  glColor3f(0,0,0);
  glVertex3f(-cornerX,-cornerY,-0.01);
  glVertex3f(-cornerX,cornerY,-0.01);
  glVertex3f(cornerX,cornerY,-0.01);
  glVertex3f(cornerX,-cornerY,-0.01);
  glColor3f(1,0,0);
  float cornerDelta = 0.2;
  glVertex3f(-cornerX-cornerDelta,-cornerY-cornerDelta,-0.02);
  glVertex3f(-cornerX-cornerDelta,cornerY+cornerDelta,-0.02);
  glVertex3f(cornerX+cornerDelta,cornerY+cornerDelta,-0.02);
  glVertex3f(cornerX+cornerDelta,-cornerY-cornerDelta,-0.02);
  glEnd();
  // Must render the text second, since it uses transparent.
  alphabet->renderText(parse, format);
}

arStructuredData* arBillboardNode::dumpData(){
  // Caller is responsible for deleting.
  _nodeLock.lock();
  arStructuredData* r = _dumpData(_text, _visibility, false);
  _nodeLock.unlock();
  return r;
}

bool arBillboardNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_BILLBOARD, inData->getID(), "arBillboardNode"))
    return false;

  const int vis = inData->getDataInt(_g->AR_BILLBOARD_VISIBILITY);
  _nodeLock.lock();
  _visibility = vis ? true : false;
  _text = inData->getDataString(_g->AR_BILLBOARD_TEXT);
  _nodeLock.unlock();
  return true;
}

string arBillboardNode::getText(){
  _nodeLock.lock();
  string r = _text;
  _nodeLock.unlock();
  return r;
}

void arBillboardNode::setText(const string& text){
  if (active()){
    _nodeLock.lock();
    arStructuredData* r = _dumpData(text, _visibility, true);
    _nodeLock.unlock();
    getOwner()->alter(r);
    getOwner()->getDataParser()->recycle(r);
  }
  else{
    _nodeLock.lock();
    _text = text;
    _nodeLock.unlock();
  }
}

// NOT thread-safe.
arStructuredData* arBillboardNode::_dumpData(const string& text,
					     bool visibility,
                                             bool owned){
  arStructuredData* result = NULL;
  if (owned){
    result = getOwner()->getDataParser()->getStorage(_g->AR_BILLBOARD);
  }
  else{
    result = _g->makeDataRecord(_g->AR_BILLBOARD);
  }
  _dumpGenericNode(result,_g->AR_BILLBOARD_ID);
  const ARint vis = visibility ? 1 : 0;
  if ((!result->dataIn(_g->AR_BILLBOARD_VISIBILITY, &vis, AR_INT, 1)
       || !result->dataInString(_g->AR_BILLBOARD_TEXT, text))){
    delete result;
    return NULL;
  }
  return result;
}

