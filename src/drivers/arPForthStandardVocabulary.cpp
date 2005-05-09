//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPForth.h"
#include "arDataUtilities.h"
#include <sstream>

using std::vector;
using std::string;
using std::istringstream;

namespace arPForthSpace {

// Run-time behaviors

class Equals : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Equals::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float val2 = pf->stackPop();  
  float val1 = pf->stackPop();
  pf->stackPush((float)val1==val2);
  return true;
}  

class LessThan : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool LessThan::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float val2 = pf->stackPop();  
  float val1 = pf->stackPop();
  pf->stackPush((float)val1<val2);
  return true;
}  

class GreaterThan : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool GreaterThan::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float val2 = pf->stackPop();  
  float val1 = pf->stackPop();
  pf->stackPush((float)val1>val2);
  return true;
}  

class LessEquals : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool LessEquals::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float val2 = pf->stackPop();  
  float val1 = pf->stackPop();
  pf->stackPush((float)val1<=val2);
  return true;
}  

class GreaterEquals : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool GreaterEquals::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float val2 = pf->stackPop();  
  float val1 = pf->stackPop();
  pf->stackPush((float)val1>=val2);
  return true;
}  

class StringEquals : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool StringEquals::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address1 = (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  std::string string1 = pf->getString( address1 );
  std::string string2 = pf->getString( address2 );
  pf->stackPush((float)(string1==string2));
  return true;
}  


class Not : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Not::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float val1 = pf->stackPop();
  pf->stackPush((float)val1<1.0);
  return true;
}  

class Fetch : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Fetch::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  pf->testFailAddress( address, 1 );
  pf->stackPush( pf->getDataValue( address ) );
  return true;
}

class Store : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Store::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  float temp = pf->stackPop();
  pf->putDataValue( address, temp );
  return true;
}

class Duplicate : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Duplicate::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float temp = pf->stackPop();
  pf->stackPush( temp );
  pf->stackPush( temp );
  return true;
}

class MatStore : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool MatStore::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  pf->testFailAddress( address, 16 );
  arMatrix4 tempMat;
  float* matPtr = tempMat.v+15;
  for (int i=0; i<16; i++)
    *matPtr-- = pf->stackPop();
  pf->putDataMatrix( address, tempMat );
  return true;
}

class VecStore : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool VecStore::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  pf->testFailAddress( address, 3 );
  arVector3 temp;
  float* ptr = temp.v+2;
  for (int i=0; i<3; i++)
    *ptr-- = pf->stackPop();
  pf->putDataArray( address, temp.v, 3 );
  return true;
}

class MatStoreTranspose : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool MatStoreTranspose::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  pf->testFailAddress( address, 16 );
  arMatrix4 tempMat;
  float* matPtr = tempMat.v+15;
  for (int i=0; i<16; i++)
    *matPtr-- = pf->stackPop();
  pf->putDataMatrix( address, tempMat.transpose() );
  return true;
}

class MatCopy : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool MatCopy::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arMatrix4 result( pf->getDataMatrix( address1 ) );
  pf->putDataMatrix( address2, result );
  return true;
}

class VecCopy : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool VecCopy::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arVector3 temp;
  pf->getDataArray( address1, temp.v, 3 );
  pf->putDataArray( address2, temp.v, 3 );
  return true;
}

class MatMultiply : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool MatMultiply::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arMatrix4 result( pf->getDataMatrix( address1 )*pf->getDataMatrix( address2 ) );
  pf->putDataMatrix( address3, result );
  return true;
}

class MatVecMultiply : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool MatVecMultiply::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arMatrix4 M = pf->getDataMatrix( address1 );
  arVector3 V;
  pf->getDataArray( address2, V.v, 3 );
  arVector3 result( M*V );
  pf->putDataArray( address3, result.v, 3 );
  return true;
}

class ConcatMatrices : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool ConcatMatrices::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outputAddress = (long)pf->stackPop();
  long numInputMatrices = (long)pf->stackPop();
  if (numInputMatrices <= 0) {
    throw arPForthException("concatMatrices # input matrices must be > 0.");
  }
  arMatrix4 result;
  for (long i=0; i<numInputMatrices; ++i) {
    long inputAddress = (long)pf->stackPop();
    arMatrix4 inputMatrix = pf->getDataMatrix( inputAddress );
    result = inputMatrix * result;
  }
  pf->putDataMatrix( outputAddress, result );
  return true;
}

