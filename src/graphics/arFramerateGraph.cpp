//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFramerateGraph.h"
#include "arGraphicsHeader.h"

arPerformanceElement::arPerformanceElement(){
  _data = NULL;
  scale = 1; 
  color = arVector3(1,1,1);
  _numberEntries = 0;
}

arPerformanceElement::~arPerformanceElement(){
  if (_data){
    delete [] _data;
  }
}

void arPerformanceElement::setNumberEntries(int number){
  float* newData = new float[number];
  if (_data){
    float* temp = _data;
    _data = newData;
    delete [] temp;
  }
  else{
    _data = newData;
  }
  _numberEntries = number;
}

void arPerformanceElement::pushNewValue(float value){
  memmove(_data, _data+1, (_numberEntries-1) * sizeof(_data[0]));
  _data[_numberEntries-1] = value;
}

void arPerformanceElement::draw() {
  glColor3fv(color.v);
  glBegin(GL_LINE_STRIP);
  for (int i=0; i<_numberEntries; i++){
    glVertex3f(-1 + 2.0*i/(_numberEntries-1), 
               -1+2.0*_data[i]/scale,
               0.02);
  }
  glEnd();
}

arFramerateGraph::arFramerateGraph(){
  _valueContainer.clear(); // windows compiler needs this
}

arFramerateGraph::~arFramerateGraph(){
  // bug: memory leak: should delete the performance elements.
}

// Not const, because some sibling classes' draw() can't be const.
void arFramerateGraph::draw() {
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1,1,-1,1,0,100);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0,0,1, 0,0,0, 0,1,0);

  map<string, arPerformanceElement*, less<string> >::const_iterator i;
  for (i = _valueContainer.begin();
       i != _valueContainer.end(); i++){
    i->second->draw();
  }

  // Draw the graph ruler. This is a scale from 0 to 10.
  glBegin(GL_LINES);
  for (int j=0; j<11; j++){
    glColor3f(1,0,0);
    glVertex3f(-1, -1+2.0*j/10, 0);
    glVertex3f(1, -1+2.0*j/10, 0);
    glColor3f(0,0,1);
    glVertex3f(-1, -1+2.0*j/10 + 0.01, 0);
    glVertex3f(1, -1+2.0*j/10 + 0.01, 0);
  }
  glEnd();
}

// Not const, because pre- and postComposition can't be const.
void arFramerateGraph::drawWithComposition() {
  preComposition(0,0,0.3333,0.3333);
  draw();
  postComposition();
}

void arFramerateGraph::drawPlaced(float startX,
				  float startY,
				  float widthX,
				  float widthY){
  preComposition(startX, startY, widthX, widthY);
  draw();
  postComposition();
}

void arFramerateGraph::addElement(const string& name,
                                  int numberEntries,
                                  float scale,
                                  const arVector3& color){
  map<string, arPerformanceElement*, less<string> >::const_iterator i =
    _valueContainer.find(name);
  if (i != _valueContainer.end()){
    ar_log_warning() << "arFramerateGraph ignoring duplicate value type '"
	 << name << "'.\n";
    return;
  }

  arPerformanceElement* element = new arPerformanceElement;
  element->setNumberEntries(numberEntries);
  element->scale = scale;
  element->color = color;
  _valueContainer.insert
    (map<string, arPerformanceElement*, less<string> >::value_type
     (name, element));
}

arPerformanceElement* arFramerateGraph::getElement(const string& name) const {
  map<string, arPerformanceElement*, less<string> >::const_iterator i =
    _valueContainer.find(name);
  return (i == _valueContainer.end()) ? NULL : i->second;
}
