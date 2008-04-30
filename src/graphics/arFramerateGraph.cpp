//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFramerateGraph.h"
#include "arGlut.h"
#include "arGraphicsHeader.h"

arPerformanceElement::arPerformanceElement(
  const int n, const float s, const arVector3& c, const int i, const string& name) :
    scale(s), color(c), _data(NULL), _fInit(false), _i(i), _name(name)
{
  setNumberEntries(n);
}

arPerformanceElement::~arPerformanceElement(){
  if (_data){
    delete [] _data;
  }
}

void arPerformanceElement::setNumberEntries(const int number){
  if (_data){
    delete [] _data;
  }
  _data = new float[number];
  _numberEntries = number;
}

void arPerformanceElement::pushNewValue(float value){
  if (!_fInit) {
    _fInit = true;
    for (int i=0; i<_numberEntries; ++i)
      _data[i] = value;
    return;
  }

  memmove(_data, _data+1, (_numberEntries-1) * sizeof(_data[0]));

  // Smooth noise due to redrawing locked to graphics card's vertical retrace.
  // todo: sinc filter instead of moving-average filter.
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
    const float y = _data[i]/scale * 2. - 1.;
    glVertex3f(x, y<.99 ? y : .99, 0.02);
    // 0 <= y < 1.
  }
  glEnd();

  // Labels.
  const float y = _data[_numberEntries-1];
  char buf[80];
  sprintf(buf, "%s %5d", _name.c_str(), int(y));
  glRasterPos2f(-.8, .3 - _i * .17);
  for (const char* c = buf; *c; ++c)
    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
}

arFramerateGraph::arFramerateGraph(){
  _valueContainer.clear(); // for windows compiler
}

arFramerateGraph::~arFramerateGraph(){
  arPerfElts::iterator i;
  for (i = _valueContainer.begin(); i != _valueContainer.end(); ++i)
    delete i->second;
}

// Not const, because some sibling classes' draw() can't be const.
void arFramerateGraph::draw() {
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1,1,-1,1,0,100);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0,0,1, 0,0,0, 0,1,0);

  // Draw the magenta graph ruler.
  // The rule halfway up is white, for readability.
  glBegin(GL_LINES);
  for (int j=0; j<11; j++){
    glColor3f(1, j==5 ? 1 : 0, 1);
    const float z = 42.; // Force ruler behind data.
    const float y = j*.2 - 1.;
    glVertex3f(-1, y, z);
    glVertex3f( 1, y, z);
  }
  glEnd();

  arPerfElts::const_iterator i;
  for (i = _valueContainer.begin(); i != _valueContainer.end(); ++i){
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
                                  const float scale,
                                  const arVector3& color){
  arPerfElts::const_iterator i(_valueContainer.find(name));
  if (i != _valueContainer.end()){
    ar_log_error() << "arFramerateGraph ignoring duplicate type '" << name << "'.\n";
    return;
  }

  _valueContainer.insert(arPerfElts::value_type
     (name, new arPerformanceElement(
       numberEntries, scale, color, _valueContainer.size(), name)));
}

arPerformanceElement* arFramerateGraph::getElement(const string& name) const {
  arPerfElts::const_iterator i(_valueContainer.find(name));
  return (i == _valueContainer.end()) ? NULL : i->second;
}