class Add : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Add::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float num2 = pf->stackPop();
  float num1 = pf->stackPop();
  pf->stackPush( num1+num2 );
  return true;
}

class Subtract : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Subtract::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float num2 = pf->stackPop();
  float num1 = pf->stackPop();
  pf->stackPush( num1-num2 );
  return true;
}

class AddVectors : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool AddVectors::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arVector3 V1, V2;
  pf->getDataArray( address1, V1.v, 3 );
  pf->getDataArray( address2, V2.v, 3 );
  arVector3 V3( V1+V2 );
  pf->putDataArray( address3, V3.v, 3 );
  return true;
}

class SubVectors : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool SubVectors::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arVector3 V1, V2;
  pf->getDataArray( address1, V1.v, 3 );
  pf->getDataArray( address2, V2.v, 3 );
  arVector3 V3( V1-V2 );
  pf->putDataArray( address3, V3.v, 3 );
  return true;
}

class VecScalarMultiply : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool VecScalarMultiply::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  float scaleFactor = pf->stackPop();
  arVector3 V1;
  pf->getDataArray( address1, V1.v, 3 );
  arVector3 V2( scaleFactor*V1 );
  pf->putDataArray( address2, V2.v, 3 );
  return true;
}

class AddArrays : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool AddArrays::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long num =      (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  for (long i=0; i<num; ++i) {
    pf->putDataValue( address3+i, pf->getDataValue( address1+i )+pf->getDataValue( address2+i ) );
  }
  return true;
}

class SubArrays : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool SubArrays::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long num =      (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  for (long i=0; i<num; ++i) {
    pf->putDataValue( address3+i, pf->getDataValue( address1+i )-pf->getDataValue( address2+i ) );
  }
  return true;
}

class MultArrays : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool MultArrays::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long num =      (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  for (long i=0; i<num; ++i) {
    pf->putDataValue( address3+i, pf->getDataValue( address1+i )*pf->getDataValue( address2+i ) );
  }
  return true;
}

class DivArrays : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool DivArrays::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address3 = (long)pf->stackPop();
  long num =      (long)pf->stackPop();
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  for (long i=0; i<num; ++i) {
    pf->putDataValue( address3+i, pf->getDataValue( address1+i )/pf->getDataValue( address2+i ) );
  }
  return true;
}

class Multiply : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Multiply::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float num2 = pf->stackPop();
  float num1 = pf->stackPop();
  pf->stackPush( num1*num2 );
  return true;
}
 
class Divide : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool Divide::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  float num2 = pf->stackPop();
  float num1 = pf->stackPop();
  pf->stackPush( num1/num2 );
  return true;
}

class StackPrint : public arPForthAction {
  public:
  bool run( arPForth* fp );
};
bool StackPrint::run( arPForth* fp ) {
  if (fp == 0)
    return false;
  cerr << "Stack:\n";
  for (int i=fp->stackSize()-1; i>=0; i--)
    cerr << " [ " << fp->stackElement(i) << " ]\n";
  return true;
}

class ClearStack : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool ClearStack::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  for (unsigned int i=0; i<pf->stackSize(); i++)
    pf->stackPop();
  return true;
}

class DataspacePrint : public arPForthAction {
  public:
  bool run( arPForth* pf );
};
bool DataspacePrint::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  pf->printDataspace();
  return true;
}

class ArrayPrint : public arPForthAction {
  public:
  bool run( arPForth* pf );
};
bool ArrayPrint::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long num = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  for (long i=0; i<num; ++i) {
    cout << pf->getDataValue( address1+i ) << " ";
  }
  cout << endl;
  return true;
}

class VectorPrint : public arPForthAction {
  public:
  bool run( arPForth* pf );
};
bool VectorPrint::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address1 = (long)pf->stackPop();
  for (long i=0; i<3; ++i) {
    cout << pf->getDataValue( address1+i ) << " ";
  }
  cout << endl;
  return true;
}

class MatrixPrint : public arPForthAction {
  public:
  bool run( arPForth* pf );
};
bool MatrixPrint::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  cout << pf->getDataMatrix( address ) << endl;
  return true;
}

