
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
 * Author: Ben Schaeffer (schaeffr@isl.uiuc.edu)
 *
 * Project Manager and Principal Investigator:
 *      Daniel A. Reed (reed@cs.uiuc.edu)
 *
 * Funded in part by DARPA contracts DABT63-96-C-0027, DABT63-96-C-0161 
 * and by the National Science Foundation under grants IRI 92-12976 and
 * NSF CDA 94-01124
 */


#ifndef TIMETUNNEL_LAYOUT_H
#define TIMETUNNEL_LAYOUT_H

#include "ShMLinkedList.h"
#include "TArray.h"
#include "arMath.h"
#include "arDataUtilities.h"
#include "arThread.h"
#include <stdio.h>

/***********************************************************************/
// Class MessageEvent
// 
// This tells us when a message was sent

class MessageEvent
{
public:
  long task;
  double time;
};

//***********************************************************************
// Class VrQueuedTTEvent
// 
// This keeps information about a link for which we don't yet have
// both the 'from' and 'to' ends.  Lists of these are kept for each 'from'
// task, so we do not need the 'from' value as part of the data in the class.

class VrQueuedTTEvent
{
public:
   int toTask;
   int tag;
   int parameter; // to keep from having to write a bunch of small classes
                  // VrQueuedTTEvent is re-used in funny ways... this slot
                  // is included for re-use in the "immediate drawing" of
                  // procedure trace events
   double time;
};

/***********************************************************************/
// Class TimeTunnelLayout
//
// Holds the basic time tunnel parameters

class TimeTunnelLayout
{
 public:
    TimeTunnelLayout( float,float,int,float,string); 
    virtual ~TimeTunnelLayout();  

    long getNumberTunnelEdges();

    virtual void drawLayout();

    virtual void animateLayout();

   protected:
      // Linked lists of 'from' information for which no matching 'to' events
      // have been seen. These lists are indexed using the 'from' identifier.
      ShMLinkedList< VrQueuedTTEvent >* _fromQList;
      ShMListIterator< VrQueuedTTEvent >* _fromQListItr;

      // Linked lists of 'to' information for which no matching 'from' events
      // have been seen.  These lists are indexed using the 'from' identifier.
      ShMLinkedList< VrQueuedTTEvent >* _toQList;
      ShMListIterator< VrQueuedTTEvent >* _toQListItr;

    // each task needs a linked list to manage which vertex to recycle next
    ShMLinkedList< MessageEvent >* deletionList;
    ShMListIterator< MessageEvent >* deletionListItr;

    // each task needs a list of edges that are still in the process of being
    // drawn
    ShMLinkedList< VrQueuedTTEvent >* _delayDrawList;    
    ShMListIterator< VrQueuedTTEvent >* _delayDrawItr;

    // since procedure-end-events can sometimes arrive before 
    // procedure-begin-events (remember that data can come in over the
    // network), we need the following
    ShMLinkedList< VrQueuedTTEvent >* _missingProcBeginList;
    ShMListIterator< VrQueuedTTEvent >* _missingProcBeginItr;

    // for the purposes of drawing nested edges, each timeline keeps
    // track of the greatest time during which an event has been recorded
    float* _greatestEventTime;

    // for the purposes of drawing nested edges, each timeline keeps
    // track of the current level (i.e. nesting) on which edges are drawn
    int* _currentLevel;

    // finally, we need a list of the vertices waiting to be recycled
    ShMLinkedList< long > recyclingList;
    ShMListIterator< long > recyclingListItr;
   
    // the radius of the tunnel
    double radius;
    // the sweep of time covered by the Tunnel
    double timeLength;
    // the base time sweep... modifiable by an attached control
    double baseTimeLength;
    // the visual length of the tunnel
    double visualLength;
    // the number of processors involved
    int numTask;
    // the maximum time for a point currently in the tunnel
    double maxTime;
    // these can improve cache hits
    TArray<bool>     visibleFlag;
    TArray<arVector3> shadowSrcPos;
    TArray<arVector3> shadowDestPos;
    TArray<arVector3> edgeColor;

    FILE* _inputFile;
    long _numberTunnelEdges;
    float _cosLookUp[3600]; // we precalculate values of cos & sin
    float _sinLookUp[3600]; // to the tenth of a degree
    arVector3 _colorLookUp[14]; // codes to RGB values
    // a flag that controls whether we attempt to draw nested edges
    bool _drawNestedEdges;

    arMatrix4 _scrollingTransform;

    // what follows are the data structures related to the primitive
    // distribution
    ARint    _vertexIDs[300];
    ARfloat  _vertexPosition[900];
    ARfloat  _colors[1200];
    int _linePointsID;
    int _colorsID;  
    int _linesID;
    int _transformID;
    bool _instrument;
    int  _numberRecordsSent;
    int  _numberBytesSent;

    arMutex _databaseLock;


  private:

    inline void updateMax( double newTime );

    int addTimeTunnelEdge( int, float, int, float, int, int);
    int addTimeTunnelEdgeNoDelete(int, float, int, float, int, int);
    void processMessageBegin( int,int,int,float,int);
    void processMessageEnd( int,int,int,float,int);
    void processDurationBegin( int, int, float, int );
    void processDurationEnd( int, int, float, int );
    void animateTunnel();
};

/*********************************************************************/
// this deals with the maxTime attribute

void TimeTunnelLayout::updateMax(double newTime)
  {
  if (newTime>maxTime) maxTime=newTime;
  }

#endif


