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
  arPerformanceElement(const int, const float, const arVector3&, const int, const string&);
  ~arPerformanceElement();

  void setNumberEntries(const int);
  void pushNewValue(float);
  void draw();

  const float     _scale;
  const arVector3 _color;
 private:
  inline float _dataValue(const int i) const;

  float* _data;
  int  _numberEntries;
  bool _fInit;
  const int _i;
  const string _name;
};

typedef map<string, arPerformanceElement*, less<string> > arPerfElts;

class SZG_CALL arFramerateGraph: public arFrameworkObject{
 public:
  arFramerateGraph();
  ~arFramerateGraph();

  virtual void draw();
  virtual void drawWithComposition();
  void drawPlaced(const float startX, const float startY, const float widthX, const float widthY);

  // graph-specific
  void addElement(const string& name, int numberEntries, const float scale,
    const arVector3& color);
  arPerformanceElement* getElement(const string& name) const;

 private:
  arPerfElts _valueContainer;
};

#endif
