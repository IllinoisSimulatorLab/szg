# A Python input device. You would use it by:
# 1) placing it in a directory on your SZG_PYTHON/path (or one level down), and
# 2) adding it to a virtual computer input map, e.g.:
#        vcpyin  SZG_INPUT0  map  this_computer/randombuttons.py
#
# It's not a very _useful_ input device, it sets a fixed head and wand position
# and presses and releases a random button every second. You could, however,
# use this framework to generate input by e.g. talking to a hardware device
# through a serial port. Or you could write your own input simulator using
# PyOpenGL or wxPython.
#

import sys
import time
import thread
import math
import random
import szg


# An input filter. The onInputEvent() method gets called for each
# event generated by the driver. This filter just looks for button
# events and replaces the (already random) event index with a
# new random event index. Note that it assumes that the button
# release event comes immediately after the corresponding button-press
# event in the input stream.
class MyFilter( szg.arPyIOFilter ):
  def onButtonEvent( self, event, index ):
    szg.ar_log_warning().write( 'Before filter: ' + str(event) + '\n' )
    # Other get...() methods: getAxis(), getMatrix().
    if event.getButton() == 1:
      self.lastIndex = random.choice( range(8) )
      # You could also call event.setButton( value ) to change the value.
      # Or event.trash() to remove it from the input stream.
      # Or create a new, additional event (e.g. newEvent = arAxisEvent( 0, 0.5 ))
      # and insert it into the stream using self.insertNewEvent( newEvent )
      event.setIndex( self.lastIndex )
    else:
      event.setIndex( self.lastIndex )
    szg.ar_log_warning().write( 'After  filter: ' + str(event) + '\n' )
    szg.ar_log_warning().write( '------------------------------------------\n' )


class MyDriverFramework( szg.arPyDeviceServerFramework ):
  def configureInputNode(self):
    self.driver = szg.arGenericDriver()
    # signature = # buttons, # axes, # matrices
    self.driver.setSignature( 8,0,2 )
    # add...Mine() means you still own the installed object and nobody
    # else should try to delete it.
    self.getInputNode().addInputSourceMine( self.driver )
    self.filter = MyFilter()
    self.getInputNode().addFilterMine( self.filter )
    # Look for another daisy-chained (via the network) input device, if
    # specified by the virtual computer definition.
    return self.addNetInput()
  def start(self):
    thread.start_new_thread( self.driverFunc, () )
  def driverFunc(self):
    startTime = time.clock()
    while True:
      time.sleep( 1. )
      buttonIndex = random.choice( range(8) )
      # the send...() and queue...() methods all have the same signature
      # as the arButtonEvent(), arAxisEvent(), arMatrixEvent() constructors:
      # ( event index, event value ).
      self.driver.sendButton( buttonIndex, 1 )
      time.sleep( .1 )
      self.driver.queueButton( buttonIndex, 0 )
      self.driver.queueMatrix( 0, szg.ar_translationMatrix(0,5.5,0) )
      self.driver.queueMatrix( 1, szg.ar_translationMatrix(1,3.5,-1) )
      self.driver.sendQueue()



if __name__=='__main__':
  app = MyDriverFramework()
  if not app.init( sys.argv ):
    print sys.argv[0], "failed to init framework."
    app.getSZGClient().failStandalone(False)
    sys.exit(1)
  app.start()
  app.messageLoop()
