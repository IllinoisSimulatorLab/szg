//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arFramerateGraph.h"
#include "arGlutRenderFuncs.h"
#include "arGraphicsHeader.h"

arPerformanceElement::arPerformanceElement(
  const int n, const float s, const arVector3& c, const int i, const string& name) :
    _scale(s>0. ? 2./s : 1.0),
    _color(c),
    _data(NULL),
    _fInit(false),
    _i(i),
    _name(name)
{
  setNumberEntries(n);
  if (s < 0) {
    ar_log_error() << "arPerformanceElement ignoring nonpositive scale " << s << ".\n";
  }
}

arPerformanceElement::~arPerformanceElement() {
  if (_data) {
    delete [] _data;
  }
}

void arPerformanceElement::setNumberEntries(const int number) {
  if (number == _numberEntries)
    return;

  if (_data) {
    delete [] _data;
  }
  _numberEntries = number;
  _data = new float[_numberEntries];
  _fInit = false;
}

void arPerformanceElement::pushNewValue(float value) {
  if (!_fInit) {
    _fInit = true;
    for (int i=0; i<_numberEntries; ++i)
      _data[i] = value;
    return;
  }

  memmove(_data, _data+1, (_numberEntries-1) * sizeof(_data[0]));

  // Smooth noise due to redrawing locked to graphics card's vertical retrace.
  // Moving-average filter.
  value = .4 * value +
          .3 * _data[_numberEntries-2] +
          .2 * _data[_numberEntries-3] +
          .1 * _data[_numberEntries-4];
  _data[_numberEntries-1] = value;
}

// In [0, 1).
inline float arPerformanceElement::_dataValue(const int i) const {
  const float y = _data[i] * _scale - 1.;
  return y < .99 ? y : .99;
}

// Z-order: ruler behind constrast behind squiggle.
const float zRuler = 42.;
const float zBorder = 30.;
const float zSquiggle = 0.02;

void arPerformanceElement::draw() {
  int i;
  float x;
  const float dx = 2. / (_numberEntries-1);

  // Black border around squiggle, for contrast.
  glLineWidth(9.);
  glColor3f(0,0,0);
  glBegin(GL_LINE_STRIP);
  for (i=0, x=-1.; i<_numberEntries; ++i) {
    glVertex3f(x += dx, _dataValue(i), zBorder);
  }
  glEnd();

  // Squiggle.
  glLineWidth(3.);
  glColor3fv(_color.v);
  glBegin(GL_LINE_STRIP);
  for (i=0, x=-1.; i<_numberEntries; ++i) {
    glVertex3f(x += dx, _dataValue(i), zSquiggle);
  }
  glEnd();

  // Labels.  Drawn last, for readability.
  char buf[80];
  sprintf(buf, "%s %5d", _name.c_str(), int(_data[_numberEntries-1]));
  glRasterPos2f(-.8, .3 - _i * .17);
  for (const char* c = buf; *c; ++c)
    ar_glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *c);
  glLineWidth(1.);
}

arFramerateGraph::arFramerateGraph() {
  _valueContainer.clear(); // for windows compiler
}

arFramerateGraph::~arFramerateGraph() {
  for (arPerfElts::iterator i = _valueContainer.begin(); i != _valueContainer.end(); ++i)
    delete i->second;
}

// Not const, because some sibling classes' draw() can't be const.
void arFramerateGraph::draw() {
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1, 1, -1, 1, 0, 100);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0,0,1, 0,0,0, 0,1,0);

  // Draw the magenta graph ruler.
  // The rule halfway up is white, for readability.
  glBegin(GL_LINES);
  for (int j=0; j<11; j++) {
    glColor3f(1, j==5 ? 1 : 0, 1);
    const float y = j*.2 - 1.;
    glVertex3f(-1, y, zRuler);
    glVertex3f( 1, y, zRuler);
  }
  glEnd();

  arPerfElts::const_iterator i;
  for (i = _valueContainer.begin(); i != _valueContainer.end(); ++i) {
    i->second->draw();
  }
}

// Not const, because pre- and postComposition can't be const.
void arFramerateGraph::drawWithComposition() {
  preComposition(0, 0, 0.3333, 0.3333);
  draw();
  postComposition();
}

void arFramerateGraph::drawPlaced(const float startX,
                                  const float startY,
                                  const float widthX,
                                  const float widthY) {
  preComposition(startX, startY, widthX, widthY);
  draw();
  postComposition();
}

void arFramerateGraph::addElement(const string& name,
                                  int numberEntries,
                                  const float scale,
                                  const arVector3& color) {
  arPerfElts::const_iterator i(_valueContainer.find(name));
  if (i != _valueContainer.end()) {
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
