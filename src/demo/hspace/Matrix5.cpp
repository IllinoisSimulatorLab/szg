//***************************************************************************
// The database for the space tiling by dodecahedra is from the
// Geometry Center at the University of Minnesota. This is an old CAVE demo
// of George Francis's group. The porting to Syzygy was done by
// Matt Woodruff and Ben Bernard.
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "Matrix5.h"

// Identity matrix is the default.
Matrix5::Matrix5()
{
  for(int ii=0;ii<25;ii++)
    theMatrix[ii]= (ii%6) ? 0 : 1;
}

Matrix5::Matrix5(float rhs[25])
{
  for(int ii=0;ii<25;ii++)
    theMatrix[ii] = rhs[ii];
}

Matrix5::Matrix5(const Matrix5& rhs)
{
  for(int ii=0;ii<25;ii++)
    theMatrix[ii]=rhs.theMatrix[ii];
}

const Matrix5& Matrix5::operator=(const Matrix5& rhs)
{
  if (&rhs == this)
    // assignment to self
    return *this;
  for(int ii=0;ii<25;ii++)
    theMatrix[ii]=rhs.theMatrix[ii];
  return *this;
}

#ifdef UNUSED
ostream& operator<<(ostream& os, const Matrix5& matrix)
{
  os << endl;
  os << " "<<matrix[0]<<"\t"<<matrix[5]<<"\t"<<matrix[10];
  os <<"\t"<<matrix[15]<<"\t"<<matrix[20]<<endl;

  os << " "<<matrix[1]<<"\t"<<matrix[6]<<"\t"<<matrix[11];
  os <<"\t"<<matrix[16]<<"\t"<<matrix[21]<<endl;

  os << " "<<matrix[2]<<"\t"<<matrix[7]<<"\t"<<matrix[12];
  os <<"\t"<<matrix[17]<<"\t"<<matrix[22]<<endl;

  os << " "<<matrix[3]<<"\t"<<matrix[8]<<"\t"<<matrix[13];
  os <<"\t"<<matrix[18]<<"\t"<<matrix[23]<<endl;

  os << " "<<matrix[4]<<"\t"<<matrix[9]<<"\t"<<matrix[14];
  os <<"\t"<<matrix[19]<<"\t"<<matrix[24]<<endl;

  return os;
}
#endif

Matrix5 Matrix5::transpose()
{
  Matrix5 tempMatrix;
  for(int ii=0;ii<5;ii++) for(int jj=0;jj<5;jj++)
    tempMatrix[ii*5+jj]=theMatrix[jj*5+ii];
  return tempMatrix;
}

void Matrix5::multiplyWithVectorOnLeft(float vector[5], float dest[5])
{
  int ii, jj;
  float tempVector[5]; // in case dest == vector, I think.
  for(ii=0;ii<5;ii++)
    tempVector[ii]=vector[ii];
  for(ii=0;ii<5;ii++)
  {
    dest[ii]=0;
    for(jj=0;jj<5;jj++)
      dest[ii]+=theMatrix[ii*5+jj]*tempVector[jj];
  }
}
 
void Matrix5::multiplyWithVectorOnRight(float vector[5], float dest[5])
{
  Matrix5 tempMatrix = transpose();
  tempMatrix.multiplyWithVectorOnLeft(vector, dest);
}

Matrix5 multiplyMatrixByMatrix(const Matrix5& left, const Matrix5& right)
{
  const float* templeft = left.theMatrix;
  const float* tempright = right.theMatrix;
  Matrix5 product;

  for(int ii=0;ii<5;ii++)
  for(int jj=0;jj<25;jj+=5)
  {
    product[ii+jj]=templeft[ii+ 0]*tempright[jj+0] +
                   templeft[ii+ 5]*tempright[jj+1] +
                   templeft[ii+10]*tempright[jj+2] +
                   templeft[ii+15]*tempright[jj+3] +
                   templeft[ii+20]*tempright[jj+4];
  }

  return product;
}
