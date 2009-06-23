//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_CONTROLLER_H
#define AR_CONTROLLER_H

#include "arMath.h"
#include "arGraphicsDatabase.h"
#include "arFrameworkCalling.h"

// Manipulate a 4x4 matrix transformation in an arGraphicsDatabase.

class SZG_CALL arController{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arController();
  virtual ~arController();

  void setTransformID(int);
  void setDatabase(arGraphicsDatabase*);
  void setTransform(const arMatrix4&);
  const arMatrix4& getTransform()
    { return _transform; }

 private:
  int _transformID; // the ID of the transform we are manipulating
  arGraphicsDatabase* _theDatabase; // the database we are manipulating
  arMatrix4 _transform; // current value of the manipulated transform
  arStructuredData* _transformData;
};

#endif
