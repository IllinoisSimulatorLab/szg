//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_BUFFER_H
#define AR_BUFFER_H

/// Utility class. Useful for the many times one wants a simple,
/// resizeable array of elements (i.e. when dealing with moving buffers of
/// network data).

// IMPORTANT NOTE: DO NOT USE THE arBuffer template class in USER CODE!
// THIS BREAKS THE ABILITY TO REDEFINE THE SHARED LIBRARY!

template<class T> class arBuffer{
 // Needs assignment operator and copy constructor, for pointer member.
 public:
  arBuffer(int numElements = 1);
  ~arBuffer();
  
  int size() const { return _numElements; }
  void resize(int);
  void grow(int);
  void push(T);

  T* data;
  int pushPosition;
 private:
  int _numElements;
};

template<class T> arBuffer<T>::arBuffer(int numElements){
  if (numElements < 1){
    cerr << "warning: arBuffer initialized with nonpositive size "
         << numElements << endl;
    numElements = 1;
  }
  _numElements = numElements;
  data = new T[_numElements];

  // This is where a new element will be pushed.
  pushPosition = 0;
}

template <class T> arBuffer<T>::~arBuffer(){
  delete [] data;
}

template <class T> void arBuffer<T>::resize(int numElements){
  if (numElements<1){
    cerr << "warning: arBuffer resized with nonpositive size "
         << numElements << endl;
    numElements=1;
  }
  T* newPtr = new T[numElements];
  memset(newPtr, 0,  numElements * sizeof(T));
  if (numElements < _numElements){
    cerr << "warning: arBuffer shrinking from "
         << _numElements << " to " << numElements << " elements.\n";
    _numElements = numElements; // we're shrinking, not growing!
  }
  memcpy(newPtr, data, _numElements * sizeof(T));
  // NOTE: this is a little bit of a change over the arLightFloatBuffer
  // implementation. There, we actually delete the data pointer!
  T* temp = data;
  data = newPtr;
  _numElements = numElements;
  delete [] temp;
}

template <class T> void arBuffer<T>::grow(int numElements){
  if (numElements > size())
    resize(numElements);
}

template <class T> void arBuffer<T>::push(T element){
  if (pushPosition >= _numElements){
    // This isn't strictly correct... 
    grow(2*_numElements + 1);
  }
  data[pushPosition] = element;
  pushPosition++;
}

#endif
