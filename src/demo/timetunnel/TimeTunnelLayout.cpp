
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

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "TimeTunnelLayout.h"
#include "arGraphicsAPI.h"
#include "arDataUtilities.h"

#ifndef cosf
#define cosf(x) cos(x)
#endif
#ifndef sinf
#define sinf(x) sin(x)
#endif

TimeTunnelLayout::TimeTunnelLayout(float theRadius, float theTimeLength,
                                   int theNumberTasks, float theVisualLength,
                                   string dataPath)
{
  int i;

  ar_mutex_init(&_databaseLock);
  
  radius=theRadius;
  timeLength=theTimeLength;
  baseTimeLength = timeLength;
  numTask=theNumberTasks;
  visualLength=theVisualLength;
  maxTime=0;

  //**********
  //variables for instrumentation
  _instrument = false;
  _numberRecordsSent = 0;
  _numberBytesSent = 0;
  

  //***********************************************************************
  //***********************************************************************
  // need to be have storage for parameters for
  // communications to the database

  arMatrix4 theMatrix = arMatrix4(1,0,0,0,
                                  0,1,0,0,
                                  0,0,visualLength/timeLength,
                                  (visualLength/timeLength)*(-timeLength/2),
                                  0,0,0,1);
  ar_mutex_lock(&_databaseLock);
  _transformID = dgTransform("tunnel trans","world",theMatrix); 
  ar_mutex_unlock(&_databaseLock);

  _vertexPosition[0] = 0; _vertexPosition[1] = 0; _vertexPosition[2] = 0;
  _linePointsID = dgPoints("line points","tunnel trans",1,_vertexPosition);

  _colors[0] = 0; _colors[1] = 0; _colors[2] = 0; _colors[3] = 0;
  _colors[4] = 0; _colors[5] = 0; _colors[6] = 0; _colors[7] = 0;
  _colorsID = dgColor4("line colors","line points",2,_colors);

  _linesID = dgDrawable("lines","line colors",DG_LINES,1);

  //***********************************************************************
  //***********************************************************************

   // finally, initialize the various linked list data members
   _fromQList = new ShMLinkedList< VrQueuedTTEvent > [numTask];
   _fromQListItr = new ShMListIterator< VrQueuedTTEvent > [numTask];
   _toQList = new ShMLinkedList< VrQueuedTTEvent > [numTask];
   _toQListItr = new ShMListIterator< VrQueuedTTEvent > [numTask];
   for ( i = 0; i < numTask; i++ )
   {
      _fromQListItr[i].setList( &_fromQList[i] );
      _toQListItr[i].setList( &_toQList[i] );
   }
  
  deletionList = new ShMLinkedList< MessageEvent > [numTask];
  deletionListItr = new ShMListIterator< MessageEvent > [numTask];
  for (i=0; i<numTask; i++)
    deletionListItr[i].setList(&deletionList[i]);

  _delayDrawList = new ShMLinkedList< VrQueuedTTEvent > [numTask];
  _delayDrawItr = new ShMListIterator< VrQueuedTTEvent > [numTask];
  for (i=0; i<numTask; i++)
    {
    _delayDrawItr[i].setList(&_delayDrawList[i]);
    }

  _missingProcBeginList = new ShMLinkedList< VrQueuedTTEvent >[numTask];
  _missingProcBeginItr = new ShMListIterator< VrQueuedTTEvent >[numTask];
  for (i=0; i<numTask; i++)
    {
    _missingProcBeginItr[i].setList(&_missingProcBeginList[i]);
    }

  _greatestEventTime = new float[numTask];
  _currentLevel = new int[numTask];
  for (i=0; i<numTask; i++)
    {
    _greatestEventTime[i] = 0;
    _currentLevel[i] = 0;
    }
  
  recyclingListItr.setList(&recyclingList);

  //************************************************************************
  // here are some structures which should improve cache hits during drawing
  visibleFlag.resize(128);
  shadowSrcPos.resize(128);
  shadowDestPos.resize(128);
  edgeColor.resize(128);
  //************************************************************************

  _numberTunnelEdges = 0;

  // initialize sin & cos look-up tables
  for (i=0; i<3600; i++)
    {
    _cosLookUp[i] = cosf(6.283*i/3600);
    _sinLookUp[i] = sinf(6.283*i/3600);
    }

  // don't really want to draw nested time tunnel edges
  _drawNestedEdges = false;

  // create the color look-up table 
  
  _colorLookUp[0] = arVector3(1,0,0);
  _colorLookUp[1] = arVector3(1,1,1);
  _colorLookUp[2] = arVector3(0,0,1);
  _colorLookUp[3] = arVector3(0,1,0);
  _colorLookUp[4] = arVector3(1,0,1);
  _colorLookUp[5] = arVector3(1,1,0);
  _colorLookUp[6] = arVector3(0.6,0.6,1);
  _colorLookUp[7] = arVector3(0,0,0);
  _colorLookUp[8] = arVector3(0.6,0.6,0.3);
  _colorLookUp[9] = arVector3(0.6,0.6,0.6);
  _colorLookUp[10] = arVector3(1,0,1);
  _colorLookUp[11] = arVector3(0,0,0.7);
  _colorLookUp[12] = arVector3(0,0.7,0);

  // finally, let's open the data file
  _inputFile = ar_fileOpen("TimeTunnelData","timetunnel",dataPath,"rb");
}

