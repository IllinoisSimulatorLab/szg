#!/usr/bin/python

# $Id: Vector4Test.py,v 1.1 2005/03/18 20:13:01 crowell Exp $
# (c) 2004, Brett Witt
#
# Unit tests for arVector4
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

import unittest
import operator
from random import *
from PySZG import *

class Vector4Test(unittest.TestCase):
   def testSetFunction(self):
      # Initialize our default values
      a = [randrange(1, 10000), randrange(1, 10000), 
           randrange(1, 10000), randrange(1, 10000)]
      a = arVector4(a[0], a[1], a[2], a[3])
      b = arVector4(0, 0, 0, 0)
      b.set(a[0], a[1], a[2], a[3])

      self.failUnless(a[0]==b[0] and a[1]==b[1] and a[2]==b[2] and a[3]==b[3])

   def testVectorOverflow(self):
      # Test for overflow on the constructor
      self.assertRaises(OverflowError, arVector4, 1000 * 10 ** 100, 0, 0, 0)
      self.assertRaises(OverflowError, arVector4, 1000 * 10 ** 1000, 0, 0, 0)
      self.assertRaises(OverflowError, arVector4, 1000 * 10 ** 10000, 0, 0, 0)

   def testVectorSetOverflow(self):
      a = arVector4(0, 0, 0, 0)
      
      # Test for overflow on the set function
      try:
         a.set(2 ** 128, 2 ** 128, 2 ** 128, 2 ** 128)
      except:
         pass
      else:
         self.fail("Overflow expected on set operation!")

      try:
         a.set(-(2 ** 128), -(2 ** 128), -(2 ** 128), -(2 ** 128))
      except:
         pass
      else:
         self.fail("Overflow expected on set operation!")

   def testVectorNormalizeOverflow(self):
      # Test for overflow on the normalize function
      checkValue = arVector4(1, 1, 1, 1)
      checkValue.normalize()

      a = arVector4(2 ** 127, 2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector4(-(2 ** 127), -(2 ** 127), -(2 ** 127), -(2 ** 127))

      try:
         a.normalize()
      except:
         self.fail("Unexpected overflow exception detected on normalize operation!")

      try:
         b.normalize()
      except:
         self.fail("Unexpected overflow exception detected on normalize operation!")

   def testVectorScalarMultiplyOverflow(self):
      a = arVector4(2 ** 127, 2 ** 127, 2 ** 127, 2 ** 127)

      try:
         a *= 2
      except:
         pass
      else:
         self.fail("Overflow expected on *= operator!")

      try:
         a *= -2
      except:
         pass
      else:
         self.fail("Overflow expected on *= operator!")

   def testVectorScalarDivideOverflow(self):
      a = arVector4(2 ** 127, 2 ** 127, 2 ** 127, 2 ** 127)

      try:
         a /= 0.5
      except:
         pass
      else:
         self.fail("Overflow expected on /= operator!")

      try:
         a /= -0.5
      except:
         pass
      else:
         self.fail("Overflow expected on /= operator!")

      try:
         a /= 0
      except:
         pass
      else:
         self.fail("Divide by zero error expected on /= operator!")

   def testVectorMagnitudeOverflow(self):
      a = arVector4(2 ** 127, 2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector4(-(2 ** 127), -(2 ** 127), -(2 ** 127), -(2 ** 127))

      try:
         a.__mag__()
      except:
         pass
      else:
         self.fail("Overflow expected on vector magnitude operator!")

      try:
         b.__mag__()
      except:
         pass
      else:
         self.fail("Overflow expected on vector magnitude operator!")

   def testVectorSetItemOverflow(self):
      a = arVector4(0, 0, 0, 0)

      try:
         a[0] = 2 ** 128
      except:
         pass
      else:
         self.fail("Overflow expected on vector set item operator!")

      try:
         a[1] = 2 ** 128
      except:
         pass
      else:
         self.fail("Overflow expected on vector set item operator!")

      try:
         a[2] = 2 ** 128
      except:
         pass
      else:
         self.fail("Overflow expected on vector set item operator!")

   # Private Member Variables and Functions
   def BinaryOperator(self, op, a, b):
      if(op == operator.add): return map((lambda x, y: x + y), a, b)
      if(op == operator.sub): return map((lambda x, y: x - y), a, b)
      if(op == operator.mul):
         return [a[1] * b[2] - a[2] * b[1],
                 a[2] * b[0] - a[0] * b[2],
                 a[0] * b[1] - a[1] * b[0]]

   def UnaryOperator(self, op, a): 
      if(op == operator.mod): 
         return reduce((lambda x, y: x + y), map((lambda x, y: x * y), a, b))

   def CheckEquality(self, szgVec, stdVec):
      for i in range(len(stdVec)):
         self.assertEqual(szgVec[i], stdVec[i])

   def CheckVectorOverflow(self, a, b, op, str):
      try:
         op(a, b)
      except OverflowError:
         pass
      else:
         self.fail(str)

if __name__ == '__main__':
   unittest.main()
