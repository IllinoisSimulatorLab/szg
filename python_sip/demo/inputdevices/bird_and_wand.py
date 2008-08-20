# A Windows-only Python input device that loads C++ dlls to talk to an Ascension Flock of Birds
#    tracker with Extended Range Controller and a gamepad or joystick.
# You would use it by:
# 1) Make sure you have defined SZG_PYTHON/executable for the computer it's on.
# 2) Place it in a directory on your SZG_PYTHON/path (or one level down).
# 3) Add it to a virtual computer input map, e.g.:
#        vcpyin  SZG_INPUT0  map  this_computer/treadmill.py
# 4) To use the Flock of birds, the Bird.dll must be in the same directory as the Python
#      executable or on your $PATH or in the current working directory.
#      Note that if you install the ascension diagnostic program
#      winbird.exe, it may install an incompatible dll in the Windows/System32 directory.
#      If this happens the incompatible dll will be found before any compatible Bird.dll
#      except for one in the python.exe directory. If an incompatible dll is loaded,
#      nothing will complain, you'll just get a stream of input
#      events with the default or zero values (that's Ascension's fault, not ours).
#

import sys
import time
import thread
import math
import random
import szg


# An input filter. The onInputEvent() method gets called for each
# event generated by the driver. This is the current filter for
# our CAVE input devices, and is equivalent to the following
# PForth code:
#
#          matrix Xin
#          matrix Xout
#          matrix C1
#          matrix C2
#          matrix originOffset
#          matrix headRotMatrix
#          matrix wandRotMatrix
#          matrix rtWandRotMatrix
#          matrix Ry
#          matrix Rz
#
#          1  0  0  0
#          0  0 -1  0
#          0  1  0  0
#          0  0  0  1
#          C1 matrixStoreTranspose
#          1  0  0  0
#          0  0  1  0
#          0 -1  0  0
#          0  0  0  1
#          C2 matrixStoreTranspose
#
#          -80 zaxis Rz rotationMatrix
#          20 yaxis Ry rotationMatrix
#          
#          Rz Ry headRotMatrix matrixMultiply
#          180 xaxis wandRotMatrix rotationMatrix
#          -90 yaxis Ry rotationMatrix
#          Ry wandRotMatrix wandRotMatrix matrixMultiply
#          180 xaxis rtWandRotMatrix rotationMatrix
#          Ry rtWandRotMatrix rtWandRotMatrix matrixMultiply
#
#          0.6 9.2 -4.5 originOffset translationMatrix
#
#          define filter_matrix_0
#            deleteCurrentEvent
#          enddef
#
#          define filter_matrix_1
#            Xin getCurrentEventMatrix
#            originOffset C1 Xin C2 headRotMatrix 5 Xout concatMatrices
#            Xout setCurrentEventMatrix
#            0 setCurrentEventIndex
#          enddef
#
#          define filter_matrix_2
#            Xin getCurrentEventMatrix
#            originOffset C1 Xin C2 wandRotMatrix 5 Xout concatMatrices
#            Xout setCurrentEventMatrix
#            1 setCurrentEventIndex
#          enddef
#
#          /* Joystick re-scaling. Also, axes 2 and 5 are re-mapped to be a
#          duplicate 0 and 1 (so that either joystick will work for
#          navigation). */
#          define filter_all_axes
#            getCurrentEventAxis 0.000031 * setCurrentEventAxis
#          enddef
#          define filter_axis_1
#            getCurrentEventAxis -1.0 * setCurrentEventAxis
#          enddef
#          define filter_axis_2
#            getCurrentEventAxis -1.0 * setCurrentEventAxis
#            1 setCurrentEventIndex
#          enddef
#          define filter_axis_3
#            0 setCurrentEventIndex
#          enddef 
#
#          /* Button remapping */
#          define filter_button_0
#            3 setCurrentEventIndex
#          enddef
#          define filter_button_1
#            0 setCurrentEventIndex
#          enddef
#          define filter_button_2
#            1 setCurrentEventIndex
#          enddef
#          define filter_button_3
#            4 setCurrentEventIndex
#          enddef
#          define filter_button_4
#            8 setCurrentEventIndex
#          enddef
#          define filter_button_5
#            9 setCurrentEventIndex
#          enddef
#          define filter_button_6
#            6 setCurrentEventIndex
#          enddef
#          define filter_button_7
#            7 setCurrentEventIndex
#          enddef
#          define filter_button_8
#            2 setCurrentEventIndex
#          enddef
#          define filter_button_9
#            5 setCurrentEventIndex
#          enddef
#          define filter_button_12
#            10 setCurrentEventIndex
#          enddef
#          define filter_button_13
#            11 setCurrentEventIndex
#          enddef

