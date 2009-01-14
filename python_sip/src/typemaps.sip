%MappedType string
{
%TypeHeaderCode
#include <string>
using namespace std;
%End
%ConvertFromTypeCode
  return PyString_FromString( sipCpp->c_str() );
%End
%ConvertToTypeCode
// See if we are just being asked to check the type of the Python
// object.
if (sipIsErr == NULL) {
  // Checking whether or not None has been passed instead of a list
  // has already been done.
  return PyString_Check(sipPy);
}
char* buf = PyString_AsString(sipPy);
*sipCppPtr = new string(buf);

// The instance should be regarded as temporary (and be destroyed as
// soon as it has been used) unless it has been transferred from
// Python.  sipGetState() is a convenience function that implements
// this common transfer behaviour.
return sipGetState(sipTransferObj);
%End
};

// vector<string> is implemented as a Python list of strings.
%MappedType vector<string>
{
%TypeHeaderCode
#include <vector>
%End

%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the list elements.
  vector<string>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyObject *pobj;
    string& s = *iter;
    if ((pobj = PyString_FromStringAndSize(s.c_str(),static_cast<int>(s.size()))) == NULL) {
      Py_DECREF(l);
      return NULL;
    }
    PyList_SET_ITEM(l, i, pobj);
    ++i;
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL)
    return PyList_Check(sipPy);
  vector<string> *v = new vector<string>;
  char *buf;
  Py_ssize_t len(0);
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    PyString_AsStringAndSize( PyList_GET_ITEM(sipPy,i), &buf, &len );
    v->push_back(string( const_cast<const char*>(buf), static_cast<size_t>(len) ));
  }
  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End
};


