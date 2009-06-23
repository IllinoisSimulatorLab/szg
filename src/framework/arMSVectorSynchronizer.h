//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARMSVECTORSYNC_H
#define ARMSVECTORSYNC_H

//********************************************************
//
// Template class for simplifying synchronization of a vector<> of objects
// between master and slaves. Objects can be added to or deleted from the
// vector<> on the master and the slave's vector<> will be resized appropriately.
// Values of specified float and int fields will also be automatically
// synchronized between corresponding vector<> elements. Example:
// 
// Declaration:
// 
//   std::vector<Leaf> leavesGlobal;
//   arMSVectorSynchronizer<Leaf> leafSyncGlobal( leavesGlobal );
// 
// The Leaf class must have a default constructor.
// Setting framework pointer and transfer fields:
// 
//   leafSyncGlobal.setFramework(&fw);
//   leafSyncGlobal.addFloatField( "leafMatrices",
//                                 &Leaf::setMatixFromBuf,
//                                 &Leaf::getMatixToBuf, 16 );
//   leafSyncGlobal.addIntField( "leafClicked",
//                               &Leaf::setClickedFromBuf,
//                               &Leaf::getClickedToBuf );
// 
// The final parameter indicate the number of elements/object,
// defaults to 1. For a matrix of course we have 16 floats.
// Calls to add...Field() _REPLACE_ the corresponding
// calls to arMasterSlaveFramework::addTransferField() (they call
// the newer arMasterSlaveFramework::addInternalTransferField() internally).
// Leaf get and set functions are declared as follows:
// (bug: why Matix not Matrix?)
// 
//   bool setMatixFromBuf( const float* const buf );
//   bool getMatixToBuf( float* const buf );
//   bool setClickedFromBuf( const int* const buf );
//   bool getClickedToBuf( int* const buf );
// 
// Synchronization: in the preExchange() callback on the master, call
// 
//   leafSyncGlobal.sendData();
// 
// and in postExchange() on the slaves, call:
// 
//   leafSyncGlobal.receiveData();
// 
// That is all.
//
//********************************************************

#include "arMasterSlaveFramework.h"
#include "arDataUtilities.h"
#include <vector>
#include <string>

template <class ObjectClass, class DataClass>
class arMSVecTransferField {
  public:
  typedef bool (ObjectClass::*arMSVecSetterFunc_t)( const DataClass* const );
  typedef bool (ObjectClass::*arMSVecGetterFunc_t)( DataClass* const );

  std::string fieldName;
  arMSVecSetterFunc_t setter;
  arMSVecGetterFunc_t getter;
  unsigned int numElements;
  
  arMSVecTransferField( const std::string& msFieldName,
               arMSVecSetterFunc_t setterFunc,
               arMSVecGetterFunc_t getterFunc,
               unsigned int num ) :
    fieldName(msFieldName),
    setter(setterFunc),
    getter(getterFunc),
    numElements(num) {}
};

template <class ObjectClass>
class arMSVectorSynchronizer  {
  public:
    
//    template <class DataClass>
//    typedef std::vector< arMSVecTransferField<ObjectClass, DataClass> > arMSTransferFieldContainer_t;
      
    typedef std::vector<ObjectClass> arMSVecObjectContainer_t;
      
    arMSVectorSynchronizer( arMSVecObjectContainer_t& vec, arMasterSlaveFramework* fw=NULL ) :
      _vector(vec),
      _framework(fw) {
      _floatFields.reserve(10);
      _intFields.reserve(10);
    }
    virtual ~arMSVectorSynchronizer() {
      _floatFields.clear();
      _intFields.clear();
    }
    void setFramework( arMasterSlaveFramework* fw ) {
      _framework = fw;
    }
    bool addFloatField( const std::string& msFieldName,
               typename arMSVecTransferField<ObjectClass, float>::arMSVecSetterFunc_t setterFunc,
               typename arMSVecTransferField<ObjectClass, float>::arMSVecGetterFunc_t getterFunc,
               unsigned int numElements = 1 );
    bool addIntField( const std::string& msFieldName,
               typename arMSVecTransferField<ObjectClass, int>::arMSVecSetterFunc_t setterFunc,
               typename arMSVecTransferField<ObjectClass, int>::arMSVecGetterFunc_t getterFunc,
               unsigned int numElements = 1 );
    bool sendData();
    bool receiveData();
  private:
//    template <class DataClass>
//      bool _sendData( std::vector< arMSVecTransferField<ObjectClass, DataClass> >& fields, arDataType dtype );
//    template <class DataClass>
//      bool _receiveData( std::vector< arMSVecTransferField<ObjectClass, DataClass> >& fields, arDataType dtype );
    void _adjustContainerSize( unsigned int newSize );
    arMSVecObjectContainer_t& _vector;
    arMasterSlaveFramework* _framework;
    std::vector< arMSVecTransferField<ObjectClass, float> > _floatFields;
    std::vector< arMSVecTransferField<ObjectClass, int> > _intFields;
};

