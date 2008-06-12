//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BILLBOARD_NODE_H
#define AR_BILLBOARD_NODE_H

#include "arGraphicsNode.h"
#include "arGraphicsCalling.h"

// Billboard display of text.

class SZG_CALL arBillboardNode: public arGraphicsNode{
 public:
  arBillboardNode();
  virtual ~arBillboardNode() {}

  void draw(arGraphicsContext*);
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  string getText();
  void   setText(const string& text);

 protected:
  bool   _visibility;
  string _text;

  arStructuredData* _dumpData(const string& text, bool visibility, bool owned);
};

#endif
