//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFramerateGraph.h"
#include "arGraphicsHeader.h"

arPerformanceElement::arPerformanceElement() :
  scale(1.),
  color(1,1,1),
  _data(NULL),
  _numberEntries(0)
{
}

arPerformanceElement::~arPerformanceElement(){
  if (_data){
    delete [] _data;
  }
}

void arPerformanceElement::setNumberEntries(int number){
  if (_data){
    delete [] _data;
  }
  _data = new float[number];
  _numberEntries = number;
}

void arPerformanceElement::pushNewValue(float value){
  memmove(_data, _data+1, (_numberEntries-1) * sizeof(_data[0]));

  // Slightly smooth away noise due to redrawing locked to graphics card's vertical retrace.
  value = (value + _data[_numberEntries-2] + _data[_numberEntries-3] + _data[_numberEntries-4]) * .25;

  _data[_numberEntries-1] = value;
}

void arPerformanceElement::draw() {
  glColor3fv(color.v);
  glBegin(GL_LINE_STRIP);
  float x = -1.;
  const float dx = 2. / (_numberEntries-1);
  for (int i=0; i<_numberEntries; ++i){
    x += dx;
    const float y = _data[i]/scale * 2 - 1;
    glVertex3f(x, y<.99 ? y : .99, 0.02);
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

  // Draw the graph ruler, a scale from 0 to 10.
  // For readability, the line halfway up is white not magenta.
  glBegin(GL_LINES);
  for (int j=0; j<11; j++){
    glColor3f(1, j==5 ? 1 : 0, 1);
    const float z = 42.; // Force ruler behind data.
    const float y = j*.2 - 1.;
    glVertex3f(-1, y, z);
    glVertex3f( 1, y, z);
  }
  glEnd();

  map<string, arPerformanceElement*, less<string> >::const_iterator i;
  for (i = _valueContainer.begin();
       i != _valueContainer.end(); i++){
    i->second->draw();
  }

}

// Not const, because pre- and postComposition can't be const.
void arFramerateGraph::drawWithComposition() {
  preComposition(0,0,0.3333,0.3333);
  draw();
  postComposition();
}

void arFramerateGraph::drawPlaced(const float startX,
				  const float startY,
				  const float widthX,
				  const float widthY){
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
    ar_log_warning() << "arFramerateGraph ignoring duplicate type '" << name << "'.\n";
    return;
  }

  arPerformanceElement* element = new arPerformanceElement;
  // todo: next 3 lines into a constructor
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
