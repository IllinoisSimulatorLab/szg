#include "arPrecompiled.h"
#include "arDataUtilities.h"
#include "arExperimentDataField.h"
#include <iostream>
#include <math.h>
using std::cerr;

//double arExperimentDataField::_floatCompPrecision = 1.e-6;
double ___floatCompPrecision = 1.e-6;

void arExperimentDataField::setFloatCompPrecision( double x ) {
  ___floatCompPrecision = x;
}

arExperimentDataField::arExperimentDataField( const std::string name, 
                                              const arDataType typ,
                                              const void* ad, const unsigned int s,
                                               bool ownPtr ) :
  _name( name ),
  _type( typ ),
  _address( const_cast<void*>(ad) ),
  _size( s ),
  _dataSize( s ),
  _ownPtr( ownPtr ) {
    if (!_address) {
      _ownPtr = true;
      _size = 0;
      _dataSize = 0;
    }
}

arExperimentDataField::arExperimentDataField( const arExperimentDataField& x ) :
  _name( x._name ),
  _type( x._type ),
  _size( x._size ),
  _dataSize( x._dataSize ),
  _ownPtr( x._ownPtr ) {
    if (_ownPtr) {
      if (_dataSize == 0) {
        _address = 0;
        return;
      }
      _address = ar_allocateBuffer( _type, _dataSize );
      if (!_address) {
        _size = 0;
        _dataSize = 0;
        return;
      }
      // Note: as far as user is concerned, there are _size elements stored here,
      // not _dataSize.
      ar_copyBuffer( _address, x._address, _type, _size );
    } else {
      _address = x._address;
    }
}
arExperimentDataField& arExperimentDataField::operator=( const arExperimentDataField& x ) {
  if (&x == this)
    return *this;
  _name = x._name;
  _type = x._type;
  _size = x._size;
  _dataSize = x._dataSize;
  if (_ownPtr && _address) {
    ar_deallocateBuffer( _address );
  }
//  _ownPtr = x._ownPtr;
  if (_ownPtr) {
    if (_dataSize == 0) {
      _address = 0;
      return *this;
    }
    _address = ar_allocateBuffer( _type, _dataSize );
    if (!_address) {
      _size = 0;
      _dataSize = 0;
      return *this;
    }
//    ar_copyBuffer( _address, x._address, _type, _size );
  } else {
    //    If I don't own the pointer, I copy the value into it
//    _address = x._address;
  }
  ar_copyBuffer( _address, x._address, _type, _size );
  return *this;
}

arExperimentDataField::~arExperimentDataField() {
  if (_ownPtr && _address) {
    ar_deallocateBuffer( _address );
  }
}

bool arExperimentDataField::operator==( const arExperimentDataField& x ) {
  if (_name != x._name)
    return false;
  if (_type != x._type)
    return false;
  if (_size != x._size)
    return false;
  if (_size > 0) {
    unsigned int i;
    switch (_type) {
      case AR_DOUBLE:
        { // This brace is to keep VC++ from barfing about "initialization of x skipped by CASE"
          double* d = static_cast<double*>(_address);
          double* d2 = static_cast<double*>(x._address);
          for (i=0; i<_size; ++i) {
            if (fabs(d[i]-d2[i]) > ___floatCompPrecision) {
//              cerr << "arExperimentDataField remark: diff = " << fabs(d[i]-d2[i])
//                   << ", prec. = " << ___floatCompPrecision << endl;
              return false;
            }
          }
        }
        break;
      case AR_FLOAT:
        {
          float* f = static_cast<float*>(_address);
          float* f2 = static_cast<float*>(x._address);
          for (i=0; i<_size; ++i) {
            if (fabs(f[i]-f2[i]) > ___floatCompPrecision) {
              return false;
            }
          }
        }
        break;
      case AR_LONG:
        {
          long* l = static_cast<long*>(_address);
          long* l2 = static_cast<long*>(x._address);
          for (i=0; i<_size; ++i) {
            if (l[i] != l2[i]) {
              return false;
            }
          }
        }
        break;
      case AR_INT:
        {
          int* j = static_cast<int*>(_address);
          int* j2 = static_cast<int*>(x._address);
          for (i=0; i<_size; ++i) {
            if (j[i] != j2[i]) {
              return false;
            }
          }
        }
        break;
      case AR_CHAR:
        {
          char* c = static_cast<char*>(_address);
          char* c2 = static_cast<char*>(x._address);
          for (i=0; i<_size; ++i) {
            if (c[i] != c2[i]) {
              return false;
            }
          }
        }
        break;
    }
  }
  return true;
}

