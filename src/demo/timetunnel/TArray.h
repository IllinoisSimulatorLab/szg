/*
 * This file is part of the Pablo Performance Analysis Environment
 *
 *          (R)
 * The Pablo    Performance Analysis Environment software is NOT in
 * the public domain.  However, it is freely available without fee for
 * education, research, and non-profit purposes.  By obtaining copies
 * of this and other files that comprise the Pablo Performance Analysis
 * Environment, you, the Licensee, agree to abide by the following
 * conditions and understandings with respect to the copyrighted software:
 * 
 * 1.  The software is copyrighted in the name of the Board of Trustees
 *     of the University of Illinois (UI), and ownership of the software
 *     remains with the UI. 
 *
 * 2.  Permission to use, copy, and modify this software and its documentation
 *     for education, research, and non-profit purposes is hereby granted
 *     to Licensee, provided that the copyright notice, the original author's
 *     names and unit identification, and this permission notice appear on
 *     all such copies, and that no charge be made for such copies.  Any
 *     entity desiring permission to incorporate this software into commercial
 *     products should contact:
 *
 *          Professor Daniel A. Reed                 reed@cs.uiuc.edu
 *          University of Illinois
 *          Department of Computer Science
 *          2413 Digital Computer Laboratory
 *          1304 West Springfield Avenue
 *          Urbana, Illinois  61801
 *          USA
 *
 * 3.  Licensee may not use the name, logo, or any other symbol of the UI
 *     nor the names of any of its employees nor any adaptation thereof in
 *     advertizing or publicity pertaining to the software without specific
 *     prior written approval of the UI.
 *
 * 4.  THE UI MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THE
 *     SOFTWARE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS
 *     OR IMPLIED WARRANTY.
 *
 * 5.  The UI shall not be liable for any damages suffered by Licensee from
 *     the use of this software.
 *
 * 6.  The software was developed under agreements between the UI and the
 *     Federal Government which entitle the Government to certain rights.
 *
 **************************************************************************
 *
 * Developed by: The Pablo Research Group
 *               University of Illinois at Urbana-Champaign
 *               Department of Computer Science
 *               1304 W. Springfield Avenue
 *               Urbana, IL     61801
 *
 *               http://www-pablo.cs.uiuc.edu
 *
 * Send comments to: pablo-feedback@guitar.cs.uiuc.edu
 *
 * Copyright (c) 1987-1997
 * The University of Illinois Board of Trustees.
 *      All Rights Reserved.
 *
 * PABLO is a registered trademark of
 * The Board of Trustees of the University of Illinois
 * registered in the U.S. Patent and Trademark Office.
 *
 * Author: Eric Shaffer (shaffer1@cs.uiuc.edu)
 *
 * Project Manager and Principal Investigator:
 *      Daniel A. Reed (reed@cs.uiuc.edu)
 *
 * Funded in part by DARPA contracts DABT63-96-C-0027, DABT63-96-C-0161 
 * and by the National Science Foundation under grants IRI 92-12976 and
 * NSF CDA 94-01124
 */

#ifndef TARRAY_H
#define TARRAY_H

#include <assert.h>
#include <iostream>
using namespace std;

#define NO_CAVE


/***************************************************************************/
/* Class: TArray -- A semi-safe template class for arrays                  */
/***************************************************************************/

template<class DATATYPE>
class TArray {

  enum { GROWTH_RATE = 2};         // The array defaults to growing by a
                                   // factor of 2 when resized

  private:

    unsigned long _dim;                      // Size of the array
    DATATYPE *_data;               // Ptr to the storage space for data 

    
  public:

    TArray(unsigned long size = 1);          // Default constructor

    TArray(const DATATYPE *data);  // Constructor allows 
                                   // initialization with a 
                                   // "regular" array

    TArray(const TArray &initArray);  // Copy constructor

    virtual ~TArray();

    unsigned long size() const;                 // Return size of array  

    void resize(unsigned long newSize);            // Change array size

    void resize();                                 // Resize array to 
						   // by making 
                                                   // it bigger by a constant
                                                   // factor	

    TArray& swap(unsigned long index1, unsigned long index2);   // Swap 2 elements
    
    TArray& rightShift();           // Circularly shift indices up by 1
  
    TArray& leftShift();            // Circularly shift indices down by 1

    /*******************************************************************/
    /* Operators on TArray                                             */
    /*******************************************************************/

    DATATYPE& operator[](unsigned int index);

    TArray& operator = ( const DATATYPE *D);

    TArray& operator = ( const TArray& copyArray);

};

/**************************************************************************/
// Access elements via an index 

template<class DATATYPE>
inline DATATYPE& TArray<DATATYPE>::operator[](unsigned int index) {
  if ( !( index < _dim) ){

    cerr << "ERROR TArray::operator[] : Out of bounds index " << index 
	 << " with array dimension of " << _dim << "\n"; 
    exit(0);
  }

  return _data[index];
} 

/**************************************************************************/
// Assignment with a regular array