class StringspacePrint : public arPForthAction {
  public:
  bool run( arPForth* pf );
};
bool StringspacePrint::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  pf->printStringspace();
  return true;
}

class StringPrint : public arPForthAction {
  public:
  bool run( arPForth* pf );
};
bool StringPrint::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  std::string theString = pf->getString( address );
  cout << theString << endl;
  return true;
}

class IdentityMatrix : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool IdentityMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  pf->putDataMatrix( address, ar_identityMatrix() );
  return true;
}
  
class TranslationMatrix : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool TranslationMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  float z = pf->stackPop();
  float y = pf->stackPop();
  float x = pf->stackPop();
  pf->putDataMatrix( address, ar_translationMatrix( x, y, z ) );
  return true;
}
  
class TranslationMatrixFromVector : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool TranslationMatrixFromVector::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address2 = (long)pf->stackPop();
  long address1 = (long)pf->stackPop();
  arVector3 V;
  pf->getDataArray( address1, V.v, 3 );
  pf->putDataMatrix( address2, ar_translationMatrix( V ) );
  return true;
}
  
class RotationMatrix : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool RotationMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long address = (long)pf->stackPop();
  long axis = (long)pf->stackPop();
  float angle = ar_convertToRad( pf->stackPop() );
  if ((axis < 0)||(axis > 2))
    throw arPForthException("illegal rotation axis, must be 0(x)-2(z).");
  pf->putDataMatrix( address, ar_rotationMatrix( 'x'+axis, angle ) );
  return true;
}

class RotationMatrixV : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool RotationMatrixV::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long axisAddress = (long)pf->stackPop();
  float angle = ar_convertToRad( pf->stackPop() );
  arVector3 V;
  pf->getDataArray( axisAddress, V.v, 3 );
  pf->putDataMatrix( outAddress, ar_rotationMatrix( V, angle ) );
  return true;
}

class ExtractTranslationMatrix : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool ExtractTranslationMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long inAddress = (long)pf->stackPop();
  arMatrix4 result( ar_extractTranslationMatrix( pf->getDataMatrix( inAddress ) ) );
  pf->putDataMatrix( outAddress, result );
  return true;
}
  
class ExtractTranslationVector : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool ExtractTranslationVector::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long inAddress = (long)pf->stackPop();
  arVector3 result( ar_extractTranslation( pf->getDataMatrix( inAddress ) ) );
  pf->putDataArray( outAddress, result.v, 3 );
  return true;
}
  
class ExtractRotationMatrix : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool ExtractRotationMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long inAddress = (long)pf->stackPop();
  arMatrix4 result( ar_extractRotationMatrix( pf->getDataMatrix( inAddress ) ) );
  pf->putDataMatrix( outAddress, result );
  return true;
}

class ExtractEulerAngles : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool ExtractEulerAngles::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long inAddress = (long)pf->stackPop();
  arVector3 result( ar_extractEulerAngles( pf->getDataMatrix( inAddress ) ) );
  for (unsigned int i=0; i<3; ++i) {
    result.v[i] = ar_convertToDeg(result.v[i]);
  }
  pf->putDataArray( outAddress, result.v, 3 );
  return true;
}
  
class RotationMatrixFromEulerAngles : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool RotationMatrixFromEulerAngles::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long inAddress = (long)pf->stackPop();
  arVector3 V;
  pf->getDataArray( inAddress, V.v, 3 );
  arMatrix4 result(
    ar_rotationMatrix('y',  ar_convertToRad(V.v[0])) *
    ar_rotationMatrix('x',  ar_convertToRad(V.v[1])) *
    ar_rotationMatrix('z',  ar_convertToRad(V.v[2]))
    );
  pf->putDataMatrix( outAddress, result );
  return true;
}

class InverseMatrix : public arPForthAction {
  public:
    virtual bool run( arPForth* pf );
};
bool InverseMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  long outAddress = (long)pf->stackPop();
  long inAddress = (long)pf->stackPop();
  arMatrix4 result( (pf->getDataMatrix( inAddress )).inverse() );
  pf->putDataMatrix( outAddress, result );
  return true;
}
  