bool arExperimentDataField::operator!=( const arExperimentDataField& x ) {
  return !operator==(x);
}

// NOTE: this returns a valid pointer even if size == 0; it sets _dataSize to 1 and
// allocates a buffer of that size. Note that getSize() returns the value of _size,
// which in that case would be 0, reflecting the amount of storage requested.
// Of course, if the user owns the pointer, none of this happens...
void* const arExperimentDataField::makeStorage( unsigned int size ) {
  if (!_ownPtr) {
    cerr << "arExperimentDataField(" << _name << ") error: attempt to makeStorage() with non-owned pointer.\n";
    return static_cast<void* const>(0);
  }
  unsigned int dataSize( size );
  if (size == 0) {
    dataSize = 1;
  }
  if (dataSize == _dataSize) {
    _size = size;
    return _address;
  }
  _dataSize = dataSize;
  _size = size;
  if (_address) {
    ar_deallocateBuffer( _address );
  }
  _address = ar_allocateBuffer( _type, _dataSize );
  if (!_address) {
    cerr << "arExperimentDataField(" << _name << ") error: failed to allocate buffer.\n";
    _size = 0;
    _dataSize = 0;
    return static_cast<void* const>(0);
  }
  return _address;
}

bool arExperimentDataField::setData( arDataType typ, const void* const address, unsigned int size ) {
  if (typ != _type) {
    cerr << "arExperimentDataField error: attempt to setData() for field " << _name 
         << " with type " << arDataTypeName(typ) << ", should be " << arDataTypeName(_type) << endl;
    return false;
  }
  if (_ownPtr) {
    void* ptr;
    if (typ == AR_CHAR) {
      ptr = const_cast<void*>(makeStorage( size+1 ));
    } else {
      ptr = const_cast<void*>(makeStorage( size ));
    }
    if (!ptr) {
      cerr << "arExperimentDataField error: makeStorage() failed in setData().\n";
      return false;
    }
  } else {
    if (typ == AR_CHAR) {
      if (size > _size) {
        cerr << "arExperimentDataField error: attempt to setData() for CHAR field " << _name
             << " (owned by calling program) with to large a size\n    (called with "
             << size << ", should be <= " << _size << endl;
        return false;
      }
    } else {
      if (size != _size) {
        cerr << "arExperimentDataField error: attempt to setData() for field " << _name
             << " (owned by calling program) with incorrect size\n    (called with "
             << size << ", should be " << _size << endl;
        return false;
      }
    }
  }
  ar_copyBuffer( _address, address, typ, size );
  if (typ == AR_CHAR) {
    *(((char*)_address)+size) = '\0';
  }
  return true;
}

bool arExperimentDataField::sameNameType( const arExperimentDataField& x ) {
  return (x._name == _name) && (x._type == _type);
}

ostream& operator<<(ostream& s, const arExperimentDataField& d) {
  s << "DataField( " << d._name << ", " << arDataTypeName(d._type) << ", " << d._address << ", " << d._size;
  if (d._size > 0) {
    s << ", ";
    unsigned int i;
    int* p;
    long* q;
    float* r;
    double* t;
    char* u;
    switch (d._type) {
      case AR_INT: 
        p = static_cast<int*>(d._address);
        for (i=0; i<d._size; i++) {
          s << p[i] << " ";
        }
        break;
      case AR_LONG: 
        q = static_cast<long*>(d._address);
        for (i=0; i<d._size; i++) {
          s << q[i] << " ";
        }
        break;
      case AR_FLOAT: 
        r = static_cast<float*>(d._address);
        for (i=0; i<d._size; i++) {
          s << r[i] << " ";
        }
        break;
      case AR_DOUBLE: 
        t = static_cast<double*>(d._address);
        for (i=0; i<d._size; i++) {
          s << t[i] << " ";
        }
        break;
      case AR_CHAR: 
        u = static_cast<char*>(d._address);
        for (i=0; i<d._size; i++) {
          s << u[i];
        }
        break;
      default:
        ;
    }
  }
  s << ") ";
  return s;
}


