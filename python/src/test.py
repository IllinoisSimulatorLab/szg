#!/usr/bin/python

# $Id: test.py,v 1.1 2005/03/18 20:13:01 crowell Exp $
# (c) 2004, Brett Witt, Peter Brinkmann
#
# Testing suite for Python bindings for Syzygy, still incomplete.
#
# TODO:
#    - add unit tests for arQuaternion, arMatrix4, assorted functions
#      in PyMath.i
#    - smoke test for graphics and interaction --- launch demos, such
#      as hello.py and ribbons.py, and have user determine whether they
#      worked correctly
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


import unittest
from Vector3Test import *
from Vector4Test import *

def TestSuite():
   testSuite = unittest.TestSuite()
   testSuite.addTest(Vector3Test)
   testSuite.addTest(Vector4Test)
   return suite

if __name__ == '__main__':
   unittest.main()
