//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CAMERA_H
#define AR_CAMERA_H

#include "arMath.h"
#include "arSZGClient.h"

class arCamera{
 public:
  arCamera(){}
  virtual ~arCamera(){}
  virtual bool configure(arSZGClient*){
    cout << "arCamera warning: using empty configure.\n";
    return true;
  }
  virtual bool configure(const string&, arSZGClient*){
    cout << "arCamera warning: using empty configure.\n";
    return true;
  }
  virtual arMatrix4 getProjectionMatrix(){ return ar_identityMatrix(); }
  virtual arMatrix4 getModelviewMatrix(){ return ar_identityMatrix(); }
  virtual void loadViewMatrices();
};

#endif
