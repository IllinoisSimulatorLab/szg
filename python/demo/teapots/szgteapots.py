'''
teapots.py
Converted to Python by Jason Petrone 6/00

/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

 Ported to Syzygy 4/24/06 Jim Crowell, with the added ability to
 randomly shuffle the teapots.
'''

#  teapots.c
#  This program demonstrates lots of material properties.
#  A single light source illuminates the objects.

from PySZG import *
from OpenGL.GLUT import *
from OpenGL.GL import *
from OpenGL.GLU import *
import sys
import random


# This is one of the great things about Python. teapots.py is a program,
# but we can also treat it as a module, import it, and call some of its
# functions (in this case, init() and renderTeapot()).
import teapots


# In teapots.py the display() function makes a whole bunch of calls
# to renderTeapot() with different arguments.

#  First column:  emerald, jade, obsidian, pearl, ruby, turquoise
#  2nd column:  brass, bronze, chrome, copper, gold, silver
#  3rd column:  black, cyan, green, red, white, yellow plastic
#  4th column:  black, cyan, green, red, white, yellow rubber

# OK, instead of using the dumb original display() function to render all the teapots,
# we're going to put their properties into a list of lists, allowing
# manipulation and sharing master->slaves.
# Each sub-list is just the set of arguments for one call to
# renderTeapot() from the original program, e.g.:
#
#   renderTeapot(2.0, 17.0, 0.0215, 0.1745, 0.0215,
#      0.07568, 0.61424, 0.07568, 0.633, 0.727811, 0.633, 0.6)

teapotDataGlobal = [
   [2.0, 17.0, 0.0215, 0.1745, 0.0215,
      0.07568, 0.61424, 0.07568, 0.633, 0.727811, 0.633, 0.6],
   [2.0, 14.0, 0.135, 0.2225, 0.1575,
      0.54, 0.89, 0.63, 0.316228, 0.316228, 0.316228, 0.1],
   [2.0, 11.0, 0.05375, 0.05, 0.06625,
      0.18275, 0.17, 0.22525, 0.332741, 0.328634, 0.346435, 0.3],
   [2.0, 8.0, 0.25, 0.20725, 0.20725,
      1, 0.829, 0.829, 0.296648, 0.296648, 0.296648, 0.088],
   [2.0, 5.0, 0.1745, 0.01175, 0.01175,
      0.61424, 0.04136, 0.04136, 0.727811, 0.626959, 0.626959, 0.6],
   [2.0, 2.0, 0.1, 0.18725, 0.1745,
      0.396, 0.74151, 0.69102, 0.297254, 0.30829, 0.306678, 0.1],
   [6.0, 17.0, 0.329412, 0.223529, 0.027451,
      0.780392, 0.568627, 0.113725, 0.992157, 0.941176, 0.807843,
      0.21794872],
   [6.0, 14.0, 0.2125, 0.1275, 0.054,
      0.714, 0.4284, 0.18144, 0.393548, 0.271906, 0.166721, 0.2],
   [6.0, 11.0, 0.25, 0.25, 0.25,
      0.4, 0.4, 0.4, 0.774597, 0.774597, 0.774597, 0.6],
   [6.0, 8.0, 0.19125, 0.0735, 0.0225,
      0.7038, 0.27048, 0.0828, 0.256777, 0.137622, 0.086014, 0.1],
   [6.0, 5.0, 0.24725, 0.1995, 0.0745,
      0.75164, 0.60648, 0.22648, 0.628281, 0.555802, 0.366065, 0.4],
   [6.0, 2.0, 0.19225, 0.19225, 0.19225,
      0.50754, 0.50754, 0.50754, 0.508273, 0.508273, 0.508273, 0.4],
   [10.0, 17.0, 0.0, 0.0, 0.0, 0.01, 0.01, 0.01,
      0.50, 0.50, 0.50, .25],
   [10.0, 14.0, 0.0, 0.1, 0.06, 0.0, 0.50980392, 0.50980392,
      0.50196078, 0.50196078, 0.50196078, .25],
   [10.0, 11.0, 0.0, 0.0, 0.0,
      0.1, 0.35, 0.1, 0.45, 0.55, 0.45, .25],
   [10.0, 8.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0,
      0.7, 0.6, 0.6, .25],
   [10.0, 5.0, 0.0, 0.0, 0.0, 0.55, 0.55, 0.55,
      0.70, 0.70, 0.70, .25],
   [10.0, 2.0, 0.0, 0.0, 0.0, 0.5, 0.5, 0.0,
      0.60, 0.60, 0.50, .25],
   [14.0, 17.0, 0.02, 0.02, 0.02, 0.01, 0.01, 0.01,
      0.4, 0.4, 0.4, .078125],
   [14.0, 14.0, 0.0, 0.05, 0.05, 0.4, 0.5, 0.5,
      0.04, 0.7, 0.7, .078125],
   [14.0, 11.0, 0.0, 0.05, 0.0, 0.4, 0.5, 0.4,
      0.04, 0.7, 0.04, .078125],
   [14.0, 8.0, 0.05, 0.0, 0.0, 0.5, 0.4, 0.4,
      0.7, 0.04, 0.04, .078125],
   [14.0, 5.0, 0.05, 0.05, 0.05, 0.5, 0.5, 0.5,
      0.7, 0.7, 0.7, .078125],
   [14.0, 2.0, 0.05, 0.05, 0.0, 0.5, 0.5, 0.4,
      0.7, 0.7, 0.04, .078125]
  ]