class DefAction : public arPForthAction {
  public:
    vector<arPForthAction*> _actionList;
    virtual ~DefAction() {
      _actionList.clear();
    }
    bool run( arPForth* pf );
};
bool DefAction::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  return pf->runSubprogram( _actionList );
}

class DefCompiler : public arPForthCompiler {
  public:
    virtual ~DefCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool DefCompiler::compile( arPForth* pf,
                         vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (pf == 0)
    return false;
  string theName = pf->nextWord();
  if (theName == "PFORTH_NULL_WORD")
    throw arPForthException("end of input reached prematurely.");
  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");
  DefAction* action = new DefAction();
  if (action == 0)
    throw arPForthException("failed to allocate action object.");
  pf->addAction( action );
  string theWord;
  pf->anonymousActionsAreTransient = false;
  while ((theWord = pf->nextWord()) != "enddef")
    if (!pf->compileWord( theWord, action->_actionList ))
      throw arPForthException("failed to compile " + theWord + ".");
  pf->anonymousActionsAreTransient = true;
  arPForthCompiler* compiler = new SimpleCompiler( action );
  if (compiler == 0)
    throw arPForthException("failed to allocate compiler object.");    
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}

class IfAction : public arPForthAction {
  public:
    vector<arPForthAction*> _trueProg, _falseProg;
    virtual ~IfAction() {
      _trueProg.clear();
      _falseProg.clear();
    }
    bool run( arPForth* pf );
};
bool IfAction::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  bool test = ((long)pf->stackPop()) > 0; // >= 1
  if (test)
    return pf->runSubprogram( _trueProg );
  else
    return pf->runSubprogram( _falseProg );
}