class MyFilter( szg.arPyIOFilter ):
  buttonMap = (3,0,1,4,8,9,6,7,2,5,10,11,10,11)
  def __init__( self ):
    szg.arPyIOFilter.__init__( self )

    axisSwap = szg.arMatrix4( 
          1, 0, 0, 0, \
          0, 0,-1, 0, \
          0, 1, 0, 0, \
          0, 0, 0, 1 )
    inverseAxisSwap = szg.arMatrix4( 
          1, 0, 0, 0, \
          0, 0, 1, 0, \
          0,-1, 0, 0, \
          0, 0, 0, 1 )

    headRotMatrix = szg.ar_rotationMatrix( 'z', math.radians(-80.) ) * \
        szg.ar_rotationMatrix( 'y', math.radians( 20. ) )
    wandRotMatrix = szg.ar_rotationMatrix( 'y', math.radians(-90.) ) * \
        szg.ar_rotationMatrix( 'x', math.radians( 180. ) )

    originOffset = szg.ar_translationMatrix( .6, 9.2, -4.5 )

    self.headPreMatrix = originOffset * axisSwap
    self.headPostMatrix = inverseAxisSwap * headRotMatrix

    self.wandPreMatrix = originOffset * axisSwap
    self.wandPostMatrix = inverseAxisSwap * wandRotMatrix

  def onButtonEvent( self, event, index ):
    try:
      event.setIndex( self.buttonMap[index] )
    except IndexError:
      print 'WARNING: button index (',index,') out of range.'

  def onAxisEvent( self, event, index ):
    value = event.getAxis() * .000031
    if index == 1 or index == 2:
      value = -value
    event.setAxis( value )
    if index == 2 or index == 3:
      index = 3-index
      event.setIndex( index )
 
  def onMatrixEvent( self, event, index ):
    if index == 0:
      event.trash()
    elif index == 1:
      event.setMatrix( self.headPreMatrix * event.getMatrix() * self.headPostMatrix )
      event.setIndex( 0 )
    elif index == 2:
      event.setMatrix( self.wandPreMatrix * event.getMatrix() * self.wandPostMatrix )
      event.setIndex( 1 )



class MyDriverFramework( szg.arPyDeviceServerFramework ):
  # Called from arPyDeviceServerFramework.init()
  # Create/load any input drivers and filters and install them in the
  # arInputNode.
  def configureInputNode(self):
    execPath = self.getSZGClient().getAttribute( 'SZG_EXEC', 'path' )

    # Load the two input drivers and install them in the arInputNode.
    # Note that they will be configured from the Syzygy database
    # inside the arPyDeviceServerFramework.init() after this method
    # returns.
    sharedLib = szg.arSharedLibInputDriver()
    status, errMsg = sharedLib.createFactory( 'arBirdWinDriver', execPath )
    if not status:
      raise RuntimeError, errMsg
    self.birdWinDriver = sharedLib.createObject()
    # add...Mine() means you still own the installed object and nobody
    # else should try to delete it.
    self.getInputNode().addInputSourceMine( self.birdWinDriver )

    sharedLib = szg.arSharedLibInputDriver()
    status, errMsg = sharedLib.createFactory( 'arJoystickDriver', execPath )
    if not status:
      raise RuntimeError, errMsg
    self.gamepadDriver = sharedLib.createObject()
    self.getInputNode().addInputSourceMine( self.gamepadDriver )

    self.filter = MyFilter()
    self.getInputNode().addFilterMine( self.filter )

    # Look for another daisy-chained (via the network) input device
    return self.addNetInput()



if __name__=='__main__':
  app = MyDriverFramework()
  if not app.init( sys.argv ):
    print sys.argv[0], "failed to init framework."
    app.getSZGClient().failStandalone(False)
    sys.exit(1)
  app.messageLoop()