/*********************************************************************/
// Time Tunnel layout class destructor

TimeTunnelLayout::~TimeTunnelLayout()
{
   delete [] _fromQList;
   delete [] _fromQListItr;
   delete [] _toQList;
   delete [] _toQListItr;
   delete [] deletionList;
   delete [] deletionListItr;
}

long TimeTunnelLayout::getNumberTunnelEdges(){
  return _numberTunnelEdges;
}

/*********************************************************************/
// This method creates an edge in the tunnel graph. By convention, the
// parameters localTask1 and localTime1 describe the location and time
// of the end of the event (nothing is added to the deletion queue)

int TimeTunnelLayout::addTimeTunnelEdgeNoDelete(int localTask1,
                                                float localTime1,
                                                int localTask2,
                                                float localTime2,
                                                int lineColor,
                                                int theLevel)
  {
  long thisEdgeID;
  arVector3 location1, location2;

  // recycle or create the vertex
  bool creationFlag = false;
  recyclingListItr.first();
  if ( recyclingListItr.isDone() )
    {
    // must create a new edge
    // note that the ID of the new edge is the edge count of 
    // the graph currently
   
    thisEdgeID = _numberTunnelEdges;
    creationFlag = true;
    }
  else
    {
    // can recycle an old vertex, saving valuable electrons
    thisEdgeID = recyclingListItr.getItem();
    recyclingList.deleteNode( recyclingListItr.getNode() );
    }

  float factor = (40 - theLevel) / 40.0;
  
  location1=arVector3
    ( radius * factor * _cosLookUp[ (int) (3600.0*localTask1)/numTask ],
      radius * factor * _sinLookUp[ (int) (3600.0*localTask1)/numTask ],
      -localTime1
    );
  location2=arVector3
    ( radius * factor * _cosLookUp[ (int) (3600.0*localTask2)/numTask ],
      radius * factor * _sinLookUp[ (int) (3600.0*localTask2)/numTask ],
      -localTime2
    );

  if (thisEdgeID >= (int) shadowSrcPos.size())
    {
    visibleFlag.resize(2*visibleFlag.size());
    shadowSrcPos.resize(2*shadowSrcPos.size());
    shadowDestPos.resize(2*shadowDestPos.size());
    edgeColor.resize(2*edgeColor.size());
    //*****************************************************************
    //*****************************************************************
    // ugly kludge #1... we resize the tunnel by sending a dummy vertex
    // first, resize the points array
    _vertexIDs[0] = 4*shadowSrcPos.size();
    _vertexPosition[0] = 0; _vertexPosition[1] = 0;
    _vertexPosition[2] = 0;
    dgPoints(_linePointsID,1,_vertexIDs,_vertexPosition);
    _colors[0] = 0; _colors[1] = 0; _colors[2] = 0; _colors[3] = 0;
    dgColor4(_colorsID,1,_vertexIDs,_colors);
    if (_instrument){
      _numberRecordsSent += 2;
    } 
    //*****************************************************************
    //*****************************************************************
    }

  visibleFlag[thisEdgeID]=true;
  shadowSrcPos[thisEdgeID]=location1;
  shadowDestPos[thisEdgeID]=location2;
  edgeColor[thisEdgeID] = _colorLookUp[lineColor];

  //*******************************************************************
  //*******************************************************************
  // ugly kludge #2... we send only one new line at a time
  // data for the vertex positions
  _vertexIDs[0] = 2*thisEdgeID;
  _vertexIDs[1] = 2*thisEdgeID+1;
  _vertexPosition[0] = location1[0];
  _vertexPosition[1] = location1[1];
  _vertexPosition[2] = location1[2];
  _vertexPosition[3] = location2[0];
  _vertexPosition[4] = location2[1];
  _vertexPosition[5] = location2[2];
  // data for the visibility of this edge
  // hmmm... I'll kludge this out for now

  // data for the new line
  _colors[0] = _colorLookUp[lineColor][0];
  _colors[1] = _colorLookUp[lineColor][1];
  _colors[2] = _colorLookUp[lineColor][2];
  _colors[3] = 1;
  _colors[4] = _colorLookUp[lineColor][0];
  _colors[5] = _colorLookUp[lineColor][1];
  _colors[6] = _colorLookUp[lineColor][2];
  _colors[7] = 1;

  dgPoints(_linePointsID,2,_vertexIDs,_vertexPosition);
  dgColor4(_colorsID,2,_vertexIDs,_colors);
  dgDrawable(_linesID,DG_LINES,_numberTunnelEdges+1);


  if (_instrument){
      _numberRecordsSent += 2;
    }

  //********************************************************************
  //********************************************************************
 
  if (creationFlag)
    {
    _numberTunnelEdges++;
    }

  return thisEdgeID;
  }

