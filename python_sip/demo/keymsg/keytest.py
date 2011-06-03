#!/usr/bin/python
from szg import *
from OpenGL.GL import *
from OpenGL.GLU import *
import sys
import os

firstFrame = True

#####  The application framework ####
#
class KeyTestFramework(arPyMasterSlaveFramework):
  #### Framework callbacks -- see szg/src/framework/arMasterSlaveFramework.h ####
  def onKey( self, key, state, ctrl, alt ):
    print 'Key: %s, state: %d,  ctrl: %d, alt: %s' \
        % (chr(key),state,ctrl,alt)

if __name__ == '__main__':
  szgrun( appClass=KeyTestFramework )
