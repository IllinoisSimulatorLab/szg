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
  // Local copies.
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
  // Avoid wraparound while calculating text size.
  format.columns = 1000;
  format.upperLeft = arVector3(-0.5, 0.5, 0);
  float width = 0;
  float height = 0;
  alphabet->getTextMetrics(parse, format, width, height);

  // Rescale width from .001 per character to 1.
  width *= 1000;
  height *= 1000;
  format.width = width;
  format.columns = int(ceil(float(width)));
  format.upperLeft = arVector3(-format.width/2.0, height/2.0, 0);

  // Render the background.
  const float cornerY = height/2.0;
  const float cornerX = width/2.0;
  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);
  glColor3f(0,0,0);
  glVertex3f(-cornerX,-cornerY,-0.01);
  glVertex3f(-cornerX,cornerY,-0.01);
  glVertex3f(cornerX,cornerY,-0.01);
  glVertex3f(cornerX,-cornerY,-0.01);
  glColor3f(1,0,0);
  const float cornerDelta = 0.2;
  const float z = -0.02;
  glVertex3f(-cornerX-cornerDelta,-cornerY-cornerDelta, z);
  glVertex3f(-cornerX-cornerDelta,cornerY+cornerDelta, z);
  glVertex3f(cornerX+cornerDelta,cornerY+cornerDelta, z);
  glVertex3f(cornerX+cornerDelta,-cornerY-cornerDelta, z);
  glEnd();

  // Render text last, for its transparency.
  alphabet->renderText(parse, format);
}

bool arBillboardNode::receiveData(arStructuredData* inData){
  if (!_g->checkNodeID(_g->AR_BILLBOARD, inData->getID(), "arBillboardNode"))
    return false;

  const bool vis = inData->getDataInt(_g->AR_BILLBOARD_VISIBILITY) ? true : false;
  arGuard dummy(_nodeLock);
  _visibility = vis;
  _text = inData->getDataString(_g->AR_BILLBOARD_TEXT);
  return true;
}

string arBillboardNode::getText(){
  arGuard dummy(_nodeLock);
  return _text;
}

void arBillboardNode::setText(const string& text){
  if (active()){
    _nodeLock.lock();
    arStructuredData* r = _dumpData(text, _visibility, true);
    _nodeLock.unlock();
    getOwner()->alter(r);
    recycle(r);
  }
  else{
    arGuard dummy(_nodeLock);
    _text = text;
  }
}

arStructuredData* arBillboardNode::dumpData(){
  arGuard dummy(_nodeLock);
  return _dumpData(_text, _visibility, false);
}

arStructuredData* arBillboardNode::_dumpData(
  const string& text, bool visibility, bool owned){
  arStructuredData* r = _getRecord(owned, _g->AR_BILLBOARD);
  _dumpGenericNode(r, _g->AR_BILLBOARD_ID);
  const ARint vis = visibility ? 1 : 0;
  if (!r->dataIn(_g->AR_BILLBOARD_VISIBILITY, &vis, AR_INT, 1) ||
      !r->dataInString(_g->AR_BILLBOARD_TEXT, text)){
    delete r;
    return NULL;
  }
  return r;
}