// This method adds the edge to the appropriate deletion queue

/****************************************************************************/

int TimeTunnelLayout::addTimeTunnelEdge(int localTask1,
                                        float localTime1,
                                        int localTask2,
                                        float localTime2,
                                        int lineColor,
                                        int theLevel)
  {
  MessageEvent thisEvent;
  int result = addTimeTunnelEdgeNoDelete(localTask1,
                                         localTime1, localTask2,
                                         localTime2, lineColor, theLevel);
  // need to insert this event into the appropriate deletion queue
  thisEvent.task = result; // note how we change the meaning of task
  thisEvent.time = localTime1;
  deletionList[localTask1].insertAtEnd(thisEvent);
  return result;
  }

//********************************************************************
// This method processes a packet corresponding to the beginning of an
// event in the visualization that involves two tasks.  The packet
// consists of information for the FROM task.

void 
TimeTunnelLayout::processMessageBegin( int fromTask, 
                                       int toTask,
                                       int idTag,
                                       float theTime,
                                       int color )
{

   if ( fromTask < numTask ) 
   {
      VrQueuedTTEvent fromEvent;
      fromEvent.toTask = toTask;
      if ( fromEvent.toTask < numTask ) 
      {
         fromEvent.tag = idTag;
         fromEvent.time = theTime;

         // Look for an already posted TO that matches this FROM
         // It would be filed under it's FROM task.

         bool success = false;
         VrQueuedTTEvent matchEvent;

         _toQListItr[fromTask].first();
         while ( !( _toQListItr[fromTask].isDone() ) && !( success ) )
         {
            matchEvent = _toQListItr[fromTask].getItem();
            if ( ( matchEvent.toTask == fromEvent.toTask ) 
                 && ( matchEvent.tag == fromEvent.tag ) )
            {
               success=true;
            }
            else
            {
               _toQListItr[fromTask].next();
            }
         }

         if ( success )
         {
            // If we have found the matching TO event, delete it from the 
            // list of posted TO's and then draw the FROM/TO arc.  Notice on 
            // the draw, that the TO task and time come before the FROM 
            // task and time in the parameter list.   That is, what we expect
	    // to be the later event and time come first in the argument list.
            ShMListNode< VrQueuedTTEvent >* node = 
						_toQListItr[fromTask].getNode();
            _toQList[fromTask].deleteNode( node );

            addTimeTunnelEdge( matchEvent.toTask,
                               matchEvent.time,
                               fromTask,
                               fromEvent.time,
                               color, 0 );


	   

         }
         else
         {
            // If we have not found the matching TO event, then add this 
            // FROM to the list of outstanding FROM's.
            _fromQList[fromTask].insertAtEnd( fromEvent );
         }
      }
      else
      {
         cerr << "Invalid to Task Number: " << fromEvent.toTask << "\n";
      }
   }
   else
   {
      cerr << "Invalid from Task Number: " << fromTask << "\n";
   }
}

