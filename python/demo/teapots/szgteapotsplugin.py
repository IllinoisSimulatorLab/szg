# This version of the program illustrates an alternative method
# for getting lots of teapots on the screen. It uses the
# arTeapotGraphicsPlugin class (defined in PySZG.py, a sub-class
# of arPythonGraphicsPluginObject) to load and render the
# arTeapotGraphicsPlugin (C++) shared library.

from PySZG import *
from OpenGL.GLUT import *
from OpenGL.GL import *
from OpenGL.GLU import *
import sys
import random

import szgteapots

# Our application framework class

class TeapotPluginApp(szgteapots.TeapotApp):
  def __init__(self):
    # Call the base class's constructor
    szgteapots.TeapotApp.__init__(self)
    # arTeapotGraphicsPlugin is defined in PySZG.py. It's a python
    # wrapper class that loads and renders the shared library/plugin
    # of the same name. It has a single relevant attribute, color,
    # which must be a 3- or 4-element _list_ of floats (ints will
    # _not_ work).
    self.teapotPlugin = arTeapotGraphicsPlugin()

  def onWindowStartGL( self, winInfo ):
    pass

  # equivalent to draw callback.
  # Called once/frame after onPostExchange().
  def onDraw( self, win, viewport ):
    # Load the navigation (driving around) matrix.
    self.loadNavMatrix()
    # draw the teapots
    for t in self.teapotData:
      # The '*t' means 'take the elements of the list t and distribute
      # them to the corresponding arguments of renderTeapot()'.
      self.renderTeapot( win, viewport, *t ) 

  def renderTeapot(self, graphicsWin, viewport, x, y, ambr, ambg, ambb, difr, difg, 
                        difb, specr, specg, specb, shine):
     self.teapotPlugin.color = [float(difr),float(difg),float(difb),1.]
     glPushMatrix()
     glTranslatef(x-8,y-5,-10)
     self.teapotPlugin.draw( graphicsWin, viewport )
     glPopMatrix()

# End of application class definition


# Python magic meaning, 'only do this if this file is run,
# not if it's imported as a module'. Roughly equivalent to main() in a
# C++ program.
if __name__ == '__main__':
  app = TeapotPluginApp()
  if not app.init(sys.argv):
    raise PySZGException,'Unable to init application.'
  # Never returns unless something goes wrong
  if not app.start():
    raise PySZGException,'Unable to start application.'

