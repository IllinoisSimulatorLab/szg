//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************
#ifndef AR_GRAPHICS_ARRAY_NODE_H
#define AR_GRAPHICS_ARRAY_NODE_H

#include "arGraphicsNode.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

class SZG_CALL arGraphicsArrayNode:public arGraphicsNode{
 public:
  arGraphicsArrayNode(){}
  ~arGraphicsArrayNode(){}

  void draw(arGraphicsContext*){}
  virtual void initialize(arDatabase* database){
    arGraphicsNode::initialize(database);
    // The following is necessary for the current inheritance scheme.
    // (otherwise, if we attach a node to a node that hasn't had its
    // data set yet, we won't get any inheritance!)
    // DEFUNCT
    //*_whichBufferToReplace = &_commandBuffer;
  }
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
  // DEFUNCT.
  //arLightFloatBuffer** _whichBufferToReplace;

  void _mergeElements(int number, void* elements, int* IDs = NULL);
  arStructuredData* _dumpData(int number, void* elements, int* IDs = NULL);
};
#endif