// vector<double> is implemented as a Python list.
%MappedType vector<double>
{
%TypeHeaderCode
#include <vector>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the vector.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the vector elements.
  vector<double>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyList_SET_ITEM(l, i++, PyFloat_FromDouble( *iter ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!PyFloat_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  vector<double> *qv = new vector<double>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    qv->push_back( PyFloat_AsDouble(PyList_GET_ITEM(sipPy, i)) );
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};


// vector<long> is implemented as a Python list.
%MappedType vector<long>
{
%TypeHeaderCode
#include <vector>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the vector.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the vector elements.
  vector<long>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyList_SET_ITEM(l, i++, PyInt_FromLong( *iter ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!PyInt_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  vector<long> *qv = new vector<long>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    qv->push_back( PyInt_AsLong(PyList_GET_ITEM(sipPy, i)) );
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};


// vector<int> is implemented as a Python list.
%MappedType vector<int>
{
%TypeHeaderCode
#include <vector>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the vector.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the vector elements.
  vector<int>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyList_SET_ITEM(l, i++, PyInt_FromLong( static_cast<long>(*iter) ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!PyInt_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  vector<int> *qv = new vector<int>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    qv->push_back( static_cast<int>( PyInt_AsLong(PyList_GET_ITEM(sipPy, i)) ) );
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};


// vector<unsigned> is implemented as a Python list.
%MappedType vector<unsigned>
{
%TypeHeaderCode
#include <vector>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the vector.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the vector elements.
  vector<unsigned>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyList_SET_ITEM(l, i++, PyInt_FromLong( static_cast<long>(*iter) ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!PyInt_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  vector<unsigned> *qv = new vector<unsigned>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    qv->push_back( static_cast<unsigned>( PyInt_AsLong(PyList_GET_ITEM(sipPy, i)) ) );
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};


// vector<TYPE> is implemented as a Python list.
template<TYPE>
%MappedType vector<TYPE>
{
%TypeHeaderCode
#include <vector>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the vector.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the vector elements.
  vector<TYPE>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    TYPE *t = new TYPE(*iter);
    PyObject *tobj;
    if ((tobj = sipConvertFromNewInstance(t, sipClass_TYPE, sipTransferObj)) == NULL) {
      Py_DECREF(l);
      delete t;
      return NULL;
    }
    PyList_SET_ITEM(l, i++, tobj);
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!sipCanConvertToInstance(PyList_GET_ITEM(sipPy, i), sipClass_TYPE, SIP_NOT_NONE))
        return 0;
    return 1;
  }
  vector<TYPE> *qv = new vector<TYPE>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    int state;
    TYPE *t = reinterpret_cast<TYPE *>(sipConvertToInstance(PyList_GET_ITEM(sipPy, i), sipClass_TYPE, sipTransferObj, SIP_NOT_NONE, &state, sipIsErr));
    if (*sipIsErr) {
      sipReleaseInstance(t, sipClass_TYPE, state);
      delete qv;
      return 0;
    }
    qv->push_back(*t);
    sipReleaseInstance(t, sipClass_TYPE, state);
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};





// list<TYPE> is implemented as a Python list.
template<TYPE>
%MappedType list<TYPE>
{
%TypeHeaderCode
#include <list>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the list elements.
  list<TYPE>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    TYPE *t = new TYPE(*iter);
    PyObject *tobj;
    if ((tobj = sipConvertFromNewInstance(t, sipClass_TYPE, sipTransferObj)) == NULL) {
      Py_DECREF(l);
      delete t;
      return NULL;
    }
    PyList_SET_ITEM(l, i++, tobj);
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!sipCanConvertToInstance(PyList_GET_ITEM(sipPy, i), sipClass_TYPE, SIP_NOT_NONE))
        return 0;
    return 1;
  }
  list<TYPE> *qv = new list<TYPE>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    int state;
    TYPE *t = reinterpret_cast<TYPE *>(sipConvertToInstance(PyList_GET_ITEM(sipPy, i), sipClass_TYPE, sipTransferObj, SIP_NOT_NONE, &state, sipIsErr));
    if (*sipIsErr) {
      sipReleaseInstance(t, sipClass_TYPE, state);
      delete qv;
      return 0;
    }
    qv->push_back(*t);
    sipReleaseInstance(t, sipClass_TYPE, state);
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};


// list<string> is implemented as a Python list of strings.
%MappedType list<string>
{
%TypeHeaderCode
#include <list>
%End

%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the list elements.
  list<string>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyObject *pobj;
    string& s = *iter;
    if ((pobj = PyString_FromStringAndSize(s.c_str(),static_cast<int>(s.size()))) == NULL) {
      Py_DECREF(l);
      return NULL;
    }
    PyList_SET_ITEM(l, i, pobj);
    ++i;
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL)
    return PyList_Check(sipPy);
  list<string> *v = new list<string>;
  char *buf;
  Py_ssize_t len(0);
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    PyString_AsStringAndSize( PyList_GET_ITEM(sipPy,i), &buf, &len );
    v->push_back(string( const_cast<const char*>(buf), static_cast<size_t>(len) ));
  }
  *sipCppPtr = v;
  return sipGetState(sipTransferObj);
%End
};



// list<int> is implemented as a Python list.
%MappedType list<int>
{
%TypeHeaderCode
#include <list>
using namespace std;
%End

%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size())) == NULL)
    return NULL;
  // Set the list elements.
  list<int>::iterator iter;
  int i(0);
  for (iter = sipCpp->begin(); iter != sipCpp->end(); ++iter) {
    PyList_SET_ITEM(l, i++, PyInt_FromLong( static_cast<long>(*iter) ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i)
      if (!PyInt_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  list<int> *qv = new list<int>;
  for (int i = 0; i < PyList_GET_SIZE(sipPy); ++i) {
    qv->push_back( static_cast<int>( PyInt_AsLong(PyList_GET_ITEM(sipPy, i)) ) );
  }
  *sipCppPtr = qv;
  return sipGetState(sipTransferObj);
%End
};


%ModuleCode
template<class T> class _NumberArray {
  public:
    _NumberArray( int numVals ) :
      size(numVals) {
      data = new T[numVals];
    }
    ~_NumberArray() { delete[] data; }
    int size;
    T* data;
};

typedef _NumberArray<float> arFloatArray;
typedef _NumberArray<int> arIntArray;
typedef _NumberArray<long> arLongArray;
typedef _NumberArray<double> arDoubleArray;
%End

// arFloatArray is implemented as a Python list.
%MappedType arFloatArray
{
%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size)) == NULL)
    return NULL;
  // Set the list elements.
  float* ptr = sipCpp->data;
  for (int i=0; i<sipCpp->size; ++i) {
    PyList_SET_ITEM(l, i, PyFloat_FromDouble( static_cast<double>(ptr[i]) ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  int size(0);
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    size = PyList_GET_SIZE(sipPy);
    for (int i = 0; i < size; ++i)
      if (!PyFloat_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  arFloatArray* a = new arFloatArray(size);
  float* ptr = a->data;
  for (int i = 0; i < size; ++i) {
    ptr[i] = static_cast<float>( PyFloat_AsDouble(PyList_GET_ITEM(sipPy, i)) );
  }
  *sipCppPtr = a;
  return sipGetState(sipTransferObj);
%End
};

// arDoubleArray is implemented as a Python list.
%MappedType arDoubleArray
{
%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size)) == NULL)
    return NULL;
  // Set the list elements.
  double* ptr = sipCpp->data;
  for (int i=0; i<sipCpp->size; ++i) {
    PyList_SET_ITEM(l, i, PyFloat_FromDouble( ptr[i] ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  int size(0);
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    size = PyList_GET_SIZE(sipPy);
    for (int i = 0; i < size; ++i)
      if (!PyFloat_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  arDoubleArray* a = new arDoubleArray(size);
  double* ptr = a->data;
  for (int i = 0; i < size; ++i) {
    ptr[i] = PyFloat_AsDouble(PyList_GET_ITEM(sipPy, i));
  }
  *sipCppPtr = a;
  return sipGetState(sipTransferObj);
%End
};

// arIntArray is implemented as a Python list.
%MappedType arIntArray
{
%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size)) == NULL)
    return NULL;
  // Set the list elements.
  int* ptr = sipCpp->data;
  for (int i=0; i<sipCpp->size; ++i) {
    PyList_SET_ITEM(l, i, PyInt_FromLong( static_cast<long>(ptr[i]) ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  int size(0);
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    size = PyList_GET_SIZE(sipPy);
    for (int i = 0; i < size; ++i)
      if (!PyInt_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  arIntArray* a = new arIntArray(size);
  int* ptr = a->data;
  for (int i = 0; i < size; ++i) {
    ptr[i] = static_cast<int>( PyInt_AsLong(PyList_GET_ITEM(sipPy, i)) );
  }
  *sipCppPtr = a;
  return sipGetState(sipTransferObj);
%End
};


// arLongArray is implemented as a Python list.
%MappedType arLongArray
{
%ConvertFromTypeCode
  // Create the list.
  PyObject *l;
  if ((l = PyList_New(sipCpp->size)) == NULL)
    return NULL;
  // Set the list elements.
  long* ptr = sipCpp->data;
  for (int i=0; i<sipCpp->size; ++i) {
    PyList_SET_ITEM(l, i, PyInt_FromLong( ptr[i] ) );
  }
  return l;
%End

%ConvertToTypeCode
  // Check the type if that is all that is required.
  int size(0);
  if (sipIsErr == NULL) {
    if (!PyList_Check(sipPy))
      return 0;
    size = PyList_GET_SIZE(sipPy);
    for (int i = 0; i < size; ++i)
      if (!PyInt_Check( PyList_GET_ITEM(sipPy, i) ))
        return 0;
    return 1;
  }
  arLongArray* a = new arLongArray(size);
  long* ptr = a->data;
  for (int i = 0; i < size; ++i) {
    ptr[i] = PyInt_AsLong(PyList_GET_ITEM(sipPy, i));
  }
  *sipCppPtr = a;
  return sipGetState(sipTransferObj);
%End
};