template <class ObjectClass>
bool arMSVectorSynchronizer<ObjectClass>::addFloatField(
               const std::string& msFieldName,
               typename arMSVecTransferField<ObjectClass, float>::arMSVecSetterFunc_t setterFunc,
               typename arMSVecTransferField<ObjectClass, float>::arMSVecGetterFunc_t getterFunc,
               unsigned int numElements ) {
  if (!_framework) {
    cerr << "arMSVectorSynchronizer error: NULL framework pointer.\n";
    return false;
  }
  if (!_framework->addInternalTransferField( msFieldName, AR_FLOAT, numElements ) ) {
    cerr << "arMSVectorSynchronizer error: addInternalTransferField() failed.\n";
    return false;
  }
  _floatFields.push_back( arMSVecTransferField<ObjectClass, float>
    ( msFieldName, setterFunc, getterFunc, numElements) );
  return true;
}

template <class ObjectClass>
bool arMSVectorSynchronizer<ObjectClass>::addIntField( const std::string& msFieldName,
                 typename arMSVecTransferField<ObjectClass, int>::arMSVecSetterFunc_t setterFunc,
                 typename arMSVecTransferField<ObjectClass, int>::arMSVecGetterFunc_t getterFunc,
                 unsigned int numElements ) {
  if (!_framework) {
    cerr << "arMSVectorSynchronizer error: NULL framework pointer.\n";
    return false;
  }
  if (!_framework->addInternalTransferField( msFieldName, AR_INT, numElements ) ) {
    cerr << "arMSVectorSynchronizer error: addInternalTransferField() failed.\n";
    return false;
  }
  _intFields.push_back( arMSVecTransferField<ObjectClass, int>
    ( msFieldName, setterFunc, getterFunc, numElements) );
  return true;
}

template <class ObjectClass>
bool arMSVectorSynchronizer<ObjectClass>::sendData() {
  if (!_framework) {
    cerr << "arMSVectorSynchronizer error: NULL framework pointer.\n";
    return false;
  }
  if (!_framework->getMaster()) {
    cerr << "arMSVectorSynchronizer warning: ignoring sendData() on slave.\n";
    return true;
  }

  typename arMSVecObjectContainer_t::iterator vecIter;
  int intFieldSize;
  unsigned int fieldSize;
  
  typename std::vector< arMSVecTransferField<ObjectClass, float> >::iterator floatIter;
  for (floatIter = _floatFields.begin(); floatIter != _floatFields.end(); ++floatIter) {
    float* floatPtr = static_cast<float*>( _framework->getTransferField( floatIter->fieldName, AR_FLOAT, intFieldSize ) );
    if (!floatPtr) {
      cerr << "arMSVectorSynchronizer error: getTransferField() failed for field "
           << floatIter->fieldName << endl;
      return false;
    }
    // pain in the arse, all these buffer dimensions returned as ints
    fieldSize = static_cast<unsigned int>( intFieldSize );
    if (fieldSize != _vector.size()*floatIter->numElements) {
      if (!_framework->setInternalTransferFieldSize( floatIter->fieldName, AR_FLOAT, _vector.size()*floatIter->numElements )) {
        cerr << "arMSVectorSynchronizer error: setInternalTransferFieldSize() failed for field "
             << floatIter->fieldName << endl;
        return false;
      }
      floatPtr = static_cast<float*>( _framework->getTransferField( floatIter->fieldName, AR_FLOAT, intFieldSize ) );
      if (!floatPtr) {
        cerr << "arMSVectorSynchronizer error: second getTransferField() failed for field "
             << floatIter->fieldName << endl;
        return false;
      }
      fieldSize = static_cast<unsigned int>( intFieldSize );
      if (fieldSize != _vector.size()*floatIter->numElements) {
        cerr << "arMSVectorSynchronizer error: setInternalTransferFieldSize() failed"
             << " to change size of field " << floatIter->fieldName << endl
             << "  (" << fieldSize << ", should be "
             << _vector.size()*floatIter->numElements << ")\n";
        return false;
      }
    }
    for (vecIter = _vector.begin(); vecIter != _vector.end(); ++vecIter) {
      if (!CALL_MEMBER_FUNCTION( *vecIter, floatIter->getter )( const_cast<float* const>(floatPtr) )) {
        cerr << "arMSVectorSynchronizer error: call to getter function failed for field "
             << floatIter->fieldName << endl;
        return false;
      }
      floatPtr += floatIter->numElements;
    }
  }  
  typename std::vector< arMSVecTransferField<ObjectClass, int> >::iterator intIter;
  for (intIter = _intFields.begin(); intIter != _intFields.end(); ++intIter) {
    int* intPtr = static_cast<int*>( _framework->getTransferField( intIter->fieldName, AR_INT, intFieldSize ) );
    if (!intPtr) {
      cerr << "arMSVectorSynchronizer error: getTransferField() failed for field "
           << intIter->fieldName << endl;
      return false;
    }
    // pain in the arse, all these buffer dimensions returned as ints
    fieldSize = static_cast<unsigned int>( intFieldSize );
    if (fieldSize != _vector.size()*intIter->numElements) {
      if (!_framework->setInternalTransferFieldSize( intIter->fieldName, AR_INT, _vector.size()*intIter->numElements )) {
        cerr << "arMSVectorSynchronizer error: setInternalTransferFieldSize() failed for field "
             << intIter->fieldName << endl;
        return false;
      }
      intPtr = static_cast<int*>( _framework->getTransferField( intIter->fieldName, AR_INT, intFieldSize ) );
      if (!intPtr) {
        cerr << "arMSVectorSynchronizer error: second getTransferField() failed for field "
             << intIter->fieldName << endl;
        return false;
      }
      fieldSize = static_cast<unsigned int>( intFieldSize );
      if (fieldSize != _vector.size()*intIter->numElements) {
        cerr << "arMSVectorSynchronizer error: setInternalTransferFieldSize() failed"
             << " to change size of field " << intIter->fieldName << endl
             << "  (" << fieldSize << ", should be "
             << _vector.size()*intIter->numElements << ")\n";
        return false;
      }
    }
    for (vecIter = _vector.begin(); vecIter != _vector.end(); ++vecIter) {
      if (!CALL_MEMBER_FUNCTION( *vecIter, intIter->getter )( const_cast<int* const>(intPtr) )) {
        cerr << "arMSVectorSynchronizer error: call to getter function failed for field "
             << intIter->fieldName << endl;
        return false;
      }
      intPtr += intIter->numElements;
    }
  }
  return true;
}

