//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FRAMERATE_GRAPH_H
#define AR_FRAMERATE_GRAPH_H

#include "arFrameworkObject.h"
#include "arMath.h"
#include "arGraphicsCalling.h"

#include <map>

class SZG_CALL arPerformanceElement{
 public:
  arPerformanceElement();
  ~arPerformanceElement();

  void setNumberEntries(int number);
  void pushNewValue(float value);
  void draw() const;

  float     scale;
  arVector3 color;
 private:
  float* _data;
  int    _numberEntries;
};

class SZG_CALL arFramerateGraph: public arFrameworkObject{
 public:
  arFramerateGraph();
  ~arFramerateGraph();

  virtual void draw() const;
  virtual void drawWithComposition();
  void drawPlaced(float startX, float startY, float widthX, float widthY);

  // The functions that specifically pertain to the graph
  void addElement(const string& name,
                   int numberEntries,
                   float scale,
                   const arVector3& color);
  arPerformanceElement* getElement(const string& name) const;

 private:
  map<string, arPerformanceElement*, less<string> > _valueContainer;
};

#endif