template<class DATATYPE>
TArray<DATATYPE>& TArray<DATATYPE>::operator = ( const DATATYPE *D){
  unsigned long i;
  
  unsigned long DSize = ( sizeof(D)/sizeof(DATATYPE) );
  
  if (DSize != _dim){
    resize(DSize);
  }

  for(i=0; i<_dim; i++){
    _data[i] = D[i];
  }
  
  return *this;
}


/**************************************************************************/
// Assignment to other TArray

template<class DATATYPE>
TArray<DATATYPE>& TArray<DATATYPE>::operator =
       ( const TArray<DATATYPE>& copyArray){
	 unsigned long i;
  
	 resize(copyArray._dim);
	 
	 for(i=0; i<_dim; i++)
	   _data[i] = copyArray._data[i];
	 
	 return *this;
}    


/**************************************************************************/
/* Member functions for TArray                                            */
/**************************************************************************/
// Default Constructor 

    template<class DATATYPE>
    inline TArray<DATATYPE>::TArray(unsigned long size): _dim(size) {

      if (_dim != 0) {
	_data = new DATATYPE[_dim];

      }
    }


/**************************************************************************/
// Construct TArray initialed to the value of a "regular" array

    template<class DATATYPE>
    TArray<DATATYPE>::TArray(const DATATYPE *data){
      unsigned long i;
      
      _dim = ( sizeof(D)/sizeof(DATATYPE) );

      _data = new DATATYPE[_dim];
  
      for(i=0; i<_dim; i++) {
	_data[i] = data[i];
      }
    }


/**************************************************************************/
//  Copy Constructor    

    template<class DATATYPE>
    TArray<DATATYPE>::TArray(const TArray<DATATYPE> &initArray)
      : _dim(initArray._dim){
	unsigned long i;
        

	_data = new DATATYPE[_dim];
	for(i=0; i<_dim; i++)
	  _data[i] = initArray._data[i];
    }


/**************************************************************************/
// Destructor 

    template<class DATATYPE>
    inline TArray<DATATYPE>::~TArray(){
     
     delete [] _data;
    }


/**************************************************************************/
// Return size
   

    template<class DATATYPE>
    inline unsigned long TArray<DATATYPE>::size() const{
      return _dim;
    }

/**************************************************************************/
// Change the size of the array

    template<class DATATYPE>
    void TArray<DATATYPE>::resize(unsigned long newSize){
      unsigned long i;
      unsigned long copyIndex;
      DATATYPE *newData;
     
     
      // Allocate new array
      
      #ifndef NO_CAVE
	   newData = (DATATYPE* ) CAVEMalloc( newSize * sizeof(DATATYPE) );
      #else
           newData = new DATATYPE[newSize];
      #endif
    
      copyIndex = _dim;                  // Copy the old into the new
      if (newSize < _dim) {
	copyIndex = newSize;
      }
      
      for(i=0; i<copyIndex; i++) {
	    newData[i] = _data[i];
      }

      _dim = newSize; 

      #ifndef NO_CAVE
      CAVEFree(_data);
      #else
      delete [] _data;
      #endif

      _data = newData;
    }

/**************************************************************************/
// Change the size of the array to twice current size

    template<class DATATYPE>
    void TArray<DATATYPE>::resize(){
      unsigned long i;
      unsigned long copyIndex;
      DATATYPE *newData;
      
      unsigned long newSize = GROWTH_RATE * _dim;  

      #ifndef NO_CAVE
	   newData = (DATATYPE* ) CAVEMalloc( newSize * sizeof(DATATYPE) );
      #else
           newData = new DATATYPE[newSize];
      #endif
      
      
      copyIndex = _dim;                  // Copy the old into the new
      if (newSize < _dim) {
	copyIndex = newSize;
      }
      
      for(i=0; i<copyIndex; i++) {
	    newData[i] = _data[i];
      }

      _dim = newSize; 

      #ifndef NO_CAVE
      CAVEFree(_data);
      #else
      delete [] _data;
      #endif

      _data = newData;
    }

/**************************************************************************/
// Swap two elements

    template<class DATATYPE>
    inline  TArray<DATATYPE>& TArray<DATATYPE>::swap(unsigned long index1, 
                                                     unsigned long index2) {
      DATATYPE temp;
      
      temp = _data[index1];
      _data[index1] = _data[index2];
      _data[index2] = temp;
      
      return *this;
    }


/**************************************************************************/
// Circularly shift up an index

    template<class DATATYPE>
    TArray<DATATYPE>& TArray<DATATYPE>::rightShift() {
      unsigned long i;
      DATATYPE temp;

      temp = _data[_dim - 1];
      
      for(i=(_dim-1); i>0; i--)
	_data[i] = _data[i-1];
      _data[0] = temp;

      return *this;
    }


/**************************************************************************/
// Circularly shift down an index

    template<class DATATYPE>
    TArray<DATATYPE>& TArray<DATATYPE>::leftShift() {
      unsigned long i;
      DATATYPE temp;

      temp = _data[0];
      
      for(i=0; i< (_dim - 1); i++)
	_data[i] = _data[i+1];
      _data[_dim-1] = temp;

      return *this;
    }


#endif












