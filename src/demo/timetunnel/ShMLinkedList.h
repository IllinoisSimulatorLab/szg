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

#ifndef SHMLINKEDLIST_H
#define SHMLINKEDLIST_H

#include <assert.h>
#include <iostream>
using namespace std;



/**************************************************************************/
/* Template class for a doubly linked list that uses shared memory        */
/**************************************************************************/

// The SGI CC compiler needs to see 
// LinkedList before the friend declaration

template<class NODETYPE>
class ShMLinkedList;

template<class NODETYPE>
class ShMListIterator;

/**************************************************************************/
/* ShMListNode class declaration                                             */
/**************************************************************************/

template<class NODETYPE>
class ShMListNode
{
  friend class ShMLinkedList<NODETYPE>;
  private:
    NODETYPE _data;
    ShMListNode *_next;
    ShMListNode *_prev;

  public:
    ShMListNode(const NODETYPE &);
    NODETYPE &getData();
    ShMListNode<NODETYPE> *next();
    ShMListNode<NODETYPE> *prev();
};

/**************************************************************************/
/* Inlined functions for ShMListNode                                         */
/**************************************************************************/

// Constructor

template<class NODETYPE>
inline ShMListNode<NODETYPE>::ShMListNode(const NODETYPE &info)
  : _data(info), _next(0), _prev(0){
}

// Return a reference to to the data

template<class NODETYPE>
inline NODETYPE& ShMListNode<NODETYPE>::getData() 
{
  return _data;
}

template<class NODETYPE>
inline ShMListNode<NODETYPE> * ShMListNode<NODETYPE>::next(){
  return _next;
}

template<class NODETYPE>
inline  ShMListNode<NODETYPE> * ShMListNode<NODETYPE>::prev(){
  return _prev;
}

/**************************************************************************/
/* ShMLinkedList class declaration                                           */
/**************************************************************************/

template<class NODETYPE>
class ShMLinkedList
{
  friend class ShMListIterator<NODETYPE>; 
  private:
    ShMListNode<NODETYPE> *_head;
    ShMListNode<NODETYPE> *_tail;
    ShMListNode<NODETYPE> *_getNewNode(const NODETYPE &);

  public:
    ShMLinkedList();
    ~ShMLinkedList();
    void insertAtEnd(const NODETYPE &);
    void insertAtHead(const NODETYPE &);
    void deleteNode(ShMListNode<NODETYPE> *);
    bool isEmpty() const;
};
/**************************************************************************/
/* Inlined functions for ShMLinkedList                                       */
/**************************************************************************/

// Constructor

template<class NODETYPE>
inline ShMLinkedList<NODETYPE>::ShMLinkedList() 
{
 
  _head = _tail = 0;
}


template<class NODETYPE>
inline bool ShMLinkedList<NODETYPE>::isEmpty() const
{
  return bool (_head == 0);
}

/**************************************************************************/
/* Member functions for ShMLinkedList                                        */
/**************************************************************************/

// Destructor

template<class NODETYPE>
ShMLinkedList<NODETYPE>::~ShMLinkedList() 
{
  ShMListNode<NODETYPE> *crntPtr;
  ShMListNode<NODETYPE> *tempPtr;

  crntPtr = _head;
  
  if (!isEmpty())
  {
    while (crntPtr != 0)
    {
      tempPtr = crntPtr;
      crntPtr = crntPtr->_next;
      delete tempPtr;      
    }
  }
}

/**************************************************************************/
// Insert at end of list

template<class NODETYPE>
void ShMLinkedList<NODETYPE>::insertAtEnd(const NODETYPE &value) 
{
  ShMListNode<NODETYPE> *newPtr = _getNewNode(value);

  if(isEmpty() )
  {
    _tail = newPtr;
    _head = newPtr;
  }
  else
  {
    newPtr->_prev = _tail;
    _tail->_next = newPtr;
    _tail = newPtr;
  }
}

