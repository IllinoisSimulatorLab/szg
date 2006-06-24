//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************
#ifndef AR_GRAPHICS_ARRAY_NODE_H
#define AR_GRAPHICS_ARRAY_NODE_H

#include "arGraphicsNode.h"
#include "arGraphicsCalling.h"

class SZG_CALL arGraphicsArrayNode:public arGraphicsNode{
 public:
  arGraphicsArrayNode(){}
  virtual ~arGraphicsArrayNode(){}

  void draw(arGraphicsContext*){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

 protected:
  arDataType _nodeDataType;

  // Each array has a 4 byte header giving the ID of the record type,
  // to e.g. distinguish between a color3 and a color4 array
  // when we only inherit a color pointer.

  // For arStructuredData records.
  int _arrayStride;
  int _recordType;
  int _IDField;
  int _indexField;
  int _dataField;

  void _mergeElements(int number, void* elements, int* IDs = NULL);
  arStructuredData* _dumpData(int number, void* elements, int* IDs, bool owned);
};
#endif