//********************************************************************
// This method processes a packet corresponding to the ending of an
// event in the visualization that involves two tasks.  The packet
// consists of information for the TO task.

void 
TimeTunnelLayout::processMessageEnd( int fromTask,
                                     int toTask,
                                     int idTag,
                                     float theTime,
                                     int color)
{

   if ( fromTask < numTask ) 
   {
      VrQueuedTTEvent toEvent;
      toEvent.toTask = toTask;
      if ( toEvent.toTask < numTask ) 
      {
         toEvent.tag = idTag;
         toEvent.time = theTime;

         // Look for an already posted FROM that matches this TO. 
         // It would be filed under it's FROM task.

         bool success = false;
         VrQueuedTTEvent matchEvent;

         _fromQListItr[fromTask].first();
         while ( !( _fromQListItr[fromTask].isDone() ) && !( success ) )
         {
            matchEvent = _fromQListItr[fromTask].getItem();
            if ( ( matchEvent.toTask == toEvent.toTask ) 
                 && ( matchEvent.tag == toEvent.tag ) )
            {
               success=true;
            }
            else
            {
               _fromQListItr[fromTask].next();
            }
         }
   
         if ( success )
         {
            // If we have found the matching FROM event, delete it from the 
            // list of posted FROM's and then draw the from/to arc.  Notice on
            // the draw, that the TO task and time come before the FROM 
            // task and time in the parameter list.   That is, what we expect
	    // to be the later event and time come first in the argument list.
            ShMListNode< VrQueuedTTEvent >* node = 
				         _fromQListItr[fromTask].getNode();
            _fromQList[fromTask].deleteNode( node );
   
            addTimeTunnelEdge( toEvent.toTask,
                               toEvent.time,
                               fromTask,
                               matchEvent.time,
                               color, 0 );
	   
         }
         else
         {
            // If we have not found the matching FROM event, then add this 
            // TO to the list of outstanding TO's.
            _toQList[fromTask].insertAtEnd( toEvent );
         }
      }
      else
      {
         cerr << "Invalid to Task Number: " << toEvent.toTask << endl;
      }
   }
   else
   {
      cerr << "Invalid from Task Number: " << fromTask << endl;
   }
}

/**********************************************************************/
// This method recenters the Time Tunnel around the viewer and deletes
// and components of the graph that are outside the field of view

void TimeTunnelLayout::animateTunnel()
  {
  int i;
  VrQueuedTTEvent theEvent;
  // if there are any edges being drawn on a delay basis, deal with them
  for (i=0; i<numTask; i++)
    {
    _delayDrawItr[i].first();
    while ( !_delayDrawItr[i].isDone() )
      {
      theEvent = _delayDrawItr[i].getItem();
      // we now extend the edge in question to the current extent
      // of the tunnel
      //**************************************************************
      //**************************************************************
      _vertexIDs[0] = 2*(theEvent.toTask);
      _vertexPosition[0] = shadowSrcPos[ theEvent.toTask ][0];
      _vertexPosition[1] = shadowSrcPos[ theEvent.toTask ][1];
      _vertexPosition[2] = -maxTime;
      ar_mutex_lock(&_databaseLock);
      dgPoints(_linePointsID,1,_vertexIDs,_vertexPosition);
      if (_instrument){
        _numberRecordsSent += 1;
      }
      ar_mutex_unlock(&_databaseLock);
      //**************************************************************
      //**************************************************************
      shadowSrcPos[ theEvent.toTask ][2] = -maxTime;
      _delayDrawItr[i].next();
      }
    }
  // delete any vertices that have z-coordinate too far in the past
  MessageEvent thisEvent;
  for (i=0; i<numTask; i++)
    {
    deletionListItr[i].first();
    if ( !deletionListItr[i].isDone() )
      {
      thisEvent = deletionListItr[i].getItem();
      if ( thisEvent.time < (maxTime-timeLength) )
        {
        //******************************************************
        // here's some code for improving data cache hits
        visibleFlag[thisEvent.task]=false;
        //******************************************************
        recyclingList.insertAtEnd( thisEvent.task );
        deletionList[i].deleteNode( deletionListItr[i].getNode() );
        }
      }
    }

  // now, position the graph appropriately
  // these transformations being correct depends heavily on the order
  // in which analogous transformations are executed in
  // ToolkitObject:: Draw

  _scrollingTransform =  arMatrix4(1,0,0,0,
                              0,1,0,0,
                              0,0,visualLength/timeLength,
                              (visualLength/timeLength)*(maxTime-timeLength/2),
                              0,0,0,1);
  //*********************************************************************
  //*********************************************************************
  arMatrix4 theMatrix = arMatrix4(1,0,0,0,
                              0,1,0,0,
                              0,0,visualLength/timeLength,
                              (visualLength/timeLength)*(maxTime-timeLength/2),
                              0,0,0,1);
  ar_mutex_lock(&_databaseLock);
  dgTransform(_transformID,theMatrix);
  if (_instrument){
    _numberRecordsSent+=1;
  }
  ar_mutex_unlock(&_databaseLock);
  //*********************************************************************
  //*********************************************************************
  if (_instrument){
    cerr << _numberRecordsSent << " " << _numberBytesSent << "\n";
  }
  }