/**************************************************************************/
// Insert at head of list

template<class NODETYPE>
void ShMLinkedList<NODETYPE>::insertAtHead(const NODETYPE &value) 
{
  ShMListNode<NODETYPE> *newPtr = _getNewNode(value);

  if (isEmpty())
    {
       _tail = newPtr;
       _head = newPtr;
    }
  else
    {
      _head->_prev = newPtr;
      newPtr->_next = _head;
      _head = newPtr;
    }
}

/**************************************************************************/
// Delete a given ShMListNode

template<class NODETYPE>
void ShMLinkedList<NODETYPE>::deleteNode(ShMListNode<NODETYPE> *delNode)
{
  // Change "next" link in predecessor
  if (delNode == _head)
    {
      _head = _head->_next;
    }
  else 
    {
      delNode->_prev->_next =  delNode->_next;
    }

  // Change "prev" link in successsor
  if (delNode == _tail)
    {
      _tail = _tail->_prev;
    }
  else 
    {
      delNode->_next->_prev =  delNode->_prev;
    }

  delete delNode;
}
  
/****************************************************************************/
// Allocate a new ShMListNode

template<class NODETYPE>
ShMListNode<NODETYPE> *ShMLinkedList<NODETYPE>::_getNewNode(const NODETYPE &value)
{
  ShMListNode<NODETYPE> *ptr = new ShMListNode<NODETYPE>(value);
  
  assert(ptr != 0);
  return ptr;
}

/**************************************************************************/
/* Iterator for ShMLinkedList class                                          */
/**************************************************************************/

template<class NODETYPE>
class ShMListIterator {
  
  private:
    const ShMLinkedList<NODETYPE> *_list;
    ShMListNode<NODETYPE> *_currentItem;
    
  public:
    ShMListIterator( ShMLinkedList<NODETYPE> *listPtr);
    ShMListIterator();
    NODETYPE& getItem() const;  
    ShMListNode<NODETYPE> *getNode() const;
    
    void setList( ShMLinkedList<NODETYPE> *); 
    void prev();
    void next();
    void first();
    void last(); 
    bool isDone() const;
};

/**************************************************************************/
/* Inlined functions for ShMListIterator                                     */
/**************************************************************************/


template<class NODETYPE>
inline ShMListIterator<NODETYPE>::ShMListIterator(ShMLinkedList<NODETYPE>
						  *listPtr)
  :_list(listPtr), _currentItem(listPtr->_head) {
}

template<class NODETYPE>
inline ShMListIterator<NODETYPE>::ShMListIterator()
  :_list(NULL), _currentItem(NULL) {
   
}

template<class NODETYPE>
inline NODETYPE&  ShMListIterator<NODETYPE>::getItem() const{
  return (_currentItem->getData());
}

template<class NODETYPE>
inline ShMListNode<NODETYPE> *ShMListIterator<NODETYPE>::getNode() const{
  return (_currentItem);
}

template<class NODETYPE>
inline void ShMListIterator<NODETYPE>::setList( ShMLinkedList<NODETYPE> *listPtr){
  _list = listPtr;
  _currentItem = listPtr->_head;
}
 
template<class NODETYPE>
inline void ShMListIterator<NODETYPE>::prev(){
  if (_currentItem != NULL)
    _currentItem = _currentItem->prev();
}

template<class NODETYPE>
inline void ShMListIterator<NODETYPE>::next(){
  if (_currentItem != NULL)
    _currentItem = _currentItem->next();
}

template<class NODETYPE>
inline void ShMListIterator<NODETYPE>::first(){
  _currentItem = _list->_head;
}

template<class NODETYPE>
inline void ShMListIterator<NODETYPE>::last(){
  _currentItem = _list->_tail;
}

template<class NODETYPE>
inline bool ShMListIterator<NODETYPE>::isDone() const{
  return bool (_currentItem == NULL);
}

#endif