class IfCompiler : public arPForthCompiler {
  public:
    virtual ~IfCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool IfCompiler::compile( arPForth* pf,
                         vector<arPForthSpace::arPForthAction*>& actionList ) {
  if (pf == 0)
    return false;
  IfAction* action = new IfAction();
  if (action == 0)
    throw arPForthException("failed to allocate action object.");
  if (pf->anonymousActionsAreTransient)
    pf->addTransientAction( action );
  else
    pf->addAction( action );
  string theWord;
  while (((theWord = pf->nextWord()) != "else") &&
          (theWord != "endif"))
    if (!pf->compileWord( theWord, action->_trueProg ))
      return false;
  if (theWord == "else")
    while ((theWord = pf->nextWord()) != "endif")
      if (!pf->compileWord( theWord, action->_falseProg ))
        return false;
  actionList.push_back( action );
  return true;
}

class StringCompiler : public arPForthCompiler {
  public:
    virtual ~StringCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool StringCompiler::compile( arPForth* pf,
                               vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (pf == 0)
    return false;
  string theName = pf->nextWord();
  if (theName == "PFORTH_NULL_WORD")
    throw arPForthException("end of input reached prematurely.");
  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");
  unsigned long address;
  address = pf->allocateString();
  std::string theString;
  std::string theWord;
  bool firstWord = true;
  while ((theWord = pf->nextWord()) != "endstring") {
    if (theWord == "PFORTH_NULL_WORD")
      throw arPForthException("end of input reached in string constant.");
    if (!firstWord)
      theString = theString + " ";
    theString = theString + theWord;
    firstWord = false;
  }
  pf->putString( address, theString );
//  std::string theWord = pf->peekNextWord();
//  if (theWord != "endstring") {
//    theWord = pf->nextWord();
//    std::string theString;
//    bool firstWord = true;
//    while ((theWord = pf->nextWord()) != "endstring") {
//      if (theWord == "PFORTH_NULL_WORD")
//        throw arPForthException("end of input reached in string constant.");
//      if (!firstWord)
//        theString = theString + " ";
//      theString = theString + theWord;
//      firstWord = false;
//    }
//cerr << "String: " << theString << endl;
//    pf->putString( address, theString );
//  } else {
//    theWord = pf->nextWord();
//  }
  arPForthAction* action = new FetchNumber( (float)address );
  pf->addAction( action );
  arPForthCompiler* compiler = new SimpleCompiler( action );
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}


class CommentCompiler : public arPForthCompiler {
  public:
    virtual ~CommentCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool CommentCompiler::compile( arPForth* pf,
                               vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (pf == 0)
    return false;
  std::string theWord;
  while ((theWord = pf->nextWord()) != "*/") {
    if (theWord == "PFORTH_NULL_WORD")
      throw arPForthException("end of input reached in comment.");
  }
  return true;
}

// Compiler for words that allocate a single cell 
// in the dataspace (determined by constructor arg)
// and initialize it with a value off the stack.
// Note that unlike a variable, the new word returns
// the _value_ passed at compilation time, not and
// address.
class ConstantCompiler : public arPForthCompiler {
  private:
    unsigned long _size;
  public:
    ConstantCompiler() {}
    virtual ~ConstantCompiler() {}
    virtual bool compile( arPForth* pf,
                  vector<arPForthSpace::arPForthAction*>& actionList );
};
bool ConstantCompiler::compile( arPForth* pf,
                               vector<arPForthSpace::arPForthAction*>& /*actionList*/ ) {
  if (pf == 0)
    return false;
  string theName = pf->nextWord();
  if (theName == "PFORTH_NULL_WORD")
    throw arPForthException("end of input reached prematurely.");
  if (pf->findWord( theName ))
    throw arPForthException("word " + theName + " already in dictionary.");
  string theValue = pf->nextWord();
  if (theValue == "PFORTH_NULL_WORD")
    throw arPForthException("end of input reached prematurely.");
  double theDouble;
  float theFloat;
  if (!ar_stringToDoubleValid( theValue, theDouble ))
    throw arPForthException("string->float conversion failed.");
  if (!ar_doubleToFloatValid( theDouble, theFloat ))
    throw arPForthException("string->float conversion failed.");
  arPForthAction* action = new FetchNumber( theFloat );
  pf->addAction( action );
  arPForthCompiler* compiler = new SimpleCompiler( action );
  pf->addCompiler( compiler );
  return pf->addDictionaryEntry( arPForthDictionaryEntry( theName, compiler ));
}


bool ar_PForthAddStandardVocabulary( arPForth* pf ) {
  arPForthCompiler* compiler;
  
  // A few words with compile-time behaviors
  
  compiler = new ConstantCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "constant", compiler ) ))
    return false;
  compiler = new VariableCompiler(1);
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "variable", compiler ) ))
    return false;
  compiler = new VariableCompiler(16);
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "matrix", compiler ) ))
    return false;
  compiler = new VariableCompiler(3);
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "vector", compiler ) ))
    return false;
  compiler = new ArrayCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "array", compiler ) ))
    return false;
    
  compiler = new StringCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "string", compiler ) ))
    return false;

  compiler = new CommentCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "/*", compiler ) ))
    return false;

  compiler = new DefCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "define", compiler ) ))
    return false;
  compiler = new PlaceHolderCompiler("enddef");
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "enddef", compiler ) ))
    return false;

  compiler = new IfCompiler();
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "if", compiler ) ))
    return false;
  compiler = new PlaceHolderCompiler("else");
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "else", compiler ) ))
    return false;
  compiler = new PlaceHolderCompiler("endif");
  if (compiler==0)
    return false;
  pf->addCompiler( compiler );
  if (!pf->addDictionaryEntry( arPForthDictionaryEntry( "endif", compiler ) ))
    return false;

  // Simple action words (run-time behavior only)
  if (!pf->addSimpleActionWord( "+", new Add() ))
    return false;
  if (!pf->addSimpleActionWord( "-", new Subtract() ))
    return false;
  if (!pf->addSimpleActionWord( "*", new Multiply() ))
    return false;
  if (!pf->addSimpleActionWord( "/", new Divide() ))
    return false;
  if (!pf->addSimpleActionWord( "fetch", new Fetch() ))
    return false;
  if (!pf->addSimpleActionWord( "store", new Store() ))
    return false;
  if (!pf->addSimpleActionWord( "dup", new Duplicate() ))
    return false;
  if (!pf->addSimpleActionWord( "=", new Equals() ))
    return false;
  if (!pf->addSimpleActionWord( "<", new LessThan() ))
    return false;
  if (!pf->addSimpleActionWord( ">", new GreaterThan() ))
    return false;
  if (!pf->addSimpleActionWord( "<=", new LessEquals() ))
    return false;
  if (!pf->addSimpleActionWord( ">=", new GreaterEquals() ))
    return false;
  if (!pf->addSimpleActionWord( "stringEquals", new StringEquals() ))
    return false;
  if (!pf->addSimpleActionWord( "not", new Not() ))
    return false;
  if (!pf->addSimpleActionWord( "vectorStore", new VecStore() ))
    return false; 
  if (!pf->addSimpleActionWord( "vectorCopy", new VecCopy() ))
    return false; 
  if (!pf->addSimpleActionWord( "vectorAdd", new AddVectors() ))
    return false; 
  if (!pf->addSimpleActionWord( "vectorSubtract", new SubVectors() ))
    return false; 
  if (!pf->addSimpleActionWord( "vectorScale", new VecScalarMultiply() ))
    return false; 
  if (!pf->addSimpleActionWord( "vectorTransform", new MatVecMultiply() ))
    return false; 
  if (!pf->addSimpleActionWord( "translationMatrixV", new TranslationMatrixFromVector() ))
    return false; 
  if (!pf->addSimpleActionWord( "extractTranslation", new ExtractTranslationVector() ))
    return false; 
  if (!pf->addSimpleActionWord( "identityMatrix", new IdentityMatrix() ))
    return false; 
  if (!pf->addSimpleActionWord( "matrixStore", new MatStore() ))
    return false; 
  if (!pf->addSimpleActionWord( "matrixStoreTranspose", new MatStoreTranspose() ))
    return false; 
  if (!pf->addSimpleActionWord( "matrixCopy", new MatCopy() ))
    return false; 
  if (!pf->addSimpleActionWord( "matrixMultiply", new MatMultiply() ))
    return false;  
  if (!pf->addSimpleActionWord( "concatMatrices", new ConcatMatrices() ))
    return false;  
  if (!pf->addSimpleActionWord( "translationMatrix", new TranslationMatrix() ))
    return false;  
  if (!pf->addSimpleActionWord( "rotationMatrix", new RotationMatrix() ))
    return false;  
  if (!pf->addSimpleActionWord( "rotationMatrixV", new RotationMatrixV() ))
    return false;  
  if (!pf->addSimpleActionWord( "rotationMatrixEuler", new RotationMatrixFromEulerAngles() ))
    return false;  
  if (!pf->addSimpleActionWord( "extractTranslationMatrix", new ExtractTranslationMatrix() ))
    return false;  
  if (!pf->addSimpleActionWord( "extractRotationMatrix", new ExtractRotationMatrix() ))
    return false;  
  if (!pf->addSimpleActionWord( "extractEulerAngles", new ExtractEulerAngles() ))
    return false;  
  if (!pf->addSimpleActionWord( "inverseMatrix", new InverseMatrix() ))
    return false;  
  if (!pf->addSimpleActionWord( "arrayAdd", new AddArrays() ))
    return false;  
  if (!pf->addSimpleActionWord( "arraySubtract", new SubArrays() ))
    return false;  
  if (!pf->addSimpleActionWord( "arrayMultiply", new MultArrays() ))
    return false;  
  if (!pf->addSimpleActionWord( "arrayDivide", new DivArrays() ))
    return false;  
  if (!pf->addSimpleActionWord( "xaxis", new FetchNumber(0) ))
    return false;
  if (!pf->addSimpleActionWord( "yaxis", new FetchNumber(1) ))
    return false;
  if (!pf->addSimpleActionWord( "zaxis", new FetchNumber(2) ))
    return false;
  if (!pf->addSimpleActionWord( "stack", new StackPrint() ))
    return false;
  if (!pf->addSimpleActionWord( "clearStack", new ClearStack() ))
    return false;
  if (!pf->addSimpleActionWord( "dataspace", new DataspacePrint() ))
    return false;
  if (!pf->addSimpleActionWord( "stringspace", new StringspacePrint() ))
    return false;
  if (!pf->addSimpleActionWord( "printString", new StringPrint() ))
    return false;
  if (!pf->addSimpleActionWord( "printArray", new ArrayPrint() ))
    return false;
  if (!pf->addSimpleActionWord( "printMatrix", new MatrixPrint() ))
    return false;
  if (!pf->addSimpleActionWord( "printVector", new VectorPrint() ))
    return false;
  return true;
}    

} // namespace arPForthSpace

