// $Id: PyTypemaps.i,v 1.1 2005/03/18 20:13:01 crowell Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

// convert Python strings to C++ strings
%typemap(in) const string& {
    $1 = new string(PyString_AsString($input));
}

// The following typemap is needed to make the above typemap work with
// overloaded functions.
%typemap(typecheck) const string& = char *;

// avoid memory leaks...
%typemap(freearg) const string& {
  free((string *) $1);
}

// convert C++ strings to Python strings
// More specifically, convert C++ strings to char**; SWIG will take care
// of the conversion from char** to Python strings.
%typemap(out) string {
    $result=PyString_FromString($1.c_str());
}

// Convert a python list of ints to a const vector<long>& (input)
%typemap(in) (const vector<long>&) {
  if (PyList_Check($input)) {
    $1 = new vector<long>;
    if (!$1) {
      PyErr_SetString(PyExc_MemoryError,"failed to make new vector<long>");
      return NULL;
    }
    int size = PyList_Size($input);
    for (int i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyLong_Check(o)) {
        $1->push_back( PyInt_AsLong(o) );
      } else {
        PyErr_SetString(PyExc_TypeError,"list must contain ints");
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}
%typemap(freearg) (const vector<long>&) {
   if ($1) delete $1;
}

// Convert a python list of floats to a const vector<double>& (input)
%typemap(in) (const vector<double>&) {
  if (PyList_Check($input)) {
    $1 = new vector<double>;
    if (!$1) {
      PyErr_SetString(PyExc_MemoryError,"failed to make new vector<double>");
      return NULL;
    }
    int size = PyList_Size($input);
    for (int i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyFloat_Check(o)) {
        $1->push_back( PyFloat_AsDouble(o) );
      } else {
        PyErr_SetString(PyExc_TypeError,"list must contain floats");
        return NULL;
      }
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}
%typemap(freearg) (const vector<double>&) {
   if ($1) delete $1;
}

// convert a vector<string> to a python list of strings (output)
%typemap(out) vector<string> {
  $result = PyList_New(0);
  if ($result != NULL) {
    vector<string>::iterator iter;
    for (iter = $1.begin(); iter != $1.end(); ++iter) {
      PyList_Append( $result, PyString_FromString( iter->c_str() ) );
    }
  }
}
    
// convert a vector<long> to a python list of ints (output)
%typemap(out) vector<long> {
  $result = PyList_New(0);
  if ($result != NULL) {
    vector<long>::iterator iter;
    for (iter = $1.begin(); iter != $1.end(); ++iter) {
      PyList_Append( $result, PyInt_FromLong( *iter ) );
    }
  }
}
    
// convert a vector<double> to a python list of floats (output)
%typemap(out) vector<double> {
  $result = PyList_New(0);
  if ($result != NULL) {
    vector<double>::iterator iter;
    for (iter = $1.begin(); iter != $1.end(); ++iter) {
      PyList_Append( $result, PyFloat_FromDouble( *iter ) );
    }
  }
}
    

// convert Python lists to int*
%typemap(in) int * {
    int size = PyList_Size($input);
    int i = 0;
    $1 = new int[size];
    for (i = 0; i < size; i++)
        $1[i]=PyInt_AsLong(PyList_GetItem($input,i));
}

// perform typecheck to make the above typemap work with overloaded functions
%typemap(typecheck) int * = PyObject *;

// avoid memory leaks...
%typemap(freearg) int * {
  free((int *) $1);
}

// This typemap makes sure that passing Python floats will not cause
// floating point exceptions in Syzygy; note that Python floats can
// represent much bigger values that C++ floats
%typemap(check) float {
%#ifdef CHECK_OVERFLOW
    if (isinf($1) || isnan($1)) {
        SWIG_exception(SWIG_OverflowError,"Floating point value out of range");
    }
%#endif
}

// Make sure that only meaningful floats get passed back to Python.
%typemap(out) float {
%#ifdef CHECK_OVERFLOW
    if (isinf($1) || isnan($1)) {
        SWIG_exception(SWIG_OverflowError,"Floating point value out of range");
    }
%#endif
    $result=PyFloat_FromDouble($1);
}

// convert Python lists to float*
%typemap(in) float * {
    int size = PyList_Size($input);
    int i = 0;
    $1 = new float[size];
    for (i = 0; i < size; i++) {
        $1[i]=PyFloat_AsDouble(PyList_GetItem($input,i));
%#ifdef CHECK_OVERFLOW
        // Check result for floating point exceptions
        if (isinf($1[i]) || isnan($1[i])) {
            SWIG_exception(SWIG_OverflowError,
                        "Floating point value out of range");
        }
%#endif
    }
}

// perform typecheck to make the above typemap work with overloaded functions
%typemap(typecheck) float * = PyObject *;

// avoid memory leaks...
%typemap(freearg) float * {
  free((float *) $1);
}

// convert a list of Python strings to (int argc,char** argv)
// Note that one Python argument gets mapped to _two_ C++ arguments.
// Typemaps can do this and more!
// Reality check: Does this typemap have a memory leak?
// JAC 1/25/05 added freearg typemap.
%typemap(in) (int&,char **) (int size) {
  if (PyList_Check($input)) {
    size = PyList_Size($input);
    int i = 0;
    $1 = &size;
    $2 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
    $2[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
    PyErr_SetString(PyExc_TypeError,"list must contain strings");
    free($2);
    return NULL;
      }
    }
    $2[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}
%typemap(freearg) (int&, char **) (int size) {
   if ($2) free($2);
}

// Typemap for callback functions in PyMasterSlave.i
// (probably still needs a lot of work...)
%typemap(python,in) PyObject *PyFunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}

