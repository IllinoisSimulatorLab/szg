//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_NAME_NODE_H
#define AR_NAME_NODE_H

#include "arDatabaseNode.h"

class SZG_CALL arNameNode: public arDatabaseNode{
 public:
  arNameNode(){
    _typeCode = AR_D_NAME_NODE;
    _typeString = "name";
  }
  virtual ~arNameNode(){}

  // just use the default virtual functions... this is really the generic
  // node!
};

#endif