# end of data copied from teapots.py



# Our application framework class

class TeapotApp(arPyMasterSlaveFramework):
  def __init__(self):
    # Call the base class's constructor
    arPyMasterSlaveFramework.__init__(self)
    # Not really necessary, I'm just prejudiced against global
    # variables. Since it's Python, the new variable is just a
    # reference to the same data anyway...
    self.teapotData = teapotDataGlobal

  # Equivalent to start callback
  # Called once from inside the application framework's start() method.
  # Used for application-wide initialization.
  def onStart( self, client ):
    # Necessary to call base class method for interactive prompt
    # functionality to work (run this program with '--prompt').
    arPyMasterSlaveFramework.onStart( self, client )
    # Tell the app we're going to be transferring a
    # sequence of data (tuple, list, etc.) to the slaves.
    # Can contain ints, floats, strings, and other
    # sequences ONLY.
    self.initSequenceTransfer('teapots')

  # Equivalent to windowStartGL callback
  # Called once right after a graphics window is created,
  # for OpenGL initialization. Note that it is possible for
  # this method to be called more than once (because
  # Syzygy applications can have multiple windows, although
  # it's not common).
  def onWindowStartGL( self, winInfo ):
    teapots.init() # call init() from teapots.py

  # equivalent to preExchange callback
  # Called on the master only, once/frame, before master->slave data exchange.
  def onPreExchange( self ):
    # Necessary to call base class method for interactive prompt
    # functionality to work (run this program with '--prompt').
    arPyMasterSlaveFramework.onPreExchange(self)
    # Allow driving around with the joystick
    self.navUpdate()
    # If user presses button #0, shuffle the teapots
    if self.getOnButton(0):
      self.shufflePositions()
    # Pack the (possibly shuffled) teapot data into the app
    # for transfer from master to slaves.
    self.setSequence( 'teapots', self.teapotData )

  # equivalent to postExchange callback
  # Called once/frame on master and slaves after data exchange.
  def onPostExchange( self ):
    # Get the teapot data from the master
    # You might think that pulling the data back out of the
    # app on the master would help with debugging of data
    # transmission. However, the data sequences always come
    # back out as tuples. So we could do that, but then
    # we'd have to manually convert the tuple of tuples back to a
    # list of lists or the shufflePositions() function
    # would bomb (because tuples are immutable).
    if not self.getMaster():
      self.teapotData = self.getSequence( 'teapots' )

  # equivalent to draw callback.
  # Called once/frame after onPostExchange().
  def onDraw( self, win, viewport ):
    # Load the navigation (driving around) matrix.
    self.loadNavMatrix()
    # Center the teapots (we're using a very different
    # projection matrix from the original teapots.py,
    # so without this they're way off-center).
    glTranslatef(-8,-5,-10)
    # draw the teapots
    for t in self.teapotData:
      # The '*t' means 'take the elements of the list t and distribute
      # them to the corresponding arguments of renderTeapot()'.
      teapots.renderTeapot( *t ) 

  # Add a little function to shuffle the teapot positions
  def shufflePositions( self ):
    # construct new list in in which element is [x,y] of a teapot
    pos = [[i[0],i[1]] for i in self.teapotData]
    random.shuffle( pos )
    # put shuffled [x,y]s back into original list
    for i in range(len(pos)):
      self.teapotData[i][0:2] = pos[i]



# End of application class definition


# Python magic meaning, 'only do this if this file is run,
# not if it's imported as a module'. Roughly equivalent to main() in a
# C++ program.
if __name__ == '__main__':
  app = TeapotApp()
  if not app.init(sys.argv):
    raise PySZGException,'Unable to init application.'
  # Never returns unless something goes wrong
  if not app.start():
    raise PySZGException,'Unable to start application.'

