//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

#ifndef MATRIX5_H
#define MATRIX5_H

#include <math.h>
#include <iostream>

class Matrix5
{
friend Matrix5 multiplyMatrixByMatrix(const Matrix5&, const Matrix5&);
public:
  Matrix5();
  Matrix5(float initialMatrix[25]);
  Matrix5(const Matrix5 &originalValue);
  ~Matrix5() {}

  const Matrix5& operator=(const Matrix5&);
  float operator[](int ii) const
    { return theMatrix[ii]; }
  float& operator[](int ii)
    { return theMatrix[ii]; }

  void multiplyWithVectorOnLeft(float vector[5], float dest[5]);
  void multiplyWithVectorOnRight(float vector[5], float dest[5]);

  Matrix5 transpose(void);

private:
  float theMatrix[25];
};

Matrix5 multiplyMatrixByMatrix(const Matrix5& left, const Matrix5& right);

//ostream& operator<<(ostream&, const Matrix5&);

#endif
