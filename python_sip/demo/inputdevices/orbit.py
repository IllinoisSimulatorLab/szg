# A Python input device that generates an orbit around the origin. Intended for
# use with framework-mediated navigation. You would use it by:
# 1) Make sure you have defined SZG_PYTHON/executable for the computer it's on.
# 2) Place it in a directory on your SZG_PYTHON/path (or one level down).
# 3) Add it to a virtual computer input map, e.g.:
#        vcpyin  SZG_INPUT0  map  this_computer/orbit.py
# 4) On the master computer for this virtual computer, set:
#        <master_computer> SZG_NAV use_nav_input_matrix true
# 5) Figure out what index the matrices generated by this script will have.
#    E.g., if this is the only input device, that would be 0, and you would set:
#        <master_computer> SZG_NAV nav_input_matrix_index 0
#    (note that in this case the same matrix will be used for navigation and head
#    position, which could be a bit confusing; as most applications assume that
#    matrix 0 is the head matrix and matrix 1 is a wand, you might want modify
#    this script to output fixed matrices for 0 and 1 and to put the orbit matrix
#    in 2.
#        <master_computer> SZG_NAV nav_input_matrix_index 2
#

import sys
import time
import math
import thread
import random
import szg

RADIUS = 10 # Feet, in real space. In other words, if your application uses
            # a unit conversion factor different from 1. (i.e. units other
            # than feet), then the translation component of this matrix
            # will be scaled appropriately.
ROTATION_RATE = 20 # deg/sec

class MyDriverFramework( szg.arPyDeviceServerFramework ):
  def configureInputNode(self):
    self.driver = szg.arGenericDriver()
    # signature = # buttons, # axes, # matrices
    self.driver.setSignature( 0,0,1 )
    # add...Mine() means you still own the installed object and nobody
    # else should try to delete it.
    self.getInputNode().addInputSourceMine( self.driver )
    # Look for another daisy-chained (via the network) input device.
    return self.addNetInput()
  def start(self):
    thread.start_new_thread( self.driverFunc, () )
  def driverFunc(self):
    startTime = time.clock()
    while True:
      time.sleep( .02 )
      angle = math.radians((time.clock()-startTime)*ROTATION_RATE)
      x = RADIUS*math.sin( angle )
      z = RADIUS*(math.cos( angle )-1)
      viewMatrix = szg.ar_translationMatrix(x, 0, z) * szg.ar_rotationMatrix( 'y', angle )
      self.driver.sendMatrix( viewMatrix )



if __name__=='__main__':
  app = MyDriverFramework()
  if not app.init( sys.argv ):
    print sys.argv[0], "failed to init framework."
    app.getSZGClient().failStandalone(False)
    sys.exit(1)
  app.start()
  app.messageLoop()
