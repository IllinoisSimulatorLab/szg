//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************
#ifndef AR_GRAPHICS_ARRAY_NODE_H
#define AR_GRAPHICS_ARRAY_NODE_H

#include "arGraphicsNode.h"

class SZG_CALL arGraphicsArrayNode:public arGraphicsNode{
 public:
  arGraphicsArrayNode(){}
  ~arGraphicsArrayNode(){}

  void draw(){}
  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

 protected:
  arDataType _nodeDataType;

  // Each array has a 4 byte header giving the ID of the record type,
  // to e.g. distinguish between a color3 and a color4 array
  // when we only inherit a color pointer.

  /// For arStructuredData records.
  int _arrayStride;
  int _recordType;
  int _IDField;
  int _indexField;
  int _dataField;

  /// Which array pointer to replace (_color, tex2, _points, etc.).
  arLightFloatBuffer** _whichBufferToReplace;

  void _mergeElements(int number, void* elements, int* IDs = NULL);
  arStructuredData* _dumpData(int number, void* elements, int* IDs = NULL);
};
#endif
