//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arBillboardNode.h"
#include "arGraphicsDatabase.h"

arBillboardNode::arBillboardNode():
  _visibility(true),
  _text(""){
  // A sensible default name.
  _name = "billboard_node";
  // Doesn't compile on gcc/Linux (RedHat 8.0) if these are outside
  // of the constructor body.
  _typeCode = AR_G_BILLBOARD_NODE;
  _typeString = "billboard";
}

void arBillboardNode::draw(arGraphicsContext*){
  // Copy _text and visibility for thread-safety.
  ar_mutex_lock(&_nodeLock);
    const string text = _text;
    const bool visibility = _visibility;
  ar_mutex_unlock(&_nodeLock);
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
  ar_mutex_lock(&_nodeLock);
  arStructuredData* r = _dumpData(_text, _visibility, false);
  ar_mutex_unlock(&_nodeLock);
  return r;
}

bool arBillboardNode::receiveData(arStructuredData* inData){
  if (inData->getID() == _g->AR_BILLBOARD){
    int vis = inData->getDataInt(_g->AR_BILLBOARD_VISIBILITY);
    ar_mutex_lock(&_nodeLock);
    _visibility = vis ? true : false;
    _text = inData->getDataString(_g->AR_BILLBOARD_TEXT);
    ar_mutex_unlock(&_nodeLock);
    return true;
  }

  cerr << "arBillboardNode error: expected "
       << _g->AR_BILLBOARD
       << " (" << _g->_stringFromID(_g->AR_BILLBOARD) << "), not "
       << inData->getID()
       << " (" << _g->_stringFromID(inData->getID()) << ")\n";
  return false;
}

string arBillboardNode::getText(){
  ar_mutex_lock(&_nodeLock);
  string r = _text;
  ar_mutex_unlock(&_nodeLock);
  return r;
}

void arBillboardNode::setText(const string& text){
  if (active()){
    ar_mutex_lock(&_nodeLock);
    arStructuredData* r = _dumpData(text, _visibility, true);
    ar_mutex_unlock(&_nodeLock);
    getOwner()->alter(r);
    getOwner()->getDataParser()->recycle(r);
  }
  else{
    ar_mutex_lock(&_nodeLock);
    _text = text;
    ar_mutex_unlock(&_nodeLock);
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

