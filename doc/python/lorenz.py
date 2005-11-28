from PySZG import *
import sys
import time
import random

class particle:
	def __init__(self):
		self.dt = 0.004
		self.x = arVector3(8,8,23)
		self.mat = arMaterial()
		self.mat.diffuse = arVector3(1,0,0)
		self.mat.specular = arVector3(0.2, 0.2, 0.2)
		self.mat.emissive = arVector3(0,0,0.1)
		self.mat.exponent = 100
		self.trans = None
	def attach(self, n):
		m = n.new("material")
		m.set(self.mat)
		self.trans = m.new("transform")
		self.trans.set(ar_TM(self.x))
		scl = self.trans.new("transform")
		scl.set(ar_SM(2))
		s = arCubeMesh()
		s.attachMesh(scl)
		#s = arSphereMesh()
		#s.setAttributes(15)
		#s.attachMesh(scl)
	def advance(self):
		x = self.x[0]
		y = self.x[1]
		z = self.x[2]
		dt = 0.004
		for i in range(5):
			x = (10*y - 10*x)*dt + x
			y = (28*x - y - x*z)*dt + y
			z = (-(2.66667)*z + x*y)*dt + z
       	 		#self.x[0] = (10*self.x[1]-10*self.x[0])*self.dt + self.x[0]
			#self.x[1] = (28*self.x[0]-self.x[1] - self.x[0]*self.x[2])*self.dt + self.x[1]
			#self.x[2] = (-(2.66667)*self.x[2] + self.x[0]*self.x[1])*self.dt + self.x[2]
		self.x[0] = x
		self.x[1] = y
		self.x[2] = z
		self.trans.set(ar_TM(self.x))

def addLights(g):
	r = g.getRoot()
	l = gcast(r.new("light"))
	light = arLight()
	light.lightID = 0
	light.position = arVector4(0,0,-1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.setLight(light)
	l = gcast(r.new("light"))
	light = arLight()
	light.lightID = 1
	light.position = arVector4(0,0,1,0)
        light.ambient = arVector3(0,0,0)
        light.diffuse = arVector3(0.5, 0.5, 0.5)
	l.setLight(light)
	
if len(sys.argv) > 1:
	number = int(sys.argv[1])
else:
	number = 80
f = arDistSceneGraphFramework()
f.setAutoBufferSwap(0)
if f.init(sys.argv) != 1:
	sys.exit()
if f.start() != 1:
	sys.exit()

f.setNavTransSpeed(0.2)
g = f.getDatabase()
addLights(g)
r = f.getNavNode()
w = gcast(r.new("transform"))
w.set(ar_TM(0,5,-5)*ar_SM(0.1))
plist = []
for i in range(number):
	p = particle()
	p.x[2] = p.x[2] + i*0.03
	p.mat.diffuse = arVector3(1 - (i+1)/(1.0*(number)), 0, (i+1)/(1.0*(number)))
	p.attach(w)
	plist.append(p)

while 1:
	f.setViewer()
	f.navUpdate()
        f.loadNavMatrix()
	for i in plist:
		i.advance()
	f.swapBuffers()