//****************************************************************************
// this routine draws the internal edges and the sliding vertices

void TimeTunnelLayout::drawLayout()
  {
  // now draw the time lines
  glPushMatrix();
  glMultMatrixf(_scrollingTransform.v);
  glDisable(GL_LIGHTING);
  glLineWidth(3.0);
  glBegin(GL_LINES);
  long first = 0;
  long number = _numberTunnelEdges;
  bool amplify = true;
  float threshold = timeLength / (visualLength*100);
  for(int i=first; i< number; i++)
    {
    if (visibleFlag[i])
      {
      if (!amplify)
        {
        glColor3fv(edgeColor[i].v);
        glVertex3fv(shadowSrcPos[i].v);
        glVertex3fv(shadowDestPos[i].v);
	
        }
      else
        {
        if ( ++(shadowSrcPos[i]-shadowDestPos[i]) >= threshold )
          {
          glColor3fv(edgeColor[i].v);
          glVertex3fv(shadowSrcPos[i].v);
          glVertex3fv(shadowDestPos[i].v);
	  
          }
        else
          { 
          glColor3fv(edgeColor[i].v);
          glVertex3fv(shadowSrcPos[i].v);
          arVector3 temp = shadowDestPos[i];
          temp[2] = shadowSrcPos[i][2] - threshold;
          glVertex3fv(temp.v);
	 
          }
        }
      }
    }
  glEnd();
  glEnable(GL_LIGHTING);
  glPopMatrix();
  }

//**************************************************************************
// animate the layout

void TimeTunnelLayout::animateLayout()
  {
  static int reps = 0;
  static int success;
  static char* localBuffer;
  static char theBuffer[2400];
  int code, toTask, fromTask, idTag, color;
  float theTime, duration;
  for (int i=0; i<20; i++)
    {
    if (reps==0)
      {
      success = fread(theBuffer,1,2400,_inputFile);
      localBuffer = theBuffer;
      reps=100;
      }
    reps--;
    if (success)
      {
      code = *( (int*) localBuffer);
      }
    else
      {
      code=-1;
      }
    if (code == 0)
      {
      toTask = *( (int*) (localBuffer+4) );
      theTime = *( (float*) (localBuffer+8) );
      duration = *( (float*) (localBuffer+12) );
      color = *( (int*) (localBuffer+16) );
      updateMax(theTime);
      addTimeTunnelEdge
       (toTask, theTime, toTask, theTime-duration, color, 0);
      }
    else if ( code == 1 )
      {
      fromTask = *( (int*) (localBuffer+4) );
      toTask = *( (int*) (localBuffer+8) );
      idTag = *( (int*) (localBuffer+12) );
      theTime = *( (float*) (localBuffer+16) );
      color = *( (int*) (localBuffer+20) );
      updateMax(theTime);
      processMessageBegin( fromTask, toTask, idTag, theTime, 
                           color );
      }
    else if ( code == 2 )
      { 
      fromTask = *( (int*) (localBuffer+4) );
      toTask = *( (int*) (localBuffer+8) );
      idTag = *( (int*) (localBuffer+12) );
      theTime = *( (float*) (localBuffer+16) );
      color = *( (int*) (localBuffer+20) );   
      updateMax(theTime);
      processMessageEnd(fromTask, toTask, idTag, theTime, 
                        color );
      }
    else if (code == 3)
      {
      toTask = *( (int*) (localBuffer+4) );
      idTag  = *( (int*) (localBuffer+8) );
      theTime   = *( (float*) (localBuffer+12) );
      color  = *( (int*) (localBuffer+16) );
      updateMax( theTime );
      processDurationBegin( toTask, idTag, theTime, color );
      }
    else if (code == 4)
      {
      toTask = *( (int*) (localBuffer+4) );
      idTag  = *( (int*) (localBuffer+8) );
      theTime   = *( (float*) (localBuffer+12) );
      color  = *( (int*) (localBuffer+16) );
      updateMax( theTime );
      processDurationEnd( toTask, idTag, theTime, color );
      }
    
    localBuffer += 24;
    }
  
  // animateTunnel is fairly costly... probably don't want to do it at every
  // record input
  animateTunnel();
  }

