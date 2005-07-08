//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arBillboardNode.h"
#include "arGraphicsDatabase.h"

arBillboardNode::arBillboardNode():
  _visibility(true),
  _text(""){

  // Doesn't compile on gcc/Linux (RedHat 8.0) if these are outside
  // of the constructor body.
  _typeCode = AR_G_BILLBOARD_NODE;
  _typeString = "billboard";
}

void arBillboardNode::draw(arGraphicsContext*){
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
  arTexture** alphabet = _owningDatabase->getAlphabet();
  const int len = _text.length();
  if (!_visibility)
    return; // not visible

  // First pass through the command buffer:  count the
  // lines of text and the max line-length.
  // Inefficient but fine for short strings.
  int numberLines = 0;
  int maxLineSize = 1;
  int currentLineSize = 0;
  int i = 0;
  for (i=0; i<len; i++){
    if (_text[i] == '/' || i == len-1){
      ++numberLines;
      if (currentLineSize > maxLineSize)
	maxLineSize = currentLineSize;
      currentLineSize = 0;
    }
    else if (_text[i] != '\0'){
      ++currentLineSize;
    }
  }
  // there is at least one line
  if (numberLines == 0){
    numberLines = 1;
  }
  // not sure why this is needed... a stupid band-aid
  ++maxLineSize;
  // draw the background
  const float cornerY=0.5;
  const float cornerX=(0.5*maxLineSize);
  glBegin(GL_QUADS);
  glColor3f(1,1,1);
  glVertex3f(-cornerX,-cornerY,0.01);
  glVertex3f(-cornerX,cornerY,0.01);
  glVertex3f(cornerX,cornerY,0.01);
  glVertex3f(cornerX,-cornerY,0.01);
  glColor3f(1,0,0);
  float cornerDelta = 0.2;
  glVertex3f(-cornerX-cornerDelta,-cornerY-cornerDelta,0.02);
  glVertex3f(-cornerX-cornerDelta,cornerY+cornerDelta,0.02);
  glVertex3f(cornerX+cornerDelta,cornerY+cornerDelta,0.02);
  glVertex3f(cornerX+cornerDelta,-cornerY-cornerDelta,0.02);
  glEnd();

  // Render the text.
  int xPos=0;
  int yPos=0;
  for (i=0; i<len; i++){
    if ( _text[i] =='/' ){
      // Start of a new line.
      yPos++;
      xPos=0;
      continue;
    }

    const int diff = _text[i] - 'a';
    if (diff>=0 && diff<26 && alphabet[diff]){
      const float upperX =
	-cornerX+(2.0*cornerX*(maxLineSize-xPos-1))/maxLineSize;
      const float upperY =
	-cornerY+(2.0*cornerY*(numberLines-yPos-1))/numberLines;
      const float deltaX =
	2.0*cornerX/maxLineSize;
      const float deltaY =
	2.0*cornerY/numberLines;
      alphabet[diff]->activate();
      glBegin(GL_QUADS);
      glTexCoord2f(0,0);
      glVertex3f(upperX,upperY,0);
      glTexCoord2f(1,0);
      glVertex3f(upperX+deltaX,upperY,0);
      glTexCoord2f(1,1);
      glVertex3f(upperX+deltaX,upperY+deltaY,0);
      glTexCoord2f(0,1);
      glVertex3f(upperX,upperY+deltaY,0);
      glEnd();
      alphabet[diff]->deactivate();
    }
    xPos++;
  }
}

arStructuredData* arBillboardNode::dumpData(){
  return _dumpData(_text, _visibility);
}

bool arBillboardNode::receiveData(arStructuredData* inData){
  if (inData->getID() == _g->AR_BILLBOARD){
    int vis = inData->getDataInt(_g->AR_BILLBOARD_VISIBILITY);
    _visibility = vis ? true : false;
    _text = inData->getDataString(_g->AR_BILLBOARD_TEXT);
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
  return _text;
}

void arBillboardNode::setText(const string& text){
  if (_owningDatabase){
    arStructuredData* r = _dumpData(text, _visibility);
    _owningDatabase->alter(r);
    delete r;
  }
  else{
    _text = text;
  }
}

arStructuredData* arBillboardNode::_dumpData(const string& text,
					     bool visibility){
  arStructuredData* result = _g->makeDataRecord(_g->AR_BILLBOARD);
  _dumpGenericNode(result,_g->AR_BILLBOARD_ID);
  const ARint vis = visibility ? 1 : 0;
  if ((!result->dataIn(_g->AR_BILLBOARD_VISIBILITY, &vis, AR_INT, 1)
       || !result->dataInString(_g->AR_BILLBOARD_TEXT, text))){
    delete result;
    return NULL;
  }
  return result;
}

