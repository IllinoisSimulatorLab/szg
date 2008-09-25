# A Python input device. You would use it by:
# 1) Make sure you have defined SZG_PYTHON/executable for the computer it's on.
# 2) Place it in a directory on your SZG_PYTHON/path (or one level down).
# 3) Add it to a virtual computer input map, e.g.:
#        vcpyin  SZG_INPUT0  map  this_computer/event_console.py
#
# Allows use to send input events from a prompt. Note that if you launch
# an application on the virtual computer using 'dex vcpyin <app>', this
# program will run in the szgd window. 
#
# Known bugs: You may have to hit <enter> a couple
# of times before things 'take'. Hangs for several seconds on exit (permanently
# if szgserver goes away).
#

import sys
import time
import thread
import math
import szg


class MyDriverFramework( szg.arPyDeviceServerFramework ):
  def __init__( self ):
    szg.arPyDeviceServerFramework.__init__(self)
    # get the remaining chars in names of all methods whose names begin with 'handle_'
    self.funcNames = [i[7:] for i in dir(self) if i[:7]=='handle_' and callable( getattr( self, i ) )]
    self.lastButtons = dict()

  def configureInputNode(self):
    self.driver = szg.arGenericDriver()
    # signature = # buttons, # axes, # matrices
    self.driver.setSignature( 8,4,3 )
    # add...Mine() means you still own the installed object and nobody
    # else should try to delete it.
    self.getInputNode().addInputSourceMine( self.driver )
    # Look for another daisy-chained (via the network) input device, if
    # specified by the virtual computer definition.
    return self.addNetInput()

  def start(self):
    thread.start_new_thread( self.driverFunc, () )

  def driverFunc(self):
    while True:
      inString = raw_input('event_console> ')
      inList = inString.split()
      if len(inList) == 0:
        continue
      command = inList[0]
      if command == 'help':
        self.print_help()
        continue
      if not command in self.funcNames:
        print "Unknown command '"+command+"'. Known commands are:"
        print '   ',', '.join( self.funcNames )
        print
        continue
      func = getattr( self, 'handle_'+command )
      try:
        index = int( inList[1] )
        func( index, *inList[2:] )
      except Exception, msg:
        print msg
        print func.__doc__
        print

  # A driver is only supposed to send button events if a button changes
  # state, i.e there shouldn't ever be successive events with the same
  # value (and the same index). That's what these three methods prevent.
  def getLastButton( self, index ):
    try:
      return self.lastButtons[index]
    except KeyError:
      return 0
  def setLastButton( self, index, value ):
    self.lastButtons[index] = value
  def doButton( self, index, value ):
    if self.getLastButton( index ) != value:
      self.driver.sendButton( index, value )
    self.setLastButton( index, value )

  # Command handlers.

  def print_help( self ):
    for n in self.funcNames:
      f = getattr( self, 'handle_'+n )
      print f.__doc__
      print

  def handle_button( self, index, *args ):
    """'button' sets the state of a button to 1 (pressed) or 0 (unpressed).
    usage: button <index> <int>"""
    if len(args) != 1:
      raise EventConsoleError
    self.doButton( index, int(args[0]) )

  def handle_click( self, index, *args ):
    """'click' presses and releases a button.
    usage: click <index>"""
    if len(args) != 0:
      raise EventConsoleError
    self.doButton( index, 1 )
    time.sleep(.1)
    self.doButton( index, 0 )

  def handle_press( self, index, *args ):
    """'press' sets the state of a button to 1.
    usage: press <index>"""
    if len(args) != 0:
      raise EventConsoleError
    self.doButton( index, 1 )

  def handle_release( self, index, *args ):
    """'release' sets a button to 0.
    usage: release <index>"""
    if len(args) != 0:
      raise EventConsoleError
    self.doButton( index, 0 )

  def handle_axis( self, index, *args ):
    """'axis' sends an axis (joystick) event.
    usage: axis <index> <float>"""
    if len(args) != 1:
      raise EventConsoleError
    self.driver.sendAxis( index, float(args[0]) )

  def handle_tmatrix( self, index, *args ):
    """'tmatrix' sends a placement matrix event for a specified translation (position).
    usage: tmatrix <index> <3 floats(x,y,z)>"""
    if len(args) != 3:
      raise EventConsoleError
    floats = map( float, args )
    self.driver.sendMatrix( index, szg.ar_translationMatrix( *floats ) )

  def handle_rmatrix( self, index, *args ):
    """'rmatrix' sends a pure rotation matrix.
    usage: either rmatrix <index> <char(x,y,or z)> <float(degrees)>,
               or rmatrix <index> <4 floats(xaxis,yaxis,zaxis,degrees)>"""
    if len(args) != 2 and len(args) != 4:
      raise EventConsoleError
    if len(args) == 2:
      axes = {'x':szg.arVector3(1,0,0), 'y':szg.arVector3(0,1,0), 'z':szg.arVector3(0,0,1) }
      rotAxis = axes[ args[0] ]
      rotAngle = math.radians( float( args[1] ) )
    else:
      floats = map( float, args )
      rotAxis = szg.arVector3( floats[:3] )
      rotAngle = math.radians( floats[-1] )
    self.driver.sendMatrix( index, szg.ar_rotationMatrix( rotAxis, rotAngle ) )

  def handle_matrix( self, index, *args ):
    """'matrix' sends a placement (translation+rotation) matrix event.
    usage 1: matrix <index> <3 floats(x,y,z)> <char(x,y,or z)> <float(degrees)> or
    usage 2: matrix <index> <3 floats(x,y,z)> <4 floats(xaxis,yaxis,zaxis,degrees)>"""
    if len(args) != 5 and len(args) != 7:
      raise EventConsoleError
    tMatrix = szg.ar_translationMatrix( *map( float, args[:3] ) )
    args = args[3:]
    if len(args) == 2:
      axes = {'x':szg.arVector3(1,0,0), 'y':szg.arVector3(0,1,0), 'z':szg.arVector3(0,0,1) }
      rotAxis = axes[ args[0] ]
      rotAngle = math.radians( float( args[1] ) )
    else:
      floats = map( float, args )
      print floats[:3], '/', floats[-1:]
      rotAxis = szg.arVector3( floats[:3] )
      rotAngle = math.radians( floats[-1] )
    print rotAxis, rotAngle
    self.driver.sendMatrix( index, tMatrix*szg.ar_rotationMatrix( rotAxis, rotAngle ) )



if __name__=='__main__':
  app = MyDriverFramework()
  if not app.init( sys.argv ):
    print sys.argv[0], "failed to init framework."
    app.getSZGClient().failStandalone(False)
    sys.exit(1)
  app.start()
  app.messageLoop()