template <class ObjectClass>
bool arMSVectorSynchronizer<ObjectClass>::receiveData() {
  if (!_framework) {
    cerr << "arMSVectorSynchronizer error: NULL framework pointer.\n";
    return false;
  }
  if (_framework->getMaster()) {
    cerr << "arMSVectorSynchronizer warning: ignoring receiveData() on master.\n";
    return true;
  }
  
  typename arMSVecObjectContainer_t::iterator vecIter;
  int intFieldSize;
  unsigned int fieldSize;
  
  typename std::vector< arMSVecTransferField<ObjectClass, float> >::iterator floatIter;
  for (floatIter = _floatFields.begin(); floatIter != _floatFields.end(); ++floatIter) {
    float* floatPtr = static_cast<float*>( _framework->getTransferField( floatIter->fieldName, AR_FLOAT, intFieldSize ) );
    fieldSize = static_cast<unsigned int>( intFieldSize );
    if (fieldSize == 0) {
      _vector.clear();
    } else {
      if (!floatPtr) {
        cerr << "arMSVectorSynchronizer error: getTransferField() failed for field "
             << floatIter->fieldName << endl;
        return false;
      }
      if (fieldSize != _vector.size()*floatIter->numElements) {
        if (floatIter == _floatFields.begin()) {
          _adjustContainerSize( fieldSize/floatIter->numElements );
        } else {
          cerr << "arMSVectorSynchronizer error: incorrect size for field "
               << floatIter->fieldName << endl;
          return false;
        }
      }
      for (vecIter = _vector.begin(); vecIter != _vector.end(); ++vecIter) {
        if (!CALL_MEMBER_FUNCTION( *vecIter, floatIter->setter )( const_cast<const float* const>(floatPtr) )) {
          cerr << "arMSVectorSynchronizer error: call to getter function failed for field "
               << floatIter->fieldName << endl;
          return false;
        }
        floatPtr += floatIter->numElements;
      }
    }
  }

  typename std::vector< arMSVecTransferField<ObjectClass, int> >::iterator intIter;
  for (intIter = _intFields.begin(); intIter != _intFields.end(); ++intIter) {
    int* intPtr = static_cast<int*>( _framework->getTransferField( intIter->fieldName, AR_INT, intFieldSize ) );
    if (fieldSize == 0) {
      _vector.clear();
    } else {
      if (!intPtr) {
        cerr << "arMSVectorSynchronizer error: getTransferField() failed for field "
             << intIter->fieldName << endl;
        return false;
      }
      fieldSize = static_cast<unsigned int>( intFieldSize );
      if (fieldSize != _vector.size()*intIter->numElements) {
        if (intIter == _intFields.begin()) {
          _adjustContainerSize( fieldSize/intIter->numElements );
        } else {
          cerr << "arMSVectorSynchronizer error: incorrect size for field "
               << intIter->fieldName << endl;
          return false;
        }
      }
      for (vecIter = _vector.begin(); vecIter != _vector.end(); ++vecIter) {
        if (!CALL_MEMBER_FUNCTION( *vecIter, intIter->setter )( const_cast<const int* const>(intPtr) )) {
          cerr << "arMSVectorSynchronizer error: failed to call to getter function for field "
               << intIter->fieldName << endl;
          return false;
        }
        intPtr += intIter->numElements;
      }
    }
  }
  return true;
}

template <class ObjectClass>
void arMSVectorSynchronizer<ObjectClass>::_adjustContainerSize( unsigned int newSize ) {
  if (_vector.size() > newSize) {
    _vector.erase( _vector.begin()+newSize, _vector.end() );
  } else if (_vector.size() < newSize) {
    _vector.insert( _vector.end(), newSize-_vector.size(), ObjectClass() );
  }
}


#endif        //  #ifndefARMSVECTORSYNC_H