void TimeTunnelLayout::processDurationBegin(int task,
                                            int idTag,
                                            float theTime,
                                            int color)
  {
  VrQueuedTTEvent thisEvent;
  if (task < numTask)
    {
    // at most 32 levels of stacking (i.e. 32 bit ints)
    // we use some bitwise operations to efficiently determine
    // the least unused level at any time
   
    int theLevel;
    if (_drawNestedEdges) // might not want edge nesting
      {
      int mask = 0;
      _delayDrawItr[task].first();
      while (!_delayDrawItr[task].isDone())
        {
        thisEvent = _delayDrawItr[task].getItem();
        mask |= 1 << thisEvent.parameter;
        _delayDrawItr[task].next();
        }
      theLevel = 0;
      while ( (mask & 1) && !(theLevel & 32))
        {
        mask = mask >> 1;
        theLevel++;
        }
      if (theLevel > 31)
        {
        theLevel = 31;
        }
      }
    else
      {
      theLevel = 0;
      }
    
    // check to see if the matching end has already arrived
    _missingProcBeginItr[task].first();
    while ( !_missingProcBeginItr[task].isDone())
      {
      thisEvent = _missingProcBeginItr[task].getItem();
      if (thisEvent.tag == idTag)
        {
        addTimeTunnelEdge(task,theTime,
                          task, thisEvent.time, color, theLevel);
        _missingProcBeginList[task].deleteNode(
	  _missingProcBeginItr[task].getNode());
        return;
        }
      _missingProcBeginItr[task].next();
      }
    // the matching end did not arrive out of order
    thisEvent.toTask = 
      addTimeTunnelEdgeNoDelete
	(task,theTime,task,theTime,color, theLevel);
    thisEvent.tag = idTag;
    thisEvent.parameter = theLevel;
    _delayDrawList[task].insertAtEnd(thisEvent);
    }
  }

void TimeTunnelLayout::processDurationEnd(int task,
                                          int idTag,
                                          float theTime,
                                          int)
  {
  VrQueuedTTEvent thisEvent;
  if (task<numTask)
    {
    _delayDrawItr[task].first();
    while (!_delayDrawItr[task].isDone())
      {
      thisEvent = _delayDrawItr[task].getItem();
      if (idTag == thisEvent.tag) // stop drawing edge
        {
        shadowSrcPos[thisEvent.toTask][2] = -theTime; // update time
        _delayDrawList[task].deleteNode( _delayDrawItr[task].getNode() );

	// need to insert this event into the appropriate deletion queue
	// note how we change the meaning of task
	MessageEvent theEvent;
	theEvent.task = thisEvent.toTask; 
	theEvent.time = theTime;
	deletionList[task].insertAtEnd(theEvent);
	return;
        }
      else
        _delayDrawItr[task].next();
      }

    // end has arrived before the matching begin
    thisEvent.time = theTime;
    thisEvent.tag = idTag;
    _missingProcBeginList[task].insertAtEnd( thisEvent );
    }
  }
