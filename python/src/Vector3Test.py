#!/usr/bin/python

# $Id: Vector3Test.py,v 1.1 2005/03/18 20:13:01 crowell Exp $
# (c) 2004, Brett Witt
#
# Unit tests for arVector3
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

import unittest
import operator
from random import *
from PySZG import *

class Vector3Test(unittest.TestCase):
   def testVectorSanity(self): 
      binOps = [operator.add, operator.sub, operator.mul]

      for i in range(5): self.CheckVectorBinaryOps(-(10 ** i), 10 ** i, binOps)

   def testSetFunction(self):
      # Initialize our default values
      a = [randrange(1, 10000), randrange(1, 10000), randrange(1, 10000)]
      a = arVector3(a[0], a[1], a[2])
      b = arVector3(0, 0, 0)
      b.set(a[0], a[1], a[2])

      self.assertEqual(a, b)

   def testVectorOverflow(self):
      # Test for overflow on the constructor
      self.assertRaises(OverflowError, arVector3, 1000 * 10 ** 100, 0, 0)
      self.assertRaises(OverflowError, arVector3, 1000 * 10 ** 1000, 0, 0)
      self.assertRaises(OverflowError, arVector3, 1000 * 10 ** 10000, 0, 0)

   def testVectorAdditionOverflow(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      c = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))
      d = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))

      # Test for overflow on the addition operator
      self.CheckVectorOverflow(a, b, operator.add,
                               "Overflow expected on addition operator!")

      self.CheckVectorOverflow(c, d, operator.add,
                               "Overflow expected on addition operator!")

   def testVectorSubtractionOverflow(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))
      c = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))
      d = arVector3((2 ** 127), (2 ** 127), (2 ** 127))

      # Test for overflow on the subtraction operator
      self.CheckVectorOverflow(a, b, operator.sub,
                               "Overflow expected on subtraction operator!")
      self.CheckVectorOverflow(c, d, operator.sub,
                               "Overflow expected on subtraction operator!")


   def testVectorCrossProductOverflow(self):
      a = arVector3(2 ** 127, 0, 0)
      b = arVector3(0, 2, 0)

      # Test for overflow on the cross product operator
      self.CheckVectorOverflow(a, b, operator.mul,
                               "Overflow expected on cross product operator!")


   def testVectorDotProductOverflow(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      c = arVector3(2 ** 64, 2 ** 64, 2 ** 64)
      d = arVector3(2 ** 64, 2 ** 64, 2 ** 64)

      e = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))
      f = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))
      g = arVector3(-(2 ** 64), -(2 ** 64), -(2 ** 64))
      h = arVector3(-(2 ** 64), -(2 ** 64), -(2 ** 64))

      # Test for overflow on the dot product operator
      self.CheckVectorOverflow(a, b, operator.mod,
                               "Overflow expected on dot product operator!")
      
      self.CheckVectorOverflow(c, d, operator.mod,
                               "Overflow expected on dot product operator!")

      self.CheckVectorOverflow(e, f, operator.mod,
                               "Overflow expected on dot product operator!")

      self.CheckVectorOverflow(g, h, operator.mod,
                               "Overflow expected on dot product operator!")

      try:
         a.dot(b)
      except:
         pass
      else:
         self.fail("Overflow expected on dot product operation!")

   def testVectorSetOverflow(self):
      a = arVector3(0, 0, 0)
      
      # Test for overflow on the set function
      try:
         a.set(2 ** 128, 2 ** 128, 2 ** 128)
      except:
         pass
      else:
         self.fail("Overflow expected on set operation!")

      try:
         a.set(-(2 ** 128), -(2 ** 128), -(2 ** 128))
      except:
         pass
      else:
         self.fail("Overflow expected on set operation!")

   def testVectorNormalizeOverflow(self):
      # Test for overflow on the normalize function
      checkValue = arVector3(1, 1, 1)
      checkValue.normalize()

      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))

      try:
         a.normalize()
      except:
         self.fail("Unexpected overflow exception detected on normalize operation!")

      try:
         b.normalize()
      except:
         self.fail("Unexpected overflow exception detected on normalize operation!")

   def testVectorScalarMultiplyOverflow(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector3(-2 ** 127, -2 ** 127, -2 ** 127)

      try:
         u= a * 2
         u[0],u[1],u[2]
      except:
         pass
      else:
         self.fail("Overflow expected on * operator!")

      try:
         u=a * -2
         u[0],u[1],u[2]
      except:
         pass
      else:
         self.fail("Overflow expected on * operator!")

   def testVectorScalarDivideOverflow(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)

      try:
         u=a / 0.5
         u[0],u[1],u[2]
      except:
         pass
      else:
         self.fail("Overflow expected on / operator!")

      try:
         u=a / (-0.5)
         u[0],u[1],u[2]
      except:
         pass
      else:
         self.fail("Overflow expected on / operator!")

      try:
         a/= 0
         a[0],a[1],a[2]
      except:
         pass
      else:
         self.fail("Divide by zero error expected on / operator!")
# Note: Ordinarily, we ought to test a/0 as well, but arMath.h handles
# division by zero in its own way

   def testVectorMagnitudeOverflow(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      b = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))

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

   def testVectorNegation(self):
      a = arVector3(2 ** 127, 2 ** 127, 2 ** 127)
      a = -a
      b = arVector3(-(2 ** 127), -(2 ** 127), -(2 ** 127))

      self.assertEqual(a, b)

   def testVectorSetItemOverflow(self):
      a = arVector3(0, 0, 0)

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
         a=szgVec[i]
         b=stdVec[i]
         self.assertAlmostEqual((a - b)/a, 0.0, 3)

   def assertAlmostEqual(self, a, b, places):
      diff = a - b
      if(diff > 10 ** places): 
         raise 'Epsilon exceeded.  a is not almost equal to b!'

   def CheckVectorBinaryOps(self, l, u, binOps):
      for i in range(1, 1000):
         a = [random()*(u-l)+l for ii in range(3)]
         b = [random()*(u-l)+l for ii in range(3)]
         ara = arVector3(a[0], a[1], a[2])
         arb = arVector3(b[0], b[1], b[2])

         op = binOps[randrange(0, 3)]

         testResult    = op(ara, arb)
         confirmResult = self.BinaryOperator(op, a, b)
         self.CheckEquality(testResult, confirmResult)

   def CheckVectorOverflow(self, a, b, op, str):
      try:
         u = op(a, b)
         u[0], u[1], u[2]
      except OverflowError:
         pass
      else:
         self.fail(str)

if __name__ == '__main__':
   unittest.main()
